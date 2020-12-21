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

#include <pshpack8.h>

/**
 * @file
 * @brief This file contains the declarations for libqdma interface structures
 *
 */

/* Xilinx namespace */
namespace xlnx {

class qdma_interface;
struct st_c2h_pkt_fragment;

/**
 * enum qdma_q_type: Q type
 */
enum qdma_q_type {
    /** @QDMA_Q_TYPE_H2C: H2C Q */
    QDMA_Q_TYPE_H2C,
    /** @QDMA_Q_TYPE_C2H: C2H Q */
    QDMA_Q_TYPE_C2H,
    /** @QDMA_Q_TYPE_CMPT: CMPT Q */
    QDMA_Q_TYPE_CMPT,
    /** @QDMA_Q_TYPE_MAX: Total Q types */
    QDMA_Q_TYPE_MAX
};

/** queue_state - State of the QDMA queue */
enum queue_state {
    /** Queue is available to configure */
    QUEUE_AVAILABLE,
    /** Queue is added with resources */
    QUEUE_ADDED,
    /** Queue is programmed and started */
    QUEUE_STARTED,
    /** Queue critical operation is in progress */
    QUEUE_BUSY,
    /** Invalid Queue State */
    QUEUE_INVALID_STATE
};

/** Streaming card to host packet type */
enum class st_c2h_pkt_type {
    /** C2H DATA Packet MACRO */
    ST_C2H_DATA_PKT,
    /** C2H USER DEFINED DATA Packet MACRO */
    ST_C2H_UDD_ONLY_PKT,
};

/** qdma_queue_dir - QDMA Queue Direction */
enum class qdma_queue_dir {
    /** Queue H2C Direction */
    QDMA_QUEUE_DIR_H2C = 0,
    /** Queue C2H Direction */
    QDMA_QUEUE_DIR_C2H
};

/** RING DESCRIPTOR TYPE */
enum class qdma_desc_type {
    /* Descriptor of type descriptor ring */
    RING_DESCRIPTOR = 0,
    /* Descriptor of type completion ring */
    CMPT_DESCRIPTOR
};

/** DESCRIPTOR SIZE IN BYTES */
enum class qdma_desc_sz {
    /* Descriptor size of 8 bytes */
    QDMA_DESC_SZ_8B = 0,
    /* Descriptor size of 16 bytes */
    QDMA_DESC_SZ_16B,
    /* Descriptor size of 32 bytes */
    QDMA_DESC_SZ_32B,
    /* Descriptor size of 64 bytes */
    QDMA_DESC_SZ_64B,
    /* Descriptor size of invalid size */
    QDMA_DESC_SZ_MAX
};

/** Completion status trigger mode */
enum class qdma_trig_mode {
    /* Trigger disabled */
    QDMA_TRIG_MODE_DISABLE = 0,
    /* Trigger on every update */
    QDMA_TRIG_MODE_EVERY,
    /* Trigger on user count */
    QDMA_TRIG_MODE_USER_COUNT,
    /* Trigger on user event */
    QDMA_TRIG_MODE_USER,
    /* Trigger on user timer */
    QDMA_TRIG_MODE_USER_TIMER,
    /* Trigger on user timer and count */
    QDMA_TRIG_MODE_USER_TIMER_COUNT,
    /* Trigger invalid */
    QDMA_TRIG_MODE_MAX
};

/** queue_op_mode - QDMA Operation Mode */
enum queue_op_mode {
    /** Poll Mode */
    POLL_MODE = 0,
    /** Direct Interrupt Mode */
    INTR_MODE,
    /** Indirect Interrupt Mode(Aggregation) */
    INTR_COAL_MODE
};

/** qdma_bar_type - QDMA PCIe BAR Types */
enum class qdma_bar_type {
    /** QDMA Configuration BAR
     *  (Contains all QDMA configuration Registers)
     */
    CONFIG_BAR,
    /** QDMA AXI Master Lite BAR
     *  (Contains User Logic Registers)
     */
    USER_BAR,
    /** QDMA AXI Bridge Master BAR
     * (Contains Bypass Registers to bypass QDMA)
     */
    BYPASS_BAR
};

/** Specifies No of CSR Global Registers avalable */
static constexpr size_t QDMA_GLBL_CSR_REG_CNT = 16;

/** qdma_glbl_csr_conf - Provides the available CSR
 *                       connfiguration registers
 */
struct qdma_glbl_csr_conf {
    /** Ring Size CSR Registers */
    UINT32 ring_sz[QDMA_GLBL_CSR_REG_CNT];
    /** C2H Timer Count CSR Registers */
    UINT32 c2h_timer_cnt[QDMA_GLBL_CSR_REG_CNT];
    /** C2H Threshold CSR Registers */
    UINT32 c2h_th_cnt[QDMA_GLBL_CSR_REG_CNT];
    /** C2H Buffer size CSR Registers */
    UINT32 c2h_buff_sz[QDMA_GLBL_CSR_REG_CNT];
    /** Writeback interval timeout register */
    UINT32 wb_interval;
};

/** 
 * dma_completion_cb() - DMA Request completion callback function type
 *
 * @param[in]   priv: Driver provided private member
 * @param[in]   status: DMA Request completion status
 *
 * @return void
 */
using dma_completion_cb = void(*)(void *priv, NTSTATUS status);

/**
 * st_rx_completion_cb() - Streaming C2H DMA Request completion callback function type
 *
 * @param[in]   rx_frags: List of packet fragments
 * @param[in]   num_pkts: entries in rx_frags
 * @param[in]   priv: Driver provided private member
 * @param[in]   status: DMA Request completion status
 *
 * @return void
 */
using st_rx_completion_cb = void(*)(const st_c2h_pkt_fragment *rx_frags, size_t num_pkts, void *priv, NTSTATUS status);

/**
 * proc_st_udd_only_cb() - User defined data received callback function type
 *
 * @param[in]   qid: Queue number
 * @param[in]   udd_addr: UDD pointer
 * @param[in]   priv: Driver provided private member
 *
 * @return void
 */
using proc_st_udd_only_cb = void(*)(UINT16 qid, void *udd_addr, void *priv);

/**
 * fp_user_isr_handler() - User defined user ISR handler
 *
 * @param[in]   event_id: Event identifier
 * @param[in]   user_data: Driver provided user data
 *
 * @return void
 */
using fp_user_isr_handler = void(*)(ULONG event_id, void *user_data);

/**
 * fp_user_interrupt_enable_handler() - User defined user ISR handler
 *
 * @param[in]   event_id: Event identifier
 * @param[in]   user_data: Driver provided user data
 *
 * @return void
 */
using fp_user_interrupt_enable_handler = void(*)(ULONG event_id, void *user_data);

/**
 * fp_user_interrupt_disable_handler() - User defined user ISR handler
 *
 * @param[in]   event_id: Event identifier
 * @param[in]   user_data: Driver provided user data
 *
 * @return void
 */
using fp_user_interrupt_disable_handler = void(*)(ULONG event_id, void *user_data);

/** dev_config - qdma device configuration
 *               needed to initialize the device
 */
struct qdma_drv_config {
    /* Queue operation mode */
    queue_op_mode operation_mode;

    /* Config BAR index */
    UINT8 cfg_bar;

    /* Maximum queues for the device */
    UINT32 qsets_max;

    /* Maximum user MSIx vector to use */
    UINT16 user_msix_max;

    /* Maximum data MSIx vector to use */
    UINT16 data_msix_max;

    /* User data for user interrupt callback functions */
    void *user_data;

    /* User ISR handler */
    fp_user_isr_handler user_isr_handler;

    /* User interrupt enable handler */
    fp_user_interrupt_enable_handler user_interrupt_enable_handler;

    /* User interrupt disable handler */
    fp_user_interrupt_disable_handler user_interrupt_disable_handler;
};

/** queue_config - qdma queue configuration
 *                 needed to add a queue
 */
struct queue_config {
    /** queue is ST or MM */
    bool is_st;
    /** H2C ring size index */
    UINT8 h2c_ring_sz_index;
    /** C2H ring size index */
    UINT8 c2h_ring_sz_index;
    /** C2H buffer size index */
    UINT8 c2h_buff_sz_index;
    /** C2H threshold index */
    UINT8 c2h_th_cnt_index;
    /** C2H timer count index */
    UINT8 c2h_timer_cnt_index;
    /** completion ring size */
    qdma_desc_sz cmpt_sz;
    /** trigger mode (valid for ST C2H) */
    qdma_trig_mode trig_mode;
    /** Software descriptor size */
    UINT8 sw_desc_sz;
    /** descriptor bypass enabled or not */
    bool desc_bypass_en;
    /** prefetch enabled or not */
    bool pfch_en;
    /** prefetch bypass enabled or not */
    bool pfch_bypass_en;
    /** Completion overflow disabled or not */
    bool cmpl_ovf_dis;
    /** MM completion enabled or not */
    bool en_mm_cmpl;
    /** ST UDD data only call back function */
    proc_st_udd_only_cb proc_st_udd_cb;
};

/** st_c2h_pkt_fragment - C2H Packet Details */
struct st_c2h_pkt_fragment {
    /** C2H Data Fragment Address */
    void    *data = nullptr;
    /** User Defined Data Address */
    void    *udd_data = nullptr;
    /** Length of the C2H data Fragment */
    size_t  length = 0;
    /** Indicates Start of packet */
    UINT32  sop      : 1;
    /** Indicates End of packet */
    UINT32  eop      : 1;
    /** Indicates Packet Type (ST_C2H_DATA_PKT or ST_C2H_UDD_ONLY_PKT) */
    st_c2h_pkt_type  pkt_type : 1;
    /** Reserved Field */
    UINT32  reserved : 29;
};

/** qdma_device_attributes_info - Device Attributes/Features */
struct qdma_device_attributes_info {
    /** No of Physical functions supported */
    UINT8 num_pfs;
    /** No of queues supported */
    UINT16 num_qs;
    /** Function Level Reset Status */
    bool flr_present;
    /** Streaming Feature enabled */
    bool st_en;
    /** Memory Mapped Feature enabled */
    bool mm_en;
    /** MM Completion Feature enabled */
    bool mm_cmpl_en;
    /** Mailbox Feature enabled */
    bool mailbox_en;
    /** Debug mode is enabled/disabled for IP */
    bool debug_mode;
    /** Descriptor Engine mode:
     *  Internal only/Bypass only/Internal & Bypass
     */
    UINT8 desc_eng_mode;
    /** Number of MM channels supported */
    UINT16 num_mm_channels;
};

#define DEVICE_VERSION_INFO_STR_LENGTH            32
/** qdma_version_info - QDMA HW and SW version information */
struct qdma_version_info {
    /** Version string */
    char qdma_rtl_version_str[DEVICE_VERSION_INFO_STR_LENGTH];
    /** Release string */
    char qdma_vivado_release_id_str[DEVICE_VERSION_INFO_STR_LENGTH];
    /** Qdma device type string*/
    char qdma_device_type_str[DEVICE_VERSION_INFO_STR_LENGTH];
    /** Versal IP state string*/
    char qdma_versal_ip_type_str[DEVICE_VERSION_INFO_STR_LENGTH];
    /** QDMA SW Version string*/
    char qdma_sw_version_str[DEVICE_VERSION_INFO_STR_LENGTH];
};

/** qdma_desc_info - Structure contains required members to
 *  retrieve descriptor information
 *
 *  The QDMA queue descriptor information will be copied into
 *  <b><em>pbuffer</em></b> for the requested <b><em>qid</em></b>,
 *  <b><em>dir</em></b>, <b><em>desc_type</em></b> from
 *  <b><em>desc_start</em></b> to <b><em>desc_end</em></b> into
 *  <b><em>pbuffer</em></b>.
 *
 *  Caller must allocate memory for <b><em>pbuffer</em></b> and
 *  the size must be specified in <b><em>buff_sz</em></b>
 *
 *  The driver specifies the actual length of data copied
 *  into <b><em>pbuffer</em></b> in <b><em>data_sz</em></b> field
 *  along with descriptor size in <b><em>desc_sz</em></b>.
 *
 */
struct qdma_desc_info {
    /** QID */
    UINT16 qid;
    /** Q Direction (H2C, C2H) */
    qdma_queue_dir dir;
    /** Descriptor Type (Ring Descriptor, Completion Descriptor) */
    qdma_desc_type   desc_type;
    /** Descriptor Start Index */
    UINT32 desc_start;
    /** Descriptor End Index */
    UINT32 desc_end;
    /** Data Buffer available size */
    size_t  buffer_sz;
    /** Data Buffer */
    UINT8   *pbuffer;
    /** Size of the descriptor */
    size_t  desc_sz;
    /** Size of the data returned */
    size_t  data_sz;
};

/** qdma_cmpt_info - Structure contains required members to
 *  retrieve completion ring information
 *
 *  The QDMA MM completion ring data will be copied into
 *  <b><em>pbuffer</em></b> for the requested <b><em>qid</em></b>
 *
 *  Caller must allocate memory for <b><em>pbuffer</em></b> and
 *  the size must be specified in <b><em>buf_len</em></b>
 *
 *  The driver specifies the actual length of data copied
 *  into <b><em>pbuffer</em></b> in <b><em>ret_len</em></b> field
 *  along with completion descriptor size in <b><em>cmpt_desc_sz</em></b>.
 *
 */
struct qdma_cmpt_info {
    /** QID */
    UINT16 qid;
    /** Data Buffer available size */
    size_t  buffer_len;
    /** Returned Data size */
    size_t  ret_len;
    /** Completion Descriptor size */
    size_t  cmpt_desc_sz;
    /** Buffer to hold Completion Descriptors */
    UINT8 *pbuffer;
};

/** qdma_ctx_info - Structure contains required members to
 *  retrieve QDMA queue context information
 *
 *  The queue context information will be copied into
 *  <b><em>pbuffer</em></b> for the requested <b><em>qid</em></b>
 *  in the requested <b><em>dir</em></b>
 *
 *  Caller must allocate memory for <b><em>pbuffer</em></b> and
 *  the size must be specified in <b><em>buff_sz</em></b>
 *
 *  The driver specifies the actual length of data copied
 *  into <b><em>pbuffer</em></b> in <b><em>ret_sz</em></b> field.
 */
struct qdma_ctx_info {
    /** QID */
    UINT16  qid;
    /** Ring Type (H2C, C2H, CMPT) */
    enum qdma_q_type  ring_type;
    /** Data Buffer available size */
    size_t  buffer_sz;
    /** Returned Data size */
    size_t  ret_sz;
    /** Buffer to hold context information */
    char *pbuffer;
};

/** qdma_intr_ring_info - Structure contains required members to
 *  retrieve Interrupt ring information.
 *
 *  The interrupt ring information will be copied into
 *  <b><em>pbuffer</em></b> from <b><em>start_idx</em></b> to
 *  <b><em>end_idx</em></b> for the requested <b><em>vec_id</em></b>
 *
 *  Caller must allocate memory for <b><em>pbuffer</em></b> and
 *  the size must be specified in <b><em>buf_len</em></b>
 *
 *  The driver specifies the actual length of data copied
 *  into <b><em>pbuffer</em></b> in <b><em>ret_len</em></b> field.
 *
 *  The driver specifies the size of the ring entry in <b><em>ring_entry_sz</em></b>
 */
struct qdma_intr_ring_info {
    /** Interrupt Vector ID */
    UINT32 vec_id;
    /** Interrupt ring start index */
    UINT32  start_idx;
    /** Interrupt ring end index */
    UINT32  end_idx;
    /** Data Buffer available size */
    size_t  buffer_len;
    /** Returned Data size */
    size_t  ret_len;
    /** Size of the interrupt ring entry */
    size_t  ring_entry_sz;
    /** Buffer to hold interrupt ring information */
    unsigned char *pbuffer;
};

/** qdma_reg_dump_info - Structure contains required members to
 *  retrieve register dump information
 *
 *  All the QDMA registers and its contents will be copied into
 *  <b><em>pbuffer</em></b>
 *
 *  Caller must allocate memory for <b><em>pbuffer</em></b> and
 *  the size must be specified in <b><em>buf_len</em></b>
 *
 *  The driver specifies the actual length of data copied
 *  into <b><em>pbuffer</em></b> in <b><em>ret_len</em></b> field.
 */
struct qdma_reg_dump_info {
    /** Data Buffer available size */
    size_t  buffer_len;
    /** Returned Data size */
    size_t  ret_len;
    /** Buffer to hold regsiter dump information */
    char *pbuffer;
};

/** qdma_queue_stats - Structure contains required members to
 *  retrieve queue stats information
 *
 *  All the queue statistics for this function is copied.
 */
struct qdma_qstat_info {
    /** Queue base for this device */
    UINT32 qbase;
    /** Maximum allocated queues for this device */
    UINT32 qmax;
    /** Active host to card queues for this device */
    UINT32 active_h2c_queues;
    /** Active card to host queues for this device */
    UINT32 active_c2h_queues;
    /** Active completion queues for this device */
    UINT32 active_cmpt_queues;
};

/** qdma_reg_info - Structure contains required members to
 *  retrieve requested qdma registers information
 */
struct qdma_reg_info {
    /** PCIe bar number */
    UINT32  bar_no;
    /** Register address offset */
    UINT32  address;
    /** number of registers to retrieve */
    UINT32  reg_cnt;
    /** Length of the buffer pointed by pbuffer */
    size_t  buf_len;
    /** Length of the data present in pbuffer */
    size_t  ret_len;
    /** output buffer to copy the register info */
    char    *pbuffer;
};

/**
 * qdma_interface - libqdma interface class
 *
 * This class defines the interfaces that can be used while using
 * the libqdma library.
 */
class qdma_interface {
public:
    /** Handle to WDF DMA Enabler object to enable and start DMA operations */
    WDFDMAENABLER dma_enabler = nullptr;

    /*****************************************************************************/
    /**
     * init() - Initializes the qdma device with operation mode and
     *          config bar number to use
     *
     * @param[in]   conf: Device operation configuration
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS init(qdma_drv_config conf) = 0;

    /*****************************************************************************/
    /**
     * open() - Prepares the qdma_device with all necessary information
     *          like allocating memory for resources, BAR mappings,
     *          Initializes the hardware structure, Initializes queues etc.,
     *
     * @param[in]   device:               WDF Device
     * @param[in]   resources:            Handle to WDF Framework raw
     *                                    HW resources list
     * @param[in]   resources_translated: Handle to WDF Framework translated
     *                                    HW resources list
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS open(WDFDEVICE device, WDFCMRESLIST resources, WDFCMRESLIST resources_translated) = 0;

    /*****************************************************************************/
    /**
     * close() - Closes the QDMA device by freeing memory for resources,
     *           unmapping BARs, De-initializes the hardware structure,
     *           De-initializes queues etc.,
     *
     * @return  void
     *****************************************************************************/
    virtual void close(void) = 0;

    /*****************************************************************************/
    /**
     * qdma_is_device_online() - Checks if qdma device is ready for operations.
     *
     * @return  true when device is online state.
     *          false when device is offline state.
     *****************************************************************************/
    virtual bool qdma_is_device_online(void) = 0;

    /*****************************************************************************/
    /**
     * read_bar() - Performs PCIe read operation on specified BAR number at
     *              requested offset of requested size
     *
     * @param[in]   bar_type:   BAR Type
     * @param[in]   offset:     address offset to read the data from
     * @param[out]  data:       data buffer to store the read data
     * @param[in]   size:       size of the requested read operation(in bytes)
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS read_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size) = 0;

    /*****************************************************************************/
    /**
     * write_bar() - Performs PCIe write operation on specified BAR number at
     *               requested offset of requested size
     *
     * @param[in]   bar_type:   BAR Type
     * @param[in]   offset:     address offset to write the data to
     * @param[in]   data:       data buffer contains the data bytes to write
     * @param[in]   size:       size of the requested write operation(in bytes)
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS write_bar(qdma_bar_type bar_type, size_t offset, void* data, size_t size) = 0;

    /*****************************************************************************/
    /**
     * write_bar() - Performs PCIe write operation on specified BAR number at
     *               requested offset of requested size
     *
     * @param[in]   bar_type:   BAR Type
     * @param[out]  bar_base:   BAR base mapped address
     * @param[out]  bar_lenght: Bar length(in bytes)
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS get_bar_info(qdma_bar_type bar_type, PVOID &bar_base, size_t &bar_length) = 0;

    /*****************************************************************************/
    /**
     * qdma_get_queues_state() - Retrieves the state of the specified queue
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     * @param[out]  qstate: state of the queue specified as enumeration
     * @param[out]  state:  state of the queue specified as character string
     * @param[in]   sz:     size of the state character array
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_get_queues_state(UINT16 qid, enum queue_state *qstate, char *state, size_t sz) = 0;

    /*****************************************************************************/
    /**
     * qdma_add_queue() - Configures the specified queue addition process with
     *                    provided configuration
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     * @param[in]   conf:   configuration parameters for qid
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_add_queue(UINT16 qid, queue_config& conf) = 0;

    /*****************************************************************************/
    /**
     * qdma_start_queue() - Starts the configured queue(qid) and the queue will be
     *                      in operational state
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_start_queue(UINT16 qid) = 0;

    /*****************************************************************************/
    /**
     * qdma_stop_queue() - Stops the queue(qid) and the queue will be
     *                     in non operational state
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_stop_queue(UINT16 qid) = 0;

    /*****************************************************************************/
    /**
     * qdma_remove_queue() - Removes the queue(qid) configuration and
     *                       puts in available state for re-use
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_remove_queue(UINT16 qid) = 0;

    /*****************************************************************************/
    /**
     * qdma_is_queue_in_range() - Validate qid of a queue for this device
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_is_queue_in_range(UINT16 qid) = 0;

    /*****************************************************************************/
    /**
     * qdma_enqueue_mm_request() - enqueues an MM request into specified queue
     *
     * @param[in]   qid:            queue id relative to this QDMA device
     * @param[in]   direction:      DMA direction (read or write)
     * @param[in]   sg_list:        scatter-gather list of data buffers
     * @param[in]   device_offset:  Device address to write/from read
     * @param[in]   compl_cb:       completion call back function
     * @param[in]   priv:           private data that gets passed to
     *                              compl_cb function
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_enqueue_mm_request(UINT16 qid, WDF_DMA_DIRECTION direction, PSCATTER_GATHER_LIST sg_list, LONGLONG device_offset, dma_completion_cb compl_cb, VOID *priv) = 0;

    /*****************************************************************************/
    /**
     * qdma_enqueue_st_tx_request() - enqueues an ST write request into specified queue
     *
     * @param[in]       qid:        queue id relative to this QDMA device
     * @param[in]       sg_list:    scatter-gather list of data buffers
     * @param[in,out]   compl_cb:   completion call back function
     *                              to indicate write operation is completed
     * @param[in,out]   priv:       private data that gets passed to
     *                              compl_cb function
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_enqueue_st_tx_request(UINT16 qid, PSCATTER_GATHER_LIST sg_list, dma_completion_cb compl_cb, VOID *priv) = 0;

    /*****************************************************************************/
    /**
     * qdma_enqueue_st_rx_request() - enqueues an ST read request into specified queue
     *
     * @param[in]       qid:        queue id relative to this QDMA device
     * @param[in]       length:     desired data length to be received
     * @param[in,out]   compl_cb:   completion call back function
     *                              once data is available
     * @param[in,out]   priv:       private data that gets passed to
     *                              compl_cb function
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_enqueue_st_rx_request(UINT16 qid, size_t length, st_rx_completion_cb compl_cb, VOID *priv) = 0;

    /*****************************************************************************/
    /**
     * qdma_retrieve_st_udd_data() - Retrieves the User Defined Data(side band data)
     *                               from ST C2H descriptors
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     * @param[in]   addr:   UDD address in descriptor
     * @param[out]  buf:    buffer to store user defined data
     * @param[out]  len:    specifies the length of UDD in bytes
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_retrieve_st_udd_data(UINT16 qid, void *addr, UINT8 *buf, UINT32 *len) = 0;

    /*****************************************************************************/
    /**
     * qdma_retrieve_last_st_udd_data() - Retrieves the User Defined Data
     *                                   (side band data) from ST C2H descriptors
     *                                   that can be consumed in driver
     *
     * @param[in]   qid:    queue id relative to this QDMA device
     * @param[out]  buf:    buffer to store user defined data
     * @param[out]  len:    specifies the length of UDD in bytes
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_retrieve_last_st_udd_data(UINT16 qid, UINT8 *buf, UINT32 *len) = 0;

    /*****************************************************************************/
    /**
     * qdma_queue_desc_dump() - Retrieves the descriptors data into desc_info
     *
     * @param[in,out]   desc_info:    pointer to qdma_desc_info
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_queue_desc_dump(qdma_desc_info *desc_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_read_mm_cmpt_data() - Retrieves the User Defined Data(side band data)
     *                            from MM write back ring (if HW support available)
     *
     * @param[in,out]   cmpt_info:  user defined data(completion) information
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_read_mm_cmpt_data(qdma_cmpt_info *cmpt_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_queue_dump_context() - Dumps the queue context information of given
     *                             direction
     *
     * @param[in,out]   ctx_info:  context information structure
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_queue_dump_context(qdma_ctx_info *ctx_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_intring_dump() - Dumps the interrupt ring context information of given
     *                  vector ID from start index to end index
     *
     * @param[in,out]   intring_info:  interrupt ring context information
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_intring_dump(qdma_intr_ring_info *intring_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_regdump() - Dumps all the QDMA registers to given buffer
     *
     * @param[in,out]   regdump_info:  Register dump information
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_regdump(qdma_reg_dump_info *regdump_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_read_csr_conf() - Retrieves the CSR registers information
     *
     * @param[out]  conf:  CSR registers information
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_read_csr_conf(qdma_glbl_csr_conf *conf) = 0;

    /*****************************************************************************/
    /**
     * qdma_get_dev_capabilities_info() - Retrieves the HW device features info
     *
     * @param[out]  dev_attr:  HW device attributes
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_get_dev_capabilities_info(qdma_device_attributes_info &dev_attr) = 0;

    /*****************************************************************************/
    /**
     * qdma_device_version_info() - Retrieves the QDMA HW and SW versions
     *                              in character array format
     *
     * @param[out]  version_info:  HW & SW versions information in character
     *                             array format
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_device_version_info(qdma_version_info &version_info) = 0;

    /*****************************************************************************/
    /**
     * qdma_set_qmax() - Set maximum queues number for this QDMA function.
     *
     * @param[in]   qmax: Maximum number of queues for this function.
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_set_qmax(UINT32 qmax) = 0;

    /*****************************************************************************/
    /**
     * qdma_get_qstats_info() - Retrieves the QDMA statistics for queue resources
     *
     * @param[out]  qstats:  Total queues for function with occupied H2C, C2H and
     *                       CMPT queues count.
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_get_qstats_info(qdma_qstat_info &qstats) = 0;

    /*****************************************************************************/
    /**
     * qdma_get_reg_info() - Retrieves the requested QDMA registers information
     *
     * @param[out]  reg_info:  Register information (Address, Len, etc.,)
     *
     * @return  STATUS_SUCCESS for success else error
     *****************************************************************************/
    virtual NTSTATUS qdma_get_reg_info(qdma_reg_info* reg_info) = 0;

    /*****************************************************************************/
    /**
     * create_qdma_device() - Allocates an instance for qdma device
     *
     * @return  Address of qdma_interface class or NULL in case of error
     *****************************************************************************/
    static qdma_interface* create_qdma_device(void);

    /*****************************************************************************/
    /**
     * remove_qdma_device() - frees the allocated instance for qdma_device
     *
     * @param[in]   qdma_dev: address of the qdma_device instance
     *
     * @return  void
     *****************************************************************************/
    static void remove_qdma_device(qdma_interface *qdma_dev);

    /// @cond
    /*****************************************************************************/
    /**
     * new - Overloaded new operator to allocate the memory
     *
     * @param[in]   num_bytes: number of bytes of memory to allocate
     *
     * @return  void *
     *****************************************************************************/
    _IRQL_requires_max_(PASSIVE_LEVEL)
    void *operator new(_In_ size_t num_bytes);

    /*****************************************************************************/
    /**
     * delete - Overloaded delete operator to free the memory
     *
     * @param[in]   addr: address of the memory
     *
     * @return  void
     *****************************************************************************/
    _IRQL_requires_max_(PASSIVE_LEVEL)
    void operator delete(_In_ void *addr);

    /*****************************************************************************/
    /**
     * ~qdma_interface() - destructor
     *
     * @return  void
     *****************************************************************************/
    virtual ~qdma_interface() {}
    /// @endcond
};

}
#include <poppack.h>
