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

#include <initguid.h>
#include "xpcie.hpp"
#include "qdma_reg_ext.h"
#include "trace.h"
#include <pciprop.h>

#ifdef ENABLE_WPP_TRACING
#include "xpcie.tmh"
#endif

using namespace xlnx;

NTSTATUS xpcie_device::map(
    const WDFCMRESLIST resources)
{
    num_bars = 0;
    const ULONG num_resources = WdfCmResourceListGetCount(resources);
    TraceVerbose(TRACE_PCIE, "# PCIe resources = %d", num_resources);

    for (ULONG i = 0; i < num_resources; i++) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = WdfCmResourceListGetDescriptor(resources, i);
        if (resource == nullptr) {
            TraceError(TRACE_PCIE, "WdfCmResourceListGetDescriptor() fails");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (resource->Type == CmResourceTypeMemory) {
            /** index = 0 indictaes the first BAR details found in resource list, 
                index = 1 indicates the second BAR detauls found in resource list, etc.,

                This index is not the BAR number.
            */
            bars[num_bars].index = num_bars;
            bars[num_bars].length = resource->u.Memory.Length;
            bars[num_bars].base = static_cast<UCHAR *>(MmMapIoSpaceEx(resource->u.Memory.Start, 
                resource->u.Memory.Length, PAGE_READWRITE | PAGE_NOCACHE));

            if (bars[num_bars].base == nullptr) {
                TraceError(TRACE_PCIE, "MmMapIoSpace returned NULL! for BAR%u", num_bars);
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            TraceVerbose(TRACE_PCIE, "MM BAR %d (addr:0x%lld, length:%llu) mapped at 0x%08p",
                      num_bars, resource->u.Memory.Start.QuadPart, bars[num_bars].length, bars[num_bars].base);
            num_bars++;
        }
    }

    return STATUS_SUCCESS;
}

void xpcie_device::unmap(void)
{
    /* Unmap any I/O ports. Disconnecting from the interrupt will be done automatically by the framework. */
    for (unsigned int i = 0; i < num_bars; i++) {
        if (bars[i].base != nullptr) {
            TraceVerbose(TRACE_PCIE, "Unmapping BAR%d, VA:(%p) Length %llu", i, bars[i].base, bars[i].length);
            MmUnmapIoSpace(bars[i].base, bars[i].length);
            bars[i].base = nullptr;
        }
    }
}

NTSTATUS xpcie_device::get_bdf(const WDFDEVICE device, UINT32 &bdf)
{
    ULONG bus_number;
    ULONG address;
    NTSTATUS status;
    ULONG length;
    union pci_sbdf dev_sbdf;

    status = WdfDeviceQueryProperty(device, DevicePropertyBusNumber,
        sizeof(bus_number), (PVOID)&bus_number,
        &length);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_PCIE,
            "WdfDeviceQueryProperty failed for DevicePropertyBusNumber: %!STATUS!",
            status);
        return status;
    }

    status = WdfDeviceQueryProperty(device, DevicePropertyAddress,
        sizeof(address), (PVOID)&address,
        &length);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_PCIE,
            "WdfDeviceQueryProperty failed for DevicePropertyAddress: %!STATUS!",
            status);
        return status;
    }

    dev_sbdf.val = 0x0;
    dev_sbdf.sbdf.seg_no = (bus_number >> 8) & 0xFFFF;
    dev_sbdf.sbdf.bus_no = bus_number & 0xFF;
    dev_sbdf.sbdf.dev_no = (address >> 16) & 0xFFFF;
    dev_sbdf.sbdf.fun_no = address & 0xFFFF;

    TraceVerbose(TRACE_PCIE,
        "PCIe Seg_No : 0x%X, Bus_No : 0x%X, Dev_No: 0x%X, Fun_No: 0x%X",
        dev_sbdf.sbdf.seg_no, dev_sbdf.sbdf.bus_no, dev_sbdf.sbdf.dev_no, dev_sbdf.sbdf.fun_no);

    bdf = dev_sbdf.val;

    return STATUS_SUCCESS;
}

ULONG xpcie_device::get_num_msix_vectors(void) const
{
    return msix_vectors;
}

NTSTATUS xpcie_device::find_num_msix_vectors(
    const WDFDEVICE device)
{
    NTSTATUS status;
    ULONG intr_support;
    ULONG intr_max;
    ULONG length;
    DEVPROPTYPE type;
    WDF_DEVICE_PROPERTY_DATA dev_prop_data;

    msix_vectors = 0;

    WDF_DEVICE_PROPERTY_DATA_INIT(&dev_prop_data, &DEVPKEY_PciDevice_InterruptSupport);

    status = WdfDeviceQueryPropertyEx(device, &dev_prop_data, sizeof(intr_support), (PVOID)&intr_support,
        &length, &type);

    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_PCIE,
            "WdfDeviceQueryProperty failed for PciDevice_InterruptSupport: %!STATUS!",
            status);
        return status;
    }

    TraceVerbose(TRACE_PCIE, "PciDevice_InterruptSupport: %d, type : %d", intr_support, type);

    if (!(intr_support & DevProp_PciDevice_InterruptType_MsiX)) {
        TraceError(TRACE_PCIE, "MSI-X interrupts are not supported");
        return STATUS_NOT_SUPPORTED;
    }

    WDF_DEVICE_PROPERTY_DATA_INIT(&dev_prop_data, &DEVPKEY_PciDevice_InterruptMessageMaximum);

    status = WdfDeviceQueryPropertyEx(device, &dev_prop_data, sizeof(intr_max), (PVOID)&intr_max,
        &length, &type);

    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_PCIE,
            "WdfDeviceQueryProperty failed for DEVPKEY_PciDevice_InterruptMessageMaximum: %!STATUS!",
            status);
        return status;
    }

    TraceVerbose(TRACE_PCIE, "InterruptMessageMaximum: %d, type : %d", intr_max, type);

    msix_vectors = intr_max;

    return STATUS_SUCCESS;
}

NTSTATUS xpcie_device::assign_config_bar(const UINT8 bar_idx)
{
    if (bar_idx > num_bars)
        return STATUS_UNSUCCESSFUL;

    config_bar = &bars[bar_idx];
    return STATUS_SUCCESS;
}


NTSTATUS xpcie_device::assign_bar_types(const UINT8 user_bar_idx)
{
    UINT32 config_bar_idx = 0;
    config_bar_idx = (UINT8)(config_bar - bars);

    if (num_bars > 1) {
        user_bar = &bars[user_bar_idx];
        TraceInfo(TRACE_PCIE, "AXI Master Lite BAR %u", (user_bar_idx * 2));

        if (num_bars > 2) {
            for (auto i = 0u; i < num_bars; ++i) {
                if (i == user_bar_idx || i == config_bar_idx)
                    continue;

                bypass_bar = &bars[i];
                TraceInfo(TRACE_PCIE, "AXI Bridge Master BAR %d", (i * 2));
                break;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS xpcie_device::read_bar(
    qdma_bar_type bar_type,
    size_t offset,
    void* data,
    size_t size) const
{
    xpcie_device::bar *bar = nullptr;

    if ((nullptr == data) || ((size_t)0 == size)) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (bar_type) {
    case qdma_bar_type::CONFIG_BAR:
        bar = config_bar;
        break;
    case qdma_bar_type::USER_BAR:
        bar = user_bar;
        break;
    case qdma_bar_type::BYPASS_BAR:
        bar = bypass_bar;
        break;
    }

    if (nullptr == bar)
        return STATUS_INVALID_PARAMETER;

    return bar->read(offset, data, size);
}

NTSTATUS xpcie_device::write_bar(
    qdma_bar_type bar_type,
    size_t offset,
    void* data,
    size_t size) const
{
    xpcie_device::bar *bar = nullptr;

    if ((nullptr == data) || ((size_t)0 == size)) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (bar_type) {
    case qdma_bar_type::CONFIG_BAR:
        bar = config_bar;
        break;
    case qdma_bar_type::USER_BAR:
        bar = user_bar;
        break;
    case qdma_bar_type::BYPASS_BAR:
        bar = bypass_bar;
        break;
    }

    if (nullptr == bar)
        return STATUS_INVALID_PARAMETER;

    return bar->write(offset, data, size);
}

NTSTATUS xpcie_device::get_bar_info(
    qdma_bar_type bar_type,
    PVOID &bar_base,
    size_t &bar_length) const
{
    switch (bar_type) {
    case qdma_bar_type::CONFIG_BAR:
        bar_base = (PVOID)config_bar->base;
        bar_length = config_bar->length;
        break;
    case qdma_bar_type::USER_BAR:
        bar_base = (PVOID)user_bar->base;
        bar_length = user_bar->length;
        break;
    case qdma_bar_type::BYPASS_BAR:
        bar_base = (PVOID)bypass_bar->base;
        bar_length = bypass_bar->length;
        break;
    default:
        bar_base = NULL;
        bar_length = (size_t)0;
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

ULONG xpcie_device::conf_reg_read(size_t offset) const
{
    NT_ASSERTMSG("Error: BAR not assigned!", config_bar->base != nullptr);
    NT_ASSERTMSG("Error: BAR overrun!", ((offset + sizeof(ULONG)) <= config_bar->length));

    return READ_REGISTER_ULONG(reinterpret_cast<volatile ULONG*>(config_bar->base + offset));
}

void xpcie_device::conf_reg_write(size_t offset, ULONG data) const
{
    NT_ASSERTMSG("Error: BAR not assigned!", config_bar->base != nullptr);
    NT_ASSERTMSG("Error: BAR overrun!", ((offset + sizeof(ULONG)) <= config_bar->length));

    WRITE_REGISTER_ULONG(reinterpret_cast<volatile ULONG*>(config_bar->base + offset), data);
}

NTSTATUS xpcie_device::bar::write(
    const size_t offset,
    void *data,
    const size_t size) const
{
    if (base == nullptr) {
        TraceError(TRACE_PCIE, "Attempted to access non-mapped BAR!");
        return STATUS_INVALID_ADDRESS;
    }

    /* check if request runs over BAR address space */
    if ((offset + size) > length) {
        TraceError(TRACE_PCIE, "Error: BAR overrun!");
        return STATUS_INVALID_PARAMETER;
    }

    UCHAR *vaddr = base + offset;

    if (size % sizeof(ULONG64) == 0) {
        WRITE_REGISTER_BUFFER_ULONG64(reinterpret_cast<volatile ULONG64*>(vaddr), 
            static_cast<PULONG64>(data), static_cast<ULONG>(size) / sizeof(ULONG64));
    } else if (size % sizeof(ULONG) == 0) {
        WRITE_REGISTER_BUFFER_ULONG(reinterpret_cast<volatile ULONG*>(vaddr), 
            static_cast<PULONG>(data), static_cast<ULONG>(size) / sizeof(ULONG));
    } else if (size % sizeof(USHORT) == 0) {
        WRITE_REGISTER_BUFFER_USHORT(reinterpret_cast<volatile USHORT*>(vaddr), 
            static_cast<PUSHORT>(data), static_cast<ULONG>(size) / sizeof(USHORT));
    } else {
        WRITE_REGISTER_BUFFER_UCHAR(reinterpret_cast<volatile UCHAR*>(vaddr), 
            static_cast<PUCHAR>(data), static_cast<ULONG>(size));
    }

    return STATUS_SUCCESS;
}

NTSTATUS xpcie_device::bar::read(
    const size_t offset,
    void *data,
    const size_t size) const
{
    if (base == nullptr) {
        TraceError(TRACE_PCIE, "Attempted to access non-mapped BAR!");
        return STATUS_INVALID_ADDRESS;
    }

    /* check if request runs over BAR address space */
    if ((offset + size) > length) {
        TraceError(TRACE_PCIE, "Error: BAR overrun!");
        return STATUS_INVALID_PARAMETER;
    }

    UCHAR* vaddr = base + offset;

    if (size % sizeof(ULONG) == 0) {
        READ_REGISTER_BUFFER_ULONG(reinterpret_cast<volatile ULONG*>(vaddr), static_cast<PULONG>(data), static_cast<ULONG>(size) / sizeof(ULONG));
    } else if (size % sizeof(USHORT) == 0) {
        READ_REGISTER_BUFFER_USHORT(reinterpret_cast<volatile USHORT*>(vaddr), static_cast<PUSHORT>(data), static_cast<ULONG>(size) / sizeof(USHORT));
    } else {
        READ_REGISTER_BUFFER_UCHAR(vaddr, static_cast<PUCHAR>(data), static_cast<ULONG>(size));
    }

    return STATUS_SUCCESS;
}
