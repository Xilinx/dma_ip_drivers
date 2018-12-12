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

#ifndef LIBQDMA_QDMA_DEVICE_H_
#define LIBQDMA_QDMA_DEVICE_H_
/**
 * @file
 * @brief This file contains the declarations for QDMA device
 *
 */
#include <linux/spinlock_types.h>
#include "libqdma_export.h"

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
	/** number of h2c queues for this function */
	unsigned short h2c_qcnt;
	/** number of c2h queues for this function */
	unsigned short c2h_qcnt;
	/** h2c descq list */
	struct qdma_descq *h2c_descq;
	/** c2h descq list */
	struct qdma_descq *c2h_descq;
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

#ifndef __QDMA_VF__
/*****************************************************************************/
/**
 * qdma_csr_read_cmpl_status_acc() - Read the completion status
 * accumulation value
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[out]	cs_acc:		cs_acc value
 *
 * @return	none
 *****************************************************************************/
void qdma_csr_read_cmpl_status_acc(struct xlnx_dma_dev *xdev,
		unsigned int *cs_acc);

/*****************************************************************************/
/**
 * qdma_csr_read_rngsz() - Read the queue ring size
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[out]	rngsz:		queue ring size
 *
 * @return	none
 *****************************************************************************/
void qdma_csr_read_rngsz(struct xlnx_dma_dev *xdev, unsigned int *rngsz);

/*****************************************************************************/
/**
 * qdma_csr_read_bufsz() - Read the buffer size
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[out]	bufsz:		buffer size
 *
 * @return	none
 *****************************************************************************/
void qdma_csr_read_bufsz(struct xlnx_dma_dev *xdev, unsigned int *bufsz);

/*****************************************************************************/
/**
 * qdma_csr_read_timer_cnt() - Read the timer count
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[out]	cnt:		timer count
 *
 * @return	none
 *****************************************************************************/
void qdma_csr_read_timer_cnt(struct xlnx_dma_dev *xdev, unsigned int *cnt);

/*****************************************************************************/
/**
 * qdma_csr_read_timer_cnt() - Read the timer threshold
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[out]	th:			timer threshold
 *
 * @return	none
 *****************************************************************************/
void qdma_csr_read_cnt_thresh(struct xlnx_dma_dev *xdev, unsigned int *th);
#else
/*****************************************************************************/
/**
 * device_set_qconf() - set device conf
 *
 * @param[in]	xdev:		pointer to xdev
 * @param[in]	qmax:		maximum request qsize for VF instance
 *
 * @return  0: success
 * @return  < 0: failure
 *****************************************************************************/
int device_set_qconf(struct xlnx_dma_dev *xdev, int qmax, u32 *qbase);

#endif


enum csr_type {
	QDMA_CSR_TYPE_NONE,
	QDMA_CSR_TYPE_RNGSZ,	/** all global csr ring size settings */
	QDMA_CSR_TYPE_BUFSZ,	/** all global csr buffer size settings */
	QDMA_CSR_TYPE_TIMER_CNT, /** all global csr timer count settings */
	QDMA_CSR_TYPE_CNT_TH,	/** all global csr counter thresh settings */

	QDMA_CSR_TYPE_MAX
};

/**
 * @struct - descq_csr_info
 * @brief	qdma Q csr register settings
 */
struct qdma_csr_info {
	enum csr_type type;	/** one csr register array */
	u32 array[QDMA_GLOBAL_CSR_ARRAY_SZ];
	u8 idx_rngsz;		/** 1x index-value pair for each type */
	u8 idx_bufsz;
	u8 idx_timer_cnt;
	u8 idx_cnt_th;
	u32 rngsz;
	u32 bufsz;
	u32 timer_cnt;
	u32 cnt_th;
	u32 cmpl_status_acc;
};

/*****************************************************************************/
/**
 * qdma_csr_read() - Read specific global csr registers
 *
 * @param[in]   xdev:           pointer to xdev
 * @param[in]   csr:            csr type & index
 * @param[out]  csr:            csr value
 *
 * @return      0: success
 * @return      <0: failure
 *****************************************************************************/
int qdma_csr_read(struct xlnx_dma_dev *xdev, struct qdma_csr_info *csr_info,
		unsigned int timeout_ms);

#endif /* LIBQDMA_QDMA_DEVICE_H_ */
