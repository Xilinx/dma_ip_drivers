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

#include "qdma_reg_dump.h"


struct xreg_info qdma_config_regs[] = {

	/* QDMA_TRQ_SEL_GLBL1 (0x00000) */
	{"CFG_BLOCK_ID", 0x00, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_BUSDEV", 0x04, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_MAX_PL_SZ", 0x08, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_MAX_RDRQ_SZ", 0x0C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_SYS_ID", 0x10, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_MSI_EN", 0x14, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_DATA_WIDTH", 0x18, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_CTRL", 0x1C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_AXI_USR_MAX_PL_SZ", 0x40, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_AXI_USR_MAX_RDRQ_SZ",
			0x44, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_MISC_CTRL", 0x4C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_SCRATCH_REG", 0x80, 8, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_SBE_MSK_A", 0xF0, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_SBE_STS_A", 0xF4, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_DBE_MSK_A", 0xF8, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_DBE_STS_A", 0xFC, 1, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_GLBL2 (0x00100) */
	{"GLBL2_ID", 0x100, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_BL_INT", 0x104, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_VF_BL_INT", 0x108, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_BL_EXT", 0x10C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_VF_BL_EXT", 0x110, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_INST", 0x114, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_QDMA", 0x118, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_STRM", 0x11C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_QDMA_CAP", 0x120, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PASID_CAP", 0x128, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_FUNC_RET", 0x12C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_SYS_ID", 0x130, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_MISC_CAP", 0x134, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_PCIE_RQ", 0x1B8, 2, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_AXIMM_WR", 0x1C0, 2, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_AXIMM_RD", 0x1C8, 2, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_GLBL (0x00200) */
	{"GLBL_RNGSZ", 0x204, 16, 1, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_ERR_STAT", 0x248, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_ERR_MASK", 0x24C, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_CFG", 0x250, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_STS", 0x254, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_MSK", 0x258, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_LOG", 0x25C, 2,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_STS", 0x264, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_MSK", 0x268, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_LOG", 0x26C, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_DBG_DAT", 0x270, 2,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_LOG2", 0x27C, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_INTERRUPT_CFG", 0x288, 1, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_FMAP (0x00400 - 0x7FC) */
	/* TODO: max 256, display 4 for now */
	{"TRQ_SEL_FMAP", 0x400, 4, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_IND (0x00800) */
	{"IND_CTXT_DATA", 0x804, 8, 0, 0, 0, QDMA_MM_ST_MODE },
	{"IND_CTXT_MASK", 0x824, 8, 0, 0, 0, QDMA_MM_ST_MODE },
	{"IND_CTXT_CMD", 0x844, 1, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_C2H (0x00A00) */
	{"C2H_TIMER_CNT", 0xA00, 16, 0, 0, 0, QDMA_COMPLETION_MODE },
	{"C2H_CNT_THRESH", 0xA40, 16, 0, 0, 0, QDMA_COMPLETION_MODE },
	{"C2H_STAT_S_AXIS_C2H_ACCEPTED", 0xA88, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_S_AXIS_CMPT_ACCEPTED",
			0xA8C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_PKT_ACCEPTED",
			0xA90, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_AXIS_PKG_CMP", 0xA94, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_ACCEPTED", 0xA98, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_CMP", 0xA9C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WRQ_OUT", 0xAA0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WPL_REN_ACCEPTED", 0xAA4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_TOTAL_WRQ_LEN", 0xAA8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_TOTAL_WPL_LEN", 0xAAC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_BUF_SZ", 0xAB0, 16, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_ERR_STAT", 0xAF0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_ERR_MASK", 0xAF4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_STAT", 0xAF8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_MASK", 0xAFC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_ENABLE", 0xB00, 1, 0, 0, 0, QDMA_ST_MODE },
	{"GLBL_ERR_INT", 0xB04, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_PFCH_CFG", 0xB08, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INT_TIMER_TICK", 0xB0C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_DROP_ACCEPTED",
			0xB10, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_ERR_ACCEPTED",
			0xB14, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_REQ", 0xB18, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG", 0xB1C, 4, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH_ERR_CTXT", 0xB2C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FIRST_ERR_QID", 0xB30, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_IN", 0xB34, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_OUT", 0xB38, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_DRP", 0xB3C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_STAT_DESC_OUT", 0xB40, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_DSC_CRDT_SENT", 0xB44, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_FCH_DSC_RCVD", 0xB48, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_BYP_DSC_RCVD", 0xB4C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_CMPT_COAL_CFG", 0xB50, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_REQ", 0xB54, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_MM_REQ", 0xB58, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_ERR_INT_REQ", 0xB5C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_REQ", 0xB60, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_MSIX_ACK", 0xB64, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_MSIX_FAIL",
			0xB68, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_NO_MSIX", 0xB6C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_CTXT_INVAL", 0xB70,
			1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_MSIX_ACK", 0xB74, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_MSIX_FAIL", 0xB78, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_NO_MSIX", 0xB7C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_CTXT_INVAL", 0xB80, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WR_CMP", 0xB84, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG_4", 0xB88, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG_5", 0xB8C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH_QID", 0xB90, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH", 0xB94, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INT_DEBUG", 0xB98, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_IMM_ACCEPTED", 0xB9C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_MARKER_ACCEPTED", 0xBA0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DISABLE_CMP_ACCEPTED",
			0xBA4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_C2H_PAYLOAD_FIFO_CRDT_CNT",
			0xBA8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_DYN_REQ", 0xBAC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_DYN_MSIX", 0xBB0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DROP_LEN_MISMATCH", 0xBB4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DROP_DESC_RSP_LEN", 0xBB8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DROP_QID_FIFO_LEN", 0xBBC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DROP_PAYLOAD_CNT", 0xBC0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"QDMA_C2H_CMPT_FORMAT", 0xBC4, 7, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_PFCH_CACHE_DEPTH", 0xBE0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_CMPT_COAL_BUF_DEPTH", 0xBE4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_PFCH_CRDT", 0xBE8, 1, 0, 0, 0, QDMA_ST_MODE },

	/* QDMA_TRQ_SEL_H2C(0x00E00) Register Space*/
	{"H2C_ERR_STAT", 0xE00, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_ERR_MASK", 0xE04, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_FIRST_ERR_QID", 0xE08, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_DBG_REG", 0xE0C, 5, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_FATAL_ERR_EN", 0xE20, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_REQ_THROT", 0xE24, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_ALN_DBG_REG0", 0xE28, 1, 0, 0, 0, QDMA_ST_MODE },

	/* QDMA_TRQ_SEL_C2H_MM (0x1000) */
	{"C2H_MM_CONTROL", 0x1004, 3, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_STATUS", 0x1040, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_CMPL_DSC_CNT", 0x1048, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_CODE_EN_MASK", 0x1054, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_CODE", 0x1058, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_INFO", 0x105C, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_CTRL", 0x10C0, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_CY_CNT", 0x10C4, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_DATA_CNT", 0x10CC, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_DBG_INFO", 0x10E8, 2, 0, 0, 0, QDMA_MM_MODE },

	/* QDMA_TRQ_SEL_H2C_MM (0x1200)*/
	{"H2C_MM_CONTROL", 0x1204, 3, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_STATUS", 0x1240, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_CMPL_DSC_CNT", 0x1248, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_CODE_EN_MASK", 0x1254, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_CODE", 0x1258, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_INFO", 0x125C, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_CTRL", 0x12C0, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_CY_CNT", 0x12C4, 2, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_DATA_CNT", 0x12CC, 2, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_DBG_INFO", 0x12E8, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_REQ_THROT", 0x12EC, 1, 0, 0, 0, QDMA_MM_MODE },

	/* QDMA_PF_MAILBOX (0x2400) */
	{"FUNC_STATUS", 0x2400, 1, 0, 0, 0, QDMA_MAILBOX },
	{"FUNC_CMD",  0x2404, 1, 0, 0, 0, QDMA_MAILBOX },
	{"FUNC_INTR_VEC",  0x2408, 1, 0, 0, 0, QDMA_MAILBOX },
	{"TARGET_FUNC",  0x240C, 1, 0, 0, 0, QDMA_MAILBOX },
	{"INTR_CTRL",  0x2410, 1, 0, 0, 0, QDMA_MAILBOX },
	{"PF_ACK",  0x2420, 8, 0, 0, 0, QDMA_MAILBOX },
	{"FLR_CTRL_STATUS",  0x2500, 1, 0, 0, 0, QDMA_MAILBOX },
	{"MSG_IN",  0x2800, 32, 0, 0, 0, QDMA_MAILBOX },
	{"MSG_OUT",  0x2C00, 32, 0, 0, 0, QDMA_MAILBOX },

	{"", 0, 0, 0, 0, 0, 0 }
};


struct xreg_info qdma_cpm_config_regs[] = {

	/* QDMA_TRQ_SEL_GLBL1 (0x00000) */
	{"CFG_BLOCK_ID", 0x00, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_BUSDEV", 0x04, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_MAX_PL_SZ", 0x08, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_MAX_RDRQ_SZ", 0x0C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_SYS_ID", 0x10, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_MSI_EN", 0x14, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_DATA_WIDTH", 0x18, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_PCIE_CTRL", 0x1C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_AXI_USR_MAX_PL_SZ", 0x40, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_AXI_USR_MAX_RDRQ_SZ",
			0x44, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_MISC_CTRL", 0x4C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"CFG_SCRATCH_REG", 0x80, 8, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_SBE_MSK_A", 0xF0, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_SBE_STS_A", 0xF4, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_DBE_MSK_A", 0xF8, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_RAM_DBE_STS_A", 0xFC, 1, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_GLBL2 (0x00100) */
	{"GLBL2_ID", 0x100, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_BL_INT", 0x104, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_VF_BL_INT", 0x108, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_BL_EXT", 0x10C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PF_VF_BL_EXT", 0x110, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_INST", 0x114, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_QDMA", 0x118, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_CHNL_STRM", 0x11C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_QDMA_CAP", 0x120, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_PASID_CAP", 0x128, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_FUNC_RET", 0x12C, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_SYS_ID", 0x130, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_MISC_CAP", 0x134, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_PCIE_RQ", 0x1B8, 2, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_AXIMM_WR", 0x1C0, 2, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL2_DBG_AXIMM_RD", 0x1C8, 2, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_GLBL (0x00200) */
	{"GLBL_RNGSZ", 0x204, 16, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_ERR_STAT", 0x248, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_ERR_MASK", 0x24C, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_CFG", 0x250, 1, 0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_STS", 0x254, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_MSK", 0x258, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_ERR_LOG", 0x25C, 2,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_STS", 0x264, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_MSK", 0x268, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_TRQ_ERR_LOG", 0x26C, 1,  0, 0, 0, QDMA_MM_ST_MODE },
	{"GLBL_DSC_DBG_DAT", 0x270, 2,  0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_FMAP (0x00400 - 0x7FC) */
	/* TODO: max 256, display 4 for now */
	{"TRQ_SEL_FMAP", 0x400, 4, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_IND (0x00800) */
	{"IND_CTXT_DATA", 0x804, 4, 0, 0, 0, QDMA_MM_ST_MODE },
	{"IND_CTXT_MASK", 0x814, 4, 0, 0, 0, QDMA_MM_ST_MODE },
	{"IND_CTXT_CMD", 0x824, 1, 0, 0, 0, QDMA_MM_ST_MODE },

	/* QDMA_TRQ_SEL_C2H (0x00A00) */
	{"C2H_TIMER_CNT", 0xA00, 16, 0, 0, 0, QDMA_COMPLETION_MODE },
	{"C2H_CNT_THRESH", 0xA40, 16, 0, 0, 0, QDMA_COMPLETION_MODE },
	{"QDMA_C2H_QID2VEC_MAP_QID", 0xA80, 1,
			0, 0, 0, QDMA_MM_ST_MODE },
	{"QDMA_C2H_QID2VEC_MAP", 0xA84, 1,
			0, 0, 0, QDMA_MM_ST_MODE },
	{"C2H_STAT_S_AXIS_C2H_ACCEPTED", 0xA88, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_S_AXIS_CMPT_ACCEPTED",
			0xA8C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_PKT_ACCEPTED",
			0xA90, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_AXIS_PKG_CMP", 0xA94, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_ACCEPTED", 0xA98, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_CMP", 0xA9C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WRQ_OUT", 0xAA0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WPL_REN_ACCEPTED", 0xAA4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_TOTAL_WRQ_LEN", 0xAA8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_TOTAL_WPL_LEN", 0xAAC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_BUF_SZ", 0xAB0, 16, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_ERR_STAT", 0xAF0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_ERR_MASK", 0xAF4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_STAT", 0xAF8, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_MASK", 0xAFC, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FATAL_ERR_ENABLE", 0xB00, 1, 0, 0, 0, QDMA_ST_MODE },
	{"GLBL_ERR_INT", 0xB04, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_PFCH_CFG", 0xB08, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INT_TIMER_TICK", 0xB0C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_DROP_ACCEPTED",
			0xB10, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_RSP_ERR_ACCEPTED",
			0xB14, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DESC_REQ", 0xB18, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG", 0xB1C, 4, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH_ERR_CTXT", 0xB2C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_FIRST_ERR_QID", 0xB30, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_IN", 0xB34, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_OUT", 0xB38, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_CMPT_DRP", 0xB3C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_STAT_DESC_OUT", 0xB40, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_DSC_CRDT_SENT", 0xB44, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_FCH_DSC_RCVD", 0xB48, 1, 0, 0, 0, QDMA_ST_MODE },
	{"STAT_NUM_BYP_DSC_RCVD", 0xB4C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_CMPT_COAL_CFG", 0xB50, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_REQ", 0xB54, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_MM_REQ", 0xB58, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_ERR_INT_REQ", 0xB5C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_REQ", 0xB60, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_MSIX_ACK", 0xB64, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_MSIX_FAIL",
			0xB68, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_NO_MSIX", 0xB6C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_H2C_ERR_MM_CTXT_INVAL", 0xB70,
			1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_MSIX_ACK", 0xB74, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_MSIX_FAIL", 0xB78, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_NO_MSIX", 0xB7C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INTR_C2H_ST_CTXT_INVAL", 0xB80, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_WR_CMP", 0xB84, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG_4", 0xB88, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DEBUG_DMA_ENG_5", 0xB8C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH_QID", 0xB90, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_DBG_PFCH", 0xB94, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_INT_DEBUG", 0xB98, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_IMM_ACCEPTED", 0xB9C, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_MARKER_ACCEPTED", 0xBA0, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_STAT_DISABLE_CMP_ACCEPTED",
			0xBA4, 1, 0, 0, 0, QDMA_ST_MODE },
	{"C2H_C2H_PAYLOAD_FIFO_CRDT_CNT",
			0xBA8, 1, 0, 0, 0, QDMA_ST_MODE },

	/* QDMA_TRQ_SEL_H2C(0x00E00) Register Space*/
	{"H2C_ERR_STAT", 0xE00, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_ERR_MASK", 0xE04, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_FIRST_ERR_QID", 0xE08, 1, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_DBG_REG", 0xE0C, 5, 0, 0, 0, QDMA_ST_MODE },
	{"H2C_FATAL_ERR_EN", 0xE20, 1, 0, 0, 0, QDMA_ST_MODE },

	/* QDMA_TRQ_SEL_C2H_MM (0x1000) */
	{"C2H_MM_CONTROL", 0x1004, 3, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_STATUS", 0x1040, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_CMPL_DSC_CNT", 0x1048, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_CODE_EN_MASK", 0x1054, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_CODE", 0x1058, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_ERR_INFO", 0x105C, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_CTRL", 0x10C0, 1, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_CY_CNT", 0x10C4, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_PERF_MON_DATA_CNT", 0x10CC, 2, 0, 0, 0, QDMA_MM_MODE },
	{"C2H_MM_DBG_INFO", 0x10E8, 2, 0, 0, 0, QDMA_MM_MODE },

	/* QDMA_TRQ_SEL_H2C_MM (0x1200)*/
	{"H2C_MM_CONTROL", 0x1204, 3, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_STATUS", 0x1240, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_CMPL_DSC_CNT", 0x1248, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_CODE_EN_MASK", 0x1254, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_CODE", 0x1258, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_ERR_INFO", 0x125C, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_CTRL", 0x12C0, 1, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_CY_CNT", 0x12C4, 2, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_PERF_MON_DATA_CNT", 0x12CC, 2, 0, 0, 0, QDMA_MM_MODE },
	{"H2C_MM_DBG_INFO", 0x12E8, 1, 0, 0, 0, QDMA_MM_MODE },

	/* QDMA_PF_MAILBOX (0x2400) */
	{"FUNC_STATUS", 0x2400, 1, 0, 0, 0, QDMA_MAILBOX },
	{"FUNC_CMD",  0x2404, 1, 0, 0, 0, QDMA_MAILBOX },
	{"FUNC_INTR_VEC",  0x2408, 1, 0, 0, 0, QDMA_MAILBOX },
	{"TARGET_FUNC",  0x240C, 1, 0, 0, 0, QDMA_MAILBOX },
	{"INTR_CTRL",  0x2410, 1, 0, 0, 0, QDMA_MAILBOX },
	{"PF_ACK",  0x2420, 8, 0, 0, 0, QDMA_MAILBOX },
	{"FLR_CTRL_STATUS",  0x2500, 1, 0, 0, 0, QDMA_MAILBOX },
	{"MSG_IN",  0x2800, 32, 0, 0, 0, QDMA_MAILBOX },
	{"MSG_OUT",  0x2C00, 32, 0, 0, 0, QDMA_MAILBOX },

	{"", 0, 0, 0, 0, 0, 0 }
};


uint32_t qdma_reg_dump_buf_len(void)
{
	uint32_t length = ((sizeof(qdma_config_regs) /
			sizeof(qdma_config_regs[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE;
	return length;
}

struct qctx_entry sw_ctxt_entries[] = {
	{"PIDX", 0},
	{"IRQ Arm", 0},
	{"Function Id", 0},
	{"Queue Enable", 0},
	{"Fetch Credit Enable", 0},
	{"Write back/Intr Check", 0},
	{"Write back/Intr Interval", 0},
	{"Address Translation", 0},
	{"Fetch Max", 0},
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
	{"Interrupt Vector/Ring Index", 0},
	{"Interrupt Aggregation", 0},
};

struct qctx_entry hw_ctxt_entries[] = {
	{"CIDX", 0},
	{"Credits Consumed", 0},
	{"Descriptors Pending", 0},
	{"Queue Invalid No Desc Pending", 0},
	{"Eviction Pending", 0},
	{"Fetch Pending", 0},
};

struct qctx_entry credit_ctxt_entries[] = {
	{"Credit", 0},
};

struct qctx_entry cmpt_ctxt_entries[] = {
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
	{"Over Flow Check Disable", 0},
	{"Address Translation", 0},
	{"Interrupt Vector/Ring Index", 0},
	{"Interrupt Aggregation", 0},
};

struct qctx_entry c2h_pftch_ctxt_entries[] = {
	{"Bypass", 0},
	{"Buffer Size Index", 0},
	{"Port Id", 0},
	{"Error", 0},
	{"Prefetch Enable", 0},
	{"In Prefetch", 0},
	{"Software Credit", 0},
	{"Valid", 0},
};

unsigned int qdma_context_buf_len(
				char pfetch_valid, char cmpt_valid)
{
	int len = 0;

	len += (((sizeof(sw_ctxt_entries) /
			sizeof(sw_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

	len += (((sizeof(hw_ctxt_entries) /
		sizeof(hw_ctxt_entries[0])) + 1) *
		REG_DUMP_SIZE_PER_LINE);

	len += (((sizeof(credit_ctxt_entries) /
		sizeof(credit_ctxt_entries[0])) + 1) *
		REG_DUMP_SIZE_PER_LINE);

	if (cmpt_valid)
		len += (((sizeof(cmpt_ctxt_entries) /
			sizeof(cmpt_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

	if (pfetch_valid)
		len += (((sizeof(c2h_pftch_ctxt_entries) /
			sizeof(c2h_pftch_ctxt_entries[0])) + 1) *
			REG_DUMP_SIZE_PER_LINE);

	return len;
}

/*
 * qdma_acc_fill_sw_ctxt() - Helper function to fill sw context into structure
 *
 */
void qdma_acc_fill_sw_ctxt(struct qdma_descq_sw_ctxt *sw_ctxt)
{
	sw_ctxt_entries[0].value = sw_ctxt->pidx;
	sw_ctxt_entries[1].value = sw_ctxt->irq_arm;
	sw_ctxt_entries[2].value = sw_ctxt->fnc_id;
	sw_ctxt_entries[3].value = sw_ctxt->qen;
	sw_ctxt_entries[4].value = sw_ctxt->frcd_en;
	sw_ctxt_entries[5].value = sw_ctxt->wbi_chk;
	sw_ctxt_entries[6].value = sw_ctxt->wbi_intvl_en;
	sw_ctxt_entries[7].value = sw_ctxt->at;
	sw_ctxt_entries[8].value = sw_ctxt->fetch_max;
	sw_ctxt_entries[9].value = sw_ctxt->rngsz_idx;
	sw_ctxt_entries[10].value = sw_ctxt->desc_sz;
	sw_ctxt_entries[11].value = sw_ctxt->bypass;
	sw_ctxt_entries[12].value = sw_ctxt->mm_chn;
	sw_ctxt_entries[13].value = sw_ctxt->wbk_en;
	sw_ctxt_entries[14].value = sw_ctxt->irq_en;
	sw_ctxt_entries[15].value = sw_ctxt->port_id;
	sw_ctxt_entries[16].value = sw_ctxt->irq_no_last;
	sw_ctxt_entries[17].value = sw_ctxt->err;
	sw_ctxt_entries[18].value = sw_ctxt->err_wb_sent;
	sw_ctxt_entries[19].value = sw_ctxt->irq_req;
	sw_ctxt_entries[20].value = sw_ctxt->mrkr_dis;
	sw_ctxt_entries[21].value = sw_ctxt->is_mm;
	sw_ctxt_entries[22].value = sw_ctxt->ring_bs_addr & 0xFFFFFFFF;
	sw_ctxt_entries[23].value =
		(sw_ctxt->ring_bs_addr >> 32) & 0xFFFFFFFF;
	sw_ctxt_entries[24].value = sw_ctxt->vec;
	sw_ctxt_entries[25].value = sw_ctxt->intr_aggr;
}

/*
 * qdma_acc_fill_cmpt_ctxt() - Helper function to fill completion context
 *                         into structure
 *
 */
void qdma_acc_fill_cmpt_ctxt(struct qdma_descq_cmpt_ctxt *cmpt_ctxt)
{
	cmpt_ctxt_entries[0].value = cmpt_ctxt->en_stat_desc;
	cmpt_ctxt_entries[1].value = cmpt_ctxt->en_int;
	cmpt_ctxt_entries[2].value = cmpt_ctxt->trig_mode;
	cmpt_ctxt_entries[3].value = cmpt_ctxt->fnc_id;
	cmpt_ctxt_entries[4].value = cmpt_ctxt->counter_idx;
	cmpt_ctxt_entries[5].value = cmpt_ctxt->timer_idx;
	cmpt_ctxt_entries[6].value = cmpt_ctxt->in_st;
	cmpt_ctxt_entries[7].value = cmpt_ctxt->color;
	cmpt_ctxt_entries[8].value = cmpt_ctxt->ringsz_idx;
	cmpt_ctxt_entries[9].value = cmpt_ctxt->bs_addr & 0xFFFFFFFF;
	cmpt_ctxt_entries[10].value =
		(cmpt_ctxt->bs_addr >> 32) & 0xFFFFFFFF;
	cmpt_ctxt_entries[11].value = cmpt_ctxt->desc_sz;
	cmpt_ctxt_entries[12].value = cmpt_ctxt->pidx;
	cmpt_ctxt_entries[13].value = cmpt_ctxt->cidx;
	cmpt_ctxt_entries[14].value = cmpt_ctxt->valid;
	cmpt_ctxt_entries[15].value = cmpt_ctxt->err;
	cmpt_ctxt_entries[16].value = cmpt_ctxt->user_trig_pend;
	cmpt_ctxt_entries[17].value = cmpt_ctxt->timer_running;
	cmpt_ctxt_entries[18].value = cmpt_ctxt->full_upd;
	cmpt_ctxt_entries[19].value = cmpt_ctxt->ovf_chk_dis;
	cmpt_ctxt_entries[20].value = cmpt_ctxt->at;
	cmpt_ctxt_entries[21].value = cmpt_ctxt->vec;
	cmpt_ctxt_entries[22].value = cmpt_ctxt->int_aggr;
}

/*
 * qdma_acc_fill_hw_ctxt() - Helper function to fill HW context into structure
 *
 */
void qdma_acc_fill_hw_ctxt(struct qdma_descq_hw_ctxt *hw_ctxt)
{
	hw_ctxt_entries[0].value = hw_ctxt->cidx;
	hw_ctxt_entries[1].value = hw_ctxt->crd_use;
	hw_ctxt_entries[2].value = hw_ctxt->dsc_pend;
	hw_ctxt_entries[3].value = hw_ctxt->idl_stp_b;
	hw_ctxt_entries[4].value = hw_ctxt->evt_pnd;
	hw_ctxt_entries[5].value = hw_ctxt->fetch_pnd;
}

/*
 * qdma_acc_fill_credit_ctxt() - Helper function to fill Credit context
 *                           into structure
 *
 */
void qdma_acc_fill_credit_ctxt(struct qdma_descq_credit_ctxt *cr_ctxt)
{
	credit_ctxt_entries[0].value = cr_ctxt->credit;
}

/*
 * qdma_acc_fill_pfetch_ctxt() - Helper function to fill Prefetch context
 *                           into structure
 *
 */
void qdma_acc_fill_pfetch_ctxt(struct qdma_descq_prefetch_ctxt *pfetch_ctxt)
{
	c2h_pftch_ctxt_entries[0].value = pfetch_ctxt->bypass;
	c2h_pftch_ctxt_entries[1].value = pfetch_ctxt->bufsz_idx;
	c2h_pftch_ctxt_entries[2].value = pfetch_ctxt->port_id;
	c2h_pftch_ctxt_entries[3].value = pfetch_ctxt->err;
	c2h_pftch_ctxt_entries[4].value = pfetch_ctxt->pfch_en;
	c2h_pftch_ctxt_entries[5].value = pfetch_ctxt->pfch;
	c2h_pftch_ctxt_entries[6].value = pfetch_ctxt->sw_crdt;
	c2h_pftch_ctxt_entries[7].value = pfetch_ctxt->valid;
}


