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
#include <linux/pci.h>
#include "qdma_device.h"
#include "qdma_descq.h"
#include "qdma_intr.h"
#include "qdma_regs.h"
#include "qdma_context.h"

/**
 * Make the interrupt context
 */
static int make_intr_context(struct xlnx_dma_dev *xdev, u32 *data, int cnt)
{
	int i, j;

	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		memset(data, 0, cnt * sizeof(u32));
		return 0;
	}

	 /** Each context size is QDMA_REG_IND_CTXT_WCNT_3
	  *  data[QDMA_REG_IND_CTXT_WCNT_3 *  QDMA_NUM_DATA_VEC_FOR_INTR_CXT]
	  */
	if (cnt <
		(QDMA_NUM_DATA_VEC_FOR_INTR_CXT * QDMA_REG_IND_CTXT_WCNT_3)) {
		pr_warn("%s, intr context %d < (%d * %d).\n",
			xdev->conf.name, cnt,
			 QDMA_NUM_DATA_VEC_FOR_INTR_CXT,
			QDMA_REG_IND_CTXT_WCNT_3);
		return -EINVAL;
	}

	memset(data, 0, cnt * sizeof(u32));
	/** program the coalescing context
	 *  i -> Number of vectors
	 *  j -> number of words for each vector context
	 */
	for (i = 0, j = 0; i <  QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
		u64 bus_64;
		u32 v;
		struct intr_coal_conf *entry = (xdev->intr_coal_list + i);

		/* TBD:
		 * Assume that Qid is irrelevant for interrupt context
		 * programming because, interrupt context is done per vector
		 * which for the function and not for each queue
		 */

		bus_64 = (PCI_DMA_H(entry->intr_ring_bus) << 20) |
			 ((PCI_DMA_L(entry->intr_ring_bus)) >> 12);

		v = bus_64 & M_INT_AGGR_W0_BADDR_64;

		/** Data[0] */
		data[j] = (1 << S_INT_AGGR_W0_F_VALID |
			   V_INT_AGGR_W0_VEC_ID(entry->vec_id)|
			   V_INT_AGGR_W0_BADDR_64(v) |
			   1 << S_INT_AGGR_W0_F_COLOR);

		/** Data[1] */
		v = (bus_64 >> L_INT_AGGR_W0_BADDR_64);
		data[++j] = v;

		v = (bus_64 >>
			(L_INT_AGGR_W0_BADDR_64 + 32)) & M_INT_AGGR_W2_BADDR_64;
		/** Data[3] */
		data[++j] = ((V_INT_AGGR_W2_BADDR_64(v)) |
				V_INT_AGGR_W2_VEC_SIZE(xdev->conf.intr_rngsz));

		j++;
	}

	return 0;
}

#ifndef __QDMA_VF__
static int make_stm_c2h_context(struct qdma_descq *descq, u32 *data)
{
	int pipe_slr_id = descq->conf.pipe_slr_id;
	int pipe_flow_id = descq->conf.pipe_flow_id;
	int pipe_tdest = descq->conf.pipe_tdest;

	/* 191..160 */
	data[5] = F_STM_C2H_CTXT_ENTRY_VALID;

	/* 128..159 */
	data[1] = (pipe_slr_id << S_STM_CTXT_C2H_SLR) |
		  ((pipe_tdest & 0xFF00) << S_STM_CTXT_C2H_TDEST_H);

	/* 96..127 */
	data[0] = ((pipe_tdest & 0xFF) << S_STM_CTXT_C2H_TDEST_L) |
		  (pipe_flow_id << S_STM_CTXT_C2H_FID);

	pr_debug("%s, STM 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x.\n",
		 descq->conf.name, data[0], data[1], data[2], data[3], data[4],
		 data[5]);
	return 0;
}

static int make_stm_h2c_context(struct qdma_descq *descq, u32 *data)
{
	int pipe_slr_id = descq->conf.pipe_slr_id;
	int pipe_flow_id = descq->conf.pipe_flow_id;
	int pipe_tdest = descq->conf.pipe_tdest;
	int dppkt = 1;
	int log2_dppkt = ilog2(dppkt);
	int pkt_lim = 0;
	int max_ask = 8;

	/* 191..160 */
	data[5] = F_STM_H2C_CTXT_ENTRY_VALID;

	/* 128..159 */
	data[4] = (descq->qidx_hw << S_STM_CTXT_QID);

	/* 96..127 */
	data[3] = (pipe_slr_id << S_STM_CTXT_H2C_SLR) |
		  ((pipe_tdest & 0xFF00) << S_STM_CTXT_H2C_TDEST_H);

	/* 64..95 */
	data[2] = ((pipe_tdest & 0xFF) << S_STM_CTXT_H2C_TDEST_L) |
		  (pipe_flow_id << S_STM_CTXT_H2C_FID) |
		  (pkt_lim << S_STM_CTXT_PKT_LIM) |
		  (max_ask << S_STM_CTXT_MAX_ASK);

	/* 32..63 */
	data[1] = (dppkt << S_STM_CTXT_DPPKT) |
		  (log2_dppkt << S_STM_CTXT_LOG2_DPPKT);

	/* 0..31 */
	/** explicitly init to 8 to workaround hw issue due to which the value
	 * is getting initialized to zero instead of its reset value of 8
	 */
	data[0] = 8;

	pr_debug("%s, STM 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x.\n",
		 descq->conf.name, data[0], data[1], data[2], data[3], data[4],
		 data[5]);
	return 0;
}
#endif

static int make_sw_context(struct qdma_descq *descq, u32 *data, int cnt)
{
	int ring_index;

	if (cnt < QDMA_REG_IND_CTXT_WCNT_5) {
		pr_warn("%s, sw context count %d < %d.\n",
			descq->xdev->conf.name, cnt, QDMA_REG_IND_CTXT_WCNT_5);
		return -EINVAL;
	}
	memset(data, 0, cnt * sizeof(u32));

	/* sw context */
	if ((descq->xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(descq->xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		ring_index = get_intr_ring_index(descq->xdev, descq->intr_id);
		data[4] = V_DESC_CTXT_W4_VEC(ring_index) |
				(0x01 << S_DESC_CTXT_W1_F_INTR_AGGR);
	} else {
		data[4] = V_DESC_CTXT_W4_VEC(descq->intr_id);
	}

	data[3] = PCI_DMA_H(descq->desc_bus);
	data[2] = PCI_DMA_L(descq->desc_bus);
	data[1] = (1 << S_DESC_CTXT_W1_F_QEN) |
			(descq->conf.cmpl_status_pend_chk <<
					S_DESC_CTXT_W1_F_CMPL_STATUS_PEND_CHK) |
			(descq->conf.cmpl_status_acc_en <<
					S_DESC_CTXT_W1_F_CMPL_STATUS_ACC_EN) |
			V_DESC_CTXT_W1_RNG_SZ(descq->conf.desc_rng_sz_idx) |
			(descq->conf.desc_bypass << S_DESC_CTXT_W1_F_BYP) |
			(descq->conf.cmpl_status_en <<
					S_DESC_CTXT_W1_F_CMPL_STATUS_EN) |
			(descq->conf.irq_en << S_DESC_CTXT_W1_F_IRQ_EN) |
			(~descq->conf.st << S_DESC_CTXT_W1_F_IS_MM);

	if (descq->conf.desc_bypass &&
			(descq->conf.sw_desc_sz == DESC_SZ_64B)) {
		data[1] |= V_DESC_CTXT_W1_DSC_SZ(descq->conf.sw_desc_sz);
	} else {
		if (!descq->conf.st) { /* mm h2c/c2h */
			data[1] |= (V_DESC_CTXT_W1_DSC_SZ(DESC_SZ_32B)) |
				   (descq->channel << S_DESC_CTXT_W1_F_MM_CHN);
		} else if (descq->conf.c2h) {  /* st c2h */
			data[1] |= (descq->conf.fetch_credit <<
					S_DESC_CTXT_W1_F_FCRD_EN) |
				(V_DESC_CTXT_W1_DSC_SZ(DESC_SZ_8B));
		} else { /* st h2c */
			data[1] |= V_DESC_CTXT_W1_DSC_SZ(DESC_SZ_16B);

			/* For STM & TM mode, set fcrd_en for ST H2C */
			if (descq->xdev->stm_en ||
			    descq->xdev->conf.tm_mode_en)
				data[1] |= (descq->conf.fetch_credit <<
					    S_DESC_CTXT_W1_F_FCRD_EN);
		}
	}

	/* pidx = 0; irq_ack = 0 */
	data[0] = ((V_DESC_CTXT_W0_FUNC_ID(descq->xdev->func_id)) |
			(((descq->xdev->conf.qdma_drv_mode !=
					POLL_MODE) ? 1 : 0) <<
					S_DESC_CTXT_W0_F_INTR_ARM));

#ifdef ERR_DEBUG
	if (descq->induce_err & (1 << param)) {
		data[0] |= (0xFFF << S_DESC_CTXT_W0_FUNC_ID);
		pr_info("induced error %d", ind_ctxt_cmd_err);
	}
#endif

	pr_debug("%s, SW 0x%08x 0x%08x, 0x%08x, 0x%08x, 0x%08x.\n",
		descq->conf.name, data[4], data[3], data[2], data[1], data[0]);

	return 0;
}

/* ST: prefetch context setup */
static int make_prefetch_context(struct qdma_descq *descq, u32 *data, int cnt)
{
	BUG_ON(!descq);
	BUG_ON(!data);

	if (cnt < QDMA_REG_IND_CTXT_WCNT_2) {
		pr_warn("%s, prefetch context count %d < %d.\n",
			descq->conf.name, cnt, QDMA_REG_IND_CTXT_WCNT_2);
		return -EINVAL;
	}
	memset(data, 0, cnt * sizeof(u32));

	/* prefetch context */
	data[1] = 1 << S_PFTCH_W1_F_VALID;
	data[0] = (descq->conf.pfetch_bypass << S_PFTCH_W0_F_BYPASS) |
		(descq->conf.c2h_buf_sz_idx << S_PFTCH_W0_BUF_SIZE_IDX) |
		/** TBD: this code needs to be deleted once the PG is updated.
		 * func_id bits are going to be reserved
		 * need to get clarity on port id
		 */
		//(descq->conf.port_id << S_PFTCH_W0_PORT_ID) |
		// (descq->xdev->func_id << S_PFTCH_W0_FUNC_ID) |
		  (descq->conf.pfetch_en << S_PFTCH_W0_F_EN_PFTCH);

	pr_debug("%s, PFTCH 0x%08x 0x%08x\n",
		descq->conf.name, data[1], data[0]);

	return 0;
}

/* ST C2H : writeback context setup */
static int make_cmpt_context(struct qdma_descq *descq, u32 *data, int cnt)
{
	u64 bus_64;
	u32 v;
	int ring_index;

	if (cnt < QDMA_REG_IND_CTXT_WCNT_5) {
		pr_warn("%s, cmpt context count %d < %d.\n",
			descq->xdev->conf.name, cnt, QDMA_REG_IND_CTXT_WCNT_5);
		return -EINVAL;
	}
	memset(data, 0, cnt * sizeof(u32));

	/* writeback context */
	bus_64 = (PCI_DMA_H(descq->desc_cmpt_bus) << 26) |
		((PCI_DMA_L(descq->desc_cmpt_bus)) >> 6);

	data[0] = (descq->conf.cmpl_stat_en << S_CMPT_CTXT_W0_F_EN_STAT_DESC) |
		  (descq->conf.irq_en << S_CMPT_CTXT_W0_F_EN_INT) |
		  (V_CMPT_CTXT_W0_TRIG_MODE(descq->conf.cmpl_trig_mode)) |
		  (V_CMPT_CTXT_W0_FNC_ID(descq->xdev->func_id)) |
		  (descq->conf.cmpl_timer_idx << S_CMPT_CTXT_W0_TIMER_IDX) |
		  (descq->conf.cmpl_cnt_th_idx << S_CMPT_CTXT_W0_COUNTER_IDX) |
		  (1 << S_CMPT_CTXT_W0_F_COLOR) |
		  (descq->conf.cmpl_rng_sz_idx << S_CMPT_CTXT_W0_RNG_SZ);

	data[1] = bus_64 & 0xFFFFFFFF;

	v = PCI_DMA_H(bus_64) & M_CMPT_CTXT_W2_BADDR_64;
	data[2] = (V_CMPT_CTXT_W2_BADDR_64(v)) |
		  (V_CMPT_CTXT_W2_DESC_SIZE(descq->conf.cmpl_desc_sz));

	data[3] = (1 << S_CMPT_CTXT_W3_F_VALID);

	if ((descq->xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(descq->xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		ring_index = get_intr_ring_index(descq->xdev, descq->intr_id);
		data[4] = (descq->conf.cmpl_ovf_chk_dis <<
				S_CMPT_CTXT_W4_F_OVF_CHK_DIS) |
				V_CMPT_CTXT_W4_VEC(ring_index) |
				(0x01 << S_CMPT_CTXT_W4_F_INTR_AGGR);
	} else {
		data[4] = (descq->conf.cmpl_ovf_chk_dis <<
				S_CMPT_CTXT_W4_F_OVF_CHK_DIS) |
				V_CMPT_CTXT_W4_VEC(descq->intr_id);
	}

	pr_debug("%s, CMPT 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x.\n",
		descq->conf.name, data[4], data[3], data[2], data[1], data[0]);

	return 0;
}

#ifdef __QDMA_VF__
int qdma_intr_context_setup(struct xlnx_dma_dev *xdev)
{
	int i = 0;
	int rv;
	struct mbox_msg *m = NULL;
	struct mbox_msg_hdr *hdr = NULL;
	struct mbox_msg_intr_ctxt *ictxt = NULL;

	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE))
		return 0;

	m = qdma_mbox_msg_alloc(xdev,
					 MBOX_OP_INTR_CTXT);
	if (!m)
		return -ENOMEM;

	hdr =  &m->hdr;
	ictxt = &m->intr_ctxt;

	ictxt->clear = 1;
	ictxt->vec_base = xdev->dvec_start_idx;
	ictxt->num_rings = QDMA_NUM_DATA_VEC_FOR_INTR_CXT;

	for (i = 0; i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
		ictxt->ring_index_list[i] =
			get_intr_ring_index(xdev, xdev->dvec_start_idx + i);
	}

	rv = make_intr_context(xdev, ictxt->w,
			(QDMA_NUM_DATA_VEC_FOR_INTR_CXT *
					QDMA_REG_IND_CTXT_WCNT_3));
	if (rv < 0)
		return rv;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_INTR_CTXT_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		if (rv != -ENODEV)
			pr_err("%s, mbox failed for interrupt context %d.\n",
				xdev->conf.name, rv);
		goto free_msg;
	}

	rv = hdr->status;

free_msg:
	qdma_mbox_msg_free(m);
	if (rv < 0)
		return rv;

	return 0;
}

int qdma_descq_context_clear(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h, bool clr)
{
	struct mbox_msg *m = qdma_mbox_msg_alloc(xdev, MBOX_OP_QCTXT_CLR);
	struct mbox_msg_hdr *hdr = m ? &m->hdr : NULL;
	struct mbox_msg_qctxt *qctxt = m ? &m->qctxt : NULL;
	int rv;

	if (!m)
		return -ENOMEM;

	qctxt->qid = qid_hw;
	qctxt->st = st;
	qctxt->c2h = c2h;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_QCTXT_CLR_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		if (rv != -ENODEV)
			pr_info("%s, qid_hw 0x%x mbox failed %d.\n",
				xdev->conf.name, qid_hw, rv);
		goto err_out;
	}

	rv = hdr->status;

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

int qdma_descq_context_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h,
				struct hw_descq_context *context)
{
	struct mbox_msg *m = qdma_mbox_msg_alloc(xdev, MBOX_OP_QCTXT_RD);
	struct mbox_msg_hdr *hdr = m ? &m->hdr : NULL;
	struct mbox_msg_qctxt *qctxt = m ? &m->qctxt : NULL;
	int rv;

	if (!m)
		return -ENOMEM;

	qctxt->qid = qid_hw;
	qctxt->st = st;
	qctxt->c2h = c2h;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_QCTXT_RD_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		if (rv != -ENODEV)
			pr_info("%s, qid_hw 0x%x mbox failed %d.\n",
				xdev->conf.name, qid_hw, rv);
		goto err_out;
	}

	if (hdr->status) {
		rv = hdr->status;
		goto err_out;
	}

	memcpy(context, &qctxt->context, sizeof(struct hw_descq_context));

	return 0;

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

int qdma_descq_context_setup(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct mbox_msg *m = qdma_mbox_msg_alloc(xdev, MBOX_OP_QCTXT_WRT);
	struct mbox_msg_hdr *hdr = m ? &m->hdr : NULL;
	struct mbox_msg_qctxt *qctxt = m ? &m->qctxt : NULL;
	struct hw_descq_context *context = m ? &qctxt->context : NULL;
	int rv;

	if (!m)
		return -ENOMEM;

	rv = qdma_descq_context_read(xdev, descq->qidx_hw,
				descq->conf.st, descq->conf.c2h,
				context);
	if (rv < 0) {
		pr_info("%s, qid_hw 0x%x, %s mbox failed %d.\n",
			xdev->conf.name, descq->qidx_hw, descq->conf.name, rv);
		goto err_out;
	}

	make_sw_context(descq, context->sw, QDMA_REG_IND_CTXT_WCNT_5);
	if (descq->conf.st && descq->conf.c2h) {
		make_prefetch_context(descq, context->prefetch, 2);
		make_cmpt_context(descq, context->cmpt,
				QDMA_REG_IND_CTXT_WCNT_5);
	}

	qctxt->clear = 1;
	qctxt->verify = 1;
	qctxt->st = descq->conf.st;
	qctxt->c2h = descq->conf.c2h;
	qctxt->qid = descq->qidx_hw;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_QCTXT_WRT_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		if (rv != -ENODEV)
			pr_info("%s, qid_hw 0x%x, %s mbox failed %d.\n",
				xdev->conf.name, descq->qidx_hw,
				descq->conf.name, rv);
		goto err_out;
	}

	if (hdr->status) {
		rv = hdr->status;
		pr_err("Failed to set intr ctxt message\n");
		goto err_out;
	}

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

#else /* PF only */

int qdma_prog_intr_context(struct xlnx_dma_dev *xdev,
		struct mbox_msg_intr_ctxt *ictxt)
{
	int i = 0;
	int rv;
	int ring_index;

	for (i = 0; i < ictxt->num_rings; i++) {
		ring_index = ictxt->ring_index_list[i];

		/* clear the interrupt context for each vector */
		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_CLR,
					QDMA_CTXT_SEL_COAL,
					NULL,
					QDMA_REG_IND_CTXT_WCNT_8,
					0);
		if (rv < 0)
			return rv;

		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_WR,
					QDMA_CTXT_SEL_COAL,
					(ictxt->w +
						(QDMA_REG_IND_CTXT_WCNT_3 * i)),
					QDMA_REG_IND_CTXT_WCNT_3,
					1);
		if (rv < 0)
			return rv;
#if 0
		/** print interrupt context */
		pr_debug("intr_ctxt WR: ring_index(Qid) = %d, data[2] = %x data[1] = %x data[0] = %x\n",
				ring_index,
				*(ictxt->w +
					((QDMA_REG_IND_CTXT_WCNT_3*i) + 2)),
				*(ictxt->w +
					((QDMA_REG_IND_CTXT_WCNT_3*i) + 1)),
				*(ictxt->w + (QDMA_REG_IND_CTXT_WCNT_3*i)));

		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_RD,
					QDMA_CTXT_SEL_COAL,
					intr_ctxt,
					QDMA_REG_IND_CTXT_WCNT_3,
					1);
		if (rv < 0)
			return rv;

		pr_debug("intr_ctxt RD: ring_index(Qid) = %d, data[2] = %x data[1] = %x data[0] = %x\n",
				ring_index,
				intr_ctxt[2],
				intr_ctxt[1],
				intr_ctxt[0]);
#endif
	}

	return 0;
}

int qdma_intr_context_setup(struct xlnx_dma_dev *xdev)
{
	u32 data[(QDMA_NUM_DATA_VEC_FOR_INTR_CXT *
			QDMA_REG_IND_CTXT_WCNT_3)];
	int i = 0;
	int rv;
	int ring_index;

	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE))
		return 0;

	/** Preparing the interrupt context for all the vectors
	 *  each vector's context width is QDMA_REG_IND_CTXT_WCNT_3(3)
	 */
	rv = make_intr_context(xdev, data,
			(QDMA_NUM_DATA_VEC_FOR_INTR_CXT *
					QDMA_REG_IND_CTXT_WCNT_3));
	if (rv < 0)
		return rv;

	for (i = 0; i <  QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
		ring_index = get_intr_ring_index(xdev,
				(i + xdev->dvec_start_idx));
		/* clear the interrupt context for each vector */
		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_CLR,
					QDMA_CTXT_SEL_COAL,
					NULL,
					QDMA_REG_IND_CTXT_WCNT_8,
					0);
		if (rv < 0)
			return rv;

		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_WR,
					QDMA_CTXT_SEL_COAL,
					(data + (QDMA_REG_IND_CTXT_WCNT_3 * i)),
					QDMA_REG_IND_CTXT_WCNT_3,
					1);
		if (rv < 0)
			return rv;
#if 0
		/** print interrupt context */
		pr_debug("intr_ctxt WR: ring_index(Qid) = %d, data[2] = %x data[1] = %x data[0] = %x\n",
				ring_index,
				*(data + ((QDMA_REG_IND_CTXT_WCNT_3*i) + 2)),
				*(data + ((QDMA_REG_IND_CTXT_WCNT_3*i) + 1)),
				*(data + (QDMA_REG_IND_CTXT_WCNT_3*i)));

		rv = hw_indirect_ctext_prog(xdev,
					ring_index,
					QDMA_CTXT_CMD_RD,
					QDMA_CTXT_SEL_COAL,
					intr_ctxt,
					QDMA_REG_IND_CTXT_WCNT_3,
					1);
		if (rv < 0)
			return rv;

		pr_debug("intr_ctxt RD: ring_index(Qid) = %d, data[2] = %x data[1] = %x data[0] = %x\n",
				ring_index,
				intr_ctxt[2],
				intr_ctxt[1],
				intr_ctxt[0]);
#endif
	}

	return 0;
}

int qdma_descq_context_clear(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h, bool clr)
{
	u8 sel;
	int rv = 0;

	sel = c2h ? QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw,
			clr ? QDMA_CTXT_CMD_CLR : QDMA_CTXT_CMD_INV,
			sel, NULL, 0, 0);
	if (rv < 0)
		return rv;

	sel = c2h ? QDMA_CTXT_SEL_HW_C2H : QDMA_CTXT_SEL_HW_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_CLR, sel,
				 NULL, 0, 0);
	if (rv < 0)
		return rv;

	sel = c2h ? QDMA_CTXT_SEL_CR_C2H : QDMA_CTXT_SEL_CR_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_CLR, sel,
				NULL, 0, 0);
	if (rv < 0)
		return rv;

	/* Only clear prefetch and writeback contexts if this queue is ST C2H */
	if (st && c2h) {
		rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_CLR,
					QDMA_CTXT_SEL_PFTCH, NULL, 0, 0);
		if (rv < 0)
			return rv;
		rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_CLR,
					QDMA_CTXT_SEL_CMPT, NULL, 0, 0);
		if (rv < 0)
			return rv;
	}

	return 0;
}

int qdma_descq_context_setup(struct qdma_descq *descq)
{
	struct xlnx_dma_dev *xdev = descq->xdev;
	struct hw_descq_context context;
	int rv;

	rv = qdma_descq_context_clear(xdev, descq->qidx_hw, descq->conf.st,
				descq->conf.c2h, 1);
	if (rv < 0)
		return rv;

	memset(&context, 0, sizeof(context));

	make_sw_context(descq, context.sw, QDMA_REG_IND_CTXT_WCNT_5);

	if (descq->conf.st && descq->conf.c2h) {
		make_prefetch_context(descq,
			context.prefetch, QDMA_REG_IND_CTXT_WCNT_2);
		make_cmpt_context(descq, context.cmpt,
				QDMA_REG_IND_CTXT_WCNT_5);
	}

	return qdma_descq_context_program(descq->xdev, descq->qidx_hw,
				descq->conf.st, descq->conf.c2h, &context);
}

int qdma_descq_stm_setup(struct qdma_descq *descq)
{
	struct stm_descq_context context;

	memset(&context, 0, sizeof(context));
	if (descq->conf.c2h)
		make_stm_c2h_context(descq, context.stm);
	else
		make_stm_h2c_context(descq, context.stm);

	return qdma_descq_stm_program(descq->xdev, descq->qidx_hw,
				      descq->conf.pipe_flow_id,
				      descq->conf.c2h, false,
				      &context);
}

int qdma_descq_stm_clear(struct qdma_descq *descq)
{
	struct stm_descq_context context;

	memset(&context, 0, sizeof(context));
	return qdma_descq_stm_program(descq->xdev, descq->qidx_hw,
				      descq->conf.pipe_flow_id,
				      descq->conf.c2h, true,
				      &context);
}

int qdma_descq_context_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h,
				struct hw_descq_context *context)
{
	u8 sel;
	int rv = 0;

	memset(context, 0, sizeof(struct hw_descq_context));

	sel = c2h ? QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_RD, sel,
						context->sw,
						QDMA_REG_IND_CTXT_WCNT_5, 0);
	if (rv < 0)
		return rv;

	sel = c2h ? QDMA_CTXT_SEL_HW_C2H : QDMA_CTXT_SEL_HW_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_RD, sel,
						context->hw,
						QDMA_REG_IND_CTXT_WCNT_2, 0);
	if (rv < 0)
		return rv;

	sel = c2h ? QDMA_CTXT_SEL_CR_C2H : QDMA_CTXT_SEL_CR_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_RD, sel,
						context->cr,
						QDMA_REG_IND_CTXT_WCNT_1, 0);
	if (rv < 0)
		return rv;

	if (st && c2h) {
		rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_RD,
						QDMA_CTXT_SEL_CMPT,
						context->cmpt,
						QDMA_REG_IND_CTXT_WCNT_5, 0);
		if (rv < 0)
			return rv;
		rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_RD,
						QDMA_CTXT_SEL_PFTCH,
						context->prefetch,
						QDMA_REG_IND_CTXT_WCNT_2, 0);
		if (rv < 0)
			return rv;
	}

	return 0;
}

int qdma_intr_context_read(struct xlnx_dma_dev *xdev,
	int ring_index, unsigned int ctxt_sz, u32 *context)
{
	int rv = 0;

	memset(context, 0, (sizeof(u32) * ctxt_sz));

	rv = hw_indirect_ctext_prog(xdev, ring_index,
				QDMA_CTXT_CMD_RD, QDMA_CTXT_SEL_COAL,
				context, ctxt_sz, 0);
	if (rv < 0)
		return rv;

	return 0;
}

int qdma_descq_context_program(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
				bool st, bool c2h,
				struct hw_descq_context *context)

{
	u8 sel;
	int rv;

	/* always clear first */
	rv = qdma_descq_context_clear(xdev, qid_hw, st, c2h, 1);
	if (rv < 0)
		return rv;

	sel = c2h ?  QDMA_CTXT_SEL_SW_C2H : QDMA_CTXT_SEL_SW_H2C;
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_WR,
					sel,
					context->sw,
					QDMA_REG_IND_CTXT_WCNT_5, 1);
	if (rv < 0)
		return rv;

	/* Only c2h st specific setup done below*/
	if (!st || !c2h)
		return 0;

	/* prefetch context */
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_WR,
					QDMA_CTXT_SEL_PFTCH,
					context->prefetch,
					QDMA_REG_IND_CTXT_WCNT_2, 1);
	if (rv < 0)
		return rv;

	/* writeback context */
	rv = hw_indirect_ctext_prog(xdev, qid_hw, QDMA_CTXT_CMD_WR,
					QDMA_CTXT_SEL_CMPT,
					context->cmpt,
					QDMA_REG_IND_CTXT_WCNT_5, 1);
	if (rv < 0)
		return rv;

	return 0;
}


int qdma_descq_stm_read(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			u8 pipe_flow_id, bool c2h, bool map, bool ctxt,
			struct stm_descq_context *context)
{
	int rv = 0;

	if (!map) {
		rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
					  STM_CSR_CMD_RD,
					  ctxt ? STM_IND_ADDR_Q_CTX_H2C :
					  STM_IND_ADDR_FORCED_CAN,
					  context->stm, 5, false);
	} else {
		rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
					  STM_CSR_CMD_RD,
					  c2h ? STM_IND_ADDR_C2H_MAP :
					  STM_IND_ADDR_H2C_MAP,
					  context->stm, c2h ? 1 : 5,
					  false);
	}
	return rv;
}

int qdma_descq_stm_program(struct xlnx_dma_dev *xdev, unsigned int qid_hw,
			   u8 pipe_flow_id, bool c2h, bool clear,
			   struct stm_descq_context *context)
{
	int rv;

	if (!c2h) {
		/* need to program stm context */
		rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
					  STM_CSR_CMD_WR,
					  STM_IND_ADDR_Q_CTX_H2C,
					  context->stm, 5, clear);
		if (rv < 0)
			return rv;
		rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
					  STM_CSR_CMD_WR,
					  STM_IND_ADDR_H2C_MAP,
					  context->stm, 1, clear);
		if (rv < 0)
			return rv;
	}

	/* Only c2h st specific setup done below*/
	if (!c2h)
		return 0;

	rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
				  STM_CSR_CMD_WR,
				  STM_IND_ADDR_Q_CTX_C2H,
				  context->stm, 2, clear);
	if (rv < 0)
		return rv;

	rv = hw_indirect_stm_prog(xdev, qid_hw, pipe_flow_id,
				  STM_CSR_CMD_WR, STM_IND_ADDR_C2H_MAP,
				  context->stm, 1, clear);
	if (rv < 0)
		return rv;

	return 0;
}
#endif
