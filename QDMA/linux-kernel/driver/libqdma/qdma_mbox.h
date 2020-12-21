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

#ifndef __QDMA_MBOX_H__
#define __QDMA_MBOX_H__

#include "qdma_compat.h"
#include "qdma_device.h"
#include "qdma_access_common.h"
#include "qdma_mbox_protocol.h"

/**
 * @file
 * @brief This file contains the declarations for qdma mailbox apis
 *
 */

/**
 * mailbox messages
 *
 * NOTE: make sure the total message length is <= 128 bytes:
 * mbox_msg_hdr: 4 bytes
 * body: <= (128 - hdr) bytes
 *
 */

struct mbox_msg {
	struct work_struct work;	/** workqueue item */
	struct list_head list;		/** message list */
	qdma_wait_queue waitq;
	struct kref refcnt;
	u8 wait_resp;
	u8 resp_op_matched;
	u16 retry_cnt;

	u32 raw[MBOX_MSG_REG_MAX];
};

/**
 * forward declaration of xlnx_dma_dev
 */
struct xlnx_dma_dev;
/**
 * @struct - qdma_mbox book keeping
 * @brief mailbox book keeping structure
 */
struct qdma_mbox {
	/** common lock */
	spinlock_t lock;
	/** tx lock */
	spinlock_t hw_tx_lock;
	/** rx lock */
	spinlock_t hw_rx_lock;
	/** work queue */
	struct workqueue_struct *workq;
	/** pointer to device data */
	struct xlnx_dma_dev *xdev;
	/** tx work_struct to pass data to tx work queue */
	struct work_struct tx_work;
	/** rx work_struct to pass data to rx work queue */
	struct work_struct rx_work;
	/** mbox rx message */
	struct mbox_msg rx;
	/** list lock */
	spinlock_t list_lock;
	/** list of messages waiting to be sent */
	struct list_head tx_todo_list;
	/** list of messages waiting for response */
	struct list_head rx_pend_list;
	/* flag to monitor tx is busy */
	uint8_t send_busy;
	/* flag for rx polling mode */
	uint8_t rx_poll;

	/** timer list */
	struct timer_list timer;

};

#define QDMA_MBOX_MSG_TIMEOUT_MS	10000 /* 10 sec*/
/*****************************************************************************/
/**
 * qdma_mbox_init() - initialize qdma mailbox
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_mbox_init(struct xlnx_dma_dev *xdev);

/*****************************************************************************/
/**
 * qdma_mbox_cleanup()	- cleanup resources of qdma mailbox
 * qdma_mbox_stop()	- stop mailbox processing
 * qdma_mbox_start()	- start mailbox processing
 * qdma_mbox_poll_start()	- start mailbox processing in poll mode
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 *
 * @return	none
 *****************************************************************************/
void qdma_mbox_cleanup(struct xlnx_dma_dev *xdev);
void qdma_mbox_stop(struct xlnx_dma_dev *xdev);
void qdma_mbox_start(struct xlnx_dma_dev *xdev);
void qdma_mbox_poll_start(struct xlnx_dma_dev *xdev);


/*****************************************************************************/
/**
 * qdma_mbox_msg_send() - handler to send a mailbox message
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 * @param	m:		mailbox message
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_mbox_msg_send(struct xlnx_dma_dev *xdev, struct mbox_msg *m,
			bool wait_resp, unsigned int timeout_ms);


/*****************************************************************************/
/**
 * qdma_mbox_msg_alloc() - allocate a mailbox message
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 *
 * @return	0: success
 * @return	NULL: failure
 *****************************************************************************/
struct mbox_msg *qdma_mbox_msg_alloc(void);

/*****************************************************************************/
/**
 * __qdma_mbox_msg_free() - free the mailbox message
 *
 * @param	f:		function name
 * @param	m:		mailbox message
 *
 * @return	none
 *****************************************************************************/
void __qdma_mbox_msg_free(const char *f, struct mbox_msg *m);
#define qdma_mbox_msg_free(m)	__qdma_mbox_msg_free(__func__, m)

#endif /* #ifndef __QDMA_MBOX_H__ */
