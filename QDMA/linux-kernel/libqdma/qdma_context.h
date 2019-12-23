/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2019,  Xilinx, Inc.
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

#ifndef __LIBQDMA_CONTEXT_H__
#define __LIBQDMA_CONTEXT_H__
/**
 * @file
 * @brief This file contains the declarations for qdma context handlers
 *
 */
#include "xdev.h"
#include "qdma_mbox.h"

#define CTXT_ENTRY_NAME_SZ        64

struct qctxt_entry {
	char name[CTXT_ENTRY_NAME_SZ];
	u32 value;
};

enum qdma_q_cntxt {
	QDMA_SW_CNTXT,
	QDMA_HW_CNTXT,
	QDMA_CMPT_CNTXT,
	QDMA_PFETCH_CNTXT,
	QDMA_CR_CNTXT,
	QDMA_INTR_CNTXT
};

/*****************************************************************************/
/**
 * qdma_fill_sw_ctxt() - Filling up the sw_ctxt array
 *
 * @param[in]	sw_ctxt: pointer to software context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_sw_ctxt(struct qdma_descq_sw_ctxt *sw_ctxt, u8 ind_mode);

/*****************************************************************************/
/**
 * qdma_fill_cmpt_ctxt() - Filling up the cmpt_ctxt array
 *
 * @param[in]	cmpt_ctxt: pointer to cmpt context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_cmpt_ctxt(struct qdma_descq_cmpt_ctxt *cmpt_ctxt, u8 ind_mode);

/*****************************************************************************/
/**
 * qdma_fill_hw_ctxt() - Filling up the hw_ctxt array
 *
 * @param[in]	hw_ctxt: pointer to hardware context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_hw_ctxt(struct qdma_descq_hw_ctxt *hw_ctxt);

/*****************************************************************************/
/**
 * qdma_fill_credit_ctxt() - Filling up the credit_ctxt array
 *
 * @param[in]	cr_ctxt: pointer to credit context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_credit_ctxt(struct qdma_descq_credit_ctxt *cr_ctxt);

/*****************************************************************************/
/**
 * qdma_fill_pfetch_ctxt() - Filling up the c2h_pftch_ctxt array
 *
 * @param[in]	pfetch_ctxt: pointer to prefetch context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_pfetch_ctxt(struct qdma_descq_prefetch_ctxt *pfetch_ctxt);

/*****************************************************************************/
/**
 * qdma_fill_intr_ctxt() - Filling up the ind_intr_ctxt array
 *
 * @param[in]	intr_ctxt: pointer to interrupt context structure
 *
 * @return	None
 *****************************************************************************/
void qdma_fill_intr_ctxt(struct qdma_indirect_intr_ctxt *intr_ctxt);

/*****************************************************************************/
/**
 * parse_ctxt_to_buf() - parses queue context to human readable format
 *
 * @param[in]	cntxt: context to parse. An enum value
 * @param[in]	version_info: HW version information
 * @param[out]	buf: buffer to write parsed context
 * @param[in]	buflen: buffer len
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int qdma_parse_ctxt_to_buf(enum qdma_q_cntxt cntxt,
		struct qdma_hw_version_info *version_info,
		char *buf, int buflen);

/*****************************************************************************/
/**
 * qdma_intr_context_setup() - handler to set the qdma interrupt context
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_intr_context_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_prog_intr_context() -
 *			handler to program the qdma interrupt context for
 *			VF from PF
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	ictxt:		interrupt context
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/

int qdma_prog_intr_context(struct xlnx_dma_dev *xdev,
		struct mbox_msg_intr_ctxt *ictxt);

/*****************************************************************************/
/**
 * qdma_descq_context_setup() - handler to set the qdma sw descriptor context
 *
 * @param[in]	descq:		pointer to qdma_descq
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_setup(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * qdma_descq_context_clear() - handler to clear the qdma sw descriptor context
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:		indicates whether the h2c or c2h direction
 * @param[in]	mm_cmpt_en:	indicates whether cmpt is enabled for mm or not
 * @param[in]	clr:	flag to indicate whether to clear the context or not
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_clear(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, u8 type, bool clr);

/*****************************************************************************/
/**
 * qdma_descq_context_read() - handler to read the queue context
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:		indicates whether the h2c or c2h direction
 * @param[in]	mm_cmpt_en:	indicates whether cmpt is enabled for mm or not
 * @param[out]	ctxt:	pointer to context data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			bool st, u8 type, struct qdma_descq_context *context);

/*****************************************************************************/
/**
 * qdma_intr_context_read() - handler to read the interrupt context
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	ring_index:	interrupt ring index
 * @param[in]	ctxt_sz:	context size
 * @param[out]	context:	pointer to interrupt context*
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_intr_context_read(struct xlnx_dma_dev *xdev,
	int ring_index, struct qdma_indirect_intr_ctxt *ctxt);

#ifndef __QDMA_VF__
/*****************************************************************************/
/**
 * qdma_descq_context_read() - handler to program the context for vf
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:		indicates whether the h2c or c2h direction
 * @param[in]	mm_cmpt_en:	indicates whether cmpt is enabled for mm or not*
 * @param[out]	ctxt:	pointer to context data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_program(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			bool st, u8 type, struct qdma_descq_context *context);

#endif

#endif /* ifndef __LIBQDMA_CONTEXT_H__ */
