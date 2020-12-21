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

#pragma once

#include "windows_common.h"
#include "qdma_exports.h"

EXTERN_C_START

using namespace xlnx;

static constexpr ULONG IO_QUEUE_TAG = 'UQOI';

NTSTATUS qdma_io_queue_initialize(WDFDEVICE wdf_device);

struct DMA_TXN_CONTEXT {
    UINT16 qid;
    size_t txn_len;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DMA_TXN_CONTEXT, get_dma_txn_context)


struct ST_DMA_ZERO_TX_PRIV {
    WDFREQUEST request;
    PVOID sg_list;
};

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL qdma_evt_ioctl;
EVT_WDF_IO_QUEUE_IO_STOP qdma_evt_io_stop;
EVT_WDF_IO_QUEUE_IO_READ qdma_evt_io_read;
EVT_WDF_IO_QUEUE_IO_WRITE qdma_evt_io_write;

EVT_WDF_DEVICE_FILE_CREATE qdma_evt_device_file_create;
EVT_WDF_FILE_CLOSE qdma_evt_device_file_close;
EVT_WDF_FILE_CLEANUP qdma_evt_device_file_cleanup;

EXTERN_C_END
