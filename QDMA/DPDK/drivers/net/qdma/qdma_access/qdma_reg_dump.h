/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef __QDMA_REG_DUMP_H__
#define __QDMA_REG_DUMP_H__

#include "qdma_platform_env.h"
#include "qdma_access.h"

#define DEBUGFS_DEV_INFO_SZ		(300)

#define DEBUGFS_INTR_CNTX_SZ	(600)
#define DBGFS_ERR_BUFLEN		(64)
#define DEBGFS_LINE_SZ			(81)
#define DEBGFS_GEN_NAME_SZ		(40)
#define REG_DUMP_SIZE_PER_LINE	(256)
#define CTXT_ENTRY_NAME_SZ        64

#define MAX_QDMA_CFG_REGS		(154)
#define MAX_QDMA_SW_CTX_ENTRIES		(26)
#define MAX_QDMA_HW_CTX_ENTRIES		(6)
#define MAX_QDMA_CREDIT_CTX_ENTRIES	(1)
#define MAX_QDMA_CMPT_CTX_ENTRIES	(23)
#define MAX_QDMA_PFTCH_CTX_ENTRIES	(8)

#define QDMA_MM_EN_SHIFT          0
#define QDMA_CMPT_EN_SHIFT        1
#define QDMA_ST_EN_SHIFT          2
#define QDMA_MAILBOX_EN_SHIFT     3

#define QDMA_MM_MODE              (1 << QDMA_MM_EN_SHIFT)
#define QDMA_COMPLETION_MODE      (1 << QDMA_CMPT_EN_SHIFT)
#define QDMA_ST_MODE              (1 << QDMA_ST_EN_SHIFT)
#define QDMA_MAILBOX      (1 << QDMA_MAILBOX_EN_SHIFT)


#define QDMA_MM_ST_MODE \
	(QDMA_MM_MODE | QDMA_COMPLETION_MODE | QDMA_ST_MODE)

#define GET_CAPABILITY_MASK(mm_en, st_en, mm_cmpt_en, mailbox_en)  \
	((mm_en << QDMA_MM_EN_SHIFT) | \
			((mm_cmpt_en | st_en) << QDMA_CMPT_EN_SHIFT) | \
			(st_en << QDMA_ST_EN_SHIFT) | \
			(mailbox_en << QDMA_MAILBOX_EN_SHIFT))

struct xreg_info {
	char name[32];
	uint32_t addr;
	uint32_t repeat;
	uint32_t step;
	uint8_t shift;
	uint8_t len;
	uint8_t mode;
};

struct qctx_entry {
	char name[CTXT_ENTRY_NAME_SZ];
	uint32_t value;
};

extern struct xreg_info qdma_config_regs[MAX_QDMA_CFG_REGS];
extern struct xreg_info qdma_cpm_config_regs[MAX_QDMA_CFG_REGS];
extern struct qctx_entry sw_ctxt_entries[MAX_QDMA_SW_CTX_ENTRIES];
extern struct qctx_entry hw_ctxt_entries[MAX_QDMA_HW_CTX_ENTRIES];
extern struct qctx_entry credit_ctxt_entries[MAX_QDMA_CREDIT_CTX_ENTRIES];
extern struct qctx_entry cmpt_ctxt_entries[MAX_QDMA_CMPT_CTX_ENTRIES];
extern struct qctx_entry c2h_pftch_ctxt_entries[MAX_QDMA_PFTCH_CTX_ENTRIES];

extern unsigned int qdma_reg_dump_buf_len(void);
extern unsigned int qdma_context_buf_len(
				char pfetch_valid, char cmpt_valid);
extern void qdma_acc_fill_sw_ctxt(struct qdma_descq_sw_ctxt *sw_ctxt);
extern void qdma_acc_fill_cmpt_ctxt(struct qdma_descq_cmpt_ctxt *cmpt_ctxt);
extern void qdma_acc_fill_hw_ctxt(struct qdma_descq_hw_ctxt *hw_ctxt);
extern void qdma_acc_fill_credit_ctxt(struct qdma_descq_credit_ctxt *cr_ctxt);
extern void qdma_acc_fill_pfetch_ctxt(struct qdma_descq_prefetch_ctxt
					*pfetch_ctxt);

#endif
