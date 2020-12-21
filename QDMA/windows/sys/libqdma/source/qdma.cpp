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

static struct drv_mode_name mode_name_list[] = {
    { POLL_MODE,		"poll"},
    { INTR_MODE,		"direct interrupt"},
    { INTR_COAL_MODE,	"indirect interrupt"},
};

NTSTATUS qdma_device::list_add_qdma_device_and_set_gbl_csr(void)
{
    if ((LONG)1 == InterlockedIncrement(&qdma_active_pf_count)) {
        TraceVerbose(TRACE_QDMA, "%s: ** Initializing global device list and wait lock **", dev_conf.name);

        /* First device in the list, Initialize the list head */
        INIT_LIST_HEAD(&qdma_dev_list_head);

        WDF_OBJECT_ATTRIBUTES attr;
        WDF_OBJECT_ATTRIBUTES_INIT(&attr);
        attr.ParentObject = WDFDRIVER();

        NTSTATUS status = WdfWaitLockCreate(&attr, &qdma_list_lock);
        if (!(NT_SUCCESS(status))) {
            TraceError(TRACE_QDMA, "%s: Failed to create qdma list wait lock %!STATUS!", dev_conf.name, status);
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
            TraceError(TRACE_QDMA, "%s: qdma list wait lock not initialized", dev_conf.name);
            return STATUS_UNSUCCESSFUL;
        }
    }

    TraceVerbose(TRACE_QDMA, "%s: ++ Active pf count : %u ++", dev_conf.name, qdma_active_pf_count);

    WdfWaitLockAcquire(qdma_list_lock, NULL);

    dev_conf.is_master_pf = is_first_qdma_pf_device();
    if (true == dev_conf.is_master_pf) {
        TraceInfo(TRACE_QDMA, "Configuring '%04X:%02X:%02X.%x' as master pf\n", 
            dev_conf.dev_sbdf.sbdf.seg_no, dev_conf.dev_sbdf.sbdf.bus_no, 
            dev_conf.dev_sbdf.sbdf.dev_no, dev_conf.dev_sbdf.sbdf.fun_no);

        TraceVerbose(TRACE_QDMA, "%s: Setting Global CSR", dev_conf.name);

        int ret = hw.qdma_set_default_global_csr(this);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s: Failed to set global CSR Configuration, ret : %d", dev_conf.name, ret);
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
    TraceVerbose(TRACE_QDMA, "%s: -- Active pf count : %u --", dev_conf.name, cnt);
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
        TraceVerbose(TRACE_QDMA, "qdma_interface allocation Success : %d", sizeof(xlnx::qdma_device));

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
        TraceError(TRACE_QDMA, "%s: Failed to read CSR Configuration", dev_conf.name);
        return status;
    }

    ret = hw.qdma_hw_error_enable((void *)this, hw.qdma_max_errors);
    if (ret < 0) {
        TraceInfo(TRACE_QDMA, "%s: Failed to enable qdma errors, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::init_func()
{
    int ret;
    struct qdma_fmap_cfg config = { 0 };

    UINT8 h2c = 0, c2h = 1;
    for (UINT8 i = 0; i < dev_conf.dev_info.mm_channel_max; i++) {
        ret = hw.qdma_mm_channel_conf((void *)this, i, h2c, TRUE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s: Failed to configure H2C MM channel %d, ret : %d",
                dev_conf.name, i, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_mm_channel_conf((void *)this, i, c2h, TRUE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s: Failed to configure C2H MM channel %d, ret : %d",
                dev_conf.name, i, ret);
            return hw.qdma_get_error_code(ret);
        }
    }

    config.qbase = static_cast<UINT16>(qbase);
    config.qmax  = static_cast<UINT16>(drv_conf.qsets_max);

    ret = hw.qdma_fmap_conf((void *)this,
                            dev_conf.dev_sbdf.sbdf.fun_no,
                            &config,
                            QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: Failed FMAP Programming for PF %d, ret: %d",
            dev_conf.name, dev_conf.dev_sbdf.sbdf.fun_no, ret);
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
}


NTSTATUS qdma_device::assign_bar_types()
{
    UINT8 user_bar_id = 0;
    int ret = hw.qdma_get_user_bar((void *)this,
                                   false,
                                   dev_conf.dev_sbdf.sbdf.fun_no,
                                   &user_bar_id);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: Failed to get AXI Master Lite BAR, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }
    user_bar_id = user_bar_id / 2;

    NTSTATUS status = pcie.assign_bar_types(user_bar_id);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: pcie assign_bar_types() failed: %!STATUS!", dev_conf.name, status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::init_dma_queues()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (drv_conf.qsets_max == 0)
        return STATUS_SUCCESS;

    queue_pairs = (queue_pair *)qdma_calloc(drv_conf.qsets_max, sizeof(queue_pair));
    if (nullptr == queue_pairs) {
        TraceError(TRACE_QDMA, "%s: Failed to allocate queue_pair memory", dev_conf.name);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (UINT16 qid = 0; qid < drv_conf.qsets_max; qid++) {
        auto& q = queue_pairs[qid];
        q.idx = qid;
        q.idx_abs = qid + static_cast<UINT16>(qbase);
        q.qdma = this;
        q.state = queue_state::QUEUE_AVAILABLE;

        if (drv_conf.operation_mode != queue_op_mode::POLL_MODE)
            q.h2c_q.lib_config.irq_en = q.c2h_q.lib_config.irq_en = true;
        else
            q.h2c_q.lib_config.irq_en = q.c2h_q.lib_config.irq_en = false;

        /* clear all context fields for this queue */
        status = clear_contexts(q);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: queue_pair::clear_contexts() failed for queue %d! %!STATUS!", dev_conf.name, q.idx_abs, status);
            destroy_dma_queues();
            return status;
        }
    }

    TraceVerbose(TRACE_QDMA, "%s: All queue contexts are cleared", dev_conf.name);

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
    for (auto vec = irq_mgr.data_vector_id_start; vec <= irq_mgr.data_vector_id_end; ++vec) {
        auto& intr_q = irq_mgr.intr_q[vec];
        intr_q.idx = (UINT16)vec;
        intr_q.idx_abs = (UINT16)(vec + (dev_conf.dev_sbdf.sbdf.fun_no * qdma_max_msix_vectors_per_pf));
        intr_q.qdma = this;

        status = intr_q.create(dma_enabler);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: intr_queue::create_resources() failed! %!STATUS!", dev_conf.name, status);
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
            TraceError(TRACE_QDMA, "%s: Failed to clear interrupt context \
                for intr queue %d, ret : %d", dev_conf.name, intr_q.idx_abs, ret);
            return hw.qdma_get_error_code(ret);
        }

        /* set the interrupt context for this queue */
        intr_ctx.valid = true;
        intr_ctx.vec = (UINT16)intr_q.vector;
        intr_ctx.color = intr_q.color;
        intr_ctx.func_id = dev_conf.dev_sbdf.sbdf.fun_no;
        intr_ctx.page_size = (UINT8)(intr_q.npages - 1);
        intr_ctx.baddr_4k = WdfCommonBufferGetAlignedLogicalAddress(intr_q.buffer).QuadPart;

        TraceVerbose(TRACE_DBG, "%s: SETTING intr_ctxt={ \
            BaseAddr=%llX, color=%u, page_sz=%u, vector=%u }",
            dev_conf.name, intr_ctx.baddr_4k, intr_ctx.color, intr_ctx.page_size, intr_ctx.vec);

        ret = hw.qdma_indirect_intr_ctx_conf((void *)this,
                                             intr_q.idx_abs,
                                             &intr_ctx,
                                             QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s: Failed to program interrupt \
                ring for intr queue %d, ret: %d", dev_conf.name, intr_q.idx_abs, ret);
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
        TraceError(TRACE_QDMA, "%s: WdfDmaEnablerCreate() failed: %!STATUS!", dev_conf.name, status);
        return status;
    }

    if ((dev_conf.is_master_pf) &&
        (drv_conf.operation_mode == queue_op_mode::POLL_MODE)) {
        int ret = hw.qdma_hw_error_enable(this, hw.qdma_max_errors);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s: qdma_error_enable() failed, ret : %d", dev_conf.name, ret);
            return hw.qdma_get_error_code(ret);
        }

        /* Create a thread for polling errors */
        status = th_mgr.create_err_poll_thread(this);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: create_err_poll_thread() failed: %!STATUS!", dev_conf.name, status);
            return status;
        }
    }

    status = th_mgr.create_sys_threads(drv_conf.operation_mode);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: create_sys_threads() failed: %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    /* INTERRUPT HANDLING */
    if (drv_conf.operation_mode != queue_op_mode::POLL_MODE) {
        /* Get MSIx vector count */
        status = pcie.find_num_msix_vectors(wdf_dev);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: pcie.get_num_msix_vectors() failed! %!STATUS!", dev_conf.name, status);
            goto ErrExit;
        }

        status = intr_setup(resources, resources_translated);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: intr_setup() failed: %!STATUS!", dev_conf.name, status);
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

    if (drv_conf.operation_mode == queue_op_mode::POLL_MODE) {
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
        TraceError(TRACE_QDMA, "%s: Resource lock creation failed!", dev_conf.name);
        return STATUS_UNSUCCESSFUL;
    }

    int ret = qdma_master_resource_create(dev_conf.dev_sbdf.sbdf.bus_no,
                                          dev_conf.dev_sbdf.sbdf.bus_no,
                                          QDMA_QBASE,
                                          QDMA_TOTAL_Q,
                                          &dma_dev_index);
    if (ret < 0 && ret != -QDMA_ERR_RM_RES_EXISTS) {
        TraceError(TRACE_QDMA, "%s: qdma_master_resource_create() failed!, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    TraceVerbose(TRACE_QDMA, "%s: Device Index : %u Bus start : %u Bus End : %u",
        dev_conf.name, dma_dev_index, dev_conf.dev_sbdf.sbdf.bus_no, dev_conf.dev_sbdf.sbdf.bus_no);

    ret = qdma_dev_entry_create(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_dev_entry_create() failed for function : %u, ret : %d",
            dev_conf.name, dev_conf.dev_sbdf.sbdf.fun_no, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    ret = qdma_dev_update(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, drv_conf.qsets_max, &qbase);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_dev_update() failed for function : %u, ret : %d", 
            dev_conf.name, dev_conf.dev_sbdf.sbdf.fun_no, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    TraceVerbose(TRACE_QDMA, "%s: QDMA Function : %u, qbase : %u, qmax : %u",
        dev_conf.name, dev_conf.dev_sbdf.sbdf.fun_no, qbase, drv_conf.qsets_max);

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
        return STATUS_INSUFFICIENT_RESOURCES;
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
    stats.tot_desc_accepted = stats.tot_desc_processed = 0;

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

    INIT_LIST_HEAD(&req_list_head);

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
        TraceError(TRACE_QDMA, "%s: desc_ring.create failed: %!STATUS!", 
            qdma->dev_conf.name, status);
        return status;
    }

    init_csr_h2c_pidx_info();

    /** Create and initialize MM/ST_H2C request tracker of
      * descriptor ring size, This is shadow ring and hence the
      * size is exact descriptor ring size
      */
    status = req_tracker.create(lib_config.ring_sz);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_dma_req_tracker() failed: %!STATUS!", 
            qdma->dev_conf.name, status);
        goto ErrExit;
    }

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: WdfSpinLockCreate() failed: %!STATUS!", 
            qdma->dev_conf.name, status);
        goto ErrExit;
    }

#ifdef TEST_64B_DESC_BYPASS_FEATURE
    if (user_conf.is_st && user_conf.desc_bypass_en && user_conf.sw_desc_sz == 3) {
        TraceVerbose(TRACE_QDMA, "%s: Initializing the descriptors with "
            "preloaded data to test TEST_64B_DESC_BYPASS_FEATURE", qdma->dev_conf.name);

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

    INIT_LIST_HEAD(&req_list_head);

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
        TraceError(TRACE_QDMA, "%s: desc_ring.create failed: %!STATUS!", qdma->dev_conf.name, status);
        return status;
    }

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_INTR, "%s: WdfSpinLockCreate failed: %!STATUS!", qdma->dev_conf.name, status);
        goto ErrExit;
    }

    if (user_conf.is_st) {
        /** Create and initialize ST C2H dma request tracker of
          * descriptor ring size to allow minimum of desc_ring entry requests
          * when each request occupies a single desc_ring entry
          */
        status = st_c2h_req_tracker.create(lib_config.ring_sz);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: Failed to init() ST C2H req tracker failed: %!STATUS!", 
                qdma->dev_conf.name, status);
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
            TraceError(TRACE_QDMA, "%s: Failed to init() MM C2H req tracker failed: %!STATUS!", 
                qdma->dev_conf.name, status);
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
            TraceError(TRACE_QDMA, "%s: desc_ring.create failed: %!STATUS!", 
                qdma->dev_conf.name, status);
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
        TraceError(TRACE_QDMA, "%s: rx_buffers qdma_calloc failed: %!STATUS!", 
            qdma->dev_conf.name, status);
        status = STATUS_INSUFFICIENT_RESOURCES;
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
        TraceError(TRACE_QDMA, "%s: received pkts init failed: %!STATUS!", qdma->dev_conf.name, status);
        goto ErrExit;
    }

    pkt_frag_list = static_cast<st_c2h_pkt_fragment*>(qdma_calloc(desc_ring_capacity,
                                                      sizeof(st_c2h_pkt_fragment)));
    if (nullptr == pkt_frag_list) {
        TraceError(TRACE_QDMA, "%s: rx_buffers qdma_calloc failed: %!STATUS!", qdma->dev_conf.name, status);
        status = STATUS_INSUFFICIENT_RESOURCES;
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
        return STATUS_INSUFFICIENT_RESOURCES;
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
        return STATUS_INSUFFICIENT_RESOURCES;
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

    RtlStringCchPrintfA(name, ARRAYSIZE(name), "%s-%s-%d",
        qdma->dev_conf.name, ((conf.is_st) ? "ST" : "MM"), idx);

    /* h2c common buffer */
    status = h2c_q.create(qdma, conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s h2c_q.create failed: %!STATUS!", this->name, status);
        return status;
    }

    /* c2h common buffer */
    status = c2h_q.create(qdma, conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s c2h_q.create failed: %!STATUS!", this->name, status);
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
    TraceVerbose(TRACE_QDMA, "%s-H2C updating h2c pidx to %u", this->name, new_pidx);

    MemoryBarrier();
    h2c_q.desc_ring.sw_index = new_pidx;
    h2c_q.csr_pidx_info.pidx = (UINT16)new_pidx;
    qdma->hw.qdma_queue_pidx_update(qdma, false /* is_vf */,
                                    idx, false /* is_c2h */,
                                    &h2c_q.csr_pidx_info);
}

PFORCEINLINE void queue_pair::update_sw_index_with_csr_c2h_pidx(UINT32 new_pidx)
{
    TraceVerbose(TRACE_QDMA, "%s-C2H updating c2h pidx to %u", this->name, new_pidx);

    MemoryBarrier();
    c2h_q.desc_ring.sw_index = new_pidx;
    c2h_q.csr_pidx_info.pidx = (UINT16)new_pidx;
    qdma->hw.qdma_queue_pidx_update(qdma, false /* is_vf */,
                                    idx, true /* is_c2h */,
                                    &c2h_q.csr_pidx_info);
}

PFORCEINLINE void queue_pair::update_sw_index_with_csr_wb(UINT32 new_cidx)
{
    TraceVerbose(TRACE_QDMA, "%s-C2H updating wb cidx to %u", this->name, new_cidx);

    c2h_q.cmpt_ring.sw_index = new_cidx;
    c2h_q.csr_cmpt_cidx_info.wrb_cidx = (UINT16)new_cidx;
    qdma->hw.qdma_queue_cmpt_cidx_update(qdma, false, idx,
                                         &c2h_q.csr_cmpt_cidx_info);
}

service_status queue_pair::process_mm_request(dma_request* request, size_t* xfer_len)
{
    bool                    poll;
    ring_buffer             *desc_ring;
    dma_req_tracker         *tracker;
    ULONG                   no_of_sub_elem = 0;
    UINT32                  credits = 0;
    ULONG                   sg_index;
    WDF_DMA_DIRECTION       direction;
    PSCATTER_GATHER_LIST    sg_list;
    LONGLONG                device_offset;
    dma_completion_cb       compl_cb;
    VOID                    *priv;
    poll_operation_entry    *poll_entry;
    char                    *dir_name;
    service_status          status = service_status::SERVICE_CONTINUE;

    /** Validate the parameters */
    if ((request == nullptr) || (xfer_len == NULL))
        return service_status::SERVICE_ERROR;

    /** Read the request parameters */
    direction       = request->direction;
    sg_list         = request->sg_list;
    device_offset   = (request->device_offset + request->offset_idx);
    compl_cb        = request->compl_cb;
    priv            = request->priv;

    if (direction == WdfDmaDirectionWriteToDevice) {
        desc_ring = &h2c_q.desc_ring;
        tracker = &h2c_q.req_tracker;
        poll = !h2c_q.lib_config.irq_en;
        poll_entry = h2c_q.poll_entry;
        dir_name = (char *)"H2C";
    }
    else {
        desc_ring = &c2h_q.desc_ring;
        tracker = &c2h_q.req_tracker;
        poll = !c2h_q.lib_config.irq_en;
        poll_entry = c2h_q.poll_entry;
        dir_name = (char *)"C2H";
    }

    credits = desc_ring->get_num_free_entries();
    if (credits == 0) {
        TraceError(TRACE_QDMA, "%s-%s: No space [%u] in sg dma list", 
            this->name, dir_name, credits);
        return status;
    }

    UINT32 ring_idx = desc_ring->sw_index;
    bool is_credit_avail = true;
    size_t dmaed_len = 0;

    TraceVerbose(TRACE_QDMA, "%s-%s enqueueing %u sg list at ring_index=%u FreeDesc : %u",
        this->name, dir_name, (sg_list->NumberOfElements - request->sg_index), ring_idx, credits);

    const auto desc = static_cast<mm_descriptor*>(desc_ring->get_va());

    for (sg_index = request->sg_index;
        ((sg_index < sg_list->NumberOfElements) && (is_credit_avail)); sg_index++) {

        size_t len = 0;
        size_t remain_len = sg_list->Elements[sg_index].Length;
        size_t frag_len = mm_max_desc_data_len;

        if (sg_list->Elements[sg_index].Length > mm_max_desc_data_len) {
            UINT32 frag_cnt = (UINT32)(sg_list->Elements[sg_index].Length / sg_frag_len);
            if (sg_list->Elements[sg_index].Length % sg_frag_len)
                frag_cnt++;

            if (frag_cnt > credits) {
                TraceInfo(TRACE_QDMA, "%s-%s Sufficient credts are not available to fit sg element, "
                    "frag_cnt : %d, credits : %d\n", this->name, dir_name, frag_cnt, credits);

                break;
            }

            frag_len = sg_frag_len;
            TraceInfo(TRACE_QDMA, "%s-%s sg_list len > mm_max_desc_data_len(%lld > %lld), "
                "splitting at idx: %d", this->name, dir_name, sg_list->Elements[sg_index].Length, 
                mm_max_desc_data_len, sg_index);
        }

        while (remain_len != 0) {
            size_t part_len = min(remain_len, frag_len);

            desc[ring_idx].length = part_len;
            desc[ring_idx].valid = true;
            desc[ring_idx].sop = (sg_index == 0);
            desc[ring_idx].eop = (sg_index == (sg_list->NumberOfElements - 1)) && (remain_len == part_len);

            if (direction == WdfDmaDirectionWriteToDevice) {
                desc[ring_idx].addr = sg_list->Elements[sg_index].Address.QuadPart + len;
                desc[ring_idx].dest_addr = device_offset;
            }
            else {
                desc[ring_idx].addr = device_offset;
                desc[ring_idx].dest_addr = sg_list->Elements[sg_index].Address.QuadPart + len;
            }

            len = len + part_len;
            remain_len = remain_len - part_len;

            device_offset += part_len;
            dump(desc[ring_idx]);

            tracker->requests[ring_idx].compl_cb = nullptr;
            /** Set Call back function if EOP is SET */
            if (desc[ring_idx].eop == true) {
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

    if (sg_index != request->sg_index) {
        request->sg_index = sg_index;
        request->offset_idx = device_offset - request->device_offset;

        /** When sg_index reaches the end, request forward done */
        if (request->sg_index >= sg_list->NumberOfElements)
            status = service_status::SERVICE_FINISHED;

        UINT32 accepted_desc = desc_ring->idx_delta(desc_ring->sw_index, ring_idx);
        desc_ring->stats.tot_desc_accepted += accepted_desc;

        *xfer_len = dmaed_len;

        TraceVerbose(TRACE_QDMA, "%s-%s +++ Request added at : %u +++", 
            this->name, dir_name, (ring_idx - 1));
        TraceVerbose(TRACE_QDMA, "%s-%s MM Accepted Desc : %d", 
            this->name, dir_name, accepted_desc);

#ifdef STRESS_TEST
        KeStallExecutionProcessor(2);
#endif

        if (direction == WdfDmaDirectionWriteToDevice) {
            update_sw_index_with_csr_h2c_pidx(ring_idx);
            TraceVerbose(TRACE_QDMA, "%s-%s csr[%u].h2c_dsc_pidx=%u", 
                this->name, dir_name, idx_abs, ring_idx);
        }
        else {
            update_sw_index_with_csr_c2h_pidx(ring_idx);
            TraceVerbose(TRACE_QDMA, "%s-%s csr[%u].c2h_dsc_pidx=%u", 
                this->name, dir_name, idx_abs, ring_idx);
        }

        if (poll)
            wakeup_thread(poll_entry->thread);
    }

    return status;
}

service_status queue_pair::process_st_h2c_request(dma_request* request, size_t* xfer_len)
{
    bool                    poll;
    ring_buffer             *desc_ring;
    dma_req_tracker         *tracker;
    poll_operation_entry    *poll_entry;
    ULONG                   NumberOfElements;
    ULONG                   no_of_sub_elem = 0;
    UINT32                  credits;
    PSCATTER_GATHER_LIST    sg_list;
    dma_completion_cb       compl_cb;
    VOID                    *priv;
    ULONG                   sg_index;
    service_status          status = service_status::SERVICE_CONTINUE;

    /** Validate the parameters */
    if ((request == nullptr) || (xfer_len == NULL))
        return service_status::SERVICE_ERROR;

    /** ST C2H has different execution flow */
    if (request->direction != WdfDmaDirectionWriteToDevice)
        return service_status::SERVICE_ERROR;

    /** Read the request parameters */
    sg_list     = request->sg_list;
    compl_cb    = request->compl_cb;
    priv        = request->priv;

    desc_ring           = &h2c_q.desc_ring;
    tracker             = &h2c_q.req_tracker;
    poll                = !h2c_q.lib_config.irq_en;
    poll_entry          = h2c_q.poll_entry;
    NumberOfElements    = sg_list->NumberOfElements;

    credits = desc_ring->get_num_free_entries();
    if (credits == 0) {
        TraceError(TRACE_QDMA, "%s-H2C: No space [%u] in sg dma list", this->name, credits);
        return status;
    }

    UINT32 pidx = desc_ring->sw_index;
    const auto desc = static_cast<h2c_descriptor *>(desc_ring->get_va());
    bool is_credit_avail = true;
    size_t dmaed_len = 0;
    
    TraceVerbose(TRACE_QDMA, "%s-H2C enqueueing %u sg list at ring_index=%u FreeDesc : %u",
        this->name, (NumberOfElements - request->sg_index), pidx, credits);

    for (sg_index = request->sg_index; ((sg_index < NumberOfElements) && (is_credit_avail)); sg_index++) {
        size_t len = 0;
        size_t remain_len = sg_list->Elements[sg_index].Length;
        size_t frag_len = st_max_desc_data_len;

        /* If the SG element length is more than ST supported len,
            * then Fragment the SG element into sub SG elements */
        if (sg_list->Elements[sg_index].Length > st_max_desc_data_len) {
            UINT32 frag_cnt = (UINT32)(sg_list->Elements[sg_index].Length / sg_frag_len);
            if (sg_list->Elements[sg_index].Length % sg_frag_len)
                frag_cnt++;

            if (frag_cnt > credits) {
                TraceInfo(TRACE_QDMA, "%s-H2C: Sufficient credits are not available to fit sg element, "
                    "frag_cnt : %d, credits : %d\n", this->name, frag_cnt, credits);

                break;
            }

            frag_len = sg_frag_len;
            TraceInfo(TRACE_QDMA, "%s-H2C sg_list len > st_max_desc_data_len(%lld > %lld), "
                "splitting at idx: %d", this->name, sg_list->Elements[sg_index].Length, 
                st_max_desc_data_len, sg_index);
        }

        /** do while loop is to support ST Zero length packet transfers */
        do {
            size_t part_len = min(remain_len, frag_len);

            desc[pidx].addr     = sg_list->Elements[sg_index].Address.QuadPart + len;
            desc[pidx].length   = (UINT16)part_len;
            desc[pidx].pld_len  = (UINT16)part_len;
            desc[pidx].sop      = (sg_index == 0);
            desc[pidx].eop      = (sg_index == (NumberOfElements - 1) && (remain_len == part_len));

            len = len + part_len;
            remain_len = remain_len - part_len;

            dump(desc[pidx]);

            tracker->requests[pidx].compl_cb = nullptr;
            /** Set Call back function if EOP is SET */
            if (desc[pidx].eop == true) {
                tracker->requests[pidx].compl_cb = (dma_completion_cb)compl_cb;
                tracker->requests[pidx].priv = priv;
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

    if (sg_index != request->sg_index) {
        request->sg_index = sg_index;

        /** When sg_index reaches the end, request forward done */
        if (request->sg_index >= sg_list->NumberOfElements)
            status = service_status::SERVICE_FINISHED;

        UINT32 accepted_desc = desc_ring->idx_delta(desc_ring->sw_index, pidx);
        desc_ring->stats.tot_desc_accepted += accepted_desc;

        *xfer_len = dmaed_len;

        TraceVerbose(TRACE_QDMA, "%s-H2C +++ Request added at : %u +++", this->name, (pidx - 1));
        TraceVerbose(TRACE_QDMA, "%s-H2C ST Accepted Desc : %d", this->name, accepted_desc);

        update_sw_index_with_csr_h2c_pidx(pidx);
        TraceVerbose(TRACE_QDMA, "%s-H2C csr[%u].h2c_dsc_pidx=%u", this->name, idx_abs, pidx);

        if (poll)
            wakeup_thread(poll_entry->thread);

    }
    return status;
}

service_status queue_pair::service_mm_st_h2c_completions(ring_buffer *desc_ring, 
    dma_req_tracker *tracker, UINT32 budget, UINT32 &proc_desc_cnt)
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
            TraceVerbose(TRACE_QDMA, "%s: --- COMPLETING REQ AT : %u ---", this->name, index);
            completed_req.compl_cb(completed_req.priv, STATUS_SUCCESS);
            completed_req.compl_cb = nullptr;
            completed_req.priv = nullptr;
        }
    }

    proc_desc_cnt = desc_ring->idx_delta(old_cidx, index);
    desc_ring->stats.tot_desc_processed += proc_desc_cnt;

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
    TraceVerbose(TRACE_QDMA, "%s: Completed desc count qid[%u] : %ld", 
        this->name, idx_abs, processed_desc_cnt);
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
        TraceVerbose(TRACE_QDMA, "%s: Completion ring sw index : %u hw pidx : %u", this->name, prev_pidx, current_pidx);
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
                TraceError(TRACE_QDMA, "%s: check_cmpt_error returned error", this->name);
                return service_status::SERVICE_ERROR;
            }

            if (c2h_cmpt_ring_va->desc_used) {
                /* Consume the used packet */
                status = process_st_c2h_data_pkt((void *)c2h_cmpt_ring_va, c2h_cmpt_ring_va->length);
                if (!NT_SUCCESS(status)) {
                    TraceError(TRACE_QDMA, "%s: process_st_c2h_data_pkt() failed: %!STATUS!", this->name, status);
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
                TraceVerbose(TRACE_QDMA, "%s: No pending requests. Available Packets : %Iu", this->name, pkt_cnt);
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
            TraceInfo(TRACE_QDMA, "%s: req_len : %lld, rxd_len:%lld", 
                this->name, req.len, c2h_q.pkt_frag_queue.get_avail_byte_cnt());
            update_c2h_pidx_in_batch(processed_desc_cnt);
            ret = service_status::SERVICE_CONTINUE;
            break;
        }
    }

    TraceVerbose(TRACE_QDMA, "%s: Completed Request count : %ld", 
        this->name, processed_dma_requests);
    return ret;
}

void process_queue_req(queue_pair* queue, WDFSPINLOCK lock, PLIST_ENTRY req_list_head)
{
    PLIST_ENTRY entry;
    PLIST_ENTRY temp;
    service_status status;
    UINT32 req_service_cnt = 0;
    size_t xfer_len = 0;

    if ((queue == nullptr) || (lock == nullptr) || (req_list_head == nullptr)) {
        TraceError(TRACE_QDMA, "Invalid Parameters : queue : %p, "
            "lock : %p, req_list_head : %p", queue, lock, req_list_head);
        return;
    }

    if (queue->state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state: %d", 
            queue->name, queue->state);
        return;
    }

    /** Come out of the loop only
      * "when the list is empty" or
      * "when there is no room in descriptor ring (!SERVICE_FINISHED)" or
      * "when max request service reached"
      */
    LIST_FOR_EACH_ENTRY_SAFE(req_list_head, temp, entry) {
        dma_request* request = LIST_GET_ENTRY(entry, dma_request, list_entry);
        if (queue->type == queue_type::STREAMING) {
            status = queue->process_st_h2c_request(request, &xfer_len);
        }
        else {
            status = queue->process_mm_request(request, &xfer_len);
        }
        switch (status) {
        case service_status::SERVICE_CONTINUE:
            /** Exit the loop as there is no room in descriptor ring */
            req_service_cnt = max_req_service_cnt;
            TraceVerbose(TRACE_QDMA, "%s-%s: Request Split", queue->name, 
                ((request->direction == WdfDmaDirectionReadFromDevice) ? "C2H" : "H2C"));
            break;

        case service_status::SERVICE_FINISHED:
            req_service_cnt++;
            WdfSpinLockAcquire(lock);
            LIST_DEL_NODE(entry);
            WdfSpinLockRelease(lock);
            qdma_memfree(request);
            break;

        case service_status::SERVICE_ERROR:
            /** Remove the request from the list,
              * Can be NULL pointer or ST C2H packet */
            WdfSpinLockAcquire(lock);
            LIST_DEL_NODE(entry);
            WdfSpinLockRelease(lock);

            if (request) {
                TraceError(TRACE_QDMA, "%s-%s: queue->process_XXX_request() SERVICE_ERROR", 
                    ((request->direction == WdfDmaDirectionReadFromDevice) ? "C2H" : "H2C"), 
                    queue->name);
                qdma_memfree(request);
            }
            break;
        default:
            TraceError(TRACE_QDMA, "%s-%s: queue->process_XXX_request() "
                "invalid return value (0x%X)", queue->name, 
                ((request->direction == WdfDmaDirectionReadFromDevice)? "C2H" : "H2C"), 
                static_cast<UINT32>(status));
            break;
        }

        if (req_service_cnt >= max_req_service_cnt) {
            TraceInfo(TRACE_QDMA, "%s: Max Requests serviced (%d)", queue->name, req_service_cnt);
            break;
        }
    }
}

void process_mm_h2c_req(void* arg)
{
    queue_pair* queue = static_cast<queue_pair*>(arg);

    process_queue_req(queue, queue->h2c_q.lock, &queue->h2c_q.req_list_head);
}

void process_mm_c2h_req(void* arg)
{
    queue_pair* queue = static_cast<queue_pair*>(arg);

    process_queue_req(queue, queue->c2h_q.lock, &queue->c2h_q.req_list_head);
}

void service_h2c_mm_queue(void* arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = mm_h2c_completion_weight;
    ring_buffer *desc_ring = &queue->h2c_q.desc_ring;
    dma_req_tracker *tracker = &queue->h2c_q.req_tracker;
    UINT32 proc_desc_cnt = 0;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget, proc_desc_cnt);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        if (proc_desc_cnt != 0) {
            TraceVerbose(TRACE_QDMA, "%s: MM H2C [%u] Flag request, "
                "proc_desc_cnt:%d", queue->name, queue->idx_abs, proc_desc_cnt);
            wakeup_thread(queue->h2c_q.req_proc_entry->thread);
        }
        wakeup_thread(queue->h2c_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "%s: MM H2C [%u] service error", queue->name, queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        wakeup_thread(queue->h2c_q.req_proc_entry->thread);
        TraceVerbose(TRACE_QDMA, "%s: ervice_h2c_mm_queue [%u] Completed", queue->name, queue->idx_abs);
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
    UINT32 proc_desc_cnt = 0;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget, proc_desc_cnt);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        if (proc_desc_cnt != 0) {
            TraceVerbose(TRACE_QDMA, "%s: MM C2H [%u] Flag request, "
                "proc_desc_cnt:%d", queue->name, queue->idx_abs, proc_desc_cnt);
            wakeup_thread(queue->c2h_q.req_proc_entry->thread);
        }
        wakeup_thread(queue->c2h_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "%s: MM C2H [%u] service error", queue->name, queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        wakeup_thread(queue->c2h_q.req_proc_entry->thread);
        TraceVerbose(TRACE_QDMA, "%s: service_c2h_mm_queue [%u] Completed", queue->name, queue->idx_abs);
    default:
        break;
    }
}

void process_st_h2c_req(void* arg)
{
    queue_pair* queue = static_cast<queue_pair*>(arg);

    process_queue_req(queue, queue->h2c_q.lock, &queue->h2c_q.req_list_head);
}

void service_h2c_st_queue(void *arg)
{
    queue_pair *queue = static_cast<queue_pair *>(arg);
    UINT32 budget = st_h2c_completion_weight;
    ring_buffer* desc_ring = &queue->h2c_q.desc_ring;
    dma_req_tracker* tracker = &queue->h2c_q.req_tracker;
    UINT32 proc_desc_cnt = 0;

    service_status status =
        queue->service_mm_st_h2c_completions(desc_ring, tracker, budget, proc_desc_cnt);
    switch (status) {
    case service_status::SERVICE_CONTINUE:
        if (proc_desc_cnt != 0) {
            TraceVerbose(TRACE_QDMA, "%s: ST H2C [%u] Flag request, "
                "proc_desc_cnt:%d", queue->name, queue->idx_abs, proc_desc_cnt);
            wakeup_thread(queue->h2c_q.req_proc_entry->thread);
        }
        wakeup_thread(queue->h2c_q.poll_entry->thread);
        break;
    case service_status::SERVICE_ERROR:
        TraceError(TRACE_QDMA, "%s: ST H2C [%u] service error", queue->name, queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        wakeup_thread(queue->h2c_q.req_proc_entry->thread);
        TraceVerbose(TRACE_QDMA, "%s: service_h2c_st_queue [%u] Completed", queue->name, queue->idx_abs);
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
        TraceError(TRACE_QDMA, "%s: ST C2H [%u] service error", queue->name, queue->idx_abs);
        break;
    case service_status::SERVICE_FINISHED:
        TraceVerbose(TRACE_QDMA, "%s: service_c2h_st_queue [%u] Completed", queue->name, queue->idx_abs);
    default:
        break;
    }
}

NTSTATUS queue_pair::enqueue_dma_request(dma_request *request)
{
    WDFSPINLOCK lock;
    PLIST_ENTRY req_list_head;
    dma_request* req_entry;
    poll_operation_entry* req_proc_entry;
    char *dir_name;

    if (request == nullptr)
        return STATUS_INVALID_PARAMETER;

    /** This function Only Support MM H2C, MM C2H and ST C2H.
      * Rule out the unsupported case
      */
    if ((request->is_st == true) && 
        (request->direction != WdfDmaDirectionWriteToDevice)) {
        return STATUS_NOT_SUPPORTED;
    }

    if (request->direction == WdfDmaDirectionWriteToDevice) {
        lock = h2c_q.lock;
        req_list_head = &h2c_q.req_list_head;
        req_proc_entry = h2c_q.req_proc_entry;
        dir_name = (char *)"H2C";
    }
    else {
        lock = c2h_q.lock;
        req_list_head = &c2h_q.req_list_head;
        req_proc_entry = c2h_q.req_proc_entry;
        dir_name = (char*)"C2H";
    }

    req_entry = static_cast<dma_request*>(qdma_calloc(1, sizeof(dma_request)));
    if (req_entry == nullptr) {
        TraceError(TRACE_QDMA, "%s: req_entry allocation failed", this->name);
        return STATUS_NO_MEMORY;
    }

    /** Copy the request details */
    RtlCopyMemory(req_entry, request, sizeof(dma_request));

    TraceVerbose(TRACE_QDMA, "%s-%s Req Added, Abs Queue: %d, Entry : %p",
        this->name, dir_name, idx_abs, 
        static_cast<void*>(&req_entry->list_entry));

    /** Add to the request linked list head */
    WdfSpinLockAcquire(lock);
    LIST_ADD_TAIL(req_list_head, &req_entry->list_entry);
    WdfSpinLockRelease(lock);

    /** Wake up the thread to forward the request to DMA engine */
    wakeup_thread(req_proc_entry->thread);

    return STATUS_SUCCESS;
}

NTSTATUS queue_pair::enqueue_dma_request(
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
        TraceError(TRACE_QDMA, "%s: Failed to add ST C2H request  %!STATUS!", this->name, status);
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
            TraceError(TRACE_QDMA, "%s: add fragment failed: %!STATUS!", this->name, status);
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

    TraceVerbose(TRACE_QDMA, "%s: UDD Len : %u", this->name, loc);

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
            TraceError(TRACE_QDMA, "%s: MM CMPT is not valid, HW MM CMPT EN : %d, SW MM CMPT EN : %d\n",
                this->name, qdma->dev_conf.dev_info.mm_cmpt_en, c2h_q.user_conf.en_mm_cmpl);
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
        TraceError(TRACE_QDMA, "%s: Descriptor Range is incorrect : start : %d, end : %d, RING SIZE : %d",
            this->name, desc_info->desc_start, desc_info->desc_end, desc_ring->get_capacity());
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
        TraceError(TRACE_QDMA, "%s: read_mm_cmpt_data supported only for MM queues", this->name);
        return STATUS_NOT_SUPPORTED;
    }

    if (!c2h_q.is_cmpt_valid) {
        TraceError(TRACE_QDMA, "%s: MM Completion is not enabled, "
            "HW En Status : %d, SW En Status : %d", this->name,
            qdma->dev_conf.dev_info.mm_cmpt_en, c2h_q.user_conf.en_mm_cmpl);

        return STATUS_NOT_CAPABLE;
    }

    WdfSpinLockAcquire(c2h_q.lock);
    const auto wb_status = reinterpret_cast<volatile c2h_wb_status*>(c2h_q.cmpt_ring.wb_status);
    auto prev_pidx = c2h_q.cmpt_ring.sw_index;
    auto current_pidx = wb_status->pidx;

    if (prev_pidx == current_pidx) {
        TraceError(TRACE_QDMA, "%s: No Completion data available : "
            "Prev PIDX : %d, Pres PIDX : %d", this->name, prev_pidx, current_pidx);
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
    const bool irq_enable = (drv_conf.operation_mode == queue_op_mode::POLL_MODE) ? false : true;
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

    TraceVerbose(TRACE_QDMA, "%s setting h2c contexts...", q.name);

    const bool irq_enable = (drv_conf.operation_mode == queue_op_mode::POLL_MODE) ? false : true;

    if (irq_enable)
        TraceVerbose(TRACE_QDMA, "%s Programming with IRQ", q.name);

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
        sw_ctx.intr_aggr = (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
    }

    ret = hw.qdma_sw_ctx_conf((void *)this, false, q.idx_abs, &sw_ctx, QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s Failed to program H2C Software context! ret : %d", q.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    if ((nullptr != hw.qdma_qid2vec_conf) && irq_enable) {
        /** Versal Hard IP supports explicit qid2vec context programming.
          * For other versions, vector Id is accommodated in software context.
          */
        qdma_qid2vec intr_ctx = {};
        intr_ctx.h2c_vector = (UINT8)q.h2c_q.lib_config.vector_id;
        intr_ctx.h2c_en_coal = (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;

        ret = hw.qdma_qid2vec_conf((void *)this, false, q.idx_abs, &intr_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to program qid2vec context! ret : %d", q.name, ret);
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

    TraceVerbose(TRACE_QDMA, "%s setting c2h contexts...", q.name);

    const bool irq_enable = (drv_conf.operation_mode == queue_op_mode::POLL_MODE) ? false : true;

    if (irq_enable)
        TraceVerbose(TRACE_QDMA, "%s Programming with IRQ", q.name);

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
        sw_ctx.intr_aggr = (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
    }

    ret = hw.qdma_sw_ctx_conf((void *)this, true, q.idx_abs, &sw_ctx, QDMA_HW_ACCESS_WRITE);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s Failed to program C2H Software context! ret : %d", q.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    if ((nullptr != hw.qdma_qid2vec_conf) && irq_enable) {
        /** Versal Hard IP supports explicit qid2vec context programming.
          * For other versions, vector Id is accommodated in software context.
          */
        qdma_qid2vec intr_ctx = {};
        intr_ctx.c2h_vector = (UINT8)q.c2h_q.lib_config.vector_id;
        intr_ctx.c2h_en_coal = (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;

        ret = hw.qdma_qid2vec_conf((void *)this, true, q.idx_abs, &intr_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to program qid2vec context!, ret : %d", q.name, ret);
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
            cmp_ctx.int_aggr = (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) ? true : false;
        }

        ret = hw.qdma_cmpt_ctx_conf((void *)this, q.idx_abs, &cmp_ctx, QDMA_HW_ACCESS_WRITE);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to program completion context! ret : %d", q.name, ret);
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
            TraceError(TRACE_QDMA, "%s Failed to program prefetch context!, ret : %d", q.name, ret);
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
    TraceVerbose(TRACE_QDMA, "%s programming...", q.name);

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

NTSTATUS qdma_device::init(qdma_drv_config conf)
{
    NTSTATUS status;

    TraceInfo(TRACE_QDMA, "Xilinx QDMA PF Reference Driver v%u.%u.%u",
        qdma_version.major, qdma_version.minor, qdma_version.patch);

    InterlockedExchange(&qdma_device_state, device_state::DEVICE_INIT);

    drv_conf = conf;
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

NTSTATUS qdma_device::get_bar_info(
    qdma_bar_type bar_type,
    PVOID &bar_base,
    size_t &bar_length)
{
    return pcie.get_bar_info(bar_type, bar_base, bar_length);
}

NTSTATUS qdma_device::qdma_is_queue_in_range(UINT16 qid)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Provided qid : %u is more than allocated : %u", dev_conf.name, qid, drv_conf.qsets_max);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS qdma_device::qdma_get_queues_state(
    _In_  UINT16 qid,
    _Out_ enum queue_state *qstate,
    _Out_writes_(str_maxlen) CHAR *str,
    _In_  size_t str_maxlen)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }
    if (qstate == NULL) {
        TraceError(TRACE_QDMA, "%s: qstate is NULL", dev_conf.name);
        return STATUS_INVALID_PARAMETER;
    }

    switch (queue_pairs[qid].state) {
    case queue_state::QUEUE_ADDED:
        *qstate = queue_state::QUEUE_ADDED;
        if (str != NULL)
            status = RtlStringCchCopyA(str, str_maxlen, "QUEUE ADDED");
        break;
    case queue_state::QUEUE_STARTED:
        *qstate = queue_state::QUEUE_STARTED;
        if (str != NULL)
            status = RtlStringCchCopyA(str, str_maxlen, "QUEUE ACTIVE");
        break;
    case queue_state::QUEUE_AVAILABLE:
        *qstate = queue_state::QUEUE_AVAILABLE;
        if (str != NULL)
            status = RtlStringCchCopyA(str, str_maxlen, "QUEUE IS AVAILABLE");
        break;
    case queue_state::QUEUE_BUSY:
        *qstate = queue_state::QUEUE_BUSY;
        if (str != NULL)
            status = RtlStringCchCopyA(str, str_maxlen, "QUEUE BUSY");
        break;
    default:
        NT_ASSERT(FALSE);
        *qstate = queue_state::QUEUE_INVALID_STATE;
        if (str != NULL)
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
        TraceError(TRACE_QDMA, "%s: Device OFFLINE, Can't proceed.", dev_conf.name);
        return STATUS_UNSUCCESSFUL;
    }

    UINT32 active_queues = qdma_get_active_queue_count(dma_dev_index);
    if (active_queues) {
        TraceError(TRACE_QDMA, "%s: Not possible to update qmax, %u queues are active", 
            dev_conf.name, active_queues);
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    int rv = qdma_dev_update(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, queue_max, &queue_base);
    if (rv < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_dev_update() Failed! %d", dev_conf.name, rv);
        status = hw.qdma_get_error_code(rv);
        goto ErrExit;
    }

    rv = qdma_dev_qinfo_get(dma_dev_index, dev_conf.dev_sbdf.sbdf.fun_no, &queue_base, &queue_max);
    if (rv < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_dev_qinfo_get() Failed! %d", dev_conf.name, rv);
        status = hw.qdma_get_error_code(rv);
        goto ErrExit;
    }

    TraceInfo(TRACE_QDMA, "%s: qmax updated. Old qmax : %u, New qmax : %u, "
        "Old qbase : %d, New qbase : %d",
        dev_conf.name, drv_conf.qsets_max, queue_max, qbase, queue_base);

    drv_conf.qsets_max = queue_max;
    qbase = queue_base;

    if (queue_pairs) {
        qdma_memfree(queue_pairs);
        queue_pairs = nullptr;
    }

    status = init_dma_queues();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_dma_queues() failed! %!STATUS!", dev_conf.name, status);
        drv_conf.qsets_max = 0u;
        qbase = -1;
    }

    status = init_func();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_func() failed! %!STATUS!", dev_conf.name, status);
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
        TraceError(TRACE_QDMA, "%s: %s mode is not enabled in device", 
            dev_conf.name, (conf.is_st ? "ST" : "MM"));
        return STATUS_INVALID_PARAMETER;
    }


    if ((hw_version_info.ip_type == EQDMA_SOFT_IP) &&
        (hw_version_info.vivado_release >= QDMA_VIVADO_2020_2)) {

        if (dev_conf.dev_info.desc_eng_mode == QDMA_DESC_ENG_BYPASS_ONLY) {
            TraceError(TRACE_QDMA, "Bypass only mode design is not supported");
            return STATUS_NOT_SUPPORTED;
        }

        if ((conf.desc_bypass_en == true) && 
            (dev_conf.dev_info.desc_eng_mode == QDMA_DESC_ENG_INTERNAL_ONLY)) {
            TraceError(TRACE_QDMA,
                "Queue config in bypass "
                "mode not supported on internal only mode design");
            return STATUS_NOT_SUPPORTED;
        }
    }


    if ((conf.h2c_ring_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_ring_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_buff_sz_index >= QDMA_GLBL_CSR_REG_CNT) ||
        (conf.c2h_th_cnt_index >= QDMA_GLBL_CSR_REG_CNT)  ||
        (conf.c2h_timer_cnt_index >= QDMA_GLBL_CSR_REG_CNT)) {
        TraceError(TRACE_QDMA, "%s: One or more invalid global CSR indexes provided", dev_conf.name);
        return STATUS_INVALID_PARAMETER;
    }

    if (conf.cmpt_sz >= qdma_desc_sz::QDMA_DESC_SZ_MAX) {
        TraceError(TRACE_QDMA, "%s: Invalid completion descriptor size provided", dev_conf.name);
        return STATUS_INVALID_PARAMETER;
    }

    if (conf.trig_mode >= qdma_trig_mode::QDMA_TRIG_MODE_MAX) {
        TraceError(TRACE_QDMA, "%s: Invalid Trigger mode provided", dev_conf.name);
        return STATUS_INVALID_PARAMETER;
    }

    if ((conf.en_mm_cmpl) && (!dev_conf.dev_info.mm_cmpt_en)) {
        TraceError(TRACE_QDMA, "%s: MM Completion not supported in HW", dev_conf.name);
        return STATUS_NOT_SUPPORTED;
    }

    if ((conf.cmpl_ovf_dis) && (!dev_conf.dev_info.cmpt_ovf_chk_dis)) {
        TraceError(TRACE_QDMA, "%s: Completion overflow disable option not supported in HW", dev_conf.name);
        return STATUS_NOT_SUPPORTED;
    }

    if (hw_version_info.ip_type == QDMA_VERSAL_HARD_IP) {
        /** 64B Descriptors not supportd for the versal hard IP (i.e., s80 device) */
        if ((conf.sw_desc_sz == static_cast<UINT8>(qdma_desc_sz::QDMA_DESC_SZ_64B)) ||
            (conf.cmpt_sz == qdma_desc_sz::QDMA_DESC_SZ_64B)) {
            TraceError(TRACE_QDMA, "%s: 64B Descriptor not supported for Versal Hard IP(S80)", dev_conf.name);
            return STATUS_NOT_SUPPORTED;
        }

        if ((conf.trig_mode == qdma_trig_mode::QDMA_TRIG_MODE_USER_TIMER_COUNT) ||
            (conf.trig_mode == qdma_trig_mode::QDMA_TRIG_MODE_USER_COUNT)) {
            TraceError(TRACE_QDMA, "%s: Counter, Timer+Counter modes are not supported for Versal Hard IP(S80)", dev_conf.name);
            return STATUS_NOT_SUPPORTED;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_add_queue(UINT16 qid, queue_config& conf)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
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
        TraceError(TRACE_QDMA, "%s : Queue %u is not available, q_state %d!", dev_conf.name, qid, q.state);
        return STATUS_UNSUCCESSFUL;
    }

    TraceVerbose(TRACE_QDMA, "%s: Adding the queue : %u", dev_conf.name, qid);

    status = q.create(conf);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: Failed to allocate resource for queue %u, status: %!STATUS!", dev_conf.name, qid, status);
        goto ErrExit;
    }

    /* Increament resource manager active queue count */
    inc_queue_pair_count(q.c2h_q.is_cmpt_valid);

    InterlockedExchange(&q.state, queue_state::QUEUE_ADDED);

    TraceInfo(TRACE_QDMA, "%s: queue added : %s", dev_conf.name, q.name);

    return status;

ErrExit:
    InterlockedExchange(&q.state, queue_state::QUEUE_AVAILABLE);
    return status;
}

NTSTATUS qdma_device::qdma_start_queue(UINT16 qid)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_ADDED);
    if (old_state != queue_state::QUEUE_ADDED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_UNSUCCESSFUL;
    }

    TraceVerbose(TRACE_QDMA, "%s: Starting the queue : %u", dev_conf.name, qid);

    bool is_intr_vec_allocated = false;
    NTSTATUS status = STATUS_SUCCESS;

    if (drv_conf.operation_mode != queue_op_mode::POLL_MODE) {
        auto vec = assign_interrupt_vector(q);
        if (vec < 0) {
            TraceError(TRACE_QDMA, "%s Failed to assign interrupt vector", q.name);
            status = STATUS_UNSUCCESSFUL;
            goto ErrExit;
        }

        q.h2c_q.lib_config.vector_id = q.c2h_q.lib_config.vector_id = vec;
        is_intr_vec_allocated = true;

        TraceInfo(TRACE_QDMA, "%s Allocated vector Id : %u", q.name, q.c2h_q.lib_config.vector_id);
    }

    /* H2C request process function registration */
    poll_op req_op;
    bool is_st = q.h2c_q.user_conf.is_st;
    req_op.fn = is_st ? process_st_h2c_req : process_mm_h2c_req;
    req_op.arg = &queue_pairs[qid];

    q.h2c_q.req_proc_entry = th_mgr.register_poll_function(req_op);
    if (nullptr == q.h2c_q.req_proc_entry) {
        TraceError(TRACE_QDMA, "%s Failed to register H2C request process method", q.name);
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    /* C2H request process function registration */
    is_st = q.c2h_q.user_conf.is_st;
    if (is_st == false) {
        req_op.fn = process_mm_c2h_req;

        q.c2h_q.req_proc_entry = th_mgr.register_poll_function(req_op);
        if (nullptr == q.c2h_q.req_proc_entry) {
            TraceError(TRACE_QDMA, "%s Failed to register C2H request process method", q.name);
            status = STATUS_UNSUCCESSFUL;
            goto ErrExit;
        }
    }

    /* H2C Poll function registration */
    poll_op compl_op;
    is_st = q.h2c_q.user_conf.is_st;
    compl_op.fn = is_st ? service_h2c_st_queue : service_h2c_mm_queue;
    compl_op.arg = &queue_pairs[qid];

    q.h2c_q.poll_entry = th_mgr.register_poll_function(compl_op);
    if (nullptr == q.h2c_q.poll_entry) {
        TraceError(TRACE_QDMA, "%s Failed to register H2C Completion poll method", q.name);
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    /* C2H Poll function registration */
    is_st = q.c2h_q.user_conf.is_st;
    compl_op.fn = is_st ? service_c2h_st_queue : service_c2h_mm_queue;

    q.c2h_q.poll_entry = th_mgr.register_poll_function(compl_op);
    if (nullptr == q.c2h_q.poll_entry) {
        TraceError(TRACE_QDMA, "%s Failed to register C2H Completion poll method", q.name);
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    status = queue_program(q);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s failed to program contexts, status : %!STATUS!", q.name, status);
        goto ErrExit;
    }

    dump(this, q.type, q.idx_abs);

    q.init_csr();

    InterlockedExchange(&q.state, queue_state::QUEUE_STARTED);

    TraceInfo(TRACE_QDMA, "%s: queue started : %s", dev_conf.name, q.name);

    return status;

ErrExit:
    if (nullptr != q.h2c_q.poll_entry) {
        th_mgr.unregister_poll_function(q.h2c_q.req_proc_entry);
        q.h2c_q.req_proc_entry = nullptr;
    }

    if (q.c2h_q.req_proc_entry) {
        if (q.type == queue_type::MEMORY_MAPPED) {
            th_mgr.unregister_poll_function(q.c2h_q.req_proc_entry);
            q.c2h_q.req_proc_entry = nullptr;
        }
    }

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
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    auto& q = queue_pairs[qid];

    TraceVerbose(TRACE_QDMA, "%s: Stopping the queue : %u", dev_conf.name, qid);

    /* Stop further traffic on the queue */
    auto old_state = InterlockedCompareExchange(&q.state,
                                                 queue_state::QUEUE_BUSY,
                                                 queue_state::QUEUE_STARTED);
    if (old_state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_UNSUCCESSFUL;
    }

    /* Wait till incoming traffic stops & let HW finish any pending jobs */
    LARGE_INTEGER wait_time;
    wait_time.QuadPart = WDF_REL_TIMEOUT_IN_MS(2);
    /** Re-adjust the system tick for better resolution 
      * 
      * As the system tick in windows can go up to 15.625 msec
      * the delay is varying between 1 msec to 15 msec
      */
    ExSetTimerResolution((ULONG)(2 * WDF_TIMEOUT_TO_MS), TRUE);

    KeDelayExecutionThread(KernelMode, FALSE, &wait_time);
    
    /** Revert back the system tick to default */
    ExSetTimerResolution(0, FALSE);

    th_mgr.unregister_poll_function(q.h2c_q.poll_entry);
    th_mgr.unregister_poll_function(q.c2h_q.poll_entry);

    th_mgr.unregister_poll_function(q.h2c_q.req_proc_entry);
    if (q.type == queue_type::MEMORY_MAPPED)
        th_mgr.unregister_poll_function(q.c2h_q.req_proc_entry);

    q.h2c_q.poll_entry = nullptr;
    q.c2h_q.poll_entry = nullptr;

    if (drv_conf.operation_mode != queue_op_mode::POLL_MODE)
        free_interrupt_vector(q, q.c2h_q.lib_config.vector_id);

    /* Drain any pending requests */
    q.flush_queue();

    /* Invalidate the contexts */
    status = clear_contexts(q, true);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s Failed to invalidate the context", q.name);
        /* Continue further even in error scenario */
    }

    InterlockedExchange(&q.state, queue_state::QUEUE_ADDED);

    TraceInfo(TRACE_QDMA, "%s: queue stopped : %s", dev_conf.name, q.name);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_remove_queue(UINT16 qid)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    TraceVerbose(TRACE_QDMA, "%s: Removing the queue : %u", dev_conf.name, qid);

    auto old_state = InterlockedCompareExchange(&q.state,
                                                queue_state::QUEUE_BUSY,
                                                queue_state::QUEUE_ADDED);
    if (old_state != queue_state::QUEUE_ADDED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_UNSUCCESSFUL;
    }

    /* Clear queue context */
    NTSTATUS status = clear_contexts(q);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s Failed to clear the context", q.name);
        /* Continue further even in error scenario */
    }

    auto is_cmpt_valid = q.c2h_q.is_cmpt_valid;

    q.destroy();

    /* Decreament resource manager active queue count */
    dec_queue_pair_count(is_cmpt_valid);

    InterlockedExchange(&q.state, queue_state::QUEUE_AVAILABLE);

    TraceInfo(TRACE_QDMA, "%s: queue removed : %s", dev_conf.name, q.name);

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_enqueue_mm_request(
    UINT16 qid,
    WDF_DMA_DIRECTION direction,
    PSCATTER_GATHER_LIST sg_list,
    LONGLONG device_offset,
    dma_completion_cb compl_cb,
    VOID *priv)
{
    if ((qid >= drv_conf.qsets_max) || (compl_cb == nullptr) ||
        (sg_list == nullptr)) {
        TraceError(TRACE_QDMA, "%s : Incorrect Parameters, qid : %d, "
            "compl_cb : %p, sg_list : %p", dev_conf.name, qid, compl_cb, sg_list);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::MEMORY_MAPPED) {
        TraceError(TRACE_QDMA, "%s is not configured in MM mode!", q.name);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s enqueue request", q.name);

    dma_request request;

    request.is_st = false;
    request.direction = direction;
    request.sg_list = sg_list;
    request.device_offset = device_offset;
    request.compl_cb = compl_cb;
    request.priv = priv;
    request.sg_index = 0;
    request.offset_idx = 0;

    NTSTATUS status = q.enqueue_dma_request(&request);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s Failed to enqueue the MM request, Status : %!STATUS!", q.name, status);
        return status;
    }

    return status;
}

NTSTATUS qdma_device::qdma_enqueue_st_tx_request(
    UINT16 qid,
    PSCATTER_GATHER_LIST sg_list,
    dma_completion_cb compl_cb,
    VOID *priv)
{
    if ((qid >= drv_conf.qsets_max) || (compl_cb == nullptr) ||
        (sg_list == nullptr)) {
        TraceError(TRACE_QDMA, "%s : Incorrect Parameters, qid : %d, "
            "compl_cb : %p, sg_list : %p", dev_conf.name, qid, compl_cb, sg_list);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s TX enqueue request", q.name);

    dma_request request;

    request.is_st = true;
    request.direction = WdfDmaDirectionWriteToDevice;
    request.sg_list = sg_list;
    request.compl_cb = compl_cb;
    request.priv = priv;
    request.sg_index = 0;
    request.offset_idx = 0;

    NTSTATUS status = q.enqueue_dma_request(&request);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s Failed to enqueue the ST TX request, Status : %!STATUS!", q.name, status);
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
    if ((qid >= drv_conf.qsets_max) || (compl_cb == nullptr)) {
        TraceError(TRACE_QDMA, "%s: Incorrect Parameters, qid : %d, compl_cb : %p len : %Iu",
            dev_conf.name, qid, compl_cb, length);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s RX enqueue request", q.name);

    NTSTATUS status = q.enqueue_dma_request(length, compl_cb, priv);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s Failed to enqueue the ST RX request, Status : %!STATUS!", q.name, status);
    }

    return status;
}

NTSTATUS qdma_device::qdma_retrieve_st_udd_data(UINT16 qid, void *addr, UINT8 *buf, UINT32 *len)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }

    if ((addr == nullptr) || (buf == nullptr) || (len == nullptr))
        return STATUS_INVALID_PARAMETER;

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s Retrieving UDD data", q.name);

    return q.read_st_udd_data(addr, buf, len);
}

NTSTATUS qdma_device::qdma_retrieve_last_st_udd_data(UINT16 qid, UINT8 *buf, UINT32 *len)
{
    if (qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid);
        return STATUS_INVALID_PARAMETER;
    }

    if ((buf == nullptr) || (len == nullptr))
        return STATUS_INVALID_PARAMETER;

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::STREAMING) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in ST mode!", qid);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s Reading latest UDD data", q.name);

    void *udd_addr = q.get_last_udd_addr();

    return q.read_st_udd_data(udd_addr, buf, len);
}

NTSTATUS qdma_device::qdma_queue_desc_dump(qdma_desc_info *desc_info)
{
    if (desc_info->qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, desc_info->qid);
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

    TraceVerbose(TRACE_QDMA, "%s descripor data dump", q.name);

    NTSTATUS status = q.desc_dump(desc_info);

    InterlockedExchange(&q.state, old_state);

    return status;
}

NTSTATUS qdma_device::qdma_read_mm_cmpt_data(qdma_cmpt_info *cmpt_info)
{
    if (cmpt_info->qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, cmpt_info->qid);
        return STATUS_INVALID_PARAMETER;
    }

    if ((cmpt_info == nullptr) || (cmpt_info->pbuffer == nullptr) || (cmpt_info->buffer_len == 0))
        return STATUS_INVALID_PARAMETER;

    auto& q = queue_pairs[cmpt_info->qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (q.type != queue_type::MEMORY_MAPPED) {
        TraceError(TRACE_QDMA, "Queue %d is not configured in MM mode!", cmpt_info->qid);
        return STATUS_ACCESS_VIOLATION;
    }

    TraceVerbose(TRACE_QDMA, "%s Read MM completion data", q.name);

    return q.read_mm_cmpt_data(cmpt_info);
}

NTSTATUS qdma_device::qdma_queue_context_read(UINT16 qid, enum qdma_dev_q_type ctx_type, struct qdma_descq_context *ctxt)
{
    if ((qid >= drv_conf.qsets_max) ||
        (ctx_type >= qdma_dev_q_type::QDMA_DEV_Q_TYPE_MAX) ||
        (ctxt == nullptr)) {
        TraceError(TRACE_QDMA, "%s: Incorrect Parameters, qid : %d, ctx_type : %d, ctxt : %p",
            dev_conf.name, qid, ctx_type, ctxt);
        return STATUS_INVALID_PARAMETER;
    }

    auto& q = queue_pairs[qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
        return STATUS_INVALID_DEVICE_STATE;
    }

    uint8_t is_c2h = (ctx_type == qdma_dev_q_type::QDMA_DEV_Q_TYPE_C2H) ? true : false;
    uint16_t hw_qid = q.idx_abs;
    int ret;
    bool dump = false;

    if (ctx_type != qdma_dev_q_type::QDMA_DEV_Q_TYPE_CMPT) {
        ret = hw.qdma_sw_ctx_conf(this, is_c2h, hw_qid, &ctxt->sw_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to read SW context, ret : %d", q.name, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_hw_ctx_conf(this, is_c2h, hw_qid, &ctxt->hw_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to read HW context, ret : %d", q.name, ret);
            return hw.qdma_get_error_code(ret);
        }

        ret = hw.qdma_credit_ctx_conf(this, is_c2h, hw_qid, &ctxt->cr_ctxt, QDMA_HW_ACCESS_READ);
        if (ret < 0) {
            TraceError(TRACE_QDMA, "%s Failed to read credit context, ret : %d", q.name, ret);
            return hw.qdma_get_error_code(ret);
        }

        if ((q.type == queue_type::STREAMING) && (is_c2h)) {
            ret = hw.qdma_pfetch_ctx_conf(this, hw_qid, &ctxt->pfetch_ctxt, QDMA_HW_ACCESS_READ);
            if (ret < 0) {
                TraceError(TRACE_QDMA, "%s Failed to read pre-fetch context, ret : %d", q.name, ret);
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
            TraceError(TRACE_QDMA, "%s Failed to read completion "
                "context, ret : %d", q.name, ret);
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

    if (drv_conf.operation_mode != queue_op_mode::INTR_COAL_MODE)
        return STATUS_ACCESS_VIOLATION;

    ret = hw.qdma_indirect_intr_ctx_conf(this, ring_idx_abs, ctxt, QDMA_HW_ACCESS_READ);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s : Failed to read interrupt context for ring index %d!, ret : %d",
            dev_conf.name, ring_idx_abs, ret);
        return hw.qdma_get_error_code(ret);
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_queue_dump_context(qdma_ctx_info *ctx_info)
{
    if (ctx_info->qid >= drv_conf.qsets_max) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, ctx_info->qid);
        return STATUS_INVALID_PARAMETER;
    }

    if ((ctx_info == nullptr) ||
        (ctx_info->ring_type >= qdma_q_type::QDMA_Q_TYPE_MAX) ||
        (ctx_info->pbuffer == nullptr) || (ctx_info->buffer_sz == 0))
        return STATUS_INVALID_PARAMETER;

    queue_pair &q = queue_pairs[ctx_info->qid];

    if (q.state != queue_state::QUEUE_STARTED) {
        TraceError(TRACE_QDMA, "%s invalid state, q_state %d", q.name, q.state);
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
        TraceError(TRACE_QDMA, "%s hw.qdma_dump_queue_context failed : %d", q.name, ctx_len);
        return hw.qdma_get_error_code(ctx_len);
    }

    len = len + ctx_len;

    if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) {
        if (ctx_info->buffer_sz < len) {
            TraceError(TRACE_QDMA, "%s Insufficient Buffer Length, "
                "buffer has %lld bytes, needs %lld bytes", q.name, ctx_info->buffer_sz, len);
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
            TraceError(TRACE_QDMA, "%s : hw.qdma_dump_intr_context returned error : %d", dev_conf.name, intr_len);
            return hw.qdma_get_error_code(intr_len);
        }

        len = len + intr_len;
    }

    ctx_info->ret_sz = len;

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::qdma_intring_dump(qdma_intr_ring_info *intring_info)
{
    if (drv_conf.operation_mode != queue_op_mode::INTR_COAL_MODE) {
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

    if (drv_conf.operation_mode != queue_op_mode::INTR_COAL_MODE) {
        TraceError(TRACE_QDMA, "%s is not configured in Interrupt Aggregation mode", dev_conf.name);
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
        TraceError(TRACE_QDMA, " %s : hw.qdma_dump_config_regs failed with err : %d", dev_conf.name, len);
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

NTSTATUS qdma_device::qdma_get_reg_info(qdma_reg_info* reg_info)
{
    int len = 0;

    if (reg_info == nullptr)
        return STATUS_INVALID_PARAMETER;

    if (!hw.qdma_dump_reg_info)
        return STATUS_NOT_SUPPORTED;

    len = hw.qdma_dump_reg_info(this,
        reg_info->address, reg_info->reg_cnt, 
        reg_info->pbuffer, (int)reg_info->buf_len);

    if (len < 0) {
        TraceError(TRACE_QDMA, "hw.qdma_dump_reg_info Failed with error : %d", len);
        return STATUS_UNSUCCESSFUL;
    }

    if (len == 0) {
        TraceError(TRACE_QDMA, "Register %d is not present in the design", reg_info->address);
        return STATUS_INVALID_PARAMETER;
    }

    reg_info->ret_len = len;

    return STATUS_SUCCESS;
}

NTSTATUS qdma_device::open(const WDFDEVICE device, WDFCMRESLIST resources,
                           const WDFCMRESLIST resources_translated)
{
    NTSTATUS status = STATUS_SUCCESS;
    int ret = 0;

    wdf_dev = device;

    status = pcie.get_bdf(wdf_dev, dev_conf.dev_sbdf.val);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "pcie.get_bdf failed! %!STATUS!", status);
        return status;
    }

    RtlStringCchPrintfA(dev_conf.name,
        ARRAYSIZE(dev_conf.name), "qdma%02X%02X%x",
        dev_conf.dev_sbdf.sbdf.bus_no,
        dev_conf.dev_sbdf.sbdf.dev_no,
        dev_conf.dev_sbdf.sbdf.fun_no);

    TraceInfo(TRACE_QDMA, "%04X:%02X:%02X.%X: func : %X, p/v 1/0", 
        dev_conf.dev_sbdf.sbdf.seg_no, dev_conf.dev_sbdf.sbdf.bus_no, 
        dev_conf.dev_sbdf.sbdf.dev_no, dev_conf.dev_sbdf.sbdf.fun_no, 
        dev_conf.dev_sbdf.sbdf.fun_no);

    status = pcie.map(resources_translated);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s : pcie.map failed! %!STATUS!", dev_conf.name, status);
        return status;
    }

    status = pcie.assign_config_bar(drv_conf.cfg_bar);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: Failed to assign config BAR", dev_conf.name);
        return status;
    }

    ret = qdma_hw_access_init((void*)this, 0, &hw);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: Failed to initialize hw access", dev_conf.name);
        return STATUS_UNSUCCESSFUL;
    }

    /** Change device state to offline after acquiring the HW callbacks */
    InterlockedExchange(&qdma_device_state, device_state::DEVICE_OFFLINE);

    ret = hw.qdma_get_version((void *)this, QDMA_DEV_PF, &hw_version_info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_get_version failed!, ret : %d", dev_conf.name, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    status = assign_bar_types();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s : assign_bar_types failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    ret = hw.qdma_get_device_attributes((void *)this, &dev_conf.dev_info);
    if (ret < 0) {
        TraceError(TRACE_QDMA, "%s: qdma_get_device_attributes failed!, ret : %d", dev_conf.name, ret);
        status = hw.qdma_get_error_code(ret);
        goto ErrExit;
    }

    TraceInfo(TRACE_QDMA, "qdma_get_device_attributes: %s: num_pfs:%d, num_qs:%d, flr_present:%d, st_en:%d, "
        "mm_en:%d, mm_cmpt_en:%d, mailbox_en:%d, mm_channel_max:%d, qid2vec_ctx:%d, ",
        dev_conf.name,
        dev_conf.dev_info.num_pfs,
        dev_conf.dev_info.num_qs,
        dev_conf.dev_info.flr_present,
        dev_conf.dev_info.st_en,
        dev_conf.dev_info.mm_en,
        dev_conf.dev_info.mm_cmpt_en,
        dev_conf.dev_info.mailbox_en,
        dev_conf.dev_info.mm_channel_max,
        dev_conf.dev_info.qid2vec_ctx);

    TraceInfo(TRACE_QDMA, "cmpt_ovf_chk_dis:%d, mailbox_intr:%d, sw_desc_64b:%d, cmpt_desc_64b:%d, "
        "dynamic_bar:%d, legacy_intr:%d, cmpt_trig_count_timer:%d",
        dev_conf.dev_info.cmpt_ovf_chk_dis,
        dev_conf.dev_info.mailbox_intr,
        dev_conf.dev_info.sw_desc_64b,
        dev_conf.dev_info.cmpt_desc_64b,
        dev_conf.dev_info.dynamic_bar,
        dev_conf.dev_info.legacy_intr,
        dev_conf.dev_info.cmpt_trig_count_timer);

    if ((dev_conf.dev_info.st_en == 0) &&
        (dev_conf.dev_info.mm_en == 0)) {
        TraceError(TRACE_QDMA, "%s: None of the modes ( ST or MM) are enabled", dev_conf.name);
        status = STATUS_INVALID_HW_PROFILE;
        goto ErrExit;
    }

    status = list_add_qdma_device_and_set_gbl_csr();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: list_add_qdma_device_and_set_gbl_csr failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    TraceInfo(TRACE_QDMA, "Driver is loaded in %s(%d) mode",
        mode_name_list[drv_conf.operation_mode].name, drv_conf.operation_mode);

    status = init_resource_manager();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_resource_manager failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    status = init_os_resources(resources, resources_translated);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_os_resources() failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    status = init_qdma_global();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_qdma_global() failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    status = init_func();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_func() failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    status = init_dma_queues();
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_QDMA, "%s: init_dma_queues() failed! %!STATUS!", dev_conf.name, status);
        goto ErrExit;
    }

    if (drv_conf.operation_mode == queue_op_mode::INTR_COAL_MODE) {
        status = init_interrupt_queues();
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "%s: init_interrupt_queues() failed! %!STATUS!", dev_conf.name, status);
            goto ErrExit;
        }
    }

    InterlockedExchange(&qdma_device_state, device_state::DEVICE_ONLINE);

    TraceInfo(TRACE_QDMA, "%s, %05x, qdma_device %p, ch %u, q %u.",
        dev_conf.name, dev_conf.dev_sbdf.val, this,
        dev_conf.dev_info.mm_channel_max, drv_conf.qsets_max);

    return status;

ErrExit:
    close();
    return status;
}

void qdma_device::close()
{
    /** Make sure to not perform multiple cleanups on same device */
    if (qdma_device_state != device_state::DEVICE_INIT) {

        TraceInfo(TRACE_QDMA, "%04X:%02X:%02X.%X qdma_device 0x%p, %s.\n",
            dev_conf.dev_sbdf.sbdf.seg_no, dev_conf.dev_sbdf.sbdf.bus_no, 
            dev_conf.dev_sbdf.sbdf.dev_no, dev_conf.dev_sbdf.sbdf.fun_no, 
            this, dev_conf.name);

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
        InterlockedExchange(&qdma_device_state, device_state::DEVICE_INIT);
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
        TraceError(TRACE_QDMA, "%s: Failed to read CSR global ring size, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_timer_cnt, QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "%s: Failed to read CSR global timer count, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_th_cnt, QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "%s: Failed to read CSR global counter threshold, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_csr_conf((void *)this, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
            conf->c2h_buff_sz, QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "%s: Failed to read CSR global buffer size, ret : %d", dev_conf.name, ret);
        return hw.qdma_get_error_code(ret);
    }

    ret = hw.qdma_global_writeback_interval_conf((void *)this,
            (qdma_wrb_interval *)&conf->wb_interval, QDMA_HW_ACCESS_READ);
    if ((ret < 0) && (ret != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)) {
        TraceError(TRACE_QDMA, "%s: Failed to read CSR global write back interval, ret : %d", dev_conf.name, ret);
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
        TraceError(TRACE_QDMA, "%s: qdma_get_device_attributes Failed, ret : %d", dev_conf.name, ret);
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
    dev_attr.debug_mode         = dev_info.debug_mode;
    dev_attr.desc_eng_mode      = dev_info.desc_eng_mode;

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
        TraceError(TRACE_QDMA, "%s: qdma_get_version failed!, ret : %d", dev_conf.name, ret);
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
    if (qid_abs >= (qbase + drv_conf.qsets_max)) {
        TraceError(TRACE_QDMA, "%s: Invalid Qid %d provided", dev_conf.name, qid_abs);
        return nullptr;
    }

    UINT16 qid_rel = qid_abs - static_cast<UINT16>(qbase);
    TraceVerbose(TRACE_QDMA, "%s: Absolute qid : %u Relative qid : %u", dev_conf.name, qid_abs, qid_rel);
    return &queue_pairs[qid_rel];
}