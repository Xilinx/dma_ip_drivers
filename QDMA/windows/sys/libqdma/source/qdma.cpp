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
#include "qdma_platform.h"
#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "qdma.tmh"
#endif

using namespace xlnx;

/* Below are per driver global variables */
static LIST_ENTRY qdma_dev_list_head;
static WDFWAITLOCK qdma_list_lock;
static volatile LONG qdma_active_pf_count = 0;

NTSTATUS qdma_device::list_add_qdma_device_and_set_gbl_csr(void)
{
    if ((LONG)1 == InterlockedIncrement(&qdma_active_pf_count)) {
        TraceVerbose(TRACE_QDMA, "** Initializing global device list and wait lock **");

        /* First device in the list, Initialize the list head */
        INIT_LIST_HEAD(&qdma_dev_list_head);

        WDF_OBJECT_ATTRIBUTES attr;
        WDF_OBJECT_ATTRIBUTES_INIT(&attr);
        attr.ParentObject = WDFDRIVER();

        NTSTATUS status = WdfWaitLockCreate(&attr, &qdma_list_lock);
        if (!(NT_SUCCESS(status))) {
            TraceError(TRACE_QDMA, "Failed to create qdma list wait lock %!STATUS!", status);
            qdma_list_lock = nullptr;
            return status;
        }
    }
    else {
        /* To avoid race, Wait for some time
         * if list head and wait lock initialization already in progress.
         */
        LARGE_INTEGER wait_time;
        wait_time.QuadPart = WDF_REL_TIMEOUT_IN_US(100);
        KeDelayExecutionThread(KernelMode, FALSE, &wait_time);

        if (nullptr == qdma_list_lock) {
            TraceError(TRACE_QDMA, "qdma list wait lock not initialized");
            return STATUS_UNSUCCESSFUL;
        }
    }

    TraceVerbose(TRACE_QDMA, "++ Active pf count : %u ++", qdma_active_pf_count);

    WdfWaitLockAcquire(qdma_list_lock, NULL);

    dev_conf.is_master_pf = is_first_qdma_pf_device();
    if (true == dev_conf.is_master_pf) {
        TraceInfo(TRACE_QDMA, "BDF : 0x%05X is master PF\n", dev_conf.dev_sbdf.val);
        TraceVerbose(TRACE_QDMA, "Setting Global CSR");

        int ret = hw.qdma_set_default_global_csr(this);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to set global CSR Configuration, ret : %d", ret);
            WdfWaitLockRelease(qdma_list_lock);
            return hw.qdma_get_error_code(ret);
        }
    }

    LIST_ADD_TAIL(&qdma_dev_list_head, &list_entry);

    WdfWaitLockRelease(qdma_list_lock);

    return STATUS_SUCCESS;
}

bool qdma_device::is_first_qdma_pf_device(void)
{
    PLIST_ENTRY entry, temp;

    LIST_FOR_EACH_ENTRY_SAFE(&qdma_dev_list_head, temp, entry) {
        qdma_device *qdma_dev = CONTAINING_RECORD(entry, qdma_device, list_entry);
        if (qdma_dev->dev_conf.dev_sbdf.sbdf.bus_no == dev_conf.dev_sbdf.sbdf.bus_no) {
            return false;
        }
    }

    return true;
}

void qdma_device::list_remove_qdma_device(void)
{
    if (nullptr == list_entry.Flink && nullptr == list_entry.Blink)
        return;

    WdfWaitLockAcquire(qdma_list_lock, nullptr);
    LIST_DEL_NODE(&list_entry);
    WdfWaitLockRelease(qdma_list_lock);
    auto cnt = InterlockedDecrement(&qdma_active_pf_count);
    TraceVerbose(TRACE_QDMA, "-- Active pf count : %u --", cnt);
}

/* --------------------------- debug prints of structs --------------------------- */

#ifdef ENABLE_WPP_TRACING

static void dump(const mm_descriptor& desc)
{
    TraceVerbose(
        TRACE_DBG,
        "descriptor={valid=%u, sop=%u, eop=%u, length=%u, addr=0x%llX,"
        "dest_addr=0x%llX }",
        desc.valid, desc.sop, desc.eop, desc.length, desc.addr,
        desc.dest_addr);
}

static void dump(const h2c_descriptor& desc)
{
    TraceVerbose(
        TRACE_DBG,
        "descriptor={sop=%u, eop=%u, length=%u, addr=0x%llX}",
        desc.sop, desc.eop, desc.length, desc.addr);
}

static void dump(const char* dir, const qdma_descq_hw_ctxt& ctxt)
{
    TraceVerbose(
        TRACE_QDMA,
        "%s_hw_desc_ctxt={cidx=%u, credit_use=%u, desc_pending=%u, "
        "idle_stop=%u evt_pending=%u fetch_pending=%u}",
        dir, ctxt.cidx, ctxt.crd_use, ctxt.dsc_pend, ctxt.idl_stp_b,
        ctxt.evt_pnd, ctxt.fetch_pnd);
}

static void dump(const char* dir, const qdma_descq_sw_ctxt& ctxt)
{
    TraceVerbose(TRACE_QDMA,
        "%s_sw_desc_ctxt={pidx=%u irq_arm=%u, funid=%u, qen=%u, "
        "frcd_en=%u, wbi_chk=%u, wbi_intvl_en=%u at=%u, fetch_max=%u, "
        "rngsz_idx=%u, desc_sz=%u, bypass=%u, mm_ch=%u wb_en=%u"
        "irq_en=%u, portid=%u, irq_no_last=%u, err=%u, err_wb=%u, "
        "irq_req=%u, mrkr_dis=%u is_mm=%u, ring_bs_addr=%llX, "
        "vec=%u, int_aggr=%u}",
        dir, ctxt.pidx, ctxt.irq_arm, ctxt.fnc_id, ctxt.qen,
        ctxt.frcd_en, ctxt.wbi_chk, ctxt.wbi_intvl_en, ctxt.at,
        ctxt.fetch_max, ctxt.rngsz_idx, ctxt.desc_sz, ctxt.bypass,
        ctxt.mm_chn, ctxt.wbk_en, ctxt.irq_en, ctxt.port_id,
        ctxt.irq_no_last, ctxt.err, ctxt.err_wb_sent, ctxt.irq_req,
        ctxt.mrkr_dis, ctxt.is_mm, ctxt.ring_bs_addr,
        ctxt.vec, ctxt.intr_aggr);
}

static void dump(const qdma_descq_prefetch_ctxt& ctxt)
{
    TraceVerbose(
        TRACE_DBG,
        "c2h_prefetch_ctxt={bypass=%u, buffer_size_idx=%u, "
        "sw_credit=%u, valid=%u}",
        ctxt.bypass, ctxt.bufsz_idx, ctxt.sw_crdt, ctxt.valid);
}

static void dump(const qdma_descq_credit_ctxt& ctxt)
{
    TraceVerbose(
        TRACE_DBG,
        "c2h_credit_ctxt={credits=%u}",
        ctxt.credit);
}

static void dump(const qdma_descq_cmpt_ctxt& ctxt)
{
    TraceVerbose(
        TRACE_DBG,
        "c2h_cmpt_ctxt={en_stat_desc=%u, en_int=%u, trig_mode=%u, "
        "funid=%u, cntr_idx=%u, tmr_idx=%u, int_st=%u, color=%u, "
        "ringsz_idx=%u, bs_addr=%llX, desc_sz=%u, pidx=%u, cidx=%u, "
        "valid=%u, err=%u, usr_trig_pnd=%u, tmr_runing=%u, fupd=%u, "
        "ovf_chk_dis=%u, at=%u, vec=%u, int_aggr=%u}",
        ctxt.en_stat_desc, ctxt.en_int, ctxt.trig_mode, ctxt.fnc_id,
        ctxt.counter_idx, ctxt.timer_idx, ctxt.in_st, ctxt.color, ctxt.ringsz_idx,
        ctxt.bs_addr, ctxt.desc_sz, ctxt.pidx, ctxt.cidx, ctxt.valid,
        ctxt.err, ctxt.user_trig_pend, ctxt.timer_running, ctxt.full_upd,
        ctxt.ovf_chk_dis, ctxt.at, ctxt.vec, ctxt.int_aggr);
}

static void dump(const qdma_qid2vec& ctxt) {
    TraceVerbose(
        TRACE_DBG,
        "qid2vec_ctxt={c2h_coal_en=%u, c2h_vec=%u, h2c_coal_en=%u, "
        "h2c_vec=%u}",
        ctxt.c2h_en_coal, ctxt.c2h_vector, ctxt.h2c_en_coal, ctxt.h2c_vector);
}

static void dump(qdma_device *qdma_dev, const queue_type q_type, const UINT16 qid)
{
    TraceVerbose(TRACE_DBG, "------------ QUEUE_%u CONTEXT DUMPS ------------", qid);
    qdma_descq_sw_ctxt sw_ctxt = { 0 };
    qdma_dev->hw.qdma_sw_ctx_conf((void *)qdma_dev, false, qid, &sw_ctxt, QDMA_HW_ACCESS_READ);
    dump("H2C", sw_ctxt);
    qdma_dev->hw.qdma_sw_ctx_conf((void *)qdma_dev, true, qid, &sw_ctxt, QDMA_HW_ACCESS_READ);
    dump("C2H", sw_ctxt);

    qdma_descq_hw_ctxt hw_ctxt = { 0 };
    qdma_dev->hw.qdma_hw_ctx_conf((void *)qdma_dev, false, qid, &hw_ctxt, QDMA_HW_ACCESS_READ);
    dump("H2C", hw_ctxt);
    qdma_dev->hw.qdma_hw_ctx_conf((void *)qdma_dev, true, qid, &hw_ctxt, QDMA_HW_ACCESS_READ);
    dump("C2H", hw_ctxt);

    if (qdma_dev->hw.qdma_qid2vec_conf) {
        qdma_qid2vec qid2vec_ctxt = { 0 };
        qdma_dev->hw.qdma_qid2vec_conf((void *)qdma_dev, false, qid, &qid2vec_ctxt, QDMA_HW_ACCESS_READ);
        dump(qid2vec_ctxt);
        qdma_dev->hw.qdma_qid2vec_conf((void *)qdma_dev, true, qid, &qid2vec_ctxt, QDMA_HW_ACCESS_READ);
        dump(qid2vec_ctxt);
    }

    if (q_type == queue_type::STREAMING) {
        qdma_descq_cmpt_ctxt cmpt_ctxt = { 0 };
        qdma_dev->hw.qdma_cmpt_ctx_conf((void *)qdma_dev, qid, &cmpt_ctxt, QDMA_HW_ACCESS_READ);
        dump(cmpt_ctxt);

        qdma_descq_prefetch_ctxt pfch_ctxt = { 0 };
        qdma_dev->hw.qdma_pfetch_ctx_conf((void *)qdma_dev, qid, &pfch_ctxt, QDMA_HW_ACCESS_READ);
        dump(pfch_ctxt);

        qdma_descq_credit_ctxt credit_ctxt = { 0 };
        qdma_dev->hw.qdma_credit_ctx_conf((void *)qdma_dev, true, qid, &credit_ctxt, QDMA_HW_ACCESS_READ);
        dump(credit_ctxt);
    }
    TraceVerbose(TRACE_DBG, "------------  ------------");
}

static void dump(const c2h_wb_status& status)
{
    TraceVerbose(
        TRACE_DBG,
        "c2h_wb_status={pidx=%u, cidx=%u, color=%u, irq_state=%u}",
        status.pidx, status.cidx, status.color, status.irq_state);
}

static void dump(int index, const c2h_wb_header_8B& status)
{
    TraceVerbose(
        TRACE_DBG,
        "c2h_cmpt_ring[%u]={length=%u, color=%u, desc_error=%u, "
        "user_defined_0=0x%llx}",
        index, status.length, status.color, status.desc_error,
        status.user_defined_0);
}
#else
#define dump(...) (__VA_ARGS__)
#endif

static size_t get_descriptor_size(qdma_desc_sz desc_sz)
{
    size_t size_in_bytes = 0;

    size_in_bytes = static_cast<size_t>(1) << (3 + static_cast<size_t>(desc_sz));

    return size_in_bytes;
}

//======================= private member function implemenations ==================================

_IRQL_requires_max_(PASSIVE_LEVEL)
void *qdma_interface::operator new(_In_ size_t num_bytes)
{
    VERIFY_IS_IRQL_PASSIVE_LEVEL();

    void *mem = qdma_calloc(1, (uint32_t)num_bytes);
    if (mem)
        RtlZeroMemory(mem, num_bytes);

    return mem;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void qdma_interface::operator delete(_In_ void *addr)
{
    VERIFY_IS_IRQL_PASSIVE_LEVEL();
    if (addr != nullptr) {
        qdma_memfree(addr);
    }
}

qdma_interface* qdma_interface::create_qdma_device(void)
{
    qdma_interface *dev_interface = nullptr;

    dev_interface = new xlnx::qdma_device;
    if (dev_interface == nullptr)
        TraceError(TRACE_QDMA, "qdma_interface allocation Failed");
    else
        TraceInfo(TRACE_QDMA, "qdma_interface allocation Success : %d", sizeof(xlnx::qdma_device));

    return dev_interface;
}

void qdma_interface::remove_qdma_device(qdma_interface *qdma_dev)
{
    delete qdma_dev;
}

NTSTATUS qdma_device::init_qdma_global()
{
    NTSTATUS status;
    int ret;

    status = qdma_read_csr_conf(&csr_conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to read CSR Configuration");
        return status;
    }

    ret = hw.qdma_hw_error_enable((void *)this, QDMA_ERRS_ALL);
    if (ret < 0) {
        TraceInfo(TRACE_QDMA, "Failed to enable qdma errors, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    UINT8 h2c = 0, c2h = 1;
    for (UINT8 i = 0; i < dev_conf.dev_info.mm_channel_max; i++) {
        ret = hw.qdma_mm_channel_conf((void *)this, i, h2c, TRUE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to configure H2C MM channel %d, ret : %d", i, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_mm_channel_conf((void *)this, i, c2h, TRUE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to configure C2H MM channel %d, ret : %d", i, ret);
            return hw.qdma_get_error_code(ret);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::init_func()
{
    int ret;

    struct qdma_fmap_cfg config = { 0 };
    config.qbase = static_cast<UINT16>(qbase);
    config.qmax  = static_cast<UINT16>(qmax);

    ret = hw.qdma_fmap_conf((void *)this,
                            dev_conf.dev_sbdf.sbdf.fun_no,
                            &config,
                            QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed FMAP Programming for PF %d, ret: %d",
            dev_conf.dev_sbdf.sbdf.fun_no, ret);
        return hw.qdma_get_error_code(ret);
    }

    return STATUS_SUCCESS;
}

void qdma_device::destroy_func(void)
{
    struct qdma_fmap_cfg config = {};
    config.qbase = 0;
    config.qmax = 0;

    if (hw.qdma_fmap_conf) {
        hw.qdma_fmap_conf((void *)this,
                           dev_conf.dev_sbdf.sbdf.fun_no,
                           &config,
                           QDMA_HW_ACCESS_WRITE);
    }

    UINT8 h2c = 0, c2h = 1;
    if (hw.qdma_mm_channel_conf) {
        for (UINT8 i = 0; i < dev_conf.dev_info.mm_channel_max; i++) {
            hw.qdma_mm_channel_conf((void *)this, i, h2c, FALSE);
            hw.qdma_mm_channel_conf((void *)this, i, c2h, FALSE);
        }
    }
}


NTSTATUS qdma_device::assign_bar_types()
{
    UINT8 user_bar_id = 0;
    int ret = hw.qdma_get_user_bar((void *)this,
                                   false,
                                   dev_conf.dev_sbdf.sbdf.fun_no,
                                   &user_bar_id);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed to get user bar, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }
    user_bar_id = user_bar_id / 2;

    NTSTATUS status = pcie.assign_bar_types(user_bar_id);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, " pcie assign_bar_types() failed: %!STATUS!", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::init_dma_queues()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (qmax == 0)
        return STATUS_SUCCESS;

    queue_pairs = (queue_pair *)qdma_calloc(qmax, sizeof(queue_pair));
    if (nullptr == queue_pairs) {
        TraceError(TRACE_INTR, "Failed to allocate queue_pair memory");
        return STATUS_NO_MEMORY;
    }

    for (UINT16 qid = 0; qid < qmax; qid++) {
        auto& q = queue_pairs[qid];
        q.idx = qid;
        q.idx_abs = qid + static_cast<UINT16>(qbase);
        q.qdma = this;
        q.state = queue_state::QUEUE_AVAILABLE;

        if (op_mode != queue_op_mode::POLL_MODE)
            q.h2c_q.lib_config.irq_en = q.c2h_q.lib_config.irq_en = true;
        else
            q.h2c_q.lib_config.irq_en = q.c2h_q.lib_config.irq_en = false;

        /* clear all context fields for this queue */
        status = clear_contexts(q);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "queue_pair::clear_contexts() failed! %!STATUS!", status);
            destroy_dma_queues();
            return status;
        }
    }

    TraceVerbose(TRACE_QDMA, "All queue contexts for device %s are cleared", this->dev_conf.name);

    return STATUS_SUCCESS;
}

void qdma_device::destroy_dma_queues(void)
{
    if (nullptr != queue_pairs) {
        qdma_memfree(queue_pairs);
        queue_pairs = nullptr;
    }
}

NTSTATUS qdma_device::init_interrupt_queues()
{
    NTSTATUS status = STATUS_SUCCESS;
    int ret;
    qdma_indirect_intr_ctxt intr_ctx = { 0 };

    /* Initialize interrupt queues */
    for (auto vec = irq_mgr.data_vector_id_start; vec < irq_mgr.data_vector_id_end; ++vec) {
        auto& intr_q = irq_mgr.intr_q[vec];
        intr_q.idx = (UINT16)vec;
        intr_q.idx_abs = (UINT16)(vec + (dev_conf.dev_sbdf.sbdf.fun_no * qdma_max_msix_vectors_per_pf));
        intr_q.qdma = this;

        status = intr_q.create(dma_enabler);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "intr_queue::create_resources() failed! %!STATUS!", status);
            return status;
        }

        /* Any failures onwards does not need cleanup. Common buffer created by q.create()
         * is freed when dma_enabler object is deleted.
         */

        /* clear context for this queue */
        ret = hw.qdma_indirect_intr_ctx_conf((void *)this,
                                             intr_q.idx_abs,
                                             NULL,
                                             QDMA_HW_ACCESS_CLEAR);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to clear interrupt context \
                for intr queue %d, ret : %d", intr_q.idx_abs, ret);
            return hw.qdma_get_error_code(ret);
        }

        /* set the interrupt context for this queue */
        intr_ctx.valid = true;
        intr_ctx.vec = (UINT16)intr_q.vector;
        intr_ctx.color = intr_q.color;
        intr_ctx.func_id = dev_conf.dev_sbdf.sbdf.fun_no;
        intr_ctx.page_size = (UINT8)(intr_q.npages - 1);
        intr_ctx.baddr_4k = WdfCommonBufferGetAlignedLogicalAddress(intr_q.buffer).QuadPart;

        TraceVerbose(TRACE_DBG, "SETTING intr_ctxt={ \
            BaseAddr=%llX, color=%u, page_sz=%u, vector=%u }",
            intr_ctx.baddr_4k, intr_ctx.color, intr_ctx.page_size, intr_ctx.vec);

        ret = hw.qdma_indirect_intr_ctx_conf((void *)this,
                                             intr_q.idx_abs,
                                             &intr_ctx,
                                             QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to program interrupt \
                ring for intr queue %d, ret: %d", intr_q.idx_abs, ret);
            return hw.qdma_get_error_code(ret);
        }
    }

    return status;
}

NTSTATUS qdma_device::init_os_resources(
    WDFCMRESLIST resources,
    const WDFCMRESLIST resources_translated)
{
    WDF_OBJECT_ATTRIBUTES attr;
    WDF_OBJECT_ATTRIBUTES_INIT(&attr);
    attr.ParentObject = wdf_dev;

    WdfDeviceSetAlignmentRequirement(wdf_dev, FILE_64_BYTE_ALIGNMENT);
    WDF_DMA_ENABLER_CONFIG dma_enabler_config;
    WDF_DMA_ENABLER_CONFIG_INIT(&dma_enabler_config,
                                WdfDmaProfileScatterGather64Duplex,
                                1024 * 1024 * 1024); /* TODO: Magic */
    NTSTATUS status = WdfDmaEnablerCreate(wdf_dev,
                                          &dma_enabler_config,
                                          &attr,
                                          &dma_enabler);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, " WdfDmaEnablerCreate() failed: %!STATUS!", status);
        return status;
    }

    if ((dev_conf.is_master_pf) &&
        (op_mode == queue_op_mode::POLL_MODE)) {
        int ret = hw.qdma_hw_error_enable(this, QDMA_ERRS_ALL);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "qdma_error_enable() failed, ret : %d", ret);
            return hw.qdma_get_error_code(ret);
        }

        /* Create a thread for polling errors */
        status = th_mgr.create_err_poll_thread(this);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "create_err_poll_thread() failed: %!STATUS!", status);
            return status;
        }
    }

    status = th_mgr.create_sys_threads(op_mode);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, " create_sys_threads() failed: %!STATUS!", status);
        goto ErrExit;
    }

    /* INTERRUPT HANDLING */
    if (op_mode != queue_op_mode::POLL_MODE) {
        /* Get MSIx vector count */
        status = pcie.find_num_msix_vectors(wdf_dev);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "pcie.get_num_msix_vectors() failed! %!STATUS!", status);
            goto ErrExit;
        }

        status = intr_setup(resources, resources_translated);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, " intr_setup() failed: %!STATUS!", status);
            goto ErrExit;
        }
    }

    return status;

ErrExit:
    destroy_os_resources();
    return status;
}

void qdma_device::destroy_os_resources(void)
{
    /* dma_enabler will be free by wdf_dev cleanup */

    if (op_mode == queue_op_mode::POLL_MODE) {
        if (dev_conf.is_master_pf)
            th_mgr.terminate_err_poll_thread();
    }
    else {
        intr_teardown();
    }

    th_mgr.terminate_sys_threads();
}

NTSTATUS qdma_device::init_resource_manager()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (qdma_resource_lock_init()) {
        TraceError(TRACE_QDMA, "Resource lock creation failed!");
        return STATUS_UNSUCCESSFUL;
    }

    int ret = qdma_master_resource_create(dev_conf.dev_sbdf.sbdf.bus_no,
                                          dev_conf.dev_sbdf.sbdf.bus_no,
                                          QDMA_QBASE,
                                          QDMA_TOTAL_Q,
                                          &dma_dev_index);
    if (ret < 0 && ret != -QDMA_ERR_RM_RES_EXISTS) {
        TraceError(TRACE_QDMA, "qdma_master_resource_create() failed!, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    TraceVerbose(TRACE_QDMA, "Device Index : %u Bus start : %u Bus End : %u",
        dma_dev_index, dev_conf.dev_sbdf.sbdf.bus_no, dev_conf.dev_sbdf.sbdf.bus_no);

    ret = qdma_dev_entry_create(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_dev_entry_create() failed for function : %u, ret : %d",
            dev_conf.dev_sbdf.sbdf.fun_no, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    ret = qdma_dev_update(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, qmax, &qbase);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_dev_update() failed for function : %u, ret : %d", dev_conf.dev_sbdf.sbdf.fun_no, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    TraceVerbose(TRACE_QDMA, "QDMA Function : %u, qbase : %u, qmax : %u",
        dev_conf.dev_sbdf.sbdf.fun_no, qbase, qmax);

    return status;

ErrExit:
    destroy_resource_manager();
    return status;
}


void qdma_device::destroy_resource_manager(void)
{
    qdma_dev_entry_destroy(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no);
    qdma_master_resource_destroy(dma_dev_index);
}

void qdma_device::inc_queue_pair_count(bool is_cmpt_valid)
{
    qdma_dev_increment_active_queue(dma_dev_index,
                                    dev_conf.dev_sbdf.sbdf.fun_no,
                                    QDMA_DEV_Q_TYPE_H2C);

    qdma_dev_increment_active_queue(dma_dev_index,
                                    dev_conf.dev_sbdf.sbdf.fun_no,
                                    QDMA_DEV_Q_TYPE_C2H);

    if (true == is_cmpt_valid) {
        qdma_dev_increment_active_queue(dma_dev_index,
                                        dev_conf.dev_sbdf.sbdf.fun_no,
                                        QDMA_DEV_Q_TYPE_CMPT);
    }
}

void qdma_device::dec_queue_pair_count(bool is_cmpt_valid)
{
    qdma_dev_decrement_active_queue(dma_dev_index,
                                    dev_conf.dev_sbdf.sbdf.fun_no,
                                    QDMA_DEV_Q_TYPE_H2C);

    qdma_dev_decrement_active_queue(dma_dev_index,
                                    dev_conf.dev_sbdf.sbdf.fun_no,
                                    QDMA_DEV_Q_TYPE_C2H);

    if (true == is_cmpt_valid) {
        qdma_dev_decrement_active_queue(dma_dev_index,
                                        dev_conf.dev_sbdf.sbdf.fun_no,
                                        QDMA_DEV_Q_TYPE_CMPT);
    }
}


/* ----- received packet fragment functions ----- */
NTSTATUS st_c2h_pkt_frag_queue::create(UINT32 entries)
{
    max_q_size = entries;

    frags = static_cast<st_c2h_pkt_fragment*>(qdma_calloc(entries,
                                              sizeof(st_c2h_pkt_fragment)));
    if (nullptr == frags) {
        TraceError(TRACE_QDMA, "Failed to allocate memory for rcvd pkts");
        return STATUS_MEMORY_NOT_ALLOCATED;
    }

    pidx = cidx = 0;
    avail_byte_cnt = 0;
    avail_frag_cnt = 0;

    return STATUS_SUCCESS;
}

void st_c2h_pkt_frag_queue::destroy(void)
{
    if(avail_frag_cnt != 0)
        TraceError(TRACE_QDMA, "Error: Still there are packets to process in"
            " received_pkts : count : %d", avail_frag_cnt);

    if (frags != nullptr) {
        qdma_memfree(frags);
        frags = nullptr;
    }
}

PFORCEINLINE bool st_c2h_pkt_frag_queue::is_queue_full(void)
{
    if (((pidx + 1) % max_q_size) == cidx) {
        return true;
    }
    return false;
}

PFORCEINLINE bool st_c2h_pkt_frag_queue::is_queue_empty(void)
{
    if (cidx == pidx) {
        return true;
    }
    return false;
}

inline LONG st_c2h_pkt_frag_queue::get_avail_frag_cnt(void)
{
    return avail_frag_cnt;
}

inline size_t st_c2h_pkt_frag_queue::get_avail_byte_cnt(void)
{
    return avail_byte_cnt;
}

NTSTATUS st_c2h_pkt_frag_queue::add(st_c2h_pkt_fragment &elem)
{
    if(is_queue_full() == true)
        return STATUS_DESTINATION_ELEMENT_FULL;

    frags[pidx] = elem;

    pidx++;
    if (pidx >= max_q_size)
        pidx = 0;

    avail_byte_cnt += elem.length;
    ++avail_frag_cnt;

    return STATUS_SUCCESS;
}

NTSTATUS st_c2h_pkt_frag_queue::consume(st_c2h_pkt_fragment &elem)
{
    if (is_queue_empty() == true)
        return STATUS_SOURCE_ELEMENT_EMPTY;

    elem = frags[cidx];

    cidx++;
    if (cidx >= max_q_size)
        cidx = 0;

    avail_byte_cnt -= elem.length;
    --avail_frag_cnt;

    return STATUS_SUCCESS;
}
/* ----- -----*/

PFORCEINLINE void ring_buffer::advance_idx(volatile UINT32& idx)
{
    NT_ASSERT(idx < capacity);

    /* This condition is to ensure that idx update should happen only once
       (either  roll back to 0 or increment by 1)
    */
    if ((idx + 1) == capacity)
        idx = 0;
    else
        idx++;
}

PFORCEINLINE void ring_buffer::advance_idx(
    volatile UINT32& idx,
    const UINT32 num)
{
    if ((idx + num) < capacity)
        idx = idx + num;
    else
        idx = num - (capacity - idx);
}

NTSTATUS ring_buffer::create(
    WDFDMAENABLER& dma_enabler,
    UINT32 num_entries,
    size_t desc_sz_bytes)
{
    const size_t buffer_sz = num_entries * desc_sz_bytes;

    /** Descriptor Capacity of the ring (does not include write back status entry)
        total ring size = capacity + 1;

        descriptor ring uses (capacity - 1) entries for queue full condition
      */
    capacity = num_entries - 1;
    hw_index = sw_index = 0u;
    stats.tot_desc_accepted = stats.tot_desc_dropped = stats.tot_desc_processed = 0;

    TraceVerbose(TRACE_QDMA, "Allocate DMA buffer size : %llu ring depth : %llu, capacity : %llu",
        buffer_sz, num_entries, capacity);

    auto status = WdfCommonBufferCreate(dma_enabler,
                                        buffer_sz,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &buffer);

    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "WdfCommonBufferCreate failed: %!STATUS!", status);
        return status;
    }

    buffer_va = WdfCommonBufferGetAlignedVirtualAddress(buffer);
    RtlZeroMemory(buffer_va, buffer_sz);

    wb_status = WDF_PTR_ADD_OFFSET_TYPE(buffer_va,
                                        capacity * desc_sz_bytes,
                                        wb_status_base *);

    wb_status->cidx = wb_status->pidx = 0;
    return status;
}

void ring_buffer::destroy(void)
{
    capacity = 0;
    buffer_va = nullptr;
    wb_status = nullptr;
    sw_index = 0;
    hw_index = 0;
    stats.tot_desc_accepted = 0;
    stats.tot_desc_processed = 0;
    stats.tot_desc_dropped = 0;
    if (buffer) {
        TraceVerbose(TRACE_QDMA, "Deleting buffer object");
        WdfObjectDelete(buffer);
        buffer = nullptr;
    }
}

PFORCEINLINE UINT32 ring_buffer::idx_delta(UINT32 start_idx, UINT32 end_idx)
{
    NT_ASSERT(start_idx < capacity);
    NT_ASSERT(end_idx < capacity);

    if (start_idx <= end_idx)
        return end_idx - start_idx;
    else
        return capacity - start_idx + end_idx;
}

void *ring_buffer::get_va(void)
{
    return buffer_va;
}

UINT32 ring_buffer::get_capacity(void)
{
    return capacity;
}

UINT32 ring_buffer::get_num_free_entries(void)
{
    /* As hw_index is proper 32-bit aligned address,
       reading and writing gets done in single cycle
       and hence not using any synchronization for performance
    */
    UINT32 wb_index = hw_index;

    if (wb_index > sw_index)
        return ((wb_index - sw_index) - 1);
    else
        return (capacity - 1 - (sw_index - wb_index));
}

NTSTATUS h2c_queue::create(
    qdma_device *qdma,
    queue_config& q_conf)
{
    user_conf = q_conf;

    if ((user_conf.desc_bypass_en) &&
        (user_conf.sw_desc_sz == static_cast<UINT8>(qdma_desc_sz::QDMA_DESC_SZ_64B))) {
        lib_config.desc_sz = qdma_desc_sz::QDMA_DESC_SZ_64B;
    }
    else {
        lib_config.desc_sz = user_conf.is_st ? qdma_desc_sz::QDMA_DESC_SZ_16B : qdma_desc_sz::QDMA_DESC_SZ_32B;
    }

    size_t desc_sz_bytes = get_descriptor_size(lib_config.desc_sz);
    lib_config.ring_sz = qdma->csr_conf.ring_sz[user_conf.h2c_ring_sz_index];

    auto status = desc_ring.create(qdma->dma_enabler,
                                   lib_config.ring_sz,
                                   desc_sz_bytes);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "desc_ring.create failed: %!STATUS!", status);
        return status;
    }

    init_csr_h2c_pidx_info();

    /** Create and initialize MM/ST_H2C request tracker of
      * descriptor ring size, This is shadow ring and hence the
      * size is exact descriptor ring size
      */
    status = req_tracker.create(lib_config.ring_sz);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_dma_req_tracker() failed: %!STATUS!", status);
        goto ErrExit;
    }

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "WdfSpinLockCreate() failed: %!STATUS!", status);
        goto ErrExit;
    }

#ifdef TEST_64B_DESC_BYPASS_FEATURE
    if (user_conf.is_st && user_conf.desc_bypass_en && user_conf.sw_desc_sz == 3) {
        TraceVerbose(TRACE_QDMA, "Initializing the descriptors with "
            "preloaded data to test TEST_64B_DESC_BYPASS_FEATURE");

        for (UINT32 desc_idx = 0; desc_idx < lib_config.ring_sz; desc_idx++) {
            UINT8 *desc =
                WDF_PTR_ADD_OFFSET_TYPE(desc_ring.buffer_va,
                    (desc_sz_bytes * desc_idx),
                    UINT8 *);
            RtlFillMemory(desc, desc_sz_bytes, desc_idx + 1);
        }
    }
#endif
    return status;

ErrExit:
    destroy();
    return status;
}

void h2c_queue::destroy(void)
{
    desc_ring.destroy();
    req_tracker.destroy();
    if (nullptr != lock) {
        WdfObjectDelete(lock);
        lock = nullptr;
    }
}

void h2c_queue::init_csr_h2c_pidx_info(void)
{
    csr_pidx_info.irq_en = lib_config.irq_en;
    csr_pidx_info.pidx = 0u;
}

NTSTATUS c2h_queue::create(
    qdma_device *qdma,
    queue_config& q_conf)
{
    user_conf = q_conf;

    if ((user_conf.desc_bypass_en) &&
        (user_conf.sw_desc_sz == static_cast<UINT8>(qdma_desc_sz::QDMA_DESC_SZ_64B))) {
        lib_config.desc_sz = qdma_desc_sz::QDMA_DESC_SZ_64B;
    }
    else {
        lib_config.desc_sz = user_conf.is_st ? qdma_desc_sz::QDMA_DESC_SZ_8B : qdma_desc_sz::QDMA_DESC_SZ_32B;
    }

    size_t desc_sz_bytes = get_descriptor_size(lib_config.desc_sz);
    lib_config.ring_sz = qdma->csr_conf.ring_sz[user_conf.c2h_ring_sz_index];

    INT32 cmpt_ring_idx =
        get_cmpt_ring_index(&qdma->csr_conf.ring_sz[0],
            user_conf.c2h_ring_sz_index);

    /** If cmpt_ring_idx is found, that is cmpt_ring index */
    if (cmpt_ring_idx >= 0)
        lib_config.cmpt_ring_id = cmpt_ring_idx;
    /** Else, cmpt_ring index is same as descriptor ring index */
    else
        lib_config.cmpt_ring_id = user_conf.c2h_ring_sz_index;

    lib_config.cmpt_ring_sz = qdma->csr_conf.ring_sz[lib_config.cmpt_ring_id];

    NTSTATUS status = desc_ring.create(qdma->dma_enabler, lib_config.ring_sz, desc_sz_bytes);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "desc_ring.create failed: %!STATUS!", status);
        return status;
    }

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "WdfSpinLockCreate failed: %!STATUS!", status);
        goto ErrExit;
    }

    if (user_conf.is_st) {
        /** Create and initialize ST C2H dma request tracker of
          * descriptor ring size to allow minimum of desc_ring entry requests
          * when each request occupies a single desc_ring entry
          */
        status = st_c2h_req_tracker.create(lib_config.ring_sz);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "Failed to init() ST C2H req tracker failed: %!STATUS!", status);
            goto ErrExit;
        }
    }
    else {
        /** Create and initialize MM/ST_H2C request tracker of
          * descriptor ring size, This is shadow ring and hence the
          * size is exact descriptor ring size
          */
        status = req_tracker.create(lib_config.ring_sz);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "Failed to init() MM C2H req tracker failed: %!STATUS!", status);
            goto ErrExit;
        }
    }

    init_csr_c2h_pidx_info();
    is_cmpt_valid = false;
    if ((user_conf.is_st) ||
        (!user_conf.is_st && user_conf.en_mm_cmpl && qdma->dev_conf.dev_info.mm_cmpt_en)) {
        /* Completion ring resources */
        is_cmpt_valid = true;
        size_t cmpt_sz_bytes = get_descriptor_size(user_conf.cmpt_sz);

        status = cmpt_ring.create(qdma->dma_enabler, lib_config.cmpt_ring_sz, cmpt_sz_bytes);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "desc_ring.create failed: %!STATUS!", status);
            goto ErrExit;
        }

        init_csr_cmpt_cidx_info();

        /* Identify completion offset */
        qdma_hw_version_info hw_version_info = { };
        qdma->qdma_get_hw_version_info(hw_version_info);
        if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
            /* UDD starts from index 2 for Versal Hard IP */
            cmpt_offset = 2;
        }
        else {
            cmpt_offset = 0;
        }
    }

    if (!user_conf.is_st)
        return status;

    lib_config.data_buf_size = qdma->csr_conf.c2h_buff_sz[user_conf.c2h_buff_sz_index];
    const auto c2h_desc_ring = static_cast<c2h_descriptor*>(desc_ring.get_va());

    auto desc_ring_capacity = desc_ring.get_capacity();
    pkt_buffer = static_cast<st_c2h_pkt_buffer *>(qdma_calloc(desc_ring_capacity,
                                                              sizeof(st_c2h_pkt_buffer)));

    if (nullptr == pkt_buffer) {
        TraceError(TRACE_QDMA, "rx_buffers qdma_calloc failed: %!STATUS!", status);
        status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrExit;
    }

    bool rx_contiguous_alloc;
    PVOID buff_va = nullptr;
    PHYSICAL_ADDRESS buff_dma;

    /* Try allocating contigous DMA common buffer */
    status = pkt_buffer[0].create(qdma->dma_enabler,
                                  (desc_ring_capacity * lib_config.data_buf_size));
    if (!NT_SUCCESS(status)) {
        rx_contiguous_alloc = false;
        no_allocated_rx_common_buffs = desc_ring_capacity;
        buff_dma.QuadPart = 0;
    }
    else {
        rx_contiguous_alloc = true;
        no_allocated_rx_common_buffs = 1;
        buff_va = pkt_buffer[0].get_va();
        buff_dma.QuadPart = pkt_buffer[0].get_dma_addr().QuadPart;
    }

    for (UINT32 i = 0; i < desc_ring_capacity; ++i) {
        if (true == rx_contiguous_alloc) {
            /* Split the contiguous buffer */
            pkt_buffer[i].fill_rx_buff(buff_va, buff_dma);

            buff_va = (PVOID)((UINT8 *)buff_va + lib_config.data_buf_size);
            buff_dma.QuadPart = buff_dma.QuadPart + lib_config.data_buf_size;
        }
        else {
            /* Allocate DMA buffers for Rx Ring */
            status = pkt_buffer[i].create(qdma->dma_enabler, lib_config.data_buf_size);
            if (!NT_SUCCESS(status)) {
                no_allocated_rx_common_buffs = i - 1;
                goto ErrExit;
            }
        }

        /* Fill DMA Address for QDMA Rx Descriptor Ring */
        c2h_desc_ring[i].addr = pkt_buffer[i].get_dma_addr().QuadPart;
    }

    status = pkt_frag_queue.create(lib_config.cmpt_ring_sz);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "received pkts init failed: %!STATUS!", status);
        goto ErrExit;
    }

    pkt_frag_list = static_cast<st_c2h_pkt_fragment*>(qdma_calloc(desc_ring_capacity,
                                                      sizeof(st_c2h_pkt_fragment)));
    if (nullptr == pkt_frag_list) {
        TraceError(TRACE_QDMA, "rx_buffers qdma_calloc failed: %!STATUS!", status);
        status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrExit;
    }

    return status;
ErrExit:
    destroy();
    return status;
}

void c2h_queue::destroy(void)
{
    desc_ring.destroy();

    if (lock) {
        WdfObjectDelete(lock);
        lock = nullptr;
    }

    cmpt_ring.destroy();
    is_cmpt_valid = false;

    if (user_conf.is_st) {
        if (pkt_buffer) {
            TraceVerbose(TRACE_QDMA, "Deleting ST Rx buffer objects");

            for (UINT32 i = 0u; i < no_allocated_rx_common_buffs; ++i) {
                pkt_buffer[i].destroy();
            }
            no_allocated_rx_common_buffs = 0;

            qdma_memfree(pkt_buffer);
            pkt_buffer = nullptr;
        }

        st_c2h_req_tracker.destroy();
        pkt_frag_queue.destroy();

        if (nullptr != pkt_frag_list) {
            qdma_memfree(pkt_frag_list);
            pkt_frag_list = nullptr;
        }
    }
    else {
        req_tracker.destroy();
    }
}

INT32 c2h_queue::get_cmpt_ring_index(UINT32* csr_ring_sz_table, UINT32 desc_ring_idx)
{
    UINT32 csr_idx;

    /** Completion Ring size deduction */
    int best_fit_idx = -1;
    for (csr_idx = 0; csr_idx < QDMA_GLOBAL_CSR_ARRAY_SZ; csr_idx++) {
        /** Check for ring size larger than descriptor ring size (lib_config.ring_sz) */
        if (csr_ring_sz_table[csr_idx] > csr_ring_sz_table[desc_ring_idx]) {

            /** best_fit_index is the index where its size is
              * next higher than descriptor ring size
              * (should be in one of the CSR ring size registers)
              */
            if (best_fit_idx < 0)
                best_fit_idx = csr_idx;
            else if ((best_fit_idx >= 0) &&
                (csr_ring_sz_table[csr_idx] <
                    csr_ring_sz_table[best_fit_idx]))
                best_fit_idx = csr_idx;
        }
    }

    return best_fit_idx;
}

void c2h_queue::init_csr_c2h_pidx_info(void)
{
    csr_pidx_info.irq_en = lib_config.irq_en;
    csr_pidx_info.pidx = 0u;
}

void c2h_queue::init_csr_cmpt_cidx_info(void)
{
    csr_cmpt_cidx_info.counter_idx = user_conf.c2h_th_cnt_index;
    csr_cmpt_cidx_info.irq_en = lib_config.irq_en;
    csr_cmpt_cidx_info.timer_idx = user_conf.c2h_timer_cnt_index;
    csr_cmpt_cidx_info.trig_mode = static_cast<uint8_t>(user_conf.trig_mode);
    csr_cmpt_cidx_info.wrb_en = true;
    csr_cmpt_cidx_info.wrb_cidx = 0u;
}

NTSTATUS dma_req_tracker::create(UINT32 entries)
{
    requests = static_cast<req_ctx *>(qdma_calloc(entries, sizeof(req_ctx)));
    if (nullptr == requests) {
        return STATUS_MEMORY_NOT_ALLOCATED;
    }

    return STATUS_SUCCESS;
}

void dma_req_tracker::destroy()
{
    if (nullptr != requests) {
        qdma_memfree(requests);
        requests = nullptr;
    }
}

NTSTATUS st_c2h_dma_req_tracker::create(UINT32 entries)
{
    requests = static_cast<st_c2h_req*>(qdma_calloc(entries, sizeof(st_c2h_req)));
    if (nullptr == requests) {
        return STATUS_MEMORY_NOT_ALLOCATED;
    }

    NTSTATUS status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "WdfSpinLockCreate failed: %!STATUS!", status);
        qdma_memfree(requests);
        return status;
    }

    capacity = entries;
    pidx = cidx = 0u;

    return STATUS_SUCCESS;
}

void st_c2h_dma_req_tracker::destroy(void)
{
    if (nullptr != requests) {
        qdma_memfree(requests);
        requests = nullptr;
    }

    if (nullptr != lock) {
        WdfObjectDelete(lock);
        lock = nullptr;
    }
}

NTSTATUS st_c2h_dma_req_tracker::st_push_dma_req(st_c2h_req& req)
{
    WdfSpinLockAcquire(lock);
    if (((pidx + 1) % capacity) == cidx) {
        WdfSpinLockRelease(lock);
        return STATUS_DESTINATION_ELEMENT_FULL;
    }

    requests[pidx].len = req.len;
    requests[pidx].priv = req.priv;
    requests[pidx].st_compl_cb = req.st_compl_cb;

    pidx = (pidx + 1) % capacity;

    WdfSpinLockRelease(lock);

    return STATUS_SUCCESS;
}

NTSTATUS st_c2h_dma_req_tracker::st_peek_dma_req(st_c2h_req& req)
{
    WdfSpinLockAcquire(lock);
    if (pidx == cidx) {
        WdfSpinLockRelease(lock);
        return STATUS_SOURCE_ELEMENT_EMPTY;
    }
    WdfSpinLockRelease(lock);

    /* Released the spin above, this call st_peek_dma_req is
       designed to use for sequential execution and hence
       is not thread safe
    */

    req.len = requests[cidx].len;
    req.priv = requests[cidx].priv;
    req.st_compl_cb = requests[cidx].st_compl_cb;

    return STATUS_SUCCESS;
}

NTSTATUS st_c2h_dma_req_tracker::st_pop_dma_req(void)
{
    WdfSpinLockAcquire(lock);
    if (pidx == cidx) {
        WdfSpinLockRelease(lock);
        return STATUS_SOURCE_ELEMENT_EMPTY;
    }
    WdfSpinLockRelease(lock);

    /* Released the spin above, this call st_peek_dma_req is
       designed to use for sequential execution and hence
       is not thread safe
    */
    requests[cidx].len = 0;
    requests[cidx].priv = nullptr;
    requests[cidx].st_compl_cb = nullptr;

    cidx = (cidx + 1) % capacity;
    return STATUS_SUCCESS;
}

inline NTSTATUS st_c2h_pkt_buffer::create(
    WDFDMAENABLER dma_enabler,
    size_t size)
{
    WDF_COMMON_BUFFER_CONFIG common_buff_config;
    WDF_COMMON_BUFFER_CONFIG_INIT(&common_buff_config, FILE_64_BYTE_ALIGNMENT);

    NTSTATUS status = WdfCommonBufferCreateWithConfig(dma_enabler,
        size,
        &common_buff_config,
        WDF_NO_OBJECT_ATTRIBUTES,
        &rx_buff_common);

    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "WdfCommonBufferCreate failed %!STATUS!", status);
        return status;
    }

    rx_buff_va = WdfCommonBufferGetAlignedVirtualAddress(rx_buff_common);
    rx_buff_dma = WdfCommonBufferGetAlignedLogicalAddress(rx_buff_common);

    RtlZeroMemory(rx_buff_va, size);

    return status;
}

inline void st_c2h_pkt_buffer::destroy(void)
{
    WdfObjectDelete(rx_buff_common);
}

inline void st_c2h_pkt_buffer::fill_rx_buff(PVOID buff_va, PHYSICAL_ADDRESS buff_dma)
{
    rx_buff_va = buff_va;
    rx_buff_dma = buff_dma;
}

inline PHYSICAL_ADDRESS st_c2h_pkt_buffer::get_dma_addr(void)
{
    return rx_buff_dma;
}

inline PVOID st_c2h_pkt_buffer::get_va(void)
{
    return rx_buff_va;
}

NTSTATUS queue_pair::create(
    queue_config& conf)
{
    NTSTATUS status;

    type = conf.is_st ? queue_type::STREAMING : queue_type::MEMORY_MAPPED;

    /* h2c common buffer */
    status = h2c_q.create(qdma, conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "h2c_q.create failed: %!STATUS!", status);
        return status;
    }

    /* c2h common buffer */
    status = c2h_q.create(qdma, conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "c2h_q.create failed: %!STATUS!", status);
        h2c_q.destroy();
        return status;
    }

    return status;
}

void queue_pair::destroy(void)
{
    h2c_q.destroy();
    c2h_q.destroy();
}

void queue_pair::init_csr(void)
{
    if (c2h_q.is_cmpt_valid == true) {
        update_sw_index_with_csr_h2c_pidx(0);
        update_sw_index_with_csr_wb(0);
        update_sw_index_with_csr_c2h_pidx(static_cast<UINT32>(c2h_q.desc_ring.capacity - 1));
    }
    else {
        update_sw_index_with_csr_h2c_pidx(0);
        update_sw_index_with_csr_c2h_pidx(0);
    }
}

PFORCEINLINE void queue_pair::update_sw_index_with_csr_h2c_pidx(UINT32 new_pidx)
{
    TraceVerbose(TRACE_QDMA, "queue_%u updating h2c pidx to %u", idx_abs, new_pidx);

    MemoryBarrier();
    h2c_q.desc_ring.sw_index = new_pidx;
    h2c_q.csr_pidx_info.pidx = (UINT16)new_pidx;
    qdma->hw.qdma_queue_pidx_update(qdma, false /* is_vf */,
                                    idx, false /* is_c2h */,
                                    &h2c_q.csr_pidx_info);
}

PFORCEINLINE void queue_pair::update_sw_index_with_csr_c2h_pidx(UINT32 new_pidx)
{
    TraceVerbose(TRACE_QDMA, "queue_%u updating c2h pidx to %u", idx_abs, new_pidx);

    MemoryBarrier();
    c2h_q.desc_ring.sw_index = new_pidx;
    c2h_q.csr_pidx_info.pidx = (UINT16)new_pidx;
    qdma->hw.qdma_queue_pidx_update(qdma, false /* is_vf */,
                                    idx, true /* is_c2h */,
                                    &c2h_q.csr_pidx_info);
}

PFORCEINLINE void queue_pair::update_sw_index_with_csr_wb(UINT32 new_cidx)
{
    TraceVerbose(TRACE_QDMA, "queue_%u updating wb cidx to %u", idx_abs, new_cidx);

    c2h_q.cmpt_ring.sw_index = new_cidx;
    c2h_q.csr_cmpt_cidx_info.wrb_cidx = (UINT16)new_cidx;
    qdma->hw.qdma_queue_cmpt_cidx_update(qdma, false, idx,
                                         &c2h_q.csr_cmpt_cidx_info);
}

service_status queue_pair::service_mm_st_h2c_completions(ring_buffer *desc_ring, dma_req_tracker *tracker, UINT32 budget)
{
    UINT32 old_cidx = desc_ring->hw_index;
    UINT32 new_cidx = desc_ring->wb_status->cidx;
    UINT32 index;

    /** Complete requests covered by the descriptors in the interval
      * [old_cidx, new_cidx] */
    for (index = old_cidx;
        ((index != new_cidx) && (budget > 0));
        desc_ring->advance_idx(index), budget--) {
        req_ctx &completed_req = tracker->requests[index];

        if (completed_req.compl_cb != nullptr) {
            TraceVerbose(TRACE_QDMA, "--- COMPLETING REQ AT : %u ---", index);
            completed_req.compl_cb(completed_req.priv, STATUS_SUCCESS);
            completed_req.compl_cb = nullptr;
            completed_req.priv = nullptr;
        }
    }

    desc_ring->stats.tot_desc_processed +=
        desc_ring->idx_delta(index, old_cidx);

    /* Update stored copy of the ring's consumer index */
    desc_ring->hw_index = index;

    if (index != new_cidx || desc_ring->sw_index != new_cidx) {
        /** Ran out of completion budget or still have pending requests.
          * In interrupt mode while ISR is running, driver might miss the
          * new interrupts. So, check if any pending jobs are for ring to
          * continue polling service.
          */
        return service_status::SERVICE_CONTINUE;
    }
    else {
        return service_status::SERVICE_FINISHED;
    }
}

NTSTATUS queue_pair::check_cmpt_error(c2h_wb_header_8B *cmpt_data)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (cmpt_data->data_frmt) {
        /**
          * format = 1 does not have length field, so the driver cannot
          * figure out how many descriptor is used
          */
        status = STATUS_UNSUCCESSFUL;
    }

    if (cmpt_data->desc_error) {
        /**
          * descriptor has error
          */
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

PFORCEINLINE void queue_pair::update_c2h_pidx_in_batch(UINT32 processed_desc_cnt)
{
    TraceVerbose(TRACE_QDMA, "Completed desc count qid[%u] : %ld", processed_desc_cnt, idx_abs);
    if (processed_desc_cnt) {
        UINT32 new_pidx = c2h_q.desc_ring.sw_index;
        c2h_q.desc_ring.advance_idx(new_pidx, processed_desc_cnt);
        update_sw_index_with_csr_c2h_pidx(new_pidx);
    }
}

service_status queue_pair::st_service_c2h_queue(UINT32 budget)
{
    NTSTATUS status;
    service_status ret = service_status::SERVICE_FINISHED;
    ring_buffer *cmpt_ring = &c2h_q.cmpt_ring;
    const auto wb_status = reinterpret_cast<volatile c2h_wb_status*>(cmpt_ring->wb_status);
    auto current_pidx = wb_status->pidx;
    auto prev_pidx = c2h_q.cmpt_ring.sw_index;

    /* Process WB ring */
    if (prev_pidx != current_pidx) {
        TraceVerbose(TRACE_QDMA, "Completion ring sw index : %u hw pidx : %u", prev_pidx, current_pidx);
        dump(*const_cast<c2h_wb_status*>(wb_status));

        c2h_wb_header_8B *c2h_cmpt_ring_va;
        UINT32 wb_desc_shift = (3 + static_cast<size_t>(c2h_q.user_conf.cmpt_sz));
        const auto c2h_cmpt_ring_base_va = static_cast<c2h_wb_header_8B*>(c2h_q.cmpt_ring.get_va());

        for (; prev_pidx != current_pidx; c2h_q.cmpt_ring.advance_idx(prev_pidx)) {
            c2h_cmpt_ring_va = (c2h_wb_header_8B *)((UINT8 *)c2h_cmpt_ring_base_va + ((UINT64)prev_pidx << wb_desc_shift));

            dump(prev_pidx, *c2h_cmpt_ring_va);

            /** Check completion descriptor for any errors */
            status = check_cmpt_error(c2h_cmpt_ring_va);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_QDMA, "check_cmpt_error returned error\n");
                return service_status::SERVICE_ERROR;
            }

            if (c2h_cmpt_ring_va->desc_used) {
                /* Consume the used packet */
                status = process_st_c2h_data_pkt((void *)c2h_cmpt_ring_va, c2h_cmpt_ring_va->length);
                if (!NT_SUCCESS(status)) {
                    TraceError(TRACE_QDMA, "process_st_c2h_data_pkt() failed: %!STATUS!", status);
                    break;
                }
            }
            else {
                /* If user provides a call back function during queue addition,
                 * callback function will be called and user can process the UDD data,
                 * else, data will be discarded
                 *
                 * This is UDD_ONLY packets, i.e, data is not associated with this completion
                 * Data associated UDD packets will be given as part of read requests
                 */
                if (c2h_q.user_conf.proc_st_udd_cb)
                    c2h_q.user_conf.proc_st_udd_cb(idx, (void *)c2h_cmpt_ring_va, (void *)qdma);
            }
        }

        update_sw_index_with_csr_wb(prev_pidx);
    }

    /* Process pending ST DMA Requests */
    UINT32 processed_dma_requests = 0;
    st_c2h_req req = { 0 };
    UINT32 processed_desc_cnt = 0;
    constexpr UINT8 pidx_update_interval = 16;

    /** For ST C2H Queues, budget acts as Upper cut-off for
      * the dma requests from user that are pending for completion */
    while (processed_dma_requests < budget) {
        status = c2h_q.st_c2h_req_tracker.st_peek_dma_req(req);
        if (!NT_SUCCESS(status)) {
            /* No more pending requests, Update PIDX and break the loop */
            update_c2h_pidx_in_batch(processed_desc_cnt);
            LONG pkt_cnt = c2h_q.pkt_frag_queue.get_avail_frag_cnt();
            if (pkt_cnt) {
                TraceVerbose(TRACE_QDMA, "No pending requests. Available Packets : %Iu", pkt_cnt);
                ret = service_status::SERVICE_CONTINUE;
            }
            break;
        }

        if (req.len <= c2h_q.pkt_frag_queue.get_avail_byte_cnt()) {
            UINT32 n = 0;
            /* For Zero length packets */
            if (req.len == 0) {
                status = c2h_q.pkt_frag_queue.consume(c2h_q.pkt_frag_list[n]);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                n++;
            }
            else {
                size_t length;
                for (n = 0u, length = 0u; ((n < c2h_q.desc_ring.get_capacity()) &&
                    (length < req.len)); ++n) {
                    status = c2h_q.pkt_frag_queue.consume(c2h_q.pkt_frag_list[n]);
                    if (!NT_SUCCESS(status)) {
                        break;
                    }
                    length += c2h_q.pkt_frag_list[n].length;
                }
            }

            if (req.st_compl_cb != nullptr) {
                req.st_compl_cb(c2h_q.pkt_frag_list, n, req.priv, status);
                req.st_compl_cb = nullptr;
            }

            c2h_q.st_c2h_req_tracker.st_pop_dma_req();
            ++processed_dma_requests;
            processed_desc_cnt += n;
            c2h_q.desc_ring.stats.tot_desc_processed += n;

            if (processed_desc_cnt >= pidx_update_interval) {
                /* Return ST Buffers to desc ring */
                update_c2h_pidx_in_batch(processed_desc_cnt);
                processed_desc_cnt = 0;
            }
        }
        else {
            update_c2h_pidx_in_batch(processed_desc_cnt);
            ret = service_status::SERVICE_CONTINUE;
            break;
        }
    }

    TraceVerbose(TRACE_QDMA, "Completed Request count : %ld", processed_dma_requests);
    return ret;
}

void service_h2c_mm_queue(void *arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = mm_h2c_completion_weight;
    ring_buffer *desc_ring = &queue->h2c_q.desc_ring;
    dma_req_tracker *tracker = &queue->h2c_q.req_tracker;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        wakeup_thread(queue->h2c_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "MM H2C [%u] service error", queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        TraceVerbose(TRACE_QDMA, "service_h2c_mm_queue [%u] Completed", queue->idx_abs);
    default:
        break;
    }
}

void service_c2h_mm_queue(void *arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = mm_c2h_completion_weight;
    ring_buffer* desc_ring = &queue->c2h_q.desc_ring;
    dma_req_tracker* tracker = &queue->c2h_q.req_tracker;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        wakeup_thread(queue->c2h_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "MM C2H [%u] service error", queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        TraceVerbose(TRACE_QDMA, "service_c2h_mm_queue [%u] Completed", queue->idx_abs);
    default:
        break;
    }
}

void service_h2c_st_queue(void *arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = st_h2c_completion_weight;
    ring_buffer* desc_ring = &queue->h2c_q.desc_ring;
    dma_req_tracker* tracker = &queue->h2c_q.req_tracker;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        wakeup_thread(queue->h2c_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "ST H2C [%u] service error", queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        TraceVerbose(TRACE_QDMA, "service_h2c_st_queue [%u] Completed", queue->idx_abs);
    default:
        break;
    }
}

void service_c2h_st_queue(void *arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = st_c2h_completion_weight;
    service_status status = queue->st_service_c2h_queue(budget);

    switch (status) {
    case service_status::SERVICE_CONTINUE:
        wakeup_thread(queue->c2h_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "ST C2H [%u] service error", queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        TraceVerbose(TRACE_QDMA, "service_c2h_st_queue [%u] Completed", queue->idx_abs);
    default:
        break;
    }
}

NTSTATUS queue_pair::enqueue_mm_request(
    const WDF_DMA_DIRECTION direction,
    PSCATTER_GATHER_LIST sg_list,
    LONGLONG device_offset,
    dma_completion_cb compl_cb,
    VOID *priv,
    size_t &xfered_len)
{
    bool poll;
    ring_buffer *desc_ring;
    dma_req_tracker *tracker;
    WDFSPINLOCK lock;
    ULONG no_of_sub_elem = 0;
    UINT32 credits = 0;

    if (direction == WdfDmaDirectionWriteToDevice) {
        desc_ring = &h2c_q.desc_ring;
        tracker = &h2c_q.req_tracker;
        poll = !h2c_q.lib_config.irq_en;
        lock = h2c_q.lock;
    }
    else {
        desc_ring = &c2h_q.desc_ring;
        tracker = &c2h_q.req_tracker;
        lock = c2h_q.lock;
        poll = !c2h_q.lib_config.irq_en;
    }

    WdfSpinLockAcquire(lock);

    credits = desc_ring->get_num_free_entries();
    if (credits == 0) {
        TraceError(TRACE_QDMA, "No space [%u] in sg dma list", credits);
        WdfSpinLockRelease(lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UINT32 ring_idx = desc_ring->sw_index;
    bool is_credit_avail = true;
    size_t dmaed_len = 0;

    TraceVerbose(TRACE_QDMA, "queue_%u_mm enqueueing %u sg list at ring_index=%u FreeDesc : %u",
        idx_abs, sg_list->NumberOfElements, ring_idx, credits);

    const auto desc = static_cast<mm_descriptor*>(desc_ring->get_va());

    for (ULONG i = 0;
        ((i < sg_list->NumberOfElements) && (is_credit_avail)); i++) {

        size_t len = 0;
        size_t remain_len = sg_list->Elements[i].Length;
        size_t frag_len = mm_max_desc_data_len;

        if (sg_list->Elements[i].Length > mm_max_desc_data_len) {
            frag_len = sg_frag_len;
            TraceInfo(TRACE_QDMA, "sg_list len > mm_max_desc_data_len(%lld > %lld), "
                "splitting at idx: %d", sg_list->Elements[i].Length, mm_max_desc_data_len, i);
        }

        while (remain_len != 0) {
            size_t part_len = min(remain_len, frag_len);

            desc[ring_idx].length = part_len;
            desc[ring_idx].valid = true;
            desc[ring_idx].sop = (i == 0);
            desc[ring_idx].eop = (i == (sg_list->NumberOfElements - 1)) && (remain_len == part_len);

            if (direction == WdfDmaDirectionWriteToDevice) {
                desc[ring_idx].addr = sg_list->Elements[i].Address.QuadPart + len;
                desc[ring_idx].dest_addr = device_offset;
            }
            else {
                desc[ring_idx].addr = device_offset;
                desc[ring_idx].dest_addr = sg_list->Elements[i].Address.QuadPart + len;
            }

            len = len + part_len;
            remain_len = remain_len - part_len;

            device_offset += part_len;
            dump(desc[ring_idx]);

            tracker->requests[ring_idx].compl_cb = nullptr;
            /** Set Call back function if EOP is SET or if it is last free descriptor in ring */
            if ((desc[ring_idx].eop == true) || (credits == 1)) {
                tracker->requests[ring_idx].compl_cb = (dma_completion_cb)compl_cb;
                tracker->requests[ring_idx].priv = priv;
                desc[ring_idx].eop = true;
            }

            desc_ring->advance_idx(ring_idx);

            no_of_sub_elem++;
            /** Descriptor ring boundary checking */
            credits--;
            if (credits == 0) {
                is_credit_avail = false;
                break;
            }
        }
        /** Discard the main element count in sg_list->NumberOfElements */
        no_of_sub_elem--;
        dmaed_len += len;
    }

    UINT32 accepted_desc = desc_ring->idx_delta(desc_ring->sw_index, ring_idx);
    UINT32 dropped_desc = (sg_list->NumberOfElements + no_of_sub_elem - accepted_desc);
    desc_ring->stats.tot_desc_accepted += accepted_desc;
    desc_ring->stats.tot_desc_dropped += dropped_desc;
    xfered_len = dmaed_len;

    TraceVerbose(TRACE_QDMA, "+++ Request added at : %u +++", (ring_idx - 1));
    TraceVerbose(TRACE_QDMA, "MM Accepted Desc : %d, Dropped Desc : %d", accepted_desc, dropped_desc);

#ifdef STRESS_TEST
    for (unsigned long i = 0; i < (1024 * 1024); i++);
#endif
    if (direction == WdfDmaDirectionWriteToDevice) {
        update_sw_index_with_csr_h2c_pidx(ring_idx);
        TraceVerbose(TRACE_QDMA, "csr[%u].h2c_dsc_pidx=%u", idx_abs, ring_idx);
    }
    else {
        update_sw_index_with_csr_c2h_pidx(ring_idx);
        TraceVerbose(TRACE_QDMA, "csr[%u].c2h_dsc_pidx=%u", idx_abs, ring_idx);
    }

    WdfSpinLockRelease(lock);

    if (poll) {
        wakeup_thread((direction == WdfDmaDirectionWriteToDevice) ? h2c_q.poll_entry->thread : c2h_q.poll_entry->thread);
    }

    return STATUS_SUCCESS;
}

NTSTATUS queue_pair::enqueue_st_tx_request(
    PSCATTER_GATHER_LIST sg_list,
    dma_completion_cb compl_cb,
    VOID *priv,
    size_t &xfered_len)
{
    auto desc_ring = &h2c_q.desc_ring;
    bool poll = !h2c_q.lib_config.irq_en;
    dma_req_tracker *tracker = &h2c_q.req_tracker;
    ULONG NumberOfElements;
    ULONG no_of_sub_elem = 0;
    UINT32 credits;

    NumberOfElements = sg_list->NumberOfElements;

    WdfSpinLockAcquire(h2c_q.lock);

    credits = desc_ring->get_num_free_entries();
    if (credits == 0) {
        TraceError(TRACE_QDMA, "No space [%u] in sg dma list", credits);
        WdfSpinLockRelease(h2c_q.lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    auto pidx = desc_ring->sw_index;
    const auto desc = static_cast<h2c_descriptor *>(desc_ring->get_va());
    bool is_credit_avail = true;
    size_t dmaed_len = 0;

    TraceVerbose(TRACE_QDMA, "queue_st_%u enqueueing %u sg list at ring_index=%u FreeDesc : %u",
        idx_abs, NumberOfElements, pidx, credits);

    for (ULONG i = 0; ((i < NumberOfElements) && (is_credit_avail)); i++) {
        size_t len = 0;
        size_t remain_len = sg_list->Elements[i].Length;
        size_t frag_len = st_max_desc_data_len;
        /* If the SG element length is more than ST supported len,
            * then Fragment the SG element into sub SG elements */
        if (sg_list->Elements[i].Length > st_max_desc_data_len) {
            frag_len = sg_frag_len;
            TraceInfo(TRACE_QDMA, "sg_list len > st_max_desc_data_len(%lld > %lld), "
                "splitting at idx: %d", sg_list->Elements[i].Length, st_max_desc_data_len, i);
        }

        /** do while loop is to support ST Zero length packet transfers */
        do {
            size_t part_len = min(remain_len, frag_len);

            desc[pidx].addr = sg_list->Elements[i].Address.QuadPart + len;
            desc[pidx].length = (UINT16)part_len;
            desc[pidx].pld_len = (UINT16)part_len;
            desc[pidx].sop = (i == 0);
            desc[pidx].eop = (i == (NumberOfElements - 1) && (remain_len == part_len));

            len = len + part_len;
            remain_len = remain_len - part_len;

            dump(desc[pidx]);

            tracker->requests[pidx].compl_cb = nullptr;
            /** Set Call back function if EOP is SET or if it is last free descriptor in ring */
            if ((desc[pidx].eop == true) || (credits == 1)) {
                tracker->requests[pidx].compl_cb = (dma_completion_cb)compl_cb;
                tracker->requests[pidx].priv = priv;
                desc[pidx].eop = true;
            }

            desc_ring->advance_idx(pidx);

            no_of_sub_elem++;
            /** Descriptor ring boundary checking */
            credits--;
            if (credits == 0) {
                is_credit_avail = false;
                break;
            }
        } while (remain_len != 0);
        /** Discard the main element count in sg_list->NumberOfElements */
        no_of_sub_elem--;
        dmaed_len += len;
    }

    UINT32 accepted_desc = desc_ring->idx_delta(desc_ring->sw_index, pidx);
    UINT32 dropped_desc = (NumberOfElements + no_of_sub_elem - accepted_desc);
    desc_ring->stats.tot_desc_accepted += accepted_desc;
    desc_ring->stats.tot_desc_dropped += dropped_desc;
    xfered_len = dmaed_len;

    TraceVerbose(TRACE_QDMA, "+++ Request added at : %u +++", (pidx - 1));
    TraceVerbose(TRACE_QDMA, "ST Accepted Desc : %d, Dropped Desc : %d", accepted_desc, dropped_desc);

    update_sw_index_with_csr_h2c_pidx(pidx);
    TraceVerbose(TRACE_QDMA, "csr[%u].h2c_dsc_pidx=%u", idx_abs, pidx);

    WdfSpinLockRelease(h2c_q.lock);

    if (poll)
        wakeup_thread(h2c_q.poll_entry->thread);

    return STATUS_SUCCESS;
}

NTSTATUS queue_pair::enqueue_st_rx_request(
    size_t length,
    st_rx_completion_cb compl_cb,
    void *priv)
{
    bool poll = !c2h_q.lib_config.irq_en;
    st_c2h_req req;
    req.len = length;
    req.priv = priv;
    req.st_compl_cb = compl_cb;

    NTSTATUS status = c2h_q.st_c2h_req_tracker.st_push_dma_req(req);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to add ST C2H request  %!STATUS!", status);
        return status;
    }

    if (poll) {
        wakeup_thread(c2h_q.poll_entry->thread);
    }

    return status;
}

NTSTATUS queue_pair::process_st_c2h_data_pkt(
    void *udd_ptr,
    const UINT32 length)
{
    st_c2h_pkt_fragment elem;
    UINT32 data_buff_sz = c2h_q.lib_config.data_buf_size;
    UINT32 remain_len = length;
    auto num_descriptors = (length + data_buff_sz - 1) / data_buff_sz;
    auto& c2h_hw_idx = c2h_q.desc_ring.hw_index;
    NTSTATUS status = STATUS_SUCCESS;

    /* For Zero Length Packet */
    if (length == 0) {
        num_descriptors = 1;
    }

    for (UINT32 i = 0; i < num_descriptors; ++i) {
        elem.pkt_type = st_c2h_pkt_type::ST_C2H_DATA_PKT;
        elem.data = c2h_q.pkt_buffer[c2h_hw_idx].get_va();
        elem.length = min(remain_len, data_buff_sz);
        remain_len -= data_buff_sz;

        if (i == (UINT32)0) {
            elem.udd_data = udd_ptr;
            elem.sop = 1;
            elem.eop = 0;
        }
        else {
            elem.udd_data = nullptr;
            elem.sop = elem.eop = 0 ;
        }

        elem.eop = (i == num_descriptors - 1) ? 1 : 0;

        status = c2h_q.pkt_frag_queue.add(elem);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "add fragment failed: %!STATUS!", status);
            return status;
        }

        c2h_q.desc_ring.advance_idx(c2h_hw_idx);
    }

    return status;
}

void queue_pair::cancel_mm_st_h2c_pending_requests(
    ring_buffer *desc_ring,
    dma_req_tracker *tracker,
    WDFSPINLOCK lock)
{
    /** From hw_index to sw_index [all pending requests in queue],
      * cancel all the requests for given descriptor ring */

    /* Acquire spin lock for rundown protection */
    WdfSpinLockAcquire(lock);
    for (UINT32 index = desc_ring->hw_index;
        index != desc_ring->sw_index; desc_ring->advance_idx(index)) {
        if (tracker->requests[index].compl_cb != nullptr) {
            dma_completion_cb compl_cb = tracker->requests[index].compl_cb;
            compl_cb(tracker->requests[index].priv, STATUS_CANCELLED);
            tracker->requests[index].compl_cb = nullptr;
            tracker->requests[index].priv = nullptr;
        }
    }
    WdfSpinLockRelease(lock);
}

void queue_pair::flush_queue(void)
{
    /* Cancel all the pending requests */
    cancel_mm_st_h2c_pending_requests(&h2c_q.desc_ring, &h2c_q.req_tracker, h2c_q.lock);
    if (type == queue_type::MEMORY_MAPPED) {
        cancel_mm_st_h2c_pending_requests(&c2h_q.desc_ring, &c2h_q.req_tracker, c2h_q.lock);
    }
    else {
        /** Cancelling ST C2H pending requests */
        st_c2h_req req;
        while (NT_SUCCESS(c2h_q.st_c2h_req_tracker.st_peek_dma_req(req))) {
            if (req.st_compl_cb != nullptr) {
                req.st_compl_cb(nullptr, 0, req.priv, STATUS_CANCELLED);
                req.st_compl_cb = nullptr;
            }
            c2h_q.st_c2h_req_tracker.st_pop_dma_req();
        }
    }
}

NTSTATUS queue_pair::read_st_udd_data(void *addr, UINT8 *buf, UINT32 *len)
{
    UINT32 udd_len = (UINT32)get_descriptor_size(c2h_q.user_conf.cmpt_sz);
    UINT8 *cmpt = (UINT8 *)addr;
    UINT8 loc = 0;
    UINT32 index = c2h_q.cmpt_offset;

    if ((nullptr == addr) || (nullptr == buf) || (nullptr == len))
        return STATUS_INVALID_PARAMETER;

    for (loc = 0; index < udd_len; loc++, index++) {
        if (loc == 0) {
            buf[loc] = cmpt[index] & 0xF0;
        }
        else {
            buf[loc] = cmpt[index];
        }
    }

    *len = loc;

    TraceVerbose(TRACE_QDMA, "UDD Len : %u", loc);

    return STATUS_SUCCESS;
}

void * queue_pair::get_last_udd_addr(void)
{
    UINT8 *udd_addr = nullptr;
    UINT32 udd_len = (UINT32)get_descriptor_size(c2h_q.user_conf.cmpt_sz);
    const auto c2h_cmpt_ring_va = static_cast<c2h_wb_header_8B*>(c2h_q.cmpt_ring.get_va());

    volatile wb_status_base *wb_status = c2h_q.cmpt_ring.wb_status;
    UINT16 latest_pidx = wb_status->pidx;

    if (latest_pidx == 0)
        latest_pidx = (UINT16)(c2h_q.cmpt_ring.get_capacity() - 1);
    else
        latest_pidx = latest_pidx - 1;

    udd_addr = (UINT8 *)c2h_cmpt_ring_va + ((UINT64)latest_pidx * udd_len);

    return udd_addr;
}

NTSTATUS queue_pair::desc_dump(qdma_desc_info *desc_info)
{
    size_t desc_sz;
    size_t desc_len;
    size_t len;
    size_t buf_idx;
    ring_buffer *desc_ring;
    UINT8 *desc_buff_addr;
    UINT8 *buf = desc_info->pbuffer;
    size_t buff_sz = desc_info->buffer_sz;
    WDFSPINLOCK lock;


    if (desc_info->desc_type == qdma_desc_type::CMPT_DESCRIPTOR) {
        if (c2h_q.is_cmpt_valid == true) {
            desc_ring = &c2h_q.cmpt_ring;
            desc_sz = (size_t)c2h_q.user_conf.cmpt_sz;
            lock = c2h_q.lock;
        }
        else {
            TraceError(TRACE_QDMA, "MM CMPT is not valid, HW MM CMPT EN : %d, SW MM CMPT EN : %d\n",
                qdma->dev_conf.dev_info.mm_cmpt_en, c2h_q.user_conf.en_mm_cmpl);
            return STATUS_NOT_SUPPORTED;
        }
    }
    else {
        if (desc_info->dir == qdma_queue_dir::QDMA_QUEUE_DIR_H2C) {
            desc_ring = &h2c_q.desc_ring;
            desc_sz = static_cast<size_t>(h2c_q.lib_config.desc_sz);
            lock = h2c_q.lock;
        }
        else {
            desc_ring = &c2h_q.desc_ring;
            desc_sz = static_cast<size_t>(c2h_q.lib_config.desc_sz);
            lock = c2h_q.lock;
        }
    }

    if ((desc_info->desc_start > desc_ring->get_capacity()) ||
        (desc_info->desc_end > desc_ring->get_capacity())) {
        TraceError(TRACE_QDMA, "Descriptor Range is incorrect : start : %d, end : %d, RING SIZE : %d",
            desc_info->desc_start, desc_info->desc_end, desc_ring->get_capacity());
        return STATUS_ACCESS_VIOLATION;
    }

    desc_len = (static_cast<size_t>(1) << (3 + desc_sz));

    desc_info->desc_sz = desc_len;

    /** This lock is to prevent queue stop/delete operations when descriptor
      * data is captured. There is no guarantee that the descriptor data
      * won't change (Ex: completion entries are filled by HW).
      *
      * This will grab instantaneous information present in the descriptors
      */
    WdfSpinLockAcquire(lock);
    if (desc_info->desc_start <= desc_info->desc_end) {
        len = ((size_t)desc_info->desc_end - (size_t)desc_info->desc_start + 1) * desc_len;
        if (buff_sz < len) {
            WdfSpinLockRelease(lock);
            return STATUS_BUFFER_TOO_SMALL;
        }

        buf_idx = desc_info->desc_start * desc_len;
        desc_buff_addr = &((UINT8 *)desc_ring->get_va())[buf_idx];
        RtlCopyMemory(buf, desc_buff_addr, len);
        desc_info->data_sz = len;
    }
    else {
        len = ((size_t)desc_ring->get_capacity() - (size_t)desc_info->desc_start + 1) * desc_len;
        if (buff_sz < len) {
            WdfSpinLockRelease(lock);
            return STATUS_BUFFER_TOO_SMALL;
        }

        buf_idx = desc_info->desc_start * desc_len;
        desc_buff_addr = &((UINT8 *)desc_ring->get_va())[buf_idx];
        RtlCopyMemory(buf, desc_buff_addr, len);
        desc_info->data_sz = len;

        size_t remain_len = ((size_t)desc_info->desc_end + 1) * desc_len;
        if ((buff_sz - len) < remain_len) {
            WdfSpinLockRelease(lock);
            return STATUS_BUFFER_TOO_SMALL;
        }

        desc_buff_addr = &((UINT8 *)desc_ring->get_va())[0];
        RtlCopyMemory(&buf[desc_info->data_sz], desc_buff_addr, len);
        desc_info->data_sz += len;
    }
    WdfSpinLockRelease(lock);

    return STATUS_SUCCESS;
}

NTSTATUS queue_pair::read_mm_cmpt_data(qdma_cmpt_info *cmpt_info)
{
    if (type != queue_type::MEMORY_MAPPED) {
        TraceError(TRACE_QDMA, "read_mm_cmpt_data supported only for MM queues");
        return STATUS_NOT_SUPPORTED;
    }

    if (!c2h_q.is_cmpt_valid) {
        TraceError(TRACE_QDMA, "MM Completion is not enabled for the queue : %d, "
            "HW En Status : %d, SW En Status : %d", cmpt_info->qid,
            qdma->dev_conf.dev_info.mm_cmpt_en, c2h_q.user_conf.en_mm_cmpl);

        return STATUS_NOT_CAPABLE;
    }

    WdfSpinLockAcquire(c2h_q.lock);
    const auto wb_status = reinterpret_cast<volatile c2h_wb_status*>(c2h_q.cmpt_ring.wb_status);
    auto prev_pidx = c2h_q.cmpt_ring.sw_index;
    auto current_pidx = wb_status->pidx;

    if (prev_pidx == current_pidx) {
        TraceError(TRACE_QDMA, "No Completion data available : "
            "Prev PIDX : %d, Pres PIDX : %d", prev_pidx, current_pidx);
        WdfSpinLockRelease(c2h_q.lock);
        return STATUS_SOURCE_ELEMENT_EMPTY;
    }

    UINT8 *dest_buff = cmpt_info->pbuffer;
    size_t index = 0;
    const UINT8 *src_buff = static_cast<UINT8 *>(c2h_q.cmpt_ring.get_va());
    size_t desc_sz = get_descriptor_size(c2h_q.user_conf.cmpt_sz);;

    for (; prev_pidx != current_pidx; c2h_q.cmpt_ring.advance_idx(prev_pidx)) {

        if ((index >= cmpt_info->buffer_len) || ((index + desc_sz) > cmpt_info->buffer_len)) {
            break;
        }

        memcpy_s(&dest_buff[index], (cmpt_info->buffer_len - index), &src_buff[prev_pidx * desc_sz], desc_sz);
        /** The first 4 bits are example design specific and
          * which contains Err, color, two user logic bits.
          *
          * Its the user design choice on the format of the completion data and
          * This part of the code is to demonstrate the MM CMPT functionality
          */
        dest_buff[index] = dest_buff[index] & 0xF0;
        index = index + desc_sz;
    }

    cmpt_info->ret_len = index;
    cmpt_info->cmpt_desc_sz = desc_sz;

    update_sw_index_with_csr_wb(prev_pidx);

    WdfSpinLockRelease(c2h_q.lock);

    return STATUS_SUCCESS;
}

//======================= indirect programming ====================================================

NTSTATUS qdma_device::clear_queue_contexts(bool is_c2h, UINT16 qid, qdma_hw_access_type context_op) const
{
    const bool irq_enable = (op_mode == queue_op_mode::POLL_MODE) ? false : true;
    int ret = hw.qdma_sw_ctx_conf((void *)this, is_c2h, qid, nullptr, context_op);
    if (ret < 0)
        return hw.qdma_get_error_code(ret);

    ret = hw.qdma_hw_ctx_conf((void *)this, is_c2h, qid, nullptr, context_op);
    if (ret < 0)
        return hw.qdma_get_error_code(ret);

    ret = hw.qdma_credit_ctx_conf((void *)this, is_c2h, qid, nullptr, context_op);
    if (ret < 0)
        return hw.qdma_get_error_code(ret);

    if ((nullptr != hw.qdma_qid2vec_conf) && irq_enable) {
        /** Versal Hard IP supports explicit qid2vec context programming.
          * For other versions, vector Id is accommodated in software context.
          */
        ret = hw.qdma_qid2vec_conf((void *)this, is_c2h, qid, nullptr, context_op);
        if (ret < 0) {
            return hw.qdma_get_error_code(ret);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::clear_cmpt_contexts(UINT16 qid, qdma_hw_access_type context_op) const
{
    int ret = hw.qdma_cmpt_ctx_conf((void *)this, qid, nullptr, context_op);
    if (ret < 0)
        return hw.qdma_get_error_code(ret);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::clear_pfetch_contexts(UINT16 qid, qdma_hw_access_type context_op) const
{
    int ret = hw.qdma_pfetch_ctx_conf((void *)this, qid, nullptr, context_op);
    if (ret < 0)
        return hw.qdma_get_error_code(ret);

    return STATUS_SUCCESS;
}


NTSTATUS qdma_device::clear_contexts(queue_pair& q, bool invalidate) const
{
    auto idx_abs = q.idx_abs;
    qdma_hw_access_type context_op = (invalidate == true) ? QDMA_HW_ACCESS_INVALIDATE : QDMA_HW_ACCESS_CLEAR;

    /* Clear/Invalidate H2C contexts */
    NTSTATUS status = clear_queue_contexts(false, idx_abs, context_op);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Clear/Invalidate C2H contexts */
    status = clear_queue_contexts(true, idx_abs, context_op);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /**
     * clear Completion context when completion ring is valid for the queue
     */
    if(q.c2h_q.is_cmpt_valid == true) {
        status = clear_cmpt_contexts(idx_abs, context_op);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    /**
     * Clear Prefetch context if queue is added in streaming mode
     */
    if (q.c2h_q.user_conf.is_st) {
        status = clear_pfetch_contexts(idx_abs, context_op);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::set_h2c_ctx(
    queue_pair& q)
{
    int ret;

    TraceVerbose(TRACE_QDMA, "queue_%u setting h2c contexts...", q.idx_abs);

    const bool irq_enable = (op_mode == queue_op_mode::POLL_MODE) ? false : true;

    if (irq_enable)
        TraceVerbose(TRACE_QDMA, "Programming with IRQ");

    qdma_descq_sw_ctxt sw_ctx = {};

    sw_ctx.pidx = 0;
    sw_ctx.qen = true;
    sw_ctx.frcd_en = false;
    sw_ctx.wbi_chk = true;
    sw_ctx.wbi_intvl_en = true;
    sw_ctx.fnc_id = dev_conf.dev_sbdf.sbdf.fun_no;
    sw_ctx.rngsz_idx = q.h2c_q.user_conf.h2c_ring_sz_index;
    sw_ctx.bypass = q.h2c_q.user_conf.desc_bypass_en;
    sw_ctx.mm_chn = 0;
    sw_ctx.wbk_en = true;
    sw_ctx.irq_en = irq_enable;
    sw_ctx.is_mm = q.type == queue_type::MEMORY_MAPPED ? 1 : 0;
    sw_ctx.desc_sz = (UINT8)q.h2c_q.lib_config.desc_sz;
    sw_ctx.at = 0;
    sw_ctx.ring_bs_addr = WdfCommonBufferGetAlignedLogicalAddress(q.h2c_q.desc_ring.buffer).QuadPart;

    if (irq_enable) {
        sw_ctx.vec = (UINT16)q.h2c_q.lib_config.vector_id;
        sw_ctx.intr_aggr = (op_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
    }

    ret = hw.qdma_sw_ctx_conf((void *)this, false, q.idx_abs, &sw_ctx, QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed to program H2C Software context! for queue %d, ret : %d", q.idx_abs, ret);
        return hw.qdma_get_error_code(ret);
    }

    if ((nullptr != hw.qdma_qid2vec_conf) && irq_enable) {
        /** Versal Hard IP supports explicit qid2vec context programming.
          * For other versions, vector Id is accommodated in software context.
          */
        qdma_qid2vec intr_ctx = {};
        intr_ctx.h2c_vector = (UINT8)q.h2c_q.lib_config.vector_id;
        intr_ctx.h2c_en_coal = (op_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;

        ret = hw.qdma_qid2vec_conf((void *)this, false, q.idx_abs, &intr_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to program qid2vec context! for queue %d, ret : %d", q.idx_abs, ret);
            clear_queue_contexts(false, q.idx_abs, QDMA_HW_ACCESS_CLEAR);
            return hw.qdma_get_error_code(ret);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::set_c2h_ctx(
    queue_pair& q)
{
    int ret;
    NTSTATUS status = STATUS_SUCCESS;

    TraceVerbose(TRACE_QDMA, "queue_%u setting c2h contexts...", q.idx_abs);

    const bool irq_enable = (op_mode == queue_op_mode::POLL_MODE) ? false : true;

    if (irq_enable)
        TraceVerbose(TRACE_QDMA, "Programming with IRQ");

    qdma_descq_sw_ctxt sw_ctx = {};

    sw_ctx.pidx = 0;
    sw_ctx.qen = true;
    sw_ctx.frcd_en = false;
    sw_ctx.wbi_chk = true;
    sw_ctx.wbi_intvl_en = false;
    sw_ctx.fnc_id = dev_conf.dev_sbdf.sbdf.fun_no;
    sw_ctx.rngsz_idx = q.c2h_q.user_conf.c2h_ring_sz_index;
    sw_ctx.bypass = q.c2h_q.user_conf.desc_bypass_en;
    sw_ctx.wbk_en = true;
    sw_ctx.irq_en = irq_enable;
    sw_ctx.is_mm = q.type == queue_type::MEMORY_MAPPED ? 1 : 0;
    sw_ctx.desc_sz = (UINT8)q.c2h_q.lib_config.desc_sz;
    sw_ctx.at = 0;
    sw_ctx.ring_bs_addr = WdfCommonBufferGetAlignedLogicalAddress(q.c2h_q.desc_ring.buffer).QuadPart;

    if (q.type == queue_type::STREAMING) {
        sw_ctx.frcd_en = true;
    }

    if (irq_enable) {
        sw_ctx.vec = (UINT16)q.c2h_q.lib_config.vector_id;
        sw_ctx.intr_aggr = (op_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
    }

    ret = hw.qdma_sw_ctx_conf((void *)this, true, q.idx_abs, &sw_ctx, QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed to program C2H Software context! for queue %d, ret : %d", q.idx_abs, ret);
        return hw.qdma_get_error_code(ret);
    }

    if ((nullptr != hw.qdma_qid2vec_conf) && irq_enable) {
        /** Versal Hard IP supports explicit qid2vec context programming.
          * For other versions, vector Id is accommodated in software context.
          */
        qdma_qid2vec intr_ctx = {};
        intr_ctx.c2h_vector = (UINT8)q.c2h_q.lib_config.vector_id;
        intr_ctx.c2h_en_coal = (op_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;

        ret = hw.qdma_qid2vec_conf((void *)this, true, q.idx_abs, &intr_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to program qid2vec context! for queue id %d, ret : %d", q.idx_abs, ret);
            status = hw.qdma_get_error_code(ret);
            goto ErrExit;
        }
    }

    /* CMPT context programming */
    if (q.c2h_q.is_cmpt_valid == true) {
        qdma_descq_cmpt_ctxt cmp_ctx = {};
        cmp_ctx.en_stat_desc = true;
        cmp_ctx.en_int = irq_enable;
        cmp_ctx.trig_mode = (uint8_t)q.c2h_q.user_conf.trig_mode;
        cmp_ctx.fnc_id = dev_conf.dev_sbdf.sbdf.fun_no;
        cmp_ctx.counter_idx = q.c2h_q.user_conf.c2h_th_cnt_index;
        cmp_ctx.timer_idx = q.c2h_q.user_conf.c2h_timer_cnt_index;
        cmp_ctx.ringsz_idx = (UINT8)q.c2h_q.lib_config.cmpt_ring_id;
        cmp_ctx.ovf_chk_dis = q.c2h_q.user_conf.cmpl_ovf_dis;
        cmp_ctx.color = 1;
        cmp_ctx.pidx = 0;
        cmp_ctx.valid = true;
        cmp_ctx.full_upd = false;
        cmp_ctx.desc_sz = (uint8_t)q.c2h_q.user_conf.cmpt_sz;
        cmp_ctx.bs_addr = WdfCommonBufferGetAlignedLogicalAddress(q.c2h_q.cmpt_ring.buffer).QuadPart;

        if (irq_enable) {
            cmp_ctx.vec = (UINT16)q.c2h_q.lib_config.vector_id;
            cmp_ctx.int_aggr = (op_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
        }

        ret = hw.qdma_cmpt_ctx_conf((void *)this, q.idx_abs, &cmp_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to program completion context! for queue id %d, ret : %d", q.idx_abs, ret);
            status = hw.qdma_get_error_code(ret);
            goto ErrExit;
        }
    }

    /* Pre-fetch context programming */
    if (q.type == queue_type::STREAMING) {
        qdma_descq_prefetch_ctxt pf_ctx = {};
        pf_ctx.bufsz_idx = q.c2h_q.user_conf.c2h_buff_sz_index;
        pf_ctx.valid = true;
        pf_ctx.pfch_en = q.c2h_q.user_conf.pfch_en;
        pf_ctx.bypass = q.c2h_q.user_conf.pfch_bypass_en;

        ret = hw.qdma_pfetch_ctx_conf((void *)this, q.idx_abs, &pf_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed to program prefetch context! for queue id %d, ret : %d", q.idx_abs, ret);
            status = hw.qdma_get_error_code(ret);
            goto ErrExit;
        }
    }

    return STATUS_SUCCESS;

ErrExit:
    clear_queue_contexts(true, q.idx_abs, QDMA_HW_ACCESS_CLEAR);

    if (q.c2h_q.is_cmpt_valid == true) {
        clear_cmpt_contexts(q.idx_abs, QDMA_HW_ACCESS_CLEAR);
    }

    if (q.type == queue_type::STREAMING) {
        clear_pfetch_contexts(q.idx_abs, QDMA_HW_ACCESS_CLEAR);
    }

    return status;
}

NTSTATUS qdma_device::queue_program(
    queue_pair& q)
{
    TraceVerbose(TRACE_QDMA, "queue_%u programming...", q.idx_abs);

    auto status = set_h2c_ctx(q);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = set_c2h_ctx(q);
    if (!NT_SUCCESS(status)) {
        /* Clear H2C queue contexts */
        clear_queue_contexts(false, q.idx_abs, QDMA_HW_ACCESS_CLEAR);
        return status;
    }

    return status;
}

/* ---------- public qdma member function implemenations ---------- */

NTSTATUS qdma_device::init(queue_op_mode operation_mode, UINT8 cfg_bar, UINT16 qsets_max)
{
    NTSTATUS status;

    TraceInfo(TRACE_QDMA, "Lib-QDMA v%u.%u.%u",
        qdma_version.major, qdma_version.minor, qdma_version.patch);

    InterlockedExchange(&qdma_device_state, device_state::DEVICE_OFFLINE);
    op_mode = operation_mode;
    config_bar = cfg_bar;
    qmax = qsets_max;

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &register_access_lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Register access lock creation failed!");
        return status;
    }

    return STATUS_SUCCESS;
}

ULONG qdma_device::qdma_conf_reg_read(size_t offset)
{
    return pcie.conf_reg_read(offset);
}

void qdma_device::qdma_conf_reg_write(size_t offset, ULONG data)
{
    pcie.conf_reg_write(offset, data);
}

NTSTATUS qdma_device::read_bar(
    qdma_bar_type bar_type,
    size_t offset,
    void* data,
    size_t size)
{
    return pcie.read_bar(bar_type, offset, data, size);
}

NTSTATUS qdma_device::write_bar(
    qdma_bar_type bar_type,
    size_t offset,
    void* data,
    size_t size)
{
    return pcie.write_bar(bar_type, offset, data, size);
}

NTSTATUS qdma_device::qdma_is_queue_in_range(UINT16 qid)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Provided qid : %u is more than allocated : %u", qid, qmax);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS qdma_device::qdma_get_queues_state(
    _In_  UINT16 qid,
    _Out_writes_(str_maxlen) CHAR *str,
    _In_  size_t str_maxlen)
{
    NTSTATUS status;

    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    switch (queue_pairs[qid].state) {
    case queue_state::QUEUE_ADDED:
        status = RtlStringCchCopyA(str, str_maxlen, "QUEUE ADDED");
        break;
    case queue_state::QUEUE_STARTED:
        status = RtlStringCchCopyA(str, str_maxlen, "QUEUE ACTIVE");
        break;
    case queue_state::QUEUE_AVAILABLE:
        status = RtlStringCchCopyA(str, str_maxlen, "QUEUE IS AVAILABLE");
        break;
    case queue_state::QUEUE_BUSY:
        status = RtlStringCchCopyA(str, str_maxlen, "QUEUE BUSY IN CRITICAL OPERATION");
        break;
    default:
        NT_ASSERT(FALSE);
        status = RtlStringCchCopyA(str, str_maxlen, "<INVALID>");
        break;
    }

    return status;
}

NTSTATUS qdma_device::qdma_set_qmax(UINT32 queue_max)
{
    INT32 queue_base = -1;
    NTSTATUS status = STATUS_SUCCESS;

    LONG state = InterlockedCompareExchange(&qdma_device_state,
                                            device_state::DEVICE_OFFLINE,
                                            device_state::DEVICE_ONLINE);
    if (state == device_state::DEVICE_OFFLINE) {
        TraceError(TRACE_QDMA, "Device in OFFLINE state. Can not proceed further.");
        return STATUS_UNSUCCESSFUL;
    }

    UINT32 active_queues = qdma_get_active_queue_count(dma_dev_index);
    if (active_queues) {
        TraceError(TRACE_QDMA, "Can not update qmax... %u queues are active", active_queues);
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    int rv = qdma_dev_update(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, queue_max, &queue_base);
    if (rv < 0) {
        TraceError(TRACE_QDMA, "qdma_dev_update() Failed! %d", rv);
        status = hw.qdma_get_error_code(rv);
        goto ErrExit;
    }

    rv = qdma_dev_qinfo_get(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, &queue_base, &queue_max);
    if (rv < 0) {
        TraceError(TRACE_QDMA, "qdma_dev_qinfo_get() Failed! %d", rv);
        status = hw.qdma_get_error_code(rv);
        goto ErrExit;
    }

    TraceInfo(TRACE_QDMA, "qmax updated. Old qmax : %u, New qmax : %u, "
                          "Old qbase : %d, New qbase : %d",
                          qmax, queue_max, qbase, queue_base);

    qmax  = queue_max;
    qbase = queue_base;

    if (queue_pairs) {
        qdma_memfree(queue_pairs);
        queue_pairs = nullptr;
    }

    status = init_dma_queues();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_dma_queues() failed! %!STATUS!", status);
        qmax = 0u;
        qbase = -1;
    }

    status = init_func();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_func() failed! %!STATUS!", status);
        return status;
    }

ErrExit:
    InterlockedExchange(&qdma_device_state, device_state::DEVICE_ONLINE);
    return status;
}

NTSTATUS qdma_device::validate_qconfig(queue_config& conf)
{
    /** check if the requested mode is enabled?
     *  the request modes are read from the HW.
     *  before serving any request, first check if the
     *  HW has the capability or not, else return error
     */

    if ((conf.is_st && !dev_conf.dev_info.st_en) ||
        (!conf.is_st && !dev_conf.dev_info.mm_en)) {
        TraceError(TRACE_QDMA, "%s mode is not enabled in device", conf.is_st ? "ST" : "MM");
        return STATUS_INVALID_PARAMETER;
    }

    if ((conf.h2c_ring_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_ring_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_buff_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_th_cnt_index >= QDMA_GLBL_CSR_REG_CNT)  ||
        (conf.c2h_timer_cnt_index >= QDMA_GLBL_CSR_REG_CNT)) {
        TraceError(TRACE_QDMA, "One or more invalid global CSR indexes provided");
        return STATUS_INVALID_PARAMETER;
    }

    if (conf.cmpt_sz >= qdma_desc_sz::QDMA_DESC_SZ_MAX) {
        TraceError(TRACE_QDMA, "Invalid completion descriptor size provided");
        return STATUS_INVALID_PARAMETER;
    }

    if (conf.trig_mode >= qdma_trig_mode::QDMA_TRIG_MODE_MAX) {
        TraceError(TRACE_QDMA, "Invalid Trigger mode provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((conf.en_mm_cmpl) && (!dev_conf.dev_info.mm_cmpt_en)) {
        TraceError(TRACE_QDMA, "MM Completion not supported in HW");
        return STATUS_NOT_SUPPORTED;
    }

    if ((conf.cmpl_ovf_dis) && (!dev_conf.dev_info.cmpt_ovf_chk_dis)) {
        TraceError(TRACE_QDMA, "Completion overflow disable option not supported in HW");
        return STATUS_NOT_SUPPORTED;
    }

    if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
        /** 64B Descriptors not supportd for the versal hard IP (i.e., s80 device) */
        if ((conf.sw_desc_sz == static_cast<UINT8>(qdma_desc_sz::QDMA_DESC_SZ_64B)) ||
            (conf.cmpt_sz == qdma_desc_sz::QDMA_DESC_SZ_64B)) {
            TraceError(TRACE_QDMA, "64B Descriptor not supported for Versal Hard IP(S80)");
            return STATUS_NOT_SUPPORTED;
        }

        if ((conf.trig_mode == qdma_trig_mode::QDMA_TRIG_MODE_USER_TIMER_COUNT) ||
            (conf.trig_mode == qdma_trig_mode::QDMA_TRIG_MODE_USER_COUNT)) {
            TraceError(TRACE_QDMA, "Counter, Timer+Counter modes are not supported for Versal Hard IP(S80)");
            return STATUS_NOT_SUPPORTED;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_add_queue(UINT16 qid, queue_config& conf)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = validate_qconfig(conf);
    if (!NT_SUCCESS(status))
        return status;

    auto& q = queue_pairs[qid];

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_AVAILABLE);
    if (old_state != queue_state::QUEUE_AVAILABLE) {
        TraceError(TRACE_QDMA, "Queue %d is not available!", qid);
        return STATUS_UNSUCCESSFUL;
    }

    TraceVerbose(TRACE_QDMA, "Adding the queue : %u", qid);

    status = q.create(conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to allocate resource %!STATUS!", status);
        InterlockedExchange(&q.state, queue_state::QUEUE_AVAILABLE);
        return status;
    }

    /* Increament resource manager active queue count */
    inc_queue_pair_count(q.c2h_q.is_cmpt_valid);

    InterlockedExchange(&q.state, queue_state::QUEUE_ADDED);

    TraceInfo(TRACE_QDMA, "PF : %u, queue : %u added", dev_conf.dev_sbdf.sbdf.fun_no, qid);

    return status;
}

NTSTATUS qdma_device::qdma_start_queue(UINT16 qid)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_ADDED);
    if (old_state != queue_state::QUEUE_ADDED) {
        TraceError(TRACE_QDMA, "Queue %d is not added!", qid);
        return STATUS_UNSUCCESSFUL;
    }

    TraceVerbose(TRACE_QDMA, "Starting the queue : %u", qid);

    bool is_intr_vec_allocated = false;
    NTSTATUS status = STATUS_SUCCESS;

    if (op_mode != queue_op_mode::POLL_MODE) {
        auto vec = assign_interrupt_vector(q);
        if (vec < 0) {
            TraceError(TRACE_QDMA, "Failed to retrieve interrupt vector");
            status = STATUS_UNSUCCESSFUL;
            goto ErrExit;
        }

        q.h2c_q.lib_config.vector_id = q.c2h_q.lib_config.vector_id = vec;
        is_intr_vec_allocated = true;

        TraceInfo(TRACE_QDMA, "Allocated vector Id : %u", q.c2h_q.lib_config.vector_id);
    }

    /* H2C Poll function registration */
    poll_op op;
    bool is_st = q.h2c_q.user_conf.is_st;
    op.fn = is_st ? service_h2c_st_queue : service_h2c_mm_queue;
    op.arg = &queue_pairs[qid];

    q.h2c_q.poll_entry = th_mgr.register_poll_function(op);
    if (nullptr == q.h2c_q.poll_entry) {
        TraceError(TRACE_QDMA, "Failed to register poll function for H2C");
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    /* C2H Poll function registration */
    is_st = q.c2h_q.user_conf.is_st;
    op.fn = is_st ? service_c2h_st_queue : service_c2h_mm_queue;

    q.c2h_q.poll_entry = th_mgr.register_poll_function(op);
    if (nullptr == q.c2h_q.poll_entry) {
        TraceError(TRACE_QDMA, "Failed to register poll function for C2H");
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    status = queue_program(q);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to program the queue %!STATUS!", status);
        goto ErrExit;
    }

    dump(this, q.type, q.idx_abs);
    dump(this, q.type, 0);

    q.init_csr();

    InterlockedExchange(&q.state, queue_state::QUEUE_STARTED);

    TraceInfo(TRACE_QDMA, "PF : %u, queue : %u Started", dev_conf.dev_sbdf.sbdf.fun_no, qid);

    return status;

ErrExit:
    if (nullptr != q.h2c_q.poll_entry) {
        th_mgr.unregister_poll_function(q.h2c_q.poll_entry);
        q.h2c_q.poll_entry = nullptr;
    }

    if (nullptr != q.c2h_q.poll_entry) {
        th_mgr.unregister_poll_function(q.c2h_q.poll_entry);
        q.c2h_q.poll_entry = nullptr;
    }

    if (is_intr_vec_allocated)
        free_interrupt_vector(q, q.c2h_q.lib_config.vector_id);

    InterlockedExchange(&q.state, queue_state::QUEUE_ADDED);
    return status;
}

NTSTATUS qdma_device::qdma_stop_queue(UINT16 qid)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid %d provided", qid);
        return STATUS_INVALID_PARAMETER;
    }

    TraceVerbose(TRACE_QDMA, "Stopping the queue : %u", qid);
    NTSTATUS status = STATUS_SUCCESS;

    auto& q = queue_pairs[qid];

    /* Stop further traffic on the queue */
    auto old_state = InterlockedCompareExchange(&q.state,
                                                 queue_state::QUEUE_BUSY,
                                                 queue_state::QUEUE_STARTED);
    if (old_state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue %d is not started to stop!", qid);
        return STATUS_UNSUCCESSFUL;
    }

    /* Wait till incoming traffic stops & let HW finish any pending jobs */
    LARGE_INTEGER wait_time;
    wait_time.QuadPart = WDF_REL_TIMEOUT_IN_MS(2);
    KeDelayExecutionThread(KernelMode, FALSE, &wait_time);

    th_mgr.unregister_poll_function(q.h2c_q.poll_entry);
    th_mgr.unregister_poll_function(q.c2h_q.poll_entry);

    q.h2c_q.poll_entry = nullptr;
    q.c2h_q.poll_entry = nullptr;

    if (op_mode != queue_op_mode::POLL_MODE)
        free_interrupt_vector(q, q.c2h_q.lib_config.vector_id);

    /* Drain any pending requests */
    q.flush_queue();

    /* Invalidate the contexts */
    status = clear_contexts(q, true);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Context Invalidation for qid %d failed", qid);
        /* Continue further even in error scenario */
    }

    InterlockedExchange(&q.state, queue_state::QUEUE_ADDED);

    TraceInfo(TRACE_QDMA, "PF : %u, Queue : %u stopped", dev_conf.dev_sbdf.sbdf.fun_no, qid);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_remove_queue(UINT16 qid)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid %d provided", qid);
        return STATUS_INVALID_PARAMETER;
    }

    TraceVerbose(TRACE_QDMA, "Removing the queue : %u", qid);

    auto& q = queue_pairs[qid];

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_ADDED);
    if (old_state != queue_state::QUEUE_ADDED) {
        TraceError(TRACE_QDMA, "Queue %u is not added to remove", qid);
        return STATUS_UNSUCCESSFUL;
    }

    /* Clear queue context */
    NTSTATUS status = clear_contexts(q);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Context Clearing for qid %d failed", qid);
        /* Continue further even in error scenario */
    }

    auto is_cmpt_valid = q.c2h_q.is_cmpt_valid;

    q.destroy();

    /* Decreament resource manager active queue count */
    dec_queue_pair_count(is_cmpt_valid);

    InterlockedExchange(&q.state, queue_state::QUEUE_AVAILABLE);

    TraceVerbose(TRACE_QDMA, "PF : %u, Queue : %u removed", dev_conf.dev_sbdf.sbdf.fun_no, qid);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_enqueue_mm_request(
    UINT16 qid,
    WDF_DMA_DIRECTION direction,
    PSCATTER_GATHER_LIST sg_list,
    LONGLONG device_offset,
    dma_completion_cb compl_cb,
    VOID *priv,
    size_t &xfered_len)
{
    if ((qid >= qmax) || (compl_cb == nullptr) ||
        (sg_list == nullptr)) {
        TraceError(TRACE_QDMA, "Incorrect Parameters, qid : %d, "
            "compl_cb : %p, sg_list : %p", qid, compl_cb, sg_list);
        return STATUS_INVALID_PARAMETER;
    }

    TraceVerbose(TRACE_QDMA, "MM enqueue request for queue : %u", qid);

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue %d is not started!", qid);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::MEMORY_MAPPED) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in MM mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    NTSTATUS status = q.enqueue_mm_request(direction, sg_list, device_offset, compl_cb, priv, xfered_len);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to enqueue the MM request in queue %d, err : %d", qid, status);
        return status;
    }

    return status;
}

NTSTATUS qdma_device::qdma_enqueue_st_tx_request(
    UINT16 qid,
    PSCATTER_GATHER_LIST sg_list,
    dma_completion_cb compl_cb,
    VOID *priv,
    size_t &xfered_len)
{
    if ((qid >= qmax) || (compl_cb == nullptr) ||
        (sg_list == nullptr)) {
        TraceError(TRACE_QDMA, "Incorrect Parameters, qid : %d, "
            "compl_cb : %p, sg_list : %p", qid, compl_cb, sg_list);
        return STATUS_INVALID_PARAMETER;
    }

    TraceVerbose(TRACE_QDMA, "ST enqueue request for queue : %u", qid);

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue %d is not started!", qid);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    NTSTATUS status = q.enqueue_st_tx_request(sg_list, compl_cb, priv, xfered_len);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to enqueue the ST request in queue %d, err : %d", qid, status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_enqueue_st_rx_request(
    UINT16 qid,
    size_t length,
    st_rx_completion_cb compl_cb,
    VOID *priv)
{
    if ((qid >= qmax) || (compl_cb == nullptr)) {
        TraceError(TRACE_QDMA, "Incorrect Parameters, qid : %d, compl_cb : %p len : %Iu",
            qid, compl_cb, length);
        return STATUS_INVALID_PARAMETER;
    }

    TraceVerbose(TRACE_QDMA, "ST read request for qid : %u", qid);

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue %d is not started!", qid);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    NTSTATUS status = q.enqueue_st_rx_request(length, compl_cb, priv);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to receive st pkts! %!STATUS! on queue %d", status, qid);
    }

    return status;
}

NTSTATUS qdma_device::qdma_retrieve_st_udd_data(UINT16 qid, void *addr, UINT8 *buf, UINT32 *len)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((addr == nullptr) || (buf == nullptr) || (len == nullptr))
        return STATUS_INVALID_PARAMETER;

    TraceVerbose(TRACE_QDMA, "Retrieving UDD data for queue : %u", qid);

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue is not started!");
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    return q.read_st_udd_data(addr, buf, len);
}

NTSTATUS qdma_device::qdma_retrieve_last_st_udd_data(UINT16 qid, UINT8 *buf, UINT32 *len)
{
    if (qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((buf == nullptr) || (len == nullptr))
        return STATUS_INVALID_PARAMETER;

    TraceVerbose(TRACE_QDMA, "Reading latest UDD data for queue : %u", qid);

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue is not started!");
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    void *udd_addr = q.get_last_udd_addr();

    return q.read_st_udd_data(udd_addr, buf, len);
}

NTSTATUS qdma_device::qdma_queue_desc_dump(qdma_desc_info *desc_info)
{
    if (desc_info->qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((desc_info == nullptr) || (desc_info->pbuffer == nullptr) || (desc_info->buffer_sz == 0))
        return STATUS_INVALID_PARAMETER;

    auto& q = queue_pairs[desc_info->qid];

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_ADDED);
    if (old_state == queue_state::QUEUE_AVAILABLE) {
        TraceError(TRACE_QDMA, "Queue %u is not added or started to get dump", desc_info->qid);
        return STATUS_UNSUCCESSFUL;
    }

    NTSTATUS status = q.desc_dump(desc_info);

    InterlockedExchange(&q.state, old_state);

    return status;
}

NTSTATUS qdma_device::qdma_read_mm_cmpt_data(qdma_cmpt_info *cmpt_info)
{
    if (cmpt_info->qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((cmpt_info == nullptr) || (cmpt_info->pbuffer == nullptr) || (cmpt_info->buffer_len == 0))
        return STATUS_INVALID_PARAMETER;

    auto& q = queue_pairs[cmpt_info->qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "Queue is not started!");
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::MEMORY_MAPPED) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in MM mode!", cmpt_info->qid);
        return STATUS_ACCESS_VIOLATION;
    }

    return q.read_mm_cmpt_data(cmpt_info);
}

NTSTATUS qdma_device::qdma_queue_context_read(UINT16 qid, enum qdma_dev_q_type ctx_type, struct qdma_descq_context *ctxt)
{
    if ((qid >= qmax) ||
        (ctx_type >= qdma_dev_q_type::QDMA_DEV_Q_TYPE_MAX) ||
        (ctxt == nullptr)) {
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_ADDED) {
        TraceError(TRACE_QDMA, "Queue %d is not configured!", qid);
        return STATUS_INVALID_DEVICE_STATE;
    }

    uint8_t is_c2h = (ctx_type == qdma_dev_q_type::QDMA_DEV_Q_TYPE_C2H) ? true : false;
    uint16_t hw_qid = q.idx_abs;
    int ret;
    bool dump = false;

    if (ctx_type != qdma_dev_q_type::QDMA_DEV_Q_TYPE_CMPT) {
        ret = hw.qdma_sw_ctx_conf(this, is_c2h, hw_qid, &ctxt->sw_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed while reading SW Context for queue %d!, ret : %d", hw_qid, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_hw_ctx_conf(this, is_c2h, hw_qid, &ctxt->hw_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed while reading HW Context for queue %d!, ret : %d", hw_qid, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_credit_ctx_conf(this, is_c2h, hw_qid, &ctxt->cr_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed while reading Credit Context for queue %d!, ret : %d", hw_qid, ret);
            return hw.qdma_get_error_code(ret);
        }

        if ((q.type == queue_type::STREAMING) && (is_c2h)) {
            ret = hw.qdma_pfetch_ctx_conf(this, hw_qid, &ctxt->pfetch_ctxt, QDMA_HW_ACCESS_READ);
            if (ret < 0) {
                TraceError(TRACE_QDMA, "Failed while reading pre-fetch Context for queue %d!, ret : %d", hw_qid, ret);
                return hw.qdma_get_error_code(ret);
            }
        }

        dump = true;
    }

    if (((q.type == queue_type::STREAMING) && (is_c2h)) ||
        ((q.type == queue_type::MEMORY_MAPPED) && (ctx_type == qdma_dev_q_type::QDMA_DEV_Q_TYPE_CMPT) &&
        (dev_conf.dev_info.mm_cmpt_en) && (q.c2h_q.user_conf.en_mm_cmpl))) {

        ret = hw.qdma_cmpt_ctx_conf(this, hw_qid, &ctxt->cmpt_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "Failed while reading completion "
                "Context for queue %d!, ret : %d", hw_qid, ret);
            return hw.qdma_get_error_code(ret);
        }
        dump = true;
    }

    if (!dump)
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_intr_context_read(UINT8 ring_idx_abs, struct qdma_indirect_intr_ctxt *ctxt)
{
    int ret;

    if (op_mode != queue_op_mode::INTR_COAL_MODE)
        return STATUS_ACCESS_VIOLATION;

    ret = hw.qdma_indirect_intr_ctx_conf(this, ring_idx_abs, ctxt, QDMA_HW_ACCESS_READ);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed while reading interrupt Context for ring index %d!, ret : %d",
            ring_idx_abs, ret);
        return hw.qdma_get_error_code(ret);
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_queue_dump_context(qdma_ctx_info *ctx_info)
{
    if (ctx_info->qid >= qmax) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return STATUS_INVALID_PARAMETER;
    }

    if ((ctx_info == nullptr) ||
        (ctx_info->ring_type >= qdma_q_type::QDMA_Q_TYPE_MAX) ||
        (ctx_info->pbuffer == nullptr) || (ctx_info->buffer_sz == 0))
        return STATUS_INVALID_PARAMETER;

    queue_pair &q = queue_pairs[ctx_info->qid];

    if (q.state == queue_state::QUEUE_AVAILABLE) {
        TraceError(TRACE_QDMA, "Queue is not configured!");
        return STATUS_INVALID_DEVICE_STATE;
    }

    uint8_t st = (q.type == queue_type::STREAMING) ? 1 : 0;
    size_t len = 0;
    NTSTATUS status;
    bool dump = false;

    if (ctx_info->ring_type != qdma_q_type::QDMA_Q_TYPE_CMPT) {
        dump = true;
    }
    else if ((q.type == queue_type::MEMORY_MAPPED) &&
        (ctx_info->ring_type == qdma_q_type::QDMA_Q_TYPE_CMPT) &&
        (dev_conf.dev_info.mm_cmpt_en) && (q.c2h_q.user_conf.en_mm_cmpl)) {
        dump = true;
    }

    if (!dump)
        return STATUS_UNSUCCESSFUL;

    int ctx_len = hw.qdma_read_dump_queue_context(this, q.idx_abs,
        st, (qdma_dev_q_type)ctx_info->ring_type,
        ctx_info->pbuffer, (uint32_t)ctx_info->buffer_sz);
    if (ctx_len < 0) {
        TraceError(TRACE_QDMA, "hw.qdma_dump_queue_context returned error : %d", ctx_len);
        return hw.qdma_get_error_code(ctx_len);
    }

    len = len + ctx_len;

    if (op_mode == queue_op_mode::INTR_COAL_MODE) {
        if (ctx_info->buffer_sz < len) {
            TraceError(TRACE_QDMA, "Insufficient Buffer Length");
            return STATUS_BUFFER_TOO_SMALL;
        }

        UINT8 ring_index = 0;
        if (ctx_info->ring_type == qdma_q_type::QDMA_Q_TYPE_H2C)
            ring_index = (UINT8)q.h2c_q.lib_config.vector_id;
        else if (ctx_info->ring_type != qdma_q_type::QDMA_Q_TYPE_MAX)
            ring_index = (UINT8)q.c2h_q.lib_config.vector_id;

        struct qdma_indirect_intr_ctxt intr_ctxt;
        status = qdma_intr_context_read(ring_index, &intr_ctxt);
        if (!NT_SUCCESS(status))
            return status;

        int intr_len = hw.qdma_dump_intr_context(this, &intr_ctxt, ring_index,
            &ctx_info->pbuffer[len], (uint32_t)(ctx_info->buffer_sz - len));
        if (intr_len < 0) {
            TraceError(TRACE_QDMA, "hw.qdma_dump_intr_context returned error : %d", intr_len);
            return hw.qdma_get_error_code(intr_len);
        }

        len = len + intr_len;
    }

    ctx_info->ret_sz = len;

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_intring_dump(qdma_intr_ring_info *intring_info)
{
    if (op_mode != queue_op_mode::INTR_COAL_MODE) {
        TraceError(TRACE_QDMA, "Interrupt ring is valid only in Aggregation Mode");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((intring_info == nullptr) ||
        (intring_info->pbuffer == nullptr) || (intring_info->buffer_len == 0)) {
        TraceError(TRACE_QDMA, "Invalid Parameters");
        return STATUS_INVALID_PARAMETER;
    }

    if ((intring_info->vec_id < irq_mgr.data_vector_id_start) ||
        (intring_info->vec_id >= irq_mgr.data_vector_id_end)) {
        TraceError(TRACE_QDMA, "Invalid Vector ID : %d, Valid range : %u ~ %u",
            intring_info->vec_id, irq_mgr.data_vector_id_start, irq_mgr.data_vector_id_end);
        return STATUS_INVALID_PARAMETER;
    }

    if (op_mode != queue_op_mode::INTR_COAL_MODE) {
        TraceError(TRACE_QDMA, "Device in not configured in Interrupt Aggregation mode");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    NTSTATUS status = irq_mgr.intr_q[intring_info->vec_id].intring_dump(intring_info);

    return status;
}

NTSTATUS qdma_device::qdma_regdump(qdma_reg_dump_info *regdump_info)
{
    bool is_vf = false;

    if ((regdump_info == nullptr) || (regdump_info->pbuffer == nullptr) || (regdump_info->buffer_len == 0))
        return STATUS_INVALID_PARAMETER;

    int len = hw.qdma_dump_config_regs(this, is_vf, regdump_info->pbuffer, (uint32_t)regdump_info->buffer_len);
    if (len < 0) {
        TraceError(TRACE_QDMA, "hw.qdma_dump_config_regs failed with err : %d", len);
        return hw.qdma_get_error_code(len);
    }

    regdump_info->ret_len = len;
    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_get_qstats_info(qdma_qstat_info &qstats)
{
    INT32 queue_base = -1; /* qdma_dev_qinfo_get() requires qbase to be int pointer */

    qdma_dev_qinfo_get(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, &queue_base, &qstats.qmax);
    qstats.qbase = queue_base;
    qstats.active_h2c_queues =
        qdma_get_device_active_queue_count(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, QDMA_DEV_Q_TYPE_H2C);
    qstats.active_c2h_queues =
        qdma_get_device_active_queue_count(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, QDMA_DEV_Q_TYPE_C2H);
    qstats.active_cmpt_queues =
        qdma_get_device_active_queue_count(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, QDMA_DEV_Q_TYPE_CMPT);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::open(const WDFDEVICE device, WDFCMRESLIST resources,
                           const WDFCMRESLIST resources_translated)
{
    NTSTATUS status = STATUS_SUCCESS;

    wdf_dev = device;

    status = pcie.get_bdf(wdf_dev, dev_conf.dev_sbdf.val);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "get_bdf failed! %!STATUS!", status);
        return status;
    }

    TraceVerbose(TRACE_QDMA, "pcie.bdf = %X", dev_conf.dev_sbdf.val);

    RtlStringCchPrintfA(dev_conf.name,
        ARRAYSIZE(dev_conf.name), "qdma%05x", dev_conf.dev_sbdf.val);

    TraceVerbose(TRACE_QDMA, "qdma_open : name : %s, BDF :0x%05X pf_no : %d",
        dev_conf.name, dev_conf.dev_sbdf.val, dev_conf.dev_sbdf.sbdf.fun_no);

    status = pcie.map(resources_translated);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "pcie.map failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = pcie.assign_config_bar(config_bar);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "Failed to assign config BAR");
        goto ErrExit;
    }

    int ret = qdma_hw_access_init(this, 0, &hw);
    if (status != QDMA_SUCCESS) {
        TraceError(TRACE_QDMA, "Failed to initialize hw access");
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    ret = hw.qdma_get_version((void *)this, QDMA_DEV_PF, &hw_version_info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_get_version failed!, ret : %d", ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    status = assign_bar_types();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "assign_bar_types failed! %!STATUS!", status);
        goto ErrExit;
    }

    ret = hw.qdma_get_device_attributes((void *)this, &dev_conf.dev_info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_get_device_attributes failed!, ret : %d", ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    TraceVerbose(TRACE_QDMA, "qdma_get_device_attributes Success : num_pfs : %d, mm_en:%d, st_en:%d\n",
        dev_conf.dev_info.num_pfs, dev_conf.dev_info.mm_en, dev_conf.dev_info.st_en);

    if ((dev_conf.dev_info.st_en == 0) &&
        (dev_conf.dev_info.mm_en == 0)) {
        TraceError(TRACE_QDMA, "None of the modes ( ST or MM) are enabled");
        status = STATUS_INVALID_HW_PROFILE;
        goto ErrExit;
    }

    status = list_add_qdma_device_and_set_gbl_csr();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "list_add_qdma_device_and_set_gbl_csr failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = init_resource_manager();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_resource_manager failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = init_os_resources(resources, resources_translated);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_os_resources() failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = init_qdma_global();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_qdma_global() failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = init_func();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_func() failed! %!STATUS!", status);
        goto ErrExit;
    }

    status = init_dma_queues();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "init_dma_queues() failed! %!STATUS!", status);
        goto ErrExit;
    }

    if (op_mode == queue_op_mode::INTR_COAL_MODE) {
        status = init_interrupt_queues();
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "init_interrupt_queues() failed! %!STATUS!", status);
            goto ErrExit;
        }
    }

    InterlockedExchange(&qdma_device_state, device_state::DEVICE_ONLINE);

    return status;

ErrExit:
    close();
    return status;
}

void qdma_device::close()
{
    TraceVerbose(TRACE_QDMA, "qdma_close()");

    destroy_dma_queues();

    destroy_func();

    destroy_os_resources();

    destroy_resource_manager();

    list_remove_qdma_device();

    pcie.unmap();

    if (nullptr != register_access_lock) {
        WdfObjectDelete(register_access_lock);
        register_access_lock = nullptr;
    }
}

bool qdma_device::qdma_is_device_online(void)
{
    if (qdma_device_state == device_state::DEVICE_ONLINE)
        return true;

    return false;
}

NTSTATUS qdma_device::qdma_read_csr_conf(qdma_glbl_csr_conf *conf)
{
    int ret;

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->ring_sz, QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "Failed in reading global ring size, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_timer_cnt, QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "Failed in reading global timer count, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_th_cnt, QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "Failed in reading global counter threshold, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_buff_sz, QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "Failed in reading global buffer size, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_writeback_interval_conf((void *)this,
            (qdma_wrb_interval *)&conf->wb_interval, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "Failed in reading global write back interval, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_get_dev_capabilities_info(qdma_device_attributes_info &dev_attr)
{
    int ret;
    struct qdma_dev_attributes dev_info;

    ret = hw.qdma_get_device_attributes((void *)this, &dev_info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_get_device_attributes Failed, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    dev_attr.num_pfs            = dev_info.num_pfs;
    dev_attr.num_qs             = dev_info.num_qs;
    dev_attr.flr_present        = dev_info.flr_present;
    dev_attr.st_en              = dev_info.st_en;
    dev_attr.mm_en              = dev_info.mm_en;
    dev_attr.mm_cmpl_en         = dev_info.mm_cmpt_en;
    dev_attr.mailbox_en         = dev_info.mailbox_en;
    dev_attr.num_mm_channels    = dev_info.mm_channel_max;

    return STATUS_SUCCESS;
}
void qdma_device::qdma_get_hw_version_info(
    qdma_hw_version_info &version_info)
{
    version_info = hw_version_info;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS qdma_device::qdma_device_version_info(
    qdma_version_info &version_info)
{
    NTSTATUS status;
    struct qdma_hw_version_info info;

    int ret = hw.qdma_get_version((void *)this, QDMA_DEV_PF, &info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "qdma_get_version failed!, ret : %d", ret);
        return hw.qdma_get_error_code(ret);
    }

    status = RtlStringCchCopyNA(version_info.qdma_rtl_version_str,
        ARRAYSIZE(version_info.qdma_rtl_version_str),
        info.qdma_rtl_version_str,
        ARRAYSIZE(info.qdma_rtl_version_str));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlStringCchCopyNA(version_info.qdma_vivado_release_id_str,
        ARRAYSIZE(version_info.qdma_vivado_release_id_str),
        info.qdma_vivado_release_id_str,
        ARRAYSIZE(info.qdma_vivado_release_id_str));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlStringCchCopyNA(version_info.qdma_device_type_str,
        ARRAYSIZE(version_info.qdma_device_type_str),
        info.qdma_device_type_str,
        ARRAYSIZE(info.qdma_device_type_str));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlStringCchCopyNA(version_info.qdma_versal_ip_type_str,
        ARRAYSIZE(version_info.qdma_versal_ip_type_str),
        info.qdma_ip_type_str,
        ARRAYSIZE(info.qdma_ip_type_str));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlStringCchPrintfA(version_info.qdma_sw_version_str,
        ARRAYSIZE(version_info.qdma_sw_version_str),
        "%d.%d.%d", qdma_version.major,
        qdma_version.minor, qdma_version.patch);

    return status;
}

queue_pair *qdma_device::qdma_get_queue_pair_by_hwid(UINT16 qid_abs)
{
    if (qid_abs >= (qbase + qmax)) {
        TraceError(TRACE_QDMA, "Invalid Qid provided");
        return nullptr;
    }

    UINT16 qid_rel = qid_abs - static_cast<UINT16>(qbase);
    TraceVerbose(TRACE_QDMA, "Absolute qid : %u Relative qid : %u", qid_abs, qid_rel);
    return &queue_pairs[qid_rel];
}