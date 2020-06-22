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

#ifndef LIBQDMA_QDMA_DEVICE_H_
#define LIBQDMA_QDMA_DEVICE_H_
/**
 * @file
 * @brief This file contains the declarations for QDMA device
 *
 */
#include <linux/spinlock_types.h>
#include "libqdma_export.h"
#include "qdma_access_common.h"

/**
 * forward declaration for qdma descriptor
 */
struct qdma_descq;
/**
 * forward declaration for xlnx_dma_dev
 */
struct xlnx_dma_dev;

/**
 * @struct - qdma_dev
 * @brief	qdma device per function
 */
struct qdma_dev {
	/** flag indicates whether the fmap programming is completed or not */
	u8 init_qrange:1;
	/** filler */
	u8 filler[3];
	/** max number of queues per function */
	unsigned short qmax;
	/** queue start number for this function */
	unsigned short qbase;
	/** qdma_dev lock */
	spinlock_t lock;
	/** h2c descq list */
	struct qdma_descq *h2c_descq;
	/** c2h descq list */
	struct qdma_descq *c2h_descq;
	/** cmpt descq list */
	struct qdma_descq *cmpt_descq;
};

/**
 * macro to convert the given xdev to qdev
 */
#define xdev_2_qdev(xdev)	(struct qdma_dev *)((xdev)->dev_priv)

/*****************************************************************************/
/**
 * qdma_device_init() - initializes the qdma device
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	-1: failure
 *****************************************************************************/
int qdma_device_init(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_device_cleanup() - clean up the qdma device
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	none
 *****************************************************************************/
void qdma_device_cleanup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_device_get_descq_by_id() - get the qhndl for descq
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	descq:	pointer to the descq
 *
 * @return	qhndl for descq on success
 * @return	<0 on failure
 *****************************************************************************/
long qdma_device_get_id_from_descq(struct xlnx_dma_dev *xdev,
				   struct qdma_descq *descq);
/*****************************************************************************/
/**
 * qdma_device_get_descq_by_id() - get the descq using the qid
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	idx:	sw qidx
 * @param[in]	init:	indicates whether to initialize the device or not
 * @param[in]	buflen:	length of the input buffer
 * @param[out]	buf:	message buffer
 *
 * @return	pointer to descq on success
 * @return	NULL on failure
 *****************************************************************************/
struct qdma_descq *qdma_device_get_descq_by_id(struct xlnx_dma_dev *xdev,
			unsigned long idx, char *buf, int buflen, int init);

#ifdef DEBUGFS
/*****************************************************************************/
/**
 * qdma_device_get_pair_descq_by_id() - get the descq using the qid
 *
 * @param[in]	xdev:	pointer to xdev
 * @param[in]	idx:	sw qidx
 * @param[in]	init:	indicates whether to initialize the device or not
 * @param[in]	buflen:	length of the input buffer
 * @param[out]	buf:	message buffer
 *
 * @return	pointer to descq on success
 * @return	NULL on failure
 *****************************************************************************/
struct qdma_descq *qdma_device_get_pair_descq_by_id(struct xlnx_dma_dev *xdev,
			unsigned long idx, char *buf, int buflen, int init);
#endif

/*****************************************************************************/
/**
 * qdma_device_get_descq_by_hw_qid() - get the descq using the hw qid
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	qidx_hw:	hw qidx
 * @param[in]	c2h:		indicates whether hw qidx belongs to c2h or h2c
 *
 * @return	pointer to descq on success
 * @return	NULL on failure
 *****************************************************************************/
struct qdma_descq *qdma_device_get_descq_by_hw_qid(struct xlnx_dma_dev *xdev,
			unsigned long qidx_hw, u8 c2h);

/*****************************************************************************/
/**
 * qdma_device_prep_q_resource() - Prepare queue resources
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_prep_q_resource(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_device_interrupt_setup() - Setup device itnerrupts
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_interrupt_setup(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_device_interrupt_cleanup() - Celanup device interrupts
 *
 * @param[in]	xdev:		pointer to xdev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
void qdma_device_interrupt_cleanup(struct xlnx_dma_dev *xdev);

#ifdef __QDMA_VF__
/*****************************************************************************/
/**
 * device_set_qconf() - set device conf
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	qmax:		maximum request qsize for VF instance
 * @param[in]	qbase:		queue base
 *
 * @return  0: success
 * @return  < 0: failure
 *****************************************************************************/
int device_set_qconf(struct xlnx_dma_dev *xdev, int *qmax, int *qbase);

#endif


enum csr_type {
	QDMA_CSR_TYPE_NONE,
	QDMA_CSR_TYPE_RNGSZ,	/** all global csr ring size settings */
	QDMA_CSR_TYPE_BUFSZ,	/** all global csr buffer size settings */
	QDMA_CSR_TYPE_TIMER_CNT, /** all global csr timer count settings */
	QDMA_CSR_TYPE_CNT_TH,	/** all global csr counter thresh settings */

	QDMA_CSR_TYPE_MAX
};

/*****************************************************************************/
/**
 * qdma_csr_read() - Read specific global csr registers
 *
 * @param[in]   xdev:           pointer to xdev
 * @param[out]  csr:            csr value
 *
 * @return      0: success
 * @return      <0: failure
 *****************************************************************************/
int qdma_csr_read(struct xlnx_dma_dev *xdev, struct global_csr_conf *csr);

/*****************************************************************************/
/**
 * qdma_set_ring_sizes() - Wrapper function to set the ring sizes values
 *
 * @param[in]   xdev:           pointer to xdev
 * @param[in]   index: Index from where the values needs to written
 * @param[in]   count: number of entries to be written
 * @param[in]	glbl_rng_sz: pointer to array of global ring sizes
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_ring_sizes(struct xlnx_dma_dev *xdev, u8 index,
		u8 count, u32 *glbl_rng_sz);

/*****************************************************************************/
/**
 * qdma_set_ring_sizes() - Wrapper function to set the ring sizes values
 *
 * @param[in]   xdev:  pointer to xdev
 * @param[in]   index: Index from where the values needs to written
 * @param[in]   count: number of entries to be written
 * @param[out]	glbl_rng_sz: pointer to array of global ring sizes
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_get_ring_sizes(struct xlnx_dma_dev *xdev, u8 index,
		u8 count, u32 *glbl_rng_sz);


/*****************************************************************************/
/**
 * qdma_pf_trigger_vf_reset() - Function to trigger FLR of all the VFs on PF
 *
 * @param[in]	dev_hndl:	Handle of the xdev
 *****************************************************************************/
void qdma_pf_trigger_vf_reset(unsigned long dev_hndl);

/**
 * qdma_pf_trigger_vf_offline() - Function to trigger offline of all the VFs
 *
 * @param[in]	dev_hndl:	Handle of the xdev
 *****************************************************************************/
void qdma_pf_trigger_vf_offline(unsigned long dev_hndl);
#endif /* LIBQDMA_QDMA_DEVICE_H_ */
