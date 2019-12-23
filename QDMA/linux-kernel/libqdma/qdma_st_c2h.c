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

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_descq.h"

#include <linux/kernel.h>
#include <linux/delay.h>

#include "qdma_device.h"
#include "qdma_intr.h"
#include "qdma_regs.h"
#include "qdma_thread.h"
#include "qdma_context.h"
#include "thread.h"
#include "qdma_compat.h"
#include "qdma_st_c2h.h"
#include "qdma_access.h"
#include "qdma_ul_ext.h"
#include "version.h"

/*
 * ST C2H descq (i.e., freelist) RX buffers
 */

static inline void flq_unmap_one(struct qdma_sw_sg *sdesc,
				struct qdma_c2h_desc *desc, struct device *dev,
				unsigned char pg_order)
{
	if (sdesc->dma_addr) {
		desc->dst_addr = 0UL;
		dma_unmap_page(dev, sdesc->dma_addr, PAGE_SIZE << pg_order,
				DMA_FROM_DEVICE);
		sdesc->dma_addr = 0UL;
	}
}

static inline void flq_free_one(struct qdma_sw_sg *sdesc,
				struct qdma_c2h_desc *desc, struct device *dev,
				unsigned char pg_order)
{
	if (sdesc && sdesc->pg) {
		flq_unmap_one(sdesc, desc, dev, pg_order);
		__free_pages(sdesc->pg, pg_order);
		sdesc->pg = NULL;
	}
}

static inline int flq_fill_one(struct qdma_sw_sg *sdesc,
				struct qdma_c2h_desc *desc, struct device *dev,
				int node, unsigned int buf_sz,
				unsigned char pg_order, gfp_t gfp)
{
	struct page *pg;
	dma_addr_t mapping;

	pg = alloc_pages_node(node, __GFP_COMP | gfp, pg_order);
	if (unlikely(!pg)) {
		pr_info("failed to allocate the pages, order %d.\n", pg_order);
		return -ENOMEM;
	}

	mapping = dma_map_page(dev, pg, 0, PAGE_SIZE << pg_order,
				PCI_DMA_FROMDEVICE);
	if (unlikely(dma_mapping_error(dev, mapping))) {
		dev_err(dev, "page 0x%p mapping error 0x%llx.\n",
			pg, (unsigned long long)mapping);
		__free_pages(pg, pg_order);
		return -EINVAL;
	}

	sdesc->pg = pg;
	sdesc->dma_addr = mapping;
	sdesc->len = buf_sz << pg_order;
	sdesc->offset = 0;

	desc->dst_addr = sdesc->dma_addr;
	return 0;
}

void descq_flq_free_resource(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct device *dev = &xdev->conf.pdev->dev;
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	struct qdma_sw_sg *sdesc = flq->sdesc;
	struct qdma_c2h_desc *desc = flq->desc;
	unsigned char pg_order = flq->pg_order;
	int i;

	for (i = 0; i < flq->size; i++, sdesc++, desc++) {
		if (sdesc)
			flq_free_one(sdesc, desc, dev, pg_order);
		else
			break;
	}

	kfree(flq->sdesc);
	flq->sdesc = NULL;
	flq->sdesc_info = NULL;

	memset(flq, 0, sizeof(struct qdma_flq));
}

int descq_flq_alloc_resource(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	struct device *dev = &xdev->conf.pdev->dev;
	int node = dev_to_node(dev);
	struct qdma_sw_sg *sdesc, *prev = NULL;
	struct qdma_sdesc_info *sinfo, *sprev = NULL;
	struct qdma_c2h_desc *desc = flq->desc;
	int i;
	int rv = 0;

	sdesc = kzalloc_node(flq->size * (sizeof(struct qdma_sw_sg) +
					  sizeof(struct qdma_sdesc_info)),
				GFP_KERNEL, node);
	if (!sdesc) {
		pr_info("OOM, sz %u.\n", flq->size);
		return -ENOMEM;
	}
	flq->sdesc = sdesc;
	flq->sdesc_info = sinfo = (struct qdma_sdesc_info *)(sdesc + flq->size);

	/* make the flq to be a linked list ring */
	for (i = 0; i < flq->size; i++, prev = sdesc, sdesc++,
					sprev = sinfo, sinfo++) {
		if (prev)
			prev->next = sdesc;
		if (sprev)
			sprev->next = sinfo;
	}
	/* last entry's next points to the first entry */
	prev->next = flq->sdesc;
	sprev->next = flq->sdesc_info;

	for (sdesc = flq->sdesc, i = 0; i < flq->size; i++, sdesc++, desc++) {
		rv = flq_fill_one(sdesc, desc, dev, node, descq->conf.c2h_bufsz,
				  flq->pg_order, GFP_KERNEL);
		if (rv < 0) {
			descq_flq_free_resource(descq);
			return rv;
		}
	}

	return 0;
}

static int qdma_flq_refill(struct qdma_descq *descq, int idx, int count,
			int recycle, gfp_t gfp)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	struct qdma_sw_sg *sdesc = flq->sdesc + idx;
	struct qdma_c2h_desc *desc = flq->desc + idx;
	struct qdma_sdesc_info *sinfo = flq->sdesc_info + idx;
	int order = flq->pg_order;
	int i;

	for (i = 0; i < count; i++, idx++, sdesc++, desc++, sinfo++) {

		if (idx == flq->size) {
			idx = 0;
			sdesc = flq->sdesc;
			desc = flq->desc;
			sinfo = flq->sdesc_info;
		}

		if (recycle) {
			sdesc->len = (descq->conf.c2h_bufsz << order);
			sdesc->offset = 0;
		} else {
			struct device *dev = &xdev->conf.pdev->dev;
			int node = dev_to_node(dev);
			int rv;

			flq_unmap_one(sdesc, desc, dev, order);
			rv = flq_fill_one(sdesc, desc, dev, node,
					  descq->conf.c2h_bufsz, order, gfp);
			if (unlikely(rv < 0)) {
				if (rv == -ENOMEM)
					flq->alloc_fail++;
				else
					flq->mapping_err++;

				break;
			}
		}
		sinfo->fbits = 0;
		descq->avail++;
	}
	if (list_empty(&descq->work_list) &&
			list_empty(&descq->pend_list)) {
		descq->pend_list_empty = 1;
		if (descq->q_stop_wait)
			qdma_waitq_wakeup(&descq->pend_list_wq);
	}

	return i;
}

/*
 *
 */
int descq_st_c2h_read(struct qdma_descq *descq, struct qdma_request *req,
			bool update_pidx, bool refill)
{
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	unsigned int pidx = flq->pidx_pend;
	struct qdma_sw_sg *fsg = flq->sdesc + pidx;
	struct qdma_sw_sg *tsg = req->sgl;
	unsigned int fsgcnt = ring_idx_delta(descq->pidx, pidx, flq->size);
	unsigned int tsgoff = cb->sg_offset;
	unsigned int foff = 0;
	int i = 0, j = 0;
	int rv = 0;
	unsigned int copied = 0;

	if (!fsgcnt)
		return 0;

	if (cb->sg_idx) {
		for (; tsg && j < cb->sg_idx; j++)
			tsg = tsg->next;

		if (!tsg) {
			pr_err("tsg error, index %u/%u.\n",
				cb->sg_idx, req->sgcnt);
			return -EINVAL;
		}
	}

	while ((i < fsgcnt) && tsg) {
		unsigned int flen = fsg->len;
		unsigned char *faddr = page_address(fsg->pg) + fsg->offset;

		foff = 0;

		while (flen && tsg) {
			unsigned int toff = tsg->offset + tsgoff;
			unsigned int copy = min_t(unsigned int, flen,
						 tsg->len - tsgoff);

			if (!req->no_memcpy)
				memcpy(page_address(tsg->pg) + toff,
				       faddr, copy);

			faddr += copy;
			flen -= copy;
			foff += copy;
			tsgoff += copy;
			copied += copy;

			if (tsgoff == tsg->len) {
				tsg = tsg->next;
				tsgoff = 0;
				j++;
			}
		}

		if (foff == fsg->len) {
			pidx = ring_idx_incr(pidx, 1, descq->conf.rngsz);
			i++;
			foff = 0;
			fsg = fsg->next;
		}
	}

	incr_cmpl_desc_cnt(descq, i);

	if (refill && i)
		qdma_flq_refill(descq, flq->pidx_pend, i, 1, GFP_ATOMIC);

	flq->pidx_pend = ring_idx_incr(flq->pidx_pend, i, flq->size);
	if (foff) {
		fsg->offset += foff;
		fsg->len -= foff;
	}

	if (i && update_pidx) {
		i = ring_idx_decr(flq->pidx_pend, 1, flq->size);
		descq->pidx_info.pidx = i;
		rv = queue_pidx_update(descq->xdev, descq->conf.qidx,
				descq->conf.q_type, &descq->pidx_info);
		if (unlikely(rv < 0)) {
			pr_err("%s: Failed to update pidx\n",
					descq->conf.name);
			return -EINVAL;
		}
	}

	cb->sg_idx = j;
	cb->sg_offset = tsgoff;
	cb->left -= copied;

	flq->pkt_dlen -= copied;

	return copied;
}

static int qdma_c2h_packets_proc_dflt(struct qdma_descq *descq)
{
	struct qdma_sgt_req_cb *cb, *tmp;

	list_for_each_entry_safe(cb, tmp, &descq->pend_list, list) {
		int rv;

		/* check for zero length dma */
		if (!cb->left) {
			pr_debug("%s, cb 0x%p pending, zero len.\n",
				descq->conf.name, cb);

			qdma_sgt_req_done(descq, cb, 0);
			return 0;
		}

		rv = descq_st_c2h_read(descq, (struct qdma_request *)cb, 0, 0);
		if (rv < 0) {
			pr_info("req 0x%p, error %d.\n", cb, rv);
			qdma_sgt_req_done(descq, cb, rv);
			continue;
		}

		if (!cb->left)
			qdma_sgt_req_done(descq, cb, 0);
		else
			break; /* no more data available */
	}

	return 0;
}

void cmpt_next(struct qdma_descq *descq)
{
	u8 *desc_cmpt_cur = descq->desc_cmpt_cur + descq->cmpt_entry_len;

	descq->desc_cmpt_cur = desc_cmpt_cur;
	if (unlikely(++descq->cidx_cmpt == descq->conf.rngsz_cmpt)) {
		descq->cidx_cmpt = 0;
		descq->color ^= 1;
		descq->desc_cmpt_cur = descq->desc_cmpt;
	}
}

static inline bool is_new_cmpl_entry(struct qdma_descq *descq,
					struct qdma_ul_cmpt_info *cmpl)
{
	return cmpl->f.color == descq->color;
}

int parse_cmpl_entry(struct qdma_descq *descq, struct qdma_ul_cmpt_info *cmpl)
{
	__be64 *cmpt = (__be64 *)descq->desc_cmpt_cur;

	dma_rmb();

#if 0
	print_hex_dump(KERN_INFO, "cmpl entry ", DUMP_PREFIX_OFFSET,
			16, 1, (void *)cmpt, descq->cmpt_entry_len,
			false);
#endif

	cmpl->entry = cmpt;
	cmpl->f.format = cmpt[0] & F_C2H_CMPT_ENTRY_F_FORMAT ? 1 : 0;
	cmpl->f.color = cmpt[0] & F_C2H_CMPT_ENTRY_F_COLOR ? 1 : 0;
	cmpl->f.err = cmpt[0] & F_C2H_CMPT_ENTRY_F_ERR ? 1 : 0;
	cmpl->f.eot = cmpt[0] & F_C2H_CMPT_ENTRY_F_EOT ? 1 : 0;
	cmpl->f.desc_used = cmpt[0] & F_C2H_CMPT_ENTRY_F_DESC_USED ? 1 : 0;
	if (!cmpl->f.format && cmpl->f.desc_used) {
		cmpl->len = (cmpt[0] >> S_C2H_CMPT_ENTRY_LENGTH) &
				M_C2H_CMPT_ENTRY_LENGTH;
		/* zero length transfer allowed */
	} else
		cmpl->len = 0;

	return 0;
}

static int rcv_pkt(struct qdma_descq *descq, struct qdma_ul_cmpt_info *cmpl,
			unsigned int len)
{
	unsigned int pidx = cmpl->pidx;
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	unsigned int pg_shift = flq->pg_shift;
	unsigned int pg_mask = (1 << pg_shift) - 1;
	unsigned int rngsz = descq->conf.rngsz;
	/* zero length still uses one descriptor */
	int fl_nr = len ? ((len + pg_mask) >> pg_shift) : 1;
	unsigned int last = ring_idx_incr(cmpl->pidx, fl_nr - 1, rngsz);
	unsigned int next = ring_idx_incr(last, 1, rngsz);
	struct qdma_sw_sg *sdesc = flq->sdesc + last;
	unsigned int cidx_next = ring_idx_incr(descq->cidx_cmpt, 1,
					descq->conf.rngsz_cmpt);

	if (descq->avail < fl_nr)
		return -EBUSY;
	descq->avail -= fl_nr;

	if (len) {
		unsigned int last_len = len & pg_mask;

		if (last_len)
			sdesc->len = last_len;
	} else
		sdesc->len = 0;

	if (descq->conf.fp_descq_c2h_packet) {
		int rv = descq->conf.fp_descq_c2h_packet(descq->q_hndl,
				descq->conf.quld, len, fl_nr, flq->sdesc + pidx,
				descq->conf.cmpl_udd_en ?
				(unsigned char *)cmpl->entry : NULL);

		if (rv < 0)
			return rv;
		flq->pidx_pend = next;
	} else {
		int i;
		struct qdma_sdesc_info *sinfo = flq->sdesc_info + pidx;

		for (i = 0; i < fl_nr; i++, sinfo = sinfo->next) {
			WARN_ON(sinfo->f.valid);
			sinfo->f.valid = 1;
			sinfo->cidx = cidx_next;
		}

		flq->sdesc_info[pidx].f.sop = 1;
		flq->sdesc_info[last].f.eop = 1;

		flq->pkt_dlen += len;
		if (descq->conf.cmpl_udd_en)
			flq->udd_cnt++;
	}
	cmpl->pidx = next;

	return 0;
}

int rcv_udd_only(struct qdma_descq *descq, struct qdma_ul_cmpt_info *cmpl)
{
#ifdef XMP_DISABLE_ST_C2H_CMPL
	__be64 cmpt_entry = cmpl->entry[0];
#endif
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;

	pr_debug("%s, rcv udd.\n", descq->conf.name);
#if 0
	print_hex_dump(KERN_INFO, "cmpl entry: ", DUMP_PREFIX_OFFSET,
			16, 1, (void *)cmpl->entry, descq->cmpt_entry_len,
			false);
#endif

	/* udd only: no descriptor used */
	if (descq->conf.fp_descq_c2h_packet) {
		return descq->conf.fp_descq_c2h_packet(descq->q_hndl,
				descq->conf.quld, 0, 0, NULL,
				(unsigned char *)cmpl->entry);
	}
#ifdef XMP_DISABLE_ST_C2H_CMPL
	if ((cmpt_entry & (1 << 20)) > 0) {
		__be16 pkt_cnt = (cmpt_entry >> 32) & 0xFFFF;
		__be16 pkt_len = (cmpt_entry >> 48) & 0xFFFF;
		int i;

		pr_info("pkt %u * %u.\n", pkt_len, pkt_cnt);
		for (i = 0; i < pkt_cnt; i++) {
			int rv = rcv_pkt(descq, cmpl, pkt_len);

			if (rv < 0)
				break;
		}
	}
#endif
	flq->udd_cnt++;

	return 0;
}

static void descq_adjust_c2h_cntr_avgs(struct qdma_descq *descq)
{
	int i;
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct global_csr_conf *csr_info = &xdev->csr_info;
	uint8_t latecy_optimized = descq->conf.latency_optimize;

	descq->c2h_pend_pkt_moving_avg =
		csr_info->c2h_cnt_th[descq->cmpt_cidx_info.counter_idx];

	if (descq->sorted_c2h_cntr_idx == (QDMA_GLOBAL_CSR_ARRAY_SZ - 1))
		i = xdev->sorted_c2h_cntr_idx[descq->sorted_c2h_cntr_idx];
	else
		i = xdev->sorted_c2h_cntr_idx[descq->sorted_c2h_cntr_idx + 1];

	if (latecy_optimized)
		descq->c2h_pend_pkt_avg_thr_hi = (csr_info->c2h_cnt_th[i] +
					csr_info->c2h_cnt_th[i]);
	else
		descq->c2h_pend_pkt_avg_thr_hi =
				(descq->c2h_pend_pkt_moving_avg +
						csr_info->c2h_cnt_th[i]);

	if (descq->sorted_c2h_cntr_idx > 0)
		i = xdev->sorted_c2h_cntr_idx[descq->sorted_c2h_cntr_idx - 1];
	else
		i = xdev->sorted_c2h_cntr_idx[descq->sorted_c2h_cntr_idx];
	if (latecy_optimized)
		descq->c2h_pend_pkt_avg_thr_lo = (csr_info->c2h_cnt_th[i] +
				csr_info->c2h_cnt_th[i]);
	else
		descq->c2h_pend_pkt_avg_thr_lo =
				(descq->c2h_pend_pkt_moving_avg +
						csr_info->c2h_cnt_th[i]);

	descq->c2h_pend_pkt_avg_thr_hi >>= 1;
	descq->c2h_pend_pkt_avg_thr_lo >>= 1;
	pr_debug("q%u: c2h_cntr_idx =  %u %u %u", descq->conf.qidx,
		descq->cmpt_cidx_info.counter_idx,
		descq->c2h_pend_pkt_avg_thr_lo,
		descq->c2h_pend_pkt_avg_thr_hi);
}

void descq_incr_c2h_cntr_th(struct qdma_descq *descq)
{
	unsigned char i, c2h_cntr_idx;
	unsigned char c2h_cntr_val_new;
	unsigned char c2h_cntr_val_curr;

	if (descq->sorted_c2h_cntr_idx ==
			(QDMA_GLOBAL_CSR_ARRAY_SZ - 1))
		return;
	descq->c2h_cntr_monitor_cnt = 0;
	i = descq->sorted_c2h_cntr_idx;
	c2h_cntr_idx = descq->xdev->sorted_c2h_cntr_idx[i];
	c2h_cntr_val_curr = descq->xdev->csr_info.c2h_cnt_th[c2h_cntr_idx];
	i++;
	c2h_cntr_idx = descq->xdev->sorted_c2h_cntr_idx[i];
	c2h_cntr_val_new = descq->xdev->csr_info.c2h_cnt_th[c2h_cntr_idx];

	if ((c2h_cntr_val_new >= descq->c2h_pend_pkt_moving_avg) &&
		(c2h_cntr_val_new - descq->c2h_pend_pkt_moving_avg) >=
		(descq->c2h_pend_pkt_moving_avg - c2h_cntr_val_curr))
		return; /* choosing the closest */
	/* do not allow c2h c2ntr value go beyond half of cmpt rng sz*/
	if (c2h_cntr_val_new < (descq->conf.rngsz >> 1)) {
		descq->cmpt_cidx_info.counter_idx = c2h_cntr_idx;
		descq->sorted_c2h_cntr_idx = i;
		descq_adjust_c2h_cntr_avgs(descq);
	}
}

void descq_decr_c2h_cntr_th(struct qdma_descq *descq, unsigned int budget)
{
	unsigned char i, c2h_cntr_idx;
	unsigned char c2h_cntr_val_new;
	unsigned char c2h_cntr_val_curr;

	if (!descq->sorted_c2h_cntr_idx)
		return;
	descq->c2h_cntr_monitor_cnt = 0;
	i = descq->sorted_c2h_cntr_idx;
	c2h_cntr_idx = descq->xdev->sorted_c2h_cntr_idx[i];
	c2h_cntr_val_curr = descq->xdev->csr_info.c2h_cnt_th[c2h_cntr_idx];
	i--;
	c2h_cntr_idx = descq->xdev->sorted_c2h_cntr_idx[i];

	c2h_cntr_val_new = descq->xdev->csr_info.c2h_cnt_th[c2h_cntr_idx];

	if ((c2h_cntr_val_new <= descq->c2h_pend_pkt_moving_avg) &&
		(descq->c2h_pend_pkt_moving_avg - c2h_cntr_val_new) >=
		(c2h_cntr_val_curr - descq->c2h_pend_pkt_moving_avg))
		return; /* choosing the closest */

	/* for better performance we do not allow c2h_cnt
	 * val below budget unless latency optimized
	 * '-2' is SW work around for HW bug
	 */
	if (!descq->conf.latency_optimize &&
			(c2h_cntr_val_new < (budget - 2)))
		return;

	descq->cmpt_cidx_info.counter_idx = c2h_cntr_idx;

	descq->sorted_c2h_cntr_idx = i;
	descq_adjust_c2h_cntr_avgs(descq);
}


#define MAX_C2H_CNTR_STAGNANT_CNT 16

static void descq_adjust_c2h_cntr_th(struct qdma_descq *descq,
					 unsigned int pend, unsigned int budget)
{
	descq->c2h_pend_pkt_moving_avg += pend;
	descq->c2h_pend_pkt_moving_avg >>= 1; /* average */
	/* if avg > hi_th, increase the counter
	 * if avg < lo_th, decrease the counter
	 */
	if (descq->c2h_pend_pkt_avg_thr_hi <= descq->c2h_pend_pkt_moving_avg)
		descq_incr_c2h_cntr_th(descq);
	else if (descq->c2h_pend_pkt_avg_thr_lo >=
				descq->c2h_pend_pkt_moving_avg)
		descq_decr_c2h_cntr_th(descq, budget);
	else {
		descq->c2h_cntr_monitor_cnt++;
		if (descq->c2h_cntr_monitor_cnt == MAX_C2H_CNTR_STAGNANT_CNT) {
			/* go down on counter value to see if we actually are
			 * increasing latency by setting
			 * higher counter threshold
			 */
			descq_decr_c2h_cntr_th(descq, budget);
			descq->c2h_cntr_monitor_cnt = 0;
		} else
			return;
	}
}

int descq_cmpl_err_check(struct qdma_descq *descq,
			 struct qdma_ul_cmpt_info *cmpl)
{
	/*
	 * format = 1 does not have length field, so the driver cannot
	 * figure out how many descriptor is used
	 */
	if (unlikely(cmpl->f.format)) {
		pr_err("%s: ERR cmpl. entry %u format=1.\n",
			descq->conf.name, descq->cidx_cmpt);
		goto err_out;
	}

	if (unlikely(!cmpl->f.desc_used && !descq->conf.cmpl_udd_en)) {
		pr_warn("%s, ERR cmpl entry %u, desc_used 0, udd_en 0.\n",
			descq->conf.name, descq->cidx_cmpt);
		goto err_out;
	}

	if (unlikely(cmpl->f.err)) {
		pr_warn("%s, ERR cmpl entry %u error set\n",
				descq->conf.name, descq->cidx_cmpt);
		goto err_out;
	}

	return 0;
err_out:
	descq->err = 1;
	print_hex_dump(KERN_INFO, "cmpl entry: ", DUMP_PREFIX_OFFSET,
			16, 1, (void *)cmpl, descq->cmpt_entry_len,
			false);
	return -EINVAL;

}

int descq_process_completion_st_c2h(struct qdma_descq *descq, int budget,
					bool upd_cmpl)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct qdma_c2h_cmpt_cmpl_status *cs =
			(struct qdma_c2h_cmpt_cmpl_status *)
			descq->desc_cmpt_cmpl_status;
	struct qdma_queue_conf *qconf = &descq->conf;
	unsigned int rngsz_cmpt = qconf->rngsz_cmpt;
	unsigned int pidx = descq->pidx;
	unsigned int cidx_cmpt = descq->cidx_cmpt;
	unsigned int pidx_cmpt = cs->pidx;
	struct qdma_flq *flq = (struct qdma_flq *)descq->flq;
	unsigned int pidx_pend = flq->pidx_pend;
	bool uld_handler = descq->conf.fp_descq_c2h_packet ? true : false;
	unsigned char is_ul_ext = (qconf->desc_bypass &&
			qconf->fp_proc_ul_cmpt_entry) ? 1 : 0;
	int pend, ret = 0;
	int proc_cnt = 0;
	int rv = 0;
	int read_weight = budget;

	/* once an error happens, stop processing of the Q */
	if (descq->err) {
		pr_info("%s: err.\n", descq->conf.name);
		return 0;
	}

	dma_rmb();
	pend = ring_idx_delta(pidx_cmpt, cidx_cmpt, rngsz_cmpt);
	if (!pend) {
		/* SW work around where next interrupt could be missed when
		 * there are no entries as of now
		 */
		if (descq->xdev->conf.qdma_drv_mode != POLL_MODE) {
			rv = queue_cmpt_cidx_update(descq->xdev,
					descq->conf.qidx,
					&descq->cmpt_cidx_info);
			if (unlikely(rv < 0))  {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);
				return -EINVAL;
			}
		}
		return 0;
	}

#if 0
	print_hex_dump(KERN_INFO, "cmpl status: ", DUMP_PREFIX_OFFSET,
				16, 1, (void *)cs, sizeof(*cs),
				false);
	pr_info("cmpl status: pidx 0x%x, cidx %x, color %d, int_state 0x%x.\n",
		cs->pidx, cs->cidx, cs->color_isr_status & 0x1,
		(cs->color_isr_status >> 1) & 0x3);
#endif

	flq->pkt_cnt = pend;

	if (!budget || budget > pend)
		budget = pend;

	while (likely(proc_cnt < budget)) {
		struct qdma_ul_cmpt_info cmpl;
		int rv;

		memset(&cmpl, 0, sizeof(struct qdma_ul_cmpt_info));
		if (is_ul_ext)
			rv = qconf->fp_proc_ul_cmpt_entry(descq->desc_cmpt_cur,
							  &cmpl);
		else
			rv = parse_cmpl_entry(descq, &cmpl);
		/* completion entry error, q is halted */
		if (rv < 0)
			return rv;
		rv = descq_cmpl_err_check(descq, &cmpl);
		if (rv < 0)
			return rv;

		if (!is_new_cmpl_entry(descq, &cmpl))
			break;

		cmpl.pidx = pidx;

		if (cmpl.f.desc_used) {
			rv = rcv_pkt(descq, &cmpl, cmpl.len);
		} else if (descq->conf.cmpl_udd_en) {
			/* udd only: no descriptor used */
			rv = rcv_udd_only(descq, &cmpl);
		}

		if (rv < 0) /* cannot process now, stop */
			break;

		pidx = cmpl.pidx;

		cmpt_next(descq);
		proc_cnt++;
	}

	flq->pkt_cnt -= proc_cnt;

	if ((xdev->conf.intr_moderation) &&
			(descq->cmpt_cidx_info.trig_mode ==
					TRIG_MODE_COMBO)) {
		pend = ring_idx_delta(cs->pidx, descq->cidx_cmpt, rngsz_cmpt);
		flq->pkt_cnt = pend;

		/* we dont need interrupt if packets available for next read */
		if (read_weight && (flq->pkt_cnt > read_weight))
			descq->cmpt_cidx_info.irq_en = 0;
		else
			descq->cmpt_cidx_info.irq_en = 1;

		/* if we use just then at right value of c2h_cntr
		 * the average goes down as there
		 * will not be many pend packet.
		 */
		if (descq->conf.adaptive_rx)
			descq_adjust_c2h_cntr_th(descq, pend + proc_cnt,
						 read_weight);
	}

	if (proc_cnt) {
		descq->pidx_cmpt = pidx_cmpt;
		descq->pidx = pidx;
		descq->cmpt_cidx_info.wrb_cidx = descq->cidx_cmpt;
		if (!descq->conf.fp_descq_c2h_packet) {
			rv = queue_cmpt_cidx_update(descq->xdev,
				descq->conf.qidx, &descq->cmpt_cidx_info);
			if (unlikely(rv < 0)) {
				pr_err("%s: Failed to update cmpt cidx\n",
						descq->conf.name);
				return -EINVAL;
			}
			qdma_c2h_packets_proc_dflt(descq);
		}

		flq->pkt_cnt = ring_idx_delta(cs->pidx, descq->cidx_cmpt,
					      rngsz_cmpt);

		/* some descq entries have been consumed */
		if (flq->pidx_pend != pidx_pend) {
			pend = ring_idx_delta(flq->pidx_pend, pidx_pend,
						flq->size);
			qdma_flq_refill(descq, pidx_pend, pend,
					uld_handler ? 0 : 1, GFP_ATOMIC);

			if (upd_cmpl && !descq->q_stop_wait) {
				pend = ring_idx_decr(flq->pidx_pend, 1,
						     flq->size);
				descq->pidx_info.pidx = pend;
				if (!descq->conf.fp_descq_c2h_packet) {
					ret = queue_pidx_update(descq->xdev,
							descq->conf.qidx,
							descq->conf.q_type,
							&descq->pidx_info);
					if (unlikely(ret < 0))  {
						pr_err("%s: Failed to update pidx\n",
							descq->conf.name);
						return -EINVAL;
					}
				}
			}
		}
	}

	return 0;
}

int qdma_queue_c2h_peek(unsigned long dev_hndl, unsigned long id,
			unsigned int *udd_cnt, unsigned int *pkt_cnt,
			unsigned int *data_len)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq;
	struct qdma_flq *flq;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	descq = qdma_device_get_descq_by_id(xdev, id, NULL, 0, 0);
	if (!descq) {
		pr_err("Invalid qid(%ld)", id);
		return -EINVAL;
	}

	flq = (struct qdma_flq *)descq->flq;
	*udd_cnt = flq->udd_cnt;
	*pkt_cnt = flq->pkt_cnt;
	*data_len = flq->pkt_dlen;

	return 0;
}

int qdma_queue_packet_read(unsigned long dev_hndl, unsigned long id,
			struct qdma_request *req, struct qdma_cmpl_ctrl *cctrl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq;
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);
	int rv = 0;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	if (!req) {
		pr_err("req is NULL");
		return -EINVAL;
	}

	descq = qdma_device_get_descq_by_id(xdev, id, NULL, 0, 1);
	if (!descq) {
		pr_err("Invalid qid(%ld)", id);
		return -EINVAL;
	}

	if (!descq->conf.st || (descq->conf.q_type != Q_C2H)) {
		pr_info("%s: st %d, type %d.\n",
			descq->conf.name, descq->conf.st, descq->conf.q_type);
		return -EINVAL;
	}

	if (cctrl) {
		lock_descq(descq);

		descq->cmpt_cidx_info.trig_mode =
			descq->conf.cmpl_trig_mode = cctrl->trigger_mode;
		descq->cmpt_cidx_info.timer_idx =
			descq->conf.cmpl_timer_idx = cctrl->timer_idx;
		descq->cmpt_cidx_info.counter_idx =
			descq->conf.cmpl_cnt_th_idx = cctrl->cnt_th_idx;
		descq->cmpt_cidx_info.irq_en =
			descq->conf.cmpl_en_intr = cctrl->cmpl_en_intr;
		descq->cmpt_cidx_info.wrb_en =
			descq->conf.cmpl_stat_en = cctrl->en_stat_desc;

		descq->cmpt_cidx_info.wrb_cidx = descq->cidx_cmpt;

		rv = queue_cmpt_cidx_update(descq->xdev,
				descq->conf.qidx, &descq->cmpt_cidx_info);
		if (unlikely(rv < 0)) {
			pr_err("%s: Failed to update cmpt cidx\n",
					descq->conf.name);
			unlock_descq(descq);
			return -EINVAL;
		}

		unlock_descq(descq);
	}

	memset(cb, 0, QDMA_REQ_OPAQUE_SIZE);

	qdma_waitq_init(&cb->wq);

	lock_descq(descq);
	descq_st_c2h_read(descq, req, 1, 1);
	unlock_descq(descq);

	return req->count - cb->left;
}
