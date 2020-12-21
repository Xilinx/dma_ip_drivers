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
#include "qdma_platform.h"
#include "qdma_access_errors.h"
#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "qdma_platform.tmh"
#endif

using namespace xlnx;

/** Xilinx TAG for tagged memory allocations */
static constexpr ULONG QDMA_MEMPOOL_TAG = 'MADQ';
static WDFSPINLOCK resource_manager_lock = nullptr;

struct err_code_map error_code_map_list[] = {
    {QDMA_SUCCESS,                          STATUS_SUCCESS},
    {QDMA_ERR_INV_PARAM,                    STATUS_INVALID_PARAMETER},
    {QDMA_ERR_NO_MEM,                       STATUS_BUFFER_TOO_SMALL},
    {QDMA_ERR_HWACC_BUSY_TIMEOUT,           STATUS_IO_TIMEOUT},
    {QDMA_ERR_HWACC_INV_CONFIG_BAR,         STATUS_INVALID_HW_PROFILE},
    {QDMA_ERR_HWACC_NO_PEND_LEGCY_INTR,     STATUS_UNSUCCESSFUL},
    {QDMA_ERR_HWACC_BAR_NOT_FOUND,          STATUS_NOT_FOUND},
    {QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED,  STATUS_NOT_SUPPORTED},
    {QDMA_ERR_RM_RES_EXISTS,                STATUS_UNSUCCESSFUL},
    {QDMA_ERR_RM_RES_NOT_EXISTS,            STATUS_UNSUCCESSFUL},
    {QDMA_ERR_RM_DEV_EXISTS,                STATUS_UNSUCCESSFUL},
    {QDMA_ERR_RM_DEV_NOT_EXISTS,            STATUS_UNSUCCESSFUL},
    {QDMA_ERR_RM_NO_QUEUES_LEFT,            STATUS_UNSUCCESSFUL},
    {QDMA_ERR_RM_QMAX_CONF_REJECTED,        STATUS_UNSUCCESSFUL}
};


void qdma_udelay(
    UINT32 delay_us)
{
    KeStallExecutionProcessor(delay_us);
}

/* Suppressing the static analysis tool reported warning.
 * Common qdma_access.c file takes care of acquiring and
 * releasing the locks properly via callbacks APIs
 * NOTE: These callback funtions are intended to use only
 * by HW access common code.
 */
#pragma warning(suppress: 28167)
int qdma_reg_access_lock(
    void *dev_hndl)
{
    qdma_device *qdma_dev = static_cast<qdma_device *>(dev_hndl);
    WdfSpinLockAcquire(qdma_dev->register_access_lock);
    return 0;
}

#pragma warning(suppress: 28167)
int qdma_reg_access_release(
    void *dev_hndl)
{
    qdma_device *qdma_dev = static_cast<qdma_device *>(dev_hndl);
    WdfSpinLockRelease(qdma_dev->register_access_lock);
    return 0;
}

void qdma_reg_write(
    void *dev_hndl,
    uint32_t reg_offset,
    uint32_t val)
{
    qdma_device *qdma_dev = static_cast<qdma_device *>(dev_hndl);
    qdma_dev->qdma_conf_reg_write(reg_offset, (ULONG)val);
}

uint32_t qdma_reg_read(
    void *dev_hndl,
    uint32_t reg_offset)
{
    qdma_device *qdma_dev = static_cast<qdma_device *>(dev_hndl);
    return qdma_dev->qdma_conf_reg_read(reg_offset);
}

void qdma_get_hw_access(
    void *dev_hndl,
    struct qdma_hw_access **hw)
{
    qdma_device *qdma_dev = static_cast<qdma_device *>(dev_hndl);
    *hw = &qdma_dev->hw;
}

void qdma_strncpy(
    char *dest,
    const char *src,
    size_t n)
{
    RtlStringCchPrintfA(dest, n, "%s", src);
}

int qdma_get_err_code(
    int acc_err_code)
{
    int n;

    if (acc_err_code < 0)
        acc_err_code = -(acc_err_code);

    n = sizeof(error_code_map_list) / sizeof(error_code_map_list[0]);
    if (acc_err_code < n)
        return (error_code_map_list[acc_err_code].err_code);
    else
        return STATUS_UNSUCCESSFUL;
}

void *qdma_calloc(
    uint32_t num_blocks,
    uint32_t size)
{
    size_t total_size = (size_t)num_blocks * size;
    void *ptr = ExAllocatePoolWithTag(NonPagedPoolNx, total_size, QDMA_MEMPOOL_TAG);
    if (ptr != NULL) {
        RtlZeroMemory(ptr, total_size);
    }

    return ptr;
}

void qdma_memfree(
    void *memptr)
{
    ExFreePoolWithTag(memptr, QDMA_MEMPOOL_TAG);
}

int qdma_resource_lock_init(void)
{
    if (resource_manager_lock == nullptr) {
        WDF_OBJECT_ATTRIBUTES attr;
        WDF_OBJECT_ATTRIBUTES_INIT(&attr);
        attr.ParentObject = WdfGetDriver();

        NTSTATUS status = WdfSpinLockCreate(&attr, &resource_manager_lock);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_QDMA, "Resource lock creation failed!");
            return -1;
        }
    }

    return 0;
}

/* Suppressing the static analysis tool reported warning.
 * Common qdma_access.c file takes care of acquiring and
 * releasing the locks properly via callback APIs
 * NOTE: These callback funtions are intended to use only
 * by HW access common code.
 */
#pragma warning(suppress: 28167)
void qdma_resource_lock_take(void)
{
    WdfSpinLockAcquire(resource_manager_lock);
}

#pragma warning(suppress: 28167)
void qdma_resource_lock_give(void)
{
    WdfSpinLockRelease(resource_manager_lock);
}
