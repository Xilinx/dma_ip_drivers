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

#include <ntintsafe.h>
#include <stddef.h>

namespace xlnx {

/* ----------- constants ----------- */
static constexpr UINT8 DEFAULT_USER_BAR = 1;

/* register base address */
static constexpr size_t qdma_trq_cmc_msix_table = 0x0000'2000;
static constexpr size_t qdma_msix_vectors_mask_step = 0x0000'000c;

/* HW Config Constants */
static constexpr size_t qdma_max_msix_vectors_per_pf = 8;

/* ---------- struct definitions ---------- */
#pragma pack(1)

/* ---------- descriptor structures ---------- */
struct c2h_descriptor {
    UINT64 addr;
};
static_assert(sizeof(c2h_descriptor) == (8 * sizeof(UINT8)), "c2h_descriptor must be 8 bytes wide!");

struct h2c_descriptor {
    UINT16 cdh_flags;     /**< Dont care bits */
    UINT16 pld_len;       /**< Packet length in bytes */
    UINT16 length;
    UINT16 sop : 1;
    UINT16 eop : 1;
    UINT16 reserved : 14;
    UINT64 addr;
};
static_assert(sizeof(h2c_descriptor) == (16 * sizeof(UINT8)), "h2c_descriptor must be 16 bytes wide!");

struct mm_descriptor {
    UINT64 addr;

    UINT64 length : 28;
    UINT64 valid : 1;
    UINT64 sop : 1;
    UINT64 eop : 1;
    UINT64 reserved_0 : 33;

    UINT64 dest_addr;

    UINT64 reserved_1;
};
static_assert(sizeof(mm_descriptor) == (32 * sizeof(UINT8)), "mm_descriptor must be 32 bytes wide!");

/* interrupt ring entry structure */
struct intr_entry {
    /* Desc bits */
    UINT32 desc_pidx : 16;
    UINT32 desc_cidx : 16;

    UINT32 desc_color : 1;
    UINT32 desc_int_state : 2;
    UINT32 desc_err : 2;
    /* source bits */
    UINT32 rsvd : 1;
    UINT32 intr_type : 1;
    UINT32 qid : 24;
    UINT32 color : 1;
};
static_assert(sizeof(intr_entry) == (8 * sizeof(UINT8)), "intr_entry must be 8 bytes wide!");

struct cpm_intr_entry {
    /* Desc bits */
    UINT32 desc_pidx : 16;
    UINT32 desc_cidx : 16;

    UINT32 desc_color : 1;
    UINT32 desc_int_state : 2;
    UINT32 desc_err : 4;
    /* source bits */
    UINT32 rsvd : 11;
    UINT32 err_int : 1;
    UINT32 intr_type : 1;
    UINT32 qid : 11;
    UINT32 color : 1;
};
static_assert(sizeof(cpm_intr_entry) == (8 * sizeof(UINT8)), "intr_entry must be 8 bytes wide!");
/* ------ */

/* ---------- writeback structures ---------- */

/** c2h_wb_header_8B -- C2H Completion data structure
  * This includes user defined data, optional color, err bits, etc.,.
  * The user defined data has four size options: 8B, 16B, 32B and 64B
  *
  * The below format is specific to QDMA example design present in vivado
  */
struct c2h_wb_header_8B {
    /** @data_frmt : 0 indicates valid length field is present */
    UINT64 data_frmt      : 1;
    /** @color : Indicates the validity of the entry */
    UINT64 color          : 1;
    /** @desc_error : Indicates the error status */
    UINT64 desc_error     : 1;
    /** @desc_used : Indicates whether data descriptor used */
    UINT64 desc_used      : 1;
    /** @length : Length of the completion entry */
    UINT64 length         : 16;
    /** @user_rsv : Reserved */
    UINT64 user_rsv       : 4;
    /** @user_defined_0 : User Defined Data (UDD) */
    UINT64 user_defined_0 : 40;
};
static_assert(sizeof(c2h_wb_header_8B) == (8 * sizeof(UINT8)), "c2h_wb_header_8B must be 8 bytes wide!");

struct c2h_wb_header_16B : c2h_wb_header_8B {
    /** @user_defined_1 : User Defined Data (UDD) for 16B completion size */
    UINT64 user_defined_1;
};
static_assert(sizeof(c2h_wb_header_16B) == (16 * sizeof(UINT8)), "c2h_wb_header_16B must be 16 bytes wide!");

struct c2h_wb_header_32B : c2h_wb_header_16B {
    /** @user_defined_2 : User Defined Data (UDD) for 32B completion size */
    UINT64 user_defined_2[2];
};
static_assert(sizeof(c2h_wb_header_32B) == (32 * sizeof(UINT8)), "c2h_wb_header_32B must be 32 bytes wide!");

struct c2h_wb_header_64B : c2h_wb_header_32B {
    /** @user_defined_3 : User Defined Data (UDD) for 64B completion size */
    UINT64 user_defined_3[4];
};
static_assert(sizeof(c2h_wb_header_64B) == (64 * sizeof(UINT8)), "c2h_wb_header_64B must be 64 bytes wide!");


struct wb_status_base {
    UINT16 pidx;
    UINT16 cidx;
};

struct c2h_wb_status : wb_status_base {
    UINT32 color : 1;
    UINT32 irq_state : 2;
    UINT32 reserved : 29;
};
static_assert(sizeof(c2h_wb_status) == (sizeof(UINT64)), "c2h_wb_status must be 64 bits wide!");
static_assert(sizeof(c2h_wb_status) == (sizeof(c2h_wb_header_8B)), "c2h_wb_status and c2h_wb_header_8B must be same size!");

struct h2c_wb_status : wb_status_base {
    UINT32 reserved_1;
};
static_assert(sizeof(h2c_wb_status) == (sizeof(UINT64)), "h2c_wb_status must be 64 bits wide!");

using mm_wb_status = h2c_wb_status; /* MM wb status has same struct layout as h2c wb */

#pragma pack()

} /* namespace xlnx */
