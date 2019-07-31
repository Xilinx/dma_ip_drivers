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

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/module.h>
#include <linux/gfp.h>
#include "qdma_device.h"
#include "qdma_context.h"
#include "qdma_descq.h"
#include "qdma_intr.h"
#include "qdma_regs.h"
#include "qdma_mbox.h"
#include "qdma_access.h"

#ifdef __QDMA_VF__
static int device_set_qrange(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct mbox_msg *m;
	int rv = 0;

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return -EINVAL;
	}

	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_fmap_prog(xdev->func_id, qdev->qmax,
				       (int)qdev->qbase,
					m->raw);
	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (unlikely(rv < 0)) {
		if (rv != -ENODEV)
			pr_info("%s set q range (fmap) failed %d.\n",
				xdev->conf.name, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw);
	if (unlikely(rv < 0)) {
		pr_err("mbox_vf_response_status failed with error = %d", rv);
		rv = -EINVAL;
	}

	pr_debug("%s, func id %u/%u, Q 0x%x + 0x%x.\n",
		xdev->conf.name, xdev->func_id, xdev->func_id_parent,
		qdev->qbase, qdev->qmax);

	if (!rv)
		qdev->init_qrange = 1;

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

int device_set_qconf(struct xlnx_dma_dev *xdev, int *qmax, int *qbase)
{
	struct mbox_msg *m;
	int rv = 0;

	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_qreq(xdev->func_id,
				  (uint16_t)*qmax & 0xFFFF, *qbase, m->raw);
	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (unlikely(rv < 0)) {
		pr_info("%s set q range (qconf) failed %d.\n",
			xdev->conf.name, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_qinfo_get(m->raw, qbase, (uint16_t *)qmax);
	if (unlikely(rv < 0)) {
		pr_err("mbox_vf_qinfo_get  failed with error = %d", rv);
		rv = -EINVAL;
	}
err_out:
	qdma_mbox_msg_free(m);
	return rv;
}
#else
static int device_set_qrange(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_fmap_cfg fmap;
	int rv = 0;

	memset(&fmap, 0, sizeof(struct qdma_fmap_cfg));

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return -EINVAL;
	}

	fmap.qmax = qdev->qmax;
	fmap.qbase = qdev->qbase;

	rv = qdma_fmap_write(xdev, xdev->func_id, &fmap);
	if (unlikely(rv < 0)) {
		pr_err("FMAP write failed with err %d\n", rv);
		return qdma_get_error_code(rv);
	}

	qdev->init_qrange = 1;

	pr_debug("%s, func id %u, Q 0x%x + 0x%x.\n",
		xdev->conf.name, xdev->func_id, qdev->qbase, qdev->qmax);

	return rv;
}
#endif /* ifndef __QDMA_VF__ */

int qdma_device_interrupt_setup(struct xlnx_dma_dev *xdev)
{
	int rv = 0;

	if ((xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		rv = intr_ring_setup(xdev);
		if (rv) {
			pr_err("failed to setup interrupt ring");
			return -EINVAL;
		}
		if (xdev->intr_coal_list != NULL) {
			rv = qdma_intr_context_setup(xdev);
			if (rv)
				return rv;
		} else {
			pr_info("dev %s intr vec[%d] >= queues[%d], No aggregation\n",
				dev_name(&xdev->conf.pdev->dev),
				(xdev->num_vecs - xdev->dvec_start_idx),
				xdev->conf.qsets_max);
			pr_warn("Changing the system mode to direct interrupt mode");
			xdev->conf.qdma_drv_mode = DIRECT_INTR_MODE;
		}
	}


#ifndef __QDMA_VF__
	if (((xdev->conf.qdma_drv_mode != POLL_MODE) &&
			(xdev->conf.qdma_drv_mode != LEGACY_INTR_MODE)) &&
			xdev->conf.master_pf) {
		rv = qdma_err_intr_setup(xdev);
		if (unlikely(rv < 0)) {
			pr_err("failed to setup error interrupt");
			return -EINVAL;
		}

		rv = qdma_error_enable(xdev, QDMA_ERRS_ALL);
		if (unlikely(rv < 0)) {
			pr_err("Failed to enable error interrupt with error = %d",
						rv);
			return -EINVAL;
		}

		rv = qdma_error_interrupt_rearm(xdev);
		if (unlikely(rv < 0)) {
			pr_err("Failed to rearm error interrupt with error = %d",
						rv);
			return -EINVAL;
		}
	}
#endif
	return rv;
}

void qdma_device_interrupt_cleanup(struct xlnx_dma_dev *xdev)
{
	if (xdev->intr_coal_list)
		intr_ring_teardown(xdev);
}

int qdma_device_prep_q_resource(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = 0;

	spin_lock(&qdev->lock);

	if (qdev->init_qrange)
		goto done;

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0))
		goto done;

	rv = device_set_qrange(xdev);
	if (unlikely(rv < 0))
		goto done;
done:
	spin_unlock(&qdev->lock);

	return rv;
}

int qdma_device_init(struct xlnx_dma_dev *xdev)
{
	int i;
	struct qdma_fmap_cfg fmap;
#ifndef __QDMA_VF__
	int rv = 0;
#endif
	int qmax = xdev->conf.qsets_max;
	struct qdma_descq *descq;
	struct qdma_dev *qdev;

	memset(&fmap, 0, sizeof(struct qdma_fmap_cfg));

	qdev = kzalloc(sizeof(struct qdma_dev) +
			sizeof(struct qdma_descq) * qmax * 2, GFP_KERNEL);
	if (!qdev) {
		pr_err("dev %s qmax %d OOM.\n",
			dev_name(&xdev->conf.pdev->dev), qmax);
		intr_teardown(xdev);
		return -ENOMEM;
	}

	spin_lock_init(&qdev->lock);
	xdev->dev_priv = (void *)qdev;
#ifdef __QDMA_VF__
	xdev->func_id = xdev->func_id_parent = 0; /* filled later */
#else
	fmap.qmax = 0;
	fmap.qbase = 0;
	rv = qdma_fmap_write(xdev, xdev->func_id, &fmap);
	if (unlikely(rv < 0)) {
		pr_err("FMAP write failed with err %d\n", rv);
		return qdma_get_error_code(rv);
	}
#endif

	descq = (struct qdma_descq *)(qdev + 1);
	qdev->h2c_descq = descq;
	qdev->c2h_descq = descq + qmax;

	qdev->qmax = qmax;
	qdev->init_qrange = 0;

	qdev->qbase = xdev->conf.qsets_base;

	for (i = 0, descq = qdev->h2c_descq; i < qdev->qmax; i++, descq++)
		qdma_descq_init(descq, xdev, i, i);
	for (i = 0, descq = qdev->c2h_descq; i < qdev->qmax; i++, descq++)
		qdma_descq_init(descq, xdev, i, i);
#ifndef __QDMA_VF__
	qdma_set_default_global_csr(xdev);
	for (i = 0; i < xdev->dev_cap.mm_channel_max; i++) {
		qdma_mm_channel_enable(xdev, i, 1);
		qdma_mm_channel_enable(xdev, i, 0);
	}
#endif
	return 0;
}

void qdma_device_cleanup(struct xlnx_dma_dev *xdev)
{
	int i;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_descq *descq;

	if  (!qdev) {
		pr_info("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return;
	}

	for (i = 0, descq = qdev->h2c_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ONLINE)
			qdma_queue_stop((unsigned long int)xdev, i, NULL, 0);
	}

	for (i = 0, descq = qdev->c2h_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ONLINE)
			qdma_queue_stop((unsigned long int)xdev,
					i + qdev->qmax, NULL, 0);
	}

#ifndef __QDMA_VF__
	if (xdev->func_id == 0) {
		for (i = 0; i < xdev->dev_cap.mm_channel_max; i++) {
			qdma_mm_channel_disable(xdev, i, DMA_TO_DEVICE);
			qdma_mm_channel_disable(xdev, i, DMA_FROM_DEVICE);
		}
	}
#endif

	for (i = 0, descq = qdev->h2c_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ENABLED)
			qdma_queue_remove((unsigned long int)xdev, i, NULL, 0);
	}

	for (i = 0, descq = qdev->c2h_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ENABLED)
			qdma_queue_remove((unsigned long int)xdev,
					  i + qdev->qmax, NULL, 0);
	}
	xdev->dev_priv = NULL;
	kfree(qdev);
}

long qdma_device_get_id_from_descq(struct xlnx_dma_dev *xdev,
				   struct qdma_descq *descq)
{
	struct qdma_dev *qdev;
	unsigned long idx;
	unsigned long idx_max;
	struct qdma_descq *_descq;

	if (!xdev) {
		pr_info("xdev NULL.\n");
		return -EINVAL;
	}

	qdev = xdev_2_qdev(xdev);

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return -EINVAL;
	}
	_descq = descq->conf.c2h ? qdev->c2h_descq : qdev->h2c_descq;
	idx = descq->conf.c2h ? qdev->qmax : 0;

	idx_max = (idx + qdev->qmax);
	for ( ; idx < idx_max; idx++, _descq++)
		if (_descq == descq)
			return idx;

	return -EINVAL;

}

struct qdma_descq *qdma_device_get_descq_by_id(struct xlnx_dma_dev *xdev,
			unsigned long idx, char *buf, int buflen, int init)
{
	struct qdma_dev *qdev;
	struct qdma_descq *descq;

	if (!xdev) {
		pr_info("xdev NULL.\n");
		return NULL;
	}

	qdev = xdev_2_qdev(xdev);

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return NULL;
	}

	if (idx >= qdev->qmax) {
		idx -= qdev->qmax;
		if (idx >= qdev->qmax) {
			pr_info("%s, q idx too big 0x%lx > 0x%x.\n",
				xdev->conf.name, idx, qdev->qmax);
			if (buf && buflen)
				snprintf(buf, buflen,
					"%s, q idx too big 0x%lx > 0x%x.\n",
					xdev->conf.name, idx, qdev->qmax);
			return NULL;
		}
		descq = qdev->c2h_descq + idx;
	} else {
		descq = qdev->h2c_descq + idx;
	}

	if (init) {
		lock_descq(descq);
		if (descq->q_state == Q_STATE_DISABLED) {
			pr_info("%s, idx 0x%lx, q 0x%p state invalid.\n",
				xdev->conf.name, idx, descq);
			if (buf && buflen)
				snprintf(buf, buflen,
				"%s, idx 0x%lx, q 0x%p state invalid.\n",
					xdev->conf.name, idx, descq);
			unlock_descq(descq);
			return NULL;
		}
		unlock_descq(descq);
	}

	return descq;
}

#ifdef DEBUGFS
struct qdma_descq *qdma_device_get_pair_descq_by_id(struct xlnx_dma_dev *xdev,
			unsigned long idx, char *buf, int buflen, int init)
{
	struct qdma_dev *qdev;
	struct qdma_descq *pair_descq;

	if (!xdev) {
		pr_info("xdev NULL.\n");
		return NULL;
	}

	qdev = xdev_2_qdev(xdev);

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return NULL;
	}

	if (idx >= qdev->qmax) {
		idx -= qdev->qmax;
		if (idx >= qdev->qmax) {
			pr_debug("%s, q idx too big 0x%lx > 0x%x.\n",
				xdev->conf.name, idx, qdev->qmax);
			if (buf && buflen)
				snprintf(buf, buflen,
					"%s, q idx too big 0x%lx > 0x%x.\n",
					xdev->conf.name, idx, qdev->qmax);
			return NULL;
		}
		pair_descq = qdev->h2c_descq + idx;
	} else {
		pair_descq = qdev->c2h_descq + idx;
	}

	if (init) {
		lock_descq(pair_descq);
		if (pair_descq->q_state == Q_STATE_DISABLED) {
			pr_debug("%s, idx 0x%lx, q 0x%p state invalid.\n",
				xdev->conf.name, idx, pair_descq);
			if (buf && buflen)
				snprintf(buf, buflen,
				"%s, idx 0x%lx, q 0x%p state invalid.\n",
					xdev->conf.name, idx, pair_descq);
			unlock_descq(pair_descq);
			return NULL;
		}
		unlock_descq(pair_descq);
	}

	return pair_descq;
}
#endif

struct qdma_descq *qdma_device_get_descq_by_hw_qid(struct xlnx_dma_dev *xdev,
			unsigned long qidx_hw, u8 c2h)
{
	struct qdma_dev *qdev;
	struct qdma_descq *descq;
	unsigned long qidx_sw = 0;

	if (!xdev) {
		pr_info("xdev NULL.\n");
		return NULL;
	}

	qdev = xdev_2_qdev(xdev);

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return NULL;
	}

	qidx_sw = qidx_hw - qdev->qbase;
	if (c2h)
		descq = &qdev->c2h_descq[qidx_sw];
	else
		descq = &qdev->h2c_descq[qidx_sw];

	return descq;
}
