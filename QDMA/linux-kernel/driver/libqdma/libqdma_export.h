/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#ifndef __LIBQDMA_EXPORT_API_H__
#define __LIBQDMA_EXPORT_API_H__


/**
 * @file libqdma_export.h
 *
 * Xilinx QDMA Library Interface Definitions
 *
 * Header file *libqdma_export.h* defines data structures and function
 * signatures exported by Xilinx QDMA (libqdma) Library.
 * libqdma is part of Xilinx QDMA Linux Driver.
 *
 * libqdma exports the configuration and control APIs for device and
 * queue management and data processing APIs. Configuration APIs
 * shall not be invoked in interrupt context by upper layers.
 */

#include <linux/types.h>
#include <linux/interrupt.h>
#include "libqdma_config.h"
#include "qdma_access_export.h"


/** @defgroup libqdma_enums Enumerations
 */
/** @defgroup libqdma_struct Data Structures
 */
/**
 * @defgroup libqdma_defines Definitions
 * @{
 */

/**
 * Invalid QDMA function number
 */
#define QDMA_FUNC_ID_INVALID	0xFFFF

/**
 * QDMA_DEV_NAME_MAXLEN - Maxinum length of the QDMA device name
 */
#define QDMA_DEV_NAME_MAXLEN	32

/**
 * DEVICE_VERSION_INFO_STR_LENGTH - QDMA HW version string array length,
 * change this if QDMA_HW_VERSION_STRING_LEN is changed in access code
 */
#define DEVICE_VERSION_INFO_STR_LENGTH         (34)

/**
 * QDMA_QUEUE_NAME_MAXLEN - Maximum queue name length
 */
#define QDMA_QUEUE_NAME_MAXLEN	32

/**
 * QDMA_QUEUE_IDX_INVALID - Invalid queue index
 */
#define QDMA_QUEUE_IDX_INVALID	0xFFFF

/**
 * QDMA_QUEUE_VEC_INVALID - Invalid MSIx vector index
 */
#define QDMA_QUEUE_VEC_INVALID	0xFF

/**
 * QDMA_REQ_OPAQUE_SIZE - Maximum request length
 */
#define QDMA_REQ_OPAQUE_SIZE	80

/**
 * QDMA_UDD_MAXLEN - Maximum length of the user defined data
 */
#define QDMA_UDD_MAXLEN		32

/** @} */


/**
 * Per queue DMA AXI Interface option
 * @ingroup libqdma_enums
 */
enum qdma_q_mode {
	/** AXI Memory Mapped Mode */
	QDMA_Q_MODE_MM,
	/** AXI Stream Mode */
	QDMA_Q_MODE_ST
};

/**
 * Direction of the queue
 * @ingroup libqdma_enums
 */
enum qdma_q_dir {
	/** host to card */
	QDMA_Q_DIR_H2C,
	/** card to host */
	QDMA_Q_DIR_C2H
};


/**
 * PF/VF qdma driver modes
 *
 * QDMA PF/VF drivers can be loaded in one of these modes.
 * Mode options is exposed as a user configurable module parameter
 * @ingroup libqdma_enums
 *
 */
enum qdma_drv_mode {
	/**
	 *  @param AUTO_MODE Auto mode is mix of Poll and Interrupt Aggregation
	 *  mode. Driver polls for the write back status updates. Interrupt
	 *  aggregation is used for processing the completion ring
	 */
	AUTO_MODE,
	/**
	 *  @param POLL_MODE In Poll Mode, Software polls for the write back
	 *  completions (Status Descriptor Write Back)
	 */
	POLL_MODE,
	/**
	 *  @param DIRECT_INTR_MODE Direct Interrupt mode, each queue is
	 *  assigned to one of the available interrupt vectors in a round robin
	 *  fashion to service the requests. Interrupt is raised by the HW upon
	 *  receiving the completions and software reads the completion status.
	 */
	DIRECT_INTR_MODE,
	/**
	 *  @param INDIRECT_INTR_MODE In Indirect Interrupt mode or Interrupt
	 *  Aggregation mode, each vector has an associated Interrupt
	 *  Aggregation Ring. The QID and status of queues requiring service
	 *  are written into the Interrupt Aggregation Ring. When a PCIe MSI-X
	 *  interrupt is received by the Host, the software reads the Interrupt
	 *  Aggregation Ring to determine which queue needs service. Mapping of
	 *  queues to vectors is programmable
	 */
	INDIRECT_INTR_MODE,
	/**
	 *  @param LEGACY_INTR_MODE Driver is inserted in legacy interrupt mode
	 *  Software serves status updates upon receiving the legacy interrupt
	 */
	LEGACY_INTR_MODE
};

/**
 * Queue direction types
 * @ingroup libqdma_enums
 *
 */
enum queue_type_t {
	/** host to card */
	Q_H2C,
	/** card to host */
	Q_C2H,
	/** cmpt queue*/
	Q_CMPT,
	/** Both H2C and C2H directions*/
	Q_H2C_C2H,
};


/**
 * Qdma interrupt ring size selection
 *
 * Each interrupt vector can be associated with 1 or more interrupt rings.
 * The software can choose 8 different interrupt ring sizes. The ring size
 * for each vector is programmed during interrupt context programming
 * @ingroup libqdma_enums
 *
 */
enum intr_ring_size_sel {
	/**  accommodates 512 entries */
	INTR_RING_SZ_4KB = 0,
	/**  accommodates 1024 entries */
	INTR_RING_SZ_8KB,
	/**  accommodates 1536 entries */
	INTR_RING_SZ_12KB,
	/**  accommodates 2048 entries */
	INTR_RING_SZ_16KB,
	/**  accommodates 2560 entries */
	INTR_RING_SZ_20KB,
	/**  accommodates 3072 entries */
	INTR_RING_SZ_24KB,
	/**  accommodates 3584 entries */
	INTR_RING_SZ_28KB,
	/**  accommodates 4096 entries */
	INTR_RING_SZ_32KB,
};

/**
 * Qdma function states
 *
 * Each PF/VF device can be configured with 0 or more number of queues.
 * When the queue is not assigned to any function, function is in unfonfigured
 * state. Sysfs interface enables the users to configure the number of
 * queues to different functions. Upon adding the queues, function moves to
 * user configured state.
 * @ingroup libqdma_enums
 *
 */
enum qdma_dev_qmax_state {
	/** @param QMAX_CFG_UNCONFIGURED queue max not configured */
	QMAX_CFG_UNCONFIGURED,
	/**
	 *  @param QMAX_CFG_INITIAL queue max configured with
	 *  initial default values
	 */
	QMAX_CFG_INITIAL,
	/**
	 *  @param QMAX_CFG_USER queue max configured from
	 *  sysfs as per user choice
	 */
	QMAX_CFG_USER,
};

/**
 * Descriptor sizes
 * @ingroup libqdma_enums
 *
 */
enum cmpt_desc_sz_t {
	/** completion size 8B */
	CMPT_DESC_SZ_8B = 0,
	/** completion size 16B */
	CMPT_DESC_SZ_16B,
	/** completion size 32B */
	CMPT_DESC_SZ_32B,
	/** completion size 64B */
	CMPT_DESC_SZ_64B
};

/**
 * Descriptor sizes
 * @ingroup libqdma_enums
 *
 */
enum desc_sz_t {
	/**  descriptor size 8B */
	DESC_SZ_8B = 0,
	/**  descriptor size 16B */
	DESC_SZ_16B,
	/**  descriptor size 32B */
	DESC_SZ_32B,
	/**  descriptor size 64B */
	DESC_SZ_64B
};

/**
 * Trigger modes
 * @ingroup libqdma_enums
 *
 */
enum tigger_mode_t {
	/**  disable trigger mode */
	TRIG_MODE_DISABLE,
	/**  any trigger mode */
	TRIG_MODE_ANY,
	/**  counter trigger mode */
	TRIG_MODE_COUNTER,
	/**  trigger mode of user choice */
	TRIG_MODE_USER,
	/**  timer trigger mode */
	TRIG_MODE_TIMER,
	/**  timer and counter combo trigger mode */
	TRIG_MODE_COMBO,
};

/**
 * Queue can be in one of the following states
 * @ingroup libqdma_enums
 *
 */
enum q_state_t {
	/** @param Q_STATE_DISABLED Queue is not taken */
	Q_STATE_DISABLED = 0,
	/** @param Q_STATE_ENABLED Assigned/taken. Partial config is done */
	Q_STATE_ENABLED,
	/**
	 *  @param Q_STATE_ONLINE Resource/context is initialized for the queue
	 *  and is available for data consumption
	 */
	Q_STATE_ONLINE,
};

/**
 * Structure to hold the driver name and mode
 *
 * Mode can be set for each PF or VF group using module parameters
 * Refer enum qdma_drv_mode for different mode options
 * @ingroup libqdma_struct
 *
 */
struct drv_mode_name {
	/**  Mode of the function */
	enum qdma_drv_mode drv_mode;
	/**  Driver Name */
	char name[20];
};


/**
 * Queue type
 *
 * Look up table for name of the queue type and enum
 * @ingroup libqdma_struct
 *
 */
struct qdma_q_type {
	/** Queue type name */
	const char *name;
	/** Queue type */
	enum queue_type_t q_type;
};


/**
 * Completion entry format
 *
 * Completion Entry is user logic dependent
 * Current SW supported the following completion entry format
 * @ingroup libqdma_struct
 *
 */
struct qdma_ul_cmpt_info {
	union {
		/**  Flag bits */
		u8 fbits;
		struct cmpl_flag {
			/**  Format of the entry */
			u8 format:1;
			/**  Indicates the validity of the entry */
			u8 color:1;
			/**  Indicates the error status */
			u8 err:1;
			/**  Indicates the descriptor used status */
			u8 desc_used:1;
			/**  Indicates the end of transfer */
			u8 eot:1;
			/**  Filler bits */
			u8 filler:3;
		} f;
	};
	/**  Reserved filed added for structure alignment */
	u8 rsvd;
	/**  Length of the completion entry */
	u16 len;
	/**  Producer Index */
	unsigned int pidx;
	/**  Completion entry */
	__be64 *entry;
};

/**
 * Externel structure definition mode_name_list
 *
 * @ingroup libqdma_struct
 */
extern struct drv_mode_name mode_name_list[];

/**
 * Externel structure definition q_type_list
 *
 * @ingroup libqdma_struct
 */
extern struct qdma_q_type q_type_list[];

/**
 * Forward declaration for struct pci_dev
 *
 * @ingroup libqdma_struct
 */
struct pci_dev;

/**
 * Defines the per-device qdma property.
 *
 * @note if any of the max requested is less than supported, the value will
 *       be updated
 * @ingroup libqdma_struct
 */
struct qdma_dev_conf {
	/** pointer to pci_dev */
	struct pci_dev *pdev;
	/** Maximum number of queue pairs per device */
	unsigned short qsets_max;
	/** Reserved */
	unsigned short rsvd2;
	/**
	 * Indicates whether zero length DMA is allowed or not
	 */
	u8 zerolen_dma:1;
	/**
	 * Indicates whether the current pf
	 *  is master_pf or not
	 */
	u8 master_pf:1;
	/**
	 * moderate interrupt generation
	 */
	u8 intr_moderation:1;
	/** Reserved1 */
	u8 rsvd1:5;
	/**
	 * Maximum number of virtual functions for
	 * current physical function
	 */
	u8 vf_max;
	/** Interrupt ring size */
	u8 intr_rngsz;
	/**
	 * interrupt:
	 * - MSI-X only
	 * max of QDMA_DEV_MSIX_VEC_MAX per function, 32 in Versal
	 * - 1 vector is reserved for user interrupt
	 * - 1 vector is reserved mailbox
	 * - 1 vector on pf0 is reserved for error interrupt
	 * - the remaining vectors will be used for queues
	 */

	/**
	 *  max. of vectors used for queues.
	 *  libqdma update w/ actual #
	 */
	u16 msix_qvec_max;
	/** Max user msix vectors */
	u16 user_msix_qvec_max;
	/** Max data msix vectors */
	u16 data_msix_qvec_max;
	/** upper layer data, i.e. callback data */
	unsigned long uld;
	/** qdma driver mode */
	enum qdma_drv_mode qdma_drv_mode;
	/**
	 * an unique string to identify the dev.
	 * current format: qdma[pf|vf][idx] filled in by libqdma
	 */
	char name[QDMA_DEV_NAME_MAXLEN];

	/** dma config bar #, < 0 not present */
	char bar_num_config;
	/** AXI Master Lite(user bar) */
	char bar_num_user;
	/** AXI Bridge Master(bypass bar) */
	char bar_num_bypass;
	/** queue base for this function */
	int qsets_base;
	/** device index */
	u32 bdf;
	/** index of device in device list */
	u32 idx;
	/**
	 *  @brief  user interrupt, if null, default libqdma handler is used
	 *
	 *  @param dev_hndl	Device Handler
	 *  @param uld		upper layer data, i.e. callback data
	 */
	void (*fp_user_isr_handler)(unsigned long dev_hndl, unsigned long uld);

	/**
	 *  @brief  Q interrupt top,
	 *  per-device addtional handling code
	 *
	 *  @param dev_hndl	Device Handler
	 *  @param uld		upper layer data, i.e. callback data
	 */
	void (*fp_q_isr_top_dev)(unsigned long dev_hndl, unsigned long uld);

	/**
	 *  @brief  for freeing any resources in FLR process
	 *
	 *  @param dev_hndl	Device Handler
	 */
	void (*fp_flr_free_resource)(unsigned long dev_hndl);

	/**
	 *  root path for debugfs
	 */
	void *debugfs_dev_root;
};

/**
 * defines the per-device version information
 * @ingroup libqdma_struct
 *
 */
struct qdma_version_info {
	/** Version string */
	char rtl_version_str[DEVICE_VERSION_INFO_STR_LENGTH];
	/** Release string */
	char vivado_release_str[DEVICE_VERSION_INFO_STR_LENGTH];
	/** IP version string */
	char ip_str[DEVICE_VERSION_INFO_STR_LENGTH];
	/** Qdma device type string */
	char device_type_str[DEVICE_VERSION_INFO_STR_LENGTH];
};

/**
 * global CSR configuration
 * @ingroup libqdma_struct
 *
 */
struct global_csr_conf {
	/** Descriptor ring size ie. queue depth */
	unsigned int ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** C2H timer count  list */
	unsigned int c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** C2H counter threshold list*/
	unsigned int c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** C2H buffer size list */
	unsigned int c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** Writeback interval */
	unsigned int wb_intvl;
};


/**
 * qdma scatter gather request
 * @ingroup libqdma_struct
 *
 */
struct qdma_sw_sg {
	/** pointer to next page */
	struct qdma_sw_sg *next;
	/** pointer to current page */
	struct page *pg;
	/** offset in current page */
	unsigned int offset;
	/** length of the page */
	unsigned int len;
	/** dma address of the allocated page */
	dma_addr_t dma_addr;
};

/** struct qdma_request forward declaration
 * @ingroup libqdma_struct
 */
struct qdma_request;

/**
 * qdma configuration parameters
 *
 * qdma_queue_conf defines the per-dma Q property.
 * if any of the max requested is less than supported, the value will
 * be updated
 * @ingroup libqdma_struct
 *
 */
struct qdma_queue_conf {
	/**
	 *  @param qidx 0xFFFF: libqdma choose the queue idx 0 ~
	 *  (qdma_dev_conf.qsets_max - 1) the calling function choose the
	 *   queue idx
	 */
	u32 qidx:24;
	/** @note config flags: byte #1 */
	/** Indicates the streaming mode */
	u32 st:1;
	/** queue_type_t */
	u32 q_type:2;
	/** SDx only: inter-kernel communication pipe */
	u32 pipe:1;
	/** poll or interrupt */
	u32 irq_en:1;

	/** descriptor ring	 */
	/** global_csr_conf.ringsz[N] */
	u32 desc_rng_sz_idx:4;

	/** @note config flags: byte #2 */
	/** writeback enable, disabled for ST C2H */
	u8 wb_status_en:1;
	/**  sw context.cmpl_status_acc_en */
	u8 cmpl_status_acc_en:1;
	/**  sw context.cmpl_stauts_pend_chk */
	u8 cmpl_status_pend_chk:1;
	/**  send descriptor to bypass out */
	u8 desc_bypass:1;
	/**  descriptor prefetch enable control */
	u8 pfetch_en:1;
	/**  sw context.frcd_en[32] */
	u8 fetch_credit:1;
	/**
	 *  @param st_pkt_mode SDx only: ST packet mode
	 *  (i.e., with TLAST to identify the packet boundary)
	 */
	u8 st_pkt_mode:1;

	/** @note config flags: byte #3 */
	/**  global_csr_conf.c2h_buf_sz[N] */
	u8 c2h_buf_sz_idx:4;

	/**  ST C2H Completion/Writeback ring */
	/**  global_csr_conf.ringsz[N] */
	u8 cmpl_rng_sz_idx:4;

	/** @note config flags: byte #4 */
	/**  C2H ST cmpt + immediate data, desc_sz_t */
	u8 cmpl_desc_sz:2;
	/**  enable status desc. for CMPT */
	u8 cmpl_stat_en:1;
	/**  C2H Completion entry user-defined data */
	u8 cmpl_udd_en:1;
	/**  global_csr_conf.c2h_timer_cnt[N] */
	u8 cmpl_timer_idx:4;

	/** @note config flags byte #5 */
	/**  global_csr_conf.c2h_cnt_th[N] */
	u8 cmpl_cnt_th_idx:4;
	/**  tigger_mode_t */
	u8 cmpl_trig_mode:3;
	/**  enable interrupt for CMPT */
	u8 cmpl_en_intr:1;

	/** @note config flags byte #6 */
	/**  SW Context desc size, desc_sz_t */
	u8 sw_desc_sz:2;
	/**  prefetch bypass en */
	u8 pfetch_bypass:1;
	/**  OVF_DIS C2H ST over flow disable */
	u8 cmpl_ovf_chk_dis:1;
	/**  Port ID */
	u8 port_id:3;
	/**  Address Translation */
	u8 at:1;
	/**  Adaptive rx counter threshold */
	u8 adaptive_rx:1;
	/**  optimize for latency */
	u8 latency_optimize:1;
	/**  Disable pidx initialiaztion for ST C2H */
	u8 init_pidx_dis:1;

	/**  MM Channel */
	u8 mm_channel:1;

	/**  user provided per-Q irq handler */
	unsigned long quld;		/* set by user for per Q data */
	/**  acummulate PIDX to batch packets */
	u32 pidx_acc:8;
	/**
	 *  @brief  Q interrupt top, per-queue additional handling
	 *  code for example, network rx napi_schedule(&Q->napi)
	 *
	 * @param  qhndl	Queue handle
	 * @param  quld		Queue ID
	 *
	 */
	void (*fp_descq_isr_top)(unsigned long qhndl, unsigned long quld);

	/**
	 * @brief optional rx packet handler:
	 * called from irq BH (i.e.qdma_queue_service_bh())
	 *
	 * @param  qhndl	Queue handle
	 * @param  quld		Queue ID
	 * @param  len		Packet Length
	 * @param  sgcnt	scatter gathher list count
	 * @param  sgl		packet data in scatter-gather list
	 * @param  udd		user defined data in the completion entry
	 *
	 * @note
	 *		a. do NOT modify any field of sgl
	 *		b. if zero copy, do a get_page() to prevent page freeing
	 *		c. do loop through the sgl with sg->next and stop
	 *		at sgcnt. the last sg may not have sg->next = NULL
	 *
	 * @returns
	 *	0 to allow libqdma free/re-task the sgl
	 *	< 0, libqdma will keep the packet for processing again
	 *
	 * @details
	 * A single packet could contain:
	 * in the case of c2h_udd_en = 1:
	 *
	 * udd only (udd valid, sgcnt = 0, sgl = NULL), or
	 * udd + packet data in the case of c2h_udd_en = 0:
	 * packet data (udd = NULL, sgcnt > 0 and sgl valid)
	 *
	 */
	int (*fp_descq_c2h_packet)(unsigned long qhndl, unsigned long quld,
				unsigned int len, unsigned int sgcnt,
				struct qdma_sw_sg *sgl, void *udd);
	/**
	 * @brief fill the all the descriptors required for
	 *                        transfer
	 * @param q_hndl handle with which bypass module can request back
	 *         info from libqdma
	 *
	 * @param q_mode mode in which q is initialized
	 * @param q_dir direction in which q is initialized
	 * @param sgcnt number of scatter gather entries for this request
	 * @param sgl list of scatter gather entries
	 *
	 * @returns On calling this API, bypass module can request for
	 * descriptor using qdma_q_desc_get and set up as many descriptors
	 * as required for each scatter gather entry. If descriptors required
	 * are not available, it can return the number of sgcnt addressed
	 * till now and return <0 in case of any failure
	 */
	int (*fp_bypass_desc_fill)(void *q_hndl, enum qdma_q_mode q_mode,
			enum qdma_q_dir, struct qdma_request *req);
	/**
	 * @brief parse cmpt entry in bypass mode
	 *
	 * @param cmpt_entry cmpt entry descriptor
	 * @param cmpt_info parsed bypass related info from cmpt_entry
	 *
	 * @returns 0 for success
	 */
	int (*fp_proc_ul_cmpt_entry)(void *cmpt_entry,
			struct qdma_ul_cmpt_info *cmpt_info);

	/** @note Following fileds are filled by libqdma */
	/**  name of the qdma device */
	char name[QDMA_QUEUE_NAME_MAXLEN];
	/**  ring size of the queue */
	unsigned int rngsz;
	/**  completion ring size of the queue */
	unsigned int rngsz_cmpt;
	/** C2H buffer size */
	unsigned int c2h_bufsz;
	/**  Ping Pong measurement */
	u8 ping_pong_en:1;
	/**  Keyhole Aperture Size */
	u32 aperture_size;
};

/**
 * display queue state in a string buffer
 * @ingroup libqdma_struct
 *
 */
struct qdma_q_state {
	/**  current q state */
	enum q_state_t qstate;
	/**
	 *   0xFFFF: libqdma choose the queue idx 0 ~
	 *  (qdma_dev_conf.qsets_max - 1) the calling function choose the
	 *   queue idx
	 */
	u32 qidx:24;
	/**  Indicates the streaming mode */
	u32 st:1;
	/**  queue type */
	enum queue_type_t q_type;
};


/**
 * qdma request for read or write
 * @ingroup libqdma_struct
 *
 */
struct qdma_request {
	/**  private to the dma driver, do NOT touch */
	unsigned char opaque[QDMA_REQ_OPAQUE_SIZE];
	/**
	 *   filled in by the calling function
	 *  for the calling function
	 */
	unsigned long uld_data;
	/**  set fp_done for non-blocking mode */
	int (*fp_done)(struct qdma_request *req, unsigned int bytes_done,
			int err);
	/**  timeout in mili-seconds, 0 - no timeout */
	unsigned int timeout_ms;
	/**  total data size */
	unsigned int count;
	/**  MM only, DDR/BRAM memory addr */
	u64 ep_addr;
	/**  flag to indicate if memcpy is required */
	u8 no_memcpy:1;
	/**  if write to the device */
	u8 write:1;
	/**  if sgt is already dma mapped */
	u8 dma_mapped:1;
	/** indicates end of transfer towards user kernel */
	u8 h2c_eot:1;
	/** state check disbaled in queue pkt API */
	u8 check_qstate_disabled:1;
	u8 _pad:3;
	/** user defined data present */
	u8 udd_len;
	/**  number of scatter-gather entries < 64K */
	unsigned int sgcnt;
	/**  scatter-gather list of data bufs */
	struct qdma_sw_sg *sgl;
	/**  udd data */
	u8 udd[QDMA_UDD_MAXLEN];
};

/**
 * Completion control
 * @ingroup libqdma_struct
 */
struct qdma_cmpl_ctrl {
	/** global_csr_conf.c2h_cnt_th[N] */
	u8 cnt_th_idx:4;
	/** global_csr_conf.c2h_timer_cnt[N] */
	u8 timer_idx:4;
	/** tigger_mode */
	u8 trigger_mode:3;
	/** enable status desc. for CMPT */
	u8 en_stat_desc:1;
	/** enable interrupt for CMPT */
	u8 cmpl_en_intr:1;
};


/**
 * QDMA queue count
 * @ingroup libqdma_struct
 */
struct qdma_queue_count {
	/** H2C queue count */
	u32 h2c_qcnt;
	/** C2H queue count */
	u32 c2h_qcnt;
	/** CMPT queue count */
	u32 cmpt_qcnt;
};


/**
 * Initializes the QDMA core library
 *
 * @param num_threads number of threads to be created each for request
 *  processing and writeback processing
 *
 * @param debugfs_root		root path for debugfs
 *
 * @returns			0:	success <0:	error
 *
 */
int libqdma_init(unsigned int num_threads, void *debugfs_root);

/*****************************************************************************/
/**
 * cleanup the QDMA core library before exiting
 *
 *
 *****************************************************************************/
void libqdma_exit(void);


/*****************************************************************************/
/**
 * legacy interrupt init
 *
 *****************************************************************************/
void intr_legacy_init(void);

/*****************************************************************************/
/**
 * Read the pci bars and configure the fpga
 * This API should be called from probe()
 *
 * User interrupt will not be enabled until qdma_user_isr_enable() is called
 *
 * @param mod_name	the module name, used for request_irq
 * @param conf		device configuration
 * @param dev_hndl	an opaque handle for libqdma to identify the device
 *
 * @returns		0 in case of success and <0 in case of error
 *
 *****************************************************************************/
int qdma_device_open(const char *mod_name, struct qdma_dev_conf *conf,
				unsigned long *dev_hndl);

/*****************************************************************************/
/**
 * Prepare fpga for removal: disable all interrupts (users
 * and qdma) and release all resources.This API should be called from remove()
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl retured from qdma_device_open()
 *
 * @returns		0 in case of success and <0 in case of error
 *
 *****************************************************************************/
int qdma_device_close(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * Set the device in offline mode
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl retured from qdma_device_open()
 * @param reset		0/1 function level reset active or not
 *
 * @returns	0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_offline(struct pci_dev *pdev, unsigned long dev_hndl,
						 int reset);

/*****************************************************************************/
/**
 * Set the device in online mode and re-initialze it
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl retured from qdma_device_open()
 * @param reset		0/1 function level reset active or not
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_online(struct pci_dev *pdev, unsigned long dev_hndl,
					   int reset);

/*****************************************************************************/
/**
 * Start pre-flr processing
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_flr_quirk_set(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * Check if pre-flr processing completed
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 *
 * @returns		0 for success <0 for error
 *
 *****************************************************************************/
int qdma_device_flr_quirk_check(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * Retrieve the current device configuration
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param conf		device configuration
 * @param buflen	input buffer length
 * @param buf		error message buffer, can be NULL/0 (i.e., optional)
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_config(unsigned long dev_hndl, struct qdma_dev_conf *conf,
				char *buf, int buflen);

/*****************************************************************************/
/**
 * Clear device statistics
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_clear_stats(unsigned long dev_hndl);

/*****************************************************************************/
/**
 * Get mm h2c packets processed
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param mmh2c_pkts	number of mm h2c packets processed
 *
 *@returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_mmh2c_pkts(unsigned long dev_hndl,
			       unsigned long long *mmh2c_pkts);

/*****************************************************************************/
/**
 * Get mm c2h packets processed
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param mmc2h_pkts	number of mm c2h packets processed
 *
 *@returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_mmc2h_pkts(unsigned long dev_hndl,
				unsigned long long *mmc2h_pkts);

/*****************************************************************************/
/**
 * Get st h2c packets processed
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param sth2c_pkts	number of st h2c packets processed
 *
 *@returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_sth2c_pkts(unsigned long dev_hndl,
				unsigned long long *sth2c_pkts);

/*****************************************************************************/
/**
 * Get st c2h packets processed
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param stc2h_pkts	number of st c2h packets processed
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_stc2h_pkts(unsigned long dev_hndl,
				unsigned long long *stc2h_pkts);

/*****************************************************************************/
/**
 *
 * Min latency (in CPU ticks) observed for
 * all packets to do H2C-C2H loopback.
 * Packet is transmitted in ST H2C direction, the
 * user-logic ST Traffic generator is configured to
 * loop back the packet in C2H direction. Timestamp
 * (in CPU ticks) of the H2C transmission
 * is embedded in H2C packet at time of PIDX
 * update, then timestamp of the loopback packet
 * is taken at time when data interrupt is hit,
 * diff is used to	measure	roundtrip latency.
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param min_lat	Minimum ping pong latency in CPU ticks. Divide with
 *			the nominal CPU freqeuncy to get latency in  NS.
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_ping_pong_min_lat(unsigned long dev_hndl,
				unsigned long long *min_lat);

/*****************************************************************************/
/**
 * Max latency (in CPU ticks) observed for
 * all packets to do H2C-C2H loopback.
 * Packet is transmitted in ST H2C direction, the
 * user-logic ST Traffic generator is configured to
 * loop back the packet in C2H direction. Timestamp
 * (in CPU ticks) of the H2C transmission
 * is embedded in H2C packet at time of PIDX
 * update, then timestamp of the loopback packet
 * is taken at time when data interrupt is hit,
 * diff is used to	measure	roundtrip latency.
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param max_lat	Max ping pong latency in CPU ticks. Divide with the
 *			nominal CPU freqeuncy to get latency in  NS.
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_ping_pong_max_lat(unsigned long dev_hndl,
				unsigned long long *max_lat);

/*****************************************************************************/
/**
 * Total latency (in CPU ticks) observed for
 * all packets to do H2C-C2H loopback.
 * Packet is transmitted in ST H2C direction, the
 * user-logic ST Traffic generator is configured to
 * loop back the packet in C2H direction. Timestamp
 * (in CPU ticks) of the H2C transmission
 * is embedded in H2C packet at time of PIDX
 * update, then timestamp of the loopback packet
 * is taken at time when data interrupt is hit,
 * diff is used to	measure	roundtrip latency.
 *
 * @param dev_hndl	dev_hndl retunred from qdma_device_open()
 * @param lat_total	Total Ping Pong latency. Divide with total loopback
 *	C2H packets to get average ping pong latency. Divide further
 *	with the nominal CPU frequency to get the avg latency in NS.
 *
 * @returns	0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_get_ping_pong_tot_lat(unsigned long dev_hndl,
				unsigned long long *lat_total);

/*****************************************************************************/
/**
 * Set the current device configuration
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param conf		device configuration to set
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_set_config(unsigned long dev_hndl, struct qdma_dev_conf *conf);

/*****************************************************************************/
/**
 * Configure sriov
 *
 * @param pdev		ptr to struct pci_dev
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param num_vfs	# of VFs to be instantiated
 *
 * @returns		0 for success and <0 for error
 *
 * configures sriov
 *****************************************************************************/
int qdma_device_sriov_config(struct pci_dev *pdev, unsigned long dev_hndl,
				int num_vfs);

/*****************************************************************************/
/**
 * Read dma config register
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param reg_addr	register address
 * @param value		pointer to hold the read value
 *
 * @returns		0 for success and <0 for error
 *
 * reads dma config register
 *
 *****************************************************************************/
int qdma_device_read_config_register(unsigned long dev_hndl,
					unsigned int reg_addr,
					unsigned int *value);

/*****************************************************************************/
/**
 * Write dma config register
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param reg_addr	register address
 * @param val		register value to be writen
 *
 * @returns		0 for success and <0 for error
 * writes dma config register
 *
 *****************************************************************************/
int qdma_device_write_config_register(unsigned long dev_hndl,
					unsigned int reg_addr,
					unsigned int val);

/*****************************************************************************/
/**
 * retrieve the capabilities of a device.
 *
 * @param dev_hndl:	handle returned from qdma_device_open()
 * @param dev_attr:	pointer to hold all the device attributes
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_device_capabilities_info(unsigned long dev_hndl,
		struct qdma_dev_attributes *dev_attr);

/*****************************************************************************/
/**
 * Retrieve the RTL version , Vivado Release ID and Versal IP info
 *
 * @param dev_hndl		handle returned from qdma_device_open()
 * @param version_info		pointer to hold all the version details
 *
 * @returns			0 for success and <0 for error
 *
 * retrieves the RTL version , Vivado Release ID and Versal IP info
 *
 *****************************************************************************/
int qdma_device_version_info(unsigned long dev_hndl,
			     struct qdma_version_info *version_info);

/*****************************************************************************/
/**
 * Retrieve the software version
 *
 * @param software_version	A pointer to a null-terminated string
 * @param length			Length of the version name string
 *
 * @returns			0 for success and <0 for error
 *
 * retrieves the software version
 *
 *****************************************************************************/
int qdma_software_version_info(char *software_version, int length);

/*****************************************************************************/
/**
 * Retrieve the global csr settings
 *
 * @param dev_hndl	handle returned from qdma_device_open()
 * @param index		Index from where the values needs to read
 * @param count		number of entries to be read
 * @param csr		data structures to hold the csr values
 *
 * @returns		0 for success and <0 for error
 *
 * retrieves the global csr settings
 *
 *****************************************************************************/
int qdma_global_csr_get(unsigned long dev_hndl, u8 index, u8 count,
		struct global_csr_conf *csr);

/*****************************************************************************/
/**
 * Add a queue
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param qconf		queue configuration parameters
 * @param qhndl		list of unsigned long values that are the opaque qhndl
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_queue_add(unsigned long dev_hndl, struct qdma_queue_conf *qconf,
			unsigned long *qhndl, char *buf, int buflen);

/*****************************************************************************/
/**
 * Configure the queue with qcong parameters
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param qid		queue id
 * @param qconf		queue configuration parameters
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0: success <0: error
 *****************************************************************************/
int qdma_queue_config(unsigned long dev_hndl, unsigned long qid,
			struct qdma_queue_conf *qconf, char *buf, int buflen);
/*****************************************************************************/
/**
 * start a queue (i.e, online, ready for dma)
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		the opaque qhndl
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_queue_start(unsigned long dev_hndl, unsigned long id,
						char *buf, int buflen);

/*****************************************************************************/
/**
 * Stop a queue (i.e., offline, NOT ready for dma)
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		the opaque qhndl
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_queue_stop(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * Get the state of the queue
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		the opaque qhndl
 * @param q_state	state of the queue
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_get_queue_state(unsigned long dev_hndl, unsigned long id,
		struct qdma_q_state *q_state, char *buf, int buflen);

/*****************************************************************************/
/**
 * remove a queue
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		the opaque qhndl
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0 for success and <0 for error
 *
 *****************************************************************************/
int qdma_queue_remove(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * retrieve the configuration of a queue
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id:		an opaque queue handle of type unsigned long
 * @param qconf		pointer to hold the qdma_queue_conf structure.
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		0: success <0: error
 *
 *****************************************************************************/
int qdma_queue_get_config(unsigned long dev_hndl, unsigned long id,
		struct qdma_queue_conf *qconf,
		char *buf, int buflen);

/*****************************************************************************/
/**
 * Display all configured queues in a string buffer
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		if optional message buffer used then strlen of buf,
 *	 otherwise QDMA_OPERATION_SUCCESSFUL and <0 for error
 *
 *****************************************************************************/
int qdma_queue_list(unsigned long dev_hndl, char *buf, int buflen);

/*****************************************************************************/
/**
 * Display a config registers in a string buffer
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		success: if optional message buffer used
 *	then strlen of buf, otherwise 0 and <0: error
 *
 *****************************************************************************/
int qdma_config_reg_dump(unsigned long dev_hndl, char *buf,
		int buflen);

/*****************************************************************************/
/**
 * display a queue's state in a string buffer
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		an opaque queue handle of type unsigned long
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		if optional message buffer used then strlen of buf,
 *	 otherwise QDMA_OPERATION_SUCCESSFUL and <0 for error
 *
 *****************************************************************************/
int qdma_queue_dump(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * Display a queue's descriptor ring from index start
 *					~ end in a string buffer
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		an opaque queue handle of type unsigned long
 * @param start		start index
 * @param end		end index
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		if optional message buffer used then strlen of buf,
 *	 otherwise QDMA_OPERATION_SUCCESSFUL and <0 for error
 *
 *****************************************************************************/
int qdma_queue_dump_desc(unsigned long dev_hndl, unsigned long id,
				unsigned int start, unsigned int end, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * display a queue's descriptor ring from index start
 *					~ end in a string buffer
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		an opaque queue handle of type unsigned long
 * @param start		start index
 * @param end		end index
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		if optional message buffer used then strlen of buf,
 *	 otherwise QDMA_OPERATION_SUCCESSFUL and <0 for error
 *
 *****************************************************************************/
int qdma_queue_dump_cmpt(unsigned long dev_hndl, unsigned long id,
				unsigned int start, unsigned int end, char *buf,
				int buflen);

#ifdef ERR_DEBUG
/*****************************************************************************/
/**
 * Induce the error
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		error id
 * @param err		error info
 * @param buflen	length of the input buffer
 * @param buf		message buffer
 *
 * @returns		if optional message buffer used then strlen of buf,
 *	 otherwise QDMA_OPERATION_SUCCESSFUL and <0 for error
 *
 *****************************************************************************/
int qdma_queue_set_err_induction(unsigned long dev_hndl, unsigned long id,
				 u32 err, char *buf, int buflen);
#endif

/*****************************************************************************/
/**
 * Submit a scatter-gather list of data for dma
 * operation (for both read and write)
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue index
 * @param req		qdma request
 *
 * @returns		# of bytes transferred for success and  <0 for error
 *
 *****************************************************************************/
ssize_t qdma_request_submit(unsigned long dev_hndl, unsigned long id,
			struct qdma_request *req);

/*****************************************************************************/
/**
 * Submit a scatter-gather list of data for dma
 * operation (for both read and write)
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue index
 * @param count		number of requests
 * @param reqv		qdma request
 *
 * @returns		# of bytes transferred for success and  <0 for error
 *
 *****************************************************************************/
ssize_t qdma_batch_request_submit(unsigned long dev_hndl, unsigned long id,
			  unsigned long count, struct qdma_request **reqv);

/*****************************************************************************/
/**
 * Peek a receive (c2h) queue
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue hndl returned from qdma_queue_add()
 *
 * filled in by libqdma:
 * @param udd_cnt	# of udd received
 * @param pkt_cnt	# of packets received
 * @param data_len	# of bytes of packet data received
 *
 * @returns		# of packets received in the Q or <0 for error
 *****************************************************************************/
int qdma_queue_c2h_peek(unsigned long dev_hndl, unsigned long id,
			unsigned int *udd_cnt, unsigned int *pkt_cnt,
			unsigned int *data_len);


/*****************************************************************************/
/**
 * Query of # of free descriptor
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue hndl returned from qdma_queue_add()
 *
 * @returns		# of available desc in the queue or <0 for error
 *****************************************************************************/
int qdma_queue_avail_desc(unsigned long dev_hndl, unsigned long id);

/** packet/streaming interfaces  */

/*****************************************************************************/
/**
 * Read/set the c2h Q's completion control
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		hndl returned from qdma_queue_add()
 * @param cctrl		completion control
 * @param set		read or set
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_queue_cmpl_ctrl(unsigned long dev_hndl, unsigned long id,
				struct qdma_cmpl_ctrl *cctrl, bool set);

/*****************************************************************************/
/**
 * Read rcv'ed data (ST C2H dma operation)
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue hndl returned from qdma_queue_add()
 * @param req		pointer to the request data
 * @param cctrl		completion control, if no change is desired,
 *                      set it to NULL
 *
 * @returns		# of bytes transferred for success and  <0 for error
 *
 *****************************************************************************/
int qdma_queue_packet_read(unsigned long dev_hndl, unsigned long id,
		struct qdma_request *req, struct qdma_cmpl_ctrl *cctrl);

/*****************************************************************************/
/**
 * Submit data for ST H2C dma operation
 *
 * @param dev_hndl	hndl returned from qdma_device_open()
 * @param id		queue hndl returned from qdma_queue_add()
 * @param req		pointer to the list of packet data
 *
 * @returns		# of bytes transferred for success and  <0 for error
 *
 *****************************************************************************/
int qdma_queue_packet_write(unsigned long dev_hndl, unsigned long id,
			struct qdma_request *req);

/*****************************************************************************/
/**
 * Service the queue in the case of irq handler is registered by the user,
 * the user should call qdma_queue_service() in its interrupt handler to
 * service the queue
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		queue hndl returned from qdma_queue_add()
 * @param budget	ST C2H only, max number of completions to be processed.
 * @param c2h_upd_cmpl	flag to update the completion
 *
 * Return:	0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_queue_service(unsigned long dev_hndl, unsigned long id,
			int budget, bool c2h_upd_cmpl);

/*****************************************************************************/
/**
 * Update queue pointers
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param qhndl		hndl returned from qdma_queue_add()
 *
 * Return:	0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_queue_update_pointers(unsigned long dev_hndl, unsigned long qhndl);

/*****************************************************************************/
/**
 * Display the interrupt ring info of a vector
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param vector_idx	vector number
 * @param start_idx	interrupt ring start idx
 * @param end_idx	interrupt ring end idx
 * @param buflen	length of the input buffer
 * @param buf		message bufferuffer
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_intr_ring_dump(unsigned long dev_hndl, unsigned int vector_idx,
			int start_idx, int end_idx, char *buf, int buflen);

/*****************************************************************************/
/**
 * Function to receive the user defined data
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		queue handle
 * @param buflen	length of the input buffer
 * @param buf		message bufferuffer
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_descq_get_cmpt_udd(unsigned long dev_hndl, unsigned long id,
		char *buf, int buflen);

/*****************************************************************************/
/**
 * Function to receive the completion data
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param id		queue handle
 * @param num_entries	I/O number of entries
 * @param cmpt_entries	List of completion entries
 * @param buflen	length of the input buffer
 * @param buf		message bufferuffer
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_descq_read_cmpt_data(unsigned long dev_hndl, unsigned long id,
				u32 *num_entries,  u8 **cmpt_entries,
				char *buf, int buflen);

/*****************************************************************************/
/**
 * Function to receive the queue count
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param q_count	queue count
 * @param buflen	length of the input buffer
 * @param buf		message bufferuffer
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_get_queue_count(unsigned long dev_hndl,
		struct qdma_queue_count *q_count,
		char *buf, int buflen);

/*****************************************************************************/
/**
 * Function to print out detailed information for register value
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param reg_addr	register address
 * @param num_regs  num of registerse to be dumped
 * @param buf		message bufferuffer
 * @param buflen	length of the input buffer
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_config_reg_info_dump(unsigned long dev_hndl, uint32_t reg_addr,
				uint32_t num_regs, char *buf, int buflen);

#ifdef __QDMA_VF__
/*****************************************************************************/
/**
 * Call for VF to request qmax number of Qs
 *
 * @param dev_hndl	dev_hndl returned from qdma_device_open()
 * @param qmax		number of qs requested by vf
 *
 * @returns		0 for success or <0 for error
 *
 *****************************************************************************/
int qdma_vf_qconf(unsigned long dev_hndl, int qmax);
#endif

#endif
