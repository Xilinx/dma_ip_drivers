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

#define QDMA_MAX_QUEUES_PER_PF  512
#define QDMA_MAX_USER_INTR      1
#define QDMA_MAX_DATA_INTR      7

struct device_context {
    struct {
        xlnx::queue_op_mode op_mode;
        UINT8 config_bar;
    }config;

    xlnx::qdma_interface *qdma;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(device_context, get_device_context)

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
qdma_create_device(
    _Inout_ PWDFDEVICE_INIT device_init
);

enum class file_target {
    USER,
    CONTROL,
    BYPASS,
    DMA_QUEUE,
    ST_QUEUE,
    MGMT,
    UNKNOWN = 255,
};

struct file_context {
    file_target target;
    UINT16 qid;
    bool no_copy;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(file_context, get_file_context)

EVT_WDF_DEVICE_PREPARE_HARDWARE qdma_evt_device_prepare_hardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE qdma_evt_device_release_hardware;

EXTERN_C_END
