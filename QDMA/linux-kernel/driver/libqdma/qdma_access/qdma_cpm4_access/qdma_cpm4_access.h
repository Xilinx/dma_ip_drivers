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

#ifndef __QDMA_CPM4_ACCESS_H_
#define __QDMA_CPM4_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform.h"

/**
 * enum qdma_error_idx - qdma errors
 */
enum qdma_cpm4_error_idx {
	/* Descriptor errors */
	QDMA_CPM4_DSC_ERR_POISON,
	QDMA_CPM4_DSC_ERR_UR_CA,
	QDMA_CPM4_DSC_ERR_PARAM,
	QDMA_CPM4_DSC_ERR_ADDR,
	QDMA_CPM4_DSC_ERR_TAG,
	QDMA_CPM4_DSC_ERR_FLR,
	QDMA_CPM4_DSC_ERR_TIMEOUT,
	QDMA_CPM4_DSC_ERR_DAT_POISON,
	QDMA_CPM4_DSC_ERR_FLR_CANCEL,
	QDMA_CPM4_DSC_ERR_DMA,
	QDMA_CPM4_DSC_ERR_DSC,
	QDMA_CPM4_DSC_ERR_RQ_CANCEL,
	QDMA_CPM4_DSC_ERR_DBE,
	QDMA_CPM4_DSC_ERR_SBE,
	QDMA_CPM4_DSC_ERR_ALL,

	/* TRQ Errors */
	QDMA_CPM4_TRQ_ERR_UNMAPPED,
	QDMA_CPM4_TRQ_ERR_QID_RANGE,
	QDMA_CPM4_TRQ_ERR_VF_ACCESS_ERR,
	QDMA_CPM4_TRQ_ERR_TCP_TIMEOUT,
	QDMA_CPM4_TRQ_ERR_ALL,

	/* C2H Errors */
	QDMA_CPM4_ST_C2H_ERR_MTY_MISMATCH,
	QDMA_CPM4_ST_C2H_ERR_LEN_MISMATCH,
	QDMA_CPM4_ST_C2H_ERR_QID_MISMATCH,
	QDMA_CPM4_ST_C2H_ERR_DESC_RSP_ERR,
	QDMA_CPM4_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
	QDMA_CPM4_ST_C2H_ERR_MSI_INT_FAIL,
	QDMA_CPM4_ST_C2H_ERR_ERR_DESC_CNT,
	QDMA_CPM4_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
	QDMA_CPM4_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
	QDMA_CPM4_ST_C2H_ERR_WRB_INV_Q_ERR,
	QDMA_CPM4_ST_C2H_ERR_WRB_QFULL_ERR,
	QDMA_CPM4_ST_C2H_ERR_WRB_CIDX_ERR,
	QDMA_CPM4_ST_C2H_ERR_WRB_PRTY_ERR,
	QDMA_CPM4_ST_C2H_ERR_ALL,

	/* Fatal Errors */
	QDMA_CPM4_ST_FATAL_ERR_MTY_MISMATCH,
	QDMA_CPM4_ST_FATAL_ERR_LEN_MISMATCH,
	QDMA_CPM4_ST_FATAL_ERR_QID_MISMATCH,
	QDMA_CPM4_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_WRB_CTXT_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_INT_QID2VEC_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_WRB_COAL_DATA_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
	QDMA_CPM4_ST_FATAL_ERR_WPL_DATA_PAR_ERR,
	QDMA_CPM4_ST_FATAL_ERR_ALL,

	/* H2C Errors */
	QDMA_CPM4_ST_H2C_ERR_ZERO_LEN_DESC_ERR,
	QDMA_CPM4_ST_H2C_ERR_SDI_MRKR_REQ_MOP_ERR,
	QDMA_CPM4_ST_H2C_ERR_NO_DMA_DSC,
	QDMA_CPM4_ST_H2C_ERR_DBE,
	QDMA_CPM4_ST_H2C_ERR_SBE,
	QDMA_CPM4_ST_H2C_ERR_ALL,

	/* Single bit errors */
	QDMA_CPM4_SBE_ERR_MI_H2C0_DAT,
	QDMA_CPM4_SBE_ERR_MI_C2H0_DAT,
	QDMA_CPM4_SBE_ERR_H2C_RD_BRG_DAT,
	QDMA_CPM4_SBE_ERR_H2C_WR_BRG_DAT,
	QDMA_CPM4_SBE_ERR_C2H_RD_BRG_DAT,
	QDMA_CPM4_SBE_ERR_C2H_WR_BRG_DAT,
	QDMA_CPM4_SBE_ERR_FUNC_MAP,
	QDMA_CPM4_SBE_ERR_DSC_HW_CTXT,
	QDMA_CPM4_SBE_ERR_DSC_CRD_RCV,
	QDMA_CPM4_SBE_ERR_DSC_SW_CTXT,
	QDMA_CPM4_SBE_ERR_DSC_CPLI,
	QDMA_CPM4_SBE_ERR_DSC_CPLD,
	QDMA_CPM4_SBE_ERR_PASID_CTXT_RAM,
	QDMA_CPM4_SBE_ERR_TIMER_FIFO_RAM,
	QDMA_CPM4_SBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_CPM4_SBE_ERR_QID_FIFO_RAM,
	QDMA_CPM4_SBE_ERR_TUSER_FIFO_RAM,
	QDMA_CPM4_SBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_CPM4_SBE_ERR_INT_QID2VEC_RAM,
	QDMA_CPM4_SBE_ERR_INT_CTXT_RAM,
	QDMA_CPM4_SBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_CPM4_SBE_ERR_PFCH_CTXT_RAM,
	QDMA_CPM4_SBE_ERR_WRB_CTXT_RAM,
	QDMA_CPM4_SBE_ERR_PFCH_LL_RAM,
	QDMA_CPM4_SBE_ERR_ALL,

	/* Double bit Errors */
	QDMA_CPM4_DBE_ERR_MI_H2C0_DAT,
	QDMA_CPM4_DBE_ERR_MI_C2H0_DAT,
	QDMA_CPM4_DBE_ERR_H2C_RD_BRG_DAT,
	QDMA_CPM4_DBE_ERR_H2C_WR_BRG_DAT,
	QDMA_CPM4_DBE_ERR_C2H_RD_BRG_DAT,
	QDMA_CPM4_DBE_ERR_C2H_WR_BRG_DAT,
	QDMA_CPM4_DBE_ERR_FUNC_MAP,
	QDMA_CPM4_DBE_ERR_DSC_HW_CTXT,
	QDMA_CPM4_DBE_ERR_DSC_CRD_RCV,
	QDMA_CPM4_DBE_ERR_DSC_SW_CTXT,
	QDMA_CPM4_DBE_ERR_DSC_CPLI,
	QDMA_CPM4_DBE_ERR_DSC_CPLD,
	QDMA_CPM4_DBE_ERR_PASID_CTXT_RAM,
	QDMA_CPM4_DBE_ERR_TIMER_FIFO_RAM,
	QDMA_CPM4_DBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_CPM4_DBE_ERR_QID_FIFO_RAM,
	QDMA_CPM4_DBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_CPM4_DBE_ERR_INT_QID2VEC_RAM,
	QDMA_CPM4_DBE_ERR_INT_CTXT_RAM,
	QDMA_CPM4_DBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_CPM4_DBE_ERR_PFCH_CTXT_RAM,
	QDMA_CPM4_DBE_ERR_WRB_CTXT_RAM,
	QDMA_CPM4_DBE_ERR_PFCH_LL_RAM,
	QDMA_CPM4_DBE_ERR_ALL,

	QDMA_CPM4_ERRS_ALL
};

struct qdma_cpm4_hw_err_info {
	enum qdma_cpm4_error_idx idx;
	const char *err_name;
	uint32_t mask_reg_addr;
	uint32_t stat_reg_addr;
	uint32_t leaf_err_mask;
	uint32_t global_err_mask;
	void (*qdma_cpm4_hw_err_process)(void *dev_hndl);
};


int qdma_cpm4_init_ctxt_memory(void *dev_hndl);

#ifdef TANDEM_BOOT_SUPPORTED
int qdma_cpm4_init_st_ctxt(void *dev_hndl);
#endif

int qdma_cpm4_qid2vec_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			 struct qdma_qid2vec *ctxt,
			 enum qdma_hw_access_type access_type);

int qdma_cpm4_fmap_conf(void *dev_hndl, uint16_t func_id,
			struct qdma_fmap_cfg *config,
			enum qdma_hw_access_type access_type);

int qdma_cpm4_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_cpm4_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_cpm4_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_set_default_global_csr(void *dev_hndl);

int qdma_cpm4_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info);

int qdma_cpm4_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_q_cmpt_cidx_reg_info *reg_info);

int qdma_cpm4_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_intr_cidx_reg_info *reg_info);

int qdma_cmp_get_user_bar(void *dev_hndl, uint8_t is_vf,
		uint16_t func_id, uint8_t *user_bar);

int qdma_cpm4_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

uint32_t qdma_cpm4_reg_dump_buf_len(void);

int qdma_cpm4_context_buf_len(uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *req_buflen);

int qdma_cpm4_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

int qdma_cpm4_hw_error_process(void *dev_hndl);
const char *qdma_cpm4_hw_get_error_name(uint32_t err_idx);
int qdma_cpm4_hw_error_enable(void *dev_hndl, uint32_t err_idx);

int qdma_cpm4_dump_queue_context(void *dev_hndl,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen);

int qdma_cpm4_dump_intr_context(void *dev_hndl,
		struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, uint32_t buflen);

int qdma_cpm4_read_dump_queue_context(void *dev_hndl,
		uint16_t func_id,
		uint16_t qid_hw,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		char *buf, uint32_t buflen);

int qdma_cpm4_dump_config_reg_list(void *dev_hndl,
		uint32_t total_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen);

int qdma_cpm4_read_reg_list(void *dev_hndl, uint8_t is_vf,
		uint16_t reg_rd_slot,
		uint16_t *total_regs,
		struct qdma_reg_data *reg_list);

int qdma_cpm4_global_csr_conf(void *dev_hndl, uint8_t index,
				uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type);

int qdma_cpm4_mm_channel_conf(void *dev_hndl, uint8_t channel,
				uint8_t is_c2h,
				uint8_t enable);

int qdma_cpm4_dump_reg_info(void *dev_hndl, uint32_t reg_addr,
				uint32_t num_regs, char *buf, uint32_t buflen);

uint32_t qdma_cpm4_get_config_num_regs(void);

struct xreg_info *qdma_cpm4_get_config_regs(void);

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_CPM4_ACCESS_H_ */
