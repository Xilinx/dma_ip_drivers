/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef QDMA_CPM_ACCESS_H_
#define QDMA_CPM_ACCESS_H_

#include "qdma_access.h"

#ifdef __cplusplus
extern "C" {
#endif

int qdma_cpm_init_ctxt_memory(void *dev_hndl);

int qdma_cpm_qid2vec_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			 struct qdma_qid2vec *ctxt,
			 enum qdma_hw_access_type access_type);

int qdma_cpm_fmap_conf(void *dev_hndl, uint16_t func_id,
			struct qdma_fmap_cfg *config,
			enum qdma_hw_access_type access_type);

int qdma_cpm_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_cpm_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type);

int qdma_cpm_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type);

int qdma_cpm_set_default_global_csr(void *dev_hndl);

int qdma_cpm_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info);

int qdma_cpm_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_q_cmpt_cidx_reg_info *reg_info);

int qdma_cpm_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_intr_cidx_reg_info *reg_info);

int qdma_cmp_get_user_bar(void *dev_hndl, uint8_t is_vf, uint8_t *user_bar);

int qdma_cpm_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

int qdma_cpm_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

int qdma_cpm_dump_queue_context(void *dev_hndl, uint16_t hw_qid, uint8_t st,
	uint8_t c2h, char *buf, uint32_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* QDMA_CPM_ACCESS_H_ */
