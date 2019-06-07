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

#include "qdma_access.h"
#include "qdma_reg.h"
#include "qdma_reg_dump.h"
#include "qdma_platform.h"

#define QDMA_BAR_NUM                        6

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


/* qdma version info */
#define RTL_BASE_VERSION                        2
#define RTL_PATCH_VERSION                       3
#define VIVADO_RELEASE_2018_3               0
#define VIVADO_RELEASE_2019_1               1
#define EVEREST_SOFT_IP                     0
#define EVEREST_HARD_IP                     1

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
static void qdma_read_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, uint32_t *values);
static void qdma_write_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, const uint32_t *values);

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

	return -QDMA_BUSY_TIMEOUT_ERR;
}

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
		return -QDMA_CONTEXT_BUSY_TIMEOUT_ERR;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

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
		return -QDMA_CONTEXT_BUSY_TIMEOUT_ERR;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

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
		return -QDMA_CONTEXT_BUSY_TIMEOUT_ERR;
	}

	for (index = 0; index < cnt; index++, reg_addr += sizeof(uint32_t))
		data[index] = qdma_reg_read(dev_hndl, reg_addr);

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}

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

	/* check if the operation wen thorugh well */
	if (hw_monitor_reg(dev_hndl, QDMA_OFFSET_IND_CTXT_CMD,
			QDMA_IND_CTXT_CMD_BUSY_MASK, 0,
			QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US)) {
		qdma_reg_access_release(dev_hndl);
		return -QDMA_CONTEXT_BUSY_TIMEOUT_ERR;
	}

	qdma_reg_access_release(dev_hndl);

	return QDMA_SUCCESS;
}


static void qdma_read_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, uint32_t *values)
{
	uint32_t index, reg_addr;

	reg_addr = reg_offst + (idx * sizeof(uint32_t));
	for (index = 0; index < cnt; index++) {
		values[index] = qdma_reg_read(dev_hndl, reg_addr +
					      (index * sizeof(uint32_t)));
	}
}

static void qdma_write_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, const uint32_t *values)
{
	uint32_t index, reg_addr;

	for (index = idx; index < (idx + cnt); index++) {
		reg_addr = reg_offst + (index * sizeof(uint32_t));
		qdma_reg_write(dev_hndl, reg_addr, values[index - idx]);
	}
}

int qdma_fmap_write(void *dev_hndl, uint16_t func_id,
		   const struct qdma_fmap_cfg *config)
{
	uint32_t fmap[QDMA_FMAP_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl || !config)
		return -QDMA_INVALID_PARAM_ERR;

	fmap[num_words_count++] =
		FIELD_SET(QDMA_FMAP_CTXT_W0_QID_MASK, config->qbase);
	fmap[num_words_count++] =
		FIELD_SET(QDMA_FMAP_CTXT_W1_QID_MAX_MASK, config->qmax);

	return qdma_indirect_reg_write(dev_hndl, sel, func_id,
			fmap, num_words_count);
}

int qdma_fmap_read(void *dev_hndl, uint16_t func_id,
			 struct qdma_fmap_cfg *config)
{
	int rv = 0;
	uint32_t fmap[QDMA_FMAP_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl || !config)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, func_id,
			QDMA_FMAP_NUM_WORDS, fmap);
	if (rv < 0)
		return rv;

	config->qbase = FIELD_GET(QDMA_FMAP_CTXT_W0_QID_MASK, fmap[0]);
	config->qmax = FIELD_GET(QDMA_FMAP_CTXT_W1_QID_MAX_MASK, fmap[1]);

	return QDMA_SUCCESS;
}

int qdma_fmap_clear(void *dev_hndl, uint16_t func_id)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_FMAP;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, func_id);
}

int qdma_sw_context_write(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 const struct qdma_descq_sw_ctxt *ctxt)
{
	uint32_t sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	/* Input args check */
	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	sw_ctxt[num_words_count++] =
		FIELD_SET(QDMA_SW_CTXT_W0_PIDX, ctxt->pidx) |
		FIELD_SET(QDMA_SW_CTXT_W0_IRQ_ARM_MASK, ctxt->irq_arm) |
		FIELD_SET(QDMA_SW_CTXT_W0_FUNC_ID_MASK, ctxt->fnc_id);

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

	sw_ctxt[num_words_count++] = ctxt->ring_bs_addr & 0xffffffff;
	sw_ctxt[num_words_count++] = (ctxt->ring_bs_addr >> 32) & 0xffffffff;

	sw_ctxt[num_words_count++] =
		FIELD_SET(QDMA_SW_CTXT_W4_VEC_MASK, ctxt->vec) |
		FIELD_SET(QDMA_SW_CTXT_W4_INTR_AGGR_MASK, ctxt->intr_aggr);

	return qdma_indirect_reg_write(dev_hndl, sel, hw_qid,
			sw_ctxt, num_words_count);
}

int qdma_sw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_sw_ctxt *ctxt)
{
	int rv = 0;
	uint32_t sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_SW_CONTEXT_NUM_WORDS, sw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->pidx = FIELD_GET(QDMA_SW_CTXT_W0_PIDX, sw_ctxt[0]);
	ctxt->irq_arm = FIELD_GET(QDMA_SW_CTXT_W0_IRQ_ARM_MASK, sw_ctxt[0]);
	ctxt->fnc_id = FIELD_GET(QDMA_SW_CTXT_W0_FUNC_ID_MASK, sw_ctxt[0]);

	ctxt->qen = FIELD_GET(QDMA_SW_CTXT_W1_QEN_MASK, sw_ctxt[1]);
	ctxt->frcd_en = FIELD_GET(QDMA_SW_CTXT_W1_FCRD_EN_MASK, sw_ctxt[1]);
	ctxt->wbi_chk = FIELD_GET(QDMA_SW_CTXT_W1_WBI_CHK_MASK, sw_ctxt[1]);
	ctxt->wbi_intvl_en =
			FIELD_GET(QDMA_SW_CTXT_W1_WB_INT_EN_MASK, sw_ctxt[1]);
	ctxt->at = FIELD_GET(QDMA_SW_CTXT_W1_AT_MASK, sw_ctxt[1]);
	ctxt->fetch_max =
			FIELD_GET(QDMA_SW_CTXT_W1_FETCH_MAX_MASK, sw_ctxt[1]);
	ctxt->rngsz_idx =
			FIELD_GET(QDMA_SW_CTXT_W1_RNG_SZ_MASK, sw_ctxt[1]);
	ctxt->desc_sz = FIELD_GET(QDMA_SW_CTXT_W1_DSC_SZ_MASK, sw_ctxt[1]);
	ctxt->bypass = FIELD_GET(QDMA_SW_CTXT_W1_BYP_MASK, sw_ctxt[1]);
	ctxt->mm_chn = FIELD_GET(QDMA_SW_CTXT_W1_MM_CHN_MASK, sw_ctxt[1]);
	ctxt->wbk_en = FIELD_GET(QDMA_SW_CTXT_W1_WBK_EN_MASK, sw_ctxt[1]);
	ctxt->irq_en = FIELD_GET(QDMA_SW_CTXT_W1_IRQ_EN_MASK, sw_ctxt[1]);
	ctxt->port_id = FIELD_GET(QDMA_SW_CTXT_W1_PORT_ID_MASK, sw_ctxt[1]);
	ctxt->irq_no_last =
			FIELD_GET(QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK, sw_ctxt[1]);
	ctxt->err = FIELD_GET(QDMA_SW_CTXT_W1_ERR_MASK, sw_ctxt[1]);
	ctxt->err_wb_sent =
			FIELD_GET(QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK, sw_ctxt[1]);
	ctxt->irq_req = FIELD_GET(QDMA_SW_CTXT_W1_IRQ_REQ_MASK, sw_ctxt[1]);
	ctxt->mrkr_dis = FIELD_GET(QDMA_SW_CTXT_W1_MRKR_DIS_MASK, sw_ctxt[1]);
	ctxt->is_mm = FIELD_GET(QDMA_SW_CTXT_W1_IS_MM_MASK, sw_ctxt[1]);

	ctxt->ring_bs_addr = ((uint64_t)sw_ctxt[3] << 32) | (sw_ctxt[2]);

	ctxt->vec = FIELD_GET(QDMA_SW_CTXT_W4_VEC_MASK, sw_ctxt[4]);
	ctxt->intr_aggr = FIELD_GET(QDMA_SW_CTXT_W4_INTR_AGGR_MASK, sw_ctxt[4]);

	return QDMA_SUCCESS;
}

int qdma_sw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

int qdma_sw_context_invalidate(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ?
			QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

int qdma_pfetch_context_write(void *dev_hndl, uint16_t hw_qid,
		const struct qdma_descq_prefetch_ctxt *ctxt)
{
	uint32_t pfetch_ctxt[QDMA_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;
	uint16_t num_words_count = 0;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	sw_crdt_l =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK, ctxt->sw_crdt);
	sw_crdt_h =
		FIELD_GET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, ctxt->sw_crdt);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(QDMA_PFTCH_CTXT_W0_BYPASS_MASK, ctxt->bypass) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_BUF_SIZE_IDX_MASK,
				ctxt->bufsz_idx) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_PORT_ID_MASK, ctxt->port_id) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_ERR_MASK, ctxt->err) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_PFETCH_EN_MASK, ctxt->pfch_en) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_Q_IN_PFETCH_MASK, ctxt->pfch) |
		FIELD_SET(QDMA_PFTCH_CTXT_W0_SW_CRDT_L_MASK, sw_crdt_l);

	pfetch_ctxt[num_words_count++] =
		FIELD_SET(QDMA_PFTCH_CTXT_W1_SW_CRDT_H_MASK, sw_crdt_h) |
		FIELD_SET(QDMA_PFTCH_CTXT_W1_VALID_MASK, ctxt->valid);

	return qdma_indirect_reg_write(dev_hndl, sel, hw_qid,
			pfetch_ctxt, num_words_count);
}

int qdma_pfetch_context_read(void *dev_hndl, uint16_t hw_qid,
		struct qdma_descq_prefetch_ctxt *ctxt)
{
	int rv = 0;
	uint32_t pfetch_ctxt[QDMA_PFETCH_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;
	uint32_t sw_crdt_l, sw_crdt_h;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

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
	ctxt->err = FIELD_GET(QDMA_PFTCH_CTXT_W0_ERR_MASK, pfetch_ctxt[0]);
	ctxt->pfch_en =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_PFETCH_EN_MASK, pfetch_ctxt[0]);
	ctxt->pfch =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_Q_IN_PFETCH_MASK, pfetch_ctxt[0]);
	sw_crdt_l =
		FIELD_GET(QDMA_PFTCH_CTXT_W0_SW_CRDT_L_MASK, pfetch_ctxt[0]);

	sw_crdt_h =
		FIELD_GET(QDMA_PFTCH_CTXT_W1_SW_CRDT_H_MASK, pfetch_ctxt[1]);
	ctxt->valid =
		FIELD_GET(QDMA_PFTCH_CTXT_W1_VALID_MASK, pfetch_ctxt[1]);

	ctxt->sw_crdt =
		FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK, sw_crdt_l) |
		FIELD_SET(QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK, sw_crdt_h);

	return QDMA_SUCCESS;
}

int qdma_pfetch_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

int qdma_pfetch_context_invalidate(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_PFTCH;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

int qdma_cmpt_context_write(void *dev_hndl, uint16_t hw_qid,
			   const struct qdma_descq_cmpt_ctxt *ctxt)
{
	uint32_t cmpt_ctxt[QDMA_CMPT_CONTEXT_NUM_WORDS] = {0};
	uint16_t num_words_count = 0;
	uint32_t baddr_l, baddr_h, pidx_l, pidx_h;
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	/* Input args check */
	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	if (ctxt->trig_mode > QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR)
		return QDMA_INVALID_PARAM_ERR;

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

int qdma_cmpt_context_read(void *dev_hndl, uint16_t hw_qid,
			   struct qdma_descq_cmpt_ctxt *ctxt)
{
	int rv = 0;
	uint32_t cmpt_ctxt[QDMA_CMPT_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;
	uint32_t baddr_l, baddr_h, pidx_l, pidx_h;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_CMPT_CONTEXT_NUM_WORDS, cmpt_ctxt);
	if (rv < 0)
		return rv;

	ctxt->en_stat_desc =
		FIELD_GET(QDMA_COMPL_CTXT_W0_EN_STAT_DESC_MASK, cmpt_ctxt[0]);
	ctxt->en_int = FIELD_GET(QDMA_COMPL_CTXT_W0_EN_INT_MASK, cmpt_ctxt[0]);
	ctxt->trig_mode =
		FIELD_GET(QDMA_COMPL_CTXT_W0_TRIG_MODE_MASK, cmpt_ctxt[0]);
	ctxt->fnc_id = FIELD_GET(QDMA_COMPL_CTXT_W0_FNC_ID_MASK, cmpt_ctxt[0]);
	ctxt->counter_idx =
		FIELD_GET(QDMA_COMPL_CTXT_W0_COUNTER_IDX_MASK, cmpt_ctxt[0]);
	ctxt->timer_idx =
		FIELD_GET(QDMA_COMPL_CTXT_W0_TIMER_IDX_MASK, cmpt_ctxt[0]);
	ctxt->in_st = FIELD_GET(QDMA_COMPL_CTXT_W0_INT_ST_MASK, cmpt_ctxt[0]);
	ctxt->color = FIELD_GET(QDMA_COMPL_CTXT_W0_COLOR_MASK, cmpt_ctxt[0]);
	ctxt->ringsz_idx =
		FIELD_GET(QDMA_COMPL_CTXT_W0_RING_SZ_MASK, cmpt_ctxt[0]);

	baddr_l = FIELD_GET(QDMA_COMPL_CTXT_W1_BADDR_64_L_MASK, cmpt_ctxt[1]);

	baddr_h = FIELD_GET(QDMA_COMPL_CTXT_W2_BADDR_64_H_MASK, cmpt_ctxt[2]);
	ctxt->desc_sz =
		FIELD_GET(QDMA_COMPL_CTXT_W2_DESC_SIZE_MASK, cmpt_ctxt[2]);
	pidx_l = FIELD_GET(QDMA_COMPL_CTXT_W2_PIDX_L_MASK, cmpt_ctxt[2]);

	pidx_h = FIELD_GET(QDMA_COMPL_CTXT_W3_PIDX_H_MASK, cmpt_ctxt[3]);
	ctxt->cidx = FIELD_GET(QDMA_COMPL_CTXT_W3_CIDX_MASK, cmpt_ctxt[3]);
	ctxt->valid = FIELD_GET(QDMA_COMPL_CTXT_W3_VALID_MASK, cmpt_ctxt[3]);
	ctxt->err = FIELD_GET(QDMA_COMPL_CTXT_W3_ERR_MASK, cmpt_ctxt[3]);
	ctxt->user_trig_pend =
		FIELD_GET(QDMA_COMPL_CTXT_W3_USR_TRG_PND_MASK, cmpt_ctxt[3]);

	ctxt->timer_running =
		FIELD_GET(QDMA_COMPL_CTXT_W4_TMR_RUN_MASK, cmpt_ctxt[4]);
	ctxt->full_upd =
		FIELD_GET(QDMA_COMPL_CTXT_W4_FULL_UPDT_MASK, cmpt_ctxt[4]);
	ctxt->ovf_chk_dis =
		FIELD_GET(QDMA_COMPL_CTXT_W4_OVF_CHK_DIS_MASK, cmpt_ctxt[4]);
	ctxt->at = FIELD_GET(QDMA_COMPL_CTXT_W4_AT_MASK, cmpt_ctxt[4]);
	ctxt->vec = FIELD_GET(QDMA_COMPL_CTXT_W4_INTR_VEC_MASK, cmpt_ctxt[4]);
	ctxt->int_aggr =
		FIELD_GET(QDMA_COMPL_CTXT_W4_INTR_AGGR_MASK, cmpt_ctxt[4]);

	ctxt->bs_addr =
		FIELD_SET(QDMA_COMPL_CTXT_BADDR_GET_L_MASK, baddr_l) |
		FIELD_SET(QDMA_COMPL_CTXT_BADDR_GET_H_MASK, (uint64_t)baddr_h);

	ctxt->pidx =
		FIELD_SET(QDMA_COMPL_CTXT_PIDX_GET_L_MASK, pidx_l) |
		FIELD_SET(QDMA_COMPL_CTXT_PIDX_GET_H_MASK, pidx_h);

	return QDMA_SUCCESS;
}

int qdma_cmpt_context_clear(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

int qdma_cmpt_context_invalidate(void *dev_hndl, uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_CMPT;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

int qdma_hw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid, struct qdma_descq_hw_ctxt *ctxt)
{
	int rv = 0;
	uint32_t hw_ctxt[QDMA_HW_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_HW_CONTEXT_NUM_WORDS, hw_ctxt);
	if (rv < 0)
		return rv;

	ctxt->cidx = FIELD_GET(QDMA_HW_CTXT_W0_CIDX_MASK, hw_ctxt[0]);
	ctxt->crd_use = FIELD_GET(QDMA_HW_CTXT_W0_CRD_USE_MASK, hw_ctxt[0]);

	ctxt->dsc_pend = FIELD_GET(QDMA_HW_CTXT_W1_DSC_PND_MASK, hw_ctxt[1]);
	ctxt->idl_stp_b = FIELD_GET(QDMA_HW_CTXT_W1_IDL_STP_B_MASK, hw_ctxt[1]);
	ctxt->evt_pnd = FIELD_GET(QDMA_HW_CTXT_W1_EVENT_PEND_MASK, hw_ctxt[1]);
	ctxt->fetch_pnd =
		FIELD_GET(QDMA_HW_CTXT_W1_FETCH_PEND_MASK, hw_ctxt[1]);

	return QDMA_SUCCESS;
}

int qdma_hw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

int qdma_hw_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_HW_C2H :
			QDMA_CTXT_SEL_HW_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

int qdma_credit_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_credit_ctxt *ctxt)
{
	int rv = 0;
	uint32_t cr_ctxt[QDMA_CR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, hw_qid,
			QDMA_CR_CONTEXT_NUM_WORDS, cr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->credit = FIELD_GET(QDMA_CR_CTXT_W0_CREDT_MASK, cr_ctxt[0]);

	return QDMA_SUCCESS;
}

int qdma_credit_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, hw_qid);
}

int qdma_credit_context_invalidate(void *dev_hndl, uint8_t c2h,
				   uint16_t hw_qid)
{
	enum ind_ctxt_cmd_sel sel = c2h ? QDMA_CTXT_SEL_CR_C2H :
			QDMA_CTXT_SEL_CR_H2C;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, hw_qid);
}

int qdma_indirect_intr_context_write(void *dev_hndl, uint16_t ring_index,
		const struct qdma_indirect_intr_ctxt *ctxt)
{
	uint32_t intr_ctxt[QDMA_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint32_t baddr_l, baddr_m, baddr_h;
	uint16_t num_words_count = 0;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

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

int qdma_indirect_intr_context_read(void *dev_hndl, uint16_t ring_index,
				   struct qdma_indirect_intr_ctxt *ctxt)
{
	int rv = 0;
	uint32_t intr_ctxt[QDMA_IND_INTR_CONTEXT_NUM_WORDS] = {0};
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;
	uint64_t baddr_l, baddr_m, baddr_h;

	if (!dev_hndl || !ctxt)
		return -QDMA_INVALID_PARAM_ERR;

	rv = qdma_indirect_reg_read(dev_hndl, sel, ring_index,
			QDMA_IND_INTR_CONTEXT_NUM_WORDS, intr_ctxt);
	if (rv < 0)
		return rv;

	ctxt->valid = FIELD_GET(QDMA_INTR_CTXT_W0_VALID_MASK, intr_ctxt[0]);
	ctxt->vec = FIELD_GET(QDMA_INTR_CTXT_W0_VEC_ID_MASK, intr_ctxt[0]);
	ctxt->int_st = FIELD_GET(QDMA_INTR_CTXT_W0_INT_ST_MASK, intr_ctxt[0]);
	ctxt->color = FIELD_GET(QDMA_INTR_CTXT_W0_COLOR_MASK, intr_ctxt[0]);
	baddr_l = FIELD_GET(QDMA_INTR_CTXT_W0_BADDR_64_MASK, intr_ctxt[0]);

	baddr_m = FIELD_GET(QDMA_INTR_CTXT_W1_BADDR_64_MASK, intr_ctxt[1]);

	baddr_h = FIELD_GET(QDMA_INTR_CTXT_W2_BADDR_64_MASK, intr_ctxt[2]);
	ctxt->page_size =
		FIELD_GET(QDMA_INTR_CTXT_W2_PAGE_SIZE_MASK, intr_ctxt[2]);
	ctxt->pidx = FIELD_GET(QDMA_INTR_CTXT_W2_PIDX_MASK, intr_ctxt[2]);
	ctxt->at = FIELD_GET(QDMA_INTR_CTXT_W2_AT_MASK, intr_ctxt[2]);

	ctxt->baddr_4k =
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_L_MASK, baddr_l) |
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_M_MASK, baddr_m) |
		FIELD_SET(QDMA_INTR_CTXT_BADDR_GET_H_MASK, baddr_h);

	return QDMA_SUCCESS;
}

int qdma_indirect_intr_context_clear(void *dev_hndl, uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_clear(dev_hndl, sel, ring_index);
}

int qdma_indirect_intr_context_invalidate(void *dev_hndl,
					  uint16_t ring_index)
{
	enum ind_ctxt_cmd_sel sel = QDMA_CTXT_SEL_INT_COAL;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	return qdma_indirect_reg_invalidate(dev_hndl, sel, ring_index);
}

int qdma_set_default_global_csr(void *dev_hndl)
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

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

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

int qdma_set_global_ring_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		const uint32_t *glbl_rng_sz)
{
	if (!dev_hndl || !glbl_rng_sz || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_RING_SIZES)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_write_csr_values(dev_hndl, QDMA_OFFSET_GLBL_RNG_SZ, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

int qdma_get_global_ring_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_rng_sz)
{
	if (!dev_hndl || !glbl_rng_sz || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_RING_SIZES)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_read_csr_values(dev_hndl, QDMA_OFFSET_GLBL_RNG_SZ, index, count,
			glbl_rng_sz);

	return QDMA_SUCCESS;
}

int qdma_set_global_timer_count(void *dev_hndl, uint8_t index, uint8_t count,
		const uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl || !glbl_tmr_cnt || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_TIMERS)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_TIMER_CNT,
				index, count, glbl_tmr_cnt);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_get_global_timer_count(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_tmr_cnt)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_tmr_cnt || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_TIMERS)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_read_csr_values(dev_hndl, QDMA_OFFSET_C2H_TIMER_CNT, index,
				count, glbl_tmr_cnt);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_set_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_cnt_th || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_COUNTERS)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_CNT_TH, index,
				count, glbl_cnt_th);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_get_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, uint32_t *glbl_cnt_th)
{
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl || !glbl_cnt_th || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_COUNTERS)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en)
		qdma_read_csr_values(dev_hndl, QDMA_OFFSET_C2H_CNT_TH, index,
				count, glbl_cnt_th);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_set_global_buffer_sizes(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes *dev_cap = NULL;

	if (!dev_hndl || !glbl_buf_sz || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_BUFFER_SIZES)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en)
		qdma_write_csr_values(dev_hndl, QDMA_OFFSET_C2H_BUF_SZ, index,
				count, glbl_buf_sz);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_get_global_buffer_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_buf_sz)
{
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl || !glbl_buf_sz || !count)
		return -QDMA_INVALID_PARAM_ERR;

	if ((index + count) > QDMA_NUM_C2H_BUFFER_SIZES)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en)
		qdma_read_csr_values(dev_hndl, QDMA_OFFSET_C2H_BUF_SZ, index,
				count, glbl_buf_sz);
	else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_set_global_writeback_interval(void *dev_hndl,
		enum qdma_wrb_interval wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	if (wb_int >=  QDMA_NUM_WRB_INTERVALS)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG);
		reg_val |= FIELD_SET(QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK, wb_int);

		qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG, reg_val);
	} else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_get_global_writeback_interval(void *dev_hndl,
		enum qdma_wrb_interval *wb_int)
{
	uint32_t reg_val;
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->st_en || dev_cap->mm_cmpt_en) {
		reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_DSC_CFG);
		*wb_int = (enum qdma_wrb_interval)FIELD_GET(
				QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK, reg_val);
	} else
		return -QDMA_FEATURE_NOT_SUPPORTED;

	return QDMA_SUCCESS;
}

int qdma_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info)
{
	uint32_t reg_addr = 0;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	if (!is_vf) {
		reg_addr = (is_c2h) ?  QDMA_OFFSET_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_OFFSET_DMAP_SEL_H2C_DSC_PIDX;
	} else {
		reg_addr = (is_c2h) ?  QDMA_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX :
			QDMA_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX;
	}

	reg_addr += (qid * QDMA_PIDX_STEP);

	qdma_reg_write(dev_hndl, reg_addr, *((const uint32_t *)reg_info));

	return QDMA_SUCCESS;
}

int qdma_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
		QDMA_OFFSET_DMAP_SEL_CMPT_CIDX;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_addr += (qid * QDMA_CMPT_CIDX_STEP);

	qdma_reg_write(dev_hndl, reg_addr, *((const uint32_t *)reg_info));

	return QDMA_SUCCESS;
}

int qdma_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_intr_cidx_reg_info *reg_info)
{
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_INT_CIDX :
		QDMA_OFFSET_DMAP_SEL_INT_CIDX;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_addr += qid * QDMA_INT_CIDX_STEP;

	qdma_reg_write(dev_hndl, reg_addr, *((const uint32_t *)reg_info));

	return QDMA_SUCCESS;
}

int qdma_queue_cmpt_cidx_read(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
			QDMA_OFFSET_DMAP_SEL_CMPT_CIDX;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_addr += qid * QDMA_CMPT_CIDX_STEP;

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	reg_info->wrb_cidx =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK, reg_val);
	reg_info->counter_idx =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK, reg_val);
	reg_info->wrb_en =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK, reg_val);
	reg_info->irq_en = FIELD_GET(QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK, reg_val);
	reg_info->timer_idx =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK, reg_val);
	reg_info->trig_mode =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK, reg_val);

	return QDMA_SUCCESS;
}

int qdma_mm_channel_enable(void *dev_hndl, uint8_t channel, uint8_t is_c2h)
{
	uint32_t reg_addr = (is_c2h) ?  QDMA_OFFSET_C2H_MM_CONTROL :
			QDMA_OFFSET_H2C_MM_CONTROL;
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->mm_en) {
		qdma_reg_write(dev_hndl,
				reg_addr + (channel * QDMA_MM_CONTROL_STEP),
				QDMA_MM_CONTROL_RUN);
	}

	return QDMA_SUCCESS;
}

int qdma_mm_channel_disable(void *dev_hndl, uint8_t channel, uint8_t is_c2h)
{
	uint32_t reg_addr = (is_c2h) ? QDMA_OFFSET_C2H_MM_CONTROL :
			QDMA_OFFSET_H2C_MM_CONTROL;
	struct qdma_dev_attributes *dev_cap;



	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	if (dev_cap->mm_en) {
		qdma_reg_write(dev_hndl,
				reg_addr + (channel * QDMA_MM_CONTROL_STEP),
				0);
	}

	return QDMA_SUCCESS;
}

int qdma_initiate_flr(void *dev_hndl, uint8_t is_vf)
{
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	qdma_reg_write(dev_hndl, reg_addr, 1);

	return QDMA_SUCCESS;
}

int qdma_is_flr_done(void *dev_hndl, uint8_t is_vf, uint8_t *done)
{
	int rv;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

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

int qdma_is_config_bar(void *dev_hndl, uint8_t is_vf)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_CONFIG_BAR_ID :
			QDMA_OFFSET_CONFIG_BLOCK_ID;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	if (FIELD_GET(QDMA_CONFIG_BLOCK_ID_MASK, reg_val) != QDMA_MAGIC_NUMBER)
		return -QDMA_INVALID_CONFIG_BAR;

	return QDMA_SUCCESS;
}

int qdma_get_user_bar(void *dev_hndl, uint8_t is_vf, uint8_t *user_bar)
{
	uint8_t bar_found = 0;
	uint8_t bar_idx = 0;
	uint32_t func_id = 0;
	uint32_t user_bar_id = 0;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_USER_BAR_ID :
			QDMA_OFFSET_GLBL2_PF_BARLITE_EXT;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

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
		return -QDMA_BAR_NOT_FOUND;
	}

	return QDMA_SUCCESS;
}

int qdma_get_function_number(void *dev_hndl, uint8_t *func_id)
{
	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	*func_id = (uint8_t)qdma_reg_read(dev_hndl,
			QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET);

	return QDMA_SUCCESS;
}

int qdma_get_version(void *dev_hndl, uint8_t is_vf,
		struct qdma_hw_version_info *version_info)
{
	uint32_t rtl_version, vivado_release_id, everest_ip;
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_VERSION :
			QDMA_OFFSET_GLBL2_MISC_CAP;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	if (!is_vf) {
		rtl_version = FIELD_GET(QDMA_GLBL2_RTL_VERSION_MASK, reg_val);
		vivado_release_id =
			FIELD_GET(QDMA_GLBL2_VIVADO_RELEASE_MASK, reg_val);
		everest_ip = FIELD_GET(QDMA_GLBL2_EVEREST_IP_MASK, reg_val);
	} else {
		rtl_version =
			FIELD_GET(QDMA_GLBL2_VF_RTL_VERSION_MASK, reg_val);
		vivado_release_id =
			FIELD_GET(QDMA_GLBL2_VF_VIVADO_RELEASE_MASK, reg_val);
		everest_ip = FIELD_GET(QDMA_GLBL2_VF_EVEREST_IP_MASK, reg_val);
	}

	switch (rtl_version) {
	case RTL_PATCH_VERSION:
		version_info->rtl_version = QDMA_RTL_PATCH;
		break;
	default:
		version_info->rtl_version = QDMA_RTL_BASE;
		break;
	}

	switch (vivado_release_id) {
	case VIVADO_RELEASE_2018_3:
		version_info->vivado_release = QDMA_VIVADO_2018_3;
		break;
	default:
		version_info->vivado_release = QDMA_VIVADO_2019_1;
		break;
	}

	if (everest_ip)
		version_info->everest_ip = QDMA_HARD_IP;
	else
		version_info->everest_ip = QDMA_SOFT_IP;

	return QDMA_SUCCESS;
}

const char *qdma_get_rtl_version(enum qdma_rtl_version rtl_version)
{
	switch (rtl_version) {
	case QDMA_RTL_PATCH:
		return "RTL Patch";
	case QDMA_RTL_BASE:
		return "RTL Base";
	default:
		return NULL;
	}
}

const char *qdma_get_everest_ip(enum qdma_everest_ip everest_ip)
{
	switch (everest_ip) {
	case QDMA_SOFT_IP:
		return "Soft IP";
	case QDMA_HARD_IP:
		return "Hard IP";
	default:
		return NULL;
	}
}

const char *qdma_get_vivado_release_id(
				enum qdma_vivado_release_id vivado_release_id)
{
	switch (vivado_release_id) {
	case QDMA_VIVADO_2018_3:
		return "vivado 2018.3";
	case QDMA_VIVADO_2019_1:
		return "vivado 2019.1";
	default:
		return NULL;
	}
}

int qdma_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info)
{
	uint8_t count = 0;
	uint32_t reg_val = 0;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

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
	// TODO : Register not yet defined for this. Hard coding it to 1.
	dev_info->mm_channel_max = 1;

	return QDMA_SUCCESS;
}

int qdma_error_interrupt_setup(void *dev_hndl, uint16_t func_id,
		uint8_t err_intr_index)
{
	uint32_t reg_val = 0;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val =
		FIELD_SET(QDMA_GLBL_ERR_FUNC_MASK, func_id) |
		FIELD_SET(QDMA_GLBL_ERR_VEC_MASK, err_intr_index);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return 0;
}

int qdma_error_interrupt_rearm(void *dev_hndl)
{
	uint32_t reg_val = 0;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT);
	reg_val |= FIELD_SET(QDMA_GLBL_ERR_ARM_MASK, 1);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return 0;
}

int qdma_error_enable(void *dev_hndl, enum qdma_error_idx err_idx)
{
	uint32_t idx = 0, i = 0;
	uint32_t reg_val = 0;
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	if (err_idx > QDMA_ERRS_ALL)
		return -QDMA_INVALID_PARAM_ERR;

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
				return 0;
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

	return 0;
}

const char *qdma_get_error_name(enum qdma_error_idx err_idx)
{
	if (err_idx >= QDMA_ERRS_ALL)
		return NULL;

	return err_info[err_idx].err_name;
}

int qdma_error_process(void *dev_hndl)
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

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

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

	return 0;
}


static int dump_reg(char *buf, int buf_sz, unsigned int raddr,
		const char *rname, unsigned int rval)
{
	int len = 0;

	/* length of the line should not exceed 80 chars, so, checking
	 * for min 80 chars. If below print pattern is changed, check for
	 * new the buffer size requirement
	 */
	if (buf_sz < DEBGFS_LINE_SZ)
		return -1;

	len += QDMA_SNPRINTF(buf + len, DEBGFS_LINE_SZ,
					"[%#7x] %-47s %#-10x %u\n", raddr,
					rname, rval, rval);

	return len;
}


int qdma_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen)
{
	unsigned int i = 0, j = 0;
	struct xreg_info *reg_info;
	uint32_t num_regs =
		sizeof(qdma_config_regs) / sizeof((qdma_config_regs)[0]);
	uint32_t len = 0, val = 0;
	int rv = 0;
	char name[DEBGFS_GEN_NAME_SZ] = "";
	struct qdma_dev_attributes *dev_cap;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	if (buflen < qdma_reg_dump_buf_len())
		return -ERROR_INSUFFICIENT_BUFFER_SPACE;

	//TODO : VF register space to be added later.
	if (is_vf)
		return -QDMA_FEATURE_NOT_SUPPORTED;

	qdma_get_device_attr(dev_hndl, &dev_cap);

	reg_info = qdma_config_regs;

	for (i = 0; i < num_regs - 1; i++) {
		if ((GET_CAPABILITY_MASK(dev_cap->mm_en, dev_cap->st_en,
				dev_cap->mm_cmpt_en,
				dev_cap->mailbox_en) & reg_info[i].mode) == 0)
			continue;

		if (reg_info[i].repeat) {
			for (j = 0; j < reg_info[i].repeat; j++) {
				QDMA_SNPRINTF(name, DEBGFS_GEN_NAME_SZ,
						"%s_%d", reg_info[i].name, j);
				val = 0;
				val = qdma_reg_read(dev_hndl,
						(reg_info[i].addr + (j * 4)));
				rv = dump_reg(buf + len, buflen - len,
						(reg_info[i].addr + (j * 4)),
						name, val);
				if (rv < 0)
					return -ERROR_INSUFFICIENT_BUFFER_SPACE;
				len += rv;
			}
		} else {
			val = 0;
			val = qdma_reg_read(dev_hndl, reg_info[i].addr);
			rv = dump_reg(buf + len, buflen - len,
					reg_info[i].addr,
					reg_info[i].name, val);
			if (rv < 0)
				return -ERROR_INSUFFICIENT_BUFFER_SPACE;
			len += rv;
		}
	}

	return len;
}

int qdma_is_legacy_interrupt_pending(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	if (FIELD_GET(QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK, reg_val))
		return QDMA_SUCCESS;

	return -QDMA_NO_PENDING_LEGACY_INTERRUPT;
}


int qdma_clear_pending_legacy_intrrupt(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	reg_val |= FIELD_SET(QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK, 1);
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG, reg_val);

	return QDMA_SUCCESS;
}

int qdma_disable_legacy_interrupt(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	reg_val |= FIELD_SET(QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK, 0);
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG, reg_val);

	return QDMA_SUCCESS;
}

int qdma_enable_legacy_interrupt(void *dev_hndl)
{
	uint32_t reg_val;

	if (!dev_hndl)
		return -QDMA_INVALID_PARAM_ERR;

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG);
	reg_val |= FIELD_SET(QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK, 1);
	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_INTERRUPT_CFG, reg_val);

	return QDMA_SUCCESS;
}
