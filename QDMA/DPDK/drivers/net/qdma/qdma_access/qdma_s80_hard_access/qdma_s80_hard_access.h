/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QDMA_S80_HARD_ACCESS_H_
#define __QDMA_S80_HARD_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform.h"

/**
 * enum qdma_error_idx - qdma errors
 */
enum qdma_s80_hard_error_idx {
	/* Descriptor errors */
	QDMA_S80_HARD_DSC_ERR_POISON,
	QDMA_S80_HARD_DSC_ERR_UR_CA,
	QDMA_S80_HARD_DSC_ERR_PARAM,
	QDMA_S80_HARD_DSC_ERR_ADDR,
	QDMA_S80_HARD_DSC_ERR_TAG,
	QDMA_S80_HARD_DSC_ERR_FLR,
	QDMA_S80_HARD_DSC_ERR_TIMEOUT,
	QDMA_S80_HARD_DSC_ERR_DAT_POISON,
	QDMA_S80_HARD_DSC_ERR_FLR_CANCEL,
	QDMA_S80_HARD_DSC_ERR_DMA,
	QDMA_S80_HARD_DSC_ERR_DSC,
	QDMA_S80_HARD_DSC_ERR_RQ_CANCEL,
	QDMA_S80_HARD_DSC_ERR_DBE,
	QDMA_S80_HARD_DSC_ERR_SBE,
	QDMA_S80_HARD_DSC_ERR_ALL,

	/* TRQ Errors */
	QDMA_S80_HARD_TRQ_ERR_UNMAPPED,
	QDMA_S80_HARD_TRQ_ERR_QID_RANGE,
	QDMA_S80_HARD_TRQ_ERR_VF_ACCESS_ERR,
	QDMA_S80_HARD_TRQ_ERR_TCP_TIMEOUT,
	QDMA_S80_HARD_TRQ_ERR_ALL,

	/* C2H Errors */
	QDMA_S80_HARD_ST_C2H_ERR_MTY_MISMATCH,
	QDMA_S80_HARD_ST_C2H_ERR_LEN_MISMATCH,
	QDMA_S80_HARD_ST_C2H_ERR_QID_MISMATCH,
	QDMA_S80_HARD_ST_C2H_ERR_DESC_RSP_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_MSI_INT_FAIL,
	QDMA_S80_HARD_ST_C2H_ERR_ERR_DESC_CNT,
	QDMA_S80_HARD_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
	QDMA_S80_HARD_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
	QDMA_S80_HARD_ST_C2H_ERR_WRB_INV_Q_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_WRB_QFULL_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_WRB_CIDX_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_WRB_PRTY_ERR,
	QDMA_S80_HARD_ST_C2H_ERR_ALL,

	/* Fatal Errors */
	QDMA_S80_HARD_ST_FATAL_ERR_MTY_MISMATCH,
	QDMA_S80_HARD_ST_FATAL_ERR_LEN_MISMATCH,
	QDMA_S80_HARD_ST_FATAL_ERR_QID_MISMATCH,
	QDMA_S80_HARD_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_WRB_CTXT_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_INT_QID2VEC_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_WRB_COAL_DATA_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
	QDMA_S80_HARD_ST_FATAL_ERR_WPL_DATA_PAR_ERR,
	QDMA_S80_HARD_ST_FATAL_ERR_ALL,

	/* H2C Errors */
	QDMA_S80_HARD_ST_H2C_ERR_ZERO_LEN_DESC_ERR,
	QDMA_S80_HARD_ST_H2C_ERR_SDI_MRKR_REQ_MOP_ERR,
	QDMA_S80_HARD_ST_H2C_ERR_NO_DMA_DSC,
	QDMA_S80_HARD_ST_H2C_ERR_DBE,
	QDMA_S80_HARD_ST_H2C_ERR_SBE,
	QDMA_S80_HARD_ST_H2C_ERR_ALL,

	/* Single bit errors */
	QDMA_S80_HARD_SBE_ERR_MI_H2C0_DAT,
	QDMA_S80_HARD_SBE_ERR_MI_C2H0_DAT,
	QDMA_S80_HARD_SBE_ERR_H2C_RD_BRG_DAT,
	QDMA_S80_HARD_SBE_ERR_H2C_WR_BRG_DAT,
	QDMA_S80_HARD_SBE_ERR_C2H_RD_BRG_DAT,
	QDMA_S80_HARD_SBE_ERR_C2H_WR_BRG_DAT,
	QDMA_S80_HARD_SBE_ERR_FUNC_MAP,
	QDMA_S80_HARD_SBE_ERR_DSC_HW_CTXT,
	QDMA_S80_HARD_SBE_ERR_DSC_CRD_RCV,
	QDMA_S80_HARD_SBE_ERR_DSC_SW_CTXT,
	QDMA_S80_HARD_SBE_ERR_DSC_CPLI,
	QDMA_S80_HARD_SBE_ERR_DSC_CPLD,
	QDMA_S80_HARD_SBE_ERR_PASID_CTXT_RAM,
	QDMA_S80_HARD_SBE_ERR_TIMER_FIFO_RAM,
	QDMA_S80_HARD_SBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_S80_HARD_SBE_ERR_QID_FIFO_RAM,
	QDMA_S80_HARD_SBE_ERR_TUSER_FIFO_RAM,
	QDMA_S80_HARD_SBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_S80_HARD_SBE_ERR_INT_QID2VEC_RAM,
	QDMA_S80_HARD_SBE_ERR_INT_CTXT_RAM,
	QDMA_S80_HARD_SBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_S80_HARD_SBE_ERR_PFCH_CTXT_RAM,
	QDMA_S80_HARD_SBE_ERR_WRB_CTXT_RAM,
	QDMA_S80_HARD_SBE_ERR_PFCH_LL_RAM,
	QDMA_S80_HARD_SBE_ERR_ALL,

	/* Double bit Errors */
	QDMA_S80_HARD_DBE_ERR_MI_H2C0_DAT,
	QDMA_S80_HARD_DBE_ERR_MI_C2H0_DAT,
	QDMA_S80_HARD_DBE_ERR_H2C_RD_BRG_DAT,
	QDMA_S80_HARD_DBE_ERR_H2C_WR_BRG_DAT,
	QDMA_S80_HARD_DBE_ERR_C2H_RD_BRG_DAT,
	QDMA_S80_HARD_DBE_ERR_C2H_WR_BRG_DAT,
	QDMA_S80_HARD_DBE_ERR_FUNC_MAP,
	QDMA_S80_HARD_DBE_ERR_DSC_HW_CTXT,
	QDMA_S80_HARD_DBE_ERR_DSC_CRD_RCV,
	QDMA_S80_HARD_DBE_ERR_DSC_SW_CTXT,
	QDMA_S80_HARD_DBE_ERR_DSC_CPLI,
	QDMA_S80_HARD_DBE_ERR_DSC_CPLD,
	QDMA_S80_HARD_DBE_ERR_PASID_CTXT_RAM,
	QDMA_S80_HARD_DBE_ERR_TIMER_FIFO_RAM,
	QDMA_S80_HARD_DBE_ERR_PAYLOAD_FIFO_RAM,
	QDMA_S80_HARD_DBE_ERR_QID_FIFO_RAM,
	QDMA_S80_HARD_DBE_ERR_WRB_COAL_DATA_RAM,
	QDMA_S80_HARD_DBE_ERR_INT_QID2VEC_RAM,
	QDMA_S80_HARD_DBE_ERR_INT_CTXT_RAM,
	QDMA_S80_HARD_DBE_ERR_DESC_REQ_FIFO_RAM,
	QDMA_S80_HARD_DBE_ERR_PFCH_CTXT_RAM,
	QDMA_S80_HARD_DBE_ERR_WRB_CTXT_RAM,
	QDMA_S80_HARD_DBE_ERR_PFCH_LL_RAM,
	QDMA_S80_HARD_DBE_ERR_ALL,

	QDMA_S80_HARD_ERRS_ALL
};

struct qdma_s80_hard_hw_err_info {
	enum qdma_s80_hard_error_idx idx;
	const char *err_name;
	uint32_t mask_reg_addr;
	uint32_t stat_reg_addr;
	uint32_t leaf_err_mask;
	uint32_t global_err_mask;
	void (*qdma_s80_hard_hw_err_process)(void *dev_hndl);
};


int qdma_s80_hard_init_ctxt_memory(void *dev_hndl);

int qdma_s80_hard_qid2vec_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			 struct qdma_qid2vec *ctxt,
			 enum qdma_hw_access_type access_type);

int qdma_s80_hard_fmap_conf(void *dev_hndl, uint16_t func_id,
			struct qdma_fmap_cfg *config,
			enum qdma_hw_access_type access_type);

int qdma_s80_hard_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_s80_hard_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_s80_hard_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_set_default_global_csr(void *dev_hndl);

int qdma_s80_hard_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info);

int qdma_s80_hard_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_q_cmpt_cidx_reg_info *reg_info);

int qdma_s80_hard_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_intr_cidx_reg_info *reg_info);

int qdma_cmp_get_user_bar(void *dev_hndl, uint8_t is_vf,
		uint8_t func_id, uint8_t *user_bar);

int qdma_s80_hard_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

uint32_t qdma_s80_hard_reg_dump_buf_len(void);

int qdma_s80_hard_context_buf_len(uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *req_buflen);

int qdma_s80_hard_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

int qdma_s80_hard_hw_error_process(void *dev_hndl);
const char *qdma_s80_hard_hw_get_error_name(uint32_t err_idx);
int qdma_s80_hard_hw_error_enable(void *dev_hndl, uint32_t err_idx);

int qdma_s80_hard_dump_queue_context(void *dev_hndl,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen);

int qdma_s80_hard_dump_intr_context(void *dev_hndl,
		struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, uint32_t buflen);

int qdma_s80_hard_read_dump_queue_context(void *dev_hndl,
		uint16_t qid_hw,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		char *buf, uint32_t buflen);

int qdma_s80_hard_dump_config_reg_list(void *dev_hndl,
		uint32_t total_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen);

int qdma_s80_hard_read_reg_list(void *dev_hndl, uint8_t is_vf,
		uint16_t reg_rd_slot,
		uint16_t *total_regs,
		struct qdma_reg_data *reg_list);

int qdma_s80_hard_global_csr_conf(void *dev_hndl, uint8_t index,
				uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type);

int qdma_s80_hard_mm_channel_conf(void *dev_hndl, uint8_t channel,
				uint8_t is_c2h,
				uint8_t enable);

int qdma_s80_hard_dump_reg_info(void *dev_hndl, uint32_t reg_addr,
				uint32_t num_regs, char *buf, uint32_t buflen);

uint32_t qdma_s80_hard_get_config_num_regs(void);

struct xreg_info *qdma_s80_hard_get_config_regs(void);

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_S80_HARD_ACCESS_H_ */
