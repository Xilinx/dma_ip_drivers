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
#include "qdma_intr.h"
#include "qdma_mbox.h"

#define MBOX_TIMER_INTERVAL	(1)

#ifdef __QDMA_VF__
#define QDMA_DEV QDMA_DEV_VF
#else
#define QDMA_DEV QDMA_DEV_PF
#endif

static int mbox_hw_send(struct qdma_mbox *mbox, struct mbox_msg *m)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	int rv;

	spin_lock_bh(&mbox->hw_tx_lock);
	rv = qdma_mbox_send(xdev, QDMA_DEV, m->raw);
	spin_unlock_bh(&mbox->hw_tx_lock);

	return rv;
}

static int mbox_hw_rcv(struct qdma_mbox *mbox, struct mbox_msg *m)
{
	struct xlnx_dma_dev *xdev = mbox->xdev;
	int rv;

	spin_lock_bh(&mbox->hw_rx_lock);
	memset(m->raw, 0, MBOX_MSG_REG_MAX *
	       sizeof(uint32_t));
	rv = qdma_mbox_rcv(xdev, QDMA_DEV, m->raw);
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

struct mbox_msg *qdma_mbox_msg_alloc(void)
{
	struct mbox_msg *m = kzalloc(sizeof(struct mbox_msg), GFP_KERNEL);

	if (!m)
		return NULL;

	kref_init(&m->refcnt);
	qdma_waitq_init(&m->waitq);

	return m;
}

int qdma_mbox_msg_send(struct xlnx_dma_dev *xdev, struct mbox_msg *m,
			bool wait_resp, unsigned int timeout_ms)
{
	struct qdma_mbox *mbox = &xdev->mbox;

	m->resp_op_matched = 0;
	m->wait_resp = wait_resp ? 1 : 0;
	m->retry_cnt = (timeout_ms / 1000) + 1;

#if defined(__QDMA_VF__)
	if (xdev->reset_state == RESET_STATE_INVALID)
		return -EINVAL;
#endif
	/* queue up to ensure order */
	spin_lock_bh(&mbox->list_lock);
	list_add_tail(&m->list, &mbox->tx_todo_list);
	spin_unlock_bh(&mbox->list_lock);

	/* kick start the tx */
	queue_work(mbox->workq, &mbox->tx_work);

	if (!wait_resp)
		return 0;

	qdma_waitq_wait_event_timeout(m->waitq, m->resp_op_matched,
			msecs_to_jiffies(timeout_ms));

	if (!m->resp_op_matched) {
		/* delete from mbox list */
		spin_lock_bh(&mbox->list_lock);
		list_del(&m->list);
		spin_unlock_bh(&mbox->list_lock);

		pr_err("%s mbox timed out. timeout %u ms.\n",
				xdev->conf.name, timeout_ms);

		return -EPIPE;
	}

	return 0;
}

/*
 * mbox rx message processing
 */
#ifdef __QDMA_VF__

static int mbox_rcv_one_msg(struct qdma_mbox *mbox)
{
	struct mbox_msg *m_rcv = &mbox->rx;
	struct mbox_msg *m_snd = NULL, *tmp1 = NULL, *tmp2 = NULL;
	struct mbox_msg *m_resp = NULL;
	int rv = 0;

	if (mbox->xdev->func_id == mbox->xdev->func_id_parent) {
		mbox->xdev->func_id = qdma_mbox_vf_func_id_get(m_rcv->raw,
							QDMA_DEV);
		mbox->xdev->func_id_parent = qdma_mbox_vf_parent_func_id_get(
				m_rcv->raw);
	}
	spin_lock_bh(&mbox->list_lock);
	/* check if the sent request expired */
	if (!list_empty(&mbox->rx_pend_list)) {
		list_for_each_entry_safe(tmp1, tmp2, &mbox->rx_pend_list,
					 list) {
			if (qdma_mbox_is_msg_response(tmp1->raw, m_rcv->raw)) {
				m_snd = tmp1;
				m_snd->resp_op_matched = 1;
				break;
			}
		}
	}
	spin_unlock_bh(&mbox->list_lock);

	if (list_empty(&mbox->rx_pend_list)) {
		m_resp = qdma_mbox_msg_alloc();
		if (!m_resp) {
			pr_err("Failed to allocate mbox msg\n");
			return -ENOMEM;
		}
		rv = qdma_mbox_vf_rcv_msg_handler(m_rcv->raw, m_resp->raw);
		if (rv == QDMA_MBOX_VF_RESET) {
			qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
						QDMA_MBOX_MSG_TIMEOUT_MS);
			mbox->xdev->reset_state = RESET_STATE_RECV_PF_RESET_REQ;
			queue_work(mbox->xdev->workq,
					   &(mbox->xdev->reset_work));
		} else if (rv == QDMA_MBOX_PF_RESET_DONE) {
			mbox->xdev->reset_state =
				RESET_STATE_RECV_PF_RESET_DONE;
			qdma_waitq_wakeup(&mbox->xdev->wq);
			qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
						QDMA_MBOX_MSG_TIMEOUT_MS);


		}  else if (rv == QDMA_MBOX_PF_BYE) {
			qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
						QDMA_MBOX_MSG_TIMEOUT_MS);
			mbox->xdev->reset_state =
				RESET_STATE_RECV_PF_OFFLINE_REQ;
			queue_work(mbox->xdev->workq,
					   &(mbox->xdev->reset_work));
		} else {
			pr_err("func_id=0x%x parent=0x%x", mbox->xdev->func_id,
				   mbox->xdev->func_id_parent);
		}

	}

	if (m_snd) {
		/* a matching request is found */
		spin_lock_bh(&mbox->list_lock);
		list_del(&m_snd->list);
		spin_unlock_bh(&mbox->list_lock);
		memcpy(m_snd->raw, m_rcv->raw, MBOX_MSG_REG_MAX *
		       sizeof(uint32_t));
		/* wake up anyone waiting on the response */
		qdma_waitq_wakeup(&m_snd->waitq);
	}

	return 0;
}

#else
static int mbox_rcv_one_msg(struct qdma_mbox *mbox)
{
	struct mbox_msg *m = &mbox->rx;
	struct mbox_msg *m_resp = qdma_mbox_msg_alloc();
	struct mbox_msg *m_snd = NULL, *tmp1 = NULL, *tmp2 = NULL;
	int rv;

	if (!m_resp)
		return -ENOMEM;

	rv = qdma_mbox_pf_rcv_msg_handler(mbox->xdev,
					  mbox->xdev->dma_device_index,
					  mbox->xdev->func_id,
					  m->raw, m_resp->raw);

	if (rv == QDMA_MBOX_VF_OFFLINE ||
		rv == QDMA_MBOX_VF_RESET_BYE) {
#ifdef CONFIG_PCI_IOV
		uint8_t vf_func_id = qdma_mbox_vf_func_id_get(m->raw, QDMA_DEV);

		xdev_sriov_vf_offline(mbox->xdev, vf_func_id);
#endif
		if (rv == QDMA_MBOX_VF_RESET_BYE)
			qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
				QDMA_MBOX_MSG_TIMEOUT_MS);
		else
			qdma_mbox_msg_free(m_resp);
	} else if (rv == QDMA_MBOX_VF_ONLINE) {
#ifdef CONFIG_PCI_IOV
		uint8_t vf_func_id = qdma_mbox_vf_func_id_get(m->raw, QDMA_DEV);

		xdev_sriov_vf_online(mbox->xdev, vf_func_id);
#endif
		qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
					QDMA_MBOX_MSG_TIMEOUT_MS);
	} else if (rv == QDMA_MBOX_VF_RESET ||
			   rv == QDMA_MBOX_PF_RESET_DONE ||
			   rv == QDMA_MBOX_PF_BYE) {
		spin_lock_bh(&mbox->list_lock);
		if (!list_empty(&mbox->rx_pend_list)) {
			list_for_each_entry_safe(tmp1, tmp2,
						&mbox->rx_pend_list,
						list) {
				if (qdma_mbox_is_msg_response(tmp1->raw,
								m->raw)) {
					m_snd = tmp1;
					m_snd->resp_op_matched = 1;
					break;
				}
			}
		}
		if (m_snd) {
			list_del(&m_snd->list);
			memcpy(m_snd->raw, m->raw, MBOX_MSG_REG_MAX *
				   sizeof(uint32_t));
			qdma_waitq_wakeup(&m_snd->waitq);
			qdma_mbox_msg_free(m_snd);
		}
		qdma_mbox_msg_free(m_resp);
		spin_unlock_bh(&mbox->list_lock);
	} else
		qdma_mbox_msg_send(mbox->xdev, m_resp, 0,
					QDMA_MBOX_MSG_TIMEOUT_MS);

	return 0;
}
#endif

static inline void mbox_timer_stop(struct qdma_mbox *mbox)
{
	del_timer(&mbox->timer);
}

static inline void mbox_timer_start(struct qdma_mbox *mbox)
{
	struct timer_list *timer = &mbox->timer;

	qdma_timer_start(timer, MBOX_TIMER_INTERVAL);
}

/*
 * tx & rx workqueue handler
 */
static void mbox_tx_work(struct work_struct *work)
{
	struct qdma_mbox *mbox = container_of(work, struct qdma_mbox, tx_work);

	while (1) {
		struct mbox_msg *m = NULL;

		spin_lock_bh(&mbox->list_lock);
		if (list_empty(&mbox->tx_todo_list)) {
			spin_unlock_bh(&mbox->list_lock);
			break;
		}

		m = list_first_entry(&mbox->tx_todo_list,
						struct mbox_msg, list);

		spin_unlock_bh(&mbox->list_lock);

		if (mbox_hw_send(mbox, m) == 0) {
			mbox->send_busy = 0;
			spin_lock_bh(&mbox->list_lock);
			/* Msg tx successful, remove from list */
			list_del(&m->list);
			spin_unlock_bh(&mbox->list_lock);

			/* response needed */
			if (m->wait_resp) {
				spin_lock_bh(&mbox->list_lock);
				list_add_tail(&m->list, &mbox->rx_pend_list);
				spin_unlock_bh(&mbox->list_lock);
			} else
				qdma_mbox_msg_free(m);
		} else {
			if (!xlnx_dma_device_flag_check(mbox->xdev,
							XDEV_FLAG_OFFLINE)) {
				if (!m->wait_resp) {
					m->retry_cnt--;
					if (!m->retry_cnt) {
						spin_lock_bh(&mbox->list_lock);
						list_del(&m->list);
						spin_unlock_bh(
							&mbox->list_lock);
						qdma_mbox_msg_free(m);
						break;
					}
				}
				mbox->send_busy = 1;
				mbox_timer_start(mbox);
			} else
				qdma_mbox_msg_free(m);
			break;
		}
	}
}

static void mbox_rx_work(struct work_struct *work)
{
	struct qdma_mbox *mbox = container_of(work, struct qdma_mbox, rx_work);
	struct xlnx_dma_dev *xdev = mbox->xdev;
	struct mbox_msg *m = &mbox->rx;
	int rv;
	int mbox_stop = 0;

	rv = mbox_hw_rcv(mbox, m);
	while (rv == 0) {
		if (unlikely(xlnx_dma_device_flag_check(xdev,
						XDEV_FLAG_OFFLINE)))
			break;
		else if (mbox_rcv_one_msg(mbox) == -EINVAL)
			break;
		rv = mbox_hw_rcv(mbox, m);
	}

	if (rv == -QDMA_ERR_MBOX_ALL_ZERO_MSG) {
#ifdef __QDMA_VF__
		if (xdev->reset_state == RESET_STATE_IDLE) {
			mbox_stop = 1;
			pr_info("func_id=0x%x parent=0x%x %s: rcv'ed all zeros msg, disable mbox processing.\n",
				xdev->func_id, xdev->func_id_parent,
				xdev->conf.name);
		}
#else
		mbox_stop = 1;
		pr_info("PF func_id=0x%x %s: rcv'ed all zero mbox msg, disable mbox processing.\n",
			xdev->func_id, xdev->conf.name);
#endif

	} else if (xlnx_dma_device_flag_check(xdev, XDEV_FLAG_OFFLINE))
		mbox_stop = 1;

	if (mbox->rx_poll) {

		if (mbox_stop == 1)
			mbox_timer_stop(mbox);
		else
			mbox_timer_start(mbox);
	} else {
		if (mbox_stop != 0)
			qdma_mbox_disable_interrupts(xdev, QDMA_DEV);
	}
}

/*
 * non-interrupt mode: use timer for periodic checking of new messages
 */
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void mbox_timer_handler(struct timer_list *t)
#else
static void mbox_timer_handler(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct qdma_mbox *mbox = from_timer(mbox, t, timer);
#else
	struct qdma_mbox *mbox = (struct qdma_mbox *)arg;
#endif
	if (mbox->rx_poll)
		queue_work(mbox->workq, &mbox->rx_work);
	if (mbox->send_busy)
		queue_work(mbox->workq, &mbox->tx_work);
}

/*
 * mailbox initialization and cleanup
 */
void qdma_mbox_stop(struct xlnx_dma_dev *xdev)
{
	struct qdma_mbox *mbox = &xdev->mbox;
	uint8_t retry_count = 100;
	int rv = 0;

#ifndef __QDMA_VF__
	if (!xdev->dev_cap.mailbox_en)
		return;
#endif
	do {
		spin_lock_bh(&mbox->list_lock);
		if (list_empty(&mbox->tx_todo_list))
			break;
		spin_unlock_bh(&mbox->list_lock);
		mdelay(10); /* sleep 10ms for msgs to be sent or freed */
	} while (1);
	spin_unlock_bh(&mbox->list_lock);

	do {
		rv = qdma_mbox_out_status(xdev, QDMA_DEV);
		if (!rv)
			break;
		retry_count--;
		mdelay(10);
	} while (retry_count != 0);
	mbox_timer_stop(&xdev->mbox);
	pr_debug("func_id=%d retry_count=%d\n", xdev->func_id, retry_count);
	if ((xdev->version_info.device_type == QDMA_DEVICE_SOFT) &&
	(xdev->version_info.vivado_release >= QDMA_VIVADO_2019_1)) {
		if (!xdev->mbox.rx_poll)
			qdma_mbox_disable_interrupts(xdev, QDMA_DEV);
	}
}

void qdma_mbox_start(struct xlnx_dma_dev *xdev)
{

#ifndef __QDMA_VF__
	if (!xdev->dev_cap.mailbox_en)
		return;
#endif
	if (xdev->mbox.rx_poll)
		mbox_timer_start(&xdev->mbox);
	else
		qdma_mbox_enable_interrupts(xdev, QDMA_DEV);
}

void qdma_mbox_poll_start(struct xlnx_dma_dev *xdev)
{

#ifndef __QDMA_VF__
	if (!xdev->dev_cap.mailbox_en)
		return;
#endif
	xdev->mbox.rx_poll = 1;
	qdma_mbox_disable_interrupts(xdev, QDMA_DEV);
	mbox_timer_start(&xdev->mbox);
}

void qdma_mbox_cleanup(struct xlnx_dma_dev *xdev)
{
	struct qdma_mbox *mbox = &xdev->mbox;

	if (mbox->workq) {
		mbox_timer_stop(mbox);
		if (!xdev->mbox.rx_poll)
			qdma_mbox_disable_interrupts(xdev, QDMA_DEV);
		flush_workqueue(mbox->workq);
		destroy_workqueue(mbox->workq);
	}
}

int qdma_mbox_init(struct xlnx_dma_dev *xdev)
{
	struct qdma_mbox *mbox = &xdev->mbox;
	struct timer_list *timer = &mbox->timer;
#ifndef __QDMA_VF__
	int i;
#endif
	struct mbox_msg m;
	char name[80];
	int rv;

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
#ifndef __QDMA_VF__
	for (i = 0; i < 256; i++)
		rv = mbox_hw_rcv(mbox, &m);
#else
	rv = mbox_hw_rcv(mbox, &m);
#endif
	/* ack any received messages in the Q */
	qdma_mbox_hw_init(xdev, QDMA_DEV);
	if ((xdev->version_info.device_type == QDMA_DEVICE_SOFT) &&
	(xdev->version_info.vivado_release >= QDMA_VIVADO_2019_1)) {
		if ((xdev->conf.qdma_drv_mode != POLL_MODE) &&
			(xdev->conf.qdma_drv_mode != LEGACY_INTR_MODE)) {
			mbox->rx_poll = 0;
			qdma_mbox_enable_interrupts(xdev, QDMA_DEV);
		} else
			mbox->rx_poll = 1;
	} else
		mbox->rx_poll = 1;

	qdma_timer_setup(timer, mbox_timer_handler, mbox);

	return 0;

cleanup:
	qdma_mbox_cleanup(xdev);
	return -ENOMEM;
}
