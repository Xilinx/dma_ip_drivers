/*
 * Copyright (c) 2019-2022, Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022, Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __EQDMA_SOFT_ACCESS_H_
#define __EQDMA_SOFT_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform.h"

/**
 * enum qdma_error_idx - qdma errors
 */
enum eqdma_error_idx {
	/* Descriptor errors */
	EQDMA_DSC_ERR_POISON,
	EQDMA_DSC_ERR_UR_CA,
	EQDMA_DSC_ERR_BCNT,
	EQDMA_DSC_ERR_PARAM,
	EQDMA_DSC_ERR_ADDR,
	EQDMA_DSC_ERR_TAG,
	EQDMA_DSC_ERR_FLR,
	EQDMA_DSC_ERR_TIMEOUT,
	EQDMA_DSC_ERR_DAT_POISON,
	EQDMA_DSC_ERR_FLR_CANCEL,
	EQDMA_DSC_ERR_DMA,
	EQDMA_DSC_ERR_DSC,
	EQDMA_DSC_ERR_RQ_CANCEL,
	EQDMA_DSC_ERR_DBE,
	EQDMA_DSC_ERR_SBE,
	EQDMA_DSC_ERR_PORT_ID,
	EQDMA_DSC_ERR_ALL,

	/* TRQ Errors */
	EQDMA_TRQ_ERR_CSR_UNMAPPED,
	EQDMA_TRQ_ERR_VF_ACCESS,
	EQDMA_TRQ_ERR_TCP_CSR_TIMEOUT,
	EQDMA_TRQ_ERR_QSPC_UNMAPPED,
	EQDMA_TRQ_ERR_QID_RANGE,
	EQDMA_TRQ_ERR_TCP_QSPC_TIMEOUT,
	EQDMA_TRQ_ERR_ALL,

	/* C2H Errors */
	EQDMA_ST_C2H_ERR_MTY_MISMATCH,
	EQDMA_ST_C2H_ERR_LEN_MISMATCH,
	EQDMA_ST_C2H_ERR_SH_CMPT_DSC,
	EQDMA_ST_C2H_ERR_QID_MISMATCH,
	EQDMA_ST_C2H_ERR_DESC_RSP_ERR,
	EQDMA_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
	EQDMA_ST_C2H_ERR_MSI_INT_FAIL,
	EQDMA_ST_C2H_ERR_ERR_DESC_CNT,
	EQDMA_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
	EQDMA_ST_C2H_ERR_CMPT_INV_Q_ERR,
	EQDMA_ST_C2H_ERR_CMPT_QFULL_ERR,
	EQDMA_ST_C2H_ERR_CMPT_CIDX_ERR,
	EQDMA_ST_C2H_ERR_CMPT_PRTY_ERR,
	EQDMA_ST_C2H_ERR_AVL_RING_DSC,
	EQDMA_ST_C2H_ERR_HDR_ECC_UNC,
	EQDMA_ST_C2H_ERR_HDR_ECC_COR,
	EQDMA_ST_C2H_ERR_WRB_PORT_ID_ERR,
	EQDMA_ST_C2H_ERR_ALL,

	/* Fatal Errors */
	EQDMA_ST_FATAL_ERR_MTY_MISMATCH,
	EQDMA_ST_FATAL_ERR_LEN_MISMATCH,
	EQDMA_ST_FATAL_ERR_QID_MISMATCH,
	EQDMA_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_CMPT_CTXT_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_CMPT_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_WPL_DATA_PAR,
	EQDMA_ST_FATAL_ERR_AVL_RING_FIFO_RAM_RDBE,
	EQDMA_ST_FATAL_ERR_HDR_ECC_UNC,
	EQDMA_ST_FATAL_ERR_ALL,

	/* H2C Errors */
	EQDMA_ST_H2C_ERR_ZERO_LEN_DESC,
	EQDMA_ST_H2C_ERR_SDI_MRKR_REQ_MOP,
	EQDMA_ST_H2C_ERR_NO_DMA_DSC,
	EQDMA_ST_H2C_ERR_SBE,
	EQDMA_ST_H2C_ERR_DBE,
	EQDMA_ST_H2C_ERR_PAR,
	EQDMA_ST_H2C_ERR_ALL,

	/* Single bit errors */
	EQDMA_SBE_1_ERR_RC_RRQ_EVEN_RAM,
	EQDMA_SBE_1_ERR_TAG_ODD_RAM,
	EQDMA_SBE_1_ERR_TAG_EVEN_RAM,
	EQDMA_SBE_1_ERR_PFCH_CTXT_CAM_RAM_0,
	EQDMA_SBE_1_ERR_PFCH_CTXT_CAM_RAM_1,
	EQDMA_SBE_1_ERR_ALL,

	/* Single bit errors */
	EQDMA_SBE_ERR_MI_H2C0_DAT,
	EQDMA_SBE_ERR_MI_H2C1_DAT,
	EQDMA_SBE_ERR_MI_H2C2_DAT,
	EQDMA_SBE_ERR_MI_H2C3_DAT,
	EQDMA_SBE_ERR_MI_C2H0_DAT,
	EQDMA_SBE_ERR_MI_C2H1_DAT,
	EQDMA_SBE_ERR_MI_C2H2_DAT,
	EQDMA_SBE_ERR_MI_C2H3_DAT,
	EQDMA_SBE_ERR_H2C_RD_BRG_DAT,
	EQDMA_SBE_ERR_H2C_WR_BRG_DAT,
	EQDMA_SBE_ERR_C2H_RD_BRG_DAT,
	EQDMA_SBE_ERR_C2H_WR_BRG_DAT,
	EQDMA_SBE_ERR_FUNC_MAP,
	EQDMA_SBE_ERR_DSC_HW_CTXT,
	EQDMA_SBE_ERR_DSC_CRD_RCV,
	EQDMA_SBE_ERR_DSC_SW_CTXT,
	EQDMA_SBE_ERR_DSC_CPLI,
	EQDMA_SBE_ERR_DSC_CPLD,
	EQDMA_SBE_ERR_MI_TL_SLV_FIFO_RAM,
	EQDMA_SBE_ERR_TIMER_FIFO_RAM,
	EQDMA_SBE_ERR_QID_FIFO_RAM,
	EQDMA_SBE_ERR_WRB_COAL_DATA_RAM,
	EQDMA_SBE_ERR_INT_CTXT_RAM,
	EQDMA_SBE_ERR_DESC_REQ_FIFO_RAM,
	EQDMA_SBE_ERR_PFCH_CTXT_RAM,
	EQDMA_SBE_ERR_WRB_CTXT_RAM,
	EQDMA_SBE_ERR_PFCH_LL_RAM,
	EQDMA_SBE_ERR_PEND_FIFO_RAM,
	EQDMA_SBE_ERR_RC_RRQ_ODD_RAM,
	EQDMA_SBE_ERR_ALL,

	/* Double bit Errors */
	EQDMA_DBE_1_ERR_RC_RRQ_EVEN_RAM,
	EQDMA_DBE_1_ERR_TAG_ODD_RAM,
	EQDMA_DBE_1_ERR_TAG_EVEN_RAM,
	EQDMA_DBE_1_ERR_PFCH_CTXT_CAM_RAM_0,
	EQDMA_DBE_1_ERR_PFCH_CTXT_CAM_RAM_1,
	EQDMA_DBE_1_ERR_ALL,

	/* Double bit Errors */
	EQDMA_DBE_ERR_MI_H2C0_DAT,
	EQDMA_DBE_ERR_MI_H2C1_DAT,
	EQDMA_DBE_ERR_MI_H2C2_DAT,
	EQDMA_DBE_ERR_MI_H2C3_DAT,
	EQDMA_DBE_ERR_MI_C2H0_DAT,
	EQDMA_DBE_ERR_MI_C2H1_DAT,
	EQDMA_DBE_ERR_MI_C2H2_DAT,
	EQDMA_DBE_ERR_MI_C2H3_DAT,
	EQDMA_DBE_ERR_H2C_RD_BRG_DAT,
	EQDMA_DBE_ERR_H2C_WR_BRG_DAT,
	EQDMA_DBE_ERR_C2H_RD_BRG_DAT,
	EQDMA_DBE_ERR_C2H_WR_BRG_DAT,
	EQDMA_DBE_ERR_FUNC_MAP,
	EQDMA_DBE_ERR_DSC_HW_CTXT,
	EQDMA_DBE_ERR_DSC_CRD_RCV,
	EQDMA_DBE_ERR_DSC_SW_CTXT,
	EQDMA_DBE_ERR_DSC_CPLI,
	EQDMA_DBE_ERR_DSC_CPLD,
	EQDMA_DBE_ERR_MI_TL_SLV_FIFO_RAM,
	EQDMA_DBE_ERR_TIMER_FIFO_RAM,
	EQDMA_DBE_ERR_QID_FIFO_RAM,
	EQDMA_DBE_ERR_WRB_COAL_DATA_RAM,
	EQDMA_DBE_ERR_INT_CTXT_RAM,
	EQDMA_DBE_ERR_DESC_REQ_FIFO_RAM,
	EQDMA_DBE_ERR_PFCH_CTXT_RAM,
	EQDMA_DBE_ERR_WRB_CTXT_RAM,
	EQDMA_DBE_ERR_PFCH_LL_RAM,
	EQDMA_DBE_ERR_PEND_FIFO_RAM,
	EQDMA_DBE_ERR_RC_RRQ_ODD_RAM,
	EQDMA_DBE_ERR_ALL,

	/* MM C2H Errors */
	EQDMA_MM_C2H_WR_SLR_ERR,
	EQDMA_MM_C2H_RD_SLR_ERR,
	EQDMA_MM_C2H_WR_FLR_ERR,
	EQDMA_MM_C2H_UR_ERR,
	EQDMA_MM_C2H_WR_UC_RAM_ERR,
	EQDMA_MM_C2H_ERR_ALL,

	/* MM H2C Engine0 Errors */
	EQDMA_MM_H2C0_RD_HDR_POISON_ERR,
	EQDMA_MM_H2C0_RD_UR_CA_ERR,
	EQDMA_MM_H2C0_RD_HDR_BYTE_ERR,
	EQDMA_MM_H2C0_RD_HDR_PARAM_ERR,
	EQDMA_MM_H2C0_RD_HDR_ADR_ERR,
	EQDMA_MM_H2C0_RD_FLR_ERR,
	EQDMA_MM_H2C0_RD_DAT_POISON_ERR,
	EQDMA_MM_H2C0_RD_RQ_DIS_ERR,
	EQDMA_MM_H2C0_WR_DEC_ERR,
	EQDMA_MM_H2C0_WR_SLV_ERR,
	EQDMA_MM_H2C0_ERR_ALL,

	EQDMA_ERRS_ALL
};

struct eqdma_hw_err_info {
	enum eqdma_error_idx idx;
	const char *err_name;
	uint32_t mask_reg_addr;
	uint32_t stat_reg_addr;
	uint32_t leaf_err_mask;
	uint32_t global_err_mask;
	void (*eqdma_hw_err_process)(void *dev_hndl);
};

/* In QDMA_GLBL2_MISC_CAP(0x134) register,
 * Bits [23:20] gives QDMA IP version.
 * 0: QDMA3.1, 1: QDMA4.0, 2: QDMA5.0
 */
#define EQDMA_IP_VERSION_4                1
#define EQDMA_IP_VERSION_5                2

#define EQDMA_OFFSET_VF_VERSION           0x5014
#define EQDMA_OFFSET_VF_USER_BAR		  0x5018

#define EQDMA_OFFSET_MBOX_BASE_PF         0x22400
#define EQDMA_OFFSET_MBOX_BASE_VF         0x5000

#define EQDMA_COMPL_CTXT_BADDR_HIGH_H_MASK             GENMASK_ULL(63, 38)
#define EQDMA_COMPL_CTXT_BADDR_HIGH_L_MASK             GENMASK_ULL(37, 6)
#define EQDMA_COMPL_CTXT_BADDR_LOW_MASK                GENMASK_ULL(5, 2)

int eqdma_init_ctxt_memory(void *dev_hndl);

int eqdma_get_version(void *dev_hndl, uint8_t is_vf,
		struct qdma_hw_version_info *version_info);

int eqdma_get_ip_version(void *dev_hndl, uint8_t is_vf,
			uint32_t *ip_version);

int eqdma_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_sw_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int eqdma_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int eqdma_credit_ctx_conf(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid, struct qdma_descq_credit_ctxt *ctxt,
		enum qdma_hw_access_type access_type);

int eqdma_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_prefetch_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int eqdma_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int eqdma_fmap_conf(void *dev_hndl, uint16_t func_id,
			struct qdma_fmap_cfg *config,
			enum qdma_hw_access_type access_type);

int eqdma_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
			struct qdma_indirect_intr_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int eqdma_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

int eqdma_dump_intr_context(void *dev_hndl,
		struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, uint32_t buflen);

int eqdma_dump_queue_context(void *dev_hndl,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen);

uint32_t eqdma_reg_dump_buf_len(void);

int eqdma_context_buf_len(uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *buflen);

int eqdma_hw_error_process(void *dev_hndl);
const char *eqdma_hw_get_error_name(uint32_t err_idx);
int eqdma_hw_error_enable(void *dev_hndl, uint32_t err_idx);

int eqdma_read_dump_queue_context(void *dev_hndl,
		uint16_t func_id,
		uint16_t qid_hw,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		char *buf, uint32_t buflen);

int eqdma_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

int eqdma_get_user_bar(void *dev_hndl, uint8_t is_vf,
		uint16_t func_id, uint8_t *user_bar);

int eqdma_dump_config_reg_list(void *dev_hndl,
		uint32_t total_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen);

int eqdma_read_reg_list(void *dev_hndl, uint8_t is_vf,
		uint16_t reg_rd_group,
		uint16_t *total_regs,
		struct qdma_reg_data *reg_list);

int eqdma_set_default_global_csr(void *dev_hndl);

int eqdma_global_csr_conf(void *dev_hndl, uint8_t index, uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type);

int eqdma_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type);

int eqdma_mm_channel_conf(void *dev_hndl, uint8_t channel, uint8_t is_c2h,
				uint8_t enable);

int eqdma_dump_reg_info(void *dev_hndl, uint32_t reg_addr,
			uint32_t num_regs, char *buf, uint32_t buflen);

uint32_t eqdma_get_config_num_regs(void);

struct xreg_info *eqdma_get_config_regs(void);

#ifdef __cplusplus
}
#endif

#endif /* __EQDMA_SOFT_ACCESS_H_ */
