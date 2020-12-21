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

#include "qdma_driver_api.h"
#include "device.h"
#include "io_queue.h"

#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "io_queue.tmh"
#endif

// remove some false positive static analysis errors related to trace messages
//#ifndef WPP_TRACING
//#define WPP_(x) (void)
//#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, qdma_io_queue_initialize)
#endif

using namespace xlnx;

void drv_st_tx_zcmp_cb(void *priv, NTSTATUS status);
void drv_st_tx_cmp_cb(void *priv, NTSTATUS status);
void drv_mm_cmp_cb(void *priv, NTSTATUS status);
void drv_st_rx_cmp_cb(const st_c2h_pkt_fragment *rx_pkts, size_t num_pkts, void *priv, NTSTATUS status);
void drv_st_process_udd_only_pkts(UINT16 qid, void *udd_addr, void *priv);

/* ----------------- static function definitions ----------------- */
const static struct device_file_details {
    file_target target;
    const wchar_t* name;
} file_name_LUT[] = {
    { file_target::USER,        QDMA_FILE_USER },
    { file_target::CONTROL,     QDMA_FILE_CONTROL },
    { file_target::BYPASS,      QDMA_FILE_BYPASS },
    { file_target::DMA_QUEUE,   QDMA_FILE_DMA_QUEUE },
    { file_target::ST_QUEUE,    QDMA_FILE_DMA_ST_QUEUE },
    { file_target::MGMT,        QDMA_FILE_DMA_MGMT},
};

/* convert from filename to device node */
static file_target parse_file_name(
    const PUNICODE_STRING file_name)
{
    for (UINT i = 0; i < sizeof(file_name_LUT) / sizeof(file_name_LUT[0]); ++i) {
        if (!wcscmp(file_name->Buffer, file_name_LUT[i].name)) {
            return file_name_LUT[i].target;
        }
        if (wcsstr(file_name->Buffer, file_name_LUT[i].name)
            && file_name->Length > wcslen(file_name_LUT[i].name)) {
            return file_name_LUT[i].target;
        }
    }
    TraceError(TRACE_DEVICE, "device file name does not match one of known types!");
    return file_target::UNKNOWN;
}

static UINT16 extract_index_token(
    const PUNICODE_STRING file_name)
{
    ULONG index = 0;
    UNICODE_STRING token_uni;
    RtlInitUnicodeString(&token_uni, wcsstr(file_name->Buffer, L"_") + 1);
    auto status = RtlUnicodeStringToInteger(&token_uni, 10, &index);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "RtlUnicodeStringToInteger failed! %!STATUS!", status);
        return UINT16_MAX;
    }
    return (UINT16)index;
}

static void io_read_bar(
    qdma_interface *qdma_dev,
    qdma_bar_type bar_type,
    const WDFREQUEST request,
    const size_t length)
{
    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(request, &params);
    const auto offset = static_cast<size_t>(params.Parameters.Read.DeviceOffset);

    /* get handle to the IO request memory which will hold the read data */
    WDFMEMORY request_mem;
    auto status = WdfRequestRetrieveOutputMemory(request, &request_mem);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfRequestRetrieveOutputMemory failed: %!STATUS!", status);
        goto ErrExit;
    }

    /* get pointer to buffer */
    const auto req_buffer = WdfMemoryGetBuffer(request_mem, nullptr);

    status = qdma_dev->read_bar(bar_type, offset, req_buffer, length);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "Reading PCIe BAR failed: %!STATUS!", status);
        goto ErrExit;
    }

    WdfRequestCompleteWithInformation(request, status, length);
    return;

ErrExit:
    WdfRequestComplete(request, status);
}

static void io_write_bar(
    qdma_interface *qdma_dev,
    qdma_bar_type bar_type,
    const WDFREQUEST request,
    const size_t length)
{
    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(request, &params);
    const auto offset = static_cast<size_t>(params.Parameters.Write.DeviceOffset);

    /* get handle to the IO request memory which will hold the read data */
    WDFMEMORY request_mem;

    auto status = WdfRequestRetrieveInputMemory(request, &request_mem);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfRequestRetrieveOutputMemory failed: %!STATUS!", status);
        goto ErrExit;
    }

    /* get pointer to buffer */
    const auto req_buffer = WdfMemoryGetBuffer(request_mem, nullptr);

    status = qdma_dev->write_bar(bar_type, offset, req_buffer, length);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "Writing PCIe BAR failed: %!STATUS!", status);
        goto ErrExit;
    }

    WdfRequestCompleteWithInformation(request, status, length);
    return;

ErrExit:
    WdfRequestComplete(request, status);
}

static BOOLEAN program_mm_dma_cb(
    const WDFDMATRANSACTION transaction,
    WDFDEVICE device,
    WDFCONTEXT context,
    const WDF_DMA_DIRECTION direction,
    const PSCATTER_GATHER_LIST sg_list)
{
    auto dev_ctx = get_device_context(device);
    DMA_TXN_CONTEXT *dma_ctx = (DMA_TXN_CONTEXT *)context;
    const auto request = WdfDmaTransactionGetRequest(transaction);
    WDF_REQUEST_PARAMETERS params;
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(request, &params);

    const auto device_offset = (direction == WdfDmaDirectionWriteToDevice) ?
        params.Parameters.Write.DeviceOffset :
        params.Parameters.Read.DeviceOffset;

    auto status = dev_ctx->qdma->qdma_enqueue_mm_request(dma_ctx->qid, direction, sg_list, device_offset, drv_mm_cmp_cb, transaction);
    if (!NT_SUCCESS(status)) {
        dma_ctx->txn_len = 0;
        /** Complete the DMA transaction */
        drv_mm_cmp_cb(transaction, status);
        TraceError(TRACE_IO, "qdma_enqueue_mm_request() failed! %!STATUS!", status);;
        return false;
    }

    TraceVerbose(TRACE_IO, "qdma_enqueue_mm_request(): txd len : %lld", dma_ctx->txn_len);

    return true;
}

static BOOLEAN program_st_tx_dma_cb(
    WDFDMATRANSACTION transaction,
    WDFDEVICE device,
    const WDFCONTEXT context,
    const WDF_DMA_DIRECTION direction,
    const PSCATTER_GATHER_LIST sg_list)
{
    UNREFERENCED_PARAMETER(direction);

    auto dev_ctx = get_device_context(device);
    DMA_TXN_CONTEXT *dma_ctx = (DMA_TXN_CONTEXT *)context;

    auto status = dev_ctx->qdma->qdma_enqueue_st_tx_request(dma_ctx->qid, sg_list, drv_st_tx_cmp_cb, transaction);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "qdma_enqueue_st_tx_request() failed! %!STATUS!", status);
        drv_st_tx_cmp_cb(transaction, status);
        return false;
    }

    TraceVerbose(TRACE_IO, "qdma_enqueue_st_tx_request(): txd len : %lld", dma_ctx->txn_len);

    return true;
}


static void io_mm_dma(
    qdma_interface *qdma_dev,
    UINT16 qid,
    WDFREQUEST request,
    const size_t length,
    const WDF_DMA_DIRECTION direction)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDMATRANSACTION dma_transaction;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DMA_TXN_CONTEXT);
    auto status = WdfDmaTransactionCreate(qdma_dev->dma_enabler, &attributes, &dma_transaction);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionCreate() failed: %!STATUS!", status);
        return;
    }

    auto dma_ctx = get_dma_txn_context(dma_transaction);
    dma_ctx->qid = qid;
    dma_ctx->txn_len = length;

    /* initialize a DMA transaction from the request */
    status = WdfDmaTransactionInitializeUsingRequest(dma_transaction,
                                                     request,
                                                     program_mm_dma_cb,
                                                     direction);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionInitializeUsingRequest failed: %!STATUS!", status);
        goto ErrExit;
    }

    status = WdfDmaTransactionExecute(dma_transaction, dma_ctx);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionExecute failed: %!STATUS!", status);
        goto ErrExit;
    }

    TraceVerbose(TRACE_IO, "DMA transfer triggered on queue %d, direction : %d", qid, direction);
    return;

ErrExit:
    WdfObjectDelete(dma_transaction);
    WdfRequestComplete(request, status);
    TraceError(TRACE_IO, "DMA transfer initiation failed, Request 0x%p: %!STATUS!", request, status);

}

static void io_st_read_dma(
    qdma_interface *qdma_dev,
    UINT16 qid,
    WDFREQUEST request,
    const size_t length)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    DMA_TXN_CONTEXT *dma_context = nullptr;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DMA_TXN_CONTEXT);

    NTSTATUS status = WdfObjectAllocateContext(request, &attributes, (PVOID *)&dma_context);
    dma_context->qid = qid;
    dma_context->txn_len = length;

    status = qdma_dev->qdma_enqueue_st_rx_request(qid, length, drv_st_rx_cmp_cb, (void *)request);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
        TraceError(TRACE_IO, "DMA transfer initiation failed: %!STATUS!", status);
    }

    TraceVerbose(TRACE_IO, "DMA transfer triggered on queue %d", qid);
}

static void io_st_zero_write_dma(
    qdma_interface *qdma_dev,
    UINT16 qid,
    WDFREQUEST request,
    const size_t length,
    const WDF_DMA_DIRECTION direction)
{
    UNREFERENCED_PARAMETER(length);
    UNREFERENCED_PARAMETER(direction);

    ST_DMA_ZERO_TX_PRIV *priv;
    /** construct one element sg_list */
    constexpr size_t sg_list_len = sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT);
    PSCATTER_GATHER_LIST sg_list;

    sg_list = (PSCATTER_GATHER_LIST)ExAllocatePoolWithTag(NonPagedPoolNx, sg_list_len, IO_QUEUE_TAG);
    if (sg_list == NULL) {
        TraceVerbose(TRACE_IO, "sg_list: Mem alloc failed\n");
        return;
    }

    RtlZeroMemory(sg_list, sg_list_len);

    sg_list->NumberOfElements = 1;
    sg_list->Elements[0].Address.QuadPart = NULL;
    sg_list->Elements[0].Length = 0x0;

    priv = (ST_DMA_ZERO_TX_PRIV*)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ST_DMA_ZERO_TX_PRIV), IO_QUEUE_TAG);
    if (priv == NULL) {
        ExFreePoolWithTag(sg_list, IO_QUEUE_TAG);
        TraceVerbose(TRACE_IO, "priv: Mem alloc failed\n");
        return;
    }

    /** Store the context info in priv data */
    priv->request = request;
    priv->sg_list = sg_list;

    /* For Zero byte transfer, pass the ST_DMA_ZERO_TX_PRIV in WDFDMATRANSACTION parameter that contains,
       locally constructed single element sglist parameter & WDFREQUEST for the function qdma_enqueue_st_request */
    auto status = qdma_dev->qdma_enqueue_st_tx_request(qid, sg_list, drv_st_tx_zcmp_cb, static_cast<PVOID>(priv));
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "DMA transfer initiation failed! %!STATUS!", status);
        drv_st_tx_zcmp_cb(priv, status);
        return;
    }

    TraceVerbose(TRACE_IO, "DMA transfer triggered on queue %d for zero length", qid);
    return;
}

static void io_st_write_dma(
    qdma_interface *qdma_dev,
    UINT16 qid,
    WDFREQUEST request,
    const size_t length,
    const WDF_DMA_DIRECTION direction)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDMATRANSACTION dma_transaction;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DMA_TXN_CONTEXT);
    auto status = WdfDmaTransactionCreate(qdma_dev->dma_enabler, &attributes, &dma_transaction);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionCreate() failed: %!STATUS!", status);
        WdfRequestComplete(request, status);
        return;
    }

    auto dma_ctx = get_dma_txn_context(dma_transaction);
    if (!dma_ctx) {
        goto ErrExit;
    }

    dma_ctx->qid = qid;
    dma_ctx->txn_len = length;

    /* initialize a DMA transaction from the request */
    status = WdfDmaTransactionInitializeUsingRequest(dma_transaction,
                                                     request,
                                                     program_st_tx_dma_cb,
                                                     direction);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionInitializeUsingRequest failed: %!STATUS!", status);
        goto ErrExit;
    }

    status = WdfDmaTransactionExecute(dma_transaction, dma_ctx);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfDmaTransactionExecute failed: %!STATUS!", status);
        goto ErrExit;
    }

    TraceVerbose(TRACE_IO, "DMA transfer triggered on queue %d", qid);
    return;

ErrExit:
    WdfObjectDelete(dma_transaction);
    WdfRequestComplete(request, status);
    TraceError(TRACE_IO, "DMA transfer initiation failed! for Request : %p,  %!STATUS!", request, status);
}

/* ----- CB Processing Functions ----- */
void drv_st_rx_cmp_cb(const st_c2h_pkt_fragment *rx_pkts, size_t num_pkts, void *priv, NTSTATUS status)
{
    WDFREQUEST request = (WDFREQUEST)priv;
    size_t offset = 0;
    auto dma_ctx = get_dma_txn_context(request);
    auto file_ctx = get_file_context(WdfRequestGetFileObject(request));

    if (nullptr == file_ctx) {
        status = STATUS_UNSUCCESSFUL;
        goto ErrExit;
    }

    if ((NT_SUCCESS(status))) {
        if (dma_ctx->txn_len != (size_t)0) {
            WDFMEMORY output_mem;
            status = WdfRequestRetrieveOutputMemory(request, &output_mem);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "WdfRequestRetrieveOutputMemory failed: %!STATUS!", status);
                WdfRequestCompleteWithInformation(request, status, (size_t)0);
                return;
            }

            for (size_t i = 0; i < num_pkts; i++) {
                st_c2h_pkt_fragment packet = rx_pkts[i];

#ifdef DBG
                if (packet.udd_data != nullptr) {
                   UINT8 len = 0;
                    constexpr unsigned short MAX_UDD_STR_LEN = (QDMA_MAX_UDD_DATA_LEN * 3) + 1;
                    char imm_data_str[MAX_UDD_STR_LEN];
                    UINT32 udd_len;
                    UINT8 udd_buffer[QDMA_MAX_UDD_DATA_LEN];
                    WDFQUEUE io_queue = WdfRequestGetIoQueue(request);
                    if (io_queue) {
                        auto dev_ctx = get_device_context(WdfIoQueueGetDevice(io_queue));
                        status = dev_ctx->qdma->qdma_retrieve_st_udd_data(dma_ctx->qid, packet.udd_data, udd_buffer, &udd_len);

                        for (auto iter = 0UL; iter < udd_len; iter++) {
                            RtlStringCchPrintfA((imm_data_str + len), (MAX_UDD_STR_LEN - len), "%02X ", udd_buffer[iter]);
                            len = len + 3; /* 3 characters are getting utilized for each byte */
                        }
                        TraceVerbose(TRACE_IO, "Immediate data Len : %d, Data: %s", udd_len, imm_data_str);
                    }
                }
#endif
                if ((packet.data) && (packet.length) &&
                    (packet.pkt_type != st_c2h_pkt_type::ST_C2H_UDD_ONLY_PKT)) {

                    if ((offset + packet.length) >= dma_ctx->txn_len) {
                        if (false == file_ctx->no_copy)
                            WdfMemoryCopyFromBuffer(output_mem, offset, packet.data, (dma_ctx->txn_len - offset));
                        offset += (dma_ctx->txn_len - offset);
                        break;
                    }
                    else {
                        if (false == file_ctx->no_copy)
                            WdfMemoryCopyFromBuffer(output_mem, offset, packet.data, packet.length);
                        offset += packet.length;
                    }
                }
            }
        }

        WdfRequestCompleteWithInformation(request, STATUS_SUCCESS, offset);
        return;
    }

ErrExit:
    WdfRequestCompleteWithInformation(request, status, (size_t)0);

    TraceVerbose(TRACE_IO, "ST C2H Request completed with %!STATUS!", status);
}

static void dma_complete_transaction(WDFDMATRANSACTION dma_transaction, NTSTATUS status)
{
    NTSTATUS ret;
    WDFREQUEST request = NULL;
    BOOLEAN transaction_complete = false;
    auto dma_ctx = get_dma_txn_context(dma_transaction);
    size_t length = dma_ctx->txn_len;
    UINT16 qid = dma_ctx->qid;

    request = WdfDmaTransactionGetRequest(dma_transaction);
    if (!request)
        /** Dont return from here, Need to delete the dma_transaction object */
        TraceError(TRACE_IO, "Callback triggered, No request pending on queue %d", qid);

    if ((NT_SUCCESS(status)))
        transaction_complete = WdfDmaTransactionDmaCompleted(dma_transaction, &ret);
    else
        transaction_complete = WdfDmaTransactionDmaCompletedFinal(dma_transaction, length, &ret);

    if (transaction_complete) {
        NT_ASSERT(status != STATUS_MORE_PROCESSING_REQUIRED);
        WdfObjectDelete(dma_transaction);
    }
    else {
        TraceError(TRACE_IO, "Err: DMA transaction not completed on queue %d, ret : %X", qid, ret);
    }

    if (request) {
        WdfRequestCompleteWithInformation(request, status, length);
        TraceVerbose(TRACE_IO, "DMA transfer completed on queue %d, Len : %lld", qid, length);
    }
}

void drv_mm_cmp_cb(void *priv, NTSTATUS status)
{
    if (priv == nullptr) {
        TraceError(TRACE_IO, "WDFDMATRANSACTION is NULL, Not possible to proceed");
        return;
    }

    dma_complete_transaction(static_cast<WDFDMATRANSACTION>(priv), status);
}

void drv_st_tx_cmp_cb(void *priv, NTSTATUS status)
{
    if (priv == nullptr) {
        TraceError(TRACE_IO, "WDFDMATRANSACTION is NULL, Not possible to proceed");
        return;
    }

    dma_complete_transaction(static_cast<WDFDMATRANSACTION>(priv), status);
}

void drv_st_tx_zcmp_cb(void *priv, NTSTATUS status)
{
    if (priv == nullptr) {
        TraceError(TRACE_IO, "WDFREQUEST is NULL, Not possible to proceed");
        return;
    }

    ST_DMA_ZERO_TX_PRIV *priv_ctx = (ST_DMA_ZERO_TX_PRIV *)priv;

    WdfRequestCompleteWithInformation(static_cast<WDFREQUEST>(priv_ctx->request), status, 0);

    ExFreePoolWithTag(priv_ctx->sg_list, IO_QUEUE_TAG);
    ExFreePoolWithTag(priv_ctx, IO_QUEUE_TAG);
    
    TraceInfo(TRACE_IO, "DMA Transfer completed for Zero length");
}

void drv_st_process_udd_only_pkts(UINT16 qid, void *udd_addr, void *priv)
{
    TraceVerbose(TRACE_IO, "UDD Only Call back function called, qid : %d, "
        "udd_addr : %p, priv : %p\n", qid, udd_addr, priv);

#ifdef DBG
    if ((udd_addr == nullptr) || (priv == nullptr))
        return;

    UINT8 len = 0;
    constexpr unsigned short MAX_UDD_STR_LEN = (QDMA_MAX_UDD_DATA_LEN * 3) + 1;
    char imm_data_str[MAX_UDD_STR_LEN];
    UINT32 udd_len;
    UINT8 udd_buffer[QDMA_MAX_UDD_DATA_LEN];
    qdma_interface *qdma_dev = (qdma_interface *)priv;

    NTSTATUS status = qdma_dev->qdma_retrieve_st_udd_data(qid, udd_addr, udd_buffer, &udd_len);

    if (NT_SUCCESS(status)) {
        for (auto iter = 0UL; iter < udd_len; iter++) {
            RtlStringCchPrintfA((imm_data_str + len), (MAX_UDD_STR_LEN - len), "%02X ", udd_buffer[iter]);
            len = len + 3; /* 3 characters are getting utilized for each byte */
        }

        TraceInfo(TRACE_IO, "Immediate data Len : %d, Data: %s", udd_len, imm_data_str);
    }
#endif
}

NTSTATUS qdma_io_queue_initialize(
    const WDFDEVICE device)
{
    PAGED_CODE();

    TraceVerbose(TRACE_IO, "Initializing main entry IO queue");

    /* Configure a default queue so that requests that are not configure-fowarded using
     * WdfDeviceConfigureRequestDispatching to goto other queues get dispatched here.
     */
    WDF_IO_QUEUE_CONFIG queue_config;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchParallel);
    queue_config.AllowZeroLengthRequests = true;
    queue_config.EvtIoDeviceControl = qdma_evt_ioctl;
    queue_config.EvtIoStop = qdma_evt_io_stop;
    queue_config.EvtIoRead = qdma_evt_io_read;
    queue_config.EvtIoWrite = qdma_evt_io_write;

    WDFQUEUE queue;
    auto status = WdfIoQueueCreate(device, &queue_config, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_IO, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

static NTSTATUS retrive_ioctl(
    WDFREQUEST request,
    PVOID ibuf,
    size_t ibuf_len,
    PVOID *obuf = nullptr,
    size_t obuf_len = 0
)
{
    PVOID   in_buff = nullptr;
    PVOID   out_buff = nullptr;
    size_t  buff_len;
    NTSTATUS status;

    if (ibuf != nullptr) {
        status = WdfRequestRetrieveInputBuffer(request, ibuf_len, &in_buff, &buff_len);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_IO, "WdfRequestRetrieveInputBuffer failed: %!STATUS!", status);
            return status;
        }

        if (ibuf_len != buff_len) {
            TraceError(TRACE_IO, "input buffer length mismatch: %lld != %lld", ibuf_len, buff_len);
            return STATUS_UNSUCCESSFUL;
        }

        RtlCopyMemory(ibuf, in_buff, buff_len);
    }

    if (obuf != nullptr) {
        status = WdfRequestRetrieveOutputBuffer(request, obuf_len, &out_buff, &buff_len);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_IO, "WdfRequestRetrieveOutputBuffer failed: %!STATUS!", status);
            return STATUS_UNSUCCESSFUL;
        }

        if (obuf_len != buff_len) {
            TraceError(TRACE_IO, "output buffer length mismatch: %lld != %lld", obuf_len, buff_len);
            return STATUS_UNSUCCESSFUL;
        }

        *obuf = out_buff;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS validate_ioctl_cmd(
    ULONG io_control_code,
    ioctl_cmd& cmd
)
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (io_control_code) {
    case IOCTL_QDMA_QUEUE_ADD:
        if (cmd.q_conf.in.h2c_ring_sz_index >= QDMA_CSR_SZ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.c2h_ring_sz_index >= QDMA_CSR_SZ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.c2h_buff_sz_index >= QDMA_CSR_SZ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.c2h_th_cnt_index >= QDMA_CSR_SZ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.c2h_timer_cnt_index >= QDMA_CSR_SZ) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.compl_sz >= CMPT_DESC_SZ_MAX) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.q_conf.in.trig_mode >= TRIG_MODE_MAX) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        break;
    case IOCTL_QDMA_QUEUE_DUMP_DESC:
        if (cmd.desc_info.in.dir > queue_direction::QUEUE_DIR_C2H) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (cmd.desc_info.in.desc_type > descriptor_type::CMPT_DESC) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        break;
    case IOCTL_QDMA_QUEUE_DUMP_CTX:
        if (cmd.ctx_info.in.type > ring_type::RING_TYPE_CMPT) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        break;
    default:
        /* default validation is success */
        break;
    }

Exit:
    return status;
}

void qdma_evt_ioctl(
    WDFQUEUE queue,
    WDFREQUEST request,
    size_t output_buffer_length,
    size_t input_buffer_length,
    ULONG io_control_code)
{
    NTSTATUS status;
    auto dev_ctx = get_device_context(WdfIoQueueGetDevice(queue));
    auto file_ctx = get_file_context(WdfRequestGetFileObject(request));
    auto qdma_dev = dev_ctx->qdma;
    union ioctl_cmd cmd;
    enum queue_state qstate;

    TraceVerbose(TRACE_IO, "Queue 0x%p, Request 0x%p OutputBufferLength %llu InputBufferLength %llu IoControlCode 0x%X",
        queue, request, output_buffer_length, input_buffer_length, io_control_code);

    if (file_target::MGMT != file_ctx->target && file_target::ST_QUEUE != file_ctx->target) {
        TraceError(TRACE_IO, "File target not supported");
        WdfRequestComplete(request, STATUS_UNSUCCESSFUL);
        return;
    }

    if (false == qdma_dev->qdma_is_device_online()) {
        TraceError(TRACE_DEVICE, "QDMA Device is offline.");
        WdfRequestComplete(request, STATUS_DEVICE_OFF_LINE);
        return;
    }

    RtlZeroMemory(&cmd, sizeof(cmd));
    switch (io_control_code) {
        case IOCTL_QDMA_CSR_DUMP :
        {
            qdma_glbl_csr_conf conf = {};

            status = retrive_ioctl(request, nullptr, 0,
                (PVOID *)&cmd.csr.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_CSR_DUMP");
            status = qdma_dev->qdma_read_csr_conf(&conf);
            if (!NT_SUCCESS(status))
                goto Exit;

            RtlCopyMemory(cmd.csr.out, &conf, sizeof(conf));

            WdfRequestCompleteWithInformation(request, status, sizeof(qdma_glbl_csr_conf));
            break;
        }
        case IOCTL_QDMA_DEVINFO :
        {
            qdma_version_info version_info = {};

            status = retrive_ioctl(request, nullptr, 0,
                (PVOID *)&cmd.dev_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_DEVINFO");
            status = qdma_dev->qdma_device_version_info(version_info);
            if (!NT_SUCCESS(status))
                goto Exit;

            device_info_out::version_info *ver_info = &cmd.dev_info.out->ver_info;

            RtlStringCchCopyNA(ver_info->qdma_rtl_version_str,
                sizeof(ver_info->qdma_rtl_version_str),
                version_info.qdma_rtl_version_str,
                QDMA_VERSION_INFO_LENGTH);

            RtlStringCchCopyNA(ver_info->qdma_vivado_release_id_str,
                sizeof(ver_info->qdma_vivado_release_id_str),
                version_info.qdma_vivado_release_id_str,
                QDMA_VERSION_INFO_LENGTH);

            RtlStringCchCopyNA(ver_info->qdma_device_type_str,
                sizeof(ver_info->qdma_device_type_str),
                version_info.qdma_device_type_str,
                QDMA_VERSION_INFO_LENGTH);

            RtlStringCchCopyNA(ver_info->qdma_versal_ip_type_str,
                sizeof(ver_info->qdma_versal_ip_type_str),
                version_info.qdma_versal_ip_type_str,
                QDMA_VERSION_INFO_LENGTH);

            RtlStringCchCopyNA(ver_info->qdma_sw_version,
                sizeof(ver_info->qdma_sw_version),
                version_info.qdma_sw_version_str,
                QDMA_VERSION_INFO_LENGTH);

            qdma_device_attributes_info dev_attr = {};
            status = qdma_dev->qdma_get_dev_capabilities_info(dev_attr);
            if (status != STATUS_SUCCESS)
                goto Exit;

            cmd.dev_info.out->num_pfs           = dev_attr.num_pfs;
            cmd.dev_info.out->num_qs            = dev_attr.num_qs;
            cmd.dev_info.out->flr_present       = dev_attr.flr_present;
            cmd.dev_info.out->st_en             = dev_attr.st_en;
            cmd.dev_info.out->mm_en             = dev_attr.mm_en;
            cmd.dev_info.out->mm_cmpl_en        = dev_attr.mm_cmpl_en;
            cmd.dev_info.out->mailbox_en        = dev_attr.mailbox_en;
            cmd.dev_info.out->num_mm_channels   = dev_attr.num_mm_channels;
            cmd.dev_info.out->debug_mode        = dev_attr.debug_mode;
            cmd.dev_info.out->desc_eng_mode     = dev_attr.desc_eng_mode;

            WdfRequestCompleteWithInformation(request, status, sizeof(device_info_out));

            break;
        }
        case IOCTL_QDMA_QUEUE_ADD :
        {
            queue_config q_conf = {};

            status = retrive_ioctl(request,
                        &cmd.q_conf.in, sizeof(cmd.q_conf.in));
            if (!NT_SUCCESS(status))
                goto Exit;

            status = validate_ioctl_cmd(io_control_code, cmd);
            if (!NT_SUCCESS(status))
                goto Exit;

            q_conf.h2c_ring_sz_index = cmd.q_conf.in.h2c_ring_sz_index;
            q_conf.c2h_ring_sz_index = cmd.q_conf.in.c2h_ring_sz_index;
            q_conf.c2h_buff_sz_index = cmd.q_conf.in.c2h_buff_sz_index;
            q_conf.c2h_th_cnt_index = cmd.q_conf.in.c2h_th_cnt_index;
            q_conf.c2h_timer_cnt_index = cmd.q_conf.in.c2h_timer_cnt_index;
            q_conf.is_st = cmd.q_conf.in.is_st;

            if (cmd.q_conf.in.trig_mode == trig_mode::TRIG_MODE_EVERY)
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_EVERY;
            else if (cmd.q_conf.in.trig_mode == trig_mode::TRIG_MODE_USER_COUNT)
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_USER_COUNT;
            else if (cmd.q_conf.in.trig_mode == trig_mode::TRIG_MODE_USER)
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_USER;
            else if (cmd.q_conf.in.trig_mode == trig_mode::TRIG_MODE_USER_TIMER)
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_USER_TIMER;
            else if (cmd.q_conf.in.trig_mode == trig_mode::TRIG_MODE_USER_TIMER_COUNT)
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_USER_TIMER_COUNT;
            else
                q_conf.trig_mode = qdma_trig_mode::QDMA_TRIG_MODE_DISABLE;

            if (cmd.q_conf.in.compl_sz == cmpt_desc_sz::CMPT_DESC_SZ_8B)
                q_conf.cmpt_sz = qdma_desc_sz::QDMA_DESC_SZ_8B;
            else if (cmd.q_conf.in.compl_sz == cmpt_desc_sz::CMPT_DESC_SZ_16B)
                q_conf.cmpt_sz = qdma_desc_sz::QDMA_DESC_SZ_16B;
            else if (cmd.q_conf.in.compl_sz == cmpt_desc_sz::CMPT_DESC_SZ_32B)
                q_conf.cmpt_sz = qdma_desc_sz::QDMA_DESC_SZ_32B;
            else if (cmd.q_conf.in.compl_sz == cmpt_desc_sz::CMPT_DESC_SZ_64B)
                q_conf.cmpt_sz = qdma_desc_sz::QDMA_DESC_SZ_64B;

            q_conf.desc_bypass_en = cmd.q_conf.in.desc_bypass_en;
            q_conf.pfch_bypass_en = cmd.q_conf.in.pfch_bypass_en;
            q_conf.pfch_en = cmd.q_conf.in.pfch_en;
            q_conf.cmpl_ovf_dis = cmd.q_conf.in.cmpl_ovf_dis;
            q_conf.sw_desc_sz = cmd.q_conf.in.sw_desc_sz;
            q_conf.en_mm_cmpl = cmd.q_conf.in.en_mm_cmpl;

            if (q_conf.is_st)
                q_conf.proc_st_udd_cb = drv_st_process_udd_only_pkts;
            else
                q_conf.proc_st_udd_cb = nullptr;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_ADD : %u", cmd.q_conf.in.qid);

            status = qdma_dev->qdma_add_queue(cmd.q_conf.in.qid, q_conf);
            WdfRequestComplete(request, status);
            break;
        }
        case IOCTL_QDMA_QUEUE_START :
        {
            status = retrive_ioctl(request,
                &cmd.q_conf.in, sizeof(cmd.q_conf.in));
            if (!NT_SUCCESS(status))
                goto Exit;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_START : %u",
                cmd.q_conf.in.qid);
            status = qdma_dev->qdma_start_queue(cmd.q_conf.in.qid);
            WdfRequestComplete(request, status);
            break;
        }
        case IOCTL_QDMA_QUEUE_STOP :
        {
            status = retrive_ioctl(request,
                &cmd.q_conf.in, sizeof(cmd.q_conf.in));
            if (!NT_SUCCESS(status))
                goto Exit;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_STOP : %u", cmd.q_conf.in.qid);
            status = qdma_dev->qdma_stop_queue(cmd.q_conf.in.qid);
            WdfRequestComplete(request, status);
            break;
        }
        case IOCTL_QDMA_QUEUE_DELETE :
        {
            status = retrive_ioctl(request,
                &cmd.q_conf.in, sizeof(cmd.q_conf.in));
            if (!NT_SUCCESS(status))
                goto Exit;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_DELETE : %u", cmd.q_conf.in.qid);
            status = qdma_dev->qdma_remove_queue(cmd.q_conf.in.qid);
            WdfRequestComplete(request, status);
            break;
        }
        case IOCTL_QDMA_QUEUE_DUMP_STATE :
        {
            status = retrive_ioctl(request,
                &cmd.q_state.in, sizeof(cmd.q_state.in),
                (PVOID *)&cmd.q_state.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            status = validate_ioctl_cmd(io_control_code, cmd);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.q_state.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for IOCTL_QDMA_QUEUE_DUMP_STATE");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_DUMP_STATE : %d", cmd.q_state.in.qid);
            status = qdma_dev->qdma_get_queues_state(cmd.q_state.in.qid, &qstate,
                cmd.q_state.out->state, sizeof(cmd.q_state.out->state));

            if (!NT_SUCCESS(status))
                goto Exit;

            WdfRequestCompleteWithInformation(request, status, sizeof(queue_state_out));
            break;
        }
        case IOCTL_QDMA_QUEUE_READ_UDD :
        {
            status = retrive_ioctl(request,
                &cmd.udd_info.in, sizeof(cmd.udd_info.in),
                (PVOID *)&cmd.udd_info.out, output_buffer_length);

            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.udd_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for IOCTL_QDMA_QUEUE_DUMP_STATE");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_READ_UDD : %d", cmd.udd_info.in.qid);

            status = qdma_dev->qdma_retrieve_last_st_udd_data(cmd.udd_info.in.qid,
                &cmd.udd_info.out->buffer[0], (UINT32 *)&cmd.udd_info.out->length);

            if (!NT_SUCCESS(status))
                goto Exit;

            WdfRequestCompleteWithInformation(request, status, sizeof(cmpt_udd_info_out));

            break;
        }
        case IOCTL_QDMA_QUEUE_DUMP_DESC :
        {
            status = retrive_ioctl(request,
                &cmd.desc_info.in, sizeof(cmd.desc_info.in),
                (PVOID *)&cmd.desc_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            status = validate_ioctl_cmd(io_control_code, cmd);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.desc_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for CMD_QUEUE_DUMP_DESC");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_desc_info desc_info;

            desc_info.qid = cmd.desc_info.in.qid;
            desc_info.dir = (cmd.desc_info.in.dir == QUEUE_DIR_H2C) ?
                qdma_queue_dir::QDMA_QUEUE_DIR_H2C : qdma_queue_dir::QDMA_QUEUE_DIR_C2H;
            desc_info.desc_type = (cmd.desc_info.in.desc_type == RING_DESC) ?
                qdma_desc_type::RING_DESCRIPTOR : qdma_desc_type::CMPT_DESCRIPTOR;
            desc_info.desc_start = cmd.desc_info.in.desc_start;
            desc_info.desc_end = cmd.desc_info.in.desc_end;
            desc_info.buffer_sz = (UINT32)output_buffer_length - sizeof(struct desc_dump_info_out);

            desc_info.pbuffer = &cmd.desc_info.out->pbuffer[0];
            desc_info.desc_sz = 0; /* updated by qdma api desc_dump */
            desc_info.data_sz = 0; /* updated by qdma api desc_dump */

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_DUMP_DESC  : %d", desc_info.qid);

            status = qdma_dev->qdma_queue_desc_dump(&desc_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_queue_desc_dump failed : Err : %X", status);
                goto Exit;
            }

            cmd.desc_info.out->desc_sz = desc_info.desc_sz;
            cmd.desc_info.out->data_sz = desc_info.data_sz;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(desc_dump_info_out) + desc_info.data_sz);

            break;
        }
        case IOCTL_QDMA_QUEUE_DUMP_CTX :
        {
            status = retrive_ioctl(request,
                &cmd.ctx_info.in, sizeof(cmd.ctx_info.in),
                (PVOID *)&cmd.ctx_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            status = validate_ioctl_cmd(io_control_code, cmd);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.ctx_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for CMD_QUEUE_DUMP_CTX");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_ctx_info ctx_info;

            ctx_info.qid = cmd.ctx_info.in.qid;

            if (cmd.ctx_info.in.type == ring_type::RING_TYPE_H2C)
                ctx_info.ring_type = qdma_q_type::QDMA_Q_TYPE_H2C;
            else if (cmd.ctx_info.in.type == ring_type::RING_TYPE_C2H)
                ctx_info.ring_type = qdma_q_type::QDMA_Q_TYPE_C2H;
            else if (cmd.ctx_info.in.type == ring_type::RING_TYPE_CMPT)
                ctx_info.ring_type = qdma_q_type::QDMA_Q_TYPE_CMPT;

            ctx_info.buffer_sz = output_buffer_length - sizeof(struct ctx_dump_info_out);
            ctx_info.pbuffer = &cmd.ctx_info.out->pbuffer[0];
            ctx_info.ret_sz = 0;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_DUMP_CTX : %d", ctx_info.qid);
            /* Read context data */
            status = qdma_dev->qdma_queue_dump_context(&ctx_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_queue_dump_context failed, qid : %d : Err : %X", ctx_info.qid, status);
                goto Exit;
            }

            cmd.ctx_info.out->ret_sz = ctx_info.ret_sz;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(ctx_dump_info_out) + ctx_info.ret_sz);
            break;
        }
        case IOCTL_QDMA_QUEUE_CMPT_READ :
        {
            status = retrive_ioctl(request,
                &cmd.cmpt_info.in, sizeof(cmd.cmpt_info.in),
                (PVOID *)&cmd.cmpt_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.cmpt_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for CMD_QUEUE_CMPT_READ");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_cmpt_info cmpt_info;

            cmpt_info.qid = cmd.cmpt_info.in.qid;
            cmpt_info.buffer_len = (UINT32)output_buffer_length - sizeof(struct cmpt_data_info_out);

            cmpt_info.pbuffer = &cmd.cmpt_info.out->pbuffer[0];
            cmpt_info.ret_len = 0;
            cmpt_info.cmpt_desc_sz = 0;

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_QUEUE_CMPT_READ : %d", cmpt_info.qid);
            /* Read MM completion data */
            status = qdma_dev->qdma_read_mm_cmpt_data(&cmpt_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_read_mm_cmpt_data failed : Err : %X", status);
                goto Exit;
            }

            cmd.cmpt_info.out->ret_len = cmpt_info.ret_len;
            cmd.cmpt_info.out->cmpt_desc_sz = cmpt_info.cmpt_desc_sz;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(cmpt_data_info_out) + cmpt_info.ret_len);
            break;
        }
        case IOCTL_QDMA_INTRING_DUMP :
        {
            status = retrive_ioctl(request,
                &cmd.int_ring_info.in, sizeof(cmd.int_ring_info.in),
                (PVOID *)&cmd.int_ring_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.int_ring_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for IOCTL_QDMA_INTRING_DUMP");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_intr_ring_info intring_info;
            intring_info.vec_id = cmd.int_ring_info.in.vec_id;
            intring_info.start_idx = cmd.int_ring_info.in.start_idx;
            intring_info.end_idx = cmd.int_ring_info.in.end_idx;
            intring_info.buffer_len = output_buffer_length - sizeof(struct intring_info_out);

            intring_info.ret_len = 0;
            intring_info.ring_entry_sz = 0;
            intring_info.pbuffer = &cmd.int_ring_info.out->pbuffer[0];

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_INTRING_DUMP : %d", intring_info.vec_id);
            status = qdma_dev->qdma_intring_dump(&intring_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_intring_dump failed : Err : %X", status);
                goto Exit;
            }

            cmd.int_ring_info.out->ret_len = intring_info.ret_len;
            cmd.int_ring_info.out->ring_entry_sz = intring_info.ring_entry_sz;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(intring_info_out) + intring_info.ret_len);
            break;
        }
        case IOCTL_QDMA_REG_DUMP :
        {
            status = retrive_ioctl(request, nullptr, 0,
                (PVOID *)&cmd.reg_dump_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.reg_dump_info.out == nullptr) {
                TraceError(TRACE_IO, "NULL Buffer for CMD_REG_DUMP");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_reg_dump_info regdump_info;

            regdump_info.buffer_len = output_buffer_length - sizeof(struct regdump_info_out);

            regdump_info.ret_len = 0;
            regdump_info.pbuffer = &cmd.reg_dump_info.out->pbuffer[0];

            TraceVerbose(TRACE_IO, "IOCTL_QDMA_REG_DUMP");
            status = qdma_dev->qdma_regdump(&regdump_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_regdump failed : Err : %X", status);
                goto Exit;
            }

            cmd.reg_dump_info.out->ret_len = regdump_info.ret_len;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(regdump_info_out) + regdump_info.ret_len);
            break;
        }
        case IOCTL_QDMA_QUEUE_NO_COPY :
        {
            TraceInfo(TRACE_IO, "No Copy Set for QID : %u", file_ctx->qid);
            file_ctx->no_copy = true;
            WdfRequestComplete(request, STATUS_SUCCESS);
            break;
        }
        case IOCTL_QDMA_SET_QMAX:
        {
            status = retrive_ioctl(request, &cmd.qmax_info.in, sizeof(cmd.qmax_info.in));
            if (!NT_SUCCESS(status))
                goto Exit;

            status = qdma_dev->qdma_set_qmax(cmd.qmax_info.in.qmax);
            WdfRequestComplete(request, status);
            break;
        }
        case IOCTL_QDMA_GET_QSTATS :
        {
            status = retrive_ioctl(request, nullptr, 0,
                (PVOID *)&cmd.qstats_info.out, sizeof(struct qstat_out));
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.qstats_info.out == nullptr) {
                TraceError(TRACE_IO, "nullptr Buffer for IOCTL_QDMA_GET_QSTATS");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            qdma_qstat_info qstats = { 0 };
            status = qdma_dev->qdma_get_qstats_info(qstats);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_get_qstats_info() failed : Err : %X", status);
                goto Exit;
            }

            cmd.qstats_info.out->qbase              = qstats.qbase;
            cmd.qstats_info.out->qmax               = qstats.qmax;
            cmd.qstats_info.out->active_h2c_queues  = qstats.active_h2c_queues;
            cmd.qstats_info.out->active_c2h_queues  = qstats.active_c2h_queues;
            cmd.qstats_info.out->active_cmpt_queues = qstats.active_cmpt_queues;
            WdfRequestCompleteWithInformation(request, status, sizeof(struct qstat_out));
            break;
        }
        case IOCTL_QDMA_REG_INFO:
        {
            status = retrive_ioctl(request, &cmd.reg_info.in, sizeof(cmd.reg_info.in),
                (PVOID*)&cmd.reg_info.out, output_buffer_length);
            if (!NT_SUCCESS(status))
                goto Exit;

            if (cmd.reg_info.out == nullptr) {
                TraceError(TRACE_IO, "nullptr Buffer for IOCTL_QDMA_REG_INFO");
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            struct qdma_reg_info reg_info = { 0 };
            reg_info.bar_no = cmd.reg_info.in.bar_no;
            reg_info.address = cmd.reg_info.in.address;
            reg_info.reg_cnt = cmd.reg_info.in.reg_cnt;
            reg_info.buf_len = output_buffer_length - sizeof(struct reg_info_out);;
            reg_info.ret_len = 0;
            reg_info.pbuffer = cmd.reg_info.out->pbuffer;

            status = qdma_dev->qdma_get_reg_info(&reg_info);
            if (!NT_SUCCESS(status)) {
                TraceError(TRACE_IO, "qdma_dev->qdma_get_reg_info() failed : Err : %X", status);
                goto Exit;
            }

            cmd.reg_info.out->ret_len = reg_info.ret_len;

            WdfRequestCompleteWithInformation(request, status,
                sizeof(reg_info_out) + reg_info.ret_len);
            break;
        }
        default:
            TraceInfo(TRACE_IO, "UNKNOWN IOCTL CALLED");
            WdfRequestComplete(request, STATUS_UNSUCCESSFUL);
            break;
    }

    return;

Exit:
    WdfRequestComplete(request, status);
}

void qdma_evt_io_stop(
    WDFQUEUE queue,
    WDFREQUEST request,
    ULONG action_flags)
{
    TraceInfo(TRACE_IO, "Queue 0x%p, Request 0x%p ActionFlags %d", queue, request, action_flags);

    return;
}

void qdma_evt_io_read(
    const WDFQUEUE io_queue,
    const WDFREQUEST request,
    size_t length)
{
    auto status = STATUS_SUCCESS;

    auto dev_ctx = get_device_context(WdfIoQueueGetDevice(io_queue));
    auto file_ctx = get_file_context(WdfRequestGetFileObject(request));
    NT_ASSERT(dev_ctx != nullptr);
    NT_ASSERT(file_ctx != nullptr);

    /* For Non ST mode, return success for zero length reads */
    if ((file_ctx->target != file_target::ST_QUEUE) && (length == 0)) {
        WdfRequestComplete(request, STATUS_SUCCESS);
        return;
    }

    switch (file_ctx->target) {
    case file_target::USER:
        TraceVerbose(TRACE_IO, "AXI Master Lite BAR reading %llu bytes", length);
        io_read_bar(dev_ctx->qdma, qdma_bar_type::USER_BAR, request, length);
        break;
    case file_target::CONTROL:
        TraceVerbose(TRACE_IO, "control BAR reading %llu bytes", length);
        io_read_bar(dev_ctx->qdma, qdma_bar_type::CONFIG_BAR, request, length);
        break;
    case file_target::BYPASS:
        TraceVerbose(TRACE_IO, "AXI Bridge Master BAR reading %llu bytes", length);
        io_read_bar(dev_ctx->qdma, qdma_bar_type::BYPASS_BAR, request, length);
        break;
    case file_target::DMA_QUEUE:
        TraceVerbose(TRACE_IO, "queue_%u reading %llu bytes", file_ctx->qid, length);
        io_mm_dma(dev_ctx->qdma, file_ctx->qid, request, length,
                  WdfDmaDirectionReadFromDevice);
        break;
    case file_target::ST_QUEUE:
        TraceVerbose(TRACE_IO, "queue_%u reading %llu bytes", file_ctx->qid, length);
        io_st_read_dma(dev_ctx->qdma, file_ctx->qid, request, length);
        break;
    default:
        TraceError(TRACE_IO, "Unknown file target!");
        status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
    }
}

void qdma_evt_io_write(
    const WDFQUEUE io_queue,
    const WDFREQUEST request,
    size_t length)
{

    auto status = STATUS_SUCCESS;

    auto dev_ctx = get_device_context(WdfIoQueueGetDevice(io_queue));
    auto file_ctx = get_file_context(WdfRequestGetFileObject(request));

    NT_ASSERT(dev_ctx != nullptr);
    NT_ASSERT(file_ctx != nullptr);

    /* For Non ST mode, return success for zero length writes */
    if ((file_ctx->target != file_target::ST_QUEUE) && (length == 0)) {
        WdfRequestComplete(request, STATUS_SUCCESS);
        return;
    }

    switch (file_ctx->target) {
    case file_target::USER:
        TraceVerbose(TRACE_IO, "AXI Master Lite BAR writing %llu bytes", length);
        io_write_bar(dev_ctx->qdma, qdma_bar_type::USER_BAR, request, length);
        break;
    case file_target::CONTROL:
        TraceVerbose(TRACE_IO, "control BAR writing %llu bytes", length);
        io_write_bar(dev_ctx->qdma, qdma_bar_type::CONFIG_BAR, request, length);
        break;
    case file_target::BYPASS:
        TraceVerbose(TRACE_IO, "AXI Bridge Master BAR writing %llu bytes", length);
        io_write_bar(dev_ctx->qdma, qdma_bar_type::BYPASS_BAR, request, length);
        break;
    case file_target::DMA_QUEUE:
        TraceVerbose(TRACE_IO, "queue_%u writing %llu bytes", file_ctx->qid, length);
        io_mm_dma(dev_ctx->qdma, file_ctx->qid, request, length, WdfDmaDirectionWriteToDevice);
        break;
    case file_target::ST_QUEUE:
        TraceVerbose(TRACE_IO, "queue_%u writing %llu bytes", file_ctx->qid, length);
        if (length == 0) {
            io_st_zero_write_dma(dev_ctx->qdma, file_ctx->qid, request, length, WdfDmaDirectionWriteToDevice);
        }
        else {
            io_st_write_dma(dev_ctx->qdma, file_ctx->qid, request, length, WdfDmaDirectionWriteToDevice);
        }
        break;
    default:
        TraceError(TRACE_IO, "Unknown file target!");
        status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
    }
}

void qdma_evt_device_file_create(
    const WDFDEVICE wdf_device,
    const WDFREQUEST request,
    const WDFFILEOBJECT wdf_file)
{
    auto status = STATUS_SUCCESS;

    auto file_name = WdfFileObjectGetFileName(wdf_file);
    auto dev_ctx = get_device_context(wdf_device);

    /* no filename given? */
    NT_ASSERT(file_name != nullptr);
    if (file_name->Buffer == nullptr) {
        TraceError(TRACE_DEVICE, "no device file name given!");
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    auto ctx = get_file_context(wdf_file);
    NT_ASSERT(ctx != nullptr);

    ctx->target = parse_file_name(file_name);
    if (ctx->target == file_target::UNKNOWN) {
        TraceError(TRACE_DEVICE, "device file %wZ is not supported!", file_name);
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }
    else if (ctx->target == file_target::DMA_QUEUE) {
        ctx->qid = extract_index_token(file_name);
        status = dev_ctx->qdma->qdma_is_queue_in_range(ctx->qid);
        if (STATUS_SUCCESS != status) {
            goto ErrExit;
        }
        TraceVerbose(TRACE_DEVICE, "MM qid=%u", ctx->qid);
    }
    else if (ctx->target == file_target::ST_QUEUE) {
        /* get the qid number */
        ctx->qid = extract_index_token(file_name);
        ctx->no_copy = false;
        status = dev_ctx->qdma->qdma_is_queue_in_range(ctx->qid);
        if (STATUS_SUCCESS != status) {
            goto ErrExit;
        }
        TraceVerbose(TRACE_DEVICE, "ST qid=%u", ctx->qid);
    }

    if (false == dev_ctx->qdma->qdma_is_device_online()) {
        TraceError(TRACE_DEVICE, "QDMA Device is offline.");
        status = STATUS_DEVICE_OFF_LINE;
    }

    TraceVerbose(TRACE_DEVICE, "Opening file %wZ", file_name);

ErrExit:
    WdfRequestComplete(request, status);
}

void qdma_evt_device_file_close(
    const WDFFILEOBJECT wdf_file)
{
    PUNICODE_STRING file_name = WdfFileObjectGetFileName(wdf_file);
    TraceVerbose(TRACE_DEVICE, "Closing file %wZ", file_name);
}

void qdma_evt_device_file_cleanup(
    const WDFFILEOBJECT wdf_file)
{
    PUNICODE_STRING file_name = WdfFileObjectGetFileName(wdf_file);
    TraceVerbose(TRACE_DEVICE, "Cleanup %wZ", file_name);
}
