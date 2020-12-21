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

#ifndef __QDMA_SOFT_ACCESS_H_
#define __QDMA_SOFT_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA common library interface definitions
 *
 * Header file *qdma_access.h* defines data structures and function signatures
 * exported by QDMA common library.
 */

#include "qdma_platform.h"

/**
 * enum qdma_error_idx - qdma errors
 */
enum qdma_error_idx {
	/* Descriptor errors */
	QDMA_DSC_ERR_POISON,
	QDMA_DSC_ERR_UR_CA,
	QDMA_DSC_ERR_PARAM,
	QDMA_DSC_ERR_ADDR,
	QDMA_DSC_ERR_TAG,
	QDMA_DSC_ERR_FLR,
	QDMA_DSC_ERR_TIMEOUT,
	QDMA_DSC_ERR_DAT_POISON,
	QDMA_DSC_ERR_FLR_CANCEL,
	QDMA_DSC_ERR_DMA,
	QDMA_DSC_ERR_DSC,
	QDMA_DSC_ERR_RQ_CANCEL,
	QDMA_DSC_ERR_DBE,
	QDMA_DSC_ERR_SBE,
	QDMA_DSC_ERR_ALL,

	/* TRQ Errors */
	QDMA_TRQ_ERR_UNMAPPED,
	QDMA_TRQ_ERR_QID_RANGE,
	QDMA_TRQ_ERR_VF_ACCESS,
	QDMA_TRQ_ERR_TCP_TIMEOUT,
	QDMA_TRQ_ERR_ALL,

	/* C2H Errors */
	QDMA_ST_C2H_ERR_MTY_MISMATCH,
	QDMA_ST_C2H_ERR_LEN_MISMATCH,
	QDMA_ST_C2H_ERR_QID_MISMATCH,
	QDMA_ST_C2H_ERR_DESC_RSP_ERR,
	QDMA_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
	QDMA_ST_C2H_ERR_MSI_INT_FAIL,
	QDMA_ST_C2H_ERR_ERR_DESC_CNT,
	QDMA_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
	QDMA_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
	QDMA_ST_C2H_ERR_CMPT_INV_Q_ERR,
	QDMA_ST_C2H_ERR_CMPT_QFULL_ERR,
	QDMA_ST_C2H_ERR_CMPT_CIDX_ERR,
	QDMA_ST_C2H_ERR_CMPT_PRTY_ERR,
	QDMA_ST_C2H_ERR_ALL,

	/* Fatal Errors */
	QDMA_ST_FATAL_ERR_MTY_MISMATCH,
	QDMA_ST_FATAL_ERR_LEN_MISMATCH,
	QDMA_ST_FATAL_ERR_QID_MISMATCH,
	QDMA_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
	QDMA_ST_FATAL_ERR_CMPT_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
	QDMA_ST_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE,
	QDMA_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
	QDMA_ST_FATAL_ERR_WPL_DATA_PAR,
	QDMA_ST_FATAL_ERR_ALL,

	/* H2C Errors */
	QDMA_ST_H2C_ERR_ZERO_LEN_DESC,
	QDMA_ST_H2C_ERR_CSI_MOP,
	QDMA_ST_H2C_ERR_NO_DMA_DSC,
	QDMA_ST_H2C_ERR_SBE,
	QDMA_ST_H2C_ERR_DBE,
	QDMA_ST_H2C_ERR_ALL,

	/* Single bit errors */
	QDMA_SBE_ERR_MI_H2C0_DAT,
	QDMA_SBE_ERR_MI_C2H0_DAT,
	QDMA_SBE_ERR_H2C_RD_BRG_DAT,
	QDMA_SBE_ERR_H2C_WR_BRG_DAT,
	QDMA_SBE_ERR_C2H_RD_BRG_DAT,
	QDMA_SBE_ERR_C2H_WR_BRG_DAT,
	QDMA_SBE_ERR_FUNC_MAP,
	QDMA_SBE_ERR_DSC_HW_CTXT,
	QDMA_SBE_ERR_DSC_CRD_RCV,
	QDMA_SBE_ERR_DSC_SW_CTXT,
	QDMA_SBE_ERR_DSC_CPLI,
	QDMA_SBE_ERR_DSC_CPLD,
	QDMA_SBE_ERR_PASID_CTXT_RAM,
	QDMA_SBE_ERR_TIMER_FIFO_RAM,
	QDMA_SBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_SBE_ERR_QID_FIFO_RAM,
	QDMA_SBE_ERR_TUSER_FIFO_RAM,
	QDMA_SBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_SBE_ERR_INT_QID2VEC_RAM,
	QDMA_SBE_ERR_INT_CTXT_RAM,
	QDMA_SBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_SBE_ERR_PFCH_CTXT_RAM,
	QDMA_SBE_ERR_WRB_CTXT_RAM,
	QDMA_SBE_ERR_PFCH_LL_RAM,
	QDMA_SBE_ERR_H2C_PEND_FIFO,
	QDMA_SBE_ERR_ALL,

	/* Double bit Errors */
	QDMA_DBE_ERR_MI_H2C0_DAT,
	QDMA_DBE_ERR_MI_C2H0_DAT,
	QDMA_DBE_ERR_H2C_RD_BRG_DAT,
	QDMA_DBE_ERR_H2C_WR_BRG_DAT,
	QDMA_DBE_ERR_C2H_RD_BRG_DAT,
	QDMA_DBE_ERR_C2H_WR_BRG_DAT,
	QDMA_DBE_ERR_FUNC_MAP,
	QDMA_DBE_ERR_DSC_HW_CTXT,
	QDMA_DBE_ERR_DSC_CRD_RCV,
	QDMA_DBE_ERR_DSC_SW_CTXT,
	QDMA_DBE_ERR_DSC_CPLI,
	QDMA_DBE_ERR_DSC_CPLD,
	QDMA_DBE_ERR_PASID_CTXT_RAM,
	QDMA_DBE_ERR_TIMER_FIFO_RAM,
	QDMA_DBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_DBE_ERR_QID_FIFO_RAM,
	QDMA_DBE_ERR_TUSER_FIFO_RAM,
	QDMA_DBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_DBE_ERR_INT_QID2VEC_RAM,
	QDMA_DBE_ERR_INT_CTXT_RAM,
	QDMA_DBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_DBE_ERR_PFCH_CTXT_RAM,
	QDMA_DBE_ERR_WRB_CTXT_RAM,
	QDMA_DBE_ERR_PFCH_LL_RAM,
	QDMA_DBE_ERR_H2C_PEND_FIFO,
	QDMA_DBE_ERR_ALL,

	QDMA_ERRS_ALL
};

struct qdma_hw_err_info {
	enum qdma_error_idx idx;
	const char *err_name;
	uint32_t mask_reg_addr;
	uint32_t stat_reg_addr;
	uint32_t leaf_err_mask;
	uint32_t global_err_mask;
	void (*qdma_hw_err_process)(void *dev_hndl);
};


int qdma_set_default_global_csr(void *dev_hndl);

int qdma_get_version(void *dev_hndl, uint8_t is_vf,
		struct qdma_hw_version_info *version_info);

int qdma_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_fmap_conf(void *dev_hndl, uint16_t func_id,
				struct qdma_fmap_cfg *config,
				enum qdma_hw_access_type access_type);

int qdma_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info);

int qdma_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_q_cmpt_cidx_reg_info *reg_info);

int qdma_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_intr_cidx_reg_info *reg_info);

int qdma_init_ctxt_memory(void *dev_hndl);

int qdma_legacy_intr_conf(void *dev_hndl, enum status_type enable);

int qdma_clear_pend_legacy_intr(void *dev_hndl);

int qdma_is_legacy_intr_pend(void *dev_hndl);

int qdma_dump_intr_context(void *dev_hndl,
		struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, uint32_t buflen);

uint32_t qdma_soft_reg_dump_buf_len(void);

uint32_t qdma_get_config_num_regs(void);

struct xreg_info *qdma_get_config_regs(void);

int qdma_soft_context_buf_len(uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *buflen);

int qdma_soft_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

int qdma_soft_dump_queue_context(void *dev_hndl,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen);

int qdma_soft_read_dump_queue_context(void *dev_hndl,
				uint16_t qid_hw,
				uint8_t st,
				enum qdma_dev_q_type q_type,
				char *buf, uint32_t buflen);

int qdma_hw_error_process(void *dev_hndl);

const char *qdma_hw_get_error_name(uint32_t err_idx);

int qdma_hw_error_enable(void *dev_hndl, uint32_t err_idx);

int qdma_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

int qdma_get_user_bar(void *dev_hndl, uint8_t is_vf,
		uint8_t func_id, uint8_t *user_bar);

int qdma_soft_dump_config_reg_list(void *dev_hndl,
		uint32_t total_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen);

int qdma_read_reg_list(void *dev_hndl, uint8_t is_vf,
		uint16_t reg_rd_group,
		uint16_t *total_regs,
		struct qdma_reg_data *reg_list);

int qdma_global_csr_conf(void *dev_hndl, uint8_t index, uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type);

int qdma_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type);

int qdma_mm_channel_conf(void *dev_hndl, uint8_t channel, uint8_t is_c2h,
				uint8_t enable);

int qdma_dump_reg_info(void *dev_hndl, uint32_t reg_addr,
			uint32_t num_regs, char *buf, uint32_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_SOFT_ACCESS_H_ */
