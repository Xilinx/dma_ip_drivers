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

#include "driver.h"
#include "device.h"

#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "driver.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, qdma_evt_device_add)
#pragma alloc_text (PAGE, qdma_evt_driver_context_cleanup)
#endif

using namespace xlnx;

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT driver_object,
    _In_ PUNICODE_STRING registry_path
)
{
    /** Initialize WPP Tracing */
    WPP_INIT_TRACING(driver_object, registry_path);

    TraceInfo(TRACE_DRIVER, "Xilinx PCIe Multi-Queue DMA Driver - Built %s,%s",
              __DATE__, __TIME__);

    /** Register a cleanup callback to call WPP_CLEANUP during driver unload */
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = qdma_evt_driver_context_cleanup;

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, qdma_evt_device_add);

    NTSTATUS status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(driver_object);
        return status;
    }

    TraceVerbose(TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

_Use_decl_annotations_
NTSTATUS
qdma_evt_device_add(
    _In_    WDFDRIVER       driver,
    _Inout_ PWDFDEVICE_INIT device_init
)
{
    UNREFERENCED_PARAMETER(driver);

    PAGED_CODE();
    return qdma_create_device(device_init);
}

_Use_decl_annotations_
VOID
qdma_evt_driver_context_cleanup(
    _In_ WDFOBJECT driver_object
)
{
    // EvtCleanupCallback for WDFDEVICE objects is always called at PASSIVE_LEVEL
    _IRQL_limited_to_(PASSIVE_LEVEL);
    PAGED_CODE();

#ifndef ENABLE_WPP_TRACING
    UNREFERENCED_PARAMETER(driver_object);
#endif

    TraceVerbose(TRACE_DRIVER, "%!FUNC! Entry");
    /** Stop WPP Tracing */
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(static_cast<WDFDRIVER>(driver_object)));
}
