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

/**
 * @file
 * @brief This file contains the declarations for libqdma interfaces
 *
 */

#include "qdma_platform_env.h"
#include "xversion.hpp"
#include "xpcie.hpp"
#include "qdma_reg_ext.h"
#include "qdma_config.h"
#include "thread.h"
#include "qdma_exports.h"
#include "qdma_access_common.h"
#include "qdma_resource_mgmt.h"
#include "interrupts.hpp"
#include "qdma_platform.h"

/* Xilinx namespace */
namespace xlnx {

#pragma pack(1)
struct version {
    UINT8 patch;
    UINT8 minor;
    UINT16 major;

    constexpr version(const UINT16 major_, const UINT8 minor_, const UINT8 patch_)
        : patch(patch_), minor(minor_), major(major_) {}

    constexpr UINT32 hash() const {
        return UINT32((major << 16) + (minor << 8) + patch);
    }

    constexpr bool operator==(const version& other) const {
        return hash() == other.hash();
    }

    constexpr bool operator!=(const version& other) const {
        return !(*this == other);
    }
};
#pragma pack()

/* ensure that version is evaluated at compile time */
static_assert(version(65535, 0, 0).major == 65535 &&
    version(0, 255, 0).minor == 255 &&
    version(0, 0, 255).patch == 255, "incorrect bits in version");
static_assert(sizeof(version) == sizeof(UINT32), "size of version struct is not 32bits");

static_assert(version(0x1234, 0x56, 0x78).hash() == 0x12345678, "error in version hash function");

static_assert(version(1, 2, 3) == version(1, 2, 3), "error in version equality comparison");
static_assert(version(1, 2, 3) != version(1, 2, 0), "error in version equality comparison");
static_assert(version(1, 2, 3) != version(1, 0, 3), "error in version equality comparison");
static_assert(version(1, 2, 3) != version(0, 2, 3), "error in version equality comparison");



/** libqdma version number */
static constexpr version qdma_version = version(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION, VER_PRODUCTREVISION);

static constexpr size_t sg_frag_len = 61440; /** 15 * 4K Fragment size */
static constexpr size_t st_max_desc_data_len = 65535;
static constexpr size_t mm_max_desc_data_len  = 65535;

static constexpr unsigned int mm_h2c_completion_weight = 2048;
static constexpr unsigned int mm_c2h_completion_weight = 2048;
static constexpr unsigned int st_h2c_completion_weight = 2048;
static constexpr unsigned int st_c2h_completion_weight = 2048;

static constexpr unsigned int max_req_service_cnt = 10;

/**
 * Structure to hold the driver name and mode
 */
struct drv_mode_name {
    /**  Mode of the function */
    queue_op_mode mode;
    /**  Driver Name */
    char name[20];
};

class qdma_device;
struct queue_pair;

#define IS_LIST_EMPTY(list_head) \
    IsListEmpty(list_head)

#define INIT_LIST_HEAD(list_head) \
    InitializeListHead(list_head)

#define LIST_ADD_HEAD(list_head, entry) \
    InsertHeadList(list_head, entry)

#define LIST_ADD_TAIL(list_head, entry) \
    InsertTailList(list_head, entry)

#define LIST_DEL_NODE(entry) \
    RemoveEntryList(entry)

#define LIST_FOR_EACH_ENTRY(list_head, entry)   \
    for ((entry)= (list_head)->Flink; (entry) != (list_head); (entry) = (entry)->Flink)

#define LIST_FOR_EACH_ENTRY_SAFE(list_head, n, entry)   \
    for ((entry) = (list_head)->Flink, (n) = (entry)->Flink; (entry) != (list_head); (entry) = (n), (n) = (entry)->Flink)

#define LIST_GET_ENTRY(entry, type, member)  \
    CONTAINING_RECORD(entry, type, member)

/** queue_type - QDMA queue type */
enum class queue_type {
    /** Memory mapped queue type */
    MEMORY_MAPPED,
    /** Streaming queue type */
    STREAMING,
    /** Invalid queue type */
    NONE
};

/** device_state - State of the QDMA device */
enum device_state {
    /** Device is in Init State */
    DEVICE_INIT,
    /** Device is offline */
    DEVICE_OFFLINE,
    /** Device is online */
    DEVICE_ONLINE,
};

/**
 *    Maxinum length of the QDMA device name
 */
#define QDMA_DEV_NAME_MAXLEN    32

 /**
  * QDMA_QUEUE_NAME_MAXLEN - Maximum queue name length
  */
#define QDMA_QUEUE_NAME_MAXLEN	32

 /**
  * qdma_dev_conf - defines the per-device qdma property.
  */
struct qdma_dev_conf {
    /** PCIe Segment-Bus-Device-Function of QDMA device */
    union pci_sbdf dev_sbdf;
    /** Indicates whether this QDMA device is master */
    UINT32  is_master_pf : 1;
    /** Reserved Field, not being used */
    UINT32  reserved02 : 31;
    /** The name of the QDMA device, in 'qdma<BBDDF>' format */
    char    name[QDMA_DEV_NAME_MAXLEN];
    /** Attributes of the QDMA device specifies the supported features */
    struct qdma_dev_attributes dev_info;
};

/**
 * st_c2h_pkt_frag_queue - Structure to manage streamed
 *                         c2h packet fragments.
 */
struct st_c2h_pkt_frag_queue {
    st_c2h_pkt_fragment *frags = nullptr;
    UINT32 avail_frag_cnt = 0;
    size_t avail_byte_cnt = 0;
    /** Maximum size of the queue,
      * (max_q_size - 1) packets will fit in this queue
      */
    UINT32 max_q_size;
    UINT32 pidx;
    UINT32 cidx;

    NTSTATUS create(UINT32 entries);
    void destroy(void);
    NTSTATUS add(st_c2h_pkt_fragment &elem);
    NTSTATUS consume(st_c2h_pkt_fragment &elem);
    PFORCEINLINE bool is_queue_full(void);
    PFORCEINLINE bool is_queue_empty(void);
    inline LONG get_avail_frag_cnt(void);
    inline size_t get_avail_byte_cnt(void);
};

struct ring_buffer {
    UINT32 capacity; /* NOT including status write-back */

    WDFCOMMONBUFFER buffer = nullptr;
    void *buffer_va = nullptr;
    volatile wb_status_base* wb_status = nullptr;

    volatile UINT32 sw_index = 0;   /* usage: e.g. [1] pidx for MM C2H, MM H2C, ST H2C. [2] cidx for ST WB */
    volatile UINT32 hw_index = 0;   /* usage: e.g. [1] cidx for MM C2H, MM H2C, ST H2C. [2] pidx for ST WB */

    struct {
        UINT32 tot_desc_accepted;
        UINT32 tot_desc_processed;
    }stats;

    PFORCEINLINE void advance_idx(volatile UINT32& idx);
    PFORCEINLINE void advance_idx(volatile UINT32& idx, UINT32 num);
    PFORCEINLINE UINT32 idx_delta(UINT32 start_idx, UINT32 end_idx);

    NTSTATUS create(WDFDMAENABLER& dma_enabler, UINT32 num_entries, size_t desc_sz_in_bytes);
    void destroy(void);
    UINT32 get_num_free_entries(void);
    void *get_va(void);
    UINT32 get_capacity(void);
};

/** libqdma_queue_config -- This structure holds
  * the QDMA configuration parameters which are
  * deduced from the user requested while adding the queue
  */
struct libqdma_queue_config {
    /** Flag indicates the interrupt enabled or not */
    bool irq_en;
    /** Holds the vector id (MSIX/Legacy) assigned for the queue */
    UINT32 vector_id;
    /** Holds the descriptor size of the ring */
    qdma_desc_sz desc_sz;
    /** Holds the size of the descriptor ring (i.e., no of entries) */
    UINT32 ring_sz;
    /** Holds the size of the completion ring (i.e., no of entries) */
    UINT32 cmpt_ring_sz;
    /** Holds the index of the completion ring (one of the Global CSR register index) */
    UINT32 cmpt_ring_id;
    /** Holds the size of the single data buffer (Valid for ST C2H only) */
    UINT32 data_buf_size;
};

/** req_ctx -- Stores the request completion parameters */
struct req_ctx {
    /** User supplied private data */
    void *priv;
    /** Completion callback routine */
    dma_completion_cb compl_cb;
};

/**  dma_req_tracker -- This is a shadow ring tracker for
  *  MM and ST H2C transfers. This structure holds the data
  *  required for making request completions
  *
  *  This shadow ring tracker is used to avoid the
  *  use of synchronization primitives for the performance
  *  cost
  */
struct dma_req_tracker {
    /** Holds the completion parameters for user requests */
    req_ctx *requests = nullptr;

    /** Allocates and initializes the request tracker */
    NTSTATUS create(UINT32 entries);
    /** Frees the memory allocated for request tracker */
    void destroy();
};

/** st_c2h_req -- Stores the ST C2H request completion parameters */
struct st_c2h_req {
    /** Length of the packet requested */
    size_t len;
    /** User supplied private data */
    void *priv;
    /** Completion callback routine */
    st_rx_completion_cb st_compl_cb;
};

/** st_c2h_dma_req_tracker -- This structure holds the user requests
  * for ST C2H direction. This tracker has the same size as descriptor
  * ring (i.e., no of entries)
  */
struct st_c2h_dma_req_tracker {
    st_c2h_req *requests = nullptr;

    /* This lock is to ensure enqueueing/adding
       ST C2H requests properly to request tracker and
       dequeueing/popping the requests during completions
    */
    WDFSPINLOCK lock = nullptr;

    /** The size of this request tracker (no of entries)
      * This tracker is a bounded queue with max capacity specified
      * during initialization by the caller.
      */
    UINT32 capacity;
    UINT32 pidx;
    UINT32 cidx;

    NTSTATUS create(UINT32 entries);
    void destroy(void);

    NTSTATUS st_push_dma_req(st_c2h_req& req);
    NTSTATUS st_peek_dma_req(st_c2h_req& req);
    NTSTATUS st_pop_dma_req(void);
};

struct st_c2h_pkt_buffer {
    WDFCOMMONBUFFER rx_buff_common;
    PVOID rx_buff_va;
    PHYSICAL_ADDRESS rx_buff_dma;

    NTSTATUS create(WDFDMAENABLER dma_enabler, size_t size);
    void destroy(void);
    void fill_rx_buff(PVOID buff_va, PHYSICAL_ADDRESS buff_dma);
    PHYSICAL_ADDRESS get_dma_addr(void);
    PVOID get_va(void);
};

struct dma_request {
    /** Linked list entry to form request queue */
    LIST_ENTRY              list_entry;
    /** DMA Mode (ST/MM) */
    bool                    is_st;
    /** Direction of DMA */
    WDF_DMA_DIRECTION       direction;
    /** SG list of request */
    PSCATTER_GATHER_LIST    sg_list;
    /** Completion callback handler */
    dma_completion_cb       compl_cb;
    /** Private data to pass during completion callback */
    VOID                    *priv;
    /** The device address to/from DMA
     *  (Only Valid for MM transfers) */
    LONGLONG                device_offset;
    /** Holds the next index to resume
      * request transfer for split request */
    UINT32                  sg_index;
    /** Holds the next device offset to resume
      * request transfer for split request
      * (Only valid for MM transfers ) */
    LONGLONG                offset_idx;
};

struct h2c_queue {
    queue_config user_conf;
    libqdma_queue_config lib_config;

    ring_buffer desc_ring;
    dma_req_tracker req_tracker;

    /** This forms a chain of h2c requests */
    LIST_ENTRY req_list_head;

    /* This lock is to ensure enqueueing/adding
       the requests to the descriptor ring properly

       During completions this lock is not needed and
       request tracker design will make sure proper execution
    */
    WDFSPINLOCK lock = nullptr;
    poll_operation_entry *req_proc_entry;
    poll_operation_entry *poll_entry;
    qdma_q_pidx_reg_info csr_pidx_info;

    NTSTATUS create(qdma_device *qdma, queue_config& conf);
    void destroy(void);
    void init_csr_h2c_pidx_info(void);
};

struct c2h_queue {
    queue_config user_conf;
    libqdma_queue_config lib_config;

    ring_buffer desc_ring;
    dma_req_tracker req_tracker;

    /** This forms a chain of c2h requests */
    LIST_ENTRY req_list_head;

    /* This lock is to ensure MM enqueueing/adding
       the requests to the descriptor ring properly

       For completions of MM requests: lock is not needed
       request tracker design takes care
    */
    WDFSPINLOCK lock = nullptr;
    poll_operation_entry *req_proc_entry;
    poll_operation_entry *poll_entry;
    qdma_q_pidx_reg_info csr_pidx_info;
    qdma_q_cmpt_cidx_reg_info csr_cmpt_cidx_info;
    bool is_cmpt_valid = false;
    UINT32 cmpt_offset = 0;
    ring_buffer cmpt_ring;

    /* Only for streaming */
    st_c2h_dma_req_tracker st_c2h_req_tracker;
    UINT32 no_allocated_rx_common_buffs = 0;
    st_c2h_pkt_buffer *pkt_buffer = nullptr;
    st_c2h_pkt_fragment *pkt_frag_list = nullptr;
    st_c2h_pkt_frag_queue pkt_frag_queue;

    NTSTATUS create(qdma_device *qdma, queue_config& conf);
    void destroy(void);
    INT32 get_cmpt_ring_index(UINT32* csr_ring_sz_table, UINT32 desc_ring_index);
    void init_csr_c2h_pidx_info(void);
    void init_csr_cmpt_cidx_info(void);
};

enum class service_status {
    SERVICE_CONTINUE,
    SERVICE_FINISHED,
    SERVICE_ERROR,
};

struct queue_pair {
    LIST_ENTRY list_entry;
    qdma_device *qdma = nullptr;

    queue_type type;
    char name[QDMA_QUEUE_NAME_MAXLEN];
    volatile LONG state;

    UINT16 idx = 0;     /* queue index - relative to this PF */
    UINT16 idx_abs = 0; /* queue index - absolute across all PF */

    h2c_queue h2c_q;
    c2h_queue c2h_q;

    /** Initialization and De-Initialization functions */
    NTSTATUS create(queue_config& conf);
    void destroy(void);
    void init_csr(void);

    /** CSR update functions */
    PFORCEINLINE void update_sw_index_with_csr_wb(UINT32 new_cidx);
    PFORCEINLINE void update_sw_index_with_csr_h2c_pidx(UINT32 new_pidx);
    PFORCEINLINE void update_sw_index_with_csr_c2h_pidx(UINT32 new_pidx);

    /** Transfer initiate functions */
    NTSTATUS enqueue_dma_request(dma_request *request);
    NTSTATUS enqueue_dma_request(size_t length, st_rx_completion_cb compl_cb, void *priv);

    /** Transfer processing functions */
    service_status process_mm_request(dma_request* request, size_t* xfer_len);
    service_status process_st_h2c_request(dma_request* request, size_t* xfer_len);
    NTSTATUS process_st_c2h_data_pkt(void* udd_ptr, const UINT32 length);

    /** Transfer completion functions */
    service_status service_mm_st_h2c_completions(ring_buffer *desc_ring, dma_req_tracker *tracker, UINT32 budget, UINT32& proc_desc_cnt);
    service_status st_service_c2h_queue(UINT32 budget);

    PFORCEINLINE void update_c2h_pidx_in_batch(UINT32 processed_desc_cnt);
    NTSTATUS check_cmpt_error(c2h_wb_header_8B *cmpt_data);

    /** User-Defined Data(Side band data) funtions */
    NTSTATUS read_st_udd_data(void *addr, UINT8 *buf, UINT32 *len);
    void * get_last_udd_addr(void);

    /** Reads MM Completion ring (UDD) data,
      * This is independent ring and not related to data packets */
    NTSTATUS read_mm_cmpt_data(qdma_cmpt_info *cmpt_info);

    /** Cleanup related functions */
    void flush_queue(void);
    void cancel_mm_st_h2c_pending_requests(ring_buffer* desc_ring, dma_req_tracker* tracker, WDFSPINLOCK lock);

    /** Debug Functions */
    NTSTATUS desc_dump(qdma_desc_info *desc_info);
};

/**
 * qdma_device - Master class for QDMA device
 *
 * This class defines the interface structures/unions/classes
 * and interface functions that can be used while using
 * the libqdma library
 */
class qdma_device : public qdma_interface {
public:
    /** Spinlock to serialize all the HW registers */
    WDFSPINLOCK register_access_lock = nullptr;
    /** Structure that provides set of call back functions to access hardware */
    qdma_hw_access hw;
    /** Structure that provides QDMA device properties */
    qdma_dev_conf dev_conf;
    /** Structure that contains QDMA global CSR registers information */
    qdma_glbl_csr_conf csr_conf;
    /** Structure that contains QDMA driver configuration */
    qdma_drv_config drv_conf;

    /** DMA Initialization/Teardown APIs */
    NTSTATUS init(qdma_drv_config conf);
    NTSTATUS open(WDFDEVICE device, WDFCMRESLIST resources, WDFCMRESLIST resources_translated);
    void close(void);
    bool qdma_is_device_online(void);

    /** PCIe BAR Read and Write APIs */
    NTSTATUS read_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size);
    NTSTATUS write_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size);
    ULONG qdma_conf_reg_read(size_t offset);
    void qdma_conf_reg_write(size_t offset, ULONG data);
    NTSTATUS get_bar_info(qdma_bar_type bar_type, PVOID &bar_base, size_t &bar_length);

    /** Queue Configuration APIs (Add, Start Stop, Delete, state) */
    NTSTATUS qdma_add_queue(UINT16 qid, queue_config& conf);
    NTSTATUS qdma_start_queue(UINT16 qid);
    NTSTATUS qdma_stop_queue(UINT16 qid);
    NTSTATUS qdma_remove_queue(UINT16 qid);
    NTSTATUS qdma_is_queue_in_range(UINT16 qid);
    NTSTATUS qdma_get_queues_state(UINT16 qid, enum queue_state *qstate, CHAR *str, size_t str_maxlen);
    NTSTATUS qdma_set_qmax(UINT32 queue_max);

    /** DMA transfer APIs (From Device and To Device) */
    NTSTATUS qdma_enqueue_mm_request(UINT16 qid, WDF_DMA_DIRECTION direction, PSCATTER_GATHER_LIST sg_list, LONGLONG device_offset, dma_completion_cb compl_cb, VOID *priv);
    NTSTATUS qdma_enqueue_st_tx_request(UINT16 qid, PSCATTER_GATHER_LIST sg_list, dma_completion_cb compl_cb, VOID *priv);
    NTSTATUS qdma_enqueue_st_rx_request(UINT16 qid, size_t length, st_rx_completion_cb compl_cb, VOID *priv);

    /** DMA Completion ring APIs */
    NTSTATUS qdma_retrieve_st_udd_data(UINT16 qid, void *addr, UINT8 *buf, UINT32 *len);
    NTSTATUS qdma_retrieve_last_st_udd_data(UINT16 qid, UINT8 *buf, UINT32 *len);
    NTSTATUS qdma_read_mm_cmpt_data(qdma_cmpt_info *cmpt_info);

    /** DMA Configuration Read APIs */
    NTSTATUS qdma_read_csr_conf(qdma_glbl_csr_conf *conf);
    NTSTATUS qdma_get_dev_capabilities_info(qdma_device_attributes_info &dev_attr);
    NTSTATUS qdma_queue_context_read(UINT16 qid, enum qdma_dev_q_type ctx_type, struct qdma_descq_context *ctxt);

    /** DMA Configuration dump APIs */
    NTSTATUS qdma_queue_desc_dump(qdma_desc_info *desc_info);
    NTSTATUS qdma_queue_dump_context(qdma_ctx_info *ctx_info);
    NTSTATUS qdma_intring_dump(qdma_intr_ring_info *intring_info);
    NTSTATUS qdma_regdump(qdma_reg_dump_info *regdump_info);
    NTSTATUS qdma_get_qstats_info(qdma_qstat_info &qstats);
    NTSTATUS qdma_get_reg_info(qdma_reg_info *reg_info);

    /** DMA Versioning APIs */
    NTSTATUS qdma_device_version_info(qdma_version_info &version_info);
    void qdma_get_hw_version_info(qdma_hw_version_info &version_info);

    /** Misc APIs */
    queue_pair *qdma_get_queue_pair_by_hwid(UINT16 qid_abs);
private:
    LIST_ENTRY list_entry;
    /** Identifier returned by resource manager */
    UINT32 dma_dev_index = 0;
    /** Start/base queue number for this device */
    INT32 qbase = -1;
    /** Device state */
    volatile LONG qdma_device_state;

    xpcie_device pcie;
    WDFDEVICE wdf_dev = nullptr;
    qdma_hw_version_info hw_version_info;
    queue_pair *queue_pairs = nullptr;
    interrupt_manager irq_mgr;
    thread_manager th_mgr;

    NTSTATUS validate_qconfig(queue_config& conf);

    /* QDMA Device(s) list helper functions */
    NTSTATUS list_add_qdma_device_and_set_gbl_csr(void);
    bool is_first_qdma_pf_device(void);
    void list_remove_qdma_device(void);

    /* Context programming */
    NTSTATUS clear_queue_contexts(bool is_c2h, UINT16 qid, qdma_hw_access_type context_op) const;
    NTSTATUS clear_cmpt_contexts(UINT16 qid, qdma_hw_access_type context_op) const;
    NTSTATUS clear_pfetch_contexts(UINT16 qid, qdma_hw_access_type context_op) const;
    NTSTATUS clear_contexts(queue_pair& q, bool invalidate = false) const;
    NTSTATUS queue_program(queue_pair& q);
    NTSTATUS set_h2c_ctx(queue_pair& q);
    NTSTATUS set_c2h_ctx(queue_pair& q);

    NTSTATUS init_qdma_global();
    NTSTATUS init_func();
    NTSTATUS assign_bar_types();
    NTSTATUS init_dma_queues();
    NTSTATUS init_interrupt_queues();
    NTSTATUS init_os_resources(WDFCMRESLIST resources, const WDFCMRESLIST resources_translated);
    NTSTATUS init_resource_manager();
    void inc_queue_pair_count(bool is_cmpt_valid);
    void dec_queue_pair_count(bool is_cmpt_valid);

    void destroy_dma_queues(void);
    void destroy_os_resources(void);
    void destroy_func(void);
    void destroy_resource_manager(void);

    NTSTATUS configure_irq(PQDMA_IRQ_CONTEXT irq_context, ULONG vec);
    NTSTATUS intr_setup(WDFCMRESLIST resources, const WDFCMRESLIST resources_translated);
    void intr_teardown(void);
    NTSTATUS setup_legacy_interrupt(WDFCMRESLIST resources, const WDFCMRESLIST resources_translated);
    int setup_legacy_vector(queue_pair& q);
    void clear_legacy_vector(queue_pair& q, UINT32 vector);
    NTSTATUS arrange_msix_vector_types(void);
    NTSTATUS setup_msix_interrupt(WDFCMRESLIST resources, const WDFCMRESLIST resources_translated);
    UINT32 alloc_msix_vector_position(queue_pair& q);
    int assign_interrupt_vector(queue_pair& q);
    void free_interrupt_vector(queue_pair& q, UINT32 vec_id);
    void free_msix_vector_position(queue_pair& q, UINT32 vector);
    void mask_msi_entry(UINT32 vector);
    void unmask_msi_entry(UINT32 vector);
    NTSTATUS qdma_intr_context_read(UINT8 ring_idx_abs, struct qdma_indirect_intr_ctxt *ctxt);
};

typedef struct devices_list {
    qdma_device *qdma_dev;
    devices_list *next;
}devices_list, *pdevices_list;

} /* namespace xlnx */
