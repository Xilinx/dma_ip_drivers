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

#include "qdma_descq.h"

#include <linux/kernel.h>
#include <linux/delay.h>

#include "qdma_device.h"
#include "qdma_intr.h"
#include "qdma_regs.h"
#include "qdma_thread.h"
#include "qdma_context.h"
#include "qdma_st_c2h.h"
#include "qdma_access_common.h"
#include "thread.h"
#include "qdma_ul_ext.h"
#include "version.h"
#ifdef ERR_DEBUG
#include "qdma_nl.h"
#endif

struct q_state_name q_state_list[] = {
	{Q_STATE_DISABLED, "disabled"},
	{Q_STATE_ENABLED, "cfg'ed"},
	{Q_STATE_ONLINE, "online"}
};

/*
 * dma transfer requests
 */
#ifdef DEBUG
static void sgl_dump(struct qdma_sw_sg *sgl, unsigned int sgcnt)
{
	struct qdma_sw_sg *sg = sgl;
	int i;

	pr_info("sgl 0x%p, sgcntt %u.\n", sgl, sgcnt);

	for (i = 0; i < sgcnt; i++, sg++)
		pr_info("%d, 0x%p, pg 0x%p,%u+%u, dma 0x%llx.\n",
			i, sg, sg->pg, sg->offset, sg->len, sg->dma_addr);
}
#endif

u64 rdtsc_gettime(void)
{
	unsigned int low, high;

	asm volatile("rdtscp" : "=a" (low), "=d" (high));
	return low | ((u64)high) << 32;
}

int qdma_sgl_find_offset(struct qdma_request *req, struct qdma_sw_sg **sg_p,
			unsigned int *sg_offset)
{
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);
	struct qdma_sw_sg *sg = req->sgl;
	unsigned int sgcnt = req->sgcnt;
	unsigned int offset = cb->offset;
	unsigned int len = 0;
	int i;

	if (req->count == 0) {
		*sg_p = sg;
		*sg_offset = 0;
		return 0;
	}


	for (i = 0;  i < sgcnt; i++, sg++) {
		len += sg->len;

		if (len == offset) {
			*sg_p = sg + 1;
			*sg_offset = 0;
			++i;
			break;
		} else if (len > offset) {
			*sg_p = sg;
			*sg_offset = sg->len - (len - offset);
			break;
		}
	}

	if (i < sgcnt)
		return i;

	return -EINVAL;
}

void qdma_update_request(void *q_hndl, struct qdma_request *req,
			unsigned int num_desc,
			unsigned int data_cnt,
			unsigned int sg_offset,
			void *sg)
{
	struct qdma_descq *descq = (struct qdma_descq *)q_hndl;
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);

	cb->desc_nr += num_desc;
	cb->offset += data_cnt;
	cb->sg_offset = sg_offset;
	cb->sg = sg;
	if (cb->offset >= req->count) {
		qdma_work_queue_del(descq, cb);
		list_add_tail(&cb->list, &descq->pend_list);
	}
}

static int descq_mm_n_h2c_cmpl_status(struct qdma_descq *descq);

static int descq_poll_mm_n_h2c_cmpl_status(struct qdma_descq *descq)
{
	enum qdma_drv_mode drv_mode = descq->xdev->conf.qdma_drv_mode;

	if ((drv_mode == POLL_MODE) || (drv_mode == AUTO_MODE)) {
		descq->proc_req_running = 1;
		return descq_mm_n_h2c_cmpl_status(descq);
	} else
		return 0;
}

static inline unsigned int incr_pidx(unsigned int pidx, unsigned int incr_val,
				     unsigned int ring_sz)
{
	pidx += incr_val;
	if (pidx >= ring_sz)
		pidx -= ring_sz;

	return pidx;
}

static inline int qdma_pidx_update(struct qdma_descq *descq,
				unsigned char force)
{
	int ret = 0;
	unsigned int desc_avail_max = descq->conf.rngsz - 1;

	/* interrupt for last update came, now submit descs that are setup */
	if (force) /* re-arm interrupt with pidx update */
		goto update;
	/* q is not very busy, go ahead and submit */
	if (descq->avail > (desc_avail_max - descq->conf.pidx_acc))
		goto update;

	/* accumulating descs to be submitted */
	if (descq->desc_pend < descq->conf.pidx_acc)
		goto exit_update;

update:
	ret = queue_pidx_update(descq->xdev, descq->conf.qidx,
			descq->conf.q_type, &descq->pidx_info);
	if (ret < 0) {
		pr_err("%s: Failed to update pidx\n",
				descq->conf.name);
		return -EINVAL;
	}

	descq->desc_pend = 0;

exit_update:
	return ret;
}

static ssize_t descq_mm_proc_request(struct qdma_descq *descq)
{
	int rv = 0;
	unsigned int desc_written = 0;
	unsigned int rngsz = descq->conf.rngsz;
	unsigned int pidx;
	u64 ep_addr = 0;
	struct qdma_mm_desc *desc;
	struct qdma_queue_conf *qconf = &descq->conf;
	unsigned char is_ul_ext = (qconf->desc_bypass &&
			qconf->fp_bypass_desc_fill) ? 1 : 0;
	u32 aperture = qconf->aperture_size ?
				 qconf->aperture_size : QDMA_DESC_BLEN_MAX;
	u8 keyhole_en = qconf->aperture_size ? 1 : 0;
	u64 ep_addr_max = 0;

	lock_descq(descq);
	/* process completion of submitted requests */
	if (descq->q_stop_wait) {
		descq_mm_n_h2c_cmpl_status(descq);
		unlock_descq(descq);
		return 0;
	}
	if (unlikely(descq->q_state != Q_STATE_ONLINE)) {
		unlock_descq(descq);
		return 0;
	}

	if (descq->proc_req_running) {
		unlock_descq(descq);
		return 0;
	}

	pidx = descq->pidx;
	desc = (struct qdma_mm_desc *)descq->desc + pidx;

	descq_poll_mm_n_h2c_cmpl_status(descq);

	while (!list_empty(&descq->work_list)) {
		struct qdma_sgt_req_cb *cb = list_first_entry(&descq->work_list,
						struct qdma_sgt_req_cb, list);
		struct qdma_request *req = (struct qdma_request *)cb;
		struct qdma_sw_sg *sg = req->sgl;
		unsigned int sg_offset = 0;
		unsigned int sg_max = req->sgcnt;
		struct qdma_mm_desc *desc_start = NULL;
		struct qdma_mm_desc *desc_end = NULL;
		unsigned int desc_max = descq->avail;
		unsigned int data_cnt = 0;
		unsigned int desc_cnt = 0;
		unsigned int len = 0;
		int i = 0;
		int rv;

		ep_addr_max = req->ep_addr + aperture - 1;

		/**
		 * When keyhole feature is enabled, the ep_addr
		 * cannot ever exceed the aperture size.
		 * The offset calculation is implemented below.
		 */
		if (!keyhole_en)
			ep_addr = req->ep_addr + cb->offset;
		else
			ep_addr = req->ep_addr + (cb->offset & (aperture - 1));

		if (!desc_max) {
			descq_poll_mm_n_h2c_cmpl_status(descq);
			desc_max = descq->avail;
		}

		if (!desc_max)
			break;

		if (is_ul_ext) {
			int desc_consumed =
				qconf->fp_bypass_desc_fill(descq,
						   QDMA_Q_MODE_MM,
						   (qconf->q_type == Q_C2H) ?
						   QDMA_Q_DIR_C2H :
						   QDMA_Q_DIR_H2C,
						   req);
			if (desc_consumed > 0)
				desc_cnt += desc_consumed;
			goto update_pidx;
		}
		rv = qdma_sgl_find_offset(req, &sg, &sg_offset);
		if (rv < 0) {
			pr_info("descq %s, req 0x%p, OOR %u/%u, %d/%u.\n",
				descq->conf.name, req, cb->offset,
				req->count, rv, sg_max);
			qdma_update_request(descq, req, 0, 0, 0, NULL);
			continue;
		}
		i = rv;
		pr_debug("%s, req 0x%p, offset %u/%u -> sg %d, 0x%p,%u.\n",
			descq->conf.name, req, cb->offset, req->count,
			i, sg, sg_offset);

		desc_start = desc;
		for (; i < sg_max && desc_cnt < desc_max; i++, sg++) {
			unsigned int tlen = sg->len;
			dma_addr_t src_addr = sg->dma_addr;
			unsigned int pg_off = sg->offset;

			pr_debug("desc %u/%u, sgl %d, len %u,%u, offset %u.\n",
				desc_cnt, desc_max, i, len, tlen, sg_offset);

			desc->flag_len = 0;
			if (sg_offset) {
				tlen -= sg_offset;
				src_addr += sg_offset;
				pg_off += sg_offset;
				sg_offset = 0;
			}

			do {
				unsigned int len = min_t(unsigned int, tlen,
							aperture);

				if (keyhole_en) {
					if (ep_addr > ep_addr_max)
						ep_addr = req->ep_addr;

					if (ep_addr + len > ep_addr_max)
						len = ep_addr_max - ep_addr + 1;
				}

				desc_end = desc;
				sg_offset += len;

				desc->rsvd1 = 0UL;
				desc->rsvd0 = 0U;

				if (descq->conf.q_type == Q_C2H) {
					desc->src_addr = ep_addr;
					desc->dst_addr = src_addr;
				} else {
					desc->dst_addr = ep_addr;
					desc->src_addr = src_addr;
				}

				desc->flag_len = len;
				desc->flag_len |= (1 << S_DESC_F_DV);
				ep_addr += len;
				data_cnt += len;
				src_addr += len;
				tlen -= len;
				pg_off += len;

				if (++pidx == rngsz) {
					pidx = 0;
					desc =
					(struct qdma_mm_desc *)descq->desc;
				} else {
					desc++;
				}

				desc_cnt++;
				if (desc_cnt == desc_max)
					break;
			} while (tlen);
			if (!tlen)
				sg_offset = 0;
		}
		if (i == sg_max) {
			sg = NULL;
			sg_offset = 0;
		}

		if (!desc_end || !desc_start) {
			pr_info("descq %s, %u, pidx 0x%x, desc 0x%p ~ 0x%p.\n",
				descq->conf.name, descq->qidx_hw, pidx,
				desc_start, desc_end);
			break;
		}

		/* set eop */
		desc_end->flag_len |= (1 << S_DESC_F_EOP);
		/* set sop */
		desc_start->flag_len |= (1 << S_DESC_F_SOP);
		qdma_update_request(descq, req, desc_cnt, data_cnt, sg_offset,
				    sg);
		descq->pidx = pidx;
		descq->avail -= desc_cnt;
update_pidx:

		desc_written += desc_cnt;

		pr_debug("descq %s, +%u,%u, avail %u, ep_addr 0x%llx + 0x%x(%u).\n",
			descq->conf.name, desc_cnt, pidx, descq->avail,
			req->ep_addr, data_cnt, data_cnt);

	}

	if (desc_written) {
		descq->pend_list_empty = 0;
		descq->pidx_info.pidx = descq->pidx;
		rv = queue_pidx_update(descq->xdev, descq->conf.qidx,
				descq->conf.q_type, &descq->pidx_info);
		if (unlikely(rv < 0)) {
			pr_err("%s: Failed to update pidx\n",
					descq->conf.name);
			unlock_descq(descq);
			return -EINVAL;
		}
		descq_poll_mm_n_h2c_cmpl_status(descq);
	}

	descq->proc_req_running = 0;
	unlock_descq(descq);

	if (desc_written && descq->cmplthp)
		qdma_kthread_wakeup(descq->cmplthp);

	return 0;
}
/**
 * This function is called directly while sending packet or as  interrupt
 * service when any pending request or pending descriptor is present.
 */
static ssize_t descq_proc_st_h2c_request_qep(struct qdma_descq *descq)
{
	int ret = 0;
	struct qdma_request *req;
	unsigned int desc_written = 0;
	unsigned int desc_cnt = 0;

	/* exit as packet have been queued and can be processes as part of who
	 * ever holding the lock or during interrupt service
	 */
	if (!(spin_trylock_bh(&(descq)->lock)))
		return 0;

	/* process completion of submitted requests */
	if (unlikely(descq->q_stop_wait)) {
		descq_mm_n_h2c_cmpl_status(descq);
		unlock_descq(descq);
		return 0;
	}

	if (unlikely(descq->q_state != Q_STATE_ONLINE)) {
		unlock_descq(descq);
		return 0;
	}

setup_desc:
	while (qdma_work_queue_len(descq) && descq->avail) {
		req = qdma_work_queue_first_entry(descq);
		desc_cnt = descq->conf.fp_bypass_desc_fill(descq,
			QDMA_Q_MODE_ST, QDMA_Q_DIR_H2C, req);

		if (unlikely(desc_cnt == 0))
			break;

		desc_written += desc_cnt;

		if (desc_written >= descq->conf.pidx_acc)
			break;
	}

	if (unlikely(!desc_written)) {
		/* packet queued while holding lock and no interrupt pending */
		if (unlikely(qdma_work_queue_len(descq) &&
				descq->avail == descq->conf.rngsz - 1)) {
			unlock_descq(descq);
			descq_proc_st_h2c_request_qep(descq);
			return 0;
		}
		unlock_descq(descq);
		return 0;
	}

	descq->desc_pend += desc_written;
	descq->pidx_info.pidx = descq->pidx;

	ret = qdma_pidx_update(descq, 0);
	if (unlikely(ret < 0)) {
		pr_err("%s: Failed to update pidx\n",
				descq->conf.name);
		unlock_descq(descq);
		return -EINVAL;
	}

	if (descq->work_req_pend && (desc_written >= descq->conf.pidx_acc)) {
		desc_written = 0;
		goto setup_desc;
	}

	unlock_descq(descq);

	return ret;
}


static ssize_t descq_proc_st_h2c_request(struct qdma_descq *descq)
{
	int ret = 0;
	struct qdma_h2c_desc *desc;
	u8 *tx_time_pkt_offset = NULL;
	unsigned int rngsz = descq->conf.rngsz;
	unsigned int pidx;
	unsigned int desc_written = 0;
	struct qdma_queue_conf *qconf = &descq->conf;
	unsigned char is_ul_ext = (qconf->desc_bypass &&
			qconf->fp_bypass_desc_fill) ? 1 : 0;
	struct qdma_dev *qdev = NULL;

	lock_descq(descq);
	/* process completion of submitted requests */
	if (descq->q_stop_wait) {
		descq_mm_n_h2c_cmpl_status(descq);
		unlock_descq(descq);
		return 0;
	}
	if (unlikely(descq->q_state != Q_STATE_ONLINE)) {
		unlock_descq(descq);
		return 0;
	}

	if (descq->proc_req_running) {
		unlock_descq(descq);
		return 0;
	}

	qdev = xdev_2_qdev(descq->xdev);
	/* service completion first */
	descq_poll_mm_n_h2c_cmpl_status(descq);

	pidx = descq->pidx;
	desc = (struct qdma_h2c_desc *)descq->desc + pidx;
	while (!list_empty(&descq->work_list)) {
		struct qdma_sgt_req_cb *cb = list_first_entry(&descq->work_list,
						struct qdma_sgt_req_cb, list);
		struct qdma_request *req = (struct qdma_request *)cb;
		struct qdma_sw_sg *sg = req->sgl;
		unsigned int sg_offset = 0;
		unsigned int sg_max = req->sgcnt;
		unsigned int desc_max = descq->avail;
		unsigned int data_cnt = 0;
		unsigned int desc_cnt = 0;
		unsigned int pktsz = req->ep_addr ?
				min_t(unsigned int, req->ep_addr, PAGE_SIZE) :
				PAGE_SIZE;
		int i = 0;
		int rv;

		if (!desc_max) {
			descq_poll_mm_n_h2c_cmpl_status(descq);
			desc_max = descq->avail;
		}

		if (!desc_max)
			break;

#ifdef DEBUG
		pr_info("%s, req %u.\n", descq->conf.name, req->count);
		sgl_dump(req->sgl, sg_max);
#endif
		if (is_ul_ext) {
			int desc_consumed =
				qconf->fp_bypass_desc_fill(descq,
							   QDMA_Q_MODE_ST,
							   QDMA_Q_DIR_H2C,
							   req);
			if (desc_consumed > 0)
				desc_cnt += desc_consumed;
			goto update_pidx;
		}

		rv = qdma_sgl_find_offset(req, &sg,
					      &sg_offset);

		if (rv < 0) {
			pr_err("descq %s, req 0x%p, OOR %u/%u, %d/%u.\n",
			descq->conf.name, req, cb->offset,
			req->count, rv, sg_max);
			qdma_update_request(descq, req, 0, 0, 0, NULL);
			continue;
		}
		i = rv;
		pr_debug("%s, req 0x%p, offset %u/%u -> sg %d, 0x%p,%u.\n",
			descq->conf.name, req, cb->offset, req->count,
			i, sg, sg_offset);
		desc->flags = 0;
		desc->cdh_flags = 0;

		for (; i < sg_max && desc_cnt < desc_max; i++, sg++) {
			unsigned int tlen = sg->len;
			dma_addr_t src_addr = sg->dma_addr;

			if (sg_offset) {
				tlen -= sg_offset;
				src_addr += sg_offset;
				sg_offset = 0;
			}

			do { /* to support zero byte transfer */
				unsigned int len = min_t(unsigned int, tlen,
							 pktsz);

				sg_offset += len;
				desc->src_addr = src_addr;
				desc->len = len;
				desc->pld_len = len;
				desc->cdh_flags |= S_H2C_DESC_F_ZERO_CDH;
				data_cnt += len;
				src_addr += len;
				tlen -= len;
				tx_time_pkt_offset =
					(u8 *)(page_address(sg->pg) +
							sg->offset);

				/* Setting SOP/EOP for the dummy bypass case */
				if (descq->conf.desc_bypass) {
					if (i == 0)
						desc->flags |= S_H2C_DESC_F_SOP;

					if ((i == sg_max - 1))
						desc->flags |= S_H2C_DESC_F_EOP;
				}

#if 0
				pr_info("desc %d, pidx 0x%x, data_cnt %u, cb off %u:\n",
					i, pidx, data_cnt, cb->offset);
				print_hex_dump(KERN_INFO, "desc",
					       DUMP_PREFIX_OFFSET, 16, 1,
					       (void *)desc, 16, false);
#endif

				if (++pidx == rngsz) {
					pidx = 0;
					desc =
					(struct qdma_h2c_desc *)descq->desc;
				} else {
					desc++;
					desc->flags = 0;
					desc->cdh_flags = 0;
				}

				desc_cnt++;
				if (desc_cnt == desc_max)
					break;
			} while (tlen);
			if (!tlen)
				sg_offset = 0;

		}
		if (i == sg_max) {
			sg = NULL;
			sg_offset = 0;
		}
		qdma_update_request(descq, req, desc_cnt, data_cnt, sg_offset,
				    sg);
		descq->pidx = pidx;
		descq->avail -= desc_cnt;
update_pidx:
		if (!desc_cnt)
			break;
		desc_written += desc_cnt;

		pr_debug("descq %s, +%u,%u, avail %u, 0x%x(%u), cb off %u.\n",
			descq->conf.name, desc_cnt, pidx, descq->avail,
			data_cnt, data_cnt, cb->offset);

	}

	if (desc_written) {
		descq->pend_list_empty = 0;
		descq->pidx_info.pidx = descq->pidx;
		if (descq->conf.ping_pong_en) {
			if (tx_time_pkt_offset != NULL) {
				u64 tx_time = rdtsc_gettime();

				qdev->c2h_descq[qconf->qidx].ping_pong_tx_time =
						tx_time;
				memcpy(tx_time_pkt_offset, &tx_time,
					   sizeof(tx_time));
			} else
				pr_err("Err: Tx Time Offset is NULL\n");
		}


		ret = queue_pidx_update(descq->xdev, descq->conf.qidx,
				descq->conf.q_type, &descq->pidx_info);
		if (ret < 0) {
			pr_err("%s: Failed to update pidx\n",
					descq->conf.name);
			unlock_descq(descq);
			return -EINVAL;
		}
		descq_poll_mm_n_h2c_cmpl_status(descq);
	}

	descq->proc_req_running = 0;

	if (desc_written && descq->cmplthp)
		qdma_kthread_wakeup(descq->cmplthp);

	unlock_descq(descq);

	return 0;
}

/*
 * descriptor Queue
 */
static inline int get_desc_size(struct qdma_descq *descq)
{
	if (descq->conf.desc_bypass && (descq->conf.sw_desc_sz == DESC_SZ_64B))
		return DESC_SZ_64B_BYTES;
	if (!descq->conf.st)
		return (int)sizeof(struct qdma_mm_desc);

	if (descq->conf.q_type == Q_C2H)
		return (int)sizeof(struct qdma_c2h_desc);

	return (int)sizeof(struct qdma_h2c_desc);
}

static inline int get_desc_cmpl_status_size(struct qdma_descq *descq)
{
	return (int)sizeof(struct qdma_desc_cmpl_status);
}

static inline void desc_ring_free(struct xlnx_dma_dev *xdev, int ring_sz,
			int desc_sz, int cs_sz, u8 *desc, dma_addr_t desc_bus)
{
	unsigned int len = ring_sz * desc_sz + cs_sz;

	pr_debug("free %u(0x%x)=%d*%u+%d, 0x%p, bus 0x%llx.\n",
		len, len, desc_sz, ring_sz, cs_sz, desc, desc_bus);

	dma_free_coherent(&xdev->conf.pdev->dev,
			((size_t)ring_sz * desc_sz + cs_sz),
			desc, desc_bus);
}

static void *desc_ring_alloc(struct xlnx_dma_dev *xdev, int ring_sz,
			int desc_sz, int cs_sz, dma_addr_t *bus, u8 **cs_pp)
{
	unsigned int len = ring_sz * desc_sz + cs_sz;
	u8 *p = dma_alloc_coherent(&xdev->conf.pdev->dev, len, bus, GFP_KERNEL);

	if (!p) {
		pr_err("%s, OOM, sz ring %d, desc %d, cmpl status sz %d.\n",
			xdev->conf.name, ring_sz, desc_sz, cs_sz);
		return NULL;
	}

	*cs_pp = p + ring_sz * desc_sz;
	memset(p, 0, len);

	pr_debug("alloc %u(0x%x)=%d*%u+%d, 0x%p, bus 0x%llx, cmpl status 0x%p.\n",
		len, len, desc_sz, ring_sz, cs_sz, p, *bus, *cs_pp);

	return p;
}

static void desc_alloc_irq(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	unsigned long flags;
	int i, idx = 0, min = -1;

	if (!xdev->num_vecs)
		return;

	/** Pick the MSI-X vector that currently has the fewest queues
	 * on PF0, vector#0 is dedicated for Error interrupts and
	 * vector #1 is dedicated for User interrupts
	 * For all other PFs, vector#0 is dedicated for User interrupts
	 */

	idx = xdev->dvec_start_idx;
	if (xdev->conf.qdma_drv_mode == DIRECT_INTR_MODE) {
		for (i = xdev->dvec_start_idx; i < xdev->num_vecs; i++) {
			struct intr_info_t *intr_info_list =
					&xdev->dev_intr_info_list[i];

			spin_lock_irqsave(&intr_info_list->vec_q_list,
					flags);
			if (!intr_info_list->intr_list_cnt) {
				spin_unlock_irqrestore(
						&intr_info_list->vec_q_list,
						flags);
				idx = i;
				break;
			}
			if (min < 0)
				min = intr_info_list->intr_list_cnt;
			if (intr_info_list->intr_list_cnt < min) {
				min = intr_info_list->intr_list_cnt;
				idx = i;
			}

			spin_unlock_irqrestore(&intr_info_list->vec_q_list,
					flags);
		}
	}
	descq->intr_id = idx;
	pr_debug("descq->intr_id = %d allocated to qidx = %d\n",
		descq->intr_id, descq->conf.qidx);
}

/*
 * writeback handling
 */
static int descq_mm_n_h2c_cmpl_status(struct qdma_descq *descq)
{
	int rv = 0;
	unsigned int cidx, cidx_hw;
	unsigned int cr;

	pr_debug("descq 0x%p, %s, pidx %u, cidx %u.\n",
		descq, descq->conf.name, descq->pidx, descq->cidx);

	if (descq->pidx == descq->cidx) { /* queue empty? */
		pr_debug("descq %s empty, return.\n", descq->conf.name);
		return 0;
	}

	cidx = descq->cidx;
#ifdef __READ_ONCE_DEFINED__
	cidx_hw = READ_ONCE(((struct qdma_desc_cmpl_status *)
				descq->desc_cmpl_status)->cidx);
#else
	cidx_hw = ((struct qdma_desc_cmpl_status *)
					descq->desc_cmpl_status)->cidx;
	dma_rmb();
#endif

	if (cidx_hw == cidx) /* no new writeback? */
		return 0;

	/* completion credits */
	cr = (cidx_hw < cidx) ? (descq->conf.rngsz - cidx) + cidx_hw :
				cidx_hw - cidx;

	pr_debug("%s descq %s, cidx 0x%x -> 0x%x, avail 0x%x + 0x%x.\n",
			__func__, descq->conf.name, cidx,
			cidx_hw, descq->avail, cr);

	descq->cidx = cidx_hw;
	descq->avail += cr;
	descq->credit += cr;

	incr_cmpl_desc_cnt(descq, cr);

	/* completes requests */
	pr_debug("%s %s, 0x%p, credit %u + %u.\n",
			__func__, descq->conf.name, descq, cr, descq->credit);

	cr = descq->credit;

	while (!list_empty(&descq->pend_list)) {
		struct qdma_sgt_req_cb *cb = list_first_entry(&descq->pend_list,
						struct qdma_sgt_req_cb, list);

		pr_debug("%s, 0x%p, cb 0x%p, desc_nr %u, credit %u.\n",
			descq->conf.name, descq, cb, cb->desc_nr, cr);

		if (cr >= cb->desc_nr) {
			pr_debug("%s, cb 0x%p done, credit %u > %u.\n",
				descq->conf.name, cb, cr, cb->desc_nr);
			cr -= cb->desc_nr;
			qdma_sgt_req_done(descq, cb, 0);
		} else {
			pr_debug("%s, cb 0x%p not done, credit %u < %u.\n",
				descq->conf.name, cb, cr, cb->desc_nr);
			cb->desc_nr -= cr;
			cr = 0;
		}

		if (!cr)
			break;
	}

	descq->credit = cr;
	pr_debug("%s, 0x%p, credit %u.\n",
		descq->conf.name, descq, descq->credit);

	rv = qdma_pidx_update(descq, 1);
	if (unlikely(rv < 0))
		pr_err("%s: Failed to update pidx\n", descq->conf.name);

	return 0;
}

/* ************** public function definitions ******************************* */

void qdma_descq_init(struct qdma_descq *descq, struct xlnx_dma_dev *xdev,
			int idx_hw, int idx_sw)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);

	memset(descq, 0, sizeof(struct qdma_descq));

	spin_lock_init(&descq->lock);
	spin_lock_init(&descq->work_list_lock);
	INIT_LIST_HEAD(&descq->work_list);
	INIT_LIST_HEAD(&descq->pend_list);
	qdma_waitq_init(&descq->pend_list_wq);
	INIT_LIST_HEAD(&descq->intr_list);
	INIT_LIST_HEAD(&descq->legacy_intr_q_list);
	INIT_WORK(&descq->work, intr_work);
	descq->xdev = xdev;
	descq->channel = 0;
	descq->qidx_hw = qdev->qbase + idx_hw;
	descq->conf.qidx = idx_sw;
}

int qdma_q_desc_get(void *q_hndl, const unsigned int desc_cnt,
		    struct qdma_q_desc_list **desc_list)
{
	struct qdma_descq *descq = (struct qdma_descq *)q_hndl;

	if (unlikely(desc_cnt >= descq->conf.rngsz)) {
		pr_err("Number of descriptors required > ring size");
		return -EINVAL; /* not possible to give so many desc */
	}

	/*
	 * it is very unlikely that below condition hits in MM or ST H2C as
	 * one packet is submitted at once. But It is ibserved that HW is not
	 * giving writeback and sometimes its corresponding interrupt. For this
	 * reason a polling logic is introduced as below
	 **/
	if (descq->avail < desc_cnt)
		descq_mm_n_h2c_cmpl_status(descq);

	if (unlikely(descq->avail < desc_cnt)) {
		pr_err("No entries available to read");
		return -EBUSY; /* curently not available */
	}

	*desc_list = descq->desc_list + descq->pidx;
	descq->pidx = incr_pidx(descq->pidx,
				desc_cnt,
				descq->conf.rngsz);
	descq->avail -= desc_cnt;
	return 0;
}

int qdma_q_init_pointers(void *q_hndl)
{
	struct qdma_descq *descq = (struct qdma_descq *)q_hndl;
	int rv;

	if ((descq->conf.st && (descq->conf.q_type == Q_C2H)) ||
			(!descq->conf.st && (descq->conf.q_type == Q_CMPT))) {
		if (descq->conf.init_pidx_dis) {
			/* use qdma_q_init_pointers
			 *for updating the pidx and cidx later
			 */
			descq->cmpt_cidx_info.wrb_cidx = 0;
			rv = queue_cmpt_cidx_update(descq->xdev,
					descq->conf.qidx,
					&descq->cmpt_cidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);
				return -EINVAL;
			}

			if (descq->conf.q_type == Q_CMPT)
				return 0;

			descq->pidx_info.pidx = descq->conf.rngsz - 1;
			rv = queue_pidx_update(descq->xdev, descq->conf.qidx,
					descq->conf.q_type, &descq->pidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update pidx\n",
						descq->conf.name);
				return -EINVAL;
			}
		} else {
			pr_err("pidx disable is active");
			return -EINVAL;
		}
	} else {
		pr_err("Invalid mode");
		return -EINVAL;
	}

	return 0;
}

int qdma_queue_update_pointers(unsigned long dev_hndl, unsigned long qhndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq = qdma_device_get_descq_by_id(xdev, qhndl,
							NULL, 0, 0);
	int ret = 0;

	if (!descq) {
		pr_err("%s descq is null\n", __func__);
		return -EINVAL;
	}

	if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
		lock_descq(descq);
		if (descq->q_state == Q_STATE_ONLINE) {
			ret = queue_cmpt_cidx_update(descq->xdev,
					descq->conf.qidx,
					&descq->cmpt_cidx_info);
			if (ret < 0) {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);

				ret = -EBUSY;
				goto func_exit;
			}
			ret = queue_pidx_update(descq->xdev,
					descq->conf.qidx,
					descq->conf.q_type,
					&descq->pidx_info);
			if (ret < 0) {
				pr_err("%s: Failed to update pidx\n",
						descq->conf.name);
				ret = -EBUSY;
				goto func_exit;
			}
			/*
			 * Memory barrier in update pointers
			 */
			wmb();
		} else {
			pr_debug("Pointer update for offline queue for %s",
					descq->conf.name);
			ret =  -ENODEV;
		}
	} else {
		pr_err("Pointer update for invalid queue for %s",
					descq->conf.name);
		ret =  -EINVAL;
	}

func_exit:
	unlock_descq(descq);
	return ret;

}

int qdma_descq_alloc_resource(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct qdma_queue_conf *qconf = &descq->conf;
	unsigned char is_ul_ext = (qconf->desc_bypass &&
			qconf->fp_bypass_desc_fill) ? 1 : 0;
	int rv;
#ifdef TEST_64B_DESC_BYPASS_FEATURE
	int i = 0;
	u8 *desc_bypass;
	u8 bypass_data[DESC_SZ_64B_BYTES];
#endif
	/* descriptor ring */
	if (descq->conf.q_type != Q_CMPT) {
		descq->desc = desc_ring_alloc(xdev, descq->conf.rngsz,
				get_desc_size(descq),
				get_desc_cmpl_status_size(descq),
				&descq->desc_bus, &descq->desc_cmpl_status);
		if (!descq->desc) {
			pr_err("dev %s, descq %s, sz %u, desc ring OOM.\n",
				xdev->conf.name, descq->conf.name,
				descq->conf.rngsz);
			goto err_out;
		}
	}

	if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
		struct qdma_flq *flq = (struct qdma_flq *)descq->flq;

		flq->desc = (struct qdma_c2h_desc *)descq->desc;
		flq->size = descq->conf.rngsz;
		flq->buf_pg_shift = fls(descq->conf.c2h_bufsz) - 1;
		flq->buf_pg_mask = (1 << flq->buf_pg_shift) - 1;

		flq->desc_buf_size = get_next_powof2(descq->conf.c2h_bufsz);
		flq->desc_pg_shift = fls(flq->desc_buf_size) - 1;

		/* These code changes are to accomodate buf_sz
		 *  of less than 4096
		 */

		if (flq->desc_pg_shift < PAGE_SHIFT)
			flq->desc_pg_order = 0;
		else
			flq->desc_pg_order = flq->desc_pg_shift - PAGE_SHIFT;

		flq->max_pg_offset = (PAGE_SIZE << flq->desc_pg_order);

		pr_debug("%s: buf_pg_shift = %d, buf_pg_mask = %d, desc_buf_size = %d, desc_pg_shift = %d, flq->desc_pg_order = %d, flq->max_pg_offset = %d",
				descq->conf.name,
				flq->buf_pg_shift,
				flq->buf_pg_mask,
				flq->desc_buf_size,
				flq->desc_pg_shift,
				flq->desc_pg_order,
				flq->max_pg_offset);


		/* freelist / rx buffers */
		rv = descq_flq_alloc_resource(descq);
		if (rv < 0) {
			pr_err("%s: resource allocation failed",
					__func__);
			goto err_out;
		}
	} else if (is_ul_ext) {
		int i;
		unsigned int desc_sz = get_desc_size(descq);

		descq->desc_list = kcalloc(descq->conf.rngsz,
					   sizeof(struct qdma_q_desc_list),
					   GFP_KERNEL);
		if (!descq->desc_list) {
			pr_err("desc_list allocation failed.OOM");
			goto err_out;
		}
		for (i = 0; i < descq->conf.rngsz; i++) {
			int next = (i == (descq->conf.rngsz - 1)) ? 0 : (i + 1);

			descq->desc_list[i].desc = descq->desc + (i * desc_sz);
			descq->desc_list[i].next = &descq->desc_list[next];
		}
	}

	if ((descq->conf.st && (descq->conf.q_type == Q_C2H)) ||
		    (!descq->conf.st && (descq->conf.q_type == Q_CMPT))) {

		descq->color = 1;

		/* writeback ring */
		descq->desc_cmpt = desc_ring_alloc(xdev,
					descq->conf.rngsz_cmpt,
					descq->cmpt_entry_len,
					sizeof(struct
					       qdma_c2h_cmpt_cmpl_status),
					&descq->desc_cmpt_bus,
					&descq->desc_cmpt_cmpl_status);
		if (!descq->desc_cmpt) {
			pr_warn("dev %s, descq %s, sz %u, cmpt ring OOM.\n",
				xdev->conf.name, descq->conf.name,
				descq->conf.rngsz_cmpt);
			goto err_out;
		}
		descq->desc_cmpt_cur = descq->desc_cmpt;

	}

	pr_debug("%s: %u/%u, rng %u,%u, desc 0x%p, cmpl status 0x%p.\n",
		descq->conf.name, descq->conf.qidx, descq->qidx_hw,
		descq->conf.rngsz, descq->conf.rngsz_cmpt, descq->desc,
		descq->desc_cmpt);

	/* interrupt vectors */
	desc_alloc_irq(descq);

	/* Fill in the descriptors with some hard coded value for testing */
#ifdef TEST_64B_DESC_BYPASS_FEATURE
	desc_bypass = descq->desc;
	if (descq->conf.st && (descq->conf.q_type == Q_H2C)) {
		if (descq->conf.desc_bypass &&
				(descq->conf.sw_desc_sz == DESC_SZ_64B)) {
			for (i = 0; i < descq->conf.rngsz; i++) {
				memset(bypass_data, i+1, DESC_SZ_64B_BYTES);
				memcpy(&desc_bypass[i*DESC_SZ_64B_BYTES],
					bypass_data, DESC_SZ_64B_BYTES);
			}
		}
	}
#endif
	return 0;

err_out:
	qdma_descq_free_resource(descq);
	pr_err("Out of Memory");
	return -ENOMEM;
}

void qdma_descq_free_resource(struct qdma_descq *descq)
{
	if (!descq)
		return;

	if (descq->desc) {

		int desc_sz = get_desc_size(descq);
		int cs_sz = get_desc_cmpl_status_size(descq);

		pr_debug("%s: desc 0x%p, cmpt 0x%p.\n",
			descq->conf.name, descq->desc, descq->desc_cmpt);

		if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
			descq_flq_free_resource(descq);
			descq_flq_free_page_resource(descq);
		} else
			kfree(descq->desc_list);

		desc_ring_free(descq->xdev, descq->conf.rngsz, desc_sz, cs_sz,
				descq->desc, descq->desc_bus);
		descq->desc_cmpl_status = NULL;
		descq->desc = NULL;
		descq->desc_bus = 0UL;
	}

	if (descq->desc_cmpt) {
		desc_ring_free(descq->xdev, descq->conf.rngsz_cmpt,
			descq->cmpt_entry_len,
			sizeof(struct qdma_c2h_cmpt_cmpl_status),
			descq->desc_cmpt, descq->desc_cmpt_bus);

		descq->desc_cmpt_cmpl_status = NULL;
		descq->desc_cmpt = NULL;
		descq->desc_cmpt_bus = 0UL;
	}
}

void qdma_descq_config(struct qdma_descq *descq, struct qdma_queue_conf *qconf,
		 int reconfig)
{
	if (!reconfig) {
		int len;

		memcpy(&descq->conf, qconf, sizeof(struct qdma_queue_conf));
		/* descq->conf.st = qconf->st;
		 * descq->conf.c2h = qconf->c2h;
		 */

		/* qdma[vf]<255>-MM/ST-H2C/C2H-Q[2048] */
#ifdef __QDMA_VF__
		len = snprintf(descq->conf.name, QDMA_QUEUE_NAME_MAXLEN,
				"qdmavf");
#else
		len = snprintf(descq->conf.name, QDMA_QUEUE_NAME_MAXLEN,
				"qdma");
#endif
		len += snprintf(descq->conf.name + len,
			QDMA_QUEUE_NAME_MAXLEN - len,
			"%05x-%s-%u",
			descq->xdev->conf.bdf, descq->conf.st ? "ST" : "MM",
			descq->conf.qidx);
		descq->conf.name[len] = '\0';

		descq->conf.st = qconf->st;
		descq->conf.q_type = qconf->q_type;

	} else {
		descq->conf.desc_rng_sz_idx = qconf->desc_rng_sz_idx;
		descq->conf.cmpl_rng_sz_idx = qconf->cmpl_rng_sz_idx;
		descq->conf.c2h_buf_sz_idx = qconf->c2h_buf_sz_idx;

		descq->conf.irq_en = (descq->xdev->conf.qdma_drv_mode !=
				POLL_MODE) ? 1 : 0;
		descq->conf.cmpl_en_intr = descq->conf.irq_en;
		descq->conf.wb_status_en = qconf->wb_status_en;
		descq->conf.cmpl_status_acc_en = qconf->cmpl_status_acc_en;
		descq->conf.cmpl_status_pend_chk = qconf->cmpl_status_pend_chk;
		descq->conf.cmpl_stat_en = qconf->cmpl_stat_en;
		descq->conf.cmpl_trig_mode = qconf->cmpl_trig_mode;
		descq->conf.cmpl_timer_idx = qconf->cmpl_timer_idx;
		descq->conf.fetch_credit = qconf->fetch_credit;
		descq->conf.cmpl_cnt_th_idx = qconf->cmpl_cnt_th_idx;
		if (descq->xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
			descq->channel = qconf->mm_channel;

		descq->conf.desc_bypass = qconf->desc_bypass;
		descq->conf.pfetch_bypass = qconf->pfetch_bypass;
		descq->conf.pfetch_en = qconf->pfetch_en;
		descq->conf.cmpl_udd_en = qconf->cmpl_udd_en;
		descq->conf.cmpl_desc_sz = qconf->cmpl_desc_sz;
		descq->conf.sw_desc_sz = qconf->sw_desc_sz;
		descq->conf.cmpl_ovf_chk_dis = qconf->cmpl_ovf_chk_dis;
		descq->conf.adaptive_rx = qconf->adaptive_rx;
		descq->conf.ping_pong_en = qconf->ping_pong_en;
		descq->conf.aperture_size = qconf->aperture_size;
		descq->conf.pidx_acc = qconf->pidx_acc;
	}
}

int qdma_descq_config_complete(struct qdma_descq *descq)
{
	struct global_csr_conf *csr_info = &descq->xdev->csr_info;
	struct qdma_queue_conf *qconf = &descq->conf;
	struct xlnx_dma_dev *xdev = descq->xdev;

	qconf->rngsz = csr_info->ring_sz[qconf->desc_rng_sz_idx] - 1;

	/* <= 2018.2 IP
	 * make the cmpl ring size bigger if possible to avoid run out of
	 * cmpl entry while desc. ring still have free entries
	 */
	if ((qconf->st && (qconf->q_type == Q_C2H)) ||
			(!qconf->st && (qconf->q_type == Q_CMPT))) {
		int i;
		unsigned int v = csr_info->ring_sz[qconf->cmpl_rng_sz_idx];
		int best_fit_idx = -1;

		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
			if (csr_info->ring_sz[i] > v) {
				if (best_fit_idx < 0)
					best_fit_idx = i;
				else if ((best_fit_idx >= 0) &&
						(csr_info->ring_sz[i] <
					csr_info->ring_sz[best_fit_idx]))
					best_fit_idx = i;
			}
		}

		if (best_fit_idx >= 0)
			qconf->cmpl_rng_sz_idx = best_fit_idx;

		qconf->rngsz_cmpt = csr_info->ring_sz[qconf->cmpl_rng_sz_idx] -
				1;
		qconf->c2h_bufsz = csr_info->c2h_buf_sz[qconf->c2h_buf_sz_idx];
		descq->cmpt_cidx_info.irq_en = qconf->cmpl_en_intr;
		descq->cmpt_cidx_info.trig_mode = qconf->cmpl_trig_mode;
		descq->cmpt_cidx_info.timer_idx = qconf->cmpl_timer_idx;
		descq->cmpt_cidx_info.counter_idx = qconf->cmpl_cnt_th_idx;
		descq->cmpt_cidx_info.wrb_en = qconf->cmpl_stat_en;
	}
	if (qconf->st && (qconf->q_type == Q_C2H))
		descq->pidx_info.irq_en = 0;
	else
		descq->pidx_info.irq_en = descq->conf.irq_en;

	/* we can never use the full ring because then cidx would equal pidx
	 * and thus the ring would be interpreted as empty. Thus max number of
	 * usable entries is ring_size - 1
	 */
	descq->avail = descq->conf.rngsz - 1;
	descq->pend_list_empty = 1;

	descq->pidx = 0;
	descq->cidx = 0;
	descq->cidx_cmpt = 0;
	descq->pidx_cmpt = 0;
	descq->credit = 0;
	descq->work_req_pend = 0;

	/* ST C2H only */
	if ((qconf->st && (qconf->q_type == Q_C2H)) ||
			(!qconf->st && (qconf->q_type == Q_CMPT))) {
		int i, hi_idx, low_idx;

		descq->cmpt_entry_len = 8 << qconf->cmpl_desc_sz;

		pr_debug("%s: cmpl sz %u(%d), udd_en %d.\n",
			descq->conf.name, descq->cmpt_entry_len,
			descq->conf.cmpl_desc_sz, descq->conf.cmpl_udd_en);
		descq->c2h_pend_pkt_moving_avg =
			csr_info->c2h_cnt_th[descq->conf.cmpl_cnt_th_idx];
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
			if (descq->conf.cmpl_cnt_th_idx ==
					descq->xdev->sorted_c2h_cntr_idx[i]) {
				descq->sorted_c2h_cntr_idx = i;
				break;
			}
		}
		hi_idx = descq->sorted_c2h_cntr_idx + 1;
		low_idx = descq->sorted_c2h_cntr_idx - 1;

		if (descq->sorted_c2h_cntr_idx ==
				(QDMA_GLOBAL_CSR_ARRAY_SZ - 1))
			hi_idx = descq->sorted_c2h_cntr_idx;

		i = xdev->sorted_c2h_cntr_idx[hi_idx];

		descq->c2h_pend_pkt_avg_thr_hi =
				(descq->c2h_pend_pkt_moving_avg +
				csr_info->c2h_cnt_th[i]);
		if (descq->sorted_c2h_cntr_idx == 0)
			low_idx = 0;

		i = xdev->sorted_c2h_cntr_idx[low_idx];

		descq->c2h_pend_pkt_avg_thr_lo =
				(descq->c2h_pend_pkt_moving_avg +
				csr_info->c2h_cnt_th[i]);
		descq->c2h_pend_pkt_avg_thr_hi >>= 1;
		descq->c2h_pend_pkt_avg_thr_lo >>= 1;
		pr_debug("q%u: sorted idx =  %u %u %u", descq->conf.qidx,
			descq->sorted_c2h_cntr_idx,
			descq->c2h_pend_pkt_avg_thr_lo,
			descq->c2h_pend_pkt_avg_thr_hi);
	}

	return 0;
}

int qdma_descq_prog_hw(struct qdma_descq *descq)
{
	int rv = qdma_descq_context_setup(descq);

	if (rv < 0) {
		pr_warn("%s failed to program contexts", descq->conf.name);
		return rv;
	}

	/* update pidx/cidx */
	if ((descq->conf.st && (descq->conf.q_type == Q_C2H)) ||
		(!descq->conf.st && (descq->conf.q_type == Q_CMPT))) {
		if (!descq->conf.init_pidx_dis) {
			/* use qdma_q_init_pointers
			 * for updating the pidx and cidx later
			 */
			descq->cmpt_cidx_info.wrb_cidx = 0;
			rv = queue_cmpt_cidx_update(descq->xdev,
					descq->conf.qidx,
					&descq->cmpt_cidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);
				return -EINVAL;
			}

			if (descq->conf.q_type == Q_CMPT)
				return rv;

			descq->pidx_info.pidx = descq->conf.rngsz - 1;
			rv = queue_pidx_update(descq->xdev, descq->conf.qidx,
					descq->conf.q_type, &descq->pidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update pidx\n",
						descq->conf.name);
				return -EINVAL;
			}
		}
	}

	return rv;
}

int qdma_descq_service_cmpl_update(struct qdma_descq *descq, int budget,
				bool c2h_upd_cmpl)
{
	int rv = 0;

	if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
		lock_descq(descq);
		if (descq->q_state == Q_STATE_ONLINE) {
			rv = descq_process_completion_st_c2h(descq, budget,
						c2h_upd_cmpl);
			if (rv && (rv != -ENODATA))
				pr_err("Error detected in %s",
				       descq->conf.name);
		} else {
			pr_debug("Invalid q state of %s ", descq->conf.name);
			rv = -EINVAL;
		}
		unlock_descq(descq);
	} else if ((descq->xdev->conf.qdma_drv_mode == POLL_MODE) ||
			(descq->xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		lock_descq(descq);
		if (!descq->proc_req_running) {
			unlock_descq(descq);
			qdma_descq_proc_sgt_request(descq);
		} else {
			pr_err("Processing previous request\n");
			unlock_descq(descq);
		}
	} else {
		lock_descq(descq);
		descq_mm_n_h2c_cmpl_status(descq);
		if (qdma_work_queue_len(descq) || descq->desc_pend) {
			unlock_descq(descq);
			rv = qdma_descq_proc_sgt_request(descq);
			return rv;
		}
		unlock_descq(descq);
	}

	return rv;
}

ssize_t qdma_descq_proc_sgt_request(struct qdma_descq *descq)
{
	if (!descq->conf.st) /* MM H2C/C2H */
		return descq_mm_proc_request(descq);
	else if (descq->conf.st && (descq->conf.q_type == Q_H2C)) {/* ST H2C */
		if (descq->conf.fp_bypass_desc_fill &&
			descq->conf.desc_bypass &&
			descq->xdev->conf.qdma_drv_mode == DIRECT_INTR_MODE)
			return descq_proc_st_h2c_request_qep(descq);
		return descq_proc_st_h2c_request(descq);
	} else	/* ST C2H - should not happen - handled separately */
		return -EINVAL;
}

void incr_cmpl_desc_cnt(struct qdma_descq *descq, unsigned int cnt)
{
	descq->total_cmpl_descs += cnt;
	switch ((descq->conf.st << 1) | descq->conf.q_type) {
	case 0:
		descq->xdev->total_mm_h2c_pkts += cnt;
		break;
	case 1:
		descq->xdev->total_mm_c2h_pkts += cnt;
		break;
	case 2:
		descq->xdev->total_st_h2c_pkts += cnt;
		break;
	case 3:
		descq->xdev->total_st_c2h_pkts += cnt;
		break;
	default:
		break;
	}
}

void qdma_sgt_req_done(struct qdma_descq *descq, struct qdma_sgt_req_cb *cb,
			int error)
{
	struct qdma_request *req = (struct qdma_request *)cb;

	if (unlikely(error))
		pr_err("req 0x%p, cb 0x%p, fp_done 0x%p done, err %d.\n",
			req, cb, req->fp_done, error);

	list_del(&cb->list);
	if (cb->unmap_needed) {
		sgl_unmap(descq->xdev->conf.pdev, req->sgl, req->sgcnt,
			(descq->conf.q_type == Q_C2H) ?
			DMA_FROM_DEVICE : DMA_TO_DEVICE);
		cb->unmap_needed = 0;
	}
	if (req->fp_done) {
		if ((cb->offset != req->count) &&
				!(descq->conf.st &&
				(descq->conf.q_type == Q_C2H))) {
			pr_info("req 0x%p not completed %u != %u.\n",
				req, cb->offset, req->count);
			error = -EINVAL;
		}
		cb->status = error;
		cb->done = 1;
		req->fp_done(req, cb->offset, error);
	} else {
		pr_debug("req 0x%p, cb 0x%p, wake up.\n", req, cb);
		cb->status = error;
		cb->done = 1;
		qdma_waitq_wakeup(&cb->wq);
	}

	if (!descq->conf.fp_descq_c2h_packet) {
		if (descq->conf.st && (descq->conf.q_type == Q_C2H))
			descq->pend_list_empty = (descq->avail == 0);
		else
			descq->pend_list_empty = (descq->avail ==
					(descq->conf.rngsz - 1));

		if (descq->q_stop_wait && descq->pend_list_empty)
			qdma_waitq_wakeup(&descq->pend_list_wq);
	} else
		descq->pend_list_empty = 1;
}

int qdma_descq_dump_desc(struct qdma_descq *descq, int start,
			int end, char *buf, int buflen)
{
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	int desc_sz = get_desc_size(descq);
	u8 *p = descq->desc + start * desc_sz;
	struct qdma_sw_sg *fl = (descq->conf.st &&
				(descq->conf.q_type == Q_C2H)) ?
				flq->sdesc + start : NULL;
	int i = start;
	int len = strlen(buf);

	if (!descq->desc)
		return 0;

	for (; i < end && i < descq->conf.rngsz; i++, p += desc_sz) {
		len += snprintf(buf + len, buflen - len,
					   "%d: 0x%p ", i, p);
		hex_dump_to_buffer(p, desc_sz,
				  (desc_sz < DESC_SZ_16B_BYTES) ? 16 : 32,
				  4, buf + len, buflen - len, 0);
		len = strlen(buf);
		if (desc_sz > DESC_SZ_32B_BYTES) {
			len += snprintf(buf + len, buflen - len, " ");
			hex_dump_to_buffer(p + DESC_SZ_32B_BYTES, desc_sz,
					32, 4, buf + len, buflen - len, 0);
			len = strlen(buf);
		}
		if (fl) {
			len += snprintf(buf + len, buflen - len,
				" fl pg 0x%p, 0x%llx.\n",
				fl->pg, fl->dma_addr);
			fl++;
		} else
			buf[len++] = '\n';
	}

	p = descq->desc_cmpl_status;

	dma_rmb();

	len += snprintf(buf + len, buflen - len, "CMPL STATUS: 0x%p ", p);
	hex_dump_to_buffer(p, get_desc_cmpl_status_size(descq), 16, 4,
			buf + len, buflen - len, 0);
	len = strlen(buf);
	buf[len++] = '\n';

	if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
		p = page_address(fl->pg);
		len += snprintf(buf + len, buflen - len, "data 0: 0x%p ", p);
		hex_dump_to_buffer(p, descq->cmpt_entry_len,
			(descq->cmpt_entry_len < DESC_SZ_16B_BYTES) ? 16 : 32,
			4, buf + len,
			buflen - len, 0);
		len = strlen(buf);
		if (descq->cmpt_entry_len > DESC_SZ_32B_BYTES) {
			len += snprintf(buf + len, buflen - len, " ");
			hex_dump_to_buffer(p + DESC_SZ_32B_BYTES,
					descq->cmpt_entry_len, 32, 4,
					buf + len, buflen - len, 0);
			len = strlen(buf);
		}
		buf[len++] = '\n';
	}

	return len;
}

int qdma_descq_dump_cmpt(struct qdma_descq *descq, int start,
			int end, char *buf, int buflen)
{
	uint8_t *cmpt = descq->desc_cmpt;
	u8 *p;
	int i = start;
	int len = strlen(buf);
	int stride = descq->cmpt_entry_len;

	if (!descq->desc_cmpt)
		return 0;

	for (cmpt += (start * stride);
			i < end && i < descq->conf.rngsz_cmpt; i++,
			cmpt += stride) {
		len += snprintf(buf + len, buflen - len,
						"%d: 0x%p ", i, cmpt);
		hex_dump_to_buffer(cmpt, descq->cmpt_entry_len,
				32, 4, buf + len, buflen - len, 0);
		len = strlen(buf);
		if (descq->cmpt_entry_len > DESC_SZ_32B_BYTES) {
			len += snprintf(buf + len, buflen - len,
							" ");
			hex_dump_to_buffer(cmpt + DESC_SZ_32B_BYTES,
					   descq->cmpt_entry_len,
					   32, 4, buf + len, buflen - len, 0);
			len = strlen(buf);
		}
		buf[len++] = '\n';
	}

	len += snprintf(buf + len, buflen - len,
			"CMPL STATUS: 0x%p ",
			descq->desc_cmpt_cmpl_status);

	p = descq->desc_cmpt_cmpl_status;
	dma_rmb();
	hex_dump_to_buffer(p, sizeof(struct qdma_c2h_cmpt_cmpl_status),
			16, 4, buf + len, buflen - len, 0);
	len = strlen(buf);
	buf[len++] = '\n';

	return len;
}

int qdma_descq_dump_state(struct qdma_descq *descq, char *buf, int buflen)
{
	char *cur = buf;
	char *const end = buf + buflen;

	if (!buf || !buflen) {
		pr_warn("incorrect arguments buf=%p buflen=%d", buf, buflen);
		return 0;
	}

	if (descq->conf.q_type != Q_CMPT)
		cur += snprintf(cur, end - cur, "%s %s ",
				descq->conf.name,
				(descq->conf.q_type == Q_C2H) ? "C2H" : "H2C");
	else
		cur += snprintf(cur, end - cur, "%s %s ",
				descq->conf.name, "CMPT");
	if (cur >= end)
		goto handle_truncation;

	if (descq->err)
		cur += snprintf(cur, end - cur, "ERR\n");
	else
		cur += snprintf(cur, end - cur, "%s\n",
					q_state_list[descq->q_state].name);

	if (cur >= end)
		goto handle_truncation;

	return cur - buf;

handle_truncation:
	*buf = '\0';
	return cur - buf;
}

int qdma_descq_dump(struct qdma_descq *descq, char *buf, int buflen, int detail)
{
	char *cur = buf;
	char *const end = buf + buflen;

	if (!buf || !buflen) {
		pr_info("%s:%s 0x%x/0x%x, desc sz %u/%u, pidx %u, cidx %u\n",
			descq->conf.name, descq->err ? "ERR" : "",
			descq->conf.qidx, descq->qidx_hw, descq->conf.rngsz,
			descq->avail, descq->pidx, descq->cidx);
		return 0;
	}

	cur += qdma_descq_dump_state(descq, cur, end - cur);
	if (cur >= end)
		goto handle_truncation;

	if (descq->q_state == Q_STATE_DISABLED)
		return cur - buf;

	cur += snprintf(cur, end - cur,
		"\thw_ID %u, thp %s, desc 0x%p/0x%llx, %u\n",
		descq->qidx_hw,
		descq->cmplthp ? descq->cmplthp->name : "?",
		descq->desc, descq->desc_bus, descq->conf.rngsz);
	if (cur >= end)
		goto handle_truncation;

	if (descq->conf.st && (descq->conf.q_type == Q_C2H)) {
		cur += snprintf(cur, end - cur,
			"\tcmpt desc 0x%p/0x%llx, %u\n",
			descq->desc_cmpt, descq->desc_cmpt_bus,
			descq->conf.rngsz_cmpt);
		if (cur >= end)
			goto handle_truncation;
	}

	if (!detail)
		return cur - buf;

	if (descq->desc_cmpl_status) {
		u8 *cs = descq->desc_cmpl_status;

		cur += snprintf(cur, end - cur, "\n\tcmpl status: 0x%p, ", cs);
		if (cur >= end)
			goto handle_truncation;

		dma_rmb();
#if KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE
		cur += hex_dump_to_buffer(cs,
					  sizeof(struct qdma_desc_cmpl_status),
					  16, 4, cur, end - cur, 0);
#else
		hex_dump_to_buffer(cs, sizeof(struct qdma_desc_cmpl_status),
					  16, 4, cur, end - cur, 0);
		cur += strlen(cur);
#endif
		if (cur >= end)
			goto handle_truncation;

		cur += snprintf(cur, end - cur, "\n");
		if (cur >= end)
			goto handle_truncation;
	}
	if (descq->desc_cmpt_cmpl_status) {
		u8 *cs = descq->desc_cmpt_cmpl_status;

		cur += snprintf(cur, end - cur, "\tCMPT CMPL STATUS: 0x%p, ",
				cs);
		if (cur >= end)
			goto handle_truncation;

		dma_rmb();
#if KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE
		cur += hex_dump_to_buffer(cs,
				sizeof(struct qdma_c2h_cmpt_cmpl_status),
				16, 4, cur, end - cur, 0);
#else
		hex_dump_to_buffer(cs, sizeof(struct qdma_c2h_cmpt_cmpl_status),
					  16, 4, cur, end - cur, 0);
		cur += strlen(cur);
#endif
		if (cur >= end)
			goto handle_truncation;

		cur += snprintf(cur, end - cur, "\n");
		if (cur >= end)
			goto handle_truncation;
	}

	return cur - buf;

handle_truncation:
	*buf = '\0';
	return cur - buf;
}

int qdma_queue_avail_desc(unsigned long dev_hndl, unsigned long id)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, NULL, 0, 1);
	int avail;

	if (!descq) {
		pr_err("Invalid qid: %ld", id);
		return -EINVAL;
	}

	lock_descq(descq);
	avail = descq->avail;
	unlock_descq(descq);

	return avail;
}

#ifdef ERR_DEBUG
int qdma_queue_set_err_induction(unsigned long dev_hndl, unsigned long id,
			u32 err, char *buf, int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, buf, buflen, 1);
	unsigned int err_en = (err >> 31);
	unsigned int err_no = (err & 0x7FFFFFFF);

	if (!descq) {
		pr_err("Invalid qid: %ld", id);
		return -EINVAL;
	}

	if (err_no < sb_mi_h2c0_dat) {
		descq->induce_err &= ~(1 << err_no);
		descq->induce_err |= (err_en << err_no);
	} else {
		descq->ecc_err &= ~(1 << (err_no - sb_mi_h2c0_dat));
		descq->ecc_err |= (err_en << (err_no - sb_mi_h2c0_dat));
	}
	pr_info("Errs enabled = [QDMA]: 0x%08x 0x%08x\n [ECC]: 0x%08x 0x%08x",
		(u32)(descq->induce_err >> 32),
		(u32)(descq->induce_err & 0xFFFFFFFF),
		(u32)(descq->ecc_err >> 32),
		(u32)(descq->ecc_err & 0xFFFFFFFF));

	return 0;
}
#endif

int qdma_queue_packet_write(unsigned long dev_hndl, unsigned long id,
				struct qdma_request *req)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq;
	struct qdma_sgt_req_cb *cb;
	int rv;

	/** make sure that the dev_hndl passed is Valid */
	if (unlikely(!xdev)) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (unlikely(xdev_check_hndl(__func__,
				xdev->conf.pdev, dev_hndl) < 0)) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	if (unlikely(!req)) {
		pr_err("req is NULL");
		return -EINVAL;
	}

	cb = qdma_req_cb_get(req);

	descq = qdma_device_get_descq_by_id(xdev, id, NULL, 0, 0);
	if (unlikely(!descq)) {
		pr_err("Invalid qid: %ld", id);
		return -EINVAL;
	}

	if (unlikely(!descq->conf.st || (descq->conf.q_type == Q_C2H))) {
		pr_err("%s: st %d, type %d.\n",
			descq->conf.name, descq->conf.st, descq->conf.q_type);
		return -EINVAL;
	}

	qdma_work_queue_add(descq, cb);

	if (!req->dma_mapped) {
		rv = sgl_map(descq->xdev->conf.pdev, req->sgl, req->sgcnt,
				DMA_TO_DEVICE);
		if (rv < 0) {
			pr_err("%s map sgl %u failed, %u.\n",
				descq->conf.name, req->sgcnt, req->count);
			goto unmap_sgl;
		}
		cb->unmap_needed = 1;
	}

	if (!req->check_qstate_disabled) {
		lock_descq(descq);
		if (descq->q_state != Q_STATE_ONLINE) {
			unlock_descq(descq);
			pr_err("%s descq %s NOT online.\n",
				descq->xdev->conf.name, descq->conf.name);
			rv = -EINVAL;
			goto unmap_sgl;
		}
		unlock_descq(descq);
	}

	qdma_descq_proc_sgt_request(descq);


	pr_debug("%s: cb 0x%p submitted for bytes %u.\n",
					descq->conf.name, cb, req->count);

	return req->count;

unmap_sgl:
	if (cb->unmap_needed)
		sgl_unmap(descq->xdev->conf.pdev, req->sgl, req->sgcnt,
			DMA_TO_DEVICE);
	return rv;
}

int qdma_descq_get_cmpt_udd(unsigned long dev_hndl, unsigned long id,
		char *buf, int buflen)
{
	uint8_t *cmpt;
	uint8_t i = 0;
	int len = 0;
	int print_len = 0;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq = NULL;
	struct qdma_c2h_cmpt_cmpl_status *cs;

	if (!buf || !buflen || !xdev) {
		pr_err("Invalid bud:%p, buflen:%d", buf, buflen);
		return -EINVAL;
	}


	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 1);

	if (!descq) {
		pr_err("Invalid qid: %ld", id);
		return -EINVAL;
	}

	cs = (struct qdma_c2h_cmpt_cmpl_status *)
						descq->desc_cmpt_cmpl_status;

	cmpt = descq->desc_cmpt + ((cs->pidx - 1) * descq->cmpt_entry_len);

	/*
	 * Ignoring the first 4 bits of the completion entry as they represent
	 * the error and color bits.
	 * TODO: May need to change the masking logic and move that in thegtest,
	 * as error and color bit positions may change in the future releases.
	 */
	for (i = 0; i < descq->cmpt_entry_len; i++) {
		if (buf && buflen) {
			if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP) {
				if (i <= 1)
					continue;
				else if (i == 2)
					print_len = snprintf(buf + len, buflen,
							"%02x",
							(0xF0 & cmpt[i]));
				else
					print_len = snprintf(buf + len, buflen,
							"%02x",
							cmpt[i]);
			} else {
				if (i == 0)
					print_len = snprintf(buf + len, buflen,
							"%02x",
							(cmpt[i] & 0xF0));
				else
					print_len = snprintf(buf + len, buflen,
							"%02x",
							cmpt[i]);
			}

		}
		buflen -= print_len;
		len += print_len;
	}
	buf[len] = '\0';

	return 0;
}

static int qdma_descq_read_cmpt(struct qdma_descq *descq,
				u32 num_entries, u8 *cmpt_data)
{
	uint8_t *cmpt;
	int i = 0;
	int stride = descq->cmpt_entry_len;
	u8 *temp = cmpt_data;

	if (!descq->desc_cmpt)
		return 0;

	lock_descq(descq);
	cmpt = descq->desc_cmpt + (descq->cidx_cmpt * stride);
	for (i = 0; i < num_entries;
			i++, cmpt += stride, cmpt_data += stride) {
		memcpy(cmpt_data, cmpt, stride);
	}
	*(temp) = (*(temp) & 0xF0);
	unlock_descq(descq);

	return i;
}

int qdma_descq_read_cmpt_data(unsigned long dev_hndl, unsigned long id,
				u32 *num_entries,  u8 **cmpt_entries,
				char *buf, int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, buf, buflen, 1);
	struct qdma_c2h_cmpt_cmpl_status *cs = NULL;
	unsigned int pidx_cmpt = 0;
	unsigned int cidx_cmpt = 0;
	int  rv = 0;
	u32 pend = 0;

	if (!descq) {
		pr_err("Invalid qid, qid = %ld", id);
		return -EINVAL;
	}

	if (!buf || !buflen) {
		pr_err("Invalid buf:%p, buflen:%d", buf, buflen);
		return -EINVAL;
	}

	if (descq->conf.st) {
		pr_err("Not supported for ST mode");
		rv = -EINVAL;
		return rv;
	}

	lock_descq(descq);
	cs = (struct qdma_c2h_cmpt_cmpl_status *)
				descq->desc_cmpt_cmpl_status;
	cidx_cmpt = descq->cidx_cmpt;
	pidx_cmpt = cs->pidx;
	unlock_descq(descq);
	pend = ring_idx_delta(pidx_cmpt,
			      cidx_cmpt,
				descq->conf.rngsz_cmpt);
	pr_debug("ringsz %d, pend = %d, cs->pidx = %d cs->cidx = %d, descq->cidx_cmpt = %d, descq->pidx_cmpt = %d",
			descq->conf.rngsz_cmpt, pend, cs->pidx, cs->cidx,
				    cidx_cmpt,
				    pidx_cmpt);
	*num_entries = min(pend, descq->conf.rngsz_cmpt);

	if (!pend) {
		int l = 0;

		pr_debug("No pending cmpt entries");
		/* SW work around where next interrupt could be missed when
		 * there are no entries as of now
		 */
		if (descq->xdev->conf.qdma_drv_mode != POLL_MODE) {
			lock_descq(descq);
			descq->cmpt_cidx_info.wrb_cidx = cidx_cmpt;
			unlock_descq(descq);
			rv = queue_cmpt_cidx_update(descq->xdev,
					descq->conf.qidx,
					&descq->cmpt_cidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);
				return -EINVAL;
			}

		}

		l = strlen(buf);

		l += snprintf(buf + l, buflen,
			"%s no pending entries in cmpt ring\n",
			descq->conf.name);
		buf[l] = '\0';
		*num_entries = 0;
		return 0;
	}

	if (*num_entries > 0) {
		/** Note: cmpt_entries will be freed by caller in nl.c
		 *  do not free it here
		 */
		*cmpt_entries = kzalloc((*num_entries * descq->cmpt_entry_len),
								GFP_KERNEL);
		if (!*cmpt_entries) {
			int l = snprintf(buf, buflen, "OOM for cmpt_entries\n");

			buf[l] = '\0';
			*num_entries = 0;
			return 0;
		}

		qdma_descq_read_cmpt(descq, *num_entries, *cmpt_entries);
		lock_descq(descq);
		descq->cidx_cmpt += *num_entries;
		descq->cmpt_cidx_info.wrb_cidx = descq->cidx_cmpt;
		unlock_descq(descq);
		rv = queue_cmpt_cidx_update(descq->xdev,
				descq->conf.qidx,
				&descq->cmpt_cidx_info);
		if (unlikely(rv < 0)) {
			pr_err("%s: Failed to update cmpt cidx\n",
				  descq->conf.name);
			return -EINVAL;
		}
	}

	return rv;
}
