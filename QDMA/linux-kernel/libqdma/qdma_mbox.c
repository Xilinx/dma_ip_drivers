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

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "qdma_compat.h"
#include "xdev.h"
#include "qdma_device.h"
#include "qdma_regs.h"
#include "qdma_context.h"
#include "qdma_mbox.h"
#include "qdma_qconf_mgr.h"

#define MBOX_TIMER_INTERVAL	(1)

/*
 * mailbox opcode string
 */
static const char *mbox_op_str[MBOX_OP_MAX] = {
	"noop",
	"bye",
	"hello",

	"fmap",
	"csr",
	"intr_ctrx",
	"qctrx_wrt",
	"qctrx_rd",
	"qctrx_clr",
	"qctrx_inv",

	"qconf",
	"rsvd_0x0b",
	"rsvd_0x0c",
	"rsvd_0x0d",
	"rsvd_0x0e",
	"rsvd_0x0f",
	"rsvd_0x10",
	"rsvd_0x11",

	"hello_resp",
	"fmap_resp",
	"csr_resp",
	"intr_ctrx_resp",
	"qctrx_wrt_resp",
	"qctrx_rd_resp",
	"qctrx_clr_resp",
	"qctrx_inv_resp",
	"qconf_resp",
};

/*
 * mailbox h/w registers access
 */

#ifndef __QDMA_VF__
static inline void mbox_pf_hw_clear_func_ack(struct xlnx_dma_dev *xdev,
					u8 func_id)
{
	int idx = func_id / 32; /* bitmask, u32 reg */
	int bit = func_id % 32;

	/* clear the function's ack status */
	__write_reg(xdev,
		MBOX_BASE + MBOX_PF_ACK_BASE + idx * MBOX_PF_ACK_STEP,
		(1 << bit));
}

static void mbox_pf_hw_clear_ack(struct xlnx_dma_dev *xdev)
{
	u32 v = __read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS);
	u32 reg = MBOX_BASE + MBOX_PF_ACK_BASE;
	int i;

	if ((v & F_MBOX_FN_STATUS_ACK) == 0)
		return;

	for (i = 0; i < MBOX_PF_ACK_COUNT; i++, reg += MBOX_PF_ACK_STEP) {
		u32 v = __read_reg(xdev, reg);

		if (!v)
			continue;

		/* clear the ack status */
		pr_debug("%s, PF_ACK %d, 0x%x.\n", xdev->conf.name, i, v);
		__write_reg(xdev, reg, v);
	}
}
#endif

static int mbox_hw_send(struct qdma_mbox *mbox, struct mbox_msg *m)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	struct mbox_msg_hdr *hdr = &m->hdr;
	u32 fn_id = hdr->dst;
	int i;
	u32 reg = MBOX_OUT_MSG_BASE;
	u32 v;
	int rv = -EAGAIN;

	pr_debug("%s, dst 0x%x, op 0x%x, %s, status reg 0x%x.\n",
		xdev->conf.name, fn_id, m->hdr.op, mbox_op_str[m->hdr.op],
		__read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS));

	spin_lock_bh(&mbox->hw_tx_lock);

#ifndef __QDMA_VF__
	__write_reg(xdev, MBOX_BASE + MBOX_FN_TARGET,
			V_MBOX_FN_TARGET_ID(fn_id));
#endif

	v = __read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS);
	if (v & F_MBOX_FN_STATUS_OUT_MSG) {
		pr_debug("%s, func 0x%x, outgoing message busy, 0x%x.\n",
			xdev->conf.name, fn_id, v);
		goto unlock;
	}

	for (i = 0; i < MBOX_MSG_REG_MAX; i++, reg += MBOX_MSG_STEP)
		__write_reg(xdev, MBOX_BASE + reg, m->raw[i]);

	pr_debug("%s, send op 0x%x,%s, src 0x%x, dst 0x%x, s 0x%x:\n",
		xdev->conf.name, m->hdr.op, mbox_op_str[m->hdr.op], m->hdr.src,
		m->hdr.dst, m->hdr.status);
#if 0
	print_hex_dump(KERN_INFO, "mbox snd: ", DUMP_PREFIX_OFFSET,
			16, 1, (void *)m, 64, false);
#endif

#ifndef __QDMA_VF__
	/* clear the outgoing ack */
	mbox_pf_hw_clear_func_ack(xdev, fn_id);
#endif

	__write_reg(xdev, MBOX_BASE + MBOX_FN_CMD, F_MBOX_FN_CMD_SND);
	rv = 0;

unlock:
	spin_unlock_bh(&mbox->hw_tx_lock);

	return rv;
}

static int mbox_hw_rcv(struct qdma_mbox *mbox, struct mbox_msg *m)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	u32 reg = MBOX_IN_MSG_BASE;
	u32 v = 0;
	int all_zero_msg = 1;
	int i;
	int rv = -EAGAIN;
#ifndef __QDMA_VF__
	unsigned int from_id = 0;
#endif

	spin_lock_bh(&mbox->hw_rx_lock);

	v = __read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS);

#if 0
	if ((v & MBOX_FN_STATUS_MASK))
		pr_debug("%s, base 0x%x, status 0x%x.\n",
			xdev->conf.name, MBOX_BASE, v);
#endif

	if (!(v & M_MBOX_FN_STATUS_IN_MSG))
		goto unlock;

#ifndef __QDMA_VF__
	from_id = G_MBOX_FN_STATUS_SRC(v);
	__write_reg(xdev, MBOX_BASE + MBOX_FN_TARGET, from_id);
#endif

	for (i = 0; i < MBOX_MSG_REG_MAX; i++, reg += MBOX_MSG_STEP) {
		m->raw[i] = __read_reg(xdev, MBOX_BASE + reg);
		/* if rcv'ed message is all zero, stop and disable the mbox,
		 * the h/w mbox is not working properly
		 */
		if (m->raw[i])
			all_zero_msg = 0;
	}
	if (all_zero_msg) {
		rv = -EPIPE;
		goto unlock;
	}

	pr_debug("%s, rcv op 0x%x, %s, src 0x%x, dst 0x%x, s 0x%x:\n",
		xdev->conf.name, m->hdr.op, mbox_op_str[m->hdr.op], m->hdr.src,
		m->hdr.dst, m->hdr.status);
#if 0
	print_hex_dump(KERN_INFO, "mbox rcv: ", DUMP_PREFIX_OFFSET,
			16, 1, (void *)m, 64, false);
#endif

#ifndef __QDMA_VF__
	if (from_id != m->hdr.src) {
		pr_debug("%s, src 0x%x -> func_id 0x%x.\n",
			xdev->conf.name, m->hdr.src, from_id);
		m->hdr.src = from_id;
	}
#endif

	/* ack'ed the sender */
	__write_reg(xdev, MBOX_BASE + MBOX_FN_CMD, F_MBOX_FN_CMD_RCV);
	rv = 0;

unlock:
	spin_unlock_bh(&mbox->hw_rx_lock);
	return rv;
}

/*
 * mbox tx message processing
 */
static void mbox_msg_destroy(struct kref *kref)
{
	struct mbox_msg *m = container_of(kref, struct mbox_msg, refcnt);

	kfree(m);
}

void __qdma_mbox_msg_free(const char *f, struct mbox_msg *m)
{
	kref_put(&m->refcnt, mbox_msg_destroy);
}

struct mbox_msg *qdma_mbox_msg_alloc(struct xlnx_dma_dev *xdev,
				enum mbox_msg_op op)
{
	struct mbox_msg *m = kmalloc(sizeof(struct mbox_msg), GFP_KERNEL);
	struct mbox_msg_hdr *hdr;

	if (!m) {
		pr_info("%s OOM, %lu, op 0x%x, %s.\n",
			xdev->conf.name, sizeof(struct mbox_msg), op,
			mbox_op_str[op]);
		return NULL;
	}

	memset(m, 0, sizeof(struct mbox_msg));

	kref_init(&m->refcnt);
	qdma_waitq_init(&m->waitq);

	hdr = &m->hdr;
	hdr->op = op;
	hdr->src = xdev->func_id;
#ifdef __QDMA_VF__
	hdr->dst = xdev->func_id_parent;
#endif

	return m;
}

void qdma_mbox_msg_cancel(struct xlnx_dma_dev *xdev, struct mbox_msg *m)
{
	struct qdma_mbox *mbox = &xdev->mbox;

	/* delete from mbox list */
	spin_lock_bh(&mbox->list_lock);
	list_del(&m->list);
	spin_unlock_bh(&mbox->list_lock);
}

int qdma_mbox_msg_send(struct xlnx_dma_dev *xdev, struct mbox_msg *m,
			bool wait_resp, u8 resp_op, unsigned int timeout_ms)
{
	struct qdma_mbox *mbox = &xdev->mbox;
	struct mbox_msg_hdr *hdr = &m->hdr;


	m->wait_resp = wait_resp ? 1 : 0;
	m->wait_op = resp_op;


	if (unlikely(xlnx_dma_device_flag_check(mbox->xdev, XDEV_FLAG_OFFLINE))
		&& (hdr->op != MBOX_OP_BYE))
		/* allow BYE message even if the device is going down */
		return -ENODEV;

	/* queue up to ensure order */
	spin_lock_bh(&mbox->list_lock);
	list_add_tail(&m->list, &mbox->tx_todo_list);
	spin_unlock_bh(&mbox->list_lock);

	/* kick start the tx */
	queue_work(mbox->workq, &mbox->tx_work);

	if (!wait_resp)
		return 0;

	qdma_waitq_wait_event_timeout(m->waitq, hdr->op == resp_op,
			msecs_to_jiffies(timeout_ms));

	if (hdr->op != resp_op) {
		/* delete from mbox list */
		spin_lock_bh(&mbox->list_lock);
		list_del(&m->list);
		spin_unlock_bh(&mbox->list_lock);

		pr_err("%s mbox timed out. op 0x%x, %s, timeout %u ms.\n",
				xdev->conf.name, hdr->op, mbox_op_str[hdr->op],
				timeout_ms);

		return -EPIPE;
	}

	if (hdr->status) {
		pr_err("%s mbox msg op failed %d. op 0x%x, %s.\n",
				xdev->conf.name, hdr->status, hdr->op,
				mbox_op_str[hdr->op]);
		return hdr->status;
	}

	return 0;
}

/*
 * mbox rx message processing
 */
#ifdef __QDMA_VF__

void dump_rx_pend_list(struct qdma_mbox *mbox)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	struct mbox_msg *_msg = NULL, *_tmp = NULL;

	spin_lock_bh(&mbox->list_lock);
	list_for_each_entry_safe(_msg, _tmp, &mbox->rx_pend_list, list) {
		pr_debug("m_snd(%u) : wait_op/src/dst = %x/%u/%u\n",
				xdev->func_id, _msg->wait_op,
				_msg->hdr.src, _msg->hdr.dst);
	}
	spin_unlock_bh(&mbox->list_lock);
}

static int mbox_rcv_one_msg(struct qdma_mbox *mbox)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	struct mbox_msg *m_rcv = &mbox->rx;
	struct mbox_msg_hdr *hdr_rcv = &m_rcv->hdr;
	struct mbox_msg *m_snd = NULL;

	u8 op = hdr_rcv->op;

	if (xdev->func_id == xdev->func_id_parent) {
		/* fill in VF's func_id */
		xdev->func_id = hdr_rcv->dst;
		xdev->func_id_parent = hdr_rcv->src;
	}

	spin_lock_bh(&mbox->list_lock);
	if (!list_empty(&mbox->rx_pend_list))
		m_snd = list_first_entry(&mbox->rx_pend_list, struct mbox_msg,
					list);
	spin_unlock_bh(&mbox->list_lock);


	if (!m_snd || op != m_snd->wait_op) {
		if (op != MBOX_OP_HELLO_RESP) {
			pr_err("%s: unexpected op 0x%x, %s.\n",
					xdev->conf.name, hdr_rcv->op,
					mbox_op_str[hdr_rcv->op]);
			if (m_snd)
				pr_err("m_snd : wait_op/src/dst = %x/%u/%u\n",
						m_snd->wait_op,
						m_snd->hdr.src,
						m_snd->hdr.dst);
			dump_rx_pend_list(mbox);
		}
		return 0;
	}

	/* a matching request is found */
	spin_lock_bh(&mbox->list_lock);
	list_del(&m_snd->list);
	spin_unlock_bh(&mbox->list_lock);

	memcpy(m_snd->raw, m_rcv->raw, sizeof(u32) * MBOX_MSG_REG_MAX);
	/* wake up anyone waiting on the response */
	qdma_waitq_wakeup(&m_snd->waitq);

	return 0;
}

#else
static int mbox_rcv_one_msg(struct qdma_mbox *mbox)
{
	struct mbox_msg *m = &mbox->rx;
	struct mbox_msg_hdr *hdr = &m->hdr;
	struct xlnx_dma_dev *xdev = mbox->xdev;
	u8 op = hdr->op;
	struct mbox_msg *m_resp;
	struct mbox_msg_hdr *hdr_resp;
	int rv = 0;
	struct qconf_entry *entry = NULL;

	pr_debug("%s, src 0x%x op 0x%x,%s.\n",
		xdev->conf.name, hdr->src, hdr->op, mbox_op_str[hdr->op]);

	switch (op) {
	case MBOX_OP_BYE:
	{
		unsigned int qbase = 0;
		unsigned int qmax = 0;

		hw_read_fmap(xdev, hdr->src, &qbase, &qmax);
		hw_init_qctxt_memory(xdev, qbase, qmax);

		hw_set_fmap(xdev, hdr->src, 0, 0);
		xdev_sriov_vf_offline(xdev, hdr->src);

		pr_debug("%s: clear 0x%x FMAP, Q 0x%x+0x%x.\n",
			xdev->conf.name, hdr->src, qbase, qmax);

		xdev_destroy_qconf(PCI_TYPE_VF, hdr->src);
		/* no response needed */
		return 0;
	}
	break;
	case MBOX_OP_HELLO:
	{
		xdev_sriov_vf_online(xdev, hdr->src);
		entry = xdev_create_qconf(PCI_TYPE_VF, hdr->src);
		if (entry) {
			int rsp_offset =
					sizeof(struct mbox_msg_hdr)/sizeof(u32);
			m->raw[rsp_offset] = entry->qmax;
			m->raw[rsp_offset + 1] = entry->qbase;
			pr_debug("qmax = %u qbase = %u m_resp->raw[%d] = %u, m_resp->raw[%d] = %u\n",
					entry->qmax, entry->qbase,
					rsp_offset, m->raw[rsp_offset],
					rsp_offset + 1, m->raw[rsp_offset + 1]);
		}
	}
	break;
	case MBOX_OP_FMAP:
	{
		struct mbox_msg_fmap *fmap = &m->fmap;
		int ret = xdev_set_qmax(PCI_TYPE_VF, hdr->src,
				fmap->qmax, &fmap->qbase);
		if (ret < 0) {
			rv = ret;
			pr_err("VF qmax set failed %s: set 0x%x QCONF, Q 0x%x+0x%x. ret = %d\n",
					xdev->conf.name, hdr->src,
					fmap->qbase, fmap->qmax, ret);
		} else {
			pr_debug("VF qmax set success %s: set 0x%x QCONF, Q 0x%x+0x%x.\n",
					xdev->conf.name, hdr->src,
					fmap->qbase, fmap->qmax);

			pr_debug("%s: set 0x%x FMAP, Q 0x%x+0x%x.\n",
					xdev->conf.name, hdr->src, fmap->qbase,
					fmap->qmax);

			hw_set_fmap(xdev, hdr->src, fmap->qbase, fmap->qmax);

			xdev_sriov_vf_fmap(xdev, hdr->src, fmap->qbase,
					fmap->qmax);
		}
	}
	break;
	case MBOX_OP_QCONF:
	{
		struct mbox_msg_fmap *fmap = &m->fmap;

		int ret = xdev_set_qmax(PCI_TYPE_VF, hdr->src,
				fmap->qmax, &fmap->qbase);
		if (ret < 0) {
			rv = ret;
			pr_err("VF qmax set failed %s: set 0x%x QCONF, Q 0x%x+0x%x.\n",
					xdev->conf.name, hdr->src,
					fmap->qbase, fmap->qmax);
		} else {
			pr_debug("VF qmax set success %s: set 0x%x QCONF, Q 0x%x+0x%x.\n",
					xdev->conf.name, hdr->src,
					fmap->qbase, fmap->qmax);
		}
	}
	break;

	case MBOX_OP_CSR:
	{
		struct mbox_msg_csr *csr = &m->csr;

		rv = qdma_csr_read(xdev, &csr->csr_info, 0);
	}
	break;
	case MBOX_OP_INTR_CTXT:
	{
		struct mbox_msg_intr_ctxt *ictxt = &m->intr_ctxt;

		pr_debug("%s, rcv 0x%x INTR_CTXT, programing the context\n",
			xdev->conf.name, hdr->src);
		rv = qdma_prog_intr_context(xdev, ictxt);
	}
	break;
	case MBOX_OP_QCTXT_CLR:
	{
		struct mbox_msg_qctxt *qctxt = &m->qctxt;

		 rv = qdma_descq_context_clear(xdev, qctxt->qid,
					qctxt->st, qctxt->c2h, 0);
	}
	break;
	case MBOX_OP_QCTXT_RD:
	{
		struct mbox_msg_qctxt *qctxt = &m->qctxt;

		rv = qdma_descq_context_read(xdev, qctxt->qid,
					qctxt->st, qctxt->c2h,
					&qctxt->context);
	}
	break;
	case MBOX_OP_QCTXT_WRT:
	{
		struct mbox_msg_qctxt *qctxt = &m->qctxt;

		pr_debug("%s, rcv 0x%x QCTXT_WRT, qid 0x%x.\n",
			xdev->conf.name, hdr->src, qctxt->qid);

		/* always clear the context first */
		rv = qdma_descq_context_clear(xdev, qctxt->qid,
					qctxt->st, qctxt->c2h, 1);
		if (rv < 0) {
			pr_err("%s, 0x%x QCTXT_WRT, qid 0x%x, clr failed %d.\n",
				xdev->conf.name, hdr->src, qctxt->qid,
				rv);
			break;
		}

		rv = qdma_descq_context_program(xdev, qctxt->qid,
					qctxt->st, qctxt->c2h,
					&qctxt->context);
	}
	break;
	default:
		pr_info("%s: rcv mbox UNKNOWN op 0x%x.\n",
			xdev->conf.name, hdr->op);
		print_hex_dump(KERN_INFO, "mbox rcv: ",
				DUMP_PREFIX_OFFSET, 16, 1, (void *)hdr,
				64, false);
		return -EINVAL;
	break;
	}

	/* respond */
	m_resp = qdma_mbox_msg_alloc(xdev, op);
	if (!m_resp)
		return -ENOMEM;

	hdr_resp = &m_resp->hdr;

	memcpy(m_resp->raw, m->raw, sizeof(u32) * MBOX_MSG_REG_MAX);

	hdr_resp->op |= MBOX_MSG_OP_PF_MASK;
	hdr_resp->dst = hdr->src;
	hdr_resp->src = xdev->func_id;

	hdr_resp->status = rv ? -1 : 0;

	qdma_mbox_msg_send(xdev, m_resp, 0, 0, 0);

	return rv;
}
#endif

/*
 * tx & rx workqueue handler
 */
static void mbox_tx_work(struct work_struct *work)
{
	struct qdma_mbox *mbox = container_of(work, struct qdma_mbox, tx_work);

	spin_lock_bh(&mbox->list_lock);
	while (!list_empty(&mbox->tx_todo_list)) {
		struct mbox_msg *m = list_first_entry(&mbox->tx_todo_list,
						struct mbox_msg, list);
		if (mbox_hw_send(mbox, m) == 0) {
			/* message sent */
			list_del(&m->list);

			/* response needed */
			if (m->wait_resp)
				list_add_tail(&m->list, &mbox->rx_pend_list);
			else
				qdma_mbox_msg_free(m);
		} else
			break;
	}
	spin_unlock_bh(&mbox->list_lock);
}

static inline void mbox_timer_stop(struct qdma_mbox *mbox)
{
	del_timer(&mbox->timer);
}

static inline void mbox_timer_start(struct qdma_mbox *mbox)
{
	struct timer_list *timer = &mbox->timer;

	qdma_timer_start(timer, MBOX_TIMER_INTERVAL);
}

static void mbox_rx_work(struct work_struct *work)
{
	struct qdma_mbox *mbox = container_of(work, struct qdma_mbox, rx_work);
	struct xlnx_dma_dev *xdev = mbox->xdev;
	struct mbox_msg *m = &mbox->rx;
	int rv;

#ifndef __QDMA_VF__
	/* clear the ack status */
	mbox_pf_hw_clear_ack(mbox->xdev);
#endif

	rv = mbox_hw_rcv(mbox, m);
	while (rv == 0) {
		if (unlikely(xlnx_dma_device_flag_check(xdev,
						XDEV_FLAG_OFFLINE)))
			break;
		else if (mbox_rcv_one_msg(mbox) == -EINVAL)
			break;

		rv = mbox_hw_rcv(mbox, m);
	}

	if (rv == -EPIPE) {
		pr_info("%s: rcv'ed all zero mbox msg, status 0x%x=0x%x. disable mbox processing.\n",
			xdev->conf.name, MBOX_BASE + MBOX_FN_STATUS,
			__read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS));
		mbox_timer_stop(mbox);
	} else if (xlnx_dma_device_flag_check(xdev, XDEV_FLAG_OFFLINE))
		mbox_timer_stop(mbox);
	else
		mbox_timer_start(mbox);
}

/*
 * non-interrupt mode: use timer for periodic checking of new messages
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
static void mbox_rx_timer_handler(struct timer_list *t)
#else
static void mbox_rx_timer_handler(unsigned long arg)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
	struct qdma_mbox *mbox = from_timer(mbox, t, timer);
#else
	struct qdma_mbox *mbox = (struct qdma_mbox *)arg;
#endif

	queue_work(mbox->workq, &mbox->rx_work);
}

/*
 * mailbox initialization and cleanup
 */
void qdma_mbox_stop(struct xlnx_dma_dev *xdev)
{
	mbox_timer_stop(&xdev->mbox);
}

void qdma_mbox_start(struct xlnx_dma_dev *xdev)
{
	mbox_timer_start(&xdev->mbox);
}

void qdma_mbox_cleanup(struct xlnx_dma_dev *xdev)
{
	struct qdma_mbox *mbox = &xdev->mbox;

	mbox_timer_stop(mbox);

	if (mbox->workq) {
		flush_workqueue(mbox->workq);
		destroy_workqueue(mbox->workq);
	}
}

int qdma_mbox_init(struct xlnx_dma_dev *xdev)
{
	struct qdma_mbox *mbox = &xdev->mbox;
	struct timer_list *timer = &mbox->timer;
	struct mbox_msg m;
	char name[80];

	/* ack any received messages in the Q */
#ifdef __QDMA_VF__
	u32 v;

	v = __read_reg(xdev, MBOX_BASE + MBOX_FN_STATUS);
	if (!(v & M_MBOX_FN_STATUS_IN_MSG))
		__write_reg(xdev, MBOX_BASE + MBOX_FN_CMD, F_MBOX_FN_CMD_RCV);
#elif defined(CONFIG_PCI_IOV)
	mbox_pf_hw_clear_ack(xdev);
#endif

	mbox->xdev = xdev;

	spin_lock_init(&mbox->lock);
	spin_lock_init(&mbox->list_lock);
	INIT_LIST_HEAD(&mbox->tx_todo_list);
	INIT_LIST_HEAD(&mbox->rx_pend_list);
	INIT_WORK(&mbox->tx_work, mbox_tx_work);
	INIT_WORK(&mbox->rx_work, mbox_rx_work);

	snprintf(name, 80, "%s_mbox_wq", xdev->conf.name);
		mbox->workq = create_singlethread_workqueue(name);

		if (!mbox->workq) {
			pr_info("%s OOM, mbox workqueue.\n", xdev->conf.name);
			goto cleanup;
		}

	/* read & discard whatever in the incoming message buffer */
#ifdef __QDMA_VF__
	mbox_hw_rcv(mbox, &m);
#else
{
	int i;

	for (i = 0; i < 256; i++)
		mbox_hw_rcv(mbox, &m);
}
#endif

	qdma_timer_setup(timer, mbox_rx_timer_handler, mbox);

	return 0;

cleanup:
	qdma_mbox_cleanup(xdev);
	return -ENOMEM;
}
