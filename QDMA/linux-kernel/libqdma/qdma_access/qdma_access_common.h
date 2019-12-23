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

#ifndef QDMA_ACCESS_COMMON_H_
#define QDMA_ACCESS_COMMON_H_


#include "qdma_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_INIT_CTXT_MEMORY			1

/* CSR Default values */
#define DEFAULT_MAX_DSC_FETCH               6
#define DEFAULT_WRB_INT                     QDMA_WRB_INTERVAL_128
#define DEFAULT_PFCH_STOP_THRESH            256
#define DEFAULT_PFCH_NUM_ENTRIES_PER_Q      8
#define DEFAULT_PFCH_MAX_Q_CNT              16
#define DEFAULT_C2H_INTR_TIMER_TICK         25
#define DEFAULT_CMPT_COAL_TIMER_CNT         5
#define DEFAULT_CMPT_COAL_TIMER_TICK        25
#define DEFAULT_CMPT_COAL_MAX_BUF_SZ        32
#define DEFAULT_H2C_THROT_DATA_THRESH       0x4000
#define DEFAULT_THROT_EN_DATA               1
#define DEFAULT_THROT_EN_REQ                0
#define DEFAULT_H2C_THROT_REQ_THRESH        0x60

#define QDMA_BAR_NUM                        6

void qdma_write_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, const uint32_t *values);

void qdma_read_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, uint32_t *values);

int dump_reg(char *buf, int buf_sz, unsigned int raddr,
		const char *rname, unsigned int rval);

int dump_context(char *buf, int buf_sz,
	char pfetch_valid, char cmpt_valid);

void qdma_memset(void *to, uint8_t val, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* QDMA_ACCESS_COMMON_H_ */
