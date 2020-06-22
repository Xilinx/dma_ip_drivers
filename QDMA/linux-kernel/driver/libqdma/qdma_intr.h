/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
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

#ifndef LIBQDMA_QDMA_INTR_H_
#define LIBQDMA_QDMA_INTR_H_
/**
 * @file
 * @brief This file contains the declarations for qdma dev interrupt handlers
 *
 */
#include <linux/types.h>
#include <linux/workqueue.h>
#include "qdma_descq.h"
/**
 * forward declaration for xlnx_dma_dev
 */
struct xlnx_dma_dev;

/**
 * @struct - qdma_intr_ring_cpm
 * @brief	Interrupt ring entry definition for 2018.2 CPM release
 */
struct qdma_intr_ring_cpm {
	/** producer index. This is from Interrupt source.
	 *  Cumulative pointer of total interrupt Aggregation
	 *  Ring entry written
	 */
	__be64 pidx:16;
	/** consumer index. This is from Interrupt source.
	 *  Cumulative consumed pointer
	 */
	__be64 cidx:16;
	/** source color. This is from Interrupt source.
	 *  This bit inverts every time pidx wraps around
	 *  and this field gets copied to color field of descriptor.
	 */
	__be64 s_color:1;
	/** This is from Interrupt source.
	 * Interrupt state, 0: CMPT_INT_ISR; 1: CMPT_INT_TRIG; 2: CMPT_INT_ARMED
	 */
	__be64 intr_satus:2;
	/** error. This is from interrupt source
	 *  {C2h_err[1:0], h2c_err[1:0]}
	 */
	__be64 error:4;
	/**  11 reserved bits*/
	__be64 rsvd:11;
	/**  Is the interrupt raised due to error ?
	 *   1: error interrupt; 0: non-error interrupt
	 */
	__be64 error_int:1;
	/**  interrupt type, 0: H2C; 1: C2H*/
	__be64 intr_type:1;
	/**  This is from Interrupt source. Queue ID*/
	__be64 qid:11;
	/**  The color bit of the Interrupt Aggregation Ring.
	 *   This bit inverts every time pidx wraps around on the
	 *   Interrupt Aggregation Ring.
	 */
	__be64 coal_color:1;
};

/**
 * @struct - qdma_intr_ring_generic
 * @brief	Interrupt ring entry definition
 */
struct qdma_intr_ring_generic {
	/** producer index. This is from Interrupt source.
	 *  Cumulative pointer of total interrupt Aggregation
	 *  Ring entry written
	 */
	__be64 pidx:16;
	/** consumer index. This is from Interrupt source.
	 *  Cumulative consumed pointer
	 */
	__be64 cidx:16;
	/** source color. This is from Interrupt source.
	 *  This bit inverts every time pidx wraps around
	 *  and this field gets copied to color field of descriptor.
	 */
	__be64 s_color:1;
	/** This is from Interrupt source.
	 * Interrupt state, 0: CMPT_INT_ISR; 1: CMPT_INT_TRIG; 2: CMPT_INT_ARMED
	 */
	__be64 intr_satus:2;
	/** error. This is from interrupt source
	 *  {C2h_err[1:0], h2c_err[1:0]}
	 */
	__be64 error:2;
	/**  1 reserved bits*/
	__be64 rsvd:1;
	/**  interrupt type, 0: H2C; 1: C2H*/
	__be64 intr_type:1;
	/**  This is from Interrupt source. Queue ID*/
	__be64 qid:24;
	/**  The color bit of the Interrupt Aggregation Ring.
	 *   This bit inverts every time pidx wraps around on the
	 *   Interrupt Aggregation Ring.
	 */
	__be64 coal_color:1;
};

union qdma_intr_ring {
	struct qdma_intr_ring_cpm ring_cpm;
	struct qdma_intr_ring_generic ring_generic;
};


/*****************************************************************************/
/**
 * intr_teardown() - un register the interrupts for the device
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	none
 *****************************************************************************/
void intr_teardown(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * intr_setup() - register the interrupts for the device
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int intr_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * intr_ring_teardown() - delete the interrupt ring
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	none
 *****************************************************************************/
void intr_ring_teardown(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * intr_context_setup() - set up the interrupt context
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int intr_context_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * intr_ring_setup() - create the interrupt ring
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int intr_ring_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * intr_legacy_setup() - setup the legacy interrupt handler
 *
 * @param[in]	descq:	descq on which the interrupt needs to be setup
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int intr_legacy_setup(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * intr_legacy_clear() - clear the legacy interrupt handler
 *
 * @param[in]	descq:	descq on which the interrupt needs to be cleared
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
void intr_legacy_clear(struct qdma_descq *descq);


/*****************************************************************************/
/**
 * intr_work() - attach the top half for the interrupt
 *
 * @param[in]	work:		pointer to struct work_struct
 *
 * @return	none
 *****************************************************************************/
void intr_work(struct work_struct *work);

/*****************************************************************************/
/**
 * qdma_err_intr_setup() - set up the error interrupt
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	none
 *****************************************************************************/
int qdma_err_intr_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_enable_hw_err() - enable the hw errors
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	hw_err_type:	hw error type
 *
 * @return	none
 *****************************************************************************/
void qdma_enable_hw_err(struct xlnx_dma_dev *xdev, u8 hw_err_type);

/*****************************************************************************/
/**
 * get_intr_ring_index() - get the interrupt ring index based on vector index
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	vector_index:	vector index
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int get_intr_ring_index(struct xlnx_dma_dev *xdev, u32 vector_index);

#ifndef __QDMA_VF__
#ifdef ERR_DEBUG
/*****************************************************************************/
/**
 * err_stat_handler() - error interrupt handler
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	none
 *****************************************************************************/
void err_stat_handler(struct xlnx_dma_dev *xdev);
#endif
#endif

#endif /* LIBQDMA_QDMA_DEVICE_H_ */

