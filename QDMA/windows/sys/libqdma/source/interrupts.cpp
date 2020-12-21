/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "qdma.h"
#include "interrupts.hpp"
#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "interrupts.tmh"
#endif

using namespace xlnx;

/* ------- Interrupt Queue ------- */
NTSTATUS intr_queue::create(
    WDFDMAENABLER& dma_enabler)
{
    auto buffer_size = size * sizeof(intr_entry);

    /* Align to page size */
    if (buffer_size % PAGE_SIZE) {
        buffer_size = ((buffer_size / PAGE_SIZE) + 1) * PAGE_SIZE;
    }

    TraceVerbose(TRACE_INTR, "%s: Intr Queue : %d, Buffer size : %llu, Ring size : %llu",
        qdma->dev_conf.name, idx_abs, buffer_size, size);

    capacity = buffer_size / sizeof(intr_entry);
    npages = buffer_size / PAGE_SIZE;
    color = 1;

    TraceVerbose(TRACE_INTR, "Intr Queue : %d, Page size : %llu, Capacity : %llu",
        idx_abs, npages, capacity);

    auto status = WdfCommonBufferCreate(dma_enabler,
                                        buffer_size,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &buffer);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "%s: Interrupt WdfCommonBufferCreate failed for "
            "intr queue : %d, : %!STATUS!", qdma->dev_conf.name, idx_abs, status);
        return status;
    }

    buffer_va = static_cast<void*>(WdfCommonBufferGetAlignedVirtualAddress(buffer));

    clear_contents();

    return status;
}

void intr_queue::clear_contents(void)
{
    auto buffer_size = size * sizeof(intr_entry);
    RtlZeroMemory(buffer_va, buffer_size);
    sw_index = 0;
    intr_cidx_info.rng_idx = 0;
    intr_cidx_info.sw_cidx = 0;
}

void intr_queue::advance_sw_index(void)
{
    ++sw_index;

    if (sw_index == capacity) {
        sw_index = 0;
        /* Flip the color */
        color = color ? 0 : 1;
    }
}

PFORCEINLINE void intr_queue::update_csr_cidx(
    queue_pair *q,
    UINT32 new_cidx)
{
    TraceVerbose(TRACE_INTR, "%s: Intr queue_%u updating c2h pidx to %u", qdma->dev_conf.name, idx, new_cidx);
    intr_cidx_info.rng_idx = (UINT8)idx_abs;
    intr_cidx_info.sw_cidx = (UINT16)new_cidx;

    qdma->hw.qdma_queue_intr_cidx_update(qdma, false /* is_vf */, q->idx, &intr_cidx_info);
}

NTSTATUS intr_queue::intring_dump(qdma_intr_ring_info *intring_info)
{
    size_t len;
    size_t buf_idx;
    size_t ring_entry_sz;
    UINT8 *ring_buff_addr;
    UINT8 *buf = intring_info->pbuffer;
    size_t buf_len = intring_info->buffer_len;

    if ((intring_info->start_idx >= size) ||
        (intring_info->end_idx >= size)) {
        TraceError(TRACE_INTR, "%s: Intr Ring index Range is incorrect : start : %d, end : %d, RING SIZE : %d",
            qdma->dev_conf.name, intring_info->start_idx, intring_info->end_idx, size);
        return STATUS_ACCESS_VIOLATION;
    }

    ring_entry_sz = sizeof(intr_entry);
    intring_info->ring_entry_sz = ring_entry_sz;
    if (intring_info->start_idx <= intring_info->end_idx) {
        len = ((size_t)intring_info->end_idx - (size_t)intring_info->start_idx + 1) * ring_entry_sz;
        len = min(len, buf_len);
        buf_idx = intring_info->start_idx * ring_entry_sz;
        ring_buff_addr = &((UINT8 *)buffer_va)[buf_idx];
        RtlCopyMemory(buf, ring_buff_addr, len);
        intring_info->ret_len = len;
    }
    else {
        len = (size - intring_info->start_idx) * ring_entry_sz;
        len = min(len, buf_len);
        buf_idx = intring_info->start_idx * ring_entry_sz;
        ring_buff_addr = &((UINT8 *)buffer_va)[buf_idx];
        RtlCopyMemory(buf, ring_buff_addr, len);
        intring_info->ret_len = len;

        size_t remain_len = ((size_t)intring_info->end_idx + 1) * ring_entry_sz;
        len = min(remain_len, (buf_len - len));
        ring_buff_addr = &((UINT8 *)buffer_va)[0];
        RtlCopyMemory(&buf[intring_info->ret_len], ring_buff_addr, len);
        intring_info->ret_len += len;
    }

    return STATUS_SUCCESS;
}

/* --------------*/

inline static size_t get_msix_mask_offset(
    UINT32 vector)
{
    return (qdma_trq_cmc_msix_table + (((size_t)vector + 1) * qdma_msix_vectors_mask_step));
};

void qdma_device::mask_msi_entry(
    UINT32 vector)
{
    /* MSIx mask value
     * [bit 0 ]   - 0 : Unmask, 1 : Mask
     * [bit 1-31] - Reserved
     */
    UINT32 mask_val = 1u;
    qdma_conf_reg_write(get_msix_mask_offset(vector), mask_val);
}

void qdma_device::unmask_msi_entry(
    UINT32 vector)
{
    /* MSIx mask value
     * [bit 0 ]   - 0 : Unmask, 1 : Mask
     * [bit 1-31] - Reserved
     */
    UINT32 mask_val = 0u;
    qdma_conf_reg_write(get_msix_mask_offset(vector), mask_val);
}

BOOLEAN EvtErrorInterruptIsr(
    WDFINTERRUPT interrupt,
    ULONG MessageID)
{
    UNREFERENCED_PARAMETER(interrupt);
    UNREFERENCED_PARAMETER(MessageID);

    return WdfInterruptQueueDpcForIsr(interrupt);
}

VOID EvtErrorInterruptDpc(
    WDFINTERRUPT interrupt,
    WDFOBJECT device)
{
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(interrupt);

    TraceError(TRACE_INTR, "Error IRQ Fired on Master PF");

    auto irq_ctx = get_qdma_irq_context(interrupt);
    if (nullptr == irq_ctx) {
        TraceError(TRACE_INTR, "Err: null irq_ctx in EvtErrorInterruptDpc");
        return;
    }

    auto qdma_dev = irq_ctx->qdma_dev;
    qdma_dev->hw.qdma_hw_error_process(qdma_dev);
    qdma_dev->hw.qdma_hw_error_intr_rearm(qdma_dev);

    return;
}

NTSTATUS EvtUserInterruptEnable(
    WDFINTERRUPT interrupt,
    WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(device);
    auto irq_ctx = get_qdma_irq_context(interrupt);
    if (irq_ctx->qdma_dev->drv_conf.user_interrupt_enable_handler) {
        irq_ctx->qdma_dev->drv_conf.user_interrupt_enable_handler(irq_ctx->vector_id,
            irq_ctx->qdma_dev->drv_conf.user_data);
    }

    TraceVerbose(TRACE_INTR, "%s: --> %s", irq_ctx->qdma_dev->dev_conf.name, __func__);
    return STATUS_SUCCESS;
}

NTSTATUS EvtUserInterruptDisable(
    WDFINTERRUPT interrupt,
    WDFDEVICE device)
{
    UNREFERENCED_PARAMETER(device);
    auto irq_ctx = get_qdma_irq_context(interrupt);
    if (irq_ctx->qdma_dev->drv_conf.user_interrupt_disable_handler) {
        irq_ctx->qdma_dev->drv_conf.user_interrupt_disable_handler(irq_ctx->vector_id,
            irq_ctx->qdma_dev->drv_conf.user_data);
    }

    TraceVerbose(TRACE_INTR, "%s: %s <--", irq_ctx->qdma_dev->dev_conf.name, __func__);
    return STATUS_SUCCESS;
}

BOOLEAN EvtUserInterruptIsr(
    WDFINTERRUPT interrupt,
    ULONG MessageID)
{
    UNREFERENCED_PARAMETER(interrupt);
    UNREFERENCED_PARAMETER(MessageID);

    TraceVerbose(TRACE_INTR, "User Interrupt ISR CALLED %lu SUCCESSFull", MessageID);

    return WdfInterruptQueueDpcForIsr(interrupt);
}

VOID EvtUserInterruptDpc(
    WDFINTERRUPT interrupt,
    WDFOBJECT device)
{
    UNREFERENCED_PARAMETER(device);
    auto irq_ctx = get_qdma_irq_context(interrupt);

    TraceVerbose(TRACE_INTR, "%s: User Interrupt DPC for vector : %u", irq_ctx->qdma_dev->dev_conf.name, irq_ctx->vector_id);

    if (irq_ctx->qdma_dev->drv_conf.user_isr_handler) {
        irq_ctx->qdma_dev->drv_conf.user_isr_handler(irq_ctx->vector_id,
            irq_ctx->qdma_dev->drv_conf.user_data);
    }

    return;
}

BOOLEAN EvtDataInterruptIsr(
    WDFINTERRUPT interrupt,
    ULONG MessageID)
{
    UNREFERENCED_PARAMETER(MessageID);
    auto irq_ctx = get_qdma_irq_context(interrupt);
    auto qdma_dev = irq_ctx->qdma_dev;

    if ((irq_ctx->intr_type == interrupt_type::LEGACY) &&
        (qdma_dev->hw.qdma_legacy_intr_conf != nullptr)) {
        TraceVerbose(TRACE_INTR, "LEGACY INTERRUPT RECEIVED");
        auto ret = qdma_dev->hw.qdma_is_legacy_intr_pend(qdma_dev);
        if (ret < 0) {
            /* No interrupt pending, Hence returning from ISR */
            return false;
        }

        qdma_dev->hw.qdma_clear_pend_legacy_intr(qdma_dev);
    }

    irq_ctx->interrupt_handler(irq_ctx);

    if ((irq_ctx->intr_type == interrupt_type::LEGACY) &&
        (qdma_dev->hw.qdma_legacy_intr_conf != nullptr)) {
        qdma_dev->hw.qdma_legacy_intr_conf(qdma_dev, ENABLE);
        TraceVerbose(TRACE_INTR, "LEGACY INTERRUPT RE-ENABLED");
    }

    return true;
}

BOOLEAN schedule_dpc(queue_pair* q, UINT8 is_c2h, CCHAR active_processors)
{
    BOOLEAN status = FALSE;
    poll_operation_entry* poll_entry = nullptr;
    PRKDPC dpc = nullptr;
    PVOID arg1 = nullptr;

    poll_entry = (is_c2h) ? q->c2h_q.poll_entry : q->h2c_q.poll_entry;
    if (poll_entry) {
        dpc = &poll_entry->thread->dpc;
        arg1 = poll_entry->thread;

        KeSetTargetProcessorDpc(dpc, (CCHAR)(q->idx % active_processors));
        status = KeInsertQueueDpc(dpc, arg1, NULL);
    }

    return status;
}

void cpm_handle_indirect_interrupt(
    PQDMA_IRQ_CONTEXT irq_ctx)
{
    queue_pair *q = nullptr;
    UINT8 is_c2h = 0;
    CCHAR active_processors = (CCHAR)KeQueryActiveProcessorCount(NULL);

    auto intr_queue = irq_ctx->intr_q;
    if (nullptr == intr_queue) {
        TraceError(TRACE_INTR, "%s: Invalid vector %lu was called in coal mode", 
            irq_ctx->qdma_dev->dev_conf.name, irq_ctx->vector_id);
        return;
    }

    const auto ring_va = static_cast<cpm_intr_entry *>(intr_queue->buffer_va);

    TraceVerbose(TRACE_INTR, "%s: CPM Coal queue SW Index : %u", 
        irq_ctx->qdma_dev->dev_conf.name, intr_queue->sw_index);
    TraceVerbose(TRACE_INTR, "%s: CPM Intr PIDX : %u, Intr CIDX : %u", 
        irq_ctx->qdma_dev->dev_conf.name, ring_va[intr_queue->sw_index].desc_pidx, 
        ring_va[intr_queue->sw_index].desc_cidx);

    while (ring_va[intr_queue->sw_index].color == intr_queue->color) {
        q = irq_ctx->qdma_dev->qdma_get_queue_pair_by_hwid(ring_va[intr_queue->sw_index].qid);
        if (nullptr == q) {
            TraceError(TRACE_INTR, "%s: Queue not found hw qid : %u Intr qid : %u",
                irq_ctx->qdma_dev->dev_conf.name, 
                ring_va[intr_queue->sw_index].qid, 
                intr_queue->idx);
            intr_queue->advance_sw_index();
            continue;
        }

        is_c2h = ring_va[intr_queue->sw_index].intr_type;

        schedule_dpc(q, is_c2h, active_processors);

        intr_queue->advance_sw_index();

        TraceVerbose(TRACE_INTR, "%s: CPM QUEUE ID : %u, is_c2h : %d", 
            irq_ctx->qdma_dev->dev_conf.name, 
            ring_va[intr_queue->sw_index].qid, is_c2h);
    }

    if (q) {
        intr_queue->update_csr_cidx(q, intr_queue->sw_index);
    }
}

void handle_indirect_interrupt(
    PQDMA_IRQ_CONTEXT irq_ctx)
{
    queue_pair *q = nullptr;
    UINT8 is_c2h = 0;
    CCHAR active_processors = (CCHAR)KeQueryActiveProcessorCount(NULL);

    auto intr_queue = irq_ctx->intr_q;
    if (nullptr == intr_queue) {
        TraceError(TRACE_INTR, "%s: Invalid vector %lu was called in coal mode", 
            irq_ctx->qdma_dev->dev_conf.name, irq_ctx->vector_id);
        return;
    }

    const auto ring_va = static_cast<intr_entry *>(intr_queue->buffer_va);

    TraceVerbose(TRACE_INTR, "%s: Coal queue SW Index : %u", 
        irq_ctx->qdma_dev->dev_conf.name, intr_queue->sw_index);
    TraceVerbose(TRACE_INTR, "%s: Intr PIDX : %u, Intr CIDX : %u", 
        irq_ctx->qdma_dev->dev_conf.name, 
        ring_va[intr_queue->sw_index].desc_pidx, 
        ring_va[intr_queue->sw_index].desc_cidx);

    while (ring_va[intr_queue->sw_index].color == intr_queue->color) {
        q = irq_ctx->qdma_dev->qdma_get_queue_pair_by_hwid(ring_va[intr_queue->sw_index].qid);
        if (nullptr == q) {
            TraceError(TRACE_INTR, "%s: Queue not found hw qid : %u Intr qid : %u",
                irq_ctx->qdma_dev->dev_conf.name, 
                ring_va[intr_queue->sw_index].qid, intr_queue->idx);
            intr_queue->advance_sw_index();
            continue;
        }

        is_c2h = ring_va[intr_queue->sw_index].intr_type;

        schedule_dpc(q, is_c2h, active_processors);

        intr_queue->advance_sw_index();

        TraceVerbose(TRACE_INTR, "%s: QUEUE ID : %u, is_c2h : %d", 
            irq_ctx->qdma_dev->dev_conf.name, ring_va[intr_queue->sw_index].qid, is_c2h);
    }

    if (q) {
        intr_queue->update_csr_cidx(q, intr_queue->sw_index);
    }
}

void handle_direct_interrupt(
    PQDMA_IRQ_CONTEXT irq_ctx)
{
    CCHAR active_processors = (CCHAR)KeQueryActiveProcessorCount(NULL);
    PLIST_ENTRY entry;
    PLIST_ENTRY temp;

    LIST_FOR_EACH_ENTRY_SAFE(&irq_ctx->queue_list_head, temp, entry) {
        queue_pair *queue = CONTAINING_RECORD(entry, queue_pair, list_entry);

        TraceVerbose(TRACE_INTR, "%s: SERVICING QUEUE : %u IN DIRECT INTERRUPT", 
            irq_ctx->qdma_dev->dev_conf.name, queue->idx);
        schedule_dpc(queue, 0 /* H2C */, active_processors);
        schedule_dpc(queue, 1 /* C2H */, active_processors);

    }
}

int qdma_device::setup_legacy_vector(queue_pair& q)
{
    int ret = 0;
    int status = 0;
    int legacy_vec = 0;

    WdfInterruptAcquireLock(irq_mgr.irq[legacy_vec]);

    auto irq_ctx = get_qdma_irq_context(irq_mgr.irq[legacy_vec]);
    if (false == IS_LIST_EMPTY(&irq_ctx->queue_list_head)) {
        TraceError(TRACE_INTR, "%s: Only One queue is supported "
            "in legacy interrupt mode", dev_conf.name);
        status = -(STATUS_UNSUCCESSFUL);
        goto ErrExit;
    }

    if (hw.qdma_legacy_intr_conf == nullptr) {
        TraceError(TRACE_INTR, "%s: legacy interrupt mode "
            "not supported", dev_conf.name);
        status = -(STATUS_UNSUCCESSFUL);
        goto ErrExit;
    }

    ret = hw.qdma_legacy_intr_conf(this, DISABLE);
    if (ret < 0) {
        TraceError(TRACE_INTR, "%s: qdma_disable_legacy_interrupt "
            "failed, ret : %d", dev_conf.name, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    LIST_ADD_TAIL(&irq_ctx->queue_list_head, &q.list_entry);

    ret = hw.qdma_legacy_intr_conf(this, ENABLE);
    if (ret < 0) {
        TraceError(TRACE_INTR, "%s: qdma_enable_legacy_interrupt "
            "failed, ret : %d", dev_conf.name, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    TraceVerbose(TRACE_INTR, "%s: Vector Allocated [0] for legacy interrupt mode", 
        dev_conf.name);

    WdfInterruptReleaseLock(irq_mgr.irq[legacy_vec]);

    return legacy_vec;

ErrExit:
    WdfInterruptReleaseLock(irq_mgr.irq[legacy_vec]);
    return status;
}

/* Allocate MSIx Vector position */
UINT32 qdma_device::alloc_msix_vector_position(queue_pair& q)
{
    UINT32 weight;
    UINT32 vector;

    WdfSpinLockAcquire(irq_mgr.lock);

    vector = irq_mgr.data_vector_id_start;
    weight = irq_mgr.irq_weight[vector];

    for (UINT32 i = irq_mgr.data_vector_id_start + 1; i <= irq_mgr.data_vector_id_end; ++i) {
        if (irq_mgr.irq_weight[i] < weight) {
            weight = irq_mgr.irq_weight[i];
            vector = i;
        }
    }

    ++irq_mgr.irq_weight[vector];

    WdfSpinLockRelease(irq_mgr.lock);

    if (drv_conf.operation_mode == queue_op_mode::INTR_MODE) {
        WdfInterruptAcquireLock(irq_mgr.irq[vector]);

        auto irq_ctx = get_qdma_irq_context(irq_mgr.irq[vector]);
        LIST_ADD_TAIL(&irq_ctx->queue_list_head, &queue_pairs[q.idx].list_entry);

        WdfInterruptReleaseLock(irq_mgr.irq[vector]);
    }
    else if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) {
        /* For indirect interrupt, return absolute interrupt queue index */
        auto irq_ctx = get_qdma_irq_context(irq_mgr.irq[vector]);
        vector = irq_ctx->intr_q->idx_abs;
    }

    TraceVerbose(TRACE_INTR, "%s: Vector Allocated [%u]. Weight : %u",
        dev_conf.name, vector, irq_mgr.irq_weight[vector]);

    return vector;
}

/* Free MSIX vector position */
void qdma_device::free_msix_vector_position(
    queue_pair& q,
    UINT32 vector)
{
    auto RELATIVE_INTR_QID = [](auto q) { return q % (UINT32)qdma_max_msix_vectors_per_pf; };
    if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE)
        vector = RELATIVE_INTR_QID(vector);
    else if (drv_conf.operation_mode == queue_op_mode::INTR_MODE) {
        WdfInterruptAcquireLock(irq_mgr.irq[vector]);
        auto irq_ctx = get_qdma_irq_context(irq_mgr.irq[vector]);
        PLIST_ENTRY entry;
        LIST_FOR_EACH_ENTRY(&irq_ctx->queue_list_head, entry) {
            queue_pair *queue = CONTAINING_RECORD(entry, queue_pair, list_entry);
            if (queue->idx == q.idx) {
                LIST_DEL_NODE(entry);
                break;
            }
        }
        WdfInterruptReleaseLock(irq_mgr.irq[vector]);
    }

    WdfSpinLockAcquire(irq_mgr.lock);

    --irq_mgr.irq_weight[vector];

    TraceVerbose(TRACE_INTR, "%s: Vector Released. New weight : %u", 
        dev_conf.name, irq_mgr.irq_weight[vector]);
    WdfSpinLockRelease(irq_mgr.lock);
}

int qdma_device::assign_interrupt_vector(queue_pair& q)
{
    UINT32 vec;

    if (irq_mgr.intr_type == interrupt_type::MSIX)
        vec = alloc_msix_vector_position(q);
    else {
        vec = setup_legacy_vector(q);
    }
    return vec;
}

void qdma_device::free_interrupt_vector(queue_pair& q, UINT32 vec_id)
{
    if (irq_mgr.intr_type == interrupt_type::MSIX)
        free_msix_vector_position(q, vec_id);
    else
        clear_legacy_vector(q, vec_id);
}

/* Clear legacy vector and disable interrupts */
void qdma_device::clear_legacy_vector(
    queue_pair& q,
    UINT32 vector)
{
    UNREFERENCED_PARAMETER(q);

    WdfInterruptAcquireLock(irq_mgr.irq[vector]);

    auto irq_ctx = get_qdma_irq_context(irq_mgr.irq[vector]);
    auto queue_item = irq_ctx->queue_list_head;

    if (hw.qdma_legacy_intr_conf != nullptr) {
        hw.qdma_legacy_intr_conf(this, DISABLE);
    }

    INIT_LIST_HEAD(&irq_ctx->queue_list_head);
    WdfInterruptReleaseLock(irq_mgr.irq[vector]);
}

NTSTATUS qdma_device::configure_irq(
    PQDMA_IRQ_CONTEXT irq_context,
    ULONG vec)
{
    irq_context->vector_id = vec;
    irq_context->qdma_dev = this;

    if ((vec >= irq_mgr.data_vector_id_start) && (vec <= irq_mgr.data_vector_id_end)) {
        /* Data interrupts */
        irq_mgr.irq_weight[vec] = 0u;

        if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) { /* Indirect interrupt */
            irq_context->intr_q = &irq_mgr.intr_q[vec];
            irq_mgr.intr_q[vec].vector = vec;
            irq_context->is_coal = true;
            if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
                irq_context->interrupt_handler = &cpm_handle_indirect_interrupt;
            }
            else {
                irq_context->interrupt_handler = &handle_indirect_interrupt;
            }
        }
        else { /* Direct interrupt */
            INIT_LIST_HEAD(&irq_context->queue_list_head);

            irq_context->is_coal = false;
            irq_context->interrupt_handler = &handle_direct_interrupt;
        }
        irq_context->intr_type = irq_mgr.intr_type;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::arrange_msix_vector_types(void)
{
    ULONG vector = 0;
    ULONG req_vec;
    ULONG num_msix_vectors = pcie.get_num_msix_vectors();

    if (num_msix_vectors == 0ul) {
        TraceError(TRACE_INTR, "%s: Not enough MSIx vectors : [%u]", 
            dev_conf.name, num_msix_vectors);
        return STATUS_UNSUCCESSFUL;
    }

    /** Reserve one vector for Error Interrupt which reports out 
      * the QDMA internal HW errors to the user 
      * 
      * Master PF will own this option and hence other PFs dont need
      * to reserve vector for error interrupt
      */
    if (dev_conf.is_master_pf) { /* Master PF */
        TraceInfo(TRACE_INTR, "%s: Setting Error Interrupt by Master PF", 
            dev_conf.name);
        irq_mgr.err_vector_id = vector;
        ++vector;
        /** Error interrupt consumes 1 vector from data interrupt vectors */
        drv_conf.data_msix_max = drv_conf.data_msix_max - 1;
    }

    req_vec = vector + drv_conf.data_msix_max + drv_conf.user_msix_max;

    if (num_msix_vectors < req_vec) {
        TraceError(TRACE_INTR, "%s: Not enough MSIx vectors : [%u]. Requested : [%u]\n",
            dev_conf.name, num_msix_vectors, req_vec);
        return STATUS_UNSUCCESSFUL;
    }

    irq_mgr.user_vector_id_start = vector;
    irq_mgr.user_vector_id_end = vector + drv_conf.user_msix_max - 1 ;
    vector += drv_conf.user_msix_max;

    irq_mgr.data_vector_id_start = vector;

    if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE)
        irq_mgr.data_vector_id_end = irq_mgr.data_vector_id_start + IND_INTR_MAX_DATA_VECTORS - 1;
    else
        irq_mgr.data_vector_id_end = vector + drv_conf.data_msix_max - 1;

    TraceVerbose(TRACE_INTR, "%s: Function: %0X, Err vec : %lu, User vec : [%u : %u] Data vec : [%u : %u]",
        dev_conf.name, dev_conf.dev_sbdf.sbdf.fun_no, irq_mgr.err_vector_id,
        irq_mgr.user_vector_id_start, irq_mgr.user_vector_id_end,
        irq_mgr.data_vector_id_start, irq_mgr.data_vector_id_end);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::setup_msix_interrupt(
    WDFCMRESLIST resources,
    const WDFCMRESLIST resources_translated)
{
    NTSTATUS status;

    /* Setup interrupts */
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource_translated;

    ULONG numResources = WdfCmResourceListGetCount(resources_translated);
    TraceVerbose(TRACE_INTR, "%s: Total number of resource : %lu", 
        dev_conf.name, numResources);

    status = arrange_msix_vector_types();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "%s: Failed to arrange MSIx vectors", dev_conf.name);
        return status;
    }

    for (UINT i = 0, vec = 0; i < numResources && vec < pcie.get_num_msix_vectors(); i++) {

        resource = WdfCmResourceListGetDescriptor(resources, i);
        resource_translated = WdfCmResourceListGetDescriptor(resources_translated, i);
        if (resource_translated->Type != CmResourceTypeInterrupt) {
            continue;
        }

        WDF_INTERRUPT_CONFIG config;

        if ((irq_mgr.err_vector_id == vec) && (dev_conf.is_master_pf)) {
            WDF_INTERRUPT_CONFIG_INIT(&config, EvtErrorInterruptIsr, EvtErrorInterruptDpc);
            config.EvtInterruptEnable = nullptr;
            config.EvtInterruptDisable = nullptr;
            TraceVerbose(TRACE_INTR, "%s: [%u] - Error interrupt configuration", 
                dev_conf.name, vec);
        }
        else if ((vec >= irq_mgr.user_vector_id_start) && (vec <= irq_mgr.user_vector_id_end)) {
            WDF_INTERRUPT_CONFIG_INIT(&config, EvtUserInterruptIsr, EvtUserInterruptDpc);
            config.EvtInterruptEnable = EvtUserInterruptEnable;
            config.EvtInterruptDisable = EvtUserInterruptDisable;
            TraceVerbose(TRACE_INTR, "%s: [%u] - User interrupt configuration", 
                dev_conf.name, vec);
        }
        else if ((vec >= irq_mgr.data_vector_id_start) && (vec <= irq_mgr.data_vector_id_end)) { /* Data interrupts */
            WDF_INTERRUPT_CONFIG_INIT(&config, EvtDataInterruptIsr, nullptr);
            config.EvtInterruptEnable = nullptr;
            config.EvtInterruptDisable = nullptr;
            TraceVerbose(TRACE_INTR, "%s: [%u] - Data interrupt configuration", dev_conf.name, vec);
        }
        else {
            TraceVerbose(TRACE_INTR, "%s: [%u] - No configuration", dev_conf.name, vec);
            continue;
        }

        config.InterruptRaw = resource;
        config.InterruptTranslated = resource_translated;
        config.AutomaticSerialization = TRUE;

        WDF_OBJECT_ATTRIBUTES attribs;
        WDF_OBJECT_ATTRIBUTES_INIT(&attribs);
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attribs, QDMA_IRQ_CONTEXT);

        status = WdfInterruptCreate(wdf_dev, &config, &attribs, &irq_mgr.irq[vec]);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "WdfInterruptCreate failed: %!STATUS!", status);
            return status;
        }

        /* FIXME: 2018.2 Bitstream Issue?? Unmasking of the MSIX vectors not happening. So doing it manually */
        if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
            unmask_msi_entry(vec);
        }

        auto irq_context = get_qdma_irq_context(irq_mgr.irq[vec]);
        status = configure_irq(irq_context, vec);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "%s: WdfInterruptCreate failed: %!STATUS!", 
                dev_conf.name, status);
            return status;
        }

        if ((irq_mgr.err_vector_id == vec) && (dev_conf.is_master_pf)) {
            int ret = hw.qdma_hw_error_intr_setup((void *)this, 
                (uint16_t)dev_conf.dev_sbdf.sbdf.fun_no, (uint8_t)irq_mgr.err_vector_id);
            if (ret < 0) {
                TraceError(TRACE_INTR, "%s: qdma_error_interrupt_setup() failed with error %d", 
                    dev_conf.name, ret);
                return hw.qdma_get_error_code(ret);
            }

            ret = hw.qdma_hw_error_enable((void *)this, hw.qdma_max_errors);
            if (ret < 0) {
                TraceError(TRACE_INTR, "%s: qdma_error_enable() failed with error %d", 
                    dev_conf.name, ret);
                return hw.qdma_get_error_code(ret);
            }

            ret = hw.qdma_hw_error_intr_rearm((void *)this);
            if (ret < 0) {
                TraceError(TRACE_INTR, "%s: qdma_error_interrupt_rearm() failed with error %d", 
                    dev_conf.name, ret);
                return hw.qdma_get_error_code(ret);
            }
        }

        ++vec;

        TraceInfo(TRACE_INTR, "%s: INTERRUPT REGISTERED FOR VECTOR ID: : %d WEIGHT : %d",
            dev_conf.name, irq_context->vector_id, irq_context->weight);
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::setup_legacy_interrupt(
    WDFCMRESLIST resources,
    const WDFCMRESLIST resources_translated)
{
    NTSTATUS status;

    /* Setup interrupts */
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource_translated;

    ULONG numResources = WdfCmResourceListGetCount(resources_translated);
    TraceVerbose(TRACE_INTR, "%s: Total number of resource : %lu", 
        dev_conf.name, numResources);


    for (UINT i = 0, vec = 0; i < numResources; i++) {

        resource = WdfCmResourceListGetDescriptor(resources, i);
        resource_translated = WdfCmResourceListGetDescriptor(resources_translated, i);
        if (resource_translated->Type != CmResourceTypeInterrupt) {
            continue;
        }

        WDF_INTERRUPT_CONFIG config;

        /* Initializing the interrupt config with Data ISR and DPC handlers */
        WDF_INTERRUPT_CONFIG_INIT(&config, EvtDataInterruptIsr, nullptr);

        config.InterruptRaw = resource;
        config.InterruptTranslated = resource_translated;
        config.EvtInterruptEnable = nullptr;
        config.EvtInterruptDisable = nullptr;
        config.AutomaticSerialization = TRUE;

        WDF_OBJECT_ATTRIBUTES attribs;
        WDF_OBJECT_ATTRIBUTES_INIT(&attribs);
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attribs, QDMA_IRQ_CONTEXT);

        status = WdfInterruptCreate(wdf_dev, &config, &attribs, &irq_mgr.irq[vec]);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "%s: WdfInterruptCreate failed: %!STATUS!", 
                dev_conf.name, status);
            return status;
        }

        auto irq_context = get_qdma_irq_context(irq_mgr.irq[vec]);
        status = configure_irq(irq_context, vec);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "%s: WdfInterruptCreate failed: %!STATUS!", 
                dev_conf.name, status);
            return status;
        }

        ++vec;

        TraceInfo(TRACE_INTR, "%s: LEGACY INTERRUPT REGISTERED FOR VECTOR ID: : %d WEIGHT : %d",
            dev_conf.name, irq_context->vector_id, irq_context->weight);

        /* Only One Vector for Legacy interrupt */
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::intr_setup(
    WDFCMRESLIST resources,
    const WDFCMRESLIST resources_translated)
{
    UINT32 i;
    NTSTATUS status = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource_desc;
    ULONG numResources = WdfCmResourceListGetCount(resources_translated);

    /* Initialize IRQ spin lock */
    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT(&attr);
    attr.ParentObject = wdf_dev;

    status = WdfSpinLockCreate(&attr, &irq_mgr.lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "%s: WdfSpinLockCreate failed: %!STATUS!", 
            dev_conf.name, status);
        return status;
    }

    irq_mgr.intr_type = interrupt_type::NONE;
    for (i = 0; i < numResources; i++) {
        resource_desc = WdfCmResourceListGetDescriptor(resources_translated, i);
        if (resource_desc->Type != CmResourceTypeInterrupt) {
            TraceVerbose(TRACE_INTR, "Non Interrupt Resource : %d", resource_desc->Type);
            continue;
        }

        if (resource_desc->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) {
            irq_mgr.intr_type = interrupt_type::MSIX;

            TraceVerbose(TRACE_INTR,
                "Message Interrupt level 0x%0x, Vector 0x%0x, MessageCount %u\n"
                , resource_desc->u.MessageInterrupt.Translated.Level
                , resource_desc->u.MessageInterrupt.Translated.Vector
                , resource_desc->u.MessageInterrupt.Raw.MessageCount
            );
        }
        else {
            irq_mgr.intr_type = interrupt_type::LEGACY;

            TraceVerbose(TRACE_INTR,
                "Legacy Interrupt level: 0x%0x, Vector: 0x%0x\n"
                , resource_desc->u.Interrupt.Level
                , resource_desc->u.Interrupt.Vector
            );
        }
        break;
    }

    if (irq_mgr.intr_type == interrupt_type::LEGACY) {
        status = setup_legacy_interrupt(resources, resources_translated);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "%s: setup_legacy_interrupt() failed: %!STATUS!", 
                dev_conf.name, status);
        }
    }
    else if (irq_mgr.intr_type == interrupt_type::MSIX) {
        status = setup_msix_interrupt(resources, resources_translated);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_INTR, "%s: setup_msix_interrupt() failed: %!STATUS!", 
                dev_conf.name, status);
        }
    }
    else {
        TraceError(TRACE_INTR, "%s: Invalid Interrupt Type : %d "
            "(valid are legacy and msix)", dev_conf.name, (int)irq_mgr.intr_type);
        return STATUS_UNSUCCESSFUL;
    }

    return status;
}

void qdma_device::intr_teardown(void)
{
    if ((irq_mgr.intr_type == interrupt_type::LEGACY) &&
        (hw.qdma_legacy_intr_conf != nullptr))
        hw.qdma_legacy_intr_conf((void *)this, DISABLE);
    else {
        /* FIXME: 2018.2 Bitstream issue?? Mask the msix vectors */
        if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
            for (auto i = 0u; i < pcie.get_num_msix_vectors(); ++i) {
                mask_msi_entry(i);
            }
        }
    }
}
