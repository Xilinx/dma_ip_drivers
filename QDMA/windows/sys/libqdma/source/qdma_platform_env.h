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

#ifndef LIBQDMA_QDMA_PLATFORM_ENV_H_
#define LIBQDMA_QDMA_PLATFORM_ENV_H_

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include <ntddk.h>
#include <wdf.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <stdio.h>
#include <stddef.h>

#include "trace.h"

#define QDMA_SNPRINTF_S _snprintf_s

typedef UINT32 uint32_t;
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT64 uint64_t;
typedef INT32 int32_t;

#endif /* LIBQDMA_QDMA_PLATFORM_ENV_H_ */
