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

#define INITGUID

#include "qdma_driver_api.h"
#include "io_queue.h"
#include "device.h"

#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, qdma_create_device)
#pragma alloc_text (PAGE, qdma_evt_device_prepare_hardware)
#pragma alloc_text (PAGE, qdma_evt_device_release_hardware)
#endif

using namespace xlnx;

_Use_decl_annotations_
NTSTATUS
qdma_create_device(
    _Inout_ PWDFDEVICE_INIT wdf_device_init
)
{
    PAGED_CODE();

    WdfDeviceInitSetIoType(wdf_device_init, WdfDeviceIoDirect);

    /* Set call-backs for any of the functions we are interested in. If no call-back is set, the
     * framework will take the default action by itself.
     */
    WDF_PNPPOWER_EVENT_CALLBACKS pnp_power_callbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnp_power_callbacks);
    pnp_power_callbacks.EvtDevicePrepareHardware = qdma_evt_device_prepare_hardware;
    pnp_power_callbacks.EvtDeviceReleaseHardware = qdma_evt_device_release_hardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(wdf_device_init, &pnp_power_callbacks);

    WDF_OBJECT_ATTRIBUTES device_attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&device_attributes, device_context);

    /* Register file object call-backs */
    WDF_FILEOBJECT_CONFIG file_config;
    WDF_FILEOBJECT_CONFIG_INIT(&file_config,
                               qdma_evt_device_file_create,
                               qdma_evt_device_file_close,
                               qdma_evt_device_file_cleanup);
    WDF_OBJECT_ATTRIBUTES file_attribs;
    WDF_OBJECT_ATTRIBUTES_INIT(&file_attribs);
    file_attribs.SynchronizationScope = WdfSynchronizationScopeNone;
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&file_attribs, file_context);
    WdfDeviceInitSetFileObjectConfig(wdf_device_init, &file_config, &file_attribs);

    WDFDEVICE device;
    auto status = WdfDeviceCreate(&wdf_device_init, &device_attributes, &device);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "WdfDeviceCreate failed! %!STATUS!", status);
        return status;
    }

    /* Create a device interface to user-space */
    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_QDMA, nullptr);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "WdfDeviceCreateDeviceInterface failed! %!STATUS!", status);
        return status;
    }

    /* Initialize the I/O Package and any Queues */
    status = qdma_io_queue_initialize(device);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "qdma_io_queue_initialize() failed! %!STATUS!", status);
        return status;
    }

    return status;
}

static NTSTATUS read_reg_params(
    xlnx::queue_op_mode &op_mode,
    UINT8 &config_bar)
{
    WDFKEY key;
    DECLARE_CONST_UNICODE_STRING(value_drv_mode, L"DRIVER_MODE");
    DECLARE_CONST_UNICODE_STRING(value_config_bar, L"CONFIG_BAR");
    op_mode = xlnx::queue_op_mode::POLL_MODE;
    WDFDRIVER driver = WdfGetDriver();

    NTSTATUS status = WdfDriverOpenParametersRegistryKey(driver,
                                                         STANDARD_RIGHTS_ALL,
                                                         WDF_NO_OBJECT_ATTRIBUTES,
                                                         &key);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "WdfDriverOpenParametersRegistryKey failed: %!STATUS!", status);
        goto ErrExit;
    }

    ULONG DRV_MODE;
    status = WdfRegistryQueryULong(key, &value_drv_mode, &DRV_MODE);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "WdfRegistryQueryULong failed: %!STATUS!", status);
        goto ErrExit;
    }

    if (DRV_MODE > INTR_COAL_MODE) {
        TraceError(TRACE_DEVICE, "Invalid parameter DRIVER_MODE - [%lu]", DRV_MODE);
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    op_mode = (xlnx::queue_op_mode)DRV_MODE;

    ULONG CONFIG_BAR;
    status = WdfRegistryQueryULong(key, &value_config_bar, &CONFIG_BAR);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "WdfRegistryQueryULong failed: %!STATUS!", status);
        goto ErrExit;
    }

    if (CONFIG_BAR > 2ul) {
        TraceError(TRACE_DEVICE, "Invalid parameter CONFIG_BAR - [%lu]", CONFIG_BAR);
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    config_bar = (UINT8)CONFIG_BAR;

    TraceVerbose(TRACE_DEVICE, "Parameter DRV_MODE - [%lu] CONFIG_BAR - [%lu]", op_mode, CONFIG_BAR);

ErrExit:
    WdfRegistryClose(key);
    return status;
}

NTSTATUS qdma_evt_device_prepare_hardware(
    const WDFDEVICE device,
    const WDFCMRESLIST resources,
    const WDFCMRESLIST resources_translated)
{
    PAGED_CODE();

    device_context* ctx = get_device_context(device);

    ctx->qdma = qdma_interface::create_qdma_device();

    NTSTATUS status = read_reg_params(ctx->config.op_mode, ctx->config.config_bar);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "failed to read registry params %!STATUS!", status);
        return STATUS_INTERNAL_ERROR;
    }

    status = ctx->qdma->init(ctx->config.op_mode, ctx->config.config_bar, QDMA_MAX_QUEUES_PER_PF);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "qdma.init() failed! %!STATUS!", status);
        return STATUS_INTERNAL_ERROR;
    }

    status = ctx->qdma->open(device, resources, resources_translated);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DEVICE, "qdma.open() failed! %!STATUS!", status);
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS qdma_evt_device_release_hardware(
    const WDFDEVICE device,
    WDFCMRESLIST resources_translated)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(resources_translated);

    device_context* ctx = get_device_context(device);
    if (ctx) {
        ctx->qdma->close();
        qdma_interface::remove_qdma_device(ctx->qdma);
        ctx->qdma = nullptr;
    }

    return STATUS_SUCCESS;
}