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

#ifndef __QDMA_REG_DUMP_H__
#define __QDMA_REG_DUMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform_env.h"
#include "qdma_access_common.h"

#define DEBUGFS_DEV_INFO_SZ		(300)

#define QDMA_REG_NAME_LENGTH	64
#define DEBUGFS_INTR_CNTX_SZ	(2048 * 2)
#define DBGFS_ERR_BUFLEN		(64)
#define DEBGFS_LINE_SZ			(81)
#define DEBGFS_GEN_NAME_SZ		(40)
#define REG_DUMP_SIZE_PER_LINE	(256)

#define MAX_QDMA_CFG_REGS			(200)

#define QDMA_MM_EN_SHIFT          0
#define QDMA_CMPT_EN_SHIFT        1
#define QDMA_ST_EN_SHIFT          2
#define QDMA_MAILBOX_EN_SHIFT     3

#define QDMA_MM_MODE              (1 << QDMA_MM_EN_SHIFT)
#define QDMA_COMPLETION_MODE      (1 << QDMA_CMPT_EN_SHIFT)
#define QDMA_ST_MODE              (1 << QDMA_ST_EN_SHIFT)
#define QDMA_MAILBOX              (1 << QDMA_MAILBOX_EN_SHIFT)


#define QDMA_MM_ST_MODE \
	(QDMA_MM_MODE | QDMA_COMPLETION_MODE | QDMA_ST_MODE)

#define GET_CAPABILITY_MASK(mm_en, st_en, mm_cmpt_en, mailbox_en)  \
	((mm_en << QDMA_MM_EN_SHIFT) | \
			((mm_cmpt_en | st_en) << QDMA_CMPT_EN_SHIFT) | \
			(st_en << QDMA_ST_EN_SHIFT) | \
			(mailbox_en << QDMA_MAILBOX_EN_SHIFT))


struct regfield_info {
		const char *field_name;
		uint32_t field_mask;
};

struct xreg_info {
	const char *name;
	uint32_t addr;
	uint32_t repeat;
	uint32_t step;
	uint8_t shift;
	uint8_t len;
	uint8_t is_debug_reg;
	uint8_t mode;
	uint8_t read_type;
	uint8_t num_bitfields;
	struct regfield_info *bitfields;
};

#ifdef __cplusplus
}
#endif

#endif
