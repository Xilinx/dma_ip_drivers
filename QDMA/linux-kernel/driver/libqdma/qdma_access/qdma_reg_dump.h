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
