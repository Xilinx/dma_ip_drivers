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

#include "qdma_access.h"
#include "qdma_reg.h"
#include "qdma_reg_dump.h"
#include "qdma_platform.h"
#include "qdma_cpm_access.h"
#include "qdma_access_common.h"

/** QDMA Context array size */
#define QDMA_FMAP_NUM_WORDS				2
#define QDMA_SW_CONTEXT_NUM_WORDS			5
#define QDMA_PFETCH_CONTEXT_NUM_WORDS			2
#define QDMA_CMPT_CONTEXT_NUM_WORDS			5
#define QDMA_HW_CONTEXT_NUM_WORDS			2
#define QDMA_CR_CONTEXT_NUM_WORDS			1
#define QDMA_IND_INTR_CONTEXT_NUM_WORDS			3
#define QDMA_REG_IND_CTXT_REG_COUNT			8

/* qdma version info */
#define RTL_BASE_VERSION                        2
#define RTL_PATCH_VERSION                       3

union qdma_ind_ctxt_cmd {
	uint32_t word;
	struct {
		uint32_t busy:1;
		uint32_t sel:4;
		uint32_t op:2;
		uint32_t qid:11;
		uint32_t rsvd:14;
	} bits;
};

struct qdma_indirect_ctxt_regs {
	uint32_t qdma_ind_ctxt_data[QDMA_IND_CTXT_DATA_NUM_REGS];
	uint32_t qdma_ind_ctxt_mask[QDMA_IND_CTXT_DATA_NUM_REGS];
	union qdma_ind_ctxt_cmd cmd;
};

struct qdma_hw_err_info {
	enum qdma_error_idx idx;
	const char *err_name;
	uint32_t mask_reg_addr;
	uint32_t stat_reg_addr;
	uint32_t leaf_err_mask;
	uint32_t global_err_mask;
};

static struct qdma_hw_err_info err_info[QDMA_ERRS_ALL] = {
	/* Descriptor errors */
	{
		QDMA_DSC_ERR_POISON,
		"Poison error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_POISON_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_UR_CA,
		"Unsupported request or completer aborted error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_UR_CA_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_PARAM,
		"Parameter mismatch error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_PARAM_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_ADDR,
		"Address mismatch error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_ADDR_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_TAG,
		"Unexpected tag error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_TAG_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_FLR,
		"FLR error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_FLR_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_TIMEOUT,
		"Timed out error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_TIMEOUT_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_DAT_POISON,
		"Poison data error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_DAT_POISON_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_FLR_CANCEL,
		"Descriptor fetch cancelled due to FLR error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_FLR_CANCEL_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_DMA,
		"DMA engine error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_DMA_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_DSC,
		"Invalid PIDX update error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_DSC_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_RQ_CANCEL,
		"Descriptor fetch cancelled due to disable register status error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_RQ_CANCEL_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_DBE,
		"UNC_ERR_RAM_DBE error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_DBE_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_SBE,
		"UNC_ERR_RAM_SBE error",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_SBE_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},
	{
		QDMA_DSC_ERR_ALL,
		"All Descriptor errors",
		QDMA_OFFSET_GLBL_DSC_ERR_MASK,
		QDMA_OFFSET_GLBL_DSC_ERR_STAT,
		QDMA_GLBL_DSC_ERR_ALL_MASK,
		QDMA_GLBL_ERR_DSC_MASK
	},

	/* TRQ errors */
	{
		QDMA_TRQ_ERR_UNMAPPED,
		"Access targeted unmapped register space error",
		QDMA_OFFSET_GLBL_TRQ_ERR_MASK,
		QDMA_OFFSET_GLBL_TRQ_ERR_STAT,
		QDMA_GLBL_TRQ_ERR_UNMAPPED_MASK,
		QDMA_GLBL_ERR_TRQ_MASK
	},
	{
		QDMA_TRQ_ERR_QID_RANGE,
		"Qid range error",
		QDMA_OFFSET_GLBL_TRQ_ERR_MASK,
		QDMA_OFFSET_GLBL_TRQ_ERR_STAT,
		QDMA_GLBL_TRQ_ERR_QID_RANGE_MASK,
		QDMA_GLBL_ERR_TRQ_MASK
	},
	{
		QDMA_TRQ_ERR_VF_ACCESS,
		"Invalid VF access error",
		QDMA_OFFSET_GLBL_TRQ_ERR_MASK,
		QDMA_OFFSET_GLBL_TRQ_ERR_STAT,
		QDMA_GLBL_TRQ_ERR_VF_ACCESS_MASK,
		QDMA_GLBL_ERR_TRQ_MASK
	},
	{
		QDMA_TRQ_ERR_TCP_TIMEOUT,
		"Timeout on request error",
		QDMA_OFFSET_GLBL_TRQ_ERR_MASK,
		QDMA_OFFSET_GLBL_TRQ_ERR_STAT,
		QDMA_GLBL_TRQ_ERR_TCP_TIMEOUT_MASK,
		QDMA_GLBL_ERR_TRQ_MASK
	},
	{
		QDMA_TRQ_ERR_ALL,
		"All TRQ errors",
		QDMA_OFFSET_GLBL_TRQ_ERR_MASK,
		QDMA_OFFSET_GLBL_TRQ_ERR_STAT,
		QDMA_GLBL_TRQ_ERR_ALL_MASK,
		QDMA_GLBL_ERR_TRQ_MASK
	},

	/* C2H Errors*/
	{
		QDMA_ST_C2H_ERR_MTY_MISMATCH,
		"MTY mismatch error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_MTY_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_LEN_MISMATCH,
		"Packet length mismatch error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_LEN_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_QID_MISMATCH,
		"Qid mismatch error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_QID_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_DESC_RSP_ERR,
		"Descriptor error bit set",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_DESC_RSP_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
		"Data parity error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_ENG_WPL_DATA_PAR_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_MSI_INT_FAIL,
		"MSI got a fail response error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_MSI_INT_FAIL_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_ERR_DESC_CNT,
		"Descriptor count error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_ERR_DESC_CNT_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
		"Port id in packet and pfetch ctxt mismatch error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_PORTID_CTXT_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
		"Port id in packet and bypass interface mismatch error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_PORTID_BYP_IN_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_CMPT_INV_Q_ERR,
		"Writeback on invalid queue error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_CMPT_INV_Q_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_CMPT_QFULL_ERR,
		"Completion queue gets full error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_CMPT_QFULL_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_CMPT_CIDX_ERR,
		"Bad CIDX update by the software error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_CMPT_CIDX_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_CMPT_PRTY_ERR,
		"C2H completion Parity error",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_CMPT_PRTY_ERR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_C2H_ERR_ALL,
		"All C2h errors",
		QDMA_OFFSET_C2H_ERR_MASK,
		QDMA_OFFSET_C2H_ERR_STAT,
		QDMA_C2H_ERR_ALL_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},

	/* C2H fatal errors */
	{
		QDMA_ST_FATAL_ERR_MTY_MISMATCH,
		"Fatal MTY mismatch error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_MTY_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_LEN_MISMATCH,
		"Fatal Len mismatch error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_LEN_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_QID_MISMATCH,
		"Fatal Qid mismatch error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_QID_MISMATCH_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_TIMER_FIFO_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_PFCH_II_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_CMPT_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_CMPT_CTXT_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_PFCH_CTXT_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_INT_CTXT_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_TUSER_FIFO_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_QID_FIFO_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_WPL_DATA_PAR,
		"RAM double bit fatal error",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_WPL_DATA_PAR_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},
	{
		QDMA_ST_FATAL_ERR_ALL,
		"All fatal errors",
		QDMA_OFFSET_C2H_FATAL_ERR_MASK,
		QDMA_OFFSET_C2H_FATAL_ERR_STAT,
		QDMA_C2H_FATAL_ERR_ALL_MASK,
		QDMA_GLBL_ERR_ST_C2H_MASK
	},

	/* H2C St errors */
	{
		QDMA_ST_H2C_ERR_ZERO_LEN_DESC,
		"Zero length descriptor error",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_ZERO_LEN_DESC_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},
	{
		QDMA_ST_H2C_ERR_CSI_MOP,
		"Non EOP descriptor received error",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_CSI_MOP_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},
	{
		QDMA_ST_H2C_ERR_NO_DMA_DSC,
		"No DMA descriptor received error",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_NO_DMA_DSC_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},
	{
		QDMA_ST_H2C_ERR_SBE,
		"Single bit error detected on H2C-ST data error",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_SBE_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},
	{
		QDMA_ST_H2C_ERR_DBE,
		"Double bit error detected on H2C-ST data error",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_DBE_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},
	{
		QDMA_ST_H2C_ERR_ALL,
		"All H2C errors",
		QDMA_OFFSET_H2C_ERR_MASK,
		QDMA_OFFSET_H2C_ERR_STAT,
		QDMA_H2C_ERR_ALL_MASK,
		QDMA_GLBL_ERR_ST_H2C_MASK
	},

	/* SBE errors */
	{
		QDMA_SBE_ERR_MI_H2C0_DAT,
		"H2C MM data buffer single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_MI_H2C0_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_MI_C2H0_DAT,
		"C2H MM data buffer single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_MI_C2H0_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_H2C_RD_BRG_DAT,
		"Bridge master read single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_H2C_RD_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_H2C_WR_BRG_DAT,
		"Bridge master write single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_H2C_WR_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_C2H_RD_BRG_DAT,
		"Bridge slave read data buffer single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_C2H_RD_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_C2H_WR_BRG_DAT,
		"Bridge slave write data buffer single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_C2H_WR_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_FUNC_MAP,
		"Function map RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_FUNC_MAP_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DSC_HW_CTXT,
		"Descriptor engine hardware context RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DSC_HW_CTXT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DSC_CRD_RCV,
		"Descriptor engine receive credit context RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DSC_CRD_RCV_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DSC_SW_CTXT,
		"Descriptor engine software context RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DSC_SW_CTXT_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DSC_CPLI,
		"Descriptor engine fetch completion information RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DSC_CPLI_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DSC_CPLD,
		"Descriptor engine fetch completion data RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DSC_CPLD_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_PASID_CTXT_RAM,
		"PASID configuration RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_PASID_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_TIMER_FIFO_RAM,
		"Timer fifo RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_TIMER_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_PAYLOAD_FIFO_RAM,
		"C2H ST payload RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_PAYLOAD_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_QID_FIFO_RAM,
		"C2H ST QID FIFO RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_QID_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_TUSER_FIFO_RAM,
		"C2H ST TUSER RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_TUSER_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_WRB_COAL_DATA_RAM,
		"Completion Coalescing RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_WRB_COAL_DATA_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_INT_QID2VEC_RAM,
		"Interrupt QID2VEC RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_INT_QID2VEC_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_INT_CTXT_RAM,
		"Interrupt context RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_INT_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_DESC_REQ_FIFO_RAM,
		"C2H ST descriptor request RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_DESC_REQ_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_PFCH_CTXT_RAM,
		"C2H ST prefetch RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_PFCH_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_WRB_CTXT_RAM,
		"C2H ST completion context RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_WRB_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_PFCH_LL_RAM,
		"C2H ST prefetch list RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_PFCH_LL_RAM_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_H2C_PEND_FIFO,
		"H2C ST pending fifo RAM single bit ECC error",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_H2C_PEND_FIFO_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},
	{
		QDMA_SBE_ERR_ALL,
		"All SBE errors",
		QDMA_OFFSET_RAM_SBE_MASK,
		QDMA_OFFSET_RAM_SBE_STAT,
		QDMA_SBE_ERR_ALL_MASK,
		QDMA_GLBL_ERR_RAM_SBE_MASK
	},


	/* DBE Errors */
	{
		QDMA_DBE_ERR_MI_H2C0_DAT,
		"H2C MM data buffer double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_MI_H2C0_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_MI_C2H0_DAT,
		"C2H MM data buffer double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_MI_C2H0_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_H2C_RD_BRG_DAT,
		"Bridge master read double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_H2C_RD_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_H2C_WR_BRG_DAT,
		"Bridge master write double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_H2C_WR_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_C2H_RD_BRG_DAT,
		"Bridge slave read data buffer double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_C2H_RD_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_C2H_WR_BRG_DAT,
		"Bridge slave write data buffer double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_C2H_WR_BRG_DAT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_FUNC_MAP,
		"Function map RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_FUNC_MAP_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DSC_HW_CTXT,
		"Descriptor engine hardware context RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DSC_HW_CTXT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DSC_CRD_RCV,
		"Descriptor engine receive credit context RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DSC_CRD_RCV_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DSC_SW_CTXT,
		"Descriptor engine software context RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DSC_SW_CTXT_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DSC_CPLI,
		"Descriptor engine fetch completion information RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DSC_CPLI_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DSC_CPLD,
		"Descriptor engine fetch completion data RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DSC_CPLD_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_PASID_CTXT_RAM,
		"PASID configuration RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_PASID_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_TIMER_FIFO_RAM,
		"Timer fifo RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_TIMER_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_PAYLOAD_FIFO_RAM,
		"C2H ST payload RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_PAYLOAD_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_QID_FIFO_RAM,
		"C2H ST QID FIFO RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_QID_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_TUSER_FIFO_RAM,
		"C2H ST TUSER RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_TUSER_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_WRB_COAL_DATA_RAM,
		"Completion Coalescing RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_WRB_COAL_DATA_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_INT_QID2VEC_RAM,
		"Interrupt QID2VEC RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_INT_QID2VEC_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_INT_CTXT_RAM,
		"Interrupt context RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_INT_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_DESC_REQ_FIFO_RAM,
		"C2H ST descriptor request RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_DESC_REQ_FIFO_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_PFCH_CTXT_RAM,
		"C2H ST prefetch RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_PFCH_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_WRB_CTXT_RAM,
		"C2H ST completion context RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_WRB_CTXT_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_PFCH_LL_RAM,
		"C2H ST prefetch list RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_PFCH_LL_RAM_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_H2C_PEND_FIFO,
		"H2C pending fifo RAM double bit ECC error",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_H2C_PEND_FIFO_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	},
	{
		QDMA_DBE_ERR_ALL,
		"All DBE errors",
		QDMA_OFFSET_RAM_DBE_MASK,
		QDMA_OFFSET_RAM_DBE_STAT,
		QDMA_DBE_ERR_ALL_MASK,
		QDMA_GLBL_ERR_RAM_DBE_MASK
	}
};

static int32_t all_hw_errs[TOTAL_LEAF_ERROR_AGGREGATORS] = {

	QDMA_DSC_ERR_ALL,
	QDMA_TRQ_ERR_ALL,
	QDMA_ST_C2H_ERR_ALL,
	QDMA_ST_FATAL_ERR_ALL,
	QDMA_ST_H2C_ERR_ALL,
	QDMA_SBE_ERR_ALL,
	QDMA_DBE_ERR_ALL
};

static int hw_monitor_reg(void *dev_hndl, unsigned int reg, uint32_t mask,
		uint32_t val, unsigned int interval_us,
		unsigned int timeout_us);
static int qdma_indirect_reg_invalidate(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid);
static int qdma_indirect_reg_clear(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid);
static int qdma_indirect_reg_read(void *dev_hndl, enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t cnt, uint32_t *data);
static int qdma_indirect_reg_write(void *dev_hndl, enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t *data, uint16_t cnt);

/*
 * hw_monitor_reg() - polling a register repeatly until
 *	(the register value & mask) == val or time is up
 *
 * return -QDMA_BUSY_IIMEOUT_ERR if register value didn't match, 0 other wise
 */
static int hw_monitor_reg(void *dev_hndl, unsigned int reg, uint32_t mask,
		uint32_t val, unsigned int interval_us, unsigned int timeout_us)
{
	int count;
	uint32_t v;

	if (!interval_us)
		interval_us = QDMA_REG_POLL_DFLT_INTERVAL_US;
	if (!timeout_us)
		timeout_us = QDMA_REG_POLL_DFLT_TIMEOUT_US;

	count = timeout_us / interval_us;

	do {
		v = qdma_reg_read(dev_hndl, reg);
		if ((v & mask) == val)
			return QDMA_SUCCESS;
		qdma_udelay(interval_us);
	} while (--count);

	v = qdma_reg_read(dev_hndl, reg);
	if ((v & mask) == val)
		return QDMA_SUCCESS;

	qdma_log_error("%s: Reg read=%u Expected=%u, err:%d\n",
				   __func__, v, val,
				   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
	return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
}

/*
 * qdma_indirect_reg_invalidate() - helper function to invalidate indirect
 *					context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_indirect_reg_invalidate(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid)
{
	union qdma_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_INV;
	cmd.bits.sel = sel;
	qdma_reg_write(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD,
			QDMA_IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed with err:%d\n",
						__func__,
					   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*
 * qdma_indirect_reg_clear() - helper function to clear indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_indirect_reg_clear(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid)
{
	union qdma_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_CLR;
	cmd.bits.sel = sel;
	qdma_reg_write(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD,
			QDMA_IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed with err:%d\n",
						__func__,
					   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*
 * qdma_indirect_reg_read() - helper function to read indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_indirect_reg_read(void *dev_hndl, enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t cnt, uint32_t *data)
{
	uint32_t index = 0, reg_addr = QDMA_OFFSET_IND_CTXT_DATA;
	union qdma_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_RD;
	cmd.bits.sel = sel;
	qdma_reg_write(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD,
			QDMA_IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed with err:%d\n",
						__func__,
					   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	for (index = 0; index < cnt; index++, reg_addr += sizeof(uint32_t))
		data[index] = qdma_reg_read(dev_hndl, reg_addr);

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*
 * qdma_indirect_reg_write() - helper function to write indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_indirect_reg_write(void *dev_hndl, enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t *data, uint16_t cnt)
{
	uint32_t index, reg_addr;
	struct qdma_indirect_ctxt_regs regs;
	uint32_t *wr_data = (uint32_t *)&regs;

	qdma_reg_access_lock(dev_hndl);

	/* write the context data */
	for (index = 0; index < QDMA_IND_CTXT_DATA_NUM_REGS; index++) {
		if (index < cnt)
			regs.qdma_ind_ctxt_data[index] = data[index];
		else
			regs.qdma_ind_ctxt_data[index] = 0;
		regs.qdma_ind_ctxt_mask[index] = 0xFFFFFFFF;
	}

	regs.cmd.word = 0;
	regs.cmd.bits.qid = hw_qid;
	regs.cmd.bits.op = QDMA_CTXT_CMD_WR;
	regs.cmd.bits.sel = sel;
	reg_addr = QDMA_OFFSET_IND_CTXT_DATA;

	for (index = 0; index < ((2 * QDMA_IND_CTXT_DATA_NUM_REGS) + 1);
			index++, reg_addr += sizeof(uint32_t))
		qdma_reg_write(dev_hndl, reg_addr, wr_data[index]);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD,
			QDMA_IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed with err:%d\n",
						__func__,
					   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_fmap_write() - create fmap context and program it
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 * @config:	pointer to the fmap data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_fmap_write(void *dev_hndl, uint16_t func_id,
		   const struct qdma_fmap_cfg *config)
{
	uint32_t fmap[QDMA_FMAP_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl || !config) {
		qdma_log_error("%s: dev_handle=%p fmap=%p NULL, err:%d\n",
				__func__, dev_hndl, config,
				-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_log_debug("%s: func_id=%hu, qbase=%hu, qmax=%hu\n", __func__,
				   func_id, config->qbase, config->qmax);
	fmap[num_words_count++] =
		FIELD_SET(QDMA_FMAP_CTXT_W0_QID_MASK, config->qbase);
	fmap[num_words_count++] =
		FIELD_SET(QDMA_FMAP_CTXT_W1_QID_MAX_MASK, config->qmax);

	return qdma_indirect_reg_write(dev_hndl, sel, func_id,
			fmap, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_fmap_read() - read fmap context
 *
 * @dev_hndl:   device handle
 * @func_id:    function id of the device
 * @config:	pointer to the output fmap data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_fmap_read(void *dev_hndl, uint16_t func_id,
			 struct qdma_fmap_cfg *config)
{
	int rv = QDMA_SUCCESS;
	uint32_t fmap[QDMA_FMAP_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl || !config) {
		qdma_log_error("%s: dev_handle=%p fmap=%p NULL, err:%d\n",
						__func__, dev_hndl, config,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, func_id,
			QDMA_FMAP_NUM_WORDS, fmap);
	if (rv < 0)
		return rv;

	config->qbase = FIELD_GET(QDMA_FMAP_CTXT_W0_QID_MASK, fmap[0]);
	config->qmax = FIELD_GET(QDMA_FMAP_CTXT_W1_QID_MAX_MASK, fmap[1]);

	qdma_log_debug("%s: func_id=%hu, qbase=%hu, qmax=%hu\n", __func__,
				   func_id, config->qbase, config->qmax);
	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_fmap_clear() - clear fmap context
 *
 * @dev_hndl:   device handle
 * @func_id:    function id of the device
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_fmap_clear(void *dev_hndl, uint16_t func_id)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_log_debug("%s: func_id=%hu\n", __func__, func_id);
	return qdma_indirect_reg_clear(dev_hndl, sel, func_id);
}

/*****************************************************************************/
/**
 * qdma_fmap_conf() - configure fmap context
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 * @config:	pointer to the fmap data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_INVALIDATE Not supported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_fmap_conf(void *dev_hndl, uint16_t func_id,
				struct qdma_fmap_cfg *config,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_fmap_read(dev_hndl, func_id, config);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_fmap_write(dev_hndl, func_id, config);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_fmap_clear(dev_hndl, func_id);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
	default:
		qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
						__func__,
						access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_sw_context_write() - create sw context and program it
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the SW context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_sw_context_write(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 const struct qdma_descq_sw_ctxt *ctxt)
{
	uint32_t sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	/* Input args check */
	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle=%p sw_ctxt=%p NULL, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	sw_ctxt[num_words_count++] =
		FIELD_SET(QDMA_SW_CTXT_W0_PIDX, ctxt->pidx) |
		FIELD_SET(QDMA_SW_CTXT_W0_IRQ_ARM_MASK, ctxt->irq_arm) |
		FIELD_SET(QDMA_SW_CTXT_W0_FUNC_ID_MASK, ctxt->fnc_id);

	qdma_log_debug("%s: pidx=%x, irq_arm=%x, fnc_id=%x\n",
			 __func__, ctxt->pidx, ctxt->irq_arm, ctxt->fnc_id);

	sw_ctxt[num_words_count++] =
		FIELD_SET(QDMA_SW_CTXT_W1_QEN_MASK, ctxt->qen) |
		FIELD_SET(QDMA_SW_CTXT_W1_FCRD_EN_MASK, ctxt->frcd_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_WBI_CHK_MASK, ctxt->wbi_chk) |
		FIELD_SET(QDMA_SW_CTXT_W1_WB_INT_EN_MASK, ctxt->wbi_intvl_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_AT_MASK, ctxt->at) |
		FIELD_SET(QDMA_SW_CTXT_W1_FETCH_MAX_MASK, ctxt->fetch_max) |
		FIELD_SET(QDMA_SW_CTXT_W1_RNG_SZ_MASK, ctxt->rngsz_idx) |
		FIELD_SET(QDMA_SW_CTXT_W1_DSC_SZ_MASK, ctxt->desc_sz) |
		FIELD_SET(QDMA_SW_CTXT_W1_BYP_MASK, ctxt->bypass) |
		FIELD_SET(QDMA_SW_CTXT_W1_MM_CHN_MASK, ctxt->mm_chn) |
		FIELD_SET(QDMA_SW_CTXT_W1_WBK_EN_MASK, ctxt->wbk_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_EN_MASK, ctxt->irq_en) |
		FIELD_SET(QDMA_SW_CTXT_W1_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK, ctxt->irq_no_last) |
		FIELD_SET(QDMA_SW_CTXT_W1_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK, ctxt->err_wb_sent) |
		FIELD_SET(QDMA_SW_CTXT_W1_IRQ_REQ_MASK, ctxt->irq_req) |
		FIELD_SET(QDMA_SW_CTXT_W1_MRKR_DIS_MASK, ctxt->mrkr_dis) |
		FIELD_SET(QDMA_SW_CTXT_W1_IS_MM_MASK, ctxt->is_mm);

	qdma_log_debug("%s: qen=%x, frcd_en=%x, wbi_chk=%x, wbi_intvl_en=%x\n",
			 __func__, ctxt->qen, ctxt->frcd_en, ctxt->wbi_chk,
			ctxt->wbi_intvl_en);

	qdma_log_debug("%s: at=%x, fetch_max=%x, rngsz_idx=%x, desc_sz=%x\n",
			__func__, ctxt->at, ctxt->fetch_max, ctxt->rngsz_idx,
			ctxt->desc_sz);

	qdma_log_debug("%s: bypass=%x, mm_chn=%x, wbk_en=%x, irq_en=%x\n",
			__func__, ctxt->bypass, ctxt->mm_chn, ctxt->wbk_en,
			ctxt->irq_en);

	qdma_log_debug("%s: port_id=%x, irq_no_last=%x,err=%x",
			__func__, ctxt->port_id, ctxt->irq_no_last, ctxt->err);
	qdma_log_debug(", err_wb_sent=%x\n", ctxt->err_wb_sent);

	qdma_log_debug("%s: irq_req=%x, mrkr_dis=%x, is_mm=%x\n",
			__func__, ctxt->irq_req, ctxt->mrkr_dis, ctxt->is_mm);

	sw_ctxt[num_words_count++] = ctxt->ring_bs_addr & 0xffffffff;
	sw_ctxt[num_words_count++] = (ctxt->ring_bs_addr >> 32) & 0xffffffff;

	sw_ctxt[num_words_count++] =
		FIELD_SET(QDMA_SW_CTXT_W4_VEC_MASK, ctxt->vec) |
		FIELD_SET(QDMA_SW_CTXT_W4_INTR_AGGR_MASK, ctxt->intr_aggr);

	qdma_log_debug("%s: vec=%x, intr_aggr=%x\n",
			__func__, ctxt->vec, ctxt->intr_aggr);

	return qdma_indirect_reg_write(dev_hndl, sel, hw_qid,
			sw_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_sw_context_read() - read sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_sw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_sw_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle=%p sw_ctxt=%p NULL, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_SW_CONTEXT_NUM_WORDS, sw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->pidx = FIELD_GET(QDMA_SW_CTXT_W0_PIDX, sw_ctxt[0]);
	ctxt->irq_arm =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W0_IRQ_ARM_MASK, sw_ctxt[0]));
	ctxt->fnc_id =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W0_FUNC_ID_MASK, sw_ctxt[0]));

	qdma_log_debug("%s: pidx=%x, irq_arm=%x, fnc_id=%x",
			 __func__, ctxt->pidx, ctxt->irq_arm, ctxt->fnc_id);

	ctxt->qen = FIELD_GET(QDMA_SW_CTXT_W1_QEN_MASK, sw_ctxt[1]);
	ctxt->frcd_en = FIELD_GET(QDMA_SW_CTXT_W1_FCRD_EN_MASK, sw_ctxt[1]);
	ctxt->wbi_chk = FIELD_GET(QDMA_SW_CTXT_W1_WBI_CHK_MASK, sw_ctxt[1]);
	ctxt->wbi_intvl_en =
		FIELD_GET(QDMA_SW_CTXT_W1_WB_INT_EN_MASK, sw_ctxt[1]);
	ctxt->at = FIELD_GET(QDMA_SW_CTXT_W1_AT_MASK, sw_ctxt[1]);
	ctxt->fetch_max =
		FIELD_GET(QDMA_SW_CTXT_W1_FETCH_MAX_MASK, sw_ctxt[1]);
	ctxt->rngsz_idx =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_RNG_SZ_MASK, sw_ctxt[1]));
	ctxt->desc_sz =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_DSC_SZ_MASK, sw_ctxt[1]));
	ctxt->bypass =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_BYP_MASK, sw_ctxt[1]));
	ctxt->mm_chn =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_MM_CHN_MASK, sw_ctxt[1]));
	ctxt->wbk_en =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_WBK_EN_MASK, sw_ctxt[1]));
	ctxt->irq_en =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_IRQ_EN_MASK, sw_ctxt[1]));
	ctxt->port_id =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_PORT_ID_MASK, sw_ctxt[1]));
	ctxt->irq_no_last =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK,
			sw_ctxt[1]));
	ctxt->err =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_ERR_MASK, sw_ctxt[1]));
	ctxt->err_wb_sent =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK,
			sw_ctxt[1]));
	ctxt->irq_req =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_IRQ_REQ_MASK, sw_ctxt[1]));
	ctxt->mrkr_dis =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_MRKR_DIS_MASK, sw_ctxt[1]));
	ctxt->is_mm =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W1_IS_MM_MASK, sw_ctxt[1]));

	qdma_log_debug("%s: qen=%x, frcd_en=%x, wbi_chk=%x, wbi_intvl_en=%x\n",
			 __func__, ctxt->qen, ctxt->frcd_en, ctxt->wbi_chk,
			ctxt->wbi_intvl_en);
	qdma_log_debug("%s: at=%x, fetch_max=%x, rngsz_idx=%x, desc_sz=%x\n",
			__func__, ctxt->at, ctxt->fetch_max, ctxt->rngsz_idx,
			ctxt->desc_sz);
	qdma_log_debug("%s: bypass=%x, mm_chn=%x, wbk_en=%x, irq_en=%x\n",
			__func__, ctxt->bypass, ctxt->mm_chn, ctxt->wbk_en,
			ctxt->irq_en);
	qdma_log_debug("%s: port_id=%x, irq_no_last=%x,",
			__func__, ctxt->port_id, ctxt->irq_no_last);
	qdma_log_debug(" err=%x, err_wb_sent=%x\n",
			ctxt->err, ctxt->err_wb_sent);
	qdma_log_debug("%s: irq_req=%x, mrkr_dis=%x, is_mm=%x\n",
			__func__, ctxt->irq_req, ctxt->mrkr_dis, ctxt->is_mm);

	ctxt->ring_bs_addr = ((uint64_t)sw_ctxt[3] << 32) | (sw_ctxt[2]);

	ctxt->vec = FIELD_GET(QDMA_SW_CTXT_W4_VEC_MASK, sw_ctxt[4]);
	ctxt->intr_aggr =
		(uint8_t)(FIELD_GET(QDMA_SW_CTXT_W4_INTR_AGGR_MASK,
			sw_ctxt[4]));

	qdma_log_debug("%s: vec=%x, intr_aggr=%x\n",
			__func__, ctxt->vec, ctxt->intr_aggr);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_sw_context_clear() - clear sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_sw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_sw_context_invalidate() - invalidate sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_sw_context_invalidate(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_sw_ctx_conf() - configure SW context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_sw_context_read(dev_hndl, c2h, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_sw_context_write(dev_hndl, c2h, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_sw_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_sw_context_invalidate(dev_hndl, c2h, hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
						__func__,
						access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_pfetch_context_write() - create prefetch context and program it
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the prefetch context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_pfetch_context_write(void *dev_hndl, uint16_t hw_qid,
		const struct qdma_descq_prefetch_ctxt *ctxt)
{
	uint32_t pfetch_ctxt[QDMA_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;
	uint16_t num_words_count = 0;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle or pfetch ctxt NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	sw_crdt_l =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK, ctxt->sw_crdt);
	sw_crdt_h =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, ctxt->sw_crdt);

	qdma_log_debug("%s: sw_crdt_l=%hu, sw_crdt_h=%hu, hw_qid=%hu\n",
			 __func__, sw_crdt_l, sw_crdt_h, hw_qid);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(QDMA_PFTCH_CTXT_W0_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_BUF_SIZE_IDX_MASK,
				ctxt->bufsz_idx) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_PFETCH_EN_MASK, ctxt->pfch_en) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_Q_IN_PFETCH_MASK, ctxt->pfch) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_SW_CRDT_L_MASK, sw_crdt_l);

	qdma_log_debug("%s: bypass=%x, bufsz_idx=%x, port_id=%x\n",
			__func__, ctxt->bypass, ctxt->bufsz_idx, ctxt->port_id);
	qdma_log_debug("%s: err=%x, pfch_en=%x, pfch=%x, ctxt->valid=%x\n",
			__func__, ctxt->err, ctxt->pfch_en, ctxt->pfch,
			ctxt->valid);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(QDMA_PFTCH_CTXT_W1_SW_CRDT_H_MASK, sw_crdt_h) |
		FIELD_SET(QDMA_PFTCH_CTXT_W1_VALID_MASK, ctxt->valid);

	return qdma_indirect_reg_write(dev_hndl, sel, hw_qid,
			pfetch_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_pfetch_context_read() - read prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_pfetch_context_read(void *dev_hndl, uint16_t hw_qid,
		struct qdma_descq_prefetch_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t pfetch_ctxt[QDMA_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle or pfetch ctxt NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_PFETCH_CONTEXT_NUM_WORDS, pfetch_ctxt);
	if (rv < 0)
		return rv;

	ctxt->bypass =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_BYPASS_MASK, pfetch_ctxt[0]);
	ctxt->bufsz_idx =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_BUF_SIZE_IDX_MASK, pfetch_ctxt[0]);
	ctxt->port_id =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_PORT_ID_MASK, pfetch_ctxt[0]);
	ctxt->err =
		(uint8_t)(FIELD_GET(QDMA_PFTCH_CTXT_W0_ERR_MASK,
			pfetch_ctxt[0]));
	ctxt->pfch_en =
		(uint8_t)(FIELD_GET(QDMA_PFTCH_CTXT_W0_PFETCH_EN_MASK,
			pfetch_ctxt[0]));
	ctxt->pfch =
		(uint8_t)(FIELD_GET(QDMA_PFTCH_CTXT_W0_Q_IN_PFETCH_MASK,
				pfetch_ctxt[0]));
	sw_crdt_l =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_SW_CRDT_L_MASK, pfetch_ctxt[0]);

	sw_crdt_h =
		FIELD_GET(QDMA_PFTCH_CTXT_W1_SW_CRDT_H_MASK, pfetch_ctxt[1]);
	ctxt->valid =
		(uint8_t)(FIELD_GET(QDMA_PFTCH_CTXT_W1_VALID_MASK,
			pfetch_ctxt[1]));

	ctxt->sw_crdt =
		FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK, sw_crdt_l) |
		FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, sw_crdt_h);

	qdma_log_debug("%s: sw_crdt_l=%hu, sw_crdt_h=%hu, hw_qid=%hu\n",
			 __func__, sw_crdt_l, sw_crdt_h, hw_qid);
	qdma_log_debug("%s: bypass=%x, bufsz_idx=%x, port_id=%x\n",
			__func__, ctxt->bypass, ctxt->bufsz_idx, ctxt->port_id);
	qdma_log_debug("%s: err=%x, pfch_en=%x, pfch=%x, ctxt->valid=%x\n",
			__func__, ctxt->err, ctxt->pfch_en, ctxt->pfch,
			ctxt->valid);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_pfetch_context_clear() - clear prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_pfetch_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_pfetch_context_invalidate() - invalidate prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_pfetch_context_invalidate(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_pfetch_ctx_conf() - configure prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_pfetch_context_read(dev_hndl, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_pfetch_context_write(dev_hndl, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_pfetch_context_clear(dev_hndl, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_pfetch_context_invalidate(dev_hndl, hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
						__func__,
						access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_cmpt_context_write() - create completion context and program it
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the cmpt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_cmpt_context_write(void *dev_hndl, uint16_t hw_qid,
			   const struct qdma_descq_cmpt_ctxt *ctxt)
{
	uint32_t cmpt_ctxt[QDMA_CMPT_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	uint32_t baddr_l, baddr_h, pidx_l, pidx_h;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	/* Input args check */
	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle or cmpt ctxt NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (ctxt->trig_mode > QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR) {
		qdma_log_error("%s: trig_mode(%d) > (%d) is invalid, err:%d\n",
					__func__,
					ctxt->trig_mode,
					QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR,
					-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	baddr_l = (uint32_t)FIELD_GET(QDMA_COMPL_CTXT_BADDR_GET_L_MASK,
			ctxt->bs_addr);
	baddr_h = (uint32_t)FIELD_GET(QDMA_COMPL_CTXT_BADDR_GET_H_MASK,
			ctxt->bs_addr);
	pidx_l = FIELD_GET(QDMA_COMPL_CTXT_PIDX_GET_L_MASK, ctxt->pidx);
	pidx_h = FIELD_GET(QDMA_COMPL_CTXT_PIDX_GET_H_MASK, ctxt->pidx);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(QDMA_COMPL_CTXT_W0_EN_STAT_DESC_MASK,
				ctxt->en_stat_desc) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_EN_INT_MASK, ctxt->en_int) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_TRIG_MODE_MASK, ctxt->trig_mode) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_FNC_ID_MASK, ctxt->fnc_id) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_COUNTER_IDX_MASK,
				ctxt->counter_idx) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_TIMER_IDX_MASK, ctxt->timer_idx) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_INT_ST_MASK, ctxt->in_st) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_COLOR_MASK, ctxt->color) |
		FIELD_SET(QDMA_COMPL_CTXT_W0_RING_SZ_MASK, ctxt->ringsz_idx);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(QDMA_COMPL_CTXT_W1_BADDR_64_L_MASK, baddr_l);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(QDMA_COMPL_CTXT_W2_BADDR_64_H_MASK, baddr_h) |
		FIELD_SET(QDMA_COMPL_CTXT_W2_DESC_SIZE_MASK, ctxt->desc_sz) |
		FIELD_SET(QDMA_COMPL_CTXT_W2_PIDX_L_MASK, pidx_l);


	cmpt_ctxt[num_words_count++] =
		FIELD_SET(QDMA_COMPL_CTXT_W3_PIDX_H_MASK, pidx_h) |
		FIELD_SET(QDMA_COMPL_CTXT_W3_CIDX_MASK, ctxt->cidx) |
		FIELD_SET(QDMA_COMPL_CTXT_W3_VALID_MASK, ctxt->valid) |
		FIELD_SET(QDMA_COMPL_CTXT_W3_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_COMPL_CTXT_W3_USR_TRG_PND_MASK,
				ctxt->user_trig_pend);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(QDMA_COMPL_CTXT_W4_TMR_RUN_MASK,
				ctxt->timer_running) |
		FIELD_SET(QDMA_COMPL_CTXT_W4_FULL_UPDT_MASK, ctxt->full_upd) |
		FIELD_SET(QDMA_COMPL_CTXT_W4_OVF_CHK_DIS_MASK,
				ctxt->ovf_chk_dis) |
		FIELD_SET(QDMA_COMPL_CTXT_W4_AT_MASK, ctxt->at) |
		FIELD_SET(QDMA_COMPL_CTXT_W4_INTR_VEC_MASK, ctxt->vec) |
		FIELD_SET(QDMA_COMPL_CTXT_W4_INTR_AGGR_MASK, ctxt->int_aggr);

	return qdma_indirect_reg_write(dev_hndl, sel, hw_qid,
			cmpt_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_cmpt_context_read() - read completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_cmpt_context_read(void *dev_hndl, uint16_t hw_qid,
			   struct qdma_descq_cmpt_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t cmpt_ctxt[QDMA_CMPT_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;
	uint32_t baddr_l, baddr_h, pidx_l, pidx_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle or cmpt ctxt NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_CMPT_CONTEXT_NUM_WORDS, cmpt_ctxt);
	if (rv < 0)
		return rv;

	ctxt->en_stat_desc =
		FIELD_GET(QDMA_COMPL_CTXT_W0_EN_STAT_DESC_MASK, cmpt_ctxt[0]);
	ctxt->en_int = FIELD_GET(QDMA_COMPL_CTXT_W0_EN_INT_MASK, cmpt_ctxt[0]);
	ctxt->trig_mode =
		FIELD_GET(QDMA_COMPL_CTXT_W0_TRIG_MODE_MASK, cmpt_ctxt[0]);
	ctxt->fnc_id =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_FNC_ID_MASK,
			cmpt_ctxt[0]));
	ctxt->counter_idx =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_COUNTER_IDX_MASK,
			cmpt_ctxt[0]));
	ctxt->timer_idx =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_TIMER_IDX_MASK,
			cmpt_ctxt[0]));
	ctxt->in_st =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_INT_ST_MASK,
			cmpt_ctxt[0]));
	ctxt->color =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_COLOR_MASK,
			cmpt_ctxt[0]));
	ctxt->ringsz_idx =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W0_RING_SZ_MASK,
			cmpt_ctxt[0]));

	baddr_l = FIELD_GET(QDMA_COMPL_CTXT_W1_BADDR_64_L_MASK, cmpt_ctxt[1]);

	baddr_h = FIELD_GET(QDMA_COMPL_CTXT_W2_BADDR_64_H_MASK, cmpt_ctxt[2]);
	ctxt->desc_sz =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W2_DESC_SIZE_MASK,
			cmpt_ctxt[2]));
	pidx_l = FIELD_GET(QDMA_COMPL_CTXT_W2_PIDX_L_MASK, cmpt_ctxt[2]);

	pidx_h = FIELD_GET(QDMA_COMPL_CTXT_W3_PIDX_H_MASK, cmpt_ctxt[3]);
	ctxt->cidx =
		(uint16_t)(FIELD_GET(QDMA_COMPL_CTXT_W3_CIDX_MASK,
			cmpt_ctxt[3]));
	ctxt->valid =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W3_VALID_MASK,
			cmpt_ctxt[3]));
	ctxt->err =
		(uint8_t)(FIELD_GET(QDMA_COMPL_CTXT_W3_ERR_MASK, cmpt_ctxt[3]));
	ctxt->user_trig_pend = (uint8_t)
		(FIELD_GET(QDMA_COMPL_CTXT_W3_USR_TRG_PND_MASK, cmpt_ctxt[3]));

	ctxt->timer_running =
		FIELD_GET(QDMA_COMPL_CTXT_W4_TMR_RUN_MASK, cmpt_ctxt[4]);
	ctxt->full_upd =
		FIELD_GET(QDMA_COMPL_CTXT_W4_FULL_UPDT_MASK, cmpt_ctxt[4]);
	ctxt->ovf_chk_dis =
		FIELD_GET(QDMA_COMPL_CTXT_W4_OVF_CHK_DIS_MASK, cmpt_ctxt[4]);
	ctxt->at = FIELD_GET(QDMA_COMPL_CTXT_W4_AT_MASK, cmpt_ctxt[4]);
	ctxt->vec = FIELD_GET(QDMA_COMPL_CTXT_W4_INTR_VEC_MASK, cmpt_ctxt[4]);
	ctxt->int_aggr = (uint8_t)
		(FIELD_GET(QDMA_COMPL_CTXT_W4_INTR_AGGR_MASK, cmpt_ctxt[4]));

	ctxt->bs_addr =
		FIELD_SET(QDMA_COMPL_CTXT_BADDR_GET_L_MASK, baddr_l) |
		FIELD_SET(QDMA_COMPL_CTXT_BADDR_GET_H_MASK, (uint64_t)baddr_h);

	ctxt->pidx =
		FIELD_SET(QDMA_COMPL_CTXT_PIDX_GET_L_MASK, pidx_l) |
		FIELD_SET(QDMA_COMPL_CTXT_PIDX_GET_H_MASK, pidx_h);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_cmpt_context_clear() - clear completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_cmpt_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_cmpt_context_invalidate() - invalidate completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_cmpt_context_invalidate(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_cmpt_ctx_conf() - configure completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_cmpt_context_read(dev_hndl, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_cmpt_context_write(dev_hndl, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_cmpt_context_clear(dev_hndl, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_cmpt_context_invalidate(dev_hndl, hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
						__func__,
						access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_hw_context_read() - read hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid, struct qdma_descq_hw_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t hw_ctxt[QDMA_HW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_handle or hw_ctxt NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_HW_CONTEXT_NUM_WORDS, hw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->cidx = FIELD_GET(QDMA_HW_CTXT_W0_CIDX_MASK, hw_ctxt[0]);
	ctxt->crd_use =
		(uint16_t)(FIELD_GET(QDMA_HW_CTXT_W0_CRD_USE_MASK, hw_ctxt[0]));

	ctxt->dsc_pend =
		(uint8_t)(FIELD_GET(QDMA_HW_CTXT_W1_DSC_PND_MASK, hw_ctxt[1]));
	ctxt->idl_stp_b =
		(uint8_t)(FIELD_GET(QDMA_HW_CTXT_W1_IDL_STP_B_MASK,
			hw_ctxt[1]));
	ctxt->evt_pnd =
		(uint8_t)(FIELD_GET(QDMA_HW_CTXT_W1_EVENT_PEND_MASK,
			hw_ctxt[1]));
	ctxt->fetch_pnd = (uint8_t)
		(FIELD_GET(QDMA_HW_CTXT_W1_FETCH_PEND_MASK, hw_ctxt[1]));

	qdma_log_debug("%s: cidx=%hu, crd_use=%hu, dsc_pend=%x\n",
			__func__, ctxt->cidx, ctxt->crd_use, ctxt->dsc_pend);
	qdma_log_debug("%s: idl_stp_b=%x, evt_pnd=%x, fetch_pnd=%x\n",
			__func__, ctxt->idl_stp_b, ctxt->evt_pnd,
			ctxt->fetch_pnd);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_context_clear() - clear hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_hw_context_invalidate() - invalidate hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_hw_ctx_conf() - configure HW context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_WRITE Not supported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_hw_context_read(dev_hndl, c2h, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_hw_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_hw_context_invalidate(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_WRITE:
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_credit_context_read() - read credit context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_credit_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_credit_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t cr_ctxt[QDMA_CR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p credit_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_CR_CONTEXT_NUM_WORDS, cr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->credit = FIELD_GET(QDMA_CR_CTXT_W0_CREDT_MASK, cr_ctxt[0]);

	qdma_log_debug("%s: credit=%u\n", __func__, ctxt->credit);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_credit_context_clear() - clear credit context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_credit_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_credit_context_invalidate() - invalidate credit context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_credit_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_credit_ctx_conf() - configure credit context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_WRITE Not supported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_credit_context_read(dev_hndl, c2h, hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_credit_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_credit_context_invalidate(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_WRITE:
	default:
		qdma_log_error("%s: Invalid access type=%d, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_write() - create indirect interrupt context
 *					and program it
 *
 * @dev_hndl:   device handle
 * @ring_index: indirect interrupt ring index
 * @ctxt:	pointer to the interrupt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_indirect_intr_context_write(void *dev_hndl, uint16_t ring_index,
		const struct qdma_indirect_intr_ctxt *ctxt)
{
	uint32_t intr_ctxt[QDMA_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint32_t baddr_l, baddr_m, baddr_h;
	uint16_t num_words_count = 0;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p intr_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	baddr_l = (uint32_t)FIELD_GET(QDMA_INTR_CTXT_BADDR_GET_L_MASK,
			ctxt->baddr_4k);
	baddr_m = (uint32_t)FIELD_GET(QDMA_INTR_CTXT_BADDR_GET_M_MASK,
			ctxt->baddr_4k);
	baddr_h = (uint32_t)FIELD_GET(QDMA_INTR_CTXT_BADDR_GET_H_MASK,
			ctxt->baddr_4k);

	intr_ctxt[num_words_count++] =
		FIELD_SET(QDMA_INTR_CTXT_W0_VALID_MASK, ctxt->valid) |
		FIELD_SET(QDMA_INTR_CTXT_W0_VEC_ID_MASK, ctxt->vec) |
		FIELD_SET(QDMA_INTR_CTXT_W0_INT_ST_MASK, ctxt->int_st) |
		FIELD_SET(QDMA_INTR_CTXT_W0_COLOR_MASK, ctxt->color) |
		FIELD_SET(QDMA_INTR_CTXT_W0_BADDR_64_MASK, baddr_l);

	intr_ctxt[num_words_count++] =
		FIELD_SET(QDMA_INTR_CTXT_W1_BADDR_64_MASK, baddr_m);

	intr_ctxt[num_words_count++] =
		FIELD_SET(QDMA_INTR_CTXT_W2_BADDR_64_MASK, baddr_h) |
		FIELD_SET(QDMA_INTR_CTXT_W2_PAGE_SIZE_MASK, ctxt->page_size) |
		FIELD_SET(QDMA_INTR_CTXT_W2_PIDX_MASK, ctxt->pidx) |
		FIELD_SET(QDMA_INTR_CTXT_W2_AT_MASK, ctxt->at);

	return qdma_indirect_reg_write(dev_hndl, sel, ring_index,
			intr_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_read() - read indirect interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_indirect_intr_context_read(void *dev_hndl, uint16_t ring_index,
				   struct qdma_indirect_intr_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t intr_ctxt[QDMA_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint64_t baddr_l, baddr_m, baddr_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p intr_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_indirect_reg_read(dev_hndl, sel, ring_index,
			QDMA_IND_INTR_CONTEXT_NUM_WORDS, intr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->valid = FIELD_GET(QDMA_INTR_CTXT_W0_VALID_MASK, intr_ctxt[0]);
	ctxt->vec = FIELD_GET(QDMA_INTR_CTXT_W0_VEC_ID_MASK, intr_ctxt[0]);
	ctxt->int_st =
		(uint8_t)(FIELD_GET(QDMA_INTR_CTXT_W0_INT_ST_MASK,
			intr_ctxt[0]));
	ctxt->color =
		(uint8_t)(FIELD_GET(QDMA_INTR_CTXT_W0_COLOR_MASK,
			intr_ctxt[0]));
	baddr_l = FIELD_GET(QDMA_INTR_CTXT_W0_BADDR_64_MASK, intr_ctxt[0]);

	baddr_m = FIELD_GET(QDMA_INTR_CTXT_W1_BADDR_64_MASK, intr_ctxt[1]);

	baddr_h = FIELD_GET(QDMA_INTR_CTXT_W2_BADDR_64_MASK, intr_ctxt[2]);
	ctxt->page_size =
		FIELD_GET(QDMA_INTR_CTXT_W2_PAGE_SIZE_MASK, intr_ctxt[2]);
	ctxt->pidx =
		(uint16_t)(FIELD_GET(QDMA_INTR_CTXT_W2_PIDX_MASK,
			intr_ctxt[2]));
	ctxt->at =
		(uint8_t)(FIELD_GET(QDMA_INTR_CTXT_W2_AT_MASK, intr_ctxt[2]));

	ctxt->baddr_4k =
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_L_MASK, baddr_l) |
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_M_MASK, baddr_m) |
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_H_MASK, baddr_h);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_clear() - clear indirect interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_indirect_intr_context_clear(void *dev_hndl, uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_clear(dev_hndl, sel, ring_index);
}

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_invalidate() - invalidate indirect interrupt
 * context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_indirect_intr_context_invalidate(void *dev_hndl,
					  uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_indirect_reg_invalidate(dev_hndl, sel, ring_index);
}

/*****************************************************************************/
/**
 * qdma_indirect_intr_ctx_conf() - configure indirect interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_indirect_intr_context_read(dev_hndl, ring_index,
							ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_indirect_intr_context_write(dev_hndl, ring_index,
							ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_indirect_intr_context_clear(dev_hndl,
							ring_index);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_indirect_intr_context_invalidate(dev_hndl,
								ring_index);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_set_default_global_csr() - function to set the global CSR register to
 * default values. The value can be modified later by using the set/get csr
 * functions
 *
 * @dev_hndl:	device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_set_default_global_csr(void *dev_hndl)
{
	/* Default values */
	uint32_t cfg_val = 0, reg_val = 0;
	uint32_t rng_sz[QDMA_NUM_RING_SIZES] = {2049, 65, 129, 193, 257, 385,
		513, 769, 1025, 1537, 3073, 4097, 6145, 8193, 12289, 16385};
	uint32_t tmr_cnt[QDMA_NUM_C2H_TIMERS] = {1, 2, 4, 5, 8, 10, 15, 20, 25,
		30, 50, 75, 100, 125, 150, 200};
	uint32_t cnt_th[QDMA_NUM_C2H_COUNTERS] = {64, 2, 4, 8, 16, 24, 32, 48,
		80, 96, 112, 128, 144, 160, 176, 192};
	uint32_t buf_sz[QDMA_NUM_C2H_BUFFER_SIZES] = {4096, 256, 512, 1024,
		2048, 3968, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 8192,
		9018, 16384};
	struct qdma_dev_attributes *dev_cap = NULL;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	/* Configuring CSR registers */
	/* Global ring sizes */
	qdma_write_csr_values(dev_hndl, QDMA_OFFSET_GLBL_RNG_SZ, 0,
			QDMA_NUM_RING_SIZES, rng_sz);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en) {
		/* Counter thresholds */
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_CNT_TH, 0,
				QDMA_NUM_C2H_COUNTERS, cnt_th);

		/* Timer Counters */
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_TIMER_CNT, 0,
				QDMA_NUM_C2H_TIMERS, tmr_cnt);


		/* Writeback Interval */
		reg_val =
			FIELD_SET(QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK,
					DEFAULT_MAX_DSC_FETCH) |
			FIELD_SET(QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK,
					DEFAULT_WRB_INT);
		qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG, reg_val);
	}

	if (dev_cap->st_en) {
		/* Buffer Sizes */
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_BUF_SZ, 0,
				QDMA_NUM_C2H_BUFFER_SIZES, buf_sz);

		/* Prefetch Configuration */
		cfg_val = qdma_reg_read(dev_hndl,
				QDMA_OFFSET_C2H_PFETCH_CACHE_DEPTH);
		reg_val =
			FIELD_SET(QDMA_C2H_PFCH_FL_TH_MASK,
					DEFAULT_PFCH_STOP_THRESH) |
			FIELD_SET(QDMA_C2H_NUM_PFCH_MASK,
					DEFAULT_PFCH_NUM_ENTRIES_PER_Q) |
			FIELD_SET(QDMA_C2H_PFCH_QCNT_MASK, (cfg_val >> 1)) |
			FIELD_SET(QDMA_C2H_EVT_QCNT_TH_MASK,
					((cfg_val >> 1) - 2));
		qdma_reg_write(dev_hndl, QDMA_OFFSET_C2H_PFETCH_CFG, reg_val);

		/* C2H interrupt timer tick */
		qdma_reg_write(dev_hndl, QDMA_OFFSET_C2H_INT_TIMER_TICK,
				DEFAULT_C2H_INTR_TIMER_TICK);

		/* C2h Completion Coalesce Configuration */
		cfg_val = qdma_reg_read(dev_hndl,
				QDMA_OFFSET_C2H_CMPT_COAL_BUF_DEPTH);
		reg_val =
			FIELD_SET(QDMA_C2H_TICK_CNT_MASK,
					DEFAULT_CMPT_COAL_TIMER_CNT) |
			FIELD_SET(QDMA_C2H_TICK_VAL_MASK,
					DEFAULT_CMPT_COAL_TIMER_TICK) |
			FIELD_SET(QDMA_C2H_MAX_BUF_SZ_MASK, cfg_val);
		qdma_reg_write(dev_hndl, QDMA_OFFSET_C2H_WRB_COAL_CFG, reg_val);

		/* H2C throttle Configuration*/
		reg_val =
			FIELD_SET(QDMA_H2C_DATA_THRESH_MASK,
					DEFAULT_H2C_THROT_DATA_THRESH) |
			FIELD_SET(QDMA_H2C_REQ_THROT_EN_DATA_MASK,
					DEFAULT_THROT_EN_DATA) |
			FIELD_SET(QDMA_H2C_REQ_THRESH_MASK,
					DEFAULT_H2C_THROT_REQ_THRESH) |
			FIELD_SET(QDMA_H2C_REQ_THROT_EN_REQ_MASK,
					DEFAULT_THROT_EN_REQ);
		qdma_reg_write(dev_hndl, QDMA_OFFSET_H2C_REQ_THROT, reg_val);
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_write_global_ring_sizes() - function to set the global ring size array
 *
 * @dev_hndl:   device handle
 * @index: Index from where the values needs to written
 * @count: number of entries to be written
 * @glbl_rng_sz: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_write_global_ring_sizes(void *dev_hndl, uint8_t index,
				uint8_t count, const uint32_t *glbl_rng_sz)
{
	if (!dev_hndl || !glbl_rng_sz || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_rng_sz=%p, err:%d\n",
					   __func__, dev_hndl, glbl_rng_sz,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_RING_SIZES) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_RING_SIZES,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_write_csr_values(dev_hndl, QDMA_OFFSET_GLBL_RNG_SZ, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_read_global_ring_sizes() - function to get the global rng_sz array
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_rng_sz: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_read_global_ring_sizes(void *dev_hndl, uint8_t index,
				uint8_t count, uint32_t *glbl_rng_sz)
{
	if (!dev_hndl || !glbl_rng_sz || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_rng_sz=%p, err:%d\n",
					   __func__, dev_hndl, glbl_rng_sz,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_RING_SIZES) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_BUFFER_SIZES,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_read_csr_values(dev_hndl, QDMA_OFFSET_GLBL_RNG_SZ, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_write_global_timer_count() - function to set the timer values
 *
 * @dev_hndl:   device handle
 * @glbl_tmr_cnt: pointer to the array having the values to write
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_write_global_timer_count(void *dev_hndl, uint8_t index,
				uint8_t count, const uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl || !glbl_tmr_cnt || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_tmr_cnt=%p, err:%d\n",
					   __func__, dev_hndl, glbl_tmr_cnt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_TIMERS) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_TIMERS,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_TIMER_CNT,
				index, count, glbl_tmr_cnt);
	else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_read_global_timer_count() - function to get the timer values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_tmr_cnt: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_read_global_timer_count(void *dev_hndl, uint8_t index,
				uint8_t count, uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_tmr_cnt || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_tmr_cnt=%p, err:%d\n",
					   __func__, dev_hndl, glbl_tmr_cnt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_TIMERS) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_TIMERS,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_read_csr_values(dev_hndl,
				QDMA_OFFSET_C2H_TIMER_CNT, index,
				count, glbl_tmr_cnt);
	else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_write_global_counter_threshold() - function to set the counter
 *						threshold values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 * @glbl_cnt_th: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_write_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_cnt_th || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_cnt_th=%p, err:%d\n",
					   __func__, dev_hndl, glbl_cnt_th,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_COUNTERS) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_BUFFER_SIZES,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_CNT_TH, index,
				count, glbl_cnt_th);
	else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_read_global_counter_threshold() - function to get the counter threshold
 * values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_cnt_th: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_read_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_cnt_th || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_cnt_th=%p, err:%d\n",
					   __func__, dev_hndl, glbl_cnt_th,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_COUNTERS) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_COUNTERS,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_read_csr_values(dev_hndl, QDMA_OFFSET_C2H_CNT_TH, index,
				count, glbl_cnt_th);
	else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
			   __func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_write_global_buffer_sizes() - function to set the buffer sizes
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 * @glbl_buf_sz: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_write_global_buffer_sizes(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes *dev_cap = NULL;

	if (!dev_hndl || !glbl_buf_sz || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_buf_sz=%p, err:%d\n",
					   __func__, dev_hndl, glbl_buf_sz,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_BUFFER_SIZES) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_BUFFER_SIZES,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_BUF_SZ, index,
				count, glbl_buf_sz);
	else {
		qdma_log_error("%s: ST not supported, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_read_global_buffer_sizes() - function to get the buffer sizes
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_buf_sz: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_read_global_buffer_sizes(void *dev_hndl, uint8_t index,
				uint8_t count, uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl || !glbl_buf_sz || !count) {
		qdma_log_error("%s: dev_hndl=%p glbl_buf_sz=%p, err:%d\n",
					   __func__, dev_hndl, glbl_buf_sz,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((index + count) > QDMA_NUM_C2H_BUFFER_SIZES) {
		qdma_log_error("%s: index=%u count=%u > %d, err:%d\n",
					   __func__, index, count,
					   QDMA_NUM_C2H_BUFFER_SIZES,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en)
		qdma_read_csr_values(dev_hndl, QDMA_OFFSET_C2H_BUF_SZ, index,
				count, glbl_buf_sz);
	else {
		qdma_log_error("%s: ST is not supported, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_global_csr_conf() - function to configure global csr
 *
 * @dev_hndl:	device handle
 * @index:	Index from where the values needs to read
 * @count:	number of entries to be read
 * @csr_val:	uint32_t pointer to csr value
 * @csr_type:	Type of the CSR (qdma_global_csr_type enum) to configure
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_CLEAR - Not supported
 *		QDMA_HW_ACCESS_INVALIDATE - Not supported
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_global_csr_conf(void *dev_hndl, uint8_t index, uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (csr_type) {
	case QDMA_CSR_RING_SZ:
		switch (access_type) {
		case QDMA_HW_ACCESS_READ:
			rv = qdma_read_global_ring_sizes(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv = qdma_write_global_ring_sizes(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		default:
			qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
							__func__,
							access_type,
						   -QDMA_ERR_INV_PARAM);
			rv = -QDMA_ERR_INV_PARAM;
			break;
		}
		break;
	case QDMA_CSR_TIMER_CNT:
		switch (access_type) {
		case QDMA_HW_ACCESS_READ:
			rv = qdma_read_global_timer_count(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv = qdma_write_global_timer_count(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		default:
			qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
							__func__,
							access_type,
						   -QDMA_ERR_INV_PARAM);
			rv = -QDMA_ERR_INV_PARAM;
			break;
		}
		break;
	case QDMA_CSR_CNT_TH:
		switch (access_type) {
		case QDMA_HW_ACCESS_READ:
			rv =
			qdma_read_global_counter_threshold(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv =
			qdma_write_global_counter_threshold(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		default:
			qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
							__func__,
							access_type,
						   -QDMA_ERR_INV_PARAM);
			rv = -QDMA_ERR_INV_PARAM;
			break;
		}
		break;
	case QDMA_CSR_BUF_SZ:
		switch (access_type) {
		case QDMA_HW_ACCESS_READ:
			rv =
			qdma_read_global_buffer_sizes(dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv =
			qdma_write_global_buffer_sizes(dev_hndl,
						index,
						count,
						csr_val);
			break;
		default:
			qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
							__func__,
							access_type,
						   -QDMA_ERR_INV_PARAM);
			rv = -QDMA_ERR_INV_PARAM;
			break;
		}
		break;
	default:
		qdma_log_error("%s: csr_type(%d) invalid, err:%d\n",
						__func__,
						csr_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_global_writeback_interval_write() -  function to set the writeback
 * interval
 *
 * @dev_hndl	device handle
 * @wb_int:	Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_global_writeback_interval_write(void *dev_hndl,
		enum qdma_wrb_interval wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (wb_int >=  QDMA_NUM_WRB_INTERVALS) {
		qdma_log_error("%s: wb_int=%d is invalid, err:%d\n",
					   __func__, wb_int,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG);
		reg_val |= FIELD_SET(QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK, wb_int);

		qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG, reg_val);
	} else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
			   __func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_global_writeback_interval_read() -  function to get the writeback
 * interval
 *
 * @dev_hndl:	device handle
 * @wb_int:	pointer to the data to hold Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_global_writeback_interval_read(void *dev_hndl,
		enum qdma_wrb_interval *wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG);
		*wb_int = (enum qdma_wrb_interval)FIELD_GET(
				QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK, reg_val);
	} else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
			   __func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_global_writeback_interval_conf() - function to configure
 *					the writeback interval
 *
 * @dev_hndl:   device handle
 * @wb_int:	pointer to the data to hold Writeback Interval
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_CLEAR - Not supported
 *		QDMA_HW_ACCESS_INVALIDATE - Not supported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_global_writeback_interval_read(dev_hndl, wb_int);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv = qdma_global_writeback_interval_write(dev_hndl, *wb_int);
		break;
	case QDMA_HW_ACCESS_CLEAR:
	case QDMA_HW_ACCESS_INVALIDATE:
	default:
		qdma_log_error("%s: access_type(%d) invalid, err:%d\n",
						__func__,
						access_type,
					   -QDMA_ERR_INV_PARAM);
		rv = -QDMA_ERR_INV_PARAM;
		break;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_queue_pidx_update() - function to update the desc PIDX
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @is_c2h:	Queue direction. Set 1 for C2H and 0 for H2C
 * @reg_info:	data needed for the PIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info)
{
	uint32_t reg_addr = 0;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
						__func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!is_vf) {
		reg_addr = (is_c2h) ?  QDMA_OFFSET_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_OFFSET_DMAP_SEL_H2C_DSC_PIDX;
	} else {
		reg_addr = (is_c2h) ?  QDMA_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX;
	}

	reg_addr += (qid * QDMA_PIDX_STEP);

	reg_val = FIELD_SET(QDMA_DMA_SEL_DESC_PIDX_MASK, reg_info->pidx) |
			  FIELD_SET(QDMA_DMA_SEL_IRQ_EN_MASK,
			  reg_info->irq_en);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_queue_cmpt_cidx_update() - function to update the CMPT CIDX update
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
		QDMA_OFFSET_DMAP_SEL_CMPT_CIDX;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
						__func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr += (qid * QDMA_CMPT_CIDX_STEP);

	reg_val =
		FIELD_SET(QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK,
				reg_info->wrb_cidx) |
		FIELD_SET(QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK,
				reg_info->counter_idx) |
		FIELD_SET(QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK,
				reg_info->timer_idx) |
		FIELD_SET(QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK,
				reg_info->trig_mode) |
		FIELD_SET(QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK,
				reg_info->wrb_en) |
		FIELD_SET(QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK, reg_info->irq_en);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_queue_intr_cidx_update() - function to update the CMPT CIDX update
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_intr_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_INT_CIDX :
		QDMA_OFFSET_DMAP_SEL_INT_CIDX;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr += qid * QDMA_INT_CIDX_STEP;

	reg_val =
		FIELD_SET(QDMA_DMA_SEL_INT_SW_CIDX_MASK, reg_info->sw_cidx) |
		FIELD_SET(QDMA_DMA_SEL_INT_RING_IDX_MASK, reg_info->rng_idx);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_queue_cmpt_cidx_read() - function to read the CMPT CIDX register
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	pointer to array to hold the values read
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_queue_cmpt_cidx_read(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
			QDMA_OFFSET_DMAP_SEL_CMPT_CIDX;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr += qid * QDMA_CMPT_CIDX_STEP;

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	reg_info->wrb_cidx =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK, reg_val);
	reg_info->counter_idx =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK,
			reg_val));
	reg_info->wrb_en =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK,
			reg_val));
	reg_info->irq_en =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK, reg_val));
	reg_info->timer_idx =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK, reg_val));
	reg_info->trig_mode =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK, reg_val));

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_mm_channel_conf() - Function to enable/disable the MM channel
 *
 * @dev_hndl:	device handle
 * @channel:	MM channel number
 * @is_c2h:	Queue direction. Set 1 for C2H and 0 for H2C
 * @enable:	Enable or disable MM channel
 *
 * Presently, we have only 1 MM channel
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_mm_channel_conf(void *dev_hndl, uint8_t channel, uint8_t is_c2h,
				uint8_t enable)
{
	uint32_t reg_addr = (is_c2h) ?  QDMA_OFFSET_C2H_MM_CONTROL :
			QDMA_OFFSET_H2C_MM_CONTROL;
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->mm_en) {
		qdma_reg_write(dev_hndl,
				reg_addr + (channel * QDMA_MM_CONTROL_STEP),
				enable);
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_initiate_flr() - function to initiate Function Level Reset
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_initiate_flr(void *dev_hndl, uint8_t is_vf)
{
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_reg_write(dev_hndl, reg_addr, 1);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_is_flr_done() - function to check whether the FLR is done or not
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @done:	if FLR process completed ,  done is 1 else 0.
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_is_flr_done(void *dev_hndl, uint8_t is_vf, uint8_t *done)
{
	int rv;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	/* wait for it to become zero */
	rv = hw_monitor_reg(dev_hndl, reg_addr, QDMA_FLR_STATUS_MASK,
			0, 5 * QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US);
	if (rv < 0)
		*done = 0;
	else
		*done = 1;

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_is_config_bar() - function for the config bar verification
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_is_config_bar(void *dev_hndl, uint8_t is_vf)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_CONFIG_BAR_ID :
			QDMA_OFFSET_CONFIG_BLOCK_ID;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
		__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	if (FIELD_GET(QDMA_CONFIG_BLOCK_ID_MASK, reg_val)
			!= QDMA_MAGIC_NUMBER) {
		if (is_vf)
			return QDMA_SUCCESS;
		qdma_log_error("%s: Invalid config bar, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_INV_CONFIG_BAR);
		return -QDMA_ERR_HWACC_INV_CONFIG_BAR;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_get_user_bar() - Function to get the user bar number
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @user_bar:	pointer to hold the user bar number
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_get_user_bar(void *dev_hndl, uint8_t is_vf, uint8_t *user_bar)
{
	uint8_t bar_found = 0;
	uint8_t bar_idx = 0;
	uint32_t func_id = 0;
	uint32_t user_bar_id = 0;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_USER_BAR_ID :
			QDMA_OFFSET_GLBL2_PF_BARLITE_EXT;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	user_bar_id = qdma_reg_read(dev_hndl, reg_addr);

	if (!is_vf) {
		func_id = qdma_reg_read(dev_hndl,
				QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET);
		user_bar_id = (user_bar_id >> (6 * func_id)) & 0x3F;
	} else {
		user_bar_id = user_bar_id & 0x3F;
	}

	for (bar_idx = 0; bar_idx < QDMA_BAR_NUM; bar_idx++) {
		if (user_bar_id & (1 << bar_idx)) {
			*user_bar = bar_idx;
			bar_found = 1;
			break;
		}
	}
	if (bar_found == 0) {
		*user_bar = 0;
		qdma_log_error("%s: Bar not found, vf:%d, usrbar:%d, err:%d\n",
					   __func__,
					   is_vf,
					   *user_bar,
					   -QDMA_ERR_HWACC_BAR_NOT_FOUND);
		return -QDMA_ERR_HWACC_BAR_NOT_FOUND;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_get_function_number() - Function to get the function number
 *
 * @dev_hndl:	device handle
 * @func_id:	pointer to hold the function id
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_get_function_number(void *dev_hndl, uint8_t *func_id)
{
	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	*func_id = (uint8_t)qdma_reg_read(dev_hndl,
			QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_get_rtl_version() - Function to get the rtl_version in
 * string format
 *
 * @rtl_version: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_rtl_version(enum qdma_rtl_version rtl_version)
{
	switch (rtl_version) {
	case QDMA_RTL_PATCH:
		return "RTL Patch";
	case QDMA_RTL_BASE:
		return "RTL Base";
	default:
		qdma_log_error("%s: invalid rtl_version(%d), err:%d\n",
				__func__, rtl_version, -QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_versal_ip_type() - Function to get the versal ip type in
 * string format
 *
 * @versal_ip_type: Versal IP Type
 * @device_type: Device Type
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_versal_ip_type(
			enum qdma_versal_ip_type versal_ip_type,
			enum qdma_device_type device_type)
{
	if (device_type != QDMA_DEVICE_SOFT) {
		switch (versal_ip_type) {
		case QDMA_VERSAL_SOFT_IP:
			return "Versal Soft IP";
		case QDMA_VERSAL_HARD_IP:
			return "Versal Hard IP";
		default:
			qdma_log_error("%s: inv versal_ip_type(%d), err:%d\n",
					__func__,
					versal_ip_type,
					-QDMA_ERR_INV_PARAM);
			return NULL;
		}
	} else
		return "Not an Versal Device";
}

/*****************************************************************************/
/**
 * qdma_get_device_type() - Function to get the device type in
 * string format
 *
 * @device_type: Device Type
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_device_type(enum qdma_device_type device_type)
{
	switch (device_type) {
	case QDMA_DEVICE_SOFT:
		return "Soft IP";
	case QDMA_DEVICE_VERSAL:
		return "Versal IP";
	case QDMA_DEVICE_VERSAL_CPM5:
		return "Versal CPM5 IP";
	default:
		qdma_log_error("%s: invalid device type(%d), err:%d\n",
				__func__, device_type, -QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_vivado_release_id() - Function to get the vivado release id in
 * string format
 *
 * @vivado_release_id: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_vivado_release_id(
				enum qdma_vivado_release_id vivado_release_id)
{
	switch (vivado_release_id) {
	case QDMA_VIVADO_2018_3:
		return "vivado 2018.3";
	case QDMA_VIVADO_2019_1:
		return "vivado 2019.1";
	case QDMA_VIVADO_2019_2:
		return "vivado 2019.2";
	default:
		qdma_log_error("%s: invalid vivado_release_id(%d), err:%d\n",
				__func__,
				vivado_release_id,
				-QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_version() - Function to get the qdma version
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @version_info:	pointer to hold the version info
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_get_version(void *dev_hndl, uint8_t is_vf,
		struct qdma_hw_version_info *version_info)
{
	uint32_t rtl_version, vivado_release_id, versal_ip_type;
	uint32_t device_type;
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_VERSION :
			QDMA_OFFSET_PF_VERSION;
	const char *version_str;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	rtl_version =
		FIELD_GET(QDMA_GLBL2_RTL_VERSION_MASK, reg_val);
	vivado_release_id =
		FIELD_GET(QDMA_GLBL2_VIVADO_RELEASE_MASK, reg_val);
	device_type =
		FIELD_GET(QDMA_GLBL2_DEVICE_TYPE, reg_val);
	versal_ip_type =
		FIELD_GET(QDMA_GLBL2_VERSAL_IP_STATE, reg_val);

	switch (rtl_version) {
	case RTL_PATCH_VERSION:
		version_info->rtl_version = QDMA_RTL_PATCH;
		break;
	default:
		version_info->rtl_version = QDMA_RTL_BASE;
		break;
	}

	version_str = qdma_get_rtl_version(version_info->rtl_version);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_rtl_version_str,
				version_str,
				QDMA_HW_VERSION_STRING_LEN);

	switch (device_type) {
	case 0:
		version_info->device_type = QDMA_DEVICE_SOFT;
		break;
	case 1:
		version_info->device_type = QDMA_DEVICE_VERSAL;
		break;
	case 2:
		version_info->device_type = QDMA_DEVICE_VERSAL_CPM5;
		break;
	default:
		version_info->device_type = QDMA_DEVICE_NONE;
		break;
	}

	version_str = qdma_get_device_type(version_info->device_type);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_device_type_str,
				version_str,
				QDMA_HW_VERSION_STRING_LEN);

	switch (versal_ip_type) {
	case 0:
		version_info->versal_ip_type = QDMA_VERSAL_HARD_IP;
		break;
	case 1:
		version_info->versal_ip_type = QDMA_VERSAL_SOFT_IP;
		break;
	default:
		version_info->versal_ip_type = QDMA_VERSAL_NONE;
		break;
	}

	version_str = qdma_get_versal_ip_type(version_info->versal_ip_type,
			version_info->device_type);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_versal_ip_type_str,
			version_str,
			QDMA_HW_VERSION_STRING_LEN);

	if (version_info->device_type == QDMA_DEVICE_SOFT) {
		switch (vivado_release_id) {
		case 0:
			version_info->vivado_release = QDMA_VIVADO_2018_3;
			break;
		case 1:
			version_info->vivado_release = QDMA_VIVADO_2019_1;
			break;
		case 2:
			version_info->vivado_release = QDMA_VIVADO_2019_2;
			break;
		default:
			version_info->vivado_release = QDMA_VIVADO_NONE;
			break;
		}

		version_str = qdma_get_vivado_release_id(
				version_info->vivado_release);
		if (version_str != NULL)
			qdma_strncpy(version_info->qdma_vivado_release_id_str,
					version_str,
					QDMA_HW_VERSION_STRING_LEN);
	}

	else if (version_info->device_type == QDMA_DEVICE_VERSAL) {
		switch (vivado_release_id) {
		case 0:
			version_info->vivado_release = QDMA_VIVADO_2019_2;
			break;
		default:
			version_info->vivado_release = QDMA_VIVADO_NONE;
			break;
		}

		version_str = qdma_get_vivado_release_id(
				version_info->vivado_release);
		if (version_str != NULL)
			qdma_strncpy(version_info->qdma_vivado_release_id_str,
					version_str,
					QDMA_HW_VERSION_STRING_LEN);
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_get_device_attributes() - Function to get the qdma device attributes
 *
 * @dev_hndl:	device handle
 * @dev_info:	pointer to hold the device info
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info)
{
	uint8_t count = 0;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	/* number of PFs */
	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL2_PF_BARLITE_INT);
	if (FIELD_GET(QDMA_GLBL2_PF0_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(QDMA_GLBL2_PF1_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(QDMA_GLBL2_PF2_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(QDMA_GLBL2_PF3_BAR_MAP_MASK, reg_val))
		count++;
	dev_info->num_pfs = count;

	/* Number of Qs */
	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL2_CHANNEL_QDMA_CAP);
	dev_info->num_qs = FIELD_GET(QDMA_GLBL2_MULTQ_MAX_MASK, reg_val);

	/* FLR present */
	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL2_MISC_CAP);
	dev_info->mailbox_en  = FIELD_GET(QDMA_GLBL2_MAILBOX_EN_MASK, reg_val);
	dev_info->flr_present = FIELD_GET(QDMA_GLBL2_FLR_PRESENT_MASK, reg_val);
	dev_info->mm_cmpt_en  = FIELD_GET(QDMA_GLBL2_MM_CMPT_EN_MASK, reg_val);

	/* ST/MM enabled? */
	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL2_CHANNEL_MDMA);
	dev_info->mm_en = (FIELD_GET(QDMA_GLBL2_MM_C2H_MASK, reg_val)
			&& FIELD_GET(QDMA_GLBL2_MM_H2C_MASK, reg_val)) ? 1 : 0;
	dev_info->st_en = (FIELD_GET(QDMA_GLBL2_ST_C2H_MASK, reg_val)
			&& FIELD_GET(QDMA_GLBL2_ST_H2C_MASK, reg_val)) ? 1 : 0;

	/* num of mm channels */
	/* TODO : Register not yet defined for this. Hard coding it to 1.*/
	dev_info->mm_channel_max = 1;

	dev_info->qid2vec_ctx = 0;
	dev_info->cmpt_ovf_chk_dis = 1;
	dev_info->mailbox_intr = 1;
	dev_info->sw_desc_64b = 1;
	dev_info->cmpt_desc_64b = 1;
	dev_info->dynamic_bar = 1;
	dev_info->legacy_intr = 1;
	dev_info->cmpt_trig_count_timer = 1;

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_error_intr_setup() - Function to set up the qdma error
 * interrupt
 *
 * @dev_hndl:	device handle
 * @func_id:	Function id
 * @err_intr_index:	Interrupt vector
 * @rearm:	rearm or not
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_intr_setup(void *dev_hndl, uint16_t func_id,
		uint8_t err_intr_index)
{
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val =
		FIELD_SET(QDMA_GLBL_ERR_FUNC_MASK, func_id) |
		FIELD_SET(QDMA_GLBL_ERR_VEC_MASK, err_intr_index);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_error_intr_rearm() - Function to re-arm the error interrupt
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_intr_rearm(void *dev_hndl)
{
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT);
	reg_val |= FIELD_SET(QDMA_GLBL_ERR_ARM_MASK, 1);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_error_enable() - Function to enable all or a specific error
 *
 * @dev_hndl: device handle
 * @err_idx: error index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_enable(void *dev_hndl, enum qdma_error_idx err_idx)
{
	uint32_t idx = 0, i = 0;
	uint32_t reg_val = 0;
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (err_idx > QDMA_ERRS_ALL) {
		qdma_log_error("%s: err_idx=%d is invalid, err:%d\n",
					   __func__, err_idx,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (err_idx == QDMA_ERRS_ALL) {
		for (i = 0; i < TOTAL_LEAF_ERROR_AGGREGATORS; i++) {

			idx = all_hw_errs[i];

			/* Don't access streaming registers in
			 * MM only bitstreams
			 */
			if (!dev_cap->st_en) {
				if (idx == QDMA_ST_C2H_ERR_ALL ||
					idx == QDMA_ST_FATAL_ERR_ALL ||
					idx == QDMA_ST_H2C_ERR_ALL)
					continue;
			}

			reg_val = err_info[idx].leaf_err_mask;
			qdma_reg_write(dev_hndl,
					err_info[idx].mask_reg_addr, reg_val);

			reg_val = qdma_reg_read(dev_hndl,
					QDMA_OFFSET_GLBL_ERR_MASK);
			reg_val |= FIELD_SET(err_info[idx].global_err_mask, 1);
			qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_MASK,
					reg_val);
		}

	} else {
		/* Don't access streaming registers in MM only bitstreams
		 *  QDMA_C2H_ERR_MTY_MISMATCH to QDMA_H2C_ERR_ALL are all
		 *  ST errors
		 */
		if (!dev_cap->st_en) {
			if (err_idx >= QDMA_ST_C2H_ERR_MTY_MISMATCH &&
					err_idx <= QDMA_ST_H2C_ERR_ALL)
				return QDMA_SUCCESS;
		}

		reg_val = qdma_reg_read(dev_hndl,
				err_info[err_idx].mask_reg_addr);
		reg_val |= FIELD_SET(err_info[err_idx].leaf_err_mask, 1);
		qdma_reg_write(dev_hndl,
				err_info[err_idx].mask_reg_addr, reg_val);

		reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_ERR_MASK);
		reg_val |= FIELD_SET(err_info[err_idx].global_err_mask, 1);
		qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_MASK, reg_val);
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_get_error_name() - Function to get the error in string format
 *
 * @err_idx: error index
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_hw_get_error_name(enum qdma_error_idx err_idx)
{
	if (err_idx >= QDMA_ERRS_ALL) {
		qdma_log_error("%s: err_idx=%d is invalid, returning NULL\n",
					   __func__, err_idx);
		return NULL;
	}

	return err_info[err_idx].err_name;
}

/*****************************************************************************/
/**
 * qdma_hw_error_process() - Function to find the error that got
 * triggered and call the handler qdma_hw_error_handler of that
 * particular error.
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_process(void *dev_hndl)
{
	uint32_t glbl_err_stat = 0, err_stat = 0;
	uint32_t bit = 0, i = 0;
	int32_t idx = 0;
	struct qdma_dev_attributes *dev_cap;
	uint32_t hw_err_position[TOTAL_LEAF_ERROR_AGGREGATORS] = {
		QDMA_DSC_ERR_POISON,
		QDMA_TRQ_ERR_UNMAPPED,
		QDMA_ST_C2H_ERR_MTY_MISMATCH,
		QDMA_ST_FATAL_ERR_MTY_MISMATCH,
		QDMA_ST_H2C_ERR_ZERO_LEN_DESC,
		QDMA_SBE_ERR_MI_H2C0_DAT,
		QDMA_DBE_ERR_MI_H2C0_DAT
	};

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	glbl_err_stat = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_ERR_STAT);
	if (!glbl_err_stat)
		return QDMA_HW_ERR_NOT_DETECTED;

	for (i = 0; i < TOTAL_LEAF_ERROR_AGGREGATORS; i++) {
		bit = hw_err_position[i];

		if ((!dev_cap->st_en) && (bit == QDMA_ST_C2H_ERR_MTY_MISMATCH ||
				bit == QDMA_ST_FATAL_ERR_MTY_MISMATCH ||
				bit == QDMA_ST_H2C_ERR_ZERO_LEN_DESC))
			continue;

		err_stat = qdma_reg_read(dev_hndl,
				err_info[bit].stat_reg_addr);
		if (err_stat)
			qdma_reg_write(dev_hndl, err_info[bit].stat_reg_addr,
					err_stat);
		else
			continue;
		for (idx = bit; idx < all_hw_errs[i]; idx++) {
			/* call the platform specific handler */
			if (err_stat & err_info[idx].leaf_err_mask)
				qdma_hw_error_handler(dev_hndl,
						(enum qdma_error_idx)idx);
		}
	}

	/* Write 1 to the global status register to clear the bits */
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_STAT, glbl_err_stat);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_dump_config_regs() - Function to get qdma config register dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen)
{
	unsigned int i = 0, j = 0;
	struct xreg_info *reg_info;
	uint32_t num_regs =
		sizeof(qdma_config_regs) / sizeof((qdma_config_regs)[0]);
	uint32_t len = 0, val = 0;
	int rv = QDMA_SUCCESS;
	char name[DEBGFS_GEN_NAME_SZ] = "";
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (buflen < qdma_reg_dump_buf_len()) {
		qdma_log_error("%s: Buffer too small, err:%d\n",
					__func__, -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	/*TODO : VF register space to be added later.*/
	if (is_vf) {
		qdma_log_error("%s: Not supported for VF, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	reg_info = qdma_config_regs;

	for (i = 0; i < num_regs - 1; i++) {
		if ((GET_CAPABILITY_MASK(dev_cap->mm_en, dev_cap->st_en,
				dev_cap->mm_cmpt_en, dev_cap->mailbox_en)
				& reg_info[i].mode) == 0)
			continue;

		for (j = 0; j < reg_info[i].repeat; j++) {
			QDMA_SNPRINTF(name, DEBGFS_GEN_NAME_SZ,
					"%s_%d", reg_info[i].name, j);
			val = 0;
			val = qdma_reg_read(dev_hndl,
					(reg_info[i].addr + (j * 4)));
			rv = dump_reg(buf + len, buflen - len,
					(reg_info[i].addr + (j * 4)),
						name, val);
			if (rv < 0) {
				qdma_log_error(
				"%s Buff too small, err:%d\n",
				__func__,
				-QDMA_ERR_NO_MEM);
				return -QDMA_ERR_NO_MEM;
			}
			len += rv;
		}
	}

	return len;
}

/*****************************************************************************/
/**
 * qdma_dump_queue_context() - Function to get qdma queue context dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @hw_qid:     queue id
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_dump_queue_context(void *dev_hndl, uint16_t hw_qid, uint8_t st,
		uint8_t c2h, char *buf, uint32_t buflen)
{
	int rv = 0;
	char pfetch_valid = 0;
	char cmpt_valid = 0;
	struct qdma_dev_attributes *dev_cap;
	struct qdma_descq_sw_ctxt sw_ctx;
	struct qdma_descq_hw_ctxt hw_ctx;
	struct qdma_descq_credit_ctxt credit_ctx;
	struct qdma_descq_prefetch_ctxt prefetch_ctx;
	struct qdma_descq_cmpt_ctxt cmpt_ctx;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (st && c2h)
		pfetch_valid = 1;
	if ((st && c2h) || (!st && dev_cap->mm_cmpt_en))
		cmpt_valid = 1;

	if (buflen < qdma_context_buf_len(pfetch_valid, cmpt_valid)) {
		qdma_log_error("%s: Buffer too small failure, err:%d\n",
			__func__, -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	if (!dev_cap->st_en && !dev_cap->mm_en) {
		qdma_log_error(
			"%s: ST or MM mode must be enabled, err:%d\n",
			__func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	/* sw context read */
	rv = qdma_sw_ctx_conf(dev_hndl, c2h,
		hw_qid, &sw_ctx, QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: Failed to read sw context, err:%d\n",
			__func__, rv);
		return rv;
	}
	qdma_acc_fill_sw_ctxt(&sw_ctx);

	/* hw context read */
	rv = qdma_hw_ctx_conf(dev_hndl, c2h,
		hw_qid, &hw_ctx, QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: Failed to read hw context, err:%d\n",
			__func__, rv);
		return rv;
	}
	qdma_acc_fill_hw_ctxt(&hw_ctx);

	/* credit context read */
	rv = qdma_credit_ctx_conf(dev_hndl, c2h,
		hw_qid, &credit_ctx, QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: Failed to read credit context, err:%d\n",
			__func__, rv);
		return rv;
	}
	qdma_acc_fill_credit_ctxt(&credit_ctx);

	if (pfetch_valid) {
		/* credit context read */
		rv = qdma_pfetch_ctx_conf(dev_hndl,
			hw_qid, &prefetch_ctx, QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
				"%s: Failed to read prefetch context, err:%d",
				__func__, rv);
			return rv;
		}
		qdma_acc_fill_pfetch_ctxt(&prefetch_ctx);
	}

	if (cmpt_valid) {
		/* completion context read */
		rv = qdma_cmpt_ctx_conf(dev_hndl,
			hw_qid, &cmpt_ctx, QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
				"%s: Failed to read completion context, err:%d",
				__func__, rv);
			return rv;
		}
		qdma_acc_fill_cmpt_ctxt(&cmpt_ctx);
	}

	rv = dump_context(buf, buflen, pfetch_valid, cmpt_valid);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_is_legacy_intr_pend() - function to get legacy_intr_pending status bit
 *
 * @dev_hndl: device handle
 *
 * Return: legacy interrupt pending status bit value
 *****************************************************************************/
static int qdma_is_legacy_intr_pend(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	if (FIELD_GET(QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK, reg_val))
		return QDMA_SUCCESS;

	qdma_log_error("%s: no pending legacy intr, err:%d\n",
				   __func__, -QDMA_ERR_INV_PARAM);
	return -QDMA_ERR_HWACC_NO_PEND_LEGCY_INTR;
}

/*****************************************************************************/
/**
 * qdma_clear_pend_legacy_intr() - function to clear legacy_intr_pending bit
 *
 * @dev_hndl: device handle
 *
 * Return: void
 *****************************************************************************/
static int qdma_clear_pend_legacy_intr(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	reg_val |= FIELD_SET(QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK, 1);
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_legacy_intr_conf() - function to disable/enable legacy interrupt
 *
 * @dev_hndl: device handle
 * @enable: enable/disable flag. 1 - enable, 0 - disable
 *
 * Return: void
 *****************************************************************************/
static int qdma_legacy_intr_conf(void *dev_hndl, enum status_type enable)
{
	uint32_t reg_val;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	reg_val |= FIELD_SET(QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK, enable);
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_init_ctxt_memory() - function to initialize the context memory
 *
 * @dev_hndl: device handle
 *
 * Return: returns the platform specific error code
 *****************************************************************************/
static int qdma_init_ctxt_memory(void *dev_hndl)
{
#ifdef ENABLE_INIT_CTXT_MEMORY
	uint32_t data[QDMA_REG_IND_CTXT_REG_COUNT];
	uint16_t i = 0;
	struct qdma_dev_attributes dev_info;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_memset(data, 0, sizeof(uint32_t) * QDMA_REG_IND_CTXT_REG_COUNT);
	qdma_get_device_attributes(dev_hndl, &dev_info);

	for (; i < dev_info.num_qs; i++) {
		int sel = QDMA_CTXT_SEL_SW_C2H;
		int rv;

		for (; sel <= QDMA_CTXT_SEL_PFTCH; sel++) {
			/** if the st mode(h2c/c2h) not enabled
			 *  in the design, then skip the PFTCH
			 *  and CMPT context setup
			 */
			if ((dev_info.st_en == 0) &&
			    ((sel == QDMA_CTXT_SEL_PFTCH) ||
				(sel == QDMA_CTXT_SEL_CMPT))) {
				qdma_log_debug("%s: ST context is skipped:",
					__func__);
				qdma_log_debug("sel = %d\n", sel);
				continue;
			}

			rv = qdma_indirect_reg_clear(dev_hndl,
					(enum ind_ctxt_cmd_sel)sel, i);
			if (rv < 0)
				return rv;
		}
	}

	/* fmap */
	for (i = 0; i < dev_info.num_pfs; i++)
		qdma_indirect_reg_clear(dev_hndl,
				QDMA_CTXT_SEL_FMAP, i);

#else
	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
#endif
	return QDMA_SUCCESS;

}

/*****************************************************************************/
/**
 * qdma_get_error_code() - function to get the qdma access mapped
 *				error code
 *
 * @acc_err_code: qdma access error code
 *
 * Return:   returns the platform specific error code
 *****************************************************************************/
int qdma_get_error_code(int acc_err_code)
{
	return qdma_get_err_code(acc_err_code);
}

int qdma_hw_access_init(void *dev_hndl, uint8_t is_vf,
				struct qdma_hw_access *hw_access)
{
	int rv = QDMA_SUCCESS;

	struct qdma_hw_version_info version_info;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_is_config_bar(dev_hndl, is_vf);
	if (rv != QDMA_SUCCESS) {
		qdma_log_error("%s: config bar passed is INVALID, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return rv;
	}

	hw_access->qdma_init_ctxt_memory = &qdma_init_ctxt_memory;
	hw_access->qdma_fmap_conf = &qdma_fmap_conf;
	hw_access->qdma_sw_ctx_conf = &qdma_sw_ctx_conf;
	hw_access->qdma_pfetch_ctx_conf = &qdma_pfetch_ctx_conf;
	hw_access->qdma_cmpt_ctx_conf = &qdma_cmpt_ctx_conf;
	hw_access->qdma_hw_ctx_conf = &qdma_hw_ctx_conf;
	hw_access->qdma_credit_ctx_conf = &qdma_credit_ctx_conf;
	hw_access->qdma_indirect_intr_ctx_conf = &qdma_indirect_intr_ctx_conf;
	hw_access->qdma_set_default_global_csr = &qdma_set_default_global_csr;
	hw_access->qdma_global_csr_conf = &qdma_global_csr_conf;
	hw_access->qdma_global_writeback_interval_conf =
					&qdma_global_writeback_interval_conf;
	hw_access->qdma_queue_pidx_update = &qdma_queue_pidx_update;
	hw_access->qdma_queue_cmpt_cidx_read = &qdma_queue_cmpt_cidx_read;
	hw_access->qdma_queue_cmpt_cidx_update = &qdma_queue_cmpt_cidx_update;
	hw_access->qdma_queue_intr_cidx_update = &qdma_queue_intr_cidx_update;
	hw_access->qdma_mm_channel_conf = &qdma_mm_channel_conf;
	hw_access->qdma_get_user_bar = &qdma_get_user_bar;
	hw_access->qdma_get_function_number = &qdma_get_function_number;
	hw_access->qdma_get_version = &qdma_get_version;
	hw_access->qdma_get_device_attributes = &qdma_get_device_attributes;
	hw_access->qdma_hw_error_intr_setup = &qdma_hw_error_intr_setup;
	hw_access->qdma_hw_error_intr_rearm = &qdma_hw_error_intr_rearm;
	hw_access->qdma_hw_error_enable = &qdma_hw_error_enable;
	hw_access->qdma_hw_get_error_name = &qdma_hw_get_error_name;
	hw_access->qdma_hw_error_process = &qdma_hw_error_process;
	hw_access->qdma_dump_config_regs = &qdma_dump_config_regs;
	hw_access->qdma_dump_queue_context = &qdma_dump_queue_context;
	hw_access->qdma_is_legacy_intr_pend = &qdma_is_legacy_intr_pend;
	hw_access->qdma_clear_pend_legacy_intr = &qdma_clear_pend_legacy_intr;
	hw_access->qdma_legacy_intr_conf = &qdma_legacy_intr_conf;
	hw_access->qdma_initiate_flr = &qdma_initiate_flr;
	hw_access->qdma_is_flr_done = &qdma_is_flr_done;
	hw_access->qdma_get_error_code = &qdma_get_error_code;

	rv = qdma_get_version(dev_hndl, is_vf, &version_info);
	if (rv != QDMA_SUCCESS)
		return rv;

	if ((version_info.device_type == QDMA_DEVICE_VERSAL) &&
			(version_info.versal_ip_type == QDMA_VERSAL_HARD_IP)) {
		hw_access->qdma_init_ctxt_memory = &qdma_cpm_init_ctxt_memory;
		hw_access->qdma_qid2vec_conf = &qdma_cpm_qid2vec_conf;
		hw_access->qdma_fmap_conf = &qdma_cpm_fmap_conf;
		hw_access->qdma_sw_ctx_conf = &qdma_cpm_sw_ctx_conf;
		hw_access->qdma_pfetch_ctx_conf = &qdma_cpm_pfetch_ctx_conf;
		hw_access->qdma_cmpt_ctx_conf = &qdma_cpm_cmpt_ctx_conf;
		hw_access->qdma_hw_ctx_conf = &qdma_cpm_hw_ctx_conf;
		hw_access->qdma_credit_ctx_conf = &qdma_cpm_credit_ctx_conf;
		hw_access->qdma_indirect_intr_ctx_conf =
				&qdma_cpm_indirect_intr_ctx_conf;
		hw_access->qdma_set_default_global_csr =
					&qdma_cpm_set_default_global_csr;
		hw_access->qdma_queue_pidx_update = &qdma_cpm_queue_pidx_update;
		hw_access->qdma_queue_cmpt_cidx_update =
					&qdma_cpm_queue_cmpt_cidx_update;
		hw_access->qdma_queue_intr_cidx_update =
					&qdma_cpm_queue_intr_cidx_update;
		hw_access->qdma_get_user_bar = &qdma_cmp_get_user_bar;
		hw_access->qdma_get_device_attributes =
					&qdma_cpm_get_device_attributes;
		hw_access->qdma_dump_config_regs = &qdma_cpm_dump_config_regs;

		hw_access->qdma_legacy_intr_conf = NULL;
		hw_access->qdma_dump_queue_context =
					&qdma_cpm_dump_queue_context;
	}

	return QDMA_SUCCESS;
}
