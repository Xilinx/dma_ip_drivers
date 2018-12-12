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

#ifndef __QDMA_MBOX_H__
#define __QDMA_MBOX_H__

#include "qdma_compat.h"
#include "qdma_device.h"

/**
 * @file
 * @brief This file contains the declarations for qdma mailbox apis
 *
 */
/**
 * mailbox registers
 */
#ifdef __QDMA_VF__
#define MBOX_BASE		0x1000
#else
#define MBOX_BASE		0x2400
#endif

/** mailbox function status */
#define MBOX_FN_STATUS			0x0
/** shift value for mailbox function status in msg */
#define		S_MBOX_FN_STATUS_IN_MSG	0
/** mask value for mailbox function status in msg*/
#define		M_MBOX_FN_STATUS_IN_MSG	0x1
/** face value for mailbox function status in msg */
#define		F_MBOX_FN_STATUS_IN_MSG	0x1

/** shift value for out msg */
#define		S_MBOX_FN_STATUS_OUT_MSG	1
/** mask value for out msg */
#define		M_MBOX_FN_STATUS_OUT_MSG	0x1
/** face value for out msg */
#define		F_MBOX_FN_STATUS_OUT_MSG	(1 << S_MBOX_FN_STATUS_OUT_MSG)
/** shift value for status ack */
#define		S_MBOX_FN_STATUS_ACK	2	/* PF only, ack status */
/** mask value for status ack */
#define		M_MBOX_FN_STATUS_ACK	0x1
/** face value for status ack */
#define		F_MBOX_FN_STATUS_ACK	(1 << S_MBOX_FN_STATUS_ACK)
/** shift value for status src */
#define		S_MBOX_FN_STATUS_SRC	4	/* PF only, source func.*/
/** mask value for status src */
#define		M_MBOX_FN_STATUS_SRC	0xFFF
/** face value for status src */
#define		G_MBOX_FN_STATUS_SRC(x)	\
		(((x) >> S_MBOX_FN_STATUS_SRC) & M_MBOX_FN_STATUS_SRC)
/** face value for mailbox function status */
#define MBOX_FN_STATUS_MASK \
		(F_MBOX_FN_STATUS_IN_MSG | \
		 F_MBOX_FN_STATUS_OUT_MSG | \
		 F_MBOX_FN_STATUS_ACK)

/** mailbox function commands register */
#define MBOX_FN_CMD			0x4
/** shift value for send command */
#define		S_MBOX_FN_CMD_SND	0
/** mask value for send command */
#define		M_MBOX_FN_CMD_SND	0x1
/** face value for send command */
#define		F_MBOX_FN_CMD_SND	(1 << S_MBOX_FN_CMD_SND)
/** shift value for receive command */
#define		S_MBOX_FN_CMD_RCV	1
/** mask value for receive command */
#define		M_MBOX_FN_CMD_RCV	0x1
/** face value for receive command */
#define		F_MBOX_FN_CMD_RCV	(1 << S_MBOX_FN_CMD_RCV)
/** shift value for vf reset */
#define		S_MBOX_FN_CMD_VF_RESET	3	/* TBD PF only: reset VF */
/** mask value for vf reset */
#define		M_MBOX_FN_CMD_VF_RESET	0x1
/** mailbox isr vector register */
#define MBOX_ISR_VEC			0x8
/** shift value for isr vector */
#define		S_MBOX_ISR_VEC		0
/** mask value for isr vector */
#define		M_MBOX_ISR_VEC		0x1F
/** face value for isr vector */
#define		V_MBOX_ISR_VEC(x)	((x) & M_MBOX_ISR_VEC)
/** mailbox FN target register */
#define MBOX_FN_TARGET			0xC
/** shift value for FN target id */
#define		S_MBOX_FN_TARGET_ID	0
/** mask value for FN target id */
#define		M_MBOX_FN_TARGET_ID	0xFFF
/** face value for FN target id */
#define		V_MBOX_FN_TARGET_ID(x)	((x) & M_MBOX_FN_TARGET_ID)
/** mailbox isr enable register */
#define MBOX_ISR_EN			0x10
/** shift value for isr enable */
#define		S_MBOX_ISR_EN		0
/** mask value for isr enable */
#define		M_MBOX_ISR_EN		0x1
/** face value for isr enable */
#define		F_MBOX_ISR_EN		0x1
/** pf acknowledge base */
#define MBOX_PF_ACK_BASE		0x20
/** pf acknowledge step */
#define MBOX_PF_ACK_STEP		4
/** pf acknowledge count */
#define MBOX_PF_ACK_COUNT		8
/** mailbox incoming msg base */
#define MBOX_IN_MSG_BASE		0x800
/** mailbox outgoing msg base */
#define MBOX_OUT_MSG_BASE		0xc00
/** mailbox msg step */
#define MBOX_MSG_STEP			4
/** mailbox register max */
#define MBOX_MSG_REG_MAX		32

/**
 * @struct - hw_descq_context
 * @brief	queue context information
 */
struct hw_descq_context {
	/** software descriptor context data: 4 data words */
	u32 sw[5];
	/** prefetch context data: 2 data words */
	u32 prefetch[2];
	/** queue completion context data: 4 data words */
	u32 cmpt[5];
	/** hardware descriptor  context data: 2 data words */
	u32 hw[2];	/* for retrieve only */
	/** C2H or H2C context: 1 data word */
	u32 cr[1];	/* for retrieve only */
	/** FMAP context data */
	u32 fmap[2];
};

/**
 * @struct - stm_descq_context
 * @brief	queue stm information
 */
struct stm_descq_context {
	/** STM data: 6 data words */
	u32 stm[6];
};

/**
 * mailbox messages
 *
 * NOTE: make sure the total message length is <= 128 bytes:
 * mbox_msg_hdr: 4 bytes
 * body: <= (128 - hdr) bytes
 *
 */

/**
 * mbox_msg_op - mailbox messages opcode: 1 ~ 0x1F
 */
#define MBOX_MSG_OP_PF_MASK	0x10
enum mbox_msg_op {
	/* 0x00 */ MBOX_OP_NOOP,
	/** VF -> PF, request */
	/* 0x01 */ MBOX_OP_BYE,		/** vf offline */
	/* 0x02 */ MBOX_OP_HELLO,	/** vf online */

	/* 0x03 */ MBOX_OP_FMAP,	/** FMAP programming request */
	/* 0x04 */ MBOX_OP_CSR,		/** global CSR registers request */
	/* 0x05 */ MBOX_OP_INTR_CTXT,	/** interrupt context programming */
	/* 0x06 */ MBOX_OP_QCTXT_WRT,	/** queue context programming */
	/* 0x07 */ MBOX_OP_QCTXT_RD,	/** queue context read */
	/* 0x08 */ MBOX_OP_QCTXT_CLR,	/** queue context clear */
	/* 0x09 */ MBOX_OP_QCTXT_INV,	/** queue context invalidate */
	/* 0x0a */ MBOX_OP_QCONF,		/** queue context invalidate */

	/** PF->VF: response */
	/* 0x12 */ MBOX_OP_HELLO_RESP = 0x12,/** vf online */
	/* 0x13 */ MBOX_OP_FMAP_RESP,	/** FMAP programming */
	/* 0x14 */ MBOX_OP_CSR_RESP,	/** global CSR read */
	/* 0x15 */ MBOX_OP_INTR_CTXT_RESP, /** interrupt context programming  */
	/* 0x16 */ MBOX_OP_QCTXT_WRT_RESP, /** queue context programming */
	/* 0x17 */ MBOX_OP_QCTXT_RD_RESP, /** queue context read */
	/* 0x18 */ MBOX_OP_QCTXT_CLR_RESP, /** queue context clear */
	/* 0x19 */ MBOX_OP_QCTXT_INV_RESP, /** queue context invalidate */
	/* 0x1a */ MBOX_OP_QCONF_RESP,  /** queue context invalidate */

	MBOX_OP_MAX
};

#define mbox_invalidate_msg(m)	{ (m)->hdr.op = MBOX_OP_NOOP; }

/**
 * @struct - mbox_msg_hdr
 * @brief	mailbox message header
 */
struct mbox_msg_hdr {
	u8 op;		/** opcode */
	u16 src;		/** src function */
	u16 dst;		/** dst function */
	char status;	/** execution status */
};

/**
 * @struct - mbox_msg_fmap
 * @brief	FMAP programming command
 */
struct mbox_msg_fmap {
	/** mailbox message header */
	struct mbox_msg_hdr hdr;
	/** start queue number in the queue range */
	unsigned int qbase;
	/** max queue number in the queue range(0-2k) */
	unsigned int qmax;
};

/**
 * @struct - mbox_msg_csr
 * @brief	mailbox csr reading message
 */
struct mbox_msg_csr {
	/** mailbox message header*/
	struct mbox_msg_hdr hdr;
	struct qdma_csr_info csr_info;
};

/**
 * @struct - mbox_msg_intr_ctxt
 * @brief	interrupt context mailbox message
 */


struct mbox_msg_intr_ctxt {
	/** mailbox message header*/
	struct mbox_msg_hdr hdr;
	/** flag to indicate clear interrupt context*/
	u16 clear:1;
	/** filler variable*/
	u16 filler:15;
	/** start vector number*/
	u8 vec_base;	/* 0 ~ 7 */
	/** number of intr context rings be assigned for virtual function*/
	u8 num_rings;	/* 1 ~ 8 */
	/** ring index associated for each vector */
	u32 ring_index_list[QDMA_NUM_DATA_VEC_FOR_INTR_CXT];
	/** interrupt context data for all rings*/
	u32 w[QDMA_NUM_DATA_VEC_FOR_INTR_CXT * 3];
};

/**
 * @struct - mbox_msg_qctxt
 * @brief queue context mailbox message header
 */
struct mbox_msg_qctxt {
	/** mailbox message header*/
	struct mbox_msg_hdr hdr;
	/** flag to indicate to clear the queue context */
	u8 clear:1;
	/** flag to indicate to verify the queue context */
	u8 verify:1;
	/** queue direction */
	u8 c2h:1;
	/** queue mode */
	u8 st:1;
	/** flag to indicate to enable the interrupts */
	u8 intr_en:1;
	/** interrupt id */
	u8 intr_id;
	/** queue id */
	unsigned short qid;
	/** complete hw context */
	struct hw_descq_context context;
};

/**
 * @struct - mbox_msg
 * @brief mailbox message
 */
struct mbox_msg {
	struct work_struct work;	/** workqueue item */
	struct list_head list;		/** message list */
	qdma_wait_queue waitq;
	struct kref refcnt;
	u8 wait_resp;
	u8 wait_op;
	u8 rsvd[2];

	union {
		/** mailbox message header*/
		struct mbox_msg_hdr hdr;
		/** fmap mailbox message */
		struct mbox_msg_fmap fmap;
		/** interrupt context mailbox message */
		struct mbox_msg_intr_ctxt intr_ctxt;
		/** queue context mailbox message */
		struct mbox_msg_qctxt qctxt;
		/** global csr mailbox message */
		struct mbox_msg_csr csr;
		/** buffer to hold raw data between pf and vf */
		u32 raw[MBOX_MSG_REG_MAX];
	};
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
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 *
 * @return	none
 *****************************************************************************/
void qdma_mbox_cleanup(struct xlnx_dma_dev *xdev);
void qdma_mbox_stop(struct xlnx_dma_dev *xdev);
void qdma_mbox_start(struct xlnx_dma_dev *xdev);

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
			bool wait_resp, u8 resp_op, unsigned int timeout_ms);

/*****************************************************************************/
/**
 * qdma_mbox_msg_alloc() - allocate a mailbox message
 *
 * @param	xdev:	pointer to xlnx_dma_dev
 *
 * @return	0: success
 * @return	NULL: failure
 *****************************************************************************/
struct mbox_msg *qdma_mbox_msg_alloc(struct xlnx_dma_dev *xdev,
					enum mbox_msg_op op);

/*****************************************************************************/
/**
 * __qdma_mbox_msg_free() - free the mailbox message
 *
 * @param	fname:	function name
 * @param	m:		mailbox message
 *
 * @return	none
 *****************************************************************************/
void __qdma_mbox_msg_free(const char *fname, struct mbox_msg *m);
#define qdma_mbox_msg_free(m)	__qdma_mbox_msg_free(__func__, m)

#endif /* #ifndef __QDMA_MBOX_H__ */
