/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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

#include "qdma_s80_hard_access.h"
#include "qdma_s80_hard_reg.h"
#include "qdma_reg_dump.h"

#ifdef ENABLE_WPP_TRACING
#include "qdma_s80_hard_access.tmh"
#endif

/** QDMA S80 Hard Context array size */
#define QDMA_S80_HARD_SW_CONTEXT_NUM_WORDS              4
#define QDMA_S80_HARD_CMPT_CONTEXT_NUM_WORDS            4
#define QDMA_S80_HARD_QID2VEC_CONTEXT_NUM_WORDS         1
#define QDMA_S80_HARD_HW_CONTEXT_NUM_WORDS              2
#define QDMA_S80_HARD_CR_CONTEXT_NUM_WORDS              1
#define QDMA_S80_HARD_IND_INTR_CONTEXT_NUM_WORDS        3
#define QDMA_S80_HARD_PFETCH_CONTEXT_NUM_WORDS          2

#define QDMA_S80_HARD_VF_USER_BAR_ID   2

#define QDMA_S80_REG_GROUP_1_START_ADDR	0x000
#define QDMA_S80_REG_GROUP_2_START_ADDR	0x400
#define QDMA_S80_REG_GROUP_3_START_ADDR	0xB00
#define QDMA_S80_REG_GROUP_4_START_ADDR	0x1014

#define QDMA_S80_HARD_REG_TRQ_SEL_FMAP_STEP	4

#define QDMA_S80_HARD_IND_CTXT_DATA_NUM_REGS	4

#define QDMA_S80_HARD_TOTAL_LEAF_ERROR_AGGREGATORS	7
#define QDMA_S80_HARD_GLBL_TRQ_ERR_ALL_MASK			0XB3
#define QDMA_S80_HARD_GLBL_DSC_ERR_ALL_MASK			0X1F9037E
#define QDMA_S80_HARD_C2H_ERR_ALL_MASK				0X3F6DF
#define QDMA_S80_HARD_C2H_FATAL_ERR_ALL_MASK			0X1FDF1B
#define QDMA_S80_HARD_H2C_ERR_ALL_MASK				0X3F
#define QDMA_S80_HARD_SBE_ERR_ALL_MASK				0XFFFFFFFF
#define QDMA_S80_HARD_DBE_ERR_ALL_MASK				0XFFFFFFFF

#define QDMA_S80_HARD_OFFSET_DMAP_SEL_INT_CIDX                  0x6400
#define QDMA_S80_HARD_OFFSET_DMAP_SEL_H2C_DSC_PIDX          0x6404
#define QDMA_S80_HARD_OFFSET_DMAP_SEL_C2H_DSC_PIDX          0x6408
#define QDMA_S80_HARD_OFFSET_DMAP_SEL_CMPT_CIDX               0x640C

#define QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_INT_CIDX             0x3000
#define QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX     0x3004
#define QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX     0x3008
#define QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_CMPT_CIDX          0x300C

#define QDMA_S80_HARD_DMA_SEL_INT_SW_CIDX_MASK               GENMASK(15, 0)
#define QDMA_S80_HARD_DMA_SEL_INT_RING_IDX_MASK              GENMASK(23, 16)
#define QDMA_S80_HARD_DMA_SEL_DESC_PIDX_MASK                   GENMASK(15, 0)
#define QDMA_S80_HARD_DMA_SEL_IRQ_EN_MASK                        BIT(16)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_IRQ_EN_MASK             BIT(28)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_STS_DESC_EN_MASK    BIT(27)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_TRG_MODE_MASK        GENMASK(26, 24)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_TMR_CNT_MASK          GENMASK(23, 20)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_CNT_THRESH_MASK     GENMASK(19, 16)
#define QDMA_S80_HARD_DMAP_SEL_CMPT_WRB_CIDX_MASK        GENMASK(15, 0)
#define QDMA_S80_HARD_INTR_CTXT_BADDR_GET_H_MASK     GENMASK_ULL(63, 35)
#define QDMA_S80_HARD_INTR_CTXT_BADDR_GET_L_MASK     GENMASK_ULL(34, 12)
#define QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_H_MASK    GENMASK_ULL(63, 42)
#define QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_M_MASK    GENMASK_ULL(41, 10)
#define QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_L_MASK    GENMASK_ULL(9, 6)
#define QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_H_MASK     GENMASK(15, 8)
#define QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_L_MASK     GENMASK(7, 0)
#define QDMA_S80_HARD_QID2VEC_H2C_VECTOR             GENMASK(16, 9)
#define QDMA_S80_HARD_QID2VEC_H2C_COAL_EN            BIT(17)

static void qdma_s80_hard_hw_st_h2c_err_process(void *dev_hndl);
static void qdma_s80_hard_hw_st_c2h_err_process(void *dev_hndl);
static void qdma_s80_hard_hw_desc_err_process(void *dev_hndl);
static void qdma_s80_hard_hw_trq_err_process(void *dev_hndl);
static void qdma_s80_hard_hw_ram_sbe_err_process(void *dev_hndl);
static void qdma_s80_hard_hw_ram_dbe_err_process(void *dev_hndl);

static struct qdma_s80_hard_hw_err_info
		qdma_s80_hard_err_info[QDMA_S80_HARD_ERRS_ALL] = {
	/* Descriptor errors */
	{
		QDMA_S80_HARD_DSC_ERR_POISON,
		"Poison error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_POISON_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_UR_CA,
		"Unsupported request or completer aborted error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_UR_CA_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_PARAM,
		"Parameter mismatch error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_PARAM_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_ADDR,
		"Address mismatch error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_ADDR_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_TAG,
		"Unexpected tag error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_TAG_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_FLR,
		"FLR error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_FLR_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_TIMEOUT,
		"Timed out error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_TIMEOUT_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_DAT_POISON,
		"Poison data error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_DAT_POISON_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_FLR_CANCEL,
		"Descriptor fetch cancelled due to FLR error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_FLR_CANCEL_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_DMA,
		"DMA engine error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_DMA_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_DSC,
		"Invalid PIDX update error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_DSC_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_RQ_CANCEL,
		"Descriptor fetch cancelled due to disable register status error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_RQ_CANCEL_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_DBE,
		"UNC_ERR_RAM_DBE error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_DBE_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_SBE,
		"UNC_ERR_RAM_SBE error",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		GLBL_DSC_ERR_STS_SBE_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},
	{
		QDMA_S80_HARD_DSC_ERR_ALL,
		"All Descriptor errors",
		QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		QDMA_S80_HARD_DBE_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_DSC_MASK,
		&qdma_s80_hard_hw_desc_err_process
	},

	/* TRQ errors */
	{
		QDMA_S80_HARD_TRQ_ERR_UNMAPPED,
		"Access targeted unmapped register space via CSR pathway error",
		QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		GLBL_TRQ_ERR_STS_UNMAPPED_MASK,
		GLBL_ERR_STAT_ERR_TRQ_MASK,
		&qdma_s80_hard_hw_trq_err_process
	},
	{
		QDMA_S80_HARD_TRQ_ERR_QID_RANGE,
		"Qid range error",
		QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		GLBL_TRQ_ERR_STS_QID_RANGE_MASK,
		GLBL_ERR_STAT_ERR_TRQ_MASK,
		&qdma_s80_hard_hw_trq_err_process
	},
	{
		QDMA_S80_HARD_TRQ_ERR_VF_ACCESS_ERR,
		"VF attempted to access Global register space or Function map",
		QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		GLBL_TRQ_ERR_STS_VF_ACCESS_ERR_MASK,
		GLBL_ERR_STAT_ERR_TRQ_MASK,
		&qdma_s80_hard_hw_trq_err_process
	},
	{
		QDMA_S80_HARD_TRQ_ERR_TCP_TIMEOUT,
		"Timeout on request to dma internal csr register",
		QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		GLBL_TRQ_ERR_STS_TCP_TIMEOUT_MASK,
		GLBL_ERR_STAT_ERR_TRQ_MASK,
		&qdma_s80_hard_hw_trq_err_process
	},
	{
		QDMA_S80_HARD_TRQ_ERR_ALL,
		"All TRQ errors",
		QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_TRQ_MASK,
		&qdma_s80_hard_hw_trq_err_process
	},

	/* C2H Errors*/
	{
		QDMA_S80_HARD_ST_C2H_ERR_MTY_MISMATCH,
		"MTY mismatch error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_MTY_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_LEN_MISMATCH,
		"Packet length mismatch error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_LEN_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_QID_MISMATCH,
		"Qid mismatch error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_QID_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_DESC_RSP_ERR,
		"Descriptor error bit set",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_DESC_RSP_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_ENG_WPL_DATA_PAR_ERR,
		"Data parity error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_ENG_WPL_DATA_PAR_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_MSI_INT_FAIL,
		"MSI got a fail response error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_MSI_INT_FAIL_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_ERR_DESC_CNT,
		"Descriptor count error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_ERR_DESC_CNT_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_PORTID_CTXT_MISMATCH,
		"Port id in packet and pfetch ctxt mismatch error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_PORT_ID_CTXT_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_PORTID_BYP_IN_MISMATCH,
		"Port id in packet and bypass in mismatch error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_PORT_ID_CTXT_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_WRB_INV_Q_ERR,
		"Writeback on invalid queue error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_WRB_INV_Q_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_WRB_QFULL_ERR,
		"Completion queue gets full error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_WRB_QFULL_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_WRB_CIDX_ERR,
		"Bad CIDX update by the software error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_WRB_CIDX_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_WRB_PRTY_ERR,
		"C2H completion Parity error",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		C2H_ERR_STAT_WRB_PRTY_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_C2H_ERR_ALL,
		"All C2h errors",
		QDMA_S80_HARD_C2H_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		QDMA_S80_HARD_C2H_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},

	/* C2H fatal errors */
	{
		QDMA_S80_HARD_ST_FATAL_ERR_MTY_MISMATCH,
		"Fatal MTY mismatch error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_MTY_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_LEN_MISMATCH,
		"Fatal Len mismatch error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_LEN_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_QID_MISMATCH,
		"Fatal Qid mismatch error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_QID_MISMATCH_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_TIMER_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_TIMER_FIFO_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_PFCH_II_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_PFCH_LL_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_WRB_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_WRB_CTXT_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_PFCH_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_PFCH_CTXT_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_DESC_REQ_FIFO_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_INT_CTXT_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_INT_CTXT_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_INT_QID2VEC_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_INT_QID2VEC_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_WRB_COAL_DATA_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_WRB_COAL_DATA_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_TUSER_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_TUSER_FIFO_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_QID_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_QID_FIFO_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_PLD_FIFO_RAM_RDBE_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_WPL_DATA_PAR_ERR,
		"RAM double bit fatal error",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		C2H_FATAL_ERR_STAT_WPL_DATA_PAR_ERR_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},
	{
		QDMA_S80_HARD_ST_FATAL_ERR_ALL,
		"All fatal errors",
		QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_C2H_ST_MASK,
		&qdma_s80_hard_hw_st_c2h_err_process
	},

	/* H2C St errors */
	{
		QDMA_S80_HARD_ST_H2C_ERR_ZERO_LEN_DESC_ERR,
		"Zero length descriptor error",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		H2C_ERR_STAT_ZERO_LEN_DS_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},
	{
		QDMA_S80_HARD_ST_H2C_ERR_SDI_MRKR_REQ_MOP_ERR,
		"A non-EOP descriptor received",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		H2C_ERR_STAT_SDI_MRKR_REQ_MOP_ERR_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},
	{
		QDMA_S80_HARD_ST_H2C_ERR_NO_DMA_DSC,
		"No DMA descriptor received error",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		H2C_ERR_STAT_NO_DMA_DS_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},
	{
		QDMA_S80_HARD_ST_H2C_ERR_DBE,
		"Double bit error detected on H2C-ST data error",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		H2C_ERR_STAT_DBE_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},
	{
		QDMA_S80_HARD_ST_H2C_ERR_SBE,
		"Single bit error detected on H2C-ST data error",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		H2C_ERR_STAT_SBE_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},
	{
		QDMA_S80_HARD_ST_H2C_ERR_ALL,
		"All H2C errors",
		QDMA_S80_HARD_H2C_ERR_MASK_ADDR,
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		QDMA_S80_HARD_H2C_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_H2C_ST_MASK,
		&qdma_s80_hard_hw_st_h2c_err_process
	},

	/* SBE errors */
	{
		QDMA_S80_HARD_SBE_ERR_MI_H2C0_DAT,
		"H2C MM data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_MI_H2C0_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_MI_C2H0_DAT,
		"C2H MM data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_MI_C2H0_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_H2C_RD_BRG_DAT,
		"Bridge master read single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_H2C_RD_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_H2C_WR_BRG_DAT,
		"Bridge master write single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_H2C_WR_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_C2H_RD_BRG_DAT,
		"Bridge slave read data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_C2H_RD_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_C2H_WR_BRG_DAT,
		"Bridge slave write data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_C2H_WR_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_FUNC_MAP,
		"Function map RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_FUNC_MAP_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DSC_HW_CTXT,
		"Descriptor engine hardware context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DSC_HW_CTXT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DSC_CRD_RCV,
		"Descriptor engine receive credit context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DSC_CRD_RCV_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DSC_SW_CTXT,
		"Descriptor engine software context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DSC_SW_CTXT_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DSC_CPLI,
		"Descriptor engine fetch completion information RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DSC_CPLI_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DSC_CPLD,
		"Descriptor engine fetch completion data RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DSC_CPLD_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_PASID_CTXT_RAM,
		"Pasid ctxt FIFO RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_PASID_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_TIMER_FIFO_RAM,
		"Timer fifo RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_TIMER_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_PAYLOAD_FIFO_RAM,
		"C2H ST payload FIFO RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_PLD_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_QID_FIFO_RAM,
		"C2H ST QID FIFO RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_QID_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_TUSER_FIFO_RAM,
		"C2H ST TUSER FIFO RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_TUSER_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_WRB_COAL_DATA_RAM,
		"Writeback Coalescing RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_WRB_COAL_DATA_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_INT_QID2VEC_RAM,
		"Interrupt QID2VEC RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_INT_QID2VEC_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_INT_CTXT_RAM,
		"Interrupt context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_INT_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_DESC_REQ_FIFO_RAM,
		"C2H ST descriptor request RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_DESC_REQ_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_PFCH_CTXT_RAM,
		"C2H ST prefetch RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_PFCH_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_WRB_CTXT_RAM,
		"C2H ST completion context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_WRB_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_PFCH_LL_RAM,
		"C2H ST prefetch list RAM single bit ECC error",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		RAM_SBE_STS_A_PFCH_LL_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},
	{
		QDMA_S80_HARD_SBE_ERR_ALL,
		"All SBE errors",
		QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
		QDMA_S80_HARD_SBE_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_RAM_SBE_MASK,
		&qdma_s80_hard_hw_ram_sbe_err_process
	},


	/* DBE errors */
	{
		QDMA_S80_HARD_DBE_ERR_MI_H2C0_DAT,
		"H2C MM data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_MI_H2C0_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_MI_C2H0_DAT,
		"C2H MM data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_MI_C2H0_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_H2C_RD_BRG_DAT,
		"Bridge master read single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_H2C_RD_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_H2C_WR_BRG_DAT,
		"Bridge master write single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_H2C_WR_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_C2H_RD_BRG_DAT,
		"Bridge slave read data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_C2H_RD_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_C2H_WR_BRG_DAT,
		"Bridge slave write data buffer single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_C2H_WR_BRG_DAT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_FUNC_MAP,
		"Function map RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_FUNC_MAP_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DSC_HW_CTXT,
		"Descriptor engine hardware context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DSC_HW_CTXT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DSC_CRD_RCV,
		"Descriptor engine receive credit context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DSC_CRD_RCV_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DSC_SW_CTXT,
		"Descriptor engine software context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DSC_SW_CTXT_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DSC_CPLI,
		"Descriptor engine fetch completion information RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DSC_CPLI_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DSC_CPLD,
		"Descriptor engine fetch completion data RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DSC_CPLD_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_PASID_CTXT_RAM,
		"PASID CTXT RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_PASID_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_TIMER_FIFO_RAM,
		"Timer fifo RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_TIMER_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_PAYLOAD_FIFO_RAM,
		"Payload fifo RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_PLD_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_QID_FIFO_RAM,
		"C2H ST QID FIFO RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_QID_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_WRB_COAL_DATA_RAM,
		"Writeback Coalescing RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_WRB_COAL_DATA_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_INT_QID2VEC_RAM,
		"QID2VEC RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_INT_QID2VEC_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_INT_CTXT_RAM,
		"Interrupt context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_INT_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_DESC_REQ_FIFO_RAM,
		"C2H ST descriptor request RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_DESC_REQ_FIFO_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_PFCH_CTXT_RAM,
		"C2H ST prefetch RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_PFCH_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_WRB_CTXT_RAM,
		"C2H ST completion context RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_WRB_CTXT_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_PFCH_LL_RAM,
		"C2H ST prefetch list RAM single bit ECC error",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		RAM_DBE_STS_A_PFCH_LL_RAM_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	},
	{
		QDMA_S80_HARD_DBE_ERR_ALL,
		"All DBE errors",
		QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR,
		QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
		QDMA_S80_HARD_DBE_ERR_ALL_MASK,
		GLBL_ERR_STAT_ERR_RAM_DBE_MASK,
		&qdma_s80_hard_hw_ram_dbe_err_process
	}
};

static int32_t all_qdma_s80_hard_hw_errs[
		QDMA_S80_HARD_TOTAL_LEAF_ERROR_AGGREGATORS] = {

	QDMA_S80_HARD_DSC_ERR_ALL,
	QDMA_S80_HARD_TRQ_ERR_ALL,
	QDMA_S80_HARD_ST_C2H_ERR_ALL,
	QDMA_S80_HARD_ST_FATAL_ERR_ALL,
	QDMA_S80_HARD_ST_H2C_ERR_ALL,
	QDMA_S80_HARD_SBE_ERR_ALL,
	QDMA_S80_HARD_DBE_ERR_ALL
};



union qdma_s80_hard_ind_ctxt_cmd {
	uint32_t word;
	struct {
		uint32_t busy:1;
		uint32_t sel:4;
		uint32_t op:2;
		uint32_t qid:11;
		uint32_t rsvd:14;
	} bits;
};

struct qdma_s80_hard_indirect_ctxt_regs {
	uint32_t qdma_ind_ctxt_data[QDMA_S80_HARD_IND_CTXT_DATA_NUM_REGS];
	uint32_t qdma_ind_ctxt_mask[QDMA_S80_HARD_IND_CTXT_DATA_NUM_REGS];
	union qdma_s80_hard_ind_ctxt_cmd cmd;
};

static struct qctx_entry qdma_s80_hard_sw_ctxt_entries[] = {
	{"PIDX", 0},
	{"IRQ Arm", 0},
	{"Queue Enable", 0},
	{"Fetch Credit Enable", 0},
	{"Write back/Intr Check", 0},
	{"Write back/Intr Interval", 0},
	{"Function Id", 0},
	{"Ring Size", 0},
	{"Descriptor Size", 0},
	{"Bypass Enable", 0},
	{"MM Channel", 0},
	{"Writeback Enable", 0},
	{"Interrupt Enable", 0},
	{"Port Id", 0},
	{"Interrupt No Last", 0},
	{"Error", 0},
	{"Writeback Error Sent", 0},
	{"IRQ Request", 0},
	{"Marker Disable", 0},
	{"Is Memory Mapped", 0},
	{"Descriptor Ring Base Addr (Low)", 0},
	{"Descriptor Ring Base Addr (High)", 0},
};

static struct qctx_entry qdma_s80_hard_hw_ctxt_entries[] = {
	{"CIDX", 0},
	{"Credits Consumed", 0},
	{"Descriptors Pending", 0},
	{"Queue Invalid No Desc Pending", 0},
	{"Eviction Pending", 0},
	{"Fetch Pending", 0},
};

static struct qctx_entry qdma_s80_hard_credit_ctxt_entries[] = {
	{"Credit", 0},
};

static struct qctx_entry qdma_s80_hard_cmpt_ctxt_entries[] = {
	{"Enable Status Desc Update", 0},
	{"Enable Interrupt", 0},
	{"Trigger Mode", 0},
	{"Function Id", 0},
	{"Counter Index", 0},
	{"Timer Index", 0},
	{"Interrupt State", 0},
	{"Color", 0},
	{"Ring Size", 0},
	{"Base Address (Low)", 0},
	{"Base Address (High)", 0},
	{"Descriptor Size", 0},
	{"PIDX", 0},
	{"CIDX", 0},
	{"Valid", 0},
	{"Error", 0},
	{"Trigger Pending", 0},
	{"Timer Running", 0},
	{"Full Update", 0},
};

static struct qctx_entry qdma_s80_hard_c2h_pftch_ctxt_entries[] = {
	{"Bypass", 0},
	{"Buffer Size Index", 0},
	{"Port Id", 0},
	{"Error", 0},
	{"Prefetch Enable", 0},
	{"In Prefetch", 0},
	{"Software Credit", 0},
	{"Valid", 0},
};

static struct qctx_entry qdma_s80_hard_qid2vec_ctxt_entries[] = {
	{"c2h_vector", 0},
	{"c2h_en_coal", 0},
	{"h2c_vector", 0},
	{"h2c_en_coal", 0},
};

static struct qctx_entry qdma_s80_hard_ind_intr_ctxt_entries[] = {
	{"valid", 0},
	{"vec", 0},
	{"int_st", 0},
	{"color", 0},
	{"baddr_4k (Low)", 0},
	{"baddr_4k (High)", 0},
	{"page_size", 0},
	{"pidx", 0},
};

static int qdma_s80_hard_indirect_reg_invalidate(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid);
static int qdma_s80_hard_indirect_reg_clear(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid);
static int qdma_s80_hard_indirect_reg_read(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t cnt, uint32_t *data);
static int qdma_s80_hard_indirect_reg_write(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t *data, uint16_t cnt);

uint32_t qdma_s80_hard_get_config_num_regs(void)
{
	return qdma_s80_hard_config_num_regs_get();
}

struct xreg_info *qdma_s80_hard_get_config_regs(void)
{
	return qdma_s80_hard_config_regs_get();
}

uint32_t qdma_s80_hard_reg_dump_buf_len(void)
{
	uint32_t length = (qdma_s80_hard_config_num_regs_get() + 1)
			* REG_DUMP_SIZE_PER_LINE;
	return length;
}

int qdma_s80_hard_context_buf_len(uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *req_buflen)
{
	uint32_t len = 0;
	int rv = 0;

	if (q_type == QDMA_DEV_Q_TYPE_CMPT) {
		len += (((sizeof(qdma_s80_hard_cmpt_ctxt_entries) /
			sizeof(qdma_s80_hard_cmpt_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);
	} else {
		len += (((sizeof(qdma_s80_hard_sw_ctxt_entries) /
				sizeof(qdma_s80_hard_sw_ctxt_entries[0])) + 1) *
				REG_DUMP_SIZE_PER_LINE);

		len += (((sizeof(qdma_s80_hard_hw_ctxt_entries) /
			sizeof(qdma_s80_hard_hw_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

		len += (((sizeof(qdma_s80_hard_credit_ctxt_entries) /
			sizeof(qdma_s80_hard_credit_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

		if (st && (q_type == QDMA_DEV_Q_TYPE_C2H)) {
			len += (((sizeof(qdma_s80_hard_cmpt_ctxt_entries) /
			sizeof(qdma_s80_hard_cmpt_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

			len += (((sizeof(qdma_s80_hard_c2h_pftch_ctxt_entries) /
			sizeof(qdma_s80_hard_c2h_pftch_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);
		}
	}

	*req_buflen = len;
	return rv;
}

static uint32_t qdma_s80_hard_intr_context_buf_len(void)
{
	uint32_t len = 0;

	len += (((sizeof(qdma_s80_hard_ind_intr_ctxt_entries) /
			sizeof(qdma_s80_hard_ind_intr_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);
	return len;
}

/*
 * qdma_acc_fill_sw_ctxt() - Helper function to fill sw context into structure
 *
 */
static void qdma_s80_hard_fill_sw_ctxt(struct qdma_descq_sw_ctxt *sw_ctxt)
{
	qdma_s80_hard_sw_ctxt_entries[0].value = sw_ctxt->pidx;
	qdma_s80_hard_sw_ctxt_entries[1].value = sw_ctxt->irq_arm;
	qdma_s80_hard_sw_ctxt_entries[2].value = sw_ctxt->qen;
	qdma_s80_hard_sw_ctxt_entries[3].value = sw_ctxt->frcd_en;
	qdma_s80_hard_sw_ctxt_entries[4].value = sw_ctxt->wbi_chk;
	qdma_s80_hard_sw_ctxt_entries[5].value = sw_ctxt->wbi_intvl_en;
	qdma_s80_hard_sw_ctxt_entries[6].value = sw_ctxt->fnc_id;
	qdma_s80_hard_sw_ctxt_entries[7].value = sw_ctxt->rngsz_idx;
	qdma_s80_hard_sw_ctxt_entries[8].value = sw_ctxt->desc_sz;
	qdma_s80_hard_sw_ctxt_entries[9].value = sw_ctxt->bypass;
	qdma_s80_hard_sw_ctxt_entries[10].value = sw_ctxt->mm_chn;
	qdma_s80_hard_sw_ctxt_entries[11].value = sw_ctxt->wbk_en;
	qdma_s80_hard_sw_ctxt_entries[12].value = sw_ctxt->irq_en;
	qdma_s80_hard_sw_ctxt_entries[13].value = sw_ctxt->port_id;
	qdma_s80_hard_sw_ctxt_entries[14].value = sw_ctxt->irq_no_last;
	qdma_s80_hard_sw_ctxt_entries[15].value = sw_ctxt->err;
	qdma_s80_hard_sw_ctxt_entries[16].value = sw_ctxt->err_wb_sent;
	qdma_s80_hard_sw_ctxt_entries[17].value = sw_ctxt->irq_req;
	qdma_s80_hard_sw_ctxt_entries[18].value = sw_ctxt->mrkr_dis;
	qdma_s80_hard_sw_ctxt_entries[19].value = sw_ctxt->is_mm;
	qdma_s80_hard_sw_ctxt_entries[20].value =
			sw_ctxt->ring_bs_addr & 0xFFFFFFFF;
	qdma_s80_hard_sw_ctxt_entries[21].value =
		(sw_ctxt->ring_bs_addr >> 32) & 0xFFFFFFFF;
}

/*
 * qdma_acc_fill_cmpt_ctxt() - Helper function to fill completion context
 *                         into structure
 *
 */
static void qdma_s80_hard_fill_cmpt_ctxt(struct qdma_descq_cmpt_ctxt *cmpt_ctxt)
{
	qdma_s80_hard_cmpt_ctxt_entries[0].value = cmpt_ctxt->en_stat_desc;
	qdma_s80_hard_cmpt_ctxt_entries[1].value = cmpt_ctxt->en_int;
	qdma_s80_hard_cmpt_ctxt_entries[2].value = cmpt_ctxt->trig_mode;
	qdma_s80_hard_cmpt_ctxt_entries[3].value = cmpt_ctxt->fnc_id;
	qdma_s80_hard_cmpt_ctxt_entries[4].value = cmpt_ctxt->counter_idx;
	qdma_s80_hard_cmpt_ctxt_entries[5].value = cmpt_ctxt->timer_idx;
	qdma_s80_hard_cmpt_ctxt_entries[6].value = cmpt_ctxt->in_st;
	qdma_s80_hard_cmpt_ctxt_entries[7].value = cmpt_ctxt->color;
	qdma_s80_hard_cmpt_ctxt_entries[8].value = cmpt_ctxt->ringsz_idx;
	qdma_s80_hard_cmpt_ctxt_entries[9].value =
			cmpt_ctxt->bs_addr & 0xFFFFFFFF;
	qdma_s80_hard_cmpt_ctxt_entries[10].value =
		(cmpt_ctxt->bs_addr >> 32) & 0xFFFFFFFF;
	qdma_s80_hard_cmpt_ctxt_entries[11].value = cmpt_ctxt->desc_sz;
	qdma_s80_hard_cmpt_ctxt_entries[12].value = cmpt_ctxt->pidx;
	qdma_s80_hard_cmpt_ctxt_entries[13].value = cmpt_ctxt->cidx;
	qdma_s80_hard_cmpt_ctxt_entries[14].value = cmpt_ctxt->valid;
	qdma_s80_hard_cmpt_ctxt_entries[15].value = cmpt_ctxt->err;
	qdma_s80_hard_cmpt_ctxt_entries[16].value = cmpt_ctxt->user_trig_pend;
	qdma_s80_hard_cmpt_ctxt_entries[17].value = cmpt_ctxt->timer_running;
	qdma_s80_hard_cmpt_ctxt_entries[18].value = cmpt_ctxt->full_upd;

}

/*
 * qdma_acc_fill_hw_ctxt() - Helper function to fill HW context into structure
 *
 */
static void qdma_s80_hard_fill_hw_ctxt(struct qdma_descq_hw_ctxt *hw_ctxt)
{
	qdma_s80_hard_hw_ctxt_entries[0].value = hw_ctxt->cidx;
	qdma_s80_hard_hw_ctxt_entries[1].value = hw_ctxt->crd_use;
	qdma_s80_hard_hw_ctxt_entries[2].value = hw_ctxt->dsc_pend;
	qdma_s80_hard_hw_ctxt_entries[3].value = hw_ctxt->idl_stp_b;
	qdma_s80_hard_hw_ctxt_entries[4].value = hw_ctxt->evt_pnd;
	qdma_s80_hard_hw_ctxt_entries[5].value = hw_ctxt->fetch_pnd;
}

/*
 * qdma_acc_fill_credit_ctxt() - Helper function to fill Credit context
 *                           into structure
 *
 */
static void qdma_s80_hard_fill_credit_ctxt(
		struct qdma_descq_credit_ctxt *cr_ctxt)
{
	qdma_s80_hard_credit_ctxt_entries[0].value = cr_ctxt->credit;
}

/*
 * qdma_acc_fill_pfetch_ctxt() - Helper function to fill Prefetch context
 *                           into structure
 *
 */
static void qdma_s80_hard_fill_pfetch_ctxt(
		struct qdma_descq_prefetch_ctxt *pfetch_ctxt)
{
	qdma_s80_hard_c2h_pftch_ctxt_entries[0].value = pfetch_ctxt->bypass;
	qdma_s80_hard_c2h_pftch_ctxt_entries[1].value = pfetch_ctxt->bufsz_idx;
	qdma_s80_hard_c2h_pftch_ctxt_entries[2].value = pfetch_ctxt->port_id;
	qdma_s80_hard_c2h_pftch_ctxt_entries[3].value = pfetch_ctxt->err;
	qdma_s80_hard_c2h_pftch_ctxt_entries[4].value = pfetch_ctxt->pfch_en;
	qdma_s80_hard_c2h_pftch_ctxt_entries[5].value = pfetch_ctxt->pfch;
	qdma_s80_hard_c2h_pftch_ctxt_entries[6].value = pfetch_ctxt->sw_crdt;
	qdma_s80_hard_c2h_pftch_ctxt_entries[7].value = pfetch_ctxt->valid;
}

static void qdma_s80_hard_fill_qid2vec_ctxt(struct qdma_qid2vec *qid2vec_ctxt)
{
	qdma_s80_hard_qid2vec_ctxt_entries[0].value = qid2vec_ctxt->c2h_vector;
	qdma_s80_hard_qid2vec_ctxt_entries[1].value = qid2vec_ctxt->c2h_en_coal;
	qdma_s80_hard_qid2vec_ctxt_entries[2].value = qid2vec_ctxt->h2c_vector;
	qdma_s80_hard_qid2vec_ctxt_entries[3].value = qid2vec_ctxt->h2c_en_coal;
}

static void qdma_s80_hard_fill_intr_ctxt(
		struct qdma_indirect_intr_ctxt *intr_ctxt)
{
	qdma_s80_hard_ind_intr_ctxt_entries[0].value = intr_ctxt->valid;
	qdma_s80_hard_ind_intr_ctxt_entries[1].value = intr_ctxt->vec;
	qdma_s80_hard_ind_intr_ctxt_entries[2].value = intr_ctxt->int_st;
	qdma_s80_hard_ind_intr_ctxt_entries[3].value = intr_ctxt->color;
	qdma_s80_hard_ind_intr_ctxt_entries[4].value =
			intr_ctxt->baddr_4k & 0xFFFFFFFF;
	qdma_s80_hard_ind_intr_ctxt_entries[5].value =
			(intr_ctxt->baddr_4k >> 32) & 0xFFFFFFFF;
	qdma_s80_hard_ind_intr_ctxt_entries[6].value = intr_ctxt->page_size;
	qdma_s80_hard_ind_intr_ctxt_entries[7].value = intr_ctxt->pidx;
}

/*
 * dump_s80_hard_context() - Helper function to dump queue context into string
 *
 * return len - length of the string copied into buffer
 */
static int dump_s80_hard_context(struct qdma_descq_context *queue_context,
		uint8_t st,	enum qdma_dev_q_type q_type,
		char *buf, int buf_sz)
{
	int i = 0;
	int n;
	int len = 0;
	int rv;
	char banner[DEBGFS_LINE_SZ];

	if (queue_context == NULL) {
		qdma_log_error("%s: queue_context is NULL, err:%d\n",
						__func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (q_type >= QDMA_DEV_Q_TYPE_CMPT) {
		qdma_log_error("%s: Invalid queue type(%d), err:%d\n",
						__func__,
						q_type,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_fill_sw_ctxt(&queue_context->sw_ctxt);
	qdma_s80_hard_fill_hw_ctxt(&queue_context->hw_ctxt);
	qdma_s80_hard_fill_credit_ctxt(&queue_context->cr_ctxt);
	qdma_s80_hard_fill_qid2vec_ctxt(&queue_context->qid2vec);
	if (st && (q_type == QDMA_DEV_Q_TYPE_C2H)) {
		qdma_s80_hard_fill_pfetch_ctxt(&queue_context->pfetch_ctxt);
		qdma_s80_hard_fill_cmpt_ctxt(&queue_context->cmpt_ctxt);
	}

	if (q_type != QDMA_DEV_Q_TYPE_CMPT) {
		for (i = 0; i < DEBGFS_LINE_SZ - 5; i++) {
			rv = QDMA_SNPRINTF_S(banner + i,
				(DEBGFS_LINE_SZ - i),
				sizeof("-"), "-");
			if ((rv < 0) || (rv > (int)sizeof("-"))) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
		}

		/* SW context dump */
		n = sizeof(qdma_s80_hard_sw_ctxt_entries) /
				sizeof((qdma_s80_hard_sw_ctxt_entries)[0]);
		for (i = 0; i < n; i++) {
			if ((len >= buf_sz) ||
				((len + DEBGFS_LINE_SZ) >= buf_sz))
				goto INSUF_BUF_EXIT;

			if (i == 0) {
				if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
					goto INSUF_BUF_EXIT;
				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%40s", "SW Context");
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s\n", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;
			}

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ,
				"%-47s %#-10x %u\n",
				qdma_s80_hard_sw_ctxt_entries[i].name,
				qdma_s80_hard_sw_ctxt_entries[i].value,
				qdma_s80_hard_sw_ctxt_entries[i].value);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}

		/* HW context dump */
		n = sizeof(qdma_s80_hard_hw_ctxt_entries) /
				sizeof((qdma_s80_hard_hw_ctxt_entries)[0]);
		for (i = 0; i < n; i++) {
			if ((len >= buf_sz) ||
				((len + DEBGFS_LINE_SZ) >= buf_sz))
				goto INSUF_BUF_EXIT;

			if (i == 0) {
				if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
					goto INSUF_BUF_EXIT;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%40s", "HW Context");
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s\n", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;
			}

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ,
				"%-47s %#-10x %u\n",
				qdma_s80_hard_hw_ctxt_entries[i].name,
				qdma_s80_hard_hw_ctxt_entries[i].value,
				qdma_s80_hard_hw_ctxt_entries[i].value);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}

		/* Credit context dump */
		n = sizeof(qdma_s80_hard_credit_ctxt_entries) /
			sizeof((qdma_s80_hard_credit_ctxt_entries)[0]);
		for (i = 0; i < n; i++) {
			if ((len >= buf_sz) ||
				((len + DEBGFS_LINE_SZ) >= buf_sz))
				goto INSUF_BUF_EXIT;

			if (i == 0) {
				if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
					goto INSUF_BUF_EXIT;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%40s",
					"Credit Context");
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s\n", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;
			}

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ,
				"%-47s %#-10x %u\n",
				qdma_s80_hard_credit_ctxt_entries[i].name,
				qdma_s80_hard_credit_ctxt_entries[i].value,
				qdma_s80_hard_credit_ctxt_entries[i].value);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}
	}

	/* SW context dump */
	n = sizeof(qdma_s80_hard_qid2vec_ctxt_entries) /
			sizeof((qdma_s80_hard_qid2vec_ctxt_entries)[0]);
	for (i = 0; i < n; i++) {
		if ((len >= buf_sz) ||
			((len + DEBGFS_LINE_SZ) >= buf_sz))
			goto INSUF_BUF_EXIT;

		if (i == 0) {
			if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
				goto INSUF_BUF_EXIT;
			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%s", banner);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%40s",
				"QID2VEC Context");
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%s\n", banner);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}

		rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len), DEBGFS_LINE_SZ,
			"%-47s %#-10x %u\n",
			qdma_s80_hard_qid2vec_ctxt_entries[i].name,
			qdma_s80_hard_qid2vec_ctxt_entries[i].value,
			qdma_s80_hard_qid2vec_ctxt_entries[i].value);
		if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
			qdma_log_error(
				"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
				__LINE__, __func__,
				rv);
			goto INSUF_BUF_EXIT;
		}
		len += rv;
	}


	if ((q_type == QDMA_DEV_Q_TYPE_CMPT) ||
			(st && q_type == QDMA_DEV_Q_TYPE_C2H)) {
		/* Completion context dump */
		n = sizeof(qdma_s80_hard_cmpt_ctxt_entries) /
				sizeof((qdma_s80_hard_cmpt_ctxt_entries)[0]);
		for (i = 0; i < n; i++) {
			if ((len >= buf_sz) ||
				((len + DEBGFS_LINE_SZ) >= buf_sz))
				goto INSUF_BUF_EXIT;

			if (i == 0) {
				if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
					goto INSUF_BUF_EXIT;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%40s",
					"Completion Context");
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s\n", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;
			}

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ,
				"%-47s %#-10x %u\n",
				qdma_s80_hard_cmpt_ctxt_entries[i].name,
				qdma_s80_hard_cmpt_ctxt_entries[i].value,
				qdma_s80_hard_cmpt_ctxt_entries[i].value);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}
	}

	if (st && q_type == QDMA_DEV_Q_TYPE_C2H) {
		/* Prefetch context dump */
		n = sizeof(qdma_s80_hard_c2h_pftch_ctxt_entries) /
			sizeof(qdma_s80_hard_c2h_pftch_ctxt_entries[0]);
		for (i = 0; i < n; i++) {
			if ((len >= buf_sz) ||
				((len + DEBGFS_LINE_SZ) >= buf_sz))
				goto INSUF_BUF_EXIT;

			if (i == 0) {
				if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
					goto INSUF_BUF_EXIT;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%40s",
					"Prefetch Context");
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;

				rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
					DEBGFS_LINE_SZ, "\n%s\n", banner);
				if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
					qdma_log_error(
						"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
						__LINE__, __func__,
						rv);
					goto INSUF_BUF_EXIT;
				}
				len += rv;
			}

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ,
				"%-47s %#-10x %u\n",
				qdma_s80_hard_c2h_pftch_ctxt_entries[i].name,
				qdma_s80_hard_c2h_pftch_ctxt_entries[i].value,
				qdma_s80_hard_c2h_pftch_ctxt_entries[i].value);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}
	}

	return len;

INSUF_BUF_EXIT:
	if (buf_sz > DEBGFS_LINE_SZ) {
		rv = QDMA_SNPRINTF_S((buf + buf_sz - DEBGFS_LINE_SZ),
			buf_sz, DEBGFS_LINE_SZ,
			"\n\nInsufficient buffer size, partial context dump\n");
		if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
			qdma_log_error(
				"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
				__LINE__, __func__,
				rv);
		}
	}

	qdma_log_error("%s: Insufficient buffer size, err:%d\n",
		__func__, -QDMA_ERR_NO_MEM);

	return -QDMA_ERR_NO_MEM;
}

static int dump_s80_hard_intr_context(struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, int buf_sz)
{
	int i = 0;
	int n;
	int len = 0;
	int rv;
	char banner[DEBGFS_LINE_SZ];

	qdma_s80_hard_fill_intr_ctxt(intr_ctx);

	for (i = 0; i < DEBGFS_LINE_SZ - 5; i++) {
		rv = QDMA_SNPRINTF_S(banner + i,
			(DEBGFS_LINE_SZ - i),
			sizeof("-"), "-");
		if ((rv < 0) || (rv > (int)sizeof("-"))) {
			qdma_log_error(
				"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
				__LINE__, __func__,
				rv);
			goto INSUF_BUF_EXIT;
		}
	}

	/* Interrupt context dump */
	n = sizeof(qdma_s80_hard_ind_intr_ctxt_entries) /
			sizeof((qdma_s80_hard_ind_intr_ctxt_entries)[0]);
	for (i = 0; i < n; i++) {
		if ((len >= buf_sz) || ((len + DEBGFS_LINE_SZ) >= buf_sz))
			goto INSUF_BUF_EXIT;

		if (i == 0) {
			if ((len + (3 * DEBGFS_LINE_SZ)) >= buf_sz)
				goto INSUF_BUF_EXIT;

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%s", banner);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%50s %d",
				"Interrupt Context for ring#", ring_index);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;

			rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len),
				DEBGFS_LINE_SZ, "\n%s\n", banner);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				goto INSUF_BUF_EXIT;
			}
			len += rv;
		}

		rv = QDMA_SNPRINTF_S(buf + len, (buf_sz - len), DEBGFS_LINE_SZ,
			"%-47s %#-10x %u\n",
			qdma_s80_hard_ind_intr_ctxt_entries[i].name,
			qdma_s80_hard_ind_intr_ctxt_entries[i].value,
			qdma_s80_hard_ind_intr_ctxt_entries[i].value);
		if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
			qdma_log_error(
				"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
				__LINE__, __func__,
				rv);
			goto INSUF_BUF_EXIT;
		}
		len += rv;
	}

	return len;

INSUF_BUF_EXIT:
	if (buf_sz > DEBGFS_LINE_SZ) {
		rv = QDMA_SNPRINTF_S((buf + buf_sz - DEBGFS_LINE_SZ),
			buf_sz, DEBGFS_LINE_SZ,
			"\n\nInsufficient buffer size, partial intr context dump\n");
		if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
			qdma_log_error(
				"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
				__LINE__, __func__,
				rv);
		}
	}

	qdma_log_error("%s: Insufficient buffer size, err:%d\n",
		__func__, -QDMA_ERR_NO_MEM);

	return -QDMA_ERR_NO_MEM;
}

/*
 * qdma_s80_hard_indirect_reg_invalidate() - helper function to invalidate
 * indirect context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_s80_hard_indirect_reg_invalidate(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid)
{
	union qdma_s80_hard_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_INV;
	cmd.bits.sel = sel;
	qdma_reg_write(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR,
			IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*
 * qdma_s80_hard_indirect_reg_clear() - helper function to clear indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_s80_hard_indirect_reg_clear(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel, uint16_t hw_qid)
{
	union qdma_s80_hard_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_CLR;
	cmd.bits.sel = sel;

	qdma_reg_write(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR,
			IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*
 * qdma_s80_hard_indirect_reg_read() - helper function to read indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_s80_hard_indirect_reg_read(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t cnt, uint32_t *data)
{
	uint32_t index = 0, reg_addr = QDMA_S80_HARD_IND_CTXT_DATA_3_ADDR;
	union qdma_s80_hard_ind_ctxt_cmd cmd;

	qdma_reg_access_lock(dev_hndl);

	/* set command register */
	cmd.word = 0;
	cmd.bits.qid = hw_qid;
	cmd.bits.op = QDMA_CTXT_CMD_RD;
	cmd.bits.sel = sel;
	qdma_reg_write(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR, cmd.word);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR,
			IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed, err:%d\n",
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
 * qdma_s80_hard_indirect_reg_write() - helper function to write indirect
 *				context registers.
 *
 * return -QDMA_ERR_HWACC_BUSY_TIMEOUT if register
 *	value didn't match, QDMA_SUCCESS other wise
 */
static int qdma_s80_hard_indirect_reg_write(void *dev_hndl,
		enum ind_ctxt_cmd_sel sel,
		uint16_t hw_qid, uint32_t *data, uint16_t cnt)
{
	uint32_t index, reg_addr;
	struct qdma_s80_hard_indirect_ctxt_regs regs;
	uint32_t *wr_data = (uint32_t *)&regs;

	qdma_reg_access_lock(dev_hndl);

	/* write the context data */
	for (index = 0; index < QDMA_S80_HARD_IND_CTXT_DATA_NUM_REGS;
			index++) {
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
	reg_addr = QDMA_S80_HARD_IND_CTXT_DATA_3_ADDR;

	for (index = 0;
		index < ((2 * QDMA_S80_HARD_IND_CTXT_DATA_NUM_REGS) + 1);
			index++, reg_addr += sizeof(uint32_t))
		qdma_reg_write(dev_hndl, reg_addr, wr_data[index]);

	/* check if the operation went through well */
	if (hw_monitor_reg(dev_hndl, QDMA_S80_HARD_IND_CTXT_CMD_ADDR,
			IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		qdma_log_error("%s: hw_monitor_reg failed, err:%d\n",
						__func__,
					   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
		return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_qid2vec_write() - create qid2vec context and program it
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_qid2vec_write(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid, struct qdma_qid2vec *ctxt)
{
	uint32_t qid2vec = 0;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;
	int rv = 0;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p qid2vec=%p, err:%d\n",
				__func__, dev_hndl, ctxt, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			1, &qid2vec);
	if (rv < 0)
		return rv;
	if (c2h) {
		qid2vec = qid2vec & (QDMA_S80_HARD_QID2VEC_H2C_VECTOR |
					QDMA_S80_HARD_QID2VEC_H2C_COAL_EN);
		qid2vec |= FIELD_SET(C2H_QID2VEC_MAP_C2H_VECTOR_MASK,
				     ctxt->c2h_vector) |
			FIELD_SET(C2H_QID2VEC_MAP_C2H_EN_COAL_MASK,
				  ctxt->c2h_en_coal);
	} else {
		qid2vec = qid2vec & (C2H_QID2VEC_MAP_C2H_VECTOR_MASK |
					C2H_QID2VEC_MAP_C2H_EN_COAL_MASK);
		qid2vec |=
			FIELD_SET(QDMA_S80_HARD_QID2VEC_H2C_VECTOR,
				  ctxt->h2c_vector) |
			FIELD_SET(QDMA_S80_HARD_QID2VEC_H2C_COAL_EN,
				  ctxt->h2c_en_coal);
	}

	return qdma_s80_hard_indirect_reg_write(dev_hndl, sel, hw_qid,
			&qid2vec, QDMA_S80_HARD_QID2VEC_CONTEXT_NUM_WORDS);

}

/*****************************************************************************/
/**
 * qdma_s80_hard_qid2vec_read() - read qid2vec context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_qid2vec_read(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid, struct qdma_qid2vec *ctxt)
{
	int rv = 0;
	uint32_t qid2vec[QDMA_S80_HARD_QID2VEC_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p qid2vec=%p, err:%d\n",
				__func__, dev_hndl, ctxt, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_S80_HARD_QID2VEC_CONTEXT_NUM_WORDS, qid2vec);
	if (rv < 0)
		return rv;

	if (c2h) {
		ctxt->c2h_vector = FIELD_GET(C2H_QID2VEC_MAP_C2H_VECTOR_MASK,
						qid2vec[0]);
		ctxt->c2h_en_coal =
			(uint8_t)(FIELD_GET(C2H_QID2VEC_MAP_C2H_EN_COAL_MASK,
						qid2vec[0]));
	} else {
		ctxt->h2c_vector =
			(uint8_t)(FIELD_GET(QDMA_S80_HARD_QID2VEC_H2C_VECTOR,
								qid2vec[0]));
		ctxt->h2c_en_coal =
			(uint8_t)(FIELD_GET(QDMA_S80_HARD_QID2VEC_H2C_COAL_EN,
								qid2vec[0]));
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_qid2vec_clear() - clear qid2vec context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_qid2vec_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_qid2vec_invalidate() - invalidate qid2vec context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_qid2vec_invalidate(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_qid2vec_conf() - configure qid2vector context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_qid2vec_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			 struct qdma_qid2vec *ctxt,
			 enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_qid2vec_read(dev_hndl, c2h,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_qid2vec_write(dev_hndl, c2h,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_qid2vec_clear(dev_hndl, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_qid2vec_invalidate(dev_hndl, hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}

	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_fmap_write() - create fmap context and program it
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 * @config:	pointer to the fmap data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_fmap_write(void *dev_hndl, uint16_t func_id,
		   const struct qdma_fmap_cfg *config)
{
	uint32_t fmap = 0;

	if (!dev_hndl || !config) {
		qdma_log_error("%s: dev_handle or config is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	fmap = FIELD_SET(TRQ_SEL_FMAP_0_QID_BASE_MASK, config->qbase) |
		FIELD_SET(TRQ_SEL_FMAP_0_QID_MAX_MASK,
				config->qmax);

	qdma_reg_write(dev_hndl, QDMA_S80_HARD_TRQ_SEL_FMAP_0_ADDR +
			func_id * QDMA_S80_HARD_REG_TRQ_SEL_FMAP_STEP,
			fmap);
	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_fmap_read() - read fmap context
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 * @config:	pointer to the output fmap data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_fmap_read(void *dev_hndl, uint16_t func_id,
			 struct qdma_fmap_cfg *config)
{
	uint32_t fmap = 0;

	if (!dev_hndl || !config) {
		qdma_log_error("%s: dev_handle=%p fmap=%p NULL, err:%d\n",
						__func__, dev_hndl, config,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	fmap = qdma_reg_read(dev_hndl, QDMA_S80_HARD_TRQ_SEL_FMAP_0_ADDR +
			     func_id * QDMA_S80_HARD_REG_TRQ_SEL_FMAP_STEP);

	config->qbase = FIELD_GET(TRQ_SEL_FMAP_0_QID_BASE_MASK, fmap);
	config->qmax =
		(uint16_t)(FIELD_GET(TRQ_SEL_FMAP_0_QID_MAX_MASK,
				fmap));

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_fmap_clear() - clear fmap context
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_fmap_clear(void *dev_hndl, uint16_t func_id)
{
	uint32_t fmap = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_reg_write(dev_hndl, QDMA_S80_HARD_TRQ_SEL_FMAP_0_ADDR +
			func_id * QDMA_S80_HARD_REG_TRQ_SEL_FMAP_STEP,
			fmap);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_fmap_conf() - configure fmap context
 *
 * @dev_hndl:	device handle
 * @func_id:	function id of the device
 * @config:	pointer to the fmap data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_INVALIDATE unsupported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_fmap_conf(void *dev_hndl, uint16_t func_id,
				struct qdma_fmap_cfg *config,
				enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_fmap_read(dev_hndl, func_id, config);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_fmap_write(dev_hndl, func_id, config);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_fmap_clear(dev_hndl, func_id);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}

	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_sw_context_write() - create sw context and program it
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the SW context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_sw_context_write(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 const struct qdma_descq_sw_ctxt *ctxt)
{
	uint32_t sw_ctxt[QDMA_S80_HARD_SW_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	/* Input args check */
	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl or ctxt is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((ctxt->desc_sz > QDMA_DESC_SIZE_64B) ||
		(ctxt->rngsz_idx >= QDMA_NUM_RING_SIZES)) {
		qdma_log_error("%s: Invalid desc_sz(%d)/rngidx(%d), err:%d\n",
					__func__,
					ctxt->desc_sz,
					ctxt->rngsz_idx,
					-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	sw_ctxt[num_words_count++] =
		FIELD_SET(SW_IND_CTXT_DATA_W0_PIDX_MASK, ctxt->pidx) |
		FIELD_SET(SW_IND_CTXT_DATA_W0_IRQ_ARM_MASK, ctxt->irq_arm);

	sw_ctxt[num_words_count++] =
		FIELD_SET(SW_IND_CTXT_DATA_W1_QEN_MASK, ctxt->qen) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_FCRD_EN_MASK, ctxt->frcd_en) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_WBI_CHK_MASK, ctxt->wbi_chk) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_WBI_INTVL_EN_MASK,
			ctxt->wbi_intvl_en) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_FNC_ID_MASK, ctxt->fnc_id) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_RNG_SZ_MASK, ctxt->rngsz_idx) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_DSC_SZ_MASK, ctxt->desc_sz) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_MM_CHN_MASK, ctxt->mm_chn) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_WBK_EN_MASK, ctxt->wbk_en) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_IRQ_EN_MASK, ctxt->irq_en) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_IRQ_NO_LAST_MASK,
			ctxt->irq_no_last) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_ERR_MASK, ctxt->err) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_ERR_WB_SENT_MASK,
			ctxt->err_wb_sent) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_IRQ_REQ_MASK, ctxt->irq_req) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_MRKR_DIS_MASK, ctxt->mrkr_dis) |
		FIELD_SET(SW_IND_CTXT_DATA_W1_IS_MM_MASK, ctxt->is_mm);

	sw_ctxt[num_words_count++] = ctxt->ring_bs_addr & 0xffffffff;
	sw_ctxt[num_words_count++] = (ctxt->ring_bs_addr >> 32) & 0xffffffff;

	return qdma_s80_hard_indirect_reg_write(dev_hndl, sel, hw_qid,
			sw_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_sw_context_read() - read sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_sw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_sw_ctxt *ctxt)
{
	int rv = 0;
	uint32_t sw_ctxt[QDMA_S80_HARD_SW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;
	struct qdma_qid2vec qid2vec_ctxt = {0};

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p sw_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_S80_HARD_SW_CONTEXT_NUM_WORDS, sw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->pidx = FIELD_GET(SW_IND_CTXT_DATA_W0_PIDX_MASK, sw_ctxt[0]);
	ctxt->irq_arm =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W0_IRQ_ARM_MASK,
			sw_ctxt[0]));

	ctxt->qen = FIELD_GET(SW_IND_CTXT_DATA_W1_QEN_MASK, sw_ctxt[1]);
	ctxt->frcd_en = FIELD_GET(SW_IND_CTXT_DATA_W1_FCRD_EN_MASK,
			sw_ctxt[1]);
	ctxt->wbi_chk = FIELD_GET(SW_IND_CTXT_DATA_W1_WBI_CHK_MASK,
			sw_ctxt[1]);
	ctxt->wbi_intvl_en =
		FIELD_GET(SW_IND_CTXT_DATA_W1_WBI_INTVL_EN_MASK,
			sw_ctxt[1]);
	ctxt->fnc_id =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_FNC_ID_MASK,
			sw_ctxt[1]));
	ctxt->rngsz_idx =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_RNG_SZ_MASK,
		sw_ctxt[1]));
	ctxt->desc_sz =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_DSC_SZ_MASK,
			sw_ctxt[1]));
	ctxt->bypass =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_BYPASS_MASK,
			sw_ctxt[1]));
	ctxt->mm_chn =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_MM_CHN_MASK,
			sw_ctxt[1]));
	ctxt->wbk_en =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_WBK_EN_MASK,
			sw_ctxt[1]));
	ctxt->irq_en =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_IRQ_EN_MASK,
			sw_ctxt[1]));
	ctxt->port_id =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_PORT_ID_MASK,
			sw_ctxt[1]));
	ctxt->irq_no_last =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_IRQ_NO_LAST_MASK,
			sw_ctxt[1]));
	ctxt->err =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_ERR_MASK, sw_ctxt[1]));
	ctxt->err_wb_sent =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_ERR_WB_SENT_MASK,
			sw_ctxt[1]));
	ctxt->irq_req =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_IRQ_REQ_MASK,
			sw_ctxt[1]));
	ctxt->mrkr_dis =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_MRKR_DIS_MASK,
			sw_ctxt[1]));
	ctxt->is_mm =
		(uint8_t)(FIELD_GET(SW_IND_CTXT_DATA_W1_IS_MM_MASK,
			sw_ctxt[1]));

	ctxt->ring_bs_addr = ((uint64_t)sw_ctxt[3] << 32) | (sw_ctxt[2]);

	/** Read the QID2VEC Context Data */
	rv = qdma_s80_hard_qid2vec_read(dev_hndl, c2h, hw_qid, &qid2vec_ctxt);
	if (rv < 0)
		return rv;

	if (c2h) {
		ctxt->vec = qid2vec_ctxt.c2h_vector;
		ctxt->intr_aggr = qid2vec_ctxt.c2h_en_coal;
	} else {
		ctxt->vec = qid2vec_ctxt.h2c_vector;
		ctxt->intr_aggr = qid2vec_ctxt.h2c_en_coal;
	}


	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_sw_context_clear() - clear sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_sw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_sw_context_invalidate() - invalidate sw context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_sw_context_invalidate(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_sw_ctx_conf() - configure SW context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_sw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_sw_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_sw_context_read(dev_hndl, c2h, hw_qid,
				ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_sw_context_write(dev_hndl, c2h, hw_qid,
				ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_sw_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_sw_context_invalidate(dev_hndl,
				c2h, hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}
	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_pfetch_context_write() - create prefetch context and program it
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the prefetch context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_pfetch_context_write(void *dev_hndl, uint16_t hw_qid,
		const struct qdma_descq_prefetch_ctxt *ctxt)
{
	uint32_t pfetch_ctxt[QDMA_S80_HARD_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;
	uint16_t num_words_count = 0;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p pfetch_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	sw_crdt_l =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK, ctxt->sw_crdt);
	sw_crdt_h =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, ctxt->sw_crdt);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(PREFETCH_CTXT_DATA_W0_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_BUF_SIZE_IDX_MASK,
				ctxt->bufsz_idx) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_ERR_MASK, ctxt->err) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_PFCH_EN_MASK, ctxt->pfch_en) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_PFCH_MASK, ctxt->pfch) |
		FIELD_SET(PREFETCH_CTXT_DATA_W0_SW_CRDT_L_MASK, sw_crdt_l);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(PREFETCH_CTXT_DATA_W1_SW_CRDT_H_MASK, sw_crdt_h) |
		FIELD_SET(PREFETCH_CTXT_DATA_W1_VALID_MASK, ctxt->valid);

	return qdma_s80_hard_indirect_reg_write(dev_hndl, sel, hw_qid,
			pfetch_ctxt, num_words_count);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_pfetch_context_read() - read prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_pfetch_context_read(void *dev_hndl, uint16_t hw_qid,
		struct qdma_descq_prefetch_ctxt *ctxt)
{
	int rv = 0;
	uint32_t pfetch_ctxt[QDMA_S80_HARD_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p pfetch_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_S80_HARD_PFETCH_CONTEXT_NUM_WORDS, pfetch_ctxt);
	if (rv < 0)
		return rv;

	ctxt->bypass =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_BYPASS_MASK,
			pfetch_ctxt[0]));
	ctxt->bufsz_idx =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_BUF_SIZE_IDX_MASK,
				pfetch_ctxt[0]));
	ctxt->port_id =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_PORT_ID_MASK,
			pfetch_ctxt[0]));
	ctxt->err =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_ERR_MASK,
			pfetch_ctxt[0]));
	ctxt->pfch_en =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_PFCH_EN_MASK,
			pfetch_ctxt[0]));
	ctxt->pfch =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W0_PFCH_MASK,
			pfetch_ctxt[0]));
	sw_crdt_l =
		(uint32_t)FIELD_GET(PREFETCH_CTXT_DATA_W0_SW_CRDT_L_MASK,
			pfetch_ctxt[0]);

	sw_crdt_h =
		(uint32_t)FIELD_GET(PREFETCH_CTXT_DATA_W1_SW_CRDT_H_MASK,
			pfetch_ctxt[1]);
	ctxt->valid =
		(uint8_t)(FIELD_GET(PREFETCH_CTXT_DATA_W1_VALID_MASK,
			pfetch_ctxt[1]));

	ctxt->sw_crdt =
		(uint16_t)(FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK,
			sw_crdt_l) |
		FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, sw_crdt_h));

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_pfetch_context_clear() - clear prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_pfetch_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_pfetch_context_invalidate() - invalidate prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_pfetch_context_invalidate(void *dev_hndl,
		uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_pfetch_ctx_conf() - configure prefetch context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_pfetch_ctx_conf(void *dev_hndl, uint16_t hw_qid,
				struct qdma_descq_prefetch_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_pfetch_context_read(dev_hndl,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_pfetch_context_write(dev_hndl,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_pfetch_context_clear(dev_hndl, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_pfetch_context_invalidate(dev_hndl,
				hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}

	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_cmpt_context_write() - create completion context and program it
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the cmpt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_cmpt_context_write(void *dev_hndl, uint16_t hw_qid,
			   const struct qdma_descq_cmpt_ctxt *ctxt)
{
	uint32_t cmpt_ctxt[QDMA_S80_HARD_CMPT_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	uint32_t baddr_l, baddr_h, baddr_m, pidx_l, pidx_h;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	/* Input args check */
	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p cmpt_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if ((ctxt->desc_sz > QDMA_DESC_SIZE_32B) ||
		(ctxt->ringsz_idx >= QDMA_NUM_RING_SIZES) ||
		(ctxt->counter_idx >= QDMA_NUM_C2H_COUNTERS) ||
		(ctxt->timer_idx >= QDMA_NUM_C2H_TIMERS) ||
		(ctxt->trig_mode > QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR)) {
		qdma_log_error
		("%s Inv dsz(%d)/ridx(%d)/cntr(%d)/tmr(%d)/tm(%d), err:%d\n",
				__func__,
				ctxt->desc_sz,
				ctxt->ringsz_idx,
				ctxt->counter_idx,
				ctxt->timer_idx,
				ctxt->trig_mode,
				-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	baddr_l =
		(uint32_t)FIELD_GET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_L_MASK,
			ctxt->bs_addr);
	baddr_m =
		(uint32_t)FIELD_GET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_M_MASK,
			ctxt->bs_addr);
	baddr_h =
		(uint32_t)FIELD_GET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_H_MASK,
			ctxt->bs_addr);

	pidx_l = FIELD_GET(QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_L_MASK,
			ctxt->pidx);
	pidx_h = FIELD_GET(QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_H_MASK,
			ctxt->pidx);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(CMPL_CTXT_DATA_W0_EN_STAT_DESC_MASK,
				ctxt->en_stat_desc) |
		FIELD_SET(CMPL_CTXT_DATA_W0_EN_INT_MASK, ctxt->en_int) |
		FIELD_SET(CMPL_CTXT_DATA_W0_TRIG_MODE_MASK, ctxt->trig_mode) |
		FIELD_SET(CMPL_CTXT_DATA_W0_FNC_ID_MASK, ctxt->fnc_id) |
		FIELD_SET(CMPL_CTXT_DATA_W0_CNTER_IDX_MASK,
				ctxt->counter_idx) |
		FIELD_SET(CMPL_CTXT_DATA_W0_TIMER_IDX_MASK,
				ctxt->timer_idx) |
		FIELD_SET(CMPL_CTXT_DATA_W0_INT_ST_MASK,
				ctxt->in_st) |
		FIELD_SET(CMPL_CTXT_DATA_W0_COLOR_MASK,
				ctxt->color) |
		FIELD_SET(CMPL_CTXT_DATA_W0_QSIZE_IDX_MASK,
				ctxt->ringsz_idx) |
		FIELD_SET(CMPL_CTXT_DATA_W0_BADDR_64_L_MASK,
				baddr_l);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(CMPL_CTXT_DATA_W1_BADDR_64_M_MASK,
				baddr_m);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(CMPL_CTXT_DATA_W2_BADDR_64_H_MASK,
				baddr_h) |
		FIELD_SET(CMPL_CTXT_DATA_W2_DESC_SIZE_MASK,
				ctxt->desc_sz) |
		FIELD_SET(CMPL_CTXT_DATA_W2_PIDX_L_MASK,
				pidx_l);

	cmpt_ctxt[num_words_count++] =
		FIELD_SET(CMPL_CTXT_DATA_W3_PIDX_H_MASK,
				pidx_h) |
		FIELD_SET(CMPL_CTXT_DATA_W3_CIDX_MASK, ctxt->cidx) |
		FIELD_SET(CMPL_CTXT_DATA_W3_VALID_MASK, ctxt->valid) |
		FIELD_SET(CMPL_CTXT_DATA_W3_ERR_MASK, ctxt->err) |
		FIELD_SET(CMPL_CTXT_DATA_W3_USER_TRIG_PEND_MASK,
				ctxt->user_trig_pend) |
		FIELD_SET(CMPL_CTXT_DATA_W3_TIMER_RUNNING_MASK,
				ctxt->timer_running) |
		FIELD_SET(CMPL_CTXT_DATA_W3_FULL_UPD_MASK,
				ctxt->full_upd);

	return qdma_s80_hard_indirect_reg_write(dev_hndl, sel, hw_qid,
			cmpt_ctxt, num_words_count);

}

/*****************************************************************************/
/**
 * qdma_s80_hard_cmpt_context_read() - read completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	    pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_cmpt_context_read(void *dev_hndl, uint16_t hw_qid,
			   struct qdma_descq_cmpt_ctxt *ctxt)
{
	int rv = 0;
	uint32_t cmpt_ctxt[QDMA_S80_HARD_CMPT_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;
	uint32_t baddr_l, baddr_h, baddr_m,
			 pidx_l, pidx_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p cmpt_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_S80_HARD_CMPT_CONTEXT_NUM_WORDS, cmpt_ctxt);
	if (rv < 0)
		return rv;

	ctxt->en_stat_desc =
		FIELD_GET(CMPL_CTXT_DATA_W0_EN_STAT_DESC_MASK, cmpt_ctxt[0]);
	ctxt->en_int = FIELD_GET(CMPL_CTXT_DATA_W0_EN_INT_MASK,
		cmpt_ctxt[0]);
	ctxt->trig_mode =
		FIELD_GET(CMPL_CTXT_DATA_W0_TRIG_MODE_MASK, cmpt_ctxt[0]);
	ctxt->fnc_id =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W0_FNC_ID_MASK,
			cmpt_ctxt[0]));
	ctxt->counter_idx =
		(uint8_t)(FIELD_GET(
			CMPL_CTXT_DATA_W0_CNTER_IDX_MASK,
			cmpt_ctxt[0]));
	ctxt->timer_idx =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W0_TIMER_IDX_MASK,
				cmpt_ctxt[0]));
	ctxt->in_st =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W0_INT_ST_MASK,
			cmpt_ctxt[0]));
	ctxt->color =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W0_COLOR_MASK,
			cmpt_ctxt[0]));
	ctxt->ringsz_idx =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W0_QSIZE_IDX_MASK,
			cmpt_ctxt[0]));

	baddr_l =
		FIELD_GET(CMPL_CTXT_DATA_W0_BADDR_64_L_MASK,
				cmpt_ctxt[0]);
	baddr_m =
		FIELD_GET(CMPL_CTXT_DATA_W1_BADDR_64_M_MASK,
				cmpt_ctxt[1]);
	baddr_h =
		FIELD_GET(CMPL_CTXT_DATA_W2_BADDR_64_H_MASK,
				cmpt_ctxt[2]);

	ctxt->desc_sz =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W2_DESC_SIZE_MASK,
			cmpt_ctxt[2]));
	pidx_l = FIELD_GET(CMPL_CTXT_DATA_W2_PIDX_L_MASK,
			cmpt_ctxt[2]);

	pidx_h = FIELD_GET(CMPL_CTXT_DATA_W3_PIDX_H_MASK,
			cmpt_ctxt[3]);
	ctxt->cidx =
		(uint16_t)(FIELD_GET(CMPL_CTXT_DATA_W3_CIDX_MASK,
			cmpt_ctxt[3]));
	ctxt->valid =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W3_VALID_MASK,
			cmpt_ctxt[3]));
	ctxt->err =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W3_ERR_MASK,
			cmpt_ctxt[3]));
	ctxt->user_trig_pend =
		(uint8_t)(FIELD_GET(
		CMPL_CTXT_DATA_W3_USER_TRIG_PEND_MASK, cmpt_ctxt[3]));

	ctxt->timer_running =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W3_TIMER_RUNNING_MASK,
			cmpt_ctxt[3]));
	ctxt->full_upd =
		(uint8_t)(FIELD_GET(CMPL_CTXT_DATA_W3_FULL_UPD_MASK,
			cmpt_ctxt[3]));

	ctxt->bs_addr =
		FIELD_SET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_L_MASK,
			(uint64_t)baddr_l) |
		FIELD_SET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_M_MASK,
			(uint64_t)baddr_m) |
		FIELD_SET(QDMA_S80_HARD_COMPL_CTXT_BADDR_GET_H_MASK,
			(uint64_t)baddr_h);

	ctxt->pidx =
		(uint16_t)(FIELD_SET(QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_L_MASK,
			pidx_l) |
		FIELD_SET(QDMA_S80_HARD_COMPL_CTXT_PIDX_GET_H_MASK,
			pidx_h));

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_cmpt_context_clear() - clear completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_cmpt_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_cmpt_context_invalidate() - invalidate completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_cmpt_context_invalidate(void *dev_hndl,
		uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_cmpt_ctx_conf() - configure completion context
 *
 * @dev_hndl:	device handle
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_cmpt_ctx_conf(void *dev_hndl, uint16_t hw_qid,
			struct qdma_descq_cmpt_ctxt *ctxt,
			enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_cmpt_context_read(dev_hndl,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_cmpt_context_write(dev_hndl,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_cmpt_context_clear(dev_hndl, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_cmpt_context_invalidate(dev_hndl,
				hw_qid);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}

	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_context_read() - read hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_hw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid, struct qdma_descq_hw_ctxt *ctxt)
{
	int rv = 0;
	uint32_t hw_ctxt[QDMA_S80_HARD_HW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p hw_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
				   QDMA_S80_HARD_HW_CONTEXT_NUM_WORDS, hw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->cidx = FIELD_GET(HW_IND_CTXT_DATA_W0_CIDX_MASK, hw_ctxt[0]);
	ctxt->crd_use =
		(uint16_t)(FIELD_GET(HW_IND_CTXT_DATA_W0_CRD_USE_MASK,
				hw_ctxt[0]));

	ctxt->dsc_pend =
		(uint8_t)(FIELD_GET(HW_IND_CTXT_DATA_W1_DSC_PND_MASK,
				hw_ctxt[1]));
	ctxt->idl_stp_b =
		(uint8_t)(FIELD_GET(HW_IND_CTXT_DATA_W1_IDL_STP_B_MASK,
			hw_ctxt[1]));
	ctxt->fetch_pnd =
		(uint8_t)(FIELD_GET(HW_IND_CTXT_DATA_W1_FETCH_PND_MASK,
			hw_ctxt[1]));

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_context_clear() - clear hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_hw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_context_invalidate() - invalidate hardware context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_hw_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_ctx_conf() - configure HW context
 *
 * @dev_hndl:	device handle
 * @c2h:	is c2h queue
 * @hw_qid:	hardware qid of the queue
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_WRITE unsupported
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_hw_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				struct qdma_descq_hw_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_hw_context_read(dev_hndl, c2h, hw_qid,
				ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_hw_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_hw_context_invalidate(dev_hndl, c2h,
				hw_qid);
		break;
	case QDMA_HW_ACCESS_WRITE:
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
						__func__, access_type,
						-QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}


	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_indirect_intr_context_write() - create indirect
 * interrupt context and program it
 *
 * @dev_hndl:   device handle
 * @ring_index: indirect interrupt ring index
 * @ctxt:	pointer to the interrupt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_indirect_intr_context_write(void *dev_hndl,
		uint16_t ring_index, const struct qdma_indirect_intr_ctxt *ctxt)
{
	uint32_t intr_ctxt[QDMA_S80_HARD_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint16_t num_words_count = 0;
	uint32_t baddr_l, baddr_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p intr_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (ctxt->page_size > QDMA_INDIRECT_INTR_RING_SIZE_32KB) {
		qdma_log_error("%s: ctxt->page_size=%u is too big, err:%d\n",
					   __func__, ctxt->page_size,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	baddr_l =
		(uint32_t)FIELD_GET(QDMA_S80_HARD_INTR_CTXT_BADDR_GET_L_MASK,
			ctxt->baddr_4k);
	baddr_h =
		(uint32_t)FIELD_GET(QDMA_S80_HARD_INTR_CTXT_BADDR_GET_H_MASK,
			ctxt->baddr_4k);

	intr_ctxt[num_words_count++] =
		FIELD_SET(INTR_CTXT_DATA_W0_VALID_MASK, ctxt->valid) |
		FIELD_SET(INTR_CTXT_DATA_W0_VEC_MASK, ctxt->vec) |
		FIELD_SET(INTR_CTXT_DATA_W0_INT_ST_MASK,
				ctxt->int_st) |
		FIELD_SET(INTR_CTXT_DATA_W0_COLOR_MASK, ctxt->color) |
		FIELD_SET(INTR_CTXT_DATA_W0_BADDR_4K_L_MASK, baddr_l);

	intr_ctxt[num_words_count++] =
		FIELD_SET(INTR_CTXT_DATA_W1_BADDR_4K_H_MASK, baddr_h) |
		FIELD_SET(INTR_CTXT_DATA_W1_PAGE_SIZE_MASK,
				ctxt->page_size);

	intr_ctxt[num_words_count++] =
		FIELD_SET(INTR_CTXT_DATA_W2_PIDX_MASK, ctxt->pidx);

	return qdma_s80_hard_indirect_reg_write(dev_hndl, sel, ring_index,
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
static int qdma_s80_hard_indirect_intr_context_read(void *dev_hndl,
		uint16_t ring_index, struct qdma_indirect_intr_ctxt *ctxt)
{
	int rv = 0;
	uint32_t intr_ctxt[QDMA_S80_HARD_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint64_t baddr_l, baddr_h;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p intr_ctxt=%p, err:%d\n",
					   __func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, ring_index,
			QDMA_S80_HARD_IND_INTR_CONTEXT_NUM_WORDS, intr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->valid = FIELD_GET(INTR_CTXT_DATA_W0_VALID_MASK, intr_ctxt[0]);
	ctxt->vec = FIELD_GET(INTR_CTXT_DATA_W0_VEC_MASK,
			intr_ctxt[0]);
	ctxt->int_st = FIELD_GET(INTR_CTXT_DATA_W0_INT_ST_MASK,
			intr_ctxt[0]);
	ctxt->color =
		(uint8_t)(FIELD_GET(INTR_CTXT_DATA_W0_COLOR_MASK,
			intr_ctxt[0]));
	baddr_l = FIELD_GET(INTR_CTXT_DATA_W0_BADDR_4K_L_MASK,
			intr_ctxt[0]);

	baddr_h = FIELD_GET(INTR_CTXT_DATA_W1_BADDR_4K_H_MASK,
			intr_ctxt[1]);
	ctxt->page_size =
		(uint8_t)(FIELD_GET(INTR_CTXT_DATA_W1_PAGE_SIZE_MASK,
			intr_ctxt[1]));
	ctxt->pidx = FIELD_GET(INTR_CTXT_DATA_W2_PIDX_MASK,
			intr_ctxt[2]);

	ctxt->baddr_4k =
		FIELD_SET(QDMA_S80_HARD_INTR_CTXT_BADDR_GET_L_MASK, baddr_l) |
		FIELD_SET(QDMA_S80_HARD_INTR_CTXT_BADDR_GET_H_MASK, baddr_h);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_indirect_intr_context_clear() - clear indirect
 * interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_indirect_intr_context_clear(void *dev_hndl,
		uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, ring_index);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_indirect_intr_context_invalidate() - invalidate
 * indirect interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_indirect_intr_context_invalidate(void *dev_hndl,
					  uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, ring_index);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_indirect_intr_ctx_conf() - configure indirect interrupt context
 *
 * @dev_hndl:	device handle
 * @ring_index:	indirect interrupt ring index
 * @ctxt:	pointer to context data
 * @access_type HW access type (qdma_hw_access_type enum) value
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_indirect_intr_ctx_conf(void *dev_hndl, uint16_t ring_index,
				struct qdma_indirect_intr_ctxt *ctxt,
				enum qdma_hw_access_type access_type)
{
	int ret_val = 0;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		ret_val = qdma_s80_hard_indirect_intr_context_read(dev_hndl,
							      ring_index,
							      ctxt);
		break;
	case QDMA_HW_ACCESS_WRITE:
		ret_val = qdma_s80_hard_indirect_intr_context_write(dev_hndl,
							       ring_index,
							       ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		ret_val = qdma_s80_hard_indirect_intr_context_clear(dev_hndl,
							   ring_index);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		ret_val = qdma_s80_hard_indirect_intr_context_invalidate(
				dev_hndl, ring_index);
		break;
	default:
		qdma_log_error("%s: access_type=%d is invalid, err:%d\n",
					   __func__, access_type,
					   -QDMA_ERR_INV_PARAM);
		ret_val = -QDMA_ERR_INV_PARAM;
		break;
	}

	return ret_val;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_set_default_global_csr() - function to set the global
 *  CSR register to default values. The value can be modified later by using
 *  the set/get csr functions
 *
 * @dev_hndl:	device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_set_default_global_csr(void *dev_hndl)
{
	/* Default values */
	uint32_t reg_val = 0;
	uint32_t rng_sz[QDMA_NUM_RING_SIZES] = {2049, 65, 129, 193, 257,
				385, 513, 769, 1025, 1537, 3073, 4097, 6145,
				8193, 12289, 16385};
	uint32_t tmr_cnt[QDMA_NUM_C2H_TIMERS] = {1, 2, 4, 5, 8, 10, 15, 20, 25,
				30, 50, 75, 100, 125, 150, 200};
	uint32_t cnt_th[QDMA_NUM_C2H_COUNTERS] = {2, 4, 8, 16, 24,
				32, 48, 64, 80, 96, 112, 128, 144,
				160, 176, 192};
	uint32_t buf_sz[QDMA_NUM_C2H_BUFFER_SIZES] = {4096, 256, 512, 1024,
				2048, 3968, 4096, 4096, 4096, 4096, 4096, 4096,
				4096, 8192, 9018, 16384};
	struct qdma_dev_attributes dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	/* Configuring CSR registers */
	/* Global ring sizes */
	qdma_write_csr_values(dev_hndl, QDMA_S80_HARD_GLBL_RNG_SZ_1_ADDR, 0,
					QDMA_NUM_RING_SIZES, rng_sz);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en) {
		/* Counter thresholds */
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_CNT_TH_1_ADDR, 0,
				QDMA_NUM_C2H_COUNTERS, cnt_th);

		/* Timer Counters */
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_TIMER_CNT_1_ADDR, 0,
				QDMA_NUM_C2H_TIMERS, tmr_cnt);


		/* Writeback Interval */
		reg_val =
			FIELD_SET(GLBL_DSC_CFG_MAXFETCH_MASK,
				  DEFAULT_MAX_DSC_FETCH) |
				  FIELD_SET(GLBL_DSC_CFG_WB_ACC_INT_MASK,
				  DEFAULT_WRB_INT);
		qdma_reg_write(dev_hndl,
				QDMA_S80_HARD_GLBL_DSC_CFG_ADDR, reg_val);
	}

	if (dev_cap.st_en) {
		/* Buffer Sizes */
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_BUF_SZ_0_ADDR, 0,
				QDMA_NUM_C2H_BUFFER_SIZES, buf_sz);

		/* Prefetch Configuration */
		reg_val =
			FIELD_SET(C2H_PFCH_CFG_FL_TH_MASK,
				DEFAULT_PFCH_STOP_THRESH) |
				FIELD_SET(C2H_PFCH_CFG_NUM_MASK,
				DEFAULT_PFCH_NUM_ENTRIES_PER_Q) |
				FIELD_SET(C2H_PFCH_CFG_QCNT_MASK,
				DEFAULT_PFCH_MAX_Q_CNT) |
				FIELD_SET(C2H_PFCH_CFG_EVT_QCNT_TH_MASK,
				DEFAULT_C2H_INTR_TIMER_TICK);
		qdma_reg_write(dev_hndl,
				QDMA_S80_HARD_C2H_PFCH_CFG_ADDR, reg_val);

		/* C2H interrupt timer tick */
		qdma_reg_write(dev_hndl, QDMA_S80_HARD_C2H_INT_TIMER_TICK_ADDR,
						DEFAULT_C2H_INTR_TIMER_TICK);

		/* C2h Completion Coalesce Configuration */
		reg_val =
			FIELD_SET(C2H_WRB_COAL_CFG_TICK_CNT_MASK,
				DEFAULT_CMPT_COAL_TIMER_CNT) |
				FIELD_SET(C2H_WRB_COAL_CFG_TICK_VAL_MASK,
				DEFAULT_CMPT_COAL_TIMER_TICK) |
				FIELD_SET(C2H_WRB_COAL_CFG_MAX_BUF_SZ_MASK,
				DEFAULT_CMPT_COAL_MAX_BUF_SZ);
		qdma_reg_write(dev_hndl,
				QDMA_S80_HARD_C2H_WRB_COAL_CFG_ADDR, reg_val);

#if 0
		/* H2C throttle Configuration*/
		reg_val =
			FIELD_SET(QDMA_H2C_DATA_THRESH_MASK,
				DEFAULT_H2C_THROT_DATA_THRESH) |
				FIELD_SET(QDMA_H2C_REQ_THROT_EN_DATA_MASK,
				DEFAULT_THROT_EN_DATA);
		qdma_reg_write(dev_hndl, QDMA_OFFSET_H2C_REQ_THROT, reg_val);
#endif
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_queue_pidx_update() - function to update the desc PIDX
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @is_c2h:	Queue direction. Set 1 for C2H and 0 for H2C
 * @reg_info:	data needed for the PIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info)
{
	uint32_t reg_addr = 0;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	if (!reg_info) {
		qdma_log_error("%s: reg_info is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!is_vf) {
		reg_addr = (is_c2h) ?
			QDMA_S80_HARD_OFFSET_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_S80_HARD_OFFSET_DMAP_SEL_H2C_DSC_PIDX;
	} else {
		reg_addr = (is_c2h) ?
			QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX;
	}

	reg_addr += (qid * QDMA_PIDX_STEP);

	reg_val = FIELD_SET(QDMA_S80_HARD_DMA_SEL_DESC_PIDX_MASK,
					reg_info->pidx) |
			  FIELD_SET(QDMA_S80_HARD_DMA_SEL_IRQ_EN_MASK,
					reg_info->irq_en);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_queue_cmpt_cidx_update() - function to update the CMPT
 * CIDX update
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ?
		QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
		QDMA_S80_HARD_OFFSET_DMAP_SEL_CMPT_CIDX;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!reg_info) {
		qdma_log_error("%s: reg_info is NULL, err:%d\n",
						__func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr += (qid * QDMA_CMPT_CIDX_STEP);

	reg_val =
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_WRB_CIDX_MASK,
				reg_info->wrb_cidx) |
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_CNT_THRESH_MASK,
				reg_info->counter_idx) |
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_TMR_CNT_MASK,
				reg_info->timer_idx) |
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_TRG_MODE_MASK,
				reg_info->trig_mode) |
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_STS_DESC_EN_MASK,
				reg_info->wrb_en) |
		FIELD_SET(QDMA_S80_HARD_DMAP_SEL_CMPT_IRQ_EN_MASK,
				reg_info->irq_en);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_queue_intr_cidx_update() - function to update the
 * CMPT CIDX update
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, const struct qdma_intr_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ?
		QDMA_S80_HARD_OFFSET_VF_DMAP_SEL_INT_CIDX :
		QDMA_S80_HARD_OFFSET_DMAP_SEL_INT_CIDX;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!reg_info) {
		qdma_log_error("%s: reg_info is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr += qid * QDMA_INT_CIDX_STEP;

	reg_val =
		FIELD_SET(QDMA_S80_HARD_DMA_SEL_INT_SW_CIDX_MASK,
			reg_info->sw_cidx) |
		FIELD_SET(QDMA_S80_HARD_DMA_SEL_INT_RING_IDX_MASK,
			reg_info->rng_idx);

	qdma_reg_write(dev_hndl, reg_addr, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_cmp_get_user_bar() - Function to get the
 *			AXI Master Lite(user bar) number
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @func_id:	function id of the PF
 * @user_bar:	pointer to hold the AXI Master Lite(user bar) number
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_cmp_get_user_bar(void *dev_hndl, uint8_t is_vf,
		uint8_t func_id, uint8_t *user_bar)
{
	uint8_t bar_found = 0;
	uint8_t bar_idx = 0;
	uint32_t user_bar_id = 0;
	uint32_t reg_addr = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!user_bar) {
		qdma_log_error("%s: user_bar is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_addr = (is_vf) ? QDMA_S80_HARD_GLBL2_PF_VF_BARLITE_EXT_ADDR :
			QDMA_S80_HARD_GLBL2_PF_BARLITE_EXT_ADDR;

	if (!is_vf) {
		user_bar_id = qdma_reg_read(dev_hndl, reg_addr);
		user_bar_id = (user_bar_id >> (6 * func_id)) & 0x3F;
	} else {
		*user_bar = QDMA_S80_HARD_VF_USER_BAR_ID;
		return QDMA_SUCCESS;
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
		qdma_log_error("%s: Bar not found, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_BAR_NOT_FOUND);
		return -QDMA_ERR_HWACC_BAR_NOT_FOUND;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_ram_sbe_err_process() -Function to dump SBE err debug info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_ram_sbe_err_process(void *dev_hndl)
{
	qdma_s80_hard_dump_reg_info(dev_hndl, QDMA_S80_HARD_RAM_SBE_STS_A_ADDR,
						1, NULL, 0);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_ram_dbe_err_process() -Function to dump DBE err debug info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_ram_dbe_err_process(void *dev_hndl)
{
	qdma_s80_hard_dump_reg_info(dev_hndl, QDMA_S80_HARD_RAM_DBE_STS_A_ADDR,
						1, NULL, 0);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_desc_err_process() -Function to dump Descriptor Error info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_desc_err_process(void *dev_hndl)
{
	int i = 0;
	uint32_t desc_err_reg_list[] = {
		QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_LOG0_ADDR,
		QDMA_S80_HARD_GLBL_DSC_ERR_LOG1_ADDR,
		QDMA_S80_HARD_GLBL_DSC_DBG_DAT0_ADDR,
		QDMA_S80_HARD_GLBL_DSC_DBG_DAT1_ADDR
	};
	int desc_err_num_regs = sizeof(desc_err_reg_list)/sizeof(uint32_t);

	for (i = 0; i < desc_err_num_regs; i++) {
		qdma_s80_hard_dump_reg_info(dev_hndl,
					desc_err_reg_list[i],
					1, NULL, 0);
	}
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_trq_err_process() -Function to dump Target Access Err info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_trq_err_process(void *dev_hndl)
{
	int i = 0;
	uint32_t trq_err_reg_list[] = {
		QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR,
		QDMA_S80_HARD_GLBL_TRQ_ERR_LOG_ADDR
	};
	int trq_err_reg_num_regs = sizeof(trq_err_reg_list)/sizeof(uint32_t);

	for (i = 0; i < trq_err_reg_num_regs; i++) {
		qdma_s80_hard_dump_reg_info(dev_hndl, trq_err_reg_list[i],
					1, NULL, 0);
	}


}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_st_h2c_err_process() - Function to dump MM H2C Error info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_st_h2c_err_process(void *dev_hndl)
{
	int i = 0;
	uint32_t st_h2c_err_reg_list[] = {
		QDMA_S80_HARD_H2C_ERR_STAT_ADDR,
		QDMA_S80_HARD_H2C_FIRST_ERR_QID_ADDR,
		QDMA_S80_HARD_H2C_DBG_REG0_ADDR,
		QDMA_S80_HARD_H2C_DBG_REG1_ADDR,
		QDMA_S80_HARD_H2C_DBG_REG2_ADDR,
		QDMA_S80_HARD_H2C_DBG_REG3_ADDR,
		QDMA_S80_HARD_H2C_DBG_REG4_ADDR
	};
	int st_h2c_err_num_regs = sizeof(st_h2c_err_reg_list)/sizeof(uint32_t);

	for (i = 0; i < st_h2c_err_num_regs; i++) {
		qdma_s80_hard_dump_reg_info(dev_hndl,
					st_h2c_err_reg_list[i],
					1, NULL, 0);
	}
}


/*****************************************************************************/
/**
 * qdma_s80_hard_hw_st_c2h_err_process() - Function to dump MM H2C Error info
 *
 * @dev_hndl: device handle
 * @buf: Bufffer for the debug info to be dumped in
 * @buflen: Length of the buffer
 *
 * Return: void
 *****************************************************************************/
static void qdma_s80_hard_hw_st_c2h_err_process(void *dev_hndl)
{
	int i = 0;
	uint32_t st_c2h_err_reg_list[] = {
		QDMA_S80_HARD_C2H_ERR_STAT_ADDR,
		QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR,
		QDMA_S80_HARD_C2H_FIRST_ERR_QID_ADDR,
		QDMA_S80_HARD_C2H_STAT_S_AXIS_C2H_ACCEPTED_ADDR,
		QDMA_S80_HARD_C2H_STAT_S_AXIS_WRB_ACCEPTED_ADDR,
		QDMA_S80_HARD_C2H_STAT_DESC_RSP_PKT_ACCEPTED_ADDR,
		QDMA_S80_HARD_C2H_STAT_AXIS_PKG_CMP_ADDR,
		QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_0_ADDR,
		QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_1_ADDR,
		QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_2_ADDR,
		QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_3_ADDR,
		QDMA_S80_HARD_C2H_STAT_DESC_RSP_DROP_ACCEPTED_ADDR,
		QDMA_S80_HARD_C2H_STAT_DESC_RSP_ERR_ACCEPTED_ADDR
	};
	int st_c2h_err_num_regs = sizeof(st_c2h_err_reg_list)/sizeof(uint32_t);

	for (i = 0; i < st_c2h_err_num_regs; i++) {
		qdma_s80_hard_dump_reg_info(dev_hndl,
					st_c2h_err_reg_list[i],
					1, NULL, 0);
	}
}



/*****************************************************************************/
/**
 * qdma_s80_hard_hw_get_error_name() - Function to get the error in str format
 *
 * @err_idx: error index
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
const char *qdma_s80_hard_hw_get_error_name(uint32_t err_idx)
{
	if (err_idx >= QDMA_S80_HARD_ERRS_ALL) {
		qdma_log_error("%s: err_idx=%d is invalid, returning NULL\n",
			__func__,
			(enum qdma_s80_hard_error_idx)err_idx);
		return NULL;
	}

	return qdma_s80_hard_err_info[
			(enum qdma_s80_hard_error_idx)err_idx].err_name;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_hw_error_process() - Function to find the error that got
 * triggered and call the handler qdma_hw_error_handler of that
 * particular error.
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_hw_error_process(void *dev_hndl)
{
	uint32_t glbl_err_stat = 0, err_stat = 0;
	uint32_t i = 0, j = 0;
	int32_t idx = 0;
	struct qdma_dev_attributes dev_cap;
	uint32_t hw_err_position[QDMA_S80_HARD_TOTAL_LEAF_ERROR_AGGREGATORS] = {
		QDMA_S80_HARD_DSC_ERR_POISON,
		QDMA_S80_HARD_TRQ_ERR_UNMAPPED,
		QDMA_S80_HARD_ST_C2H_ERR_MTY_MISMATCH,
		QDMA_S80_HARD_ST_FATAL_ERR_MTY_MISMATCH,
		QDMA_S80_HARD_ST_H2C_ERR_ZERO_LEN_DESC_ERR,
		QDMA_S80_HARD_SBE_ERR_MI_H2C0_DAT,
		QDMA_S80_HARD_DBE_ERR_MI_H2C0_DAT
	};

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}


	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	glbl_err_stat = qdma_reg_read(dev_hndl,
			QDMA_S80_HARD_GLBL_ERR_STAT_ADDR);
	if (!glbl_err_stat)
		return QDMA_HW_ERR_NOT_DETECTED;

	qdma_log_info("%s: Global Err Reg(0x%x) = 0x%x\n",
				  __func__, QDMA_S80_HARD_GLBL_ERR_STAT_ADDR,
				  glbl_err_stat);

	for (i = 0; i < QDMA_S80_HARD_TOTAL_LEAF_ERROR_AGGREGATORS; i++) {
		j = hw_err_position[i];

		if ((!dev_cap.st_en) &&
			(j == QDMA_S80_HARD_ST_C2H_ERR_MTY_MISMATCH ||
			j == QDMA_S80_HARD_ST_FATAL_ERR_MTY_MISMATCH ||
			j == QDMA_S80_HARD_ST_H2C_ERR_ZERO_LEN_DESC_ERR))
			continue;

		err_stat = qdma_reg_read(dev_hndl,
				qdma_s80_hard_err_info[j].stat_reg_addr);
		if (err_stat) {
			qdma_log_info("addr = 0x%08x val = 0x%08x",
				qdma_s80_hard_err_info[j].stat_reg_addr,
				err_stat);

			qdma_s80_hard_err_info[j].qdma_s80_hard_hw_err_process(
				dev_hndl);
			for (idx = j;
				idx < all_qdma_s80_hard_hw_errs[i];
				idx++) {
				/* call the platform specific handler */
				if (err_stat &
				qdma_s80_hard_err_info[idx].leaf_err_mask)
					qdma_log_error("%s detected %s\n",
						__func__,
						qdma_s80_hard_hw_get_error_name(
							idx));
			}
			qdma_reg_write(dev_hndl,
				qdma_s80_hard_err_info[j].stat_reg_addr,
				err_stat);
		}

	}

	/* Write 1 to the global status register to clear the bits */
	qdma_reg_write(dev_hndl,
			QDMA_S80_HARD_GLBL_ERR_STAT_ADDR, glbl_err_stat);

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
int qdma_s80_hard_hw_error_enable(void *dev_hndl, uint32_t err_idx)
{
	uint32_t idx = 0, i = 0;
	uint32_t reg_val = 0;
	struct qdma_dev_attributes dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (err_idx > QDMA_S80_HARD_ERRS_ALL) {
		qdma_log_error("%s: err_idx=%d is invalid, err:%d\n",
				__func__, (enum qdma_s80_hard_error_idx)err_idx,
				-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (err_idx == QDMA_S80_HARD_ERRS_ALL) {
		for (i = 0;
				i < QDMA_S80_HARD_TOTAL_LEAF_ERROR_AGGREGATORS;
				i++) {

			idx = all_qdma_s80_hard_hw_errs[i];

			/* Don't access streaming registers in
			 * MM only bitstreams
			 */
			if (!dev_cap.st_en) {
				if (idx == QDMA_S80_HARD_ST_C2H_ERR_ALL ||
					idx == QDMA_S80_HARD_ST_FATAL_ERR_ALL ||
					idx == QDMA_S80_HARD_ST_H2C_ERR_ALL)
					continue;
			}

			reg_val = qdma_s80_hard_err_info[idx].leaf_err_mask;
			qdma_reg_write(dev_hndl,
				qdma_s80_hard_err_info[idx].mask_reg_addr,
					reg_val);

			reg_val = qdma_reg_read(dev_hndl,
					QDMA_S80_HARD_GLBL_ERR_MASK_ADDR);
			reg_val |= FIELD_SET(
				qdma_s80_hard_err_info[idx].global_err_mask, 1);
			qdma_reg_write(dev_hndl,
					QDMA_S80_HARD_GLBL_ERR_MASK_ADDR,
					reg_val);
		}

	} else {
		/* Don't access streaming registers in MM only bitstreams
		 *  QDMA_C2H_ERR_MTY_MISMATCH to QDMA_H2C_ERR_ALL are all
		 *  ST errors
		 */
		if (!dev_cap.st_en) {
			if (err_idx >= QDMA_S80_HARD_ST_C2H_ERR_MTY_MISMATCH &&
					err_idx <= QDMA_S80_HARD_ST_H2C_ERR_ALL)
				return QDMA_SUCCESS;
		}

		reg_val = qdma_reg_read(dev_hndl,
				qdma_s80_hard_err_info[err_idx].mask_reg_addr);
		reg_val |=
			FIELD_SET(qdma_s80_hard_err_info[err_idx].leaf_err_mask,
				1);
		qdma_reg_write(dev_hndl,
				qdma_s80_hard_err_info[err_idx].mask_reg_addr,
						reg_val);

		reg_val = qdma_reg_read(dev_hndl,
			QDMA_S80_HARD_GLBL_ERR_MASK_ADDR);
		reg_val |=
			FIELD_SET(
				qdma_s80_hard_err_info[err_idx].global_err_mask,
				1);
		qdma_reg_write(dev_hndl,
			QDMA_S80_HARD_GLBL_ERR_MASK_ADDR, reg_val);
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_get_device_attributes() - Function to get the qdma
 * device attributes
 *
 * @dev_hndl:	device handle
 * @dev_info:	pointer to hold the device info
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info)
{
	uint8_t count = 0;
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	if (!dev_info) {
		qdma_log_error("%s: dev_info is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	/* number of PFs */
	reg_val = qdma_reg_read(dev_hndl,
		QDMA_S80_HARD_GLBL2_PF_BARLITE_INT_ADDR);
	if (FIELD_GET(GLBL2_PF_BARLITE_INT_PF0_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(GLBL2_PF_BARLITE_INT_PF1_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(GLBL2_PF_BARLITE_INT_PF2_BAR_MAP_MASK, reg_val))
		count++;
	if (FIELD_GET(GLBL2_PF_BARLITE_INT_PF3_BAR_MAP_MASK, reg_val))
		count++;
	dev_info->num_pfs = count;

	/* Number of Qs */
	reg_val = qdma_reg_read(dev_hndl, QDMA_S80_HARD_GLBL2_CHANNEL_CAP_ADDR);
	dev_info->num_qs = (FIELD_GET(GLBL2_CHANNEL_CAP_MULTIQ_MAX_MASK,
			reg_val));

	/* FLR present */
	reg_val = qdma_reg_read(dev_hndl, QDMA_S80_HARD_GLBL2_MISC_CAP_ADDR);
	dev_info->mailbox_en  = FIELD_GET(QDMA_GLBL2_MAILBOX_EN_MASK, reg_val);
	dev_info->flr_present = FIELD_GET(QDMA_GLBL2_FLR_PRESENT_MASK, reg_val);
	dev_info->mm_cmpt_en  = 0;

	/* ST/MM enabled? */
	reg_val = qdma_reg_read(dev_hndl,
		QDMA_S80_HARD_GLBL2_CHANNEL_MDMA_ADDR);
	dev_info->mm_en = (FIELD_GET(GLBL2_CHANNEL_MDMA_C2H_ENG_MASK, reg_val)
		&& FIELD_GET(GLBL2_CHANNEL_MDMA_H2C_ENG_MASK, reg_val)) ? 1 : 0;
	dev_info->st_en = (FIELD_GET(GLBL2_CHANNEL_MDMA_C2H_ST_MASK, reg_val)
		&& FIELD_GET(GLBL2_CHANNEL_MDMA_H2C_ST_MASK,
		reg_val)) ? 1 : 0;

	/* num of mm channels for Versal Hard is 2 */
	dev_info->mm_channel_max = 2;

	dev_info->debug_mode = 0;
	dev_info->desc_eng_mode = 0;
	dev_info->qid2vec_ctx = 1;
	dev_info->cmpt_ovf_chk_dis = 0;
	dev_info->mailbox_intr = 0;
	dev_info->sw_desc_64b = 0;
	dev_info->cmpt_desc_64b = 0;
	dev_info->dynamic_bar = 0;
	dev_info->legacy_intr = 0;
	dev_info->cmpt_trig_count_timer = 0;

	return QDMA_SUCCESS;
}


/*****************************************************************************/
/**
 * qdma_s80_hard_credit_context_read() - read credit context
 *
 * @dev_hndl:	device handle
 * @c2h     :	is c2h queue
 * @hw_qid  :	hardware qid of the queue
 * @ctxt    :	pointer to the context data
 *
 * Return   :	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_credit_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_credit_ctxt *ctxt)
{
	int rv = QDMA_SUCCESS;
	uint32_t cr_ctxt[QDMA_S80_HARD_CR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl || !ctxt) {
		qdma_log_error("%s: dev_hndl=%p credit_ctxt=%p, err:%d\n",
						__func__, dev_hndl, ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_S80_HARD_CR_CONTEXT_NUM_WORDS, cr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->credit = FIELD_GET(CRED_CTXT_DATA_W0_CREDT_MASK,
			cr_ctxt[0]);

	qdma_log_debug("%s: credit=%u\n", __func__, ctxt->credit);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_credit_context_clear() - clear credit context
 *
 * @dev_hndl:	device handle
 * @c2h     :	is c2h queue
 * @hw_qid  :	hardware qid of the queue
 *
 * Return   :	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_credit_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_credit_context_invalidate() - invalidate credit context
 *
 * @dev_hndl:	device handle
 * @c2h     :	is c2h queue
 * @hw_qid  :	hardware qid of the queue
 *
 * Return   :	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_credit_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return qdma_s80_hard_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

/*****************************************************************************/
/**
 * qdma_s80_hard_credit_ctx_conf() - configure credit context
 *
 * @dev_hndl    :	device handle
 * @c2h         :	is c2h queue
 * @hw_qid      :	hardware qid of the queue
 * @ctxt        :	pointer to the context data
 * @access_type :	HW access type (qdma_hw_access_type enum) value
 *		QDMA_HW_ACCESS_WRITE Not supported
 *
 * Return       :	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_credit_ctx_conf(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
			struct qdma_descq_credit_ctxt *ctxt,
			enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv = qdma_s80_hard_credit_context_read(dev_hndl, c2h,
				hw_qid, ctxt);
		break;
	case QDMA_HW_ACCESS_CLEAR:
		rv = qdma_s80_hard_credit_context_clear(dev_hndl, c2h, hw_qid);
		break;
	case QDMA_HW_ACCESS_INVALIDATE:
		rv = qdma_s80_hard_credit_context_invalidate(dev_hndl, c2h,
				hw_qid);
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
 * qdma_s80_hard_dump_config_regs() - Function to get qdma config register
 * dump in a buffer
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @buf :	pointer to buffer to be filled
 * @buflen :	Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen)
{
	uint32_t i = 0, j = 0;
	struct xreg_info *reg_info;
	uint32_t num_regs = qdma_s80_hard_config_num_regs_get();
	uint32_t len = 0, val = 0;
	int rv = QDMA_SUCCESS;
	char name[DEBGFS_GEN_NAME_SZ] = "";
	struct qdma_dev_attributes dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (buflen < qdma_s80_hard_reg_dump_buf_len()) {
		qdma_log_error("%s: Buffer too small, err:%d\n", __func__,
					   -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	if (is_vf) {
		qdma_log_error("%s: Wrong API used for VF, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	reg_info = qdma_s80_hard_config_regs_get();

	for (i = 0; i < num_regs; i++) {
		if ((GET_CAPABILITY_MASK(dev_cap.mm_en, dev_cap.st_en,
				dev_cap.mm_cmpt_en, dev_cap.mailbox_en)
				& reg_info[i].mode) == 0)
			continue;

		for (j = 0; j < reg_info[i].repeat; j++) {
			rv = QDMA_SNPRINTF_S(name, DEBGFS_GEN_NAME_SZ,
					DEBGFS_GEN_NAME_SZ,
					"%s_%d", reg_info[i].name, j);
			if ((rv < 0) || (rv > DEBGFS_GEN_NAME_SZ)) {
				qdma_log_error(
					"%d:%s QDMA_SNPRINTF_S() failed, err:%d\n",
					__LINE__, __func__,
					rv);
				return -QDMA_ERR_NO_MEM;
			}

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
 * qdma_dump_s80_hard_queue_context() - Function to get qdma queue context dump
 * in a buffer
 *
 * @dev_hndl:   device handle
 * @hw_qid:     queue id
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_dump_queue_context(void *dev_hndl,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen)
{
	int rv = 0;
	uint32_t req_buflen = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!ctxt_data) {
		qdma_log_error("%s: ctxt_data is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!buf) {
		qdma_log_error("%s: buf is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}
	if (q_type >= QDMA_DEV_Q_TYPE_MAX) {
		qdma_log_error("%s: invalid q_type, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_context_buf_len(st, q_type, &req_buflen);
	if (rv != QDMA_SUCCESS)
		return rv;

	if (buflen < req_buflen) {
		qdma_log_error("%s: Too small buffer(%d), reqd(%d), err:%d\n",
			__func__, buflen, req_buflen, -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	rv = dump_s80_hard_context(ctxt_data, st, q_type,
				buf, buflen);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_dump_s80_hard_intr_context() - Function to get qdma interrupt
 * context dump in a buffer
 *
 * @dev_hndl:   device handle
 * @hw_qid:     queue id
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_dump_intr_context(void *dev_hndl,
		struct qdma_indirect_intr_ctxt *intr_ctx,
		int ring_index,
		char *buf, uint32_t buflen)
{
	int rv = 0;
	uint32_t req_buflen = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!intr_ctx) {
		qdma_log_error("%s: intr_ctx is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!buf) {
		qdma_log_error("%s: buf is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	req_buflen = qdma_s80_hard_intr_context_buf_len();
	if (buflen < req_buflen) {
		qdma_log_error("%s: Too small buffer(%d), reqd(%d), err:%d\n",
			__func__, buflen, req_buflen, -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	rv = dump_s80_hard_intr_context(intr_ctx, ring_index, buf, buflen);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_read_dump_queue_context() - Function to read and dump the queue
 * context in a buffer
 *
 * @dev_hndl:   device handle
 * @hw_qid:     queue id
 * @st:			Queue Mode(ST or MM)
 * @q_type:		Queue type(H2C/C2H/CMPT)
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_s80_hard_read_dump_queue_context(void *dev_hndl,
		uint16_t qid_hw,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		char *buf, uint32_t buflen)
{
	int rv = QDMA_SUCCESS;
	uint32_t req_buflen = 0;
	struct qdma_descq_context context;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!buf) {
		qdma_log_error("%s: buf is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (q_type >= QDMA_DEV_Q_TYPE_CMPT) {
		qdma_log_error("%s: Not supported for q_type, err = %d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_s80_hard_context_buf_len(st, q_type, &req_buflen);

	if (rv != QDMA_SUCCESS)
		return rv;

	if (buflen < req_buflen) {
		qdma_log_error("%s: Too small buffer(%d), reqd(%d), err:%d\n",
			__func__, buflen, req_buflen, -QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	qdma_memset(&context, 0, sizeof(struct qdma_descq_context));

	if (q_type != QDMA_DEV_Q_TYPE_CMPT) {
		rv = qdma_s80_hard_sw_ctx_conf(dev_hndl, (uint8_t)q_type,
				qid_hw, &(context.sw_ctxt),
				QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
			"%s: Failed to read sw context, err = %d",
					__func__, rv);
			return rv;
		}

		rv = qdma_s80_hard_hw_ctx_conf(dev_hndl, (uint8_t)q_type,
				qid_hw, &(context.hw_ctxt),
				QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
			"%s: Failed to read hw context, err = %d",
					__func__, rv);
			return rv;
		}

		rv = qdma_s80_hard_qid2vec_conf(dev_hndl, (uint8_t)q_type,
				qid_hw, &(context.qid2vec),
				QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
			"%s: Failed to read qid2vec context, err = %d",
					__func__, rv);
			return rv;
		}

		rv = qdma_s80_hard_credit_ctx_conf(dev_hndl, (uint8_t)q_type,
				qid_hw, &(context.cr_ctxt),
				QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
			"%s: Failed to read credit context, err = %d",
					__func__, rv);
			return rv;
		}

		if (st && (q_type == QDMA_DEV_Q_TYPE_C2H)) {
			rv = qdma_s80_hard_pfetch_ctx_conf(dev_hndl,
					qid_hw,
					&(context.pfetch_ctxt),
					QDMA_HW_ACCESS_READ);
			if (rv < 0) {
				qdma_log_error(
			"%s: Failed to read pftech context, err = %d",
						__func__, rv);
				return rv;
			}
		}
	}

	if ((st && (q_type == QDMA_DEV_Q_TYPE_C2H)) ||
			(!st && (q_type == QDMA_DEV_Q_TYPE_CMPT))) {
		rv = qdma_s80_hard_cmpt_ctx_conf(dev_hndl, qid_hw,
						&(context.cmpt_ctxt),
						 QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error(
			"%s: Failed to read cmpt context, err = %d",
					__func__, rv);
			return rv;
		}
	}



	rv = dump_s80_hard_context(&context, st, q_type,
				buf, buflen);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_init_ctxt_memory() - Initialize the context for all queues
 *
 * @dev_hndl    :	device handle
 *
 * Return       :	0   - success and < 0 - failure
 *****************************************************************************/

int qdma_s80_hard_init_ctxt_memory(void *dev_hndl)
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
	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_info);
	qdma_log_info("%s: clearing the context for all qs",
			__func__);
	for (; i < dev_info.num_qs; i++) {
		int sel = QDMA_CTXT_SEL_SW_C2H;
		int rv;

		for (; sel <= QDMA_CTXT_SEL_PFTCH; sel++) {
			/** if the st mode(h2c/c2h) not enabled
			 *  in the design, then skip the PFTCH
			 *  and CMPT context setup
			 */
			if ((dev_info.st_en == 0) &&
			    (sel == QDMA_CTXT_SEL_PFTCH ||
				sel == QDMA_CTXT_SEL_CMPT)) {
				qdma_log_debug("%s: ST context is skipped:",
					__func__);
				qdma_log_debug(" sel = %d", sel);
				continue;
			}

			rv = qdma_s80_hard_indirect_reg_clear(dev_hndl,
					(enum ind_ctxt_cmd_sel)sel, i);
			if (rv < 0)
				return rv;
		}
	}

	/* fmap */
	for (i = 0; i < dev_info.num_pfs; i++)
		qdma_s80_hard_fmap_clear(dev_hndl, i);
#else
	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
#endif
	return 0;
}

static int get_reg_entry(uint32_t reg_addr, int *reg_entry)
{
	uint32_t i = 0;
	struct xreg_info *reg_info;
	uint32_t num_regs = qdma_s80_hard_config_num_regs_get();

	reg_info = qdma_s80_hard_config_regs_get();

	for (i = 0; (i < num_regs - 1); i++) {
		if (reg_info[i].addr == reg_addr) {
			*reg_entry = i;
			break;
		}
	}

	if (i >= num_regs - 1) {
		qdma_log_error("%s: 0x%08x is missing register list, err:%d\n",
					__func__,
					reg_addr,
					-QDMA_ERR_INV_PARAM);
		*reg_entry = -1;
		return -QDMA_ERR_INV_PARAM;
	}

	return 0;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_dump_config_reg_list() - Dump the registers
 *
 * @dev_hndl:		device handle
 * @total_regs :	Max registers to read
 * @reg_list :		array of reg addr and reg values
 * @buf :		pointer to buffer to be filled
 * @buflen :		Length of the buffer
 *
 * Return: returns the platform specific error code
 *****************************************************************************/
int qdma_s80_hard_dump_config_reg_list(void *dev_hndl, uint32_t total_regs,
		struct qdma_reg_data *reg_list, char *buf, uint32_t buflen)
{
	uint32_t j = 0, len = 0;
	uint32_t reg_count = 0;
	int reg_data_entry;
	int rv = 0;
	char name[DEBGFS_GEN_NAME_SZ] = "";
	struct xreg_info *reg_info = qdma_s80_hard_config_regs_get();

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!buf) {
		qdma_log_error("%s: buf is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	for (reg_count = 0;
			(reg_count < total_regs);) {

		rv = get_reg_entry(reg_list[reg_count].reg_addr,
					&reg_data_entry);
		if (rv < 0) {
			qdma_log_error("%s: register missing in list, err:%d\n",
						   __func__,
						   -QDMA_ERR_INV_PARAM);
			return rv;
		}

		for (j = 0; j < reg_info[reg_data_entry].repeat; j++) {
			rv = QDMA_SNPRINTF_S(name, DEBGFS_GEN_NAME_SZ,
					DEBGFS_GEN_NAME_SZ,
					"%s_%d",
					reg_info[reg_data_entry].name, j);
			if ((rv < 0) || (rv > DEBGFS_GEN_NAME_SZ)) {
				qdma_log_error(
					"%d:%s snprintf failed, err:%d\n",
					__LINE__, __func__,
					rv);
				return -QDMA_ERR_NO_MEM;
			}
			rv = dump_reg(buf + len, buflen - len,
				(reg_info[reg_data_entry].addr + (j * 4)),
					name,
					reg_list[reg_count + j].reg_val);
			if (rv < 0) {
				qdma_log_error(
				"%s Buff too small, err:%d\n",
				__func__,
				-QDMA_ERR_NO_MEM);
				return -QDMA_ERR_NO_MEM;
			}
			len += rv;
		}
		reg_count += j;
	}

	return len;

}


/*****************************************************************************/
/**
 * qdma_read_reg_list() - read the register values
 *
 * @dev_hndl:		device handle
 * @is_vf:		Whether PF or VF
 * @total_regs :	Max registers to read
 * @reg_list :		array of reg addr and reg values
 *
 * Return: returns the platform specific error code
 *****************************************************************************/
int qdma_s80_hard_read_reg_list(void *dev_hndl, uint8_t is_vf,
		uint16_t reg_rd_slot,
		uint16_t *total_regs,
		struct qdma_reg_data *reg_list)
{
	uint16_t reg_count = 0, i = 0, j = 0;
	uint32_t num_regs = qdma_s80_hard_config_num_regs_get();
	struct xreg_info *reg_info = qdma_s80_hard_config_regs_get();
	struct qdma_dev_attributes dev_cap;
	uint32_t reg_start_addr = 0;
	int reg_index = 0;
	int rv = 0;

	if (!is_vf) {
		qdma_log_error("%s: not supported for PF, err:%d\n",
				__func__,
				-QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!reg_list) {
		qdma_log_error("%s: reg_list is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	switch (reg_rd_slot) {
	case QDMA_REG_READ_GROUP_1:
			reg_start_addr = QDMA_S80_REG_GROUP_1_START_ADDR;
			break;
	case QDMA_REG_READ_GROUP_2:
			reg_start_addr = QDMA_S80_REG_GROUP_2_START_ADDR;
			break;
	case QDMA_REG_READ_GROUP_3:
			reg_start_addr = QDMA_S80_REG_GROUP_3_START_ADDR;
			break;
	case QDMA_REG_READ_GROUP_4:
			reg_start_addr = QDMA_S80_REG_GROUP_4_START_ADDR;
			break;
	default:
		qdma_log_error("%s: Invalid slot received\n",
			   __func__);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = get_reg_entry(reg_start_addr, &reg_index);
	if (rv < 0) {
		qdma_log_error("%s: register missing in list, err:%d\n",
					   __func__,
					   -QDMA_ERR_INV_PARAM);
		return rv;
	}

	for (i = 0, reg_count = 0;
			((i < num_regs - 1 - reg_index) &&
			(reg_count < QDMA_MAX_REGISTER_DUMP)); i++) {

		if (((GET_CAPABILITY_MASK(dev_cap.mm_en, dev_cap.st_en,
				dev_cap.mm_cmpt_en, dev_cap.mailbox_en)
				& reg_info[i].mode) == 0) ||
			(reg_info[i].read_type == QDMA_REG_READ_PF_ONLY))
			continue;

		for (j = 0; j < reg_info[i].repeat &&
				(reg_count < QDMA_MAX_REGISTER_DUMP);
				j++) {
			reg_list[reg_count].reg_addr =
					(reg_info[i].addr + (j * 4));
			reg_list[reg_count].reg_val =
				qdma_reg_read(dev_hndl,
					reg_list[reg_count].reg_addr);
			reg_count++;
		}
	}

	*total_regs = reg_count;
	return rv;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_write_global_ring_sizes() - set the global ring size array
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
static int qdma_s80_hard_write_global_ring_sizes(void *dev_hndl, uint8_t index,
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

	qdma_write_csr_values(dev_hndl,
			QDMA_S80_HARD_GLBL_RNG_SZ_1_ADDR, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_read_global_ring_sizes() - function to get the
 *	global rng_sz array
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
static int qdma_s80_hard_read_global_ring_sizes(void *dev_hndl, uint8_t index,
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

	qdma_read_csr_values(dev_hndl,
			QDMA_S80_HARD_GLBL_RNG_SZ_1_ADDR, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_write_global_timer_count() - function to set the timer values
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
static int qdma_s80_hard_write_global_timer_count(void *dev_hndl, uint8_t index,
				uint8_t count, const uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en)
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_TIMER_CNT_1_ADDR,
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
 * qdma_s80_hard_read_global_timer_count() - function to get the timer values
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
static int qdma_s80_hard_read_global_timer_count(void *dev_hndl, uint8_t index,
				uint8_t count, uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en)
		qdma_read_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_TIMER_CNT_1_ADDR, index,
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
 * qdma_s80_hard_write_global_counter_threshold() - function to set the counter
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
static int qdma_s80_hard_write_global_counter_threshold(void *dev_hndl,
		uint8_t index,
		uint8_t count, const uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en)
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_CNT_TH_1_ADDR, index,
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
 * qdma_s80_hard_read_global_counter_threshold() - get the counter
 *	threshold values
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
static int qdma_s80_hard_read_global_counter_threshold(void *dev_hndl,
		uint8_t index,
		uint8_t count, uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en)
		qdma_read_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_CNT_TH_1_ADDR, index,
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
 * qdma_s80_hard_write_global_buffer_sizes() - function to set the buffer sizes
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
static int qdma_s80_hard_write_global_buffer_sizes(void *dev_hndl,
		uint8_t index,
		uint8_t count, const uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en)
		qdma_write_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_BUF_SZ_0_ADDR, index,
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
 * qdma_s80_hard_read_global_buffer_sizes() - function to get the buffer sizes
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
static int qdma_s80_hard_read_global_buffer_sizes(void *dev_hndl, uint8_t index,
				uint8_t count, uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en)
		qdma_read_csr_values(dev_hndl,
				QDMA_S80_HARD_C2H_BUF_SZ_0_ADDR, index,
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
 * qdma_s80_hard_global_csr_conf() - function to configure global csr
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
int qdma_s80_hard_global_csr_conf(void *dev_hndl, uint8_t index, uint8_t count,
				uint32_t *csr_val,
				enum qdma_global_csr_type csr_type,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (csr_type) {
	case QDMA_CSR_RING_SZ:
		switch (access_type) {
		case QDMA_HW_ACCESS_READ:
			rv = qdma_s80_hard_read_global_ring_sizes(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv = qdma_s80_hard_write_global_ring_sizes(
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
			rv = qdma_s80_hard_read_global_timer_count(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv = qdma_s80_hard_write_global_timer_count(
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
			qdma_s80_hard_read_global_counter_threshold(
						dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv =
			qdma_s80_hard_write_global_counter_threshold(
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
			qdma_s80_hard_read_global_buffer_sizes(dev_hndl,
						index,
						count,
						csr_val);
			break;
		case QDMA_HW_ACCESS_WRITE:
			rv =
			qdma_s80_hard_write_global_buffer_sizes(dev_hndl,
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
 * qdma_s80_hard_global_writeback_interval_write() -  function to set the
 * writeback interval
 *
 * @dev_hndl	device handle
 * @wb_int:	Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_global_writeback_interval_write(void *dev_hndl,
		enum qdma_wrb_interval wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes dev_cap;

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

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl,
				QDMA_S80_HARD_GLBL_DSC_CFG_ADDR);
		reg_val |= FIELD_SET(GLBL_DSC_CFG_WB_ACC_INT_MASK, wb_int);

		qdma_reg_write(dev_hndl,
				QDMA_S80_HARD_GLBL_DSC_CFG_ADDR, reg_val);
	} else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
			   __func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_global_writeback_interval_read() -  function to get the
 * writeback interval
 *
 * @dev_hndl:	device handle
 * @wb_int:	pointer to the data to hold Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_s80_hard_global_writeback_interval_read(void *dev_hndl,
		enum qdma_wrb_interval *wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!wb_int) {
		qdma_log_error("%s: wb_int is NULL, err:%d\n", __func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.st_en || dev_cap.mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl,
				QDMA_S80_HARD_GLBL_DSC_CFG_ADDR);
		*wb_int = (enum qdma_wrb_interval)FIELD_GET(
				GLBL_DSC_CFG_WB_ACC_INT_MASK, reg_val);
	} else {
		qdma_log_error("%s: ST or MM cmpt not supported, err:%d\n",
			   __func__, -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED);
		return -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED;
	}

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_s80_hard_global_writeback_interval_conf() - function to configure
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
int qdma_s80_hard_global_writeback_interval_conf(void *dev_hndl,
				enum qdma_wrb_interval *wb_int,
				enum qdma_hw_access_type access_type)
{
	int rv = QDMA_SUCCESS;

	switch (access_type) {
	case QDMA_HW_ACCESS_READ:
		rv =
		qdma_s80_hard_global_writeback_interval_read(dev_hndl, wb_int);
		break;
	case QDMA_HW_ACCESS_WRITE:
		rv =
		qdma_s80_hard_global_writeback_interval_write(dev_hndl,
								*wb_int);
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
 * qdma_s80_hard_mm_channel_conf() - Function to enable/disable the MM channel
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
int qdma_s80_hard_mm_channel_conf(void *dev_hndl, uint8_t channel,
				uint8_t is_c2h,
				uint8_t enable)
{
	uint32_t reg_addr = (is_c2h) ?  QDMA_S80_HARD_C2H_CHANNEL_CTL_ADDR :
			QDMA_S80_HARD_H2C_CHANNEL_CTL_ADDR;
	struct qdma_dev_attributes dev_cap;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_s80_hard_get_device_attributes(dev_hndl, &dev_cap);

	if (dev_cap.mm_en) {
		qdma_reg_write(dev_hndl,
				reg_addr + (channel * QDMA_MM_CONTROL_STEP),
				enable);
	}

	return QDMA_SUCCESS;
}

int qdma_s80_hard_dump_reg_info(void *dev_hndl, uint32_t reg_addr,
		uint32_t num_regs, char *buf, uint32_t buflen)
{
	uint32_t total_num_regs = qdma_s80_hard_config_num_regs_get();
	struct xreg_info *config_regs  = qdma_s80_hard_config_regs_get();
	const char *bitfield_name;
	uint32_t i = 0, num_regs_idx = 0, k = 0, j = 0,
			bitfield = 0, lsb = 0, msb = 31;
	int rv = 0;
	uint32_t reg_val;
	uint32_t data_len = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	for (i = 0; i < total_num_regs; i++) {
		if (reg_addr == config_regs[i].addr) {
			j = i;
			break;
		}
	}

	if (i == total_num_regs) {
		qdma_log_error("%s: Register not found err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		if (buf)
			QDMA_SNPRINTF_S(buf, buflen,
					DEBGFS_LINE_SZ,
					"Register not found 0x%x\n", reg_addr);
		return -QDMA_ERR_INV_PARAM;
	}

	num_regs_idx = (j + num_regs < total_num_regs) ?
					(j + num_regs) : total_num_regs;

	for (; j < num_regs_idx ; j++) {
		reg_val = qdma_reg_read(dev_hndl,
				config_regs[j].addr);

		if (buf) {
			rv = QDMA_SNPRINTF_S(buf, buflen,
						DEBGFS_LINE_SZ,
						"\n%-40s 0x%-7x %-#10x %-10d\n",
						config_regs[j].name,
						config_regs[j].addr,
						reg_val, reg_val);
			if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
				qdma_log_error(
					"%s: Insufficient buffer, err:%d\n",
					__func__, -QDMA_ERR_NO_MEM);
				return -QDMA_ERR_NO_MEM;
			}
			buf += rv;
			data_len += rv;
			buflen -= rv;
		} else
			qdma_log_info("%-40s 0x%-7x %-#10x %-10d\n",
						  config_regs[j].name,
						  config_regs[j].addr,
						  reg_val, reg_val);


		for (k = 0;
			 k < config_regs[j].num_bitfields; k++) {

			bitfield =
				config_regs[j].bitfields[k].field_mask;
			bitfield_name =
				config_regs[i].bitfields[k].field_name;
			lsb = 0;
			msb = 31;

			while (!(BIT(lsb) & bitfield))
				lsb++;

			while (!(BIT(msb) & bitfield))
				msb--;

			if (msb != lsb) {
				if (buf) {
					rv = QDMA_SNPRINTF_S(buf, buflen,
							DEBGFS_LINE_SZ,
							"%-40s [%2u,%2u]   %#-10x\n",
							bitfield_name,
							msb, lsb,
							(reg_val & bitfield) >>
								lsb);
					if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
						qdma_log_error(
							"%s: Insufficient buffer, err:%d\n",
							__func__,
							-QDMA_ERR_NO_MEM);
						return -QDMA_ERR_NO_MEM;
					}
					buf += rv;
					data_len += rv;
					buflen -= rv;
				} else
					qdma_log_info(
						"%-40s [%2u,%2u]   %#-10x\n",
						bitfield_name,
						msb, lsb,
						(reg_val & bitfield) >> lsb);


			} else {
				if (buf) {
					rv = QDMA_SNPRINTF_S(buf, buflen,
							DEBGFS_LINE_SZ,
							"%-40s [%5u]   %#-10x\n",
							bitfield_name,
							lsb,
							(reg_val & bitfield) >>
								lsb);
					if ((rv < 0) || (rv > DEBGFS_LINE_SZ)) {
						qdma_log_error(
							"%s: Insufficient buffer, err:%d\n",
							__func__,
							-QDMA_ERR_NO_MEM);
						return -QDMA_ERR_NO_MEM;
					}
					buf += rv;
					data_len += rv;
					buflen -= rv;
				} else
					qdma_log_info(
						"%-40s [%5u]   %#-10x\n",
						bitfield_name,
						lsb,
						(reg_val & bitfield) >> lsb);

			}
		}
	}

	return data_len;

}
