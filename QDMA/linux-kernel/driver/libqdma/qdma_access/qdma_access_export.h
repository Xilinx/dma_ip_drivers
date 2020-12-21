/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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

#ifndef __QDMA_ACCESS_EXPORT_H_
#define __QDMA_ACCESS_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform_env.h"

/** QDMA Global CSR array size */
#define QDMA_GLOBAL_CSR_ARRAY_SZ        16

/**
 * struct qdma_dev_attributes - QDMA device attributes
 */
struct qdma_dev_attributes {
	/** @num_pfs - Num of PFs*/
	uint8_t num_pfs;
	/** @num_qs - Num of Queues */
	uint16_t num_qs;
	/** @flr_present - FLR resent or not? */
	uint8_t flr_present:1;
	/** @st_en - ST mode supported or not? */
	uint8_t st_en:1;
	/** @mm_en - MM mode supported or not? */
	uint8_t mm_en:1;
	/** @mm_cmpt_en - MM with Completions supported or not? */
	uint8_t mm_cmpt_en:1;
	/** @mailbox_en - Mailbox supported or not? */
	uint8_t mailbox_en:1;
	/** @debug_mode - Debug mode is enabled/disabled for IP */
	uint8_t debug_mode:1;
	/** @desc_eng_mode - Descriptor Engine mode:
	 * Internal only/Bypass only/Internal & Bypass
	 */
	uint8_t desc_eng_mode:2;
	/** @mm_channel_max - Num of MM channels */
	uint8_t mm_channel_max;

	/** Below are the list of HW features which are populated by qdma_access
	 * based on RTL version
	 */
	/** @qid2vec_ctx - To indicate support of qid2vec context */
	uint8_t qid2vec_ctx:1;
	/** @cmpt_ovf_chk_dis - To indicate support of overflow check
	 * disable in CMPT ring
	 */
	uint8_t cmpt_ovf_chk_dis:1;
	/** @mailbox_intr - To indicate support of mailbox interrupt */
	uint8_t mailbox_intr:1;
	/** @sw_desc_64b - To indicate support of 64 bytes C2H/H2C
	 * descriptor format
	 */
	uint8_t sw_desc_64b:1;
	/** @cmpt_desc_64b - To indicate support of 64 bytes CMPT
	 * descriptor format
	 */
	uint8_t cmpt_desc_64b:1;
	/** @dynamic_bar - To indicate support of dynamic bar detection */
	uint8_t dynamic_bar:1;
	/** @legacy_intr - To indicate support of legacy interrupt */
	uint8_t legacy_intr:1;
	/** @cmpt_trig_count_timer - To indicate support of counter + timer
	 * trigger mode
	 */
	uint8_t cmpt_trig_count_timer:1;
};

/** qdma_dev_attributes structure size */
#define QDMA_DEV_ATTR_STRUCT_SIZE	(sizeof(struct qdma_dev_attributes))

/** global_csr_conf structure size */
#define QDMA_DEV_GLOBAL_CSR_STRUCT_SIZE	(sizeof(struct global_csr_conf))

/**
 * enum qdma_dev_type - To hold qdma device type
 */
enum qdma_dev_type {
	QDMA_DEV_PF,
	QDMA_DEV_VF
};

/**
 * enum qdma_dev_q_type: Q type
 */
enum qdma_dev_q_type {
	/** @QDMA_DEV_Q_TYPE_H2C: H2C Q */
	QDMA_DEV_Q_TYPE_H2C,
	/** @QDMA_DEV_Q_TYPE_C2H: C2H Q */
	QDMA_DEV_Q_TYPE_C2H,
	/** @QDMA_DEV_Q_TYPE_CMPT: CMPT Q */
	QDMA_DEV_Q_TYPE_CMPT,
	/** @QDMA_DEV_Q_TYPE_MAX: Total Q types */
	QDMA_DEV_Q_TYPE_MAX
};

/**
 * @enum qdma_desc_size - QDMA queue descriptor size
 */
enum qdma_desc_size {
	/** @QDMA_DESC_SIZE_8B - 8 byte descriptor */
	QDMA_DESC_SIZE_8B,
	/** @QDMA_DESC_SIZE_16B - 16 byte descriptor */
	QDMA_DESC_SIZE_16B,
	/** @QDMA_DESC_SIZE_32B - 32 byte descriptor */
	QDMA_DESC_SIZE_32B,
	/** @QDMA_DESC_SIZE_64B - 64 byte descriptor */
	QDMA_DESC_SIZE_64B
};

/**
 * @enum qdma_cmpt_update_trig_mode - Interrupt and Completion status write
 * trigger mode
 */
enum qdma_cmpt_update_trig_mode {
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_DIS - disabled */
	QDMA_CMPT_UPDATE_TRIG_MODE_DIS,
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_EVERY - every */
	QDMA_CMPT_UPDATE_TRIG_MODE_EVERY,
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_USR_CNT - user counter */
	QDMA_CMPT_UPDATE_TRIG_MODE_USR_CNT,
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_USR - user */
	QDMA_CMPT_UPDATE_TRIG_MODE_USR,
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_USR_TMR - user timer */
	QDMA_CMPT_UPDATE_TRIG_MODE_USR_TMR,
	/** @QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR - timer + counter combo */
	QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR
};


/**
 * @enum qdma_indirect_intr_ring_size - Indirect interrupt ring size
 */
enum qdma_indirect_intr_ring_size {
	/** @QDMA_INDIRECT_INTR_RING_SIZE_4KB - Accommodates 512 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_4KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_8KB - Accommodates 1024 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_8KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_12KB - Accommodates 1536 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_12KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_16KB - Accommodates 2048 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_16KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_20KB - Accommodates 2560 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_20KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_24KB - Accommodates 3072 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_24KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_28KB - Accommodates 3584 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_28KB,
	/** @QDMA_INDIRECT_INTR_RING_SIZE_32KB - Accommodates 4096 entries */
	QDMA_INDIRECT_INTR_RING_SIZE_32KB
};

/**
 * @enum qdma_wrb_interval - writeback update interval
 */
enum qdma_wrb_interval {
	/** @QDMA_WRB_INTERVAL_4 - writeback update interval of 4 */
	QDMA_WRB_INTERVAL_4,
	/** @QDMA_WRB_INTERVAL_8 - writeback update interval of 8 */
	QDMA_WRB_INTERVAL_8,
	/** @QDMA_WRB_INTERVAL_16 - writeback update interval of 16 */
	QDMA_WRB_INTERVAL_16,
	/** @QDMA_WRB_INTERVAL_32 - writeback update interval of 32 */
	QDMA_WRB_INTERVAL_32,
	/** @QDMA_WRB_INTERVAL_64 - writeback update interval of 64 */
	QDMA_WRB_INTERVAL_64,
	/** @QDMA_WRB_INTERVAL_128 - writeback update interval of 128 */
	QDMA_WRB_INTERVAL_128,
	/** @QDMA_WRB_INTERVAL_256 - writeback update interval of 256 */
	QDMA_WRB_INTERVAL_256,
	/** @QDMA_WRB_INTERVAL_512 - writeback update interval of 512 */
	QDMA_WRB_INTERVAL_512,
	/** @QDMA_NUM_WRB_INTERVALS - total number of writeback intervals */
	QDMA_NUM_WRB_INTERVALS
};

enum qdma_rtl_version {
	/** @QDMA_RTL_BASE - RTL Base  */
	QDMA_RTL_BASE,
	/** @QDMA_RTL_PATCH - RTL Patch  */
	QDMA_RTL_PATCH,
	/** @QDMA_RTL_NONE - Not a valid RTL version */
	QDMA_RTL_NONE,
};

enum qdma_vivado_release_id {
	/** @QDMA_VIVADO_2018_3 - Vivado version 2018.3  */
	QDMA_VIVADO_2018_3,
	/** @QDMA_VIVADO_2019_1 - Vivado version 2019.1  */
	QDMA_VIVADO_2019_1,
	/** @QDMA_VIVADO_2019_2 - Vivado version 2019.2  */
	QDMA_VIVADO_2019_2,
	/** @QDMA_VIVADO_2020_1 - Vivado version 2020.1  */
	QDMA_VIVADO_2020_1,
	/** @QDMA_VIVADO_2020_2 - Vivado version 2020.2  */
	QDMA_VIVADO_2020_2,
	/** @QDMA_VIVADO_NONE - Not a valid Vivado version*/
	QDMA_VIVADO_NONE
};

enum qdma_ip_type {
	/** @QDMA_VERSAL_HARD_IP - Hard IP  */
	QDMA_VERSAL_HARD_IP,
	/** @QDMA_VERSAL_SOFT_IP - Soft IP  */
	QDMA_VERSAL_SOFT_IP,
	/** @QDMA_SOFT_IP - Hard IP  */
	QDMA_SOFT_IP,
	/** @EQDMA_SOFT_IP - Soft IP  */
	EQDMA_SOFT_IP,
	/** @QDMA_VERSAL_NONE - Not versal device  */
	QDMA_NONE_IP
};


enum qdma_device_type {
	/** @QDMA_DEVICE_SOFT - UltraScale+ IP's  */
	QDMA_DEVICE_SOFT,
	/** @QDMA_DEVICE_VERSAL -VERSAL IP  */
	QDMA_DEVICE_VERSAL,
	/** @QDMA_DEVICE_NONE - Not a valid device  */
	QDMA_DEVICE_NONE
};

enum qdma_desc_eng_mode {
	/** @QDMA_DESC_ENG_INTERNAL_BYPASS - Internal and Bypass mode */
	QDMA_DESC_ENG_INTERNAL_BYPASS,
	/** @QDMA_DESC_ENG_BYPASS_ONLY - Only Bypass mode  */
	QDMA_DESC_ENG_BYPASS_ONLY,
	/** @QDMA_DESC_ENG_INTERNAL_ONLY - Only Internal mode  */
	QDMA_DESC_ENG_INTERNAL_ONLY,
	/** @QDMA_DESC_ENG_MODE_MAX - Max of desc engine modes  */
	QDMA_DESC_ENG_MODE_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_ACCESS_EXPORT_H_ */
