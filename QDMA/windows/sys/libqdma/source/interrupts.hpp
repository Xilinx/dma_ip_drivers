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
#include "qdma_config.h"
#include "qdma_reg_ext.h"

namespace xlnx {
/* Forward declaration */
class qdma_device;
struct queue_pair;

#define VECTOR_TYPE_ERROR           0
#define VECTOR_TYPE_USER            1
#define VECTOR_TYPE_DATA_START      2

#define IND_INTR_MAX_DATA_VECTORS   QDMA_NUM_DATA_VEC_FOR_INTR_CXT

enum class interrupt_type {
    NONE,
    LEGACY,
    MSIX
};

struct intr_queue {
    static constexpr size_t size = 512;
    size_t capacity = size;
    size_t npages = 1;

    UINT16 idx = 0;     /* queue index - relative to this PF */
    UINT16 idx_abs = 0; /* queue index - abolute across all PF */

    UINT8 color = 1;
    ULONG vector = 0;

    qdma_intr_cidx_reg_info intr_cidx_info;
    qdma_device* qdma = nullptr;
    WDFCOMMONBUFFER buffer = nullptr;
    void *buffer_va = nullptr;

    volatile UINT32 sw_index = 0; /* Driver CIDX */

    NTSTATUS create(WDFDMAENABLER& dma_enabler);
    void clear_contents(void);

    void advance_sw_index(void);
    PFORCEINLINE void update_csr_cidx(queue_pair *q, UINT32 new_cidx);
    NTSTATUS intring_dump(qdma_intr_ring_info *intring_info);
};

typedef struct QDMA_IRQ_CONTEXT {
    bool is_coal;
    interrupt_type intr_type = interrupt_type::NONE;
    ULONG vector_id;
    UINT32 weight;

    /* For user interrupt handling */
    void *user_data;

    /* For Error interrupt handling */
    qdma_device *qdma_dev = nullptr;

    /* For direct interrupt handling */
    LIST_ENTRY queue_list_head;

    /* For indirect interrupt handling */
    intr_queue *intr_q = nullptr;

    /* Interrupt handler function */
    void (*interrupt_handler)(QDMA_IRQ_CONTEXT *);

}*PQDMA_IRQ_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QDMA_IRQ_CONTEXT, get_qdma_irq_context);

struct interrupt_manager {
    interrupt_type intr_type = interrupt_type::NONE;
    ULONG err_vector_id = 0;
    ULONG user_vector_id_start = 0;
    ULONG user_vector_id_end = 0;
    ULONG data_vector_id_end = 0;
    ULONG data_vector_id_start = 0;

    WDFSPINLOCK lock = nullptr;
    UINT32 irq_weight[qdma_max_msix_vectors_per_pf];
    WDFINTERRUPT irq[qdma_max_msix_vectors_per_pf];
    intr_queue intr_q[qdma_max_msix_vectors_per_pf];
};

} /* namespace xlnx */


