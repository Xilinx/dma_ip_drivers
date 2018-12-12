/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
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
 * qdma_descq_stm_setup() - handler to set the qdma stm
 *
 * @param[in]	descq:		pointer to qdma_descq
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_stm_setup(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * qdma_descq_stm_clear() - handler to clear the qdma stm
 *
 * @param[in]	descq:		pointer to qdma_descq
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_stm_clear(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * qdma_descq_context_clear() - handler to clear the qdma sw descriptor context
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:	indicates whether the h2c or c2h direction
 * @param[in]	clr:	flag to indicate whether to clear the context or not
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_clear(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h, bool clr);

/*****************************************************************************/
/**
 * qdma_descq_context_read() - handler to read the queue context
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:	indicates whether the h2c or c2h direction
 * @param[out]	ctxt:	pointer to context data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h,
				struct hw_descq_context *ctxt);

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
				int ring_index, unsigned int ctxt_sz,
				u32 *context);

#ifndef __QDMA_VF__
/*****************************************************************************/
/**
 * qdma_descq_context_read() - handler to program the context for vf
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	st:		indicated whether the mm mode or st mode
 * @param[in]	c2h:	indicates whether the h2c or c2h direction
 * @param[out]	ctxt:	pointer to context data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_context_program(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h,
				struct hw_descq_context *ctxt);


/*****************************************************************************/
/**
 * qdma_descq_stm_read() - handler to read stm context, can, maps
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	pipe_flow_id: pipe_flow_id for queue
 * @param[in]	c2h:	indicates whether the h2c or c2h direction
 * @param[in]	map:	indicates whether to read map or ctxt/can
 * @param[in]   ctxt:	indicates whether to read ctxt or can
 * @param[out]  context: pointer to context data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_stm_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			u8 pipe_flow_id, bool c2h, bool map, bool ctxt,
			struct stm_descq_context *context);


/*****************************************************************************/
/**
 * qdma_descq_stm_program() - handler to program the stm
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	qid_hw:	hw qidx
 * @param[in]	pipe_flow_id:	flow id for pipe
 * @param[in]	c2h:	indicates whether the h2c or c2h direction
 * @param[in]   clear:  flag to prog/clear stm context/maps
 * @param[out]	stm:	pointer to stm data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_stm_program(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			   uint8_t pipe_flow_id, bool c2h, bool clear,
			   struct stm_descq_context *context);

#endif

#endif /* ifndef __LIBQDMA_CONTEXT_H__ */
