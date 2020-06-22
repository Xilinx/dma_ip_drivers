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

EXTERN_C_START

/**
 *  WDFDRIVER Events
 */

DRIVER_INITIALIZE               DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD       qdma_evt_device_add;
EVT_WDF_OBJECT_CONTEXT_CLEANUP  qdma_evt_driver_context_cleanup;


EXTERN_C_END
