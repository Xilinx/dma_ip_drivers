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
#include "qdma_descq.h"
#include "qdma_device.h"
#include "qdma_regs.h"
#include "thread.h"
#include "version.h"
#include "qdma_mbox_protocol.h"
#include "qdma_intr.h"
#ifdef DUMP_ON_ERROR_INTERRUPT
#include "qdma_reg_dump.h"
#endif
#include "qdma_access_common.h"

#ifndef __QDMA_VF__
static LIST_HEAD(legacy_intr_q_list);
static spinlock_t legacy_intr_lock;
static spinlock_t legacy_q_add_lock;
static unsigned long legacy_intr_flags = IRQF_SHARED;
#endif


#ifndef __QDMA_VF__
#ifdef DUMP_ON_ERROR_INTERRUPT
#define REG_BANNER_LEN (81 * 5)
static int dump_qdma_regs(struct xlnx_dma_dev *xdev)
{
	int len = 0, dis_len = 0;
	int rv;
	char *buf = NULL, *tbuff = NULL;
	int buflen;
	char temp_buf[512];

	if (!xdev) {
		pr_err("Invalid device\n");
		return -EINVAL;
	}

	rv = qdma_reg_dump_buf_len((void *)xdev,
			xdev->version_info.ip_type, &buflen);
	if (rv < 0) {
		pr_err("Failed to get reg dump buffer length\n");
		return rv;
	}
	buflen += REG_BANNER_LEN;

	/** allocate memory */
	tbuff = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!tbuff)
		return -ENOMEM;

	buf = tbuff;
	rv = qdma_dump_config_regs(xdev, buf + len, buflen - len);
	if (rv < 0) {
		pr_warn("Failed to dump Config Bar register values\n");
		goto free_buf;
	}
	len += rv;

	*data = buf;
	*data_len = buflen;

	buf[++len] = '\0';
	memset(temp_buf, '\0', 512);
	for (dis_len = 0; dis_len < len; dis_len += 512) {
		memcpy(temp_buf, buf, 512);
		pr_info("\n%s", temp_buf);
		memset(temp_buf, '\0', 512);
		buf += 512;
	}

free_buf:
	kfree(tbuf);
	return 0;
}
#endif
#endif

#ifndef MBOX_INTERRUPT_DISABLE
static irqreturn_t mbox_intr_handler(int irq_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;
	struct qdma_mbox *mbox = &xdev->mbox;

	pr_debug("Mailbox IRQ fired on Funtion#%d: index=%d, vector=%d\n",
		xdev->func_id, irq_index, irq);

	queue_work(mbox->workq, &mbox->rx_work);

	return IRQ_HANDLED;
}
#endif

#ifndef USER_INTERRUPT_DISABLE
static irqreturn_t user_intr_handler(int irq_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	pr_info("User IRQ fired on Funtion#%d: index=%d, vector=%d\n",
		xdev->func_id, irq_index, irq);

	if (xdev->conf.fp_user_isr_handler)
		xdev->conf.fp_user_isr_handler((unsigned long)xdev,
						xdev->conf.uld);

	return IRQ_HANDLED;
}
#endif

#ifndef __QDMA_VF__
static irqreturn_t error_intr_handler(int irq_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	pr_info("Error IRQ fired on Funtion#%d: index=%d, vector=%d\n",
			xdev->func_id, irq_index, irq);

	xdev->hw.qdma_hw_error_process(xdev);

	xdev->hw.qdma_hw_error_intr_rearm(xdev);
#ifdef DUMP_ON_ERROR_INTERRUPT
	dump_qdma_regs(xdev);
#endif
	return IRQ_HANDLED;
}
#endif

static void data_intr_aggregate(struct xlnx_dma_dev *xdev, int vidx, int irq,
		u64 timestamp)
{
	struct qdma_descq *descq = NULL;
	u32 counter = 0;
	struct intr_coal_conf *coal_entry =
			(xdev->intr_coal_list + vidx - xdev->dvec_start_idx);
	union qdma_intr_ring *ring_entry;
	struct qdma_intr_cidx_reg_info *intr_cidx_info;
	uint8_t color = 0;
	uint8_t intr_type = 0;
	uint32_t qid = 0;
	uint32_t num_entries_processed = 0;


	if (!coal_entry) {
		pr_err("Failed to locate the coalescing entry for vector = %d\n",
			vidx);
		return;
	}
	intr_cidx_info = &coal_entry->intr_cidx_info;
	pr_debug("INTR_COAL: msix[%d].vector=%d, msix[%d].entry=%d, rngsize=%d, cidx = %d\n",
		vidx, xdev->msix[vidx].vector,
		vidx,
		xdev->msix[vidx].entry,
		coal_entry->intr_rng_num_entries,
		intr_cidx_info->sw_cidx);

	pr_debug("vidx = %d, dvec_start_idx = %d\n", vidx,
		 xdev->dvec_start_idx);

	if ((xdev->msix[vidx].entry) !=  coal_entry->vec_id) {
		pr_err("msix[%d].entry[%d] != vec_id[%d]\n",
			vidx, xdev->msix[vidx].entry,
			coal_entry->vec_id);

		return;
	}

	counter = intr_cidx_info->sw_cidx;
	ring_entry = (coal_entry->intr_ring_base + counter);
	if (!ring_entry) {
		pr_err("Failed to locate the ring entry for vector = %d\n",
			vidx);
		return;
	}

	do {
		if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP) {
			color = ring_entry->ring_cpm.coal_color;
			intr_type = ring_entry->ring_cpm.intr_type;
			qid = ring_entry->ring_cpm.qid;
		} else {
			color = ring_entry->ring_generic.coal_color;
			intr_type = ring_entry->ring_generic.intr_type;
			qid = ring_entry->ring_generic.qid;
		}

		if (color != coal_entry->color)
			break;
		pr_debug("IRQ[%d]: IVE[%d], Qid = %d, e_color = %d, c_color = %d, intr_type = %d\n",
				irq, vidx, qid, coal_entry->color,
				color, intr_type);

		descq = qdma_device_get_descq_by_hw_qid(xdev, qid,
				intr_type);
		if (!descq) {
			pr_err("IRQ[%d]: IVE[%d], Qid = %d: desc not found\n",
					irq, vidx, qid);
			return;
		}
		xdev->prev_descq = descq;
		pr_debug("IRQ[%d]: IVE[%d], Qid = %d, e_color = %d, c_color = %d, intr_type = %d\n",
				irq, vidx, qid, coal_entry->color,
				color, intr_type);

		if (descq->conf.ping_pong_en &&
			descq->conf.q_type == Q_C2H && descq->conf.st)
			descq->ping_pong_rx_time = timestamp;

		if (descq->conf.fp_descq_isr_top) {
			descq->conf.fp_descq_isr_top(descq->q_hndl,
					descq->conf.quld);
		} else {
			if (descq->cpu_assigned)
				schedule_work_on(descq->intr_work_cpu,
						&descq->work);
			else
				schedule_work(&descq->work);
		}

		if (++intr_cidx_info->sw_cidx ==
				coal_entry->intr_rng_num_entries) {
			counter = 0;
			xdev->intr_coal_list->color =
				(xdev->intr_coal_list->color) ? 0 : 1;
			intr_cidx_info->sw_cidx = 0;
		} else
			counter++;
		num_entries_processed++;
		ring_entry = (coal_entry->intr_ring_base + counter);
	} while (1);

	if (descq) {
		queue_intr_cidx_update(descq->xdev,
			descq->conf.qidx, &coal_entry->intr_cidx_info);
	} else if (num_entries_processed == 0) {
		pr_warn("No entries processed\n");
		descq = xdev->prev_descq;
		if (descq) {
			pr_warn("Doing stale update\n");
			queue_intr_cidx_update(descq->xdev,
				descq->conf.qidx, &coal_entry->intr_cidx_info);
		}
	}

}

static void data_intr_direct(struct xlnx_dma_dev *xdev, int vidx, int irq,
			u64 timestamp)
{
	struct qdma_descq *descq;
	unsigned long flags;
	struct list_head *descq_list =
			&xdev->dev_intr_info_list[vidx].intr_list;
	struct list_head *entry, *tmp;

	spin_lock_irqsave(&xdev->dev_intr_info_list[vidx].vec_q_list,
			  flags);
	list_for_each_safe(entry, tmp, descq_list) {
		descq = container_of(entry, struct qdma_descq, intr_list);
		if (!descq)
			continue;

		if (descq->conf.ping_pong_en &&
				descq->conf.q_type == Q_C2H && descq->conf.st)
			descq->ping_pong_rx_time = timestamp;

		if (descq->conf.fp_descq_isr_top) {
			descq->conf.fp_descq_isr_top(descq->q_hndl,
					descq->conf.quld);
		} else {
			if (descq->cpu_assigned)
				schedule_work_on(descq->intr_work_cpu,
						&descq->work);
			else
				schedule_work(&descq->work);
		}
	}
	spin_unlock_irqrestore(&xdev->dev_intr_info_list[vidx].vec_q_list,
			    flags);
}

static irqreturn_t data_intr_handler(int vector_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;
	u64 timestamp;

	pr_debug("%s: Data IRQ fired on Funtion#%05x: index=%d, vector=%d\n",
		xdev->mod_name, xdev->func_id, vector_index, irq);
	timestamp = rdtsc_gettime();

	if ((xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(xdev->conf.qdma_drv_mode == AUTO_MODE))
		data_intr_aggregate(xdev, vector_index, irq, timestamp);
	else
		data_intr_direct(xdev, vector_index, irq, timestamp);

	return IRQ_HANDLED;
}

static inline void intr_ring_free(struct xlnx_dma_dev *xdev, int ring_sz,
			int intr_desc_sz, u8 *intr_desc, dma_addr_t desc_bus)
{
	unsigned int len = ring_sz * intr_desc_sz;

	pr_debug("free %u(0x%x)=%d*%u, 0x%p, bus 0x%llx.\n",
		len, len, intr_desc_sz, ring_sz, intr_desc, desc_bus);

	dma_free_coherent(&xdev->conf.pdev->dev, (size_t)ring_sz * intr_desc_sz,
			intr_desc, desc_bus);
}

static void *intr_ring_alloc(struct xlnx_dma_dev *xdev, int ring_sz,
				int intr_desc_sz, dma_addr_t *bus)
{
	unsigned int len = ring_sz * intr_desc_sz;
	u8 *p = dma_alloc_coherent(&xdev->conf.pdev->dev, len, bus, GFP_KERNEL);

	if (!p) {
		pr_err("%s, OOM, sz ring %d, intr_desc %d.\n",
			xdev->conf.name, ring_sz, intr_desc_sz);
		return NULL;
	}

	memset(p, 0, len);

	pr_debug("alloc %u(0x%x)=%d*%u, bus 0x%llx .\n",
		len, len, intr_desc_sz, ring_sz, *bus);

	return p;
}

#ifdef __QDMA_VF__
static void intr_context_invalidate(struct xlnx_dma_dev *xdev)
{
	int i = 0;
	struct mbox_msg *m;
	int rv = 0;
	struct mbox_msg_intr_ctxt ictxt;
	struct intr_coal_conf  *ring_entry;

	m = qdma_mbox_msg_alloc();
	if (!m)
		return;
	memset(&ictxt, 0, sizeof(struct mbox_msg_intr_ctxt));
	ictxt.num_rings = QDMA_NUM_DATA_VEC_FOR_INTR_CXT;

	for (i = 0; i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
		ictxt.ring_index_list[i] =
			get_intr_ring_index(xdev, xdev->dvec_start_idx + i);
	}
	qdma_mbox_compose_vf_intr_ctxt_invalidate(xdev->func_id,
			&ictxt, m->raw);
	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		pr_err("%s invalidate interrupt context failed %d.\n",
			xdev->conf.name, rv);
	}

	qdma_mbox_msg_free(m);

	for (i = 0; i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
		ring_entry = (xdev->intr_coal_list + i);
		if (ring_entry) {
			intr_ring_free(xdev,
				ring_entry->intr_rng_num_entries,
				sizeof(union qdma_intr_ring),
				(u8 *)ring_entry->intr_ring_base,
				ring_entry->intr_ring_bus);
		}
	}

}
#else
static void intr_context_invalidate(struct xlnx_dma_dev *xdev)
{
	int i = 0;
	unsigned int ring_index = 0;
	struct intr_coal_conf  *ring_entry;
	int rv = 0;

	while (i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT) {
		ring_index = get_intr_ring_index(xdev,
				(i + xdev->dvec_start_idx));
		rv = xdev->hw.qdma_indirect_intr_ctx_conf(xdev, ring_index,
				NULL, QDMA_HW_ACCESS_INVALIDATE);
		if (rv < 0) {
			pr_err("Intr ctxt invalidate failed, err = %d",
						rv);
			return;
		}
		ring_entry = (xdev->intr_coal_list + i);
		if (ring_entry) {
			intr_ring_free(xdev,
				ring_entry->intr_rng_num_entries,
				sizeof(union qdma_intr_ring),
				(u8 *)ring_entry->intr_ring_base,
				ring_entry->intr_ring_bus);
		}
		i++;
	}

}
#endif

void intr_ring_teardown(struct xlnx_dma_dev *xdev)
{
	intr_context_invalidate(xdev);
	kfree(xdev->intr_coal_list);
}

static void data_vector_handler(int irq, struct xlnx_dma_dev *xdev)
{
	int i;

	for (i = 0; i < xdev->num_vecs; i++) {
		if (xdev->msix[i].vector == irq) {
			xdev->dev_intr_info_list[i].intr_vec_map.intr_handler(i,
					irq, (void *)xdev);
			break;
		}
	}
}

static irqreturn_t irq_bottom(int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	data_vector_handler(irq, xdev);

	return IRQ_HANDLED;
}

static irqreturn_t irq_top(int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	if (xdev->conf.fp_q_isr_top_dev) {
		xdev->conf.fp_q_isr_top_dev((unsigned long)xdev,
					xdev->conf.uld);
	}

	return IRQ_WAKE_THREAD;
}


void intr_teardown(struct xlnx_dma_dev *xdev)
{
	int i = xdev->num_vecs;

	while (--i >= 0)
		free_irq(xdev->msix[i].vector, xdev);

	if (xdev->num_vecs)
		pci_disable_msix(xdev->conf.pdev);

	kfree(xdev->msix);
	kfree(xdev->dev_intr_info_list);
}


static int intr_vector_setup(struct xlnx_dma_dev *xdev, int idx,
			enum intr_type_list type, f_intr_handler handler)
{
	int rv;

	if (type == INTR_TYPE_ERROR)
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-error",
			 xdev->conf.name);

	if (type == INTR_TYPE_USER)
#ifndef USER_INTERRUPT_DISABLE
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-user", xdev->conf.name);
#else
		return -EINVAL;
#endif
	if (type == INTR_TYPE_DATA)
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-data", xdev->conf.name);
	if (type == INTR_TYPE_MBOX)
#ifndef MBOX_INTERRUPT_DISABLE
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-mbox", xdev->conf.name);
#else
		return -EINVAL;
#endif

	xdev->dev_intr_info_list[idx].intr_vec_map.intr_type = type;
	xdev->dev_intr_info_list[idx].intr_vec_map.intr_vec_index = idx;
	xdev->dev_intr_info_list[idx].intr_vec_map.intr_handler = handler;

	if ((type == INTR_TYPE_DATA) || (type == INTR_TYPE_MBOX)) {
		rv = request_irq(xdev->msix[idx].vector, irq_bottom, 0,
				 xdev->dev_intr_info_list[idx].msix_name, xdev);
	} else
		rv = request_threaded_irq(xdev->msix[idx].vector, irq_top,
					  irq_bottom, 0,
				  xdev->dev_intr_info_list[idx].msix_name,
				  xdev);

	pr_debug("%s requesting IRQ vector #%d: vec %d, type %d, %s.\n",
			xdev->conf.name, idx, xdev->msix[idx].vector,
			type, xdev->dev_intr_info_list[idx].msix_name);

	if (rv) {
		pr_err("%s requesting IRQ vector #%d: vec %d failed %d.\n",
			xdev->conf.name, idx, xdev->msix[idx].vector, rv);
		return rv;
	}

	return 0;
}
#ifdef __PCI_MSI_VEC_COUNT__

#define msix_table_size(flags)	((flags & PCI_MSIX_FLAGS_QSIZE) + 1)

static int pci_msix_vec_count(struct pci_dev *dev)
{
	u16 control;

	if (!dev->msix_cap)
		return 0;

	pci_read_config_word(dev, dev->msix_cap + PCI_MSIX_FLAGS, &control);
	return msix_table_size(control);
}
#endif

int intr_setup(struct xlnx_dma_dev *xdev)
{
	int rv = 0;
	int i = 0;
	int num_vecs = 0;
	int num_vecs_req = 0;
#ifndef USER_INTERRUPT_DISABLE
	int intr_count = 0;
#endif

	if ((xdev->conf.qdma_drv_mode == POLL_MODE) ||
			(xdev->conf.qdma_drv_mode == LEGACY_INTR_MODE)) {
		goto exit;
	}
	num_vecs = pci_msix_vec_count(xdev->conf.pdev);
	pr_debug("dev %s, xdev->num_vecs = %d\n",
			dev_name(&xdev->conf.pdev->dev), xdev->num_vecs);

	if (num_vecs == 0) {
		pr_warn("MSI-X not supported, running in polled mode\n");
		return 0;
	}

	if (xdev->conf.data_msix_qvec_max == 0) {
		pr_err("At least 1 data vector is required. input invalid: data_masix_qvec_max(%u)",
			xdev->conf.data_msix_qvec_max);
		return -EINVAL;
	}

	xdev->num_vecs = min_t(int, num_vecs, xdev->conf.msix_qvec_max);
	if (xdev->num_vecs < xdev->conf.msix_qvec_max)
		pr_info("current device supports only (%u) msix vectors per function. ignoring input for (%u) vectors",
			xdev->num_vecs,
			xdev->conf.msix_qvec_max);

	/** Make sure the total supported vectors =
	 *  (user vectors + data vectors + 1 error vectors
	 *  + 1 mailbox vectors)
	 *  find the requested vectors and check the available vectors
	 *  can satisfy the request
	 */
	num_vecs_req = xdev->conf.user_msix_qvec_max +
					xdev->conf.data_msix_qvec_max;

	/** Dedicate 1 vector for error interrupts */
	if (xdev->conf.master_pf)
		num_vecs_req++;

#ifndef MBOX_INTERRUPT_DISABLE
	/** Dedicate 1 vector for mailbox interrupts */
	if ((xdev->version_info.device_type == QDMA_DEVICE_SOFT) &&
	(xdev->version_info.vivado_release >= QDMA_VIVADO_2019_1))
		num_vecs_req++;
#endif

	if (num_vecs_req > xdev->num_vecs) {
		pr_warn("Available vectors(%u) is less than Requested vectors(%u) [u:%u|d:%u]\n",
				xdev->num_vecs,
				num_vecs_req,
				xdev->conf.user_msix_qvec_max,
				xdev->conf.data_msix_qvec_max);
		return -EINVAL;
	}

	xdev->msix = kzalloc((sizeof(struct msix_entry) * xdev->num_vecs),
						GFP_KERNEL);
	if (!xdev->msix) {
		pr_err("dev %s xdev->msix OOM.\n",
			dev_name(&xdev->conf.pdev->dev));
		rv = -ENOMEM;
		goto exit;
	}

	xdev->dev_intr_info_list =
			kzalloc((sizeof(struct intr_info_t) * xdev->num_vecs),
					GFP_KERNEL);
	if (!xdev->dev_intr_info_list) {
		pr_err("dev %s xdev->dev_intr_info_list OOM.\n",
			dev_name(&xdev->conf.pdev->dev));
		rv = -ENOMEM;
		goto free_msix;
	}

	for (i = 0; i < xdev->num_vecs; i++) {
		xdev->msix[i].entry = i;
		INIT_LIST_HEAD(&xdev->dev_intr_info_list[i].intr_list);
		spin_lock_init(&xdev->dev_intr_info_list[i].vec_q_list);
	}

#if KERNEL_VERSION(4, 12, 0) <= LINUX_VERSION_CODE
	rv = pci_enable_msix_exact(xdev->conf.pdev, xdev->msix, xdev->num_vecs);
#else
	rv = pci_enable_msix(xdev->conf.pdev, xdev->msix, xdev->num_vecs);
#endif
	if (rv < 0) {
		pr_err("Error enabling MSI-X (%d)\n", rv);
		goto free_intr_info;
	}

	/** On master PF0, vector#2 is dedicated for Error interrupts and
	 * vector #1 is dedicated for User interrupts
	 * For all other PFs and VFs, vector#0 is dedicated for User interrupts
	 * The remaining vectors are for Data interrupts
	 */
	i = 0; /* This is mandatory, do not delete */

#ifndef MBOX_INTERRUPT_DISABLE
	if ((xdev->version_info.device_type == QDMA_DEVICE_SOFT) &&
	(xdev->version_info.vivado_release >= QDMA_VIVADO_2019_1)) {
		/* Mail box interrupt */
		rv = intr_vector_setup(xdev, i, INTR_TYPE_MBOX,
				mbox_intr_handler);
		if (rv)
			goto cleanup_irq;
		i++;
	}
#endif

#ifndef USER_INTERRUPT_DISABLE
	for (intr_count = 0;
		intr_count < xdev->conf.user_msix_qvec_max;
		intr_count++) {
		/* user interrupt */
		rv = intr_vector_setup(xdev, i, INTR_TYPE_USER,
				user_intr_handler);
		if (rv)
			goto cleanup_irq;
		i++;
	}
#endif

#ifndef __QDMA_VF__
	/* global error interrupt */
	if (xdev->conf.master_pf) {
		rv = intr_vector_setup(xdev, i, INTR_TYPE_ERROR,
				error_intr_handler);
		if (rv)
			goto cleanup_irq;
		i++;
	}
#endif

	/* data interrupt */
	xdev->dvec_start_idx = i;
	for (; i < xdev->num_vecs; i++) {
		rv = intr_vector_setup(xdev, i, INTR_TYPE_DATA,
					data_intr_handler);
		if (rv)
			goto cleanup_irq;
	}

	xdev->flags |= XDEV_FLAG_IRQ;
	return rv;

cleanup_irq:
	while (--i >= 0)
		free_irq(xdev->msix[i].vector, xdev);

	pci_disable_msix(xdev->conf.pdev);
	xdev->num_vecs = 0;
free_intr_info:
	kfree(xdev->dev_intr_info_list);
free_msix:
	kfree(xdev->msix);
exit:
	return rv;
}



#ifndef __QDMA_VF__
static irqreturn_t irq_legacy(int irq, void *irq_data)
{
	struct list_head *entry, *tmp;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)irq_data;
	irqreturn_t ret = IRQ_NONE;

	if (!xdev) {
		pr_err("Invalid Xdev");
		goto irq_return;
	}

	spin_lock_irqsave(&legacy_intr_lock, legacy_intr_flags);
	if (!xdev->hw.qdma_is_legacy_intr_pend(xdev)) {

		list_for_each_safe(entry, tmp, &legacy_intr_q_list) {
			struct qdma_descq *descq =
					container_of(entry,
						     struct qdma_descq,
						     legacy_intr_q_list);

			qdma_descq_service_cmpl_update(descq, 0, 1);
		}
		xdev->hw.qdma_clear_pend_legacy_intr(xdev);
		xdev->hw.qdma_legacy_intr_conf(xdev, ENABLE);
		ret = IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&legacy_intr_lock, legacy_intr_flags);

irq_return:
	return ret;
}

void intr_legacy_clear(struct qdma_descq *descq)
{

	if (!descq) {
		pr_err("Invalid descq received");
		return;
	}
	list_del(&descq->legacy_intr_q_list);

	if (list_empty(&legacy_intr_q_list)) {

		pr_info("un-registering legacy interrupt from qdma%05x\n",
			descq->xdev->conf.bdf);

		descq->xdev->hw.qdma_legacy_intr_conf(descq->xdev, DISABLE);

		free_irq(descq->xdev->conf.pdev->irq, descq->xdev);
	}
}

int intr_legacy_setup(struct qdma_descq *descq)
{
	int req_irq = 0;
	int rv = 0;

	if (!descq) {
		pr_err("Invalid descq received");
		return -EINVAL;
	}

	spin_lock(&legacy_q_add_lock);
	req_irq = list_empty(&legacy_intr_q_list);
	rv = req_irq ? 0 : 1;

	if (req_irq != 0) {
		spin_lock_init(&legacy_intr_lock);
		pr_debug("registering legacy interrupt for irq-%d from qdma%05x\n",
			descq->xdev->conf.pdev->irq, descq->xdev->conf.bdf);

		if (descq->xdev->hw.qdma_legacy_intr_conf(descq->xdev,
								DISABLE)) {
			spin_unlock(&legacy_q_add_lock);
			return -EINVAL;
		}

		rv = request_threaded_irq(descq->xdev->conf.pdev->irq, irq_top,
					  irq_legacy, legacy_intr_flags,
					  "qdma legacy intr",
					  descq->xdev);

		if (rv < 0)
			goto exit_intr_setup;
		else {
			list_add_tail(&descq->legacy_intr_q_list,
				      &legacy_intr_q_list);
			rv = 0;
		}
		if (descq->xdev->hw.qdma_legacy_intr_conf(descq->xdev,
								ENABLE)) {
			spin_unlock(&legacy_q_add_lock);
			return -EINVAL;
		}
	} else
		list_add_tail(&descq->legacy_intr_q_list,
			      &legacy_intr_q_list);

exit_intr_setup:
	spin_unlock(&legacy_q_add_lock);
	return rv;
}
#endif

int intr_ring_setup(struct xlnx_dma_dev *xdev)
{
	int num_entries = 0;
	int counter = 0;
	struct intr_coal_conf  *intr_coal_list;
	struct intr_coal_conf  *intr_coal_list_entry;

	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		pr_debug("skipping interrupt aggregation: driver is loaded in %s mode\n",
			mode_name_list[xdev->conf.qdma_drv_mode].name);
		xdev->intr_coal_list = NULL;
		return 0;
	}

	/** For master_pf, vec1 and vec2 is used for
	 *  error and user interrupts
	 *  for other pfs, vec0 is used for user interrupts
	 */
	if (xdev->num_vecs != 0) {
		pr_debug("dev %s num_vectors[%d] < num_queues [%d]\n",
					dev_name(&xdev->conf.pdev->dev),
					xdev->num_vecs,
					xdev->conf.qsets_max);
		pr_debug("Enabling Interrupt aggregation\n");

		/** obtain the number of queue entries
		 * in each inr_ring based on ring size
		 */
		num_entries = ((xdev->conf.intr_rngsz + 1) * 512);

		pr_debug("%s interrupt coalescing ring with %d entries\n",
			dev_name(&xdev->conf.pdev->dev), num_entries);
		/**
		 * Initially assuming that each vector has the same size of the
		 * ring, In practical it is possible to have different ring
		 * size of different vectors (?)
		 */
		intr_coal_list = kzalloc(
				sizeof(struct intr_coal_conf) *
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT,
				GFP_KERNEL);
		if (!intr_coal_list) {
			pr_err("dev %s num_vecs %d OOM.\n",
				dev_name(&xdev->conf.pdev->dev),
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT);
			return -ENOMEM;
		}

		for (counter = 0;
			counter < QDMA_NUM_DATA_VEC_FOR_INTR_CXT;
			counter++) {
			intr_coal_list_entry = (intr_coal_list + counter);
			intr_coal_list_entry->intr_rng_num_entries =
							num_entries;
			intr_coal_list_entry->intr_ring_base = intr_ring_alloc(
					xdev, num_entries,
					sizeof(union qdma_intr_ring),
					&intr_coal_list_entry->intr_ring_bus);
			if (!intr_coal_list_entry->intr_ring_base) {
				pr_err("dev %s, sz %u, intr_desc ring OOM.\n",
				xdev->conf.name,
				intr_coal_list_entry->intr_rng_num_entries);
				goto err_out;
			}

			intr_coal_list_entry->vec_id =
			xdev->msix[counter + xdev->dvec_start_idx].entry;
			intr_coal_list_entry->intr_cidx_info.sw_cidx = 0;
			intr_coal_list_entry->color = 1;
			intr_coal_list_entry->intr_cidx_info.rng_idx =
					get_intr_ring_index(xdev,
					    intr_coal_list_entry->vec_id);
			pr_debug("ring_number = %d, vector_index = %d, ring_size = %d, ring_base = 0x%08x",
			    counter, intr_coal_list_entry->vec_id,
			    intr_coal_list_entry->intr_rng_num_entries,
			    (unsigned int)intr_coal_list_entry->intr_ring_bus);
		}

		pr_debug("dev %s interrupt coalescing ring setup successful\n",
					dev_name(&xdev->conf.pdev->dev));

		xdev->intr_coal_list = intr_coal_list;
	} else {
		pr_info("dev %s intr vec[%d] >= queues[%d], No aggregation\n",
			dev_name(&xdev->conf.pdev->dev),
			(xdev->num_vecs - xdev->dvec_start_idx),
			xdev->conf.qsets_max);

		xdev->intr_coal_list = NULL;
		/* Fallback from indirect interrupt mode */
		xdev->conf.qdma_drv_mode = POLL_MODE;
	}
	return 0;

err_out:
	while (--counter >= 0) {
		intr_coal_list_entry = (intr_coal_list + counter);
		intr_ring_free(xdev, intr_coal_list_entry->intr_rng_num_entries,
				sizeof(union qdma_intr_ring),
				(u8 *)intr_coal_list_entry->intr_ring_base,
				intr_coal_list_entry->intr_ring_bus);
	}
	kfree(intr_coal_list);
	return -ENOMEM;
}

void intr_work(struct work_struct *work)
{
	struct qdma_descq *descq;

	descq = container_of(work, struct qdma_descq, work);
	qdma_descq_service_cmpl_update(descq, 0, 1);
}

/**
 * qdma_queue_service - service the queue
 * in the case of irq handler is registered by the user, the user should
 * call qdma_queue_service() in its interrupt handler to service the queue
 * @dev_hndl: hndl retured from qdma_device_open()
 * @qhndl: hndl retured from qdma_queue_add()
 */
int qdma_queue_service(unsigned long dev_hndl, unsigned long id, int budget,
			bool c2h_upd_cmpl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq;

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
	if (descq)
		return qdma_descq_service_cmpl_update(descq,
					budget, c2h_upd_cmpl);

	return -EINVAL;
}

static u8 get_intr_vec_index(struct xlnx_dma_dev *xdev, u8 intr_type)
{
	int i = 0;

	for (i = 0; i < xdev->num_vecs; i++) {
		if (xdev->dev_intr_info_list[i].intr_vec_map.intr_type ==
		    intr_type) {
			struct intr_info_t *dev_intr_info_list =
					&xdev->dev_intr_info_list[i];
			return dev_intr_info_list->intr_vec_map.intr_vec_index;
		}
	}
	return 0;
}

int qdma_err_intr_setup(struct xlnx_dma_dev *xdev)
{
	int rv = 0;
	u8  err_intr_index = 0;

	err_intr_index = get_intr_vec_index(xdev, INTR_TYPE_ERROR);

	rv = xdev->hw.qdma_hw_error_intr_setup(xdev, xdev->func_id,
					    err_intr_index);
	if (rv < 0) {
		pr_err("Failed to setup error interrupt, err = %d", rv);
		return -EINVAL;
	}

	return 0;
}

int get_intr_ring_index(struct xlnx_dma_dev *xdev, u32 vector_index)
{
	int ring_index = 0;

	ring_index = (vector_index - xdev->dvec_start_idx) +
			(xdev->func_id * QDMA_NUM_DATA_VEC_FOR_INTR_CXT);
	pr_debug("func_id = %d, vector_index = %d, ring_index = %d\n",
			xdev->func_id, vector_index, ring_index);

	return ring_index;
}

void intr_legacy_init(void)
{
#ifndef __QDMA_VF__
	spin_lock_init(&legacy_q_add_lock);
#endif
}
