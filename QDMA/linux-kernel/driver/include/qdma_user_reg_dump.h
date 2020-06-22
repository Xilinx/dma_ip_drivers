/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
 * All rights reserved.
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

#include "qdma_reg_dump.h"

#ifndef __QDMA_USER_REG_DUMP_H__
#define __QDMA_USER_REG_DUMP_H__



static struct xreg_info qdma_user_regs[] = {
	{"ST_C2H_QID", 0x0, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_PKTLEN", 0x4, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_CONTROL", 0x8, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	/*  ST_C2H_CONTROL:
	 *	[1] : start C2H
	 *	[2] : immediate data
	 *	[3] : every packet statrs with 00 instead of continuous data
	 *	      stream until # of packets is complete
	 *	[31]: gen_user_reset_n
	 */
	{"ST_H2C_CONTROL", 0xC, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	/*  ST_H2C_CONTROL:
	 *	[0] : clear match for H2C transfer
	 */
	{"ST_H2C_STATUS", 0x10, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_H2C_XFER_CNT", 0x14, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_PKT_CNT", 0x20, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_CMPT_DATA", 0x30, 8, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_CMPT_SIZE", 0x50, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_SCRATCH_REG", 0x60, 2, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_PKT_DROP", 0x88, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"ST_C2H_PKT_ACCEPT", 0x8C, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"DSC_BYPASS_LOOP", 0x90, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"USER_INTERRUPT", 0x94, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"USER_INTERRUPT_MASK", 0x98, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"USER_INTERRUPT_VEC", 0x9C, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"DMA_CONTROL", 0xA0, 0, 0, 0, 0, QDMA_MM_ST_MODE},
	{"VDM_MSG_READ", 0xA4, 0, 0, 0, 0, QDMA_MM_ST_MODE},

	{"", 0, 0, 0 }
};


#endif
