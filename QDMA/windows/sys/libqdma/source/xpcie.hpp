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

#include "qdma_platform_env.h"
#include "qdma_exports.h"
#include <wdmguid.h>
#include <wchar.h>

namespace xlnx {

struct sbdf {
    UINT32  fun_no : 3;
    UINT32  dev_no : 5;
    UINT32  bus_no : 8;
    UINT32  seg_no : 16;
};

union pci_sbdf {
    struct sbdf sbdf;
    UINT32 val;
};

class xpcie_device {
    struct bar {
        /** kernel virtual address of pcie BAR */
        UCHAR *base = nullptr;
        /** length of address space in Bytes */
        size_t length = 0;
        /** BAR index, not to confuse with BAR number */
        unsigned int index = 0;

        NTSTATUS read(size_t offset, void* data, size_t size) const;
        NTSTATUS write(size_t offset, void* data, size_t size) const;
    };

    static constexpr size_t MAX_NUM_BARS = 6;
    ULONG msix_vectors = 0;
    UINT32 num_bars = 0;
    bar bars[MAX_NUM_BARS];
    bar* config_bar = nullptr;
    bar* user_bar = nullptr;
    bar* bypass_bar = nullptr;

public:
    NTSTATUS map(const WDFCMRESLIST resources);
    void unmap(void);
    NTSTATUS find_num_msix_vectors(const WDFDEVICE device);
    ULONG get_num_msix_vectors(void) const;
    NTSTATUS assign_config_bar(const UINT8 bar_idx);
    NTSTATUS assign_bar_types(const UINT8 user_bar_idx);
    NTSTATUS get_bdf(const WDFDEVICE device, UINT32 &bdf);

    NTSTATUS read_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size) const;
    NTSTATUS write_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size) const;
    ULONG conf_reg_read(size_t offset) const;
    void conf_reg_write(size_t offset, ULONG data) const;
    NTSTATUS get_bar_info(qdma_bar_type bar_type, PVOID &bar_base, size_t &bar_length) const;
};
} /* namespace xlnx */


