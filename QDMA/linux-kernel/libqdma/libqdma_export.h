/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
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
 * @file
 * @brief This file contains the declarations for libqdma interfaces
 *
 */
#include <linux/types.h>
#include <linux/interrupt.h>
#include "libqdma_config.h"

/** Invalid QDMA function number */
#define QDMA_FUNC_ID_INVALID	(QDMA_PF_MAX + QDMA_VF_MAX)

/**
 * QDMA Error codes
 */
enum qdma_error_codes {
	QDMA_OPERATION_SUCCESSFUL		= 0,
	/*!< QDMA driver API operation successful */
	QDMA_ERR_PCI_DEVICE_NOT_FOUND		= -1,
	/*!< QDMA PCI device not found on the PCIe bus */
	QDMA_ERR_PCI_DEVICE_ALREADY_ATTACHED	= -2,
	/*!< QDMA PCI device already attached */
	QDMA_ERR_PCI_DEVICE_ENABLE_FAILED	= -3,
	/*!< Failed to enable the QDMA PCIe device */
	QDMA_ERR_PCI_DEVICE_INIT_FAILED		= -4,
	/*!< Failed to initialize the QDMA PCIe device */
	QDMA_ERR_INVALID_INPUT_PARAM		= -5,
	/*!< Invalid input parameter given to QDMA API */
	QDMA_ERR_INVALID_PCI_DEV		= -6,
	/*!< Invalid PCIe device */
	QDMA_ERR_INVALID_QIDX			= -7,
	/*!< Invalid Queue ID provided as input */
	QDMA_ERR_INVALID_DESCQ_STATE		= -8,
	/*!< Invalid descriptor queue state */
	QDMA_ERR_INVALID_DIRECTION		= -9,
	/*!< Invalid descriptor direction provided */
	QDMA_ERR_DESCQ_SETUP_FAILED		= -10,
	/*!< Failed to setup the descriptor queue */
	QDMA_ERR_DESCQ_FULL			= -11,
	/*!< Descriptor queue is full */
	QDMA_ERR_DESCQ_IDX_ALREADY_ADDED	= -12,
	/*!< Descriptor queue index is already added */
	QDMA_ERR_QUEUE_ALREADY_CONFIGURED	= -13,
	/*!< Queue is already configured */
	QDMA_ERR_OUT_OF_MEMORY			= -14,
	/*!< Out of memory */
	QDMA_ERR_INVALID_QDMA_DEVICE		= -15,
	/*!< Invalid QDMA device, QDMA device is not yet created */
	QDMA_ERR_INTERFACE_NOT_ENABLED_IN_DEVICE	= -16,
	/*!< The ST or MM or Both interface not enabled in the device */
};


/**
 * qdma_drv_state_t - qdma driver state
 */
enum qdma_drv_mode {
	/*!< auto mode decided automatically, mix of poll and interrupt mode */
	AUTO_MODE,
	/*!< driver is inserted in poll mode */
	POLL_MODE,
	/*!< driver is inserted in direct interrupt mode */
	DIRECT_INTR_MODE,
	/*!< driver is inserted in indirect interrupt mode */
	INDIRECT_INTR_MODE,
	/*!< driver is inserted in legacy interrupt mode */
	LEGACY_INTR_MODE
};

struct drv_mode_name {
	enum qdma_drv_mode drv_mode;
	char name[20];
};
extern struct drv_mode_name mode_name_list[];

struct pci_dev;

/*****************************************************************************/
/**
 * libqdma_init()       initialize the QDMA core library
 *
 * @param[in] qdma_drv_mode - mode in which the driver needs to operate
 * @param[in] num_threads - number of threads to be created each for request
 *  processing and writeback processing
 *
 * @return	0:	success
 * @return	<0:	error
 *****************************************************************************/
int libqdma_init(enum qdma_drv_mode qdma_drv_mode, unsigned int num_threads);

/*****************************************************************************/
/**
 * libqdma_exit()       cleanup the QDMA core library before exiting
 *
 * @return	none
 *****************************************************************************/
void libqdma_exit(void);

/**
 * intr_ring_size_sel - qdma interrupt ring size selection
 */
enum intr_ring_size_sel {
	/**
	 *	0 - INTR_RING_SZ_4KB, Accommodates 512 entries
	 */
	INTR_RING_SZ_4KB = 0,
	/**
	 *	1 - INTR_RING_SZ_8KB, Accommodates 1024 entries
	 */
	INTR_RING_SZ_8KB,
	/**
	 *	2 - INTR_RING_SZ_12KB, Accommodates 1536 entries
	 */
	INTR_RING_SZ_12KB,
	/**
	 *	3 - INTR_RING_SZ_16KB, Accommodates 2048 entries
	 */
	INTR_RING_SZ_16KB,
	/**
	 *	4 - INTR_RING_SZ_20KB, Accommodates 2560 entries
	 */
	INTR_RING_SZ_20KB,
	/**
	 *	5 - INTR_RING_SZ_24KB, Accommodates 3072 entries
	 */
	INTR_RING_SZ_24KB,
	/**
	 *	6 - INTR_RING_SZ_24KB, Accommodates 3584 entries
	 */
	INTR_RING_SZ_28KB,
	/**
	 *	7 - INTR_RING_SZ_24KB, Accommodates 4096 entries
	 */
	INTR_RING_SZ_32KB,
};

enum qdma_dev_qmax_state {
	/** device qmax not configured */
	QMAX_CFG_UNCONFIGURED,
	/** device qmax configured with initial values */
	QMAX_CFG_INITIAL,
	/** device qmax configured from sysfs */
	QMAX_CFG_USER,
};

/**
 *	Maxinum length of the QDMA device name
 */
#define QDMA_DEV_NAME_MAXLEN	32

/**
 * qdma_dev_conf defines the per-device qdma property.
 *
 * NOTE: if any of the max requested is less than supported, the value will
 *       be updated
 */
struct qdma_dev_conf {
	/**	pointer to pci_dev */
	struct pci_dev *pdev;
	/**	Maximum number of queue pairs per device */
	unsigned short qsets_max;
	/**	Reserved */
	unsigned short rsvd2;
	/**	Indicates whether zero length DMA is allowed or not */
	u8 zerolen_dma:1;
	/**	Indicates whether the current pf is master_pf or not */
	u8 master_pf:1;
	/** extra handling of per descq handling in
	 *  top half (i.e., qdma_descq.fp_descq_isr_top will be set)
	 */
	u8 isr_top_q_en:1;
	/**	Reserved1 */
	u8 rsvd1:5;
	/** Maximum number of virtual functions for current physical function */
	u8 vf_max;
	/**	Interrupt ring size */
	u8 intr_rngsz;
	/**
	 * interrupt:
	 * - MSI-X only
	 * max of QDMA_DEV_MSIX_VEC_MAX per function, 32 in Everest
	 * - 1 vector is reserved for user interrupt
	 * - 1 vector is reserved mailbox
	 * - 1 vector on pf0 is reserved for error interrupt
	 * - the remaining vectors will be used for queues
	 */

	/** max. of vectors used for queues. libqdma update w/ actual # */
	u8 msix_qvec_max;
	/** upper layer data, i.e. callback data */
	unsigned long uld;
	/** qdma driver mode */
	enum qdma_drv_mode qdma_drv_mode;
	/**
	 * an unique string to identify the dev.
	 * current format: qdma[pf|vf][idx] filled in by libqdma
	 */
	char name[QDMA_DEV_NAME_MAXLEN];

#ifdef RTL2_FEATURE_CONFIGURABLE_BAR
	/** dma config bar #, < 0 not present */
	char bar_num_config;
	/** user bar */
	char bar_num_user;
	/**	bypass bar */
	char bar_num_bypass;
#endif
	/** dma config bar #, < 0 not present */
	char bar_num_config;
	/** user bar */
	char bar_num_user;
	/**	bypass bar */
	char bar_num_bypass;
	/** STM bar, PF only */
	char bar_num_stm;
	/** user bar, PF only */
	unsigned int qsets_base;
	/** device index */
	u32 bdf;
	/** index of device in device list */
	u32 idx;
	/**current configuration state of device*/
	enum qdma_dev_qmax_state cur_cfg_state;
	/** xmit in traffic manager mode */
	u8 tm_mode_en;
	/** enable 1 CDH for Traffic Manager */
	u8 tm_one_cdh_en;

	/** user interrupt, if null, default libqdma handler is used */
	void (*fp_user_isr_handler)(unsigned long dev_hndl, unsigned long uld);

	/**
	 * example flow of ST C2H:
	 * a. interrupt fires
	 * b. Hard IRQ: libqdma isr top -> dev->fp_q_isr_top_dev ->
	 *	isr_top_qproc && Q->fp_descq_isr_top
	 * c. Soft IRQ:
	 *	irq handler
	 *	qdma_queue_service_bh() ->
	 *	if rx: Q->fp_descq_rx_packet() called for each packet
	 *	qdma_queue_cmpl_ctrl(set=true) to update h/w and re-enable
	 *	interrupt
	 */

	/** Q interrupt top, per-device addtional handling code */
	void (*fp_q_isr_top_dev)(unsigned long dev_hndl, unsigned long uld);
};

/*****************************************************************************/
/**
 * qdma_device_open() - read the pci bars and configure the fpga
 * This API should be called from probe()
 *
 * User interrupt will not be enabled until qdma_user_isr_enable() is called
 *
 * @param[in]	mod_name:	the module name, used for request_irq
 * @param[in]	conf:		device configuration
 * @param[out]	dev_hndl:	an opaque handle
 *				for libqdma to identify the device
 *
 * @return	QDMA_OPERATION_SUCCESSFUL success
 * @return	<0 in case of error
 *****************************************************************************/
int qdma_device_open(const char *mod_name, struct qdma_dev_conf *conf,
				unsigned long *dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_close() - prepare fpga for removal: disable all interrupts (users
 * and qdma) and release all resources.This API should be called from remove()
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl retured from qdma_device_open()
 *
 *****************************************************************************/
void qdma_device_close(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_offline() - Set the device in offline mode
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl retured from qdma_device_open()
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
void qdma_device_offline(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_online() - Set the device in online mode and re-initialze it
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl retured from qdma_device_open()
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_online(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_flr_quirk_set() - start pre-flr processing
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_flr_quirk_set(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_flr_quirk_check() - check if pre-flr processing completed
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl retunred from qdma_device_open()
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_flr_quirk_check(struct pci_dev *pdev, unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_get_config() - retrieve the current device configuration
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 * @param[in]	conf:		device configuration
 * @param[in]	ebuflen:	input buffer length
 * @param[out]	ebuf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_get_config(unsigned long dev_hndl, struct qdma_dev_conf *conf,
				char *ebuf, int ebuflen);

/*****************************************************************************/
/**
 * qdma_device_clear_stats() - clear device statistics
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_clear_stats(unsigned long dev_hndl);

/*****************************************************************************/
/**
 * qdma_device_get_mmh2c_pkts() - get mm h2c packets processed
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 * @param[out]	mmh2c_pkts: number of mm h2c packets processed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_get_mmh2c_pkts(unsigned long dev_hndl,
			       unsigned long long *mmh2c_pkts);

/*****************************************************************************/
/**
 * qdma_device_get_mmc2h_pkts() - get mm c2h packets processed
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 * @param[out]	mmc2h_pkts: number of mm c2h packets processed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_get_mmc2h_pkts(unsigned long dev_hndl,
				unsigned long long *mmc2h_pkts);

/*****************************************************************************/
/**
 * qdma_device_get_sth2c_pkts() - get st h2c packets processed
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 * @param[out]	sth2c_pkts: number of st h2c packets processed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_get_sth2c_pkts(unsigned long dev_hndl,
				unsigned long long *sth2c_pkts);

/*****************************************************************************/
/**
 * qdma_device_get_stc2h_pkts() - get st c2h packets processed
 *
 * @param[in]	dev_hndl: dev_hndl retunred from qdma_device_open()
 * @param[out]	stc2h_pkts: number of st c2h packets processed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_get_stc2h_pkts(unsigned long dev_hndl,
				unsigned long long *stc2h_pkts);
/*****************************************************************************/
/**
 * qdma_device_set_config() - set the current device configuration
 *
 * @param[in]	dev_hndl: dev_hndl returned from qdma_device_open()
 * @param[in]	conf:		device configuration to set
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_set_config(unsigned long dev_hndl, struct qdma_dev_conf *conf);

/*****************************************************************************/
/**
 * qdma_device_set_cfg_state - set the device configuration state
 *
 * @param[in]	dev_hndl:	device handle
 * @param[in]	new_cfg_state:	dma device conf state to set
 *
 *
 * @return	0 on success ,<0 on failure
 *****************************************************************************/
int qdma_device_set_cfg_state(unsigned long dev_hndl,
			      enum qdma_dev_qmax_state new_cfg_state);

/*****************************************************************************/
/**
 * qdma_device_sriov_config() - configure sriov
 *
 * @param[in]	pdev:		ptr to struct pci_dev
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	num_vfs:	# of VFs to be instantiated
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_sriov_config(struct pci_dev *pdev, unsigned long dev_hndl,
				int num_vfs);

/*****************************************************************************/
/**
 * qdma_device_read_config_register() - read dma config. register
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	reg_addr:	register address
 *
 * @return	value of the config register
 *****************************************************************************/
unsigned int qdma_device_read_config_register(unsigned long dev_hndl,
					unsigned int reg_addr);

/*****************************************************************************/
/**
 * qdma_device_write_config_register() - write dma config. register
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	reg_addr:	register address
 * @param[in]	value:		register value to be writen
 *
 *****************************************************************************/
void qdma_device_write_config_register(unsigned long dev_hndl,
					unsigned int reg_addr, u32 value);


/**
 * qdma_version_info defines the per-device version information
 *
 */
#define DEVICE_VERSION_INFO_STR_LENGTH            10
struct qdma_version_info {
	u8 rtl_version;
	char rtl_version_str[DEVICE_VERSION_INFO_STR_LENGTH];
	u8 vivado_release_id;
	char vivado_release_str[DEVICE_VERSION_INFO_STR_LENGTH];
	u8 everest_ip;
	char everest_ip_str[DEVICE_VERSION_INFO_STR_LENGTH];
};

/*****************************************************************************/
/**
 * qdma_device_version_info() - retrieve the RTL version , Vivado Release ID and
 *				Everest IP info
 *
 * @param[in]	dev_hndl:	handle returned from qdma_device_open()
 * @param[out]	version_info: pointer to hold all the version details
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_device_version_info(unsigned long dev_hndl,
			     struct qdma_version_info *version_info);

/*****************************************************************************/
/**
 * qdma_software_version_info() - retrieve the software version
 *
 * @param[out]	software_version: A pointer to a null-terminated string
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_software_version_info(char *software_version);

/** QDMA Global CSR array size */
#define QDMA_GLOBAL_CSR_ARRAY_SZ        16

/**
 * struct global_csr_conf - global CSR configuration
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
	/** wireback acculation enable/disable */
	unsigned int cmpl_status_acc;
};

/*****************************************************************************/
/**
 * qdma_glbal_csr_get() - retrieve the global csr settings
 *
 * @param[in]	dev_hndl:	handle returned from qdma_device_open()
 * @param[out]	csr:		data structures to hold the csr values
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_global_csr_get(unsigned long dev_hndl, struct global_csr_conf *csr);

/*****************************************************************************/
/**
 * qdma_glbal_csr_set() - set the global csr values
 * NOTE: for set, libqdma will enforce the access control
 *
 * @param[in]	dev_hndl:	handle returned from qdma_device_open()
 * @param[in]	csr:		data structures to hold the csr values
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_global_csr_set(unsigned long dev_hndl, struct global_csr_conf *csr);

/**
 * cmpt_desc_sz_t - descriptor sizes
 */
enum cmpt_desc_sz_t {
	/** 0 - completion size 8B */
	CMPT_DESC_SZ_8B = 0,
	/** 0 - completion size 16B */
	CMPT_DESC_SZ_16B,
	/** 0 - completion size 32B */
	CMPT_DESC_SZ_32B,
	/** 0 - completion size 64B */
	CMPT_DESC_SZ_64B
};

/**
 * desc_sz_t - descriptor sizes
 */
enum desc_sz_t {
	/** 0 - size 8B */
	DESC_SZ_8B = 0,
	/** 0 - size 16B */
	DESC_SZ_16B,
	/** 0 - size 32B */
	DESC_SZ_32B,
	/** 0 - size 64B */
	DESC_SZ_64B
};

/**
 * tigger_mode_t - trigger modes
 */
enum tigger_mode_t {
	/** 0 - disable trigger mode */
	TRIG_MODE_DISABLE,
	/** 1 - any trigger mode */
	TRIG_MODE_ANY,
	/** 2 - counter trigger mode */
	TRIG_MODE_COUNTER,
	/** 3 - trigger mode of user choice */
	TRIG_MODE_USER,
	/** 4 - timer trigger mode */
	TRIG_MODE_TIMER,
	/** 5 - timer and counter combo trigger mode */
	TRIG_MODE_COMBO,
};

/**
 * struct qdma_sw_sg - qdma scatter gather request
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

/**
 *	maximum queue name length
 */
#define QDMA_QUEUE_NAME_MAXLEN	32
/**
 *	invalid queue index
 */
#define QDMA_QUEUE_IDX_INVALID	0xFFFF

/**
 *	invalid MSI-x vector index
 */
#define QDMA_QUEUE_VEC_INVALID	0xFF

/**
 * struct qdma_queue_conf - qdma configuration parameters
 * qdma_queue_conf defines the per-dma Q property.
 * if any of the max requested is less than supported, the value will
 * be updated
 */
struct qdma_queue_conf {
	/** 0xFFFF: libqdma choose the queue idx
	 *	0 ~ (qdma_dev_conf.qsets_max - 1)
	 *	the calling function choose the queue idx
	 */
	u32 qidx:24;

	/** config flags: byte #1 */
	/** st mode */
	u32 st:1;
	/** c2h direction */
	u32 c2h:1;
	/** SDx only: inter-kernel communication pipe */
	u32 pipe:1;
	/** poll or interrupt */
	u32 irq_en:1;

	/** descriptor ring	 */
	/** global_csr_conf.ringsz[N] */
	u32 desc_rng_sz_idx:4;

	/** config flags: byte #2 */
	/** writeback enable, disabled for ST C2H */
	u8 cmpl_status_en:1;
	/** sw context.cmpl_status_acc_en */
	u8 cmpl_status_acc_en:1;
	/** sw context.cmpl_stauts_pend_chk */
	u8 cmpl_status_pend_chk:1;
	/** send descriptor to bypass out */
	u8 desc_bypass:1;
	/** descriptor prefetch enable control */
	u8 pfetch_en:1;
	/** sw context.frcd_en[32] */
	u8 fetch_credit:1;
	/** SDx only: ST packet mode (i.e., with TLAST
	 * to identify the packet boundary)
	 */
	u8 st_pkt_mode:1;
	/** c2h use pre-alloc free list */
	u8 c2h_use_fl:1;

	/** config flags: byte #3 */
	/** global_csr_conf.c2h_buf_sz[N] */
	u8 c2h_buf_sz_idx:4;

	/**  ST C2H Completion/Writeback ring */
	/** global_csr_conf.ringsz[N] */
	u8 cmpl_rng_sz_idx:4;

	/** config flags: byte #4 */
	/** C2H ST cmpt + immediate data, desc_sz_t */
	u8 cmpl_desc_sz:2;
	/** enable status desc. for CMPT */
	u8 cmpl_stat_en:1;
	/** C2H Completion entry user-defined data */
	u8 cmpl_udd_en:1;
	/** global_csr_conf.c2h_timer_cnt[N] */
	u8 cmpl_timer_idx:4;

	/** config flags: byte #5 */
	/** global_csr_conf.c2h_cnt_th[N] */
	u8 cmpl_cnt_th_idx:4;
	/** tigger_mode_t */
	u8 cmpl_trig_mode:3;
	/** enable interrupt for CMPT */
	u8 cmpl_en_intr:1;

	/** config flags: byte #6 */
	/** SW Context desc size, desc_sz_t */
	u8 sw_desc_sz:2;
	/** prefetch bypass en */
	u8 pfetch_bypass:1;
	/** OVF_DIS C2H ST over flow disable */
	u8 cmpl_ovf_chk_dis:1;
	/** Port ID */
	u8 port_id:3;
	/** Address Translation */
	u8 at:1;

	/*
	 * TODO: for Platform streaming DSA
	 */
	/** only if pipe = 1 */
	/** max 16. CDH length per packet */
	u8 cdh_max;
	/** <= 7, max # gather buf. per packet */
	u8 pipe_gl_max;
	/** pipe flow id */
	u8 pipe_flow_id;
	/** pipe SLR id */
	u8 pipe_slr_id;
	/** pipe route id */
	u16 pipe_tdest;

	/** user provided per-Q irq handler */
	unsigned long quld;		/* set by user for per Q data */

	/** TBA: Q interrupt top, per-queue additional handling code
	 * for example, network rx: napi_schedule(&Q->napi)
	 */
	void (*fp_descq_isr_top)(unsigned long qhndl, unsigned long quld);
	/**
	 * optional rx packet handler:
	 *	 called from irq BH (i.e.qdma_queue_service_bh())
	 * - udd: user defined data in the completion entry
	 * - sgcnt / sgl: packet data in scatter-gather list
	 *   NOTE: a. do NOT modify any field of sgl
	 *	   b. if zero copy, do a get_page() to prevent page freeing
	 *	   c. do loop through the sgl with sg->next and stop
	 *	      at sgcnt. the last sg may not have sg->next = NULL
	 * Returns:
	 *	- 0 to allow libqdma free/re-task the sgl
	 *	- < 0, libqdma will keep the packet for processing again
	 *
	 * A single packet could contain:
	 * in the case of c2h_udd_en = 1:
	 * - udd only (udd valid, sgcnt = 0, sgl = NULL), or
	 * - udd + packdet data
	 * in the case of c2h_udd_en = 0:
	 * - packet data (udd = NULL, sgcnt > 0 and sgl valid)
	 */
	int (*fp_descq_c2h_packet)(unsigned long qhndl, unsigned long quld,
				unsigned int len, unsigned int sgcnt,
				struct qdma_sw_sg *sgl, void *udd);

	/** fill in by libqdma */
	/** name of the qdma device */
	char name[QDMA_QUEUE_NAME_MAXLEN];
	/** ring size of the queue */
	unsigned int rngsz;
	/** completion ring size of the queue */
	unsigned int rngsz_cmpt;
	/** C2H buffer size */
	unsigned int c2h_bufsz;
};

/*****************************************************************************/
/**
 * qdma_queue_add() - add a queue
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qconf:		queue configuration parameters
 * @param[in]	qhndl:	list of unsigned long values that are the opaque qhndl
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer

 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_add(unsigned long dev_hndl, struct qdma_queue_conf *qconf,
			unsigned long *qhndl, char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_queue_reconfig() - reconfigure the queue
 *
 * @param[in]	dev_hndl:	qdma device handle
 * @param[in]	id:		queue index
 * @param[in]	qconf:		queue configuration
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_queue_reconfig(unsigned long dev_hndl, unsigned long id,
			struct qdma_queue_conf *qconf, char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_queue_start() - start a queue (i.e, online, ready for dma)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		the opaque qhndl
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_start(unsigned long dev_hndl, unsigned long id,
						char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_queue_stop() - stop a queue (i.e., offline, NOT ready for dma)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		the opaque qhndl
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_stop(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/*****************************************************************************/
/**
 *  * qdma_queue_prog_stm() - Program STM for queue (context, map, etc)
 *   *
 * @param[in]   dev_hndl:       dev_hndl returned from qdma_device_open()
 * @param[in]   id:             queue index
 * @param[in]   buflen:         length of the input buffer
 * @param[out]  buf:            message buffer
 *
 * @return      0: success
 * @return      <0: error
 *****************************************************************************/
int qdma_queue_prog_stm(unsigned long dev_hndl, unsigned long id, char *buf,
			int buflen);

/*****************************************************************************/
/**
 * qdma_queue_remove() - remove a queue
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		the opaque qhndl
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_remove(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/**
 * queue helper/debug functions
 */

/*****************************************************************************/
/**
 * qdma_queue_get_config() - retrieve the configuration of a queue
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		an opaque queue handle of type unsigned long
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
struct qdma_queue_conf *qdma_queue_get_config(unsigned long dev_hndl,
				unsigned long id, char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_queue_list() - display all configured queues in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_list(unsigned long dev_hndl, char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_queue_dump() - display a queue's state in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		an opaque queue handle of type unsigned long
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * qdma_queue_dump_desc() - display a queue's descriptor ring from index start
 *							~ end in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		an opaque queue handle of type unsigned long
 * @param[in]	start:		start index
 * @param[in]	end:		end index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump_desc(unsigned long dev_hndl, unsigned long id,
				unsigned int start, unsigned int end, char *buf,
				int buflen);

/*****************************************************************************/
/**
 * qdma_queue_dump_cmpt() - display a queue's descriptor ring from index start
 *							~ end in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		an opaque queue handle of type unsigned long
 * @param[in]	start:		start index
 * @param[in]	end:		end index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump_cmpt(unsigned long dev_hndl, unsigned long id,
				unsigned int start, unsigned int end, char *buf,
				int buflen);

#ifdef ERR_DEBUG
/*****************************************************************************/
/**
 * qdma_queue_set_err_induction() - Induce the error
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		error id
 * @param[in]	err:		error info
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_set_err_induction(unsigned long dev_hndl, unsigned long id,
				 u32 err, char *buf, int buflen);
#endif


/**
 * maximum request length
 */
#define QDMA_REQ_OPAQUE_SIZE	72
/**
 * Max length of the user defined data
 */
#define QDMA_UDD_MAXLEN		32

/**
 * struct qdma_request - qdma request for read or write
 */
struct qdma_request {
	/** private to the dma driver, do NOT touch */
	unsigned char opaque[QDMA_REQ_OPAQUE_SIZE];
	/** filled in by the calling function */
	/** for the calling function */
	unsigned long uld_data;
	/** set fp_done for non-blocking mode */
	int (*fp_done)(struct qdma_request *, unsigned int bytes_done, int err);
	/** timeout in mili-seconds, 0 - no timeout */
	unsigned int timeout_ms;
	/** total data size */
	unsigned int count;
	/** MM only, DDR/BRAM memory addr */
	u64 ep_addr;
	/* flag to indicate if memcpy is required */
	u8 no_memcpy:1;
	/** write: if write to the device */
	u8 write:1;
	/** if sgt is already dma mapped */
	u8 dma_mapped:1;
	/** user defined data present */
	u8 h2c_eot:1;
	/** indicates end of transfer towards user kernel */
	u8 udd_len;
	/** # of scatter-gather entries < 64K */
	unsigned int sgcnt;
	/** scatter-gather list of data bufs */
	struct qdma_sw_sg *sgl;
	/** udd data */
	u8 udd[QDMA_UDD_MAXLEN];
};

/*****************************************************************************/
/**
 * qdma_request_submit() - submit a scatter-gather list of data for dma
 * operation (for both read and write)
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	id:			queue index
 * @param[in]	req:		qdma request
 *
 * @return	# of bytes transferred
 * @return	<0: error
 *****************************************************************************/
ssize_t qdma_request_submit(unsigned long dev_hndl, unsigned long id,
			struct qdma_request *req);

/*****************************************************************************/
/**
 * qdma_batch_request_submit() - submit a scatter-gather list of data for dma
 * operation (for both read and write)
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	id:			queue index
 * @param[in]	count:			number of requests
 * @param[in]	reqv:		qdma request
 *
 * @return	# of bytes transferred
 * @return	<0: error
 *****************************************************************************/
ssize_t qdma_batch_request_submit(unsigned long dev_hndl, unsigned long id,
			  unsigned long count, struct qdma_request **reqv);

/*****************************************************************************/
/**
 * qdma_queue_c2h_peek() - peek a receive (c2h) queue
 *
 * @param	dev_hndl:	hndl returned from qdma_device_open()
 * @param	qhndl:		hndl returned from qdma_queue_add()
 *
 * filled in by libqdma:
 * @param[in]	udd_cnt:	# of udd received
 * @param[in]	pkt_cnt:	# of packets received
 * @param[in]	data_len:	# of bytes of packet data received
 *
 * @return	0: success and # of packets received in the Q
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_c2h_peek(unsigned long dev_hndl, unsigned long qhndl,
			unsigned int *udd_cnt, unsigned int *pkt_cnt,
			unsigned int *data_len);

/*****************************************************************************/
/**
 * qdma_queue_avail_desc() - query of # of free descriptor
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	qhndl:		hndl returned from qdma_queue_add()
 *
 * @return	# of available desc in the queue
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_avail_desc(unsigned long dev_hndl, unsigned long qhndl);

/** packet/streaming interfaces  */

/**
 * struct qdma_cmpl_ctrl - completion control
 */
struct qdma_cmpl_ctrl {
	/** global_csr_conf.c2h_cnt_th[N] */
	u8 cnt_th_idx:4;
	/** global_csr_conf.c2h_timer_cnt[N] */
	u8 timer_idx:4;
	/** tigger_mode_t */
	u8 trigger_mode:3;
	/** enable status desc. for CMPT */
	u8 en_stat_desc:1;
	/** enable interrupt for CMPT */
	u8 cmpl_en_intr:1;
};

/*****************************************************************************/
/**
 * qdma_queue_cmpl_ctrl() - read/set the c2h Q's completion control
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	qhndl:		hndl returned from qdma_queue_add()
 * @param[in]	cctrl:		completion control
 * @param[in]	set:		read or set
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_cmpl_ctrl(unsigned long dev_hndl, unsigned long qhndl,
				struct qdma_cmpl_ctrl *cctrl, bool set);

/*****************************************************************************/
/**
 * qdma_queue_packet_read() - read rcv'ed data (ST C2H dma operation)
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	qhndl:		hndl returned from qdma_queue_add()
 * @param[in]	req:		pointer to the request data
 * @param[out]	cctrl:		completion control, if no change is desired,
 *                      set it to NULL
 *
 * @return	number of packets read
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_packet_read(unsigned long dev_hndl, unsigned long qhndl,
			struct qdma_request *req, struct qdma_cmpl_ctrl *cctrl);

/*****************************************************************************/
/**
 * qdma_queue_packet_write() - submit data for h2c dma operation
 *
 * @param[in]	dev_hndl:	hndl returned from qdma_device_open()
 * @param[in]	qhndl:		hndl returned from qdma_queue_add()
 * @param[in]	req:		pointer to the list of packet data
 *
 * @return	number of desc posted
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_packet_write(unsigned long dev_hndl, unsigned long qhndl,
			struct qdma_request *req);

/*****************************************************************************/
/**
 * qdma_queue_service() - service the queue
 *	in the case of irq handler is registered by the user, the user should
 *	call qdma_queue_service() in its interrupt handler to service the queue
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qhndl:		hndl returned from qdma_queue_add()
 * @param[in]	budget:		ST C2H only, max # of completions
 *						to be processed. 0 - no limit
 * @param[in]	c2h_upd_cmpl:   To keep intrrupt disabled, set to false,
 *				Set to true to re-enable the interrupt:
 *					ST C2H only, max # of completions
 *					to be processed. 0 - no limit
 *
 *****************************************************************************/
void qdma_queue_service(unsigned long dev_hndl, unsigned long qhndl,
			int budget, bool c2h_upd_cmpl);

/*****************************************************************************/
/**
 * qdma_intr_ring_dump() - display the interrupt ring info of a vector
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	vector_idx:	vector number
 * @param[in]	start_idx:	interrupt ring start idx
 * @param[in]	end_idx:	interrupt ring end idx
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message bufferuffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_intr_ring_dump(unsigned long dev_hndl, unsigned int vector_idx,
			int start_idx, int end_idx, char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_descq_get_cmpt_udd() - function to receive the user defined data
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qhndl:		queue handle
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message bufferuffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/

int qdma_descq_get_cmpt_udd(unsigned long dev_hndl, unsigned long qhndl,
		char *buf, int buflen);

#ifdef __QDMA_VF__
/*****************************************************************************/
/**
 * qdma_vf_qconf	call for VF to request qmax number of Qs
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qmax:	number of qs requested by vf
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_vf_qconf(unsigned long dev_hndl, int qmax);
#endif

#endif
