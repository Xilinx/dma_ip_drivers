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

#include <linux/module.h>
#include <linux/gfp.h>
#include "qdma_device.h"
#include "qdma_context.h"
#include "qdma_descq.h"
#include "qdma_intr.h"
#include "qdma_regs.h"
#include "qdma_mbox.h"

#ifdef __QDMA_VF__
static int device_set_qrange(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct mbox_msg *m;
	struct mbox_msg_hdr *hdr;
	struct mbox_msg_fmap *fmap;
	int rv = 0;

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	if (xdev->conf.cur_cfg_state == QMAX_CFG_INITIAL) {
		qdev->qmax = 1;
		qdev->qbase = 0;
	}

	m = qdma_mbox_msg_alloc(xdev, MBOX_OP_FMAP);
	if (!m)
		return -ENOMEM;

	hdr = &m->hdr;
	fmap = &m->fmap;

	fmap->qbase = qdev->qbase;
	fmap->qmax = qdev->qmax;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_FMAP_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		if (rv != -ENODEV)
			pr_info("%s set q range (fmap) failed %d.\n",
				xdev->conf.name, rv);
		goto err_out;
	}

	if (hdr->status) {
		rv = hdr->status;
		if (xdev->conf.cur_cfg_state == QMAX_CFG_INITIAL) {
			qdev->qbase = xdev->conf.qsets_base = fmap->qbase;
			qdev->qmax = xdev->conf.qsets_max = 0;
		}
		pr_err("Failed to set qconf %d max/base = %u/%u\n", rv,
				qdev->qmax, qdev->qbase);
		goto err_out;
	}

	qdev->qbase = xdev->conf.qsets_base = fmap->qbase;

	if (xdev->conf.cur_cfg_state == QMAX_CFG_INITIAL) {
		qdev->h2c_descq->qidx_hw = fmap->qbase;
		qdev->c2h_descq->qidx_hw = fmap->qbase;
		xdev->conf.cur_cfg_state = QMAX_CFG_USER;
	}

	pr_debug("%s, func id %u/%u, Q 0x%x + 0x%x.\n",
		xdev->conf.name, xdev->func_id, xdev->func_id_parent,
		qdev->qbase, qdev->qmax);

	qdev->init_qrange = 1;

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

int device_set_qconf(struct xlnx_dma_dev *xdev, int qmax, u32 *qbase)
{
	struct mbox_msg *m;
	struct mbox_msg_hdr *hdr;
	struct mbox_msg_fmap *fmap;
	int rv = 0;

	m = qdma_mbox_msg_alloc(xdev, MBOX_OP_QCONF);
	if (!m)
		return -ENOMEM;

	hdr = &m->hdr;
	fmap = &m->fmap;

	fmap->qbase = 0;
	fmap->qmax = qmax;

	rv = qdma_mbox_msg_send(xdev, m, 1, MBOX_OP_QCONF_RESP,
				QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		pr_info("%s set q range (qconf) failed %d.\n",
			xdev->conf.name, rv);
		goto err_out;
	}

	if (hdr->status) {
		rv = hdr->status;
		pr_err("Failed to set qconf\n");
		goto err_out;
	}

	*qbase = fmap->qbase;

	qdma_device_set_cfg_state((unsigned long)xdev, QMAX_CFG_USER);
	pr_debug("%s, func id %u/%u, Q 0x%x + 0x%x.\n",
			xdev->conf.name, xdev->func_id, xdev->func_id_parent,
		*qbase, qmax);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}
#else
static int device_set_qrange(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = 0;

	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	hw_set_fmap(xdev, xdev->func_id, qdev->qbase, qdev->qmax);

	qdev->init_qrange = 1;

	pr_debug("%s, func id %u, Q 0x%x + 0x%x.\n",
		xdev->conf.name, xdev->func_id, qdev->qbase, qdev->qmax);

	return rv;
}
#endif /* ifndef __QDMA_VF__ */

#ifndef __QDMA_VF__
#ifdef ERR_DEBUG
static void qdma_err_mon(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,
						struct delayed_work, work);
	struct xlnx_dma_dev *xdev = container_of(dwork,
					struct xlnx_dma_dev, err_mon);

	if (!xdev) {
		pr_err("Invalid xdev");
		return;
	}
	spin_lock(&xdev->err_lock);

	if (xdev->err_mon_cancel == 0) {
		err_stat_handler(xdev);
		schedule_delayed_work(dwork, msecs_to_jiffies(50)); /* 50msec */
	}
	spin_unlock(&xdev->err_lock);
}
#endif
#endif

int qdma_device_prep_q_resource(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = 0;

	spin_lock(&qdev->lock);

	if (qdev->init_qrange)
		goto done;

	rv = device_set_qrange(xdev);
	if (rv < 0)
		goto done;

	rv = intr_ring_setup(xdev);
	if (rv)
		goto done;

	if ((xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		if (xdev->intr_coal_list != NULL) {
			rv = qdma_intr_context_setup(xdev);
			if (rv)
				goto done;
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
			xdev->conf.master_pf)
		qdma_err_intr_setup(xdev, 0);

#ifdef ERR_DEBUG
	else {
		if ((xdev->conf.master_pf) &&
				(xdev->conf.qdma_drv_mode == POLL_MODE)) {
			spin_lock_init(&xdev->err_lock);
			xdev->err_mon_cancel = 0;
			INIT_DELAYED_WORK(&xdev->err_mon, qdma_err_mon);
			schedule_delayed_work(&xdev->err_mon,
					      msecs_to_jiffies(50));
		}
	}
#endif
#endif

done:
	spin_unlock(&qdev->lock);

	return rv;
}

int qdma_device_init(struct xlnx_dma_dev *xdev)
{
	int i;
	int rv = 0;
	int qmax = xdev->conf.qsets_max;
	struct qdma_descq *descq;
	struct qdma_dev *qdev;

#ifdef __QDMA_VF__
	xdev->func_id = xdev->func_id_parent = 0; /* filled later */
#else
	if (xdev->conf.master_pf) {
		pr_info("%s master PF clearing memory.\n", xdev->conf.name);
		rv = hw_init_global_context_memory(xdev);
		if (rv)
			return rv;
	}
#endif

	if (xdev->conf.qdma_drv_mode != POLL_MODE &&
			xdev->conf.qdma_drv_mode != LEGACY_INTR_MODE) {
		rv = intr_setup(xdev);
		if (rv)
			return -EINVAL;
	}
	qdev = kzalloc(sizeof(struct qdma_dev) +
			sizeof(struct qdma_descq) * qmax * 2, GFP_KERNEL);
	if (!qdev) {
		pr_err("dev %s qmax %d OOM.\n",
			dev_name(&xdev->conf.pdev->dev), qmax);
		intr_teardown(xdev);
		return -ENOMEM;
	}

	spin_lock_init(&qdev->lock);

	descq = (struct qdma_descq *)(qdev + 1);
	qdev->h2c_descq = descq;
	qdev->c2h_descq = descq + qmax;

	xdev->dev_priv = (void *)qdev;
	qdev->qmax = qmax;
	qdev->init_qrange = 0;

#ifdef __QDMA_VF__
	if (xdev->conf.cur_cfg_state == QMAX_CFG_UNCONFIGURED) {
		qdev->qbase = TOTAL_QDMA_QS - TOTAL_VF_QS
			+ (xdev->conf.idx - 1) * QDMA_Q_PER_VF_MAX;
	} else
		qdev->qbase = xdev->conf.qsets_base;
#else
	/*
	 * for the first device make qbase as 0 indicated by <0 value
	 * for others initial configuration, go for QDMA_Q_PER_PF_MAX
	 * for qbase
	 * if modified using sysfs/qmax, take it from calculated qbase
	 */
	if (xdev->conf.cur_cfg_state == QMAX_CFG_UNCONFIGURED) {
		qdev->qbase = (xdev->conf.idx - 1) * MAX_QS_PER_PF;
		xdev->conf.cur_cfg_state = QMAX_CFG_INITIAL;
	} else
		qdev->qbase = xdev->conf.qsets_base;
#endif
	xdev->conf.qsets_base = qdev->qbase;

	for (i = 0, descq = qdev->h2c_descq; i < qdev->qmax; i++, descq++)
		qdma_descq_init(descq, xdev, i, i);
	for (i = 0, descq = qdev->c2h_descq; i < qdev->qmax; i++, descq++)
		qdma_descq_init(descq, xdev, i, i);
#ifndef __QDMA_VF__
	if (xdev->conf.master_pf) {
		pr_info("%s master PF.\n", xdev->conf.name);
		hw_set_global_csr(xdev);
		qdma_trq_c2h_config(xdev);
		for (i = 0; i < xdev->mm_channel_max; i++) {
			hw_mm_channel_enable(xdev, i, 1);
			hw_mm_channel_enable(xdev, i, 0);
		}
	}
#endif
	/** STM specific init */
	xdev->pipe_stm_max_pkt_size = STM_MAX_PKT_SIZE;

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

#ifndef __QDMA_VF__
#ifdef ERR_DEBUG
	if ((xdev->conf.master_pf) &&
		(xdev->conf.qdma_drv_mode == POLL_MODE)) {
		pr_info("Cancelling delayed work");
		spin_lock(&xdev->err_lock);
		xdev->err_mon_cancel = 1;
		cancel_delayed_work_sync(&xdev->err_mon);
		spin_unlock(&xdev->err_lock);
	}
#endif
#endif

	for (i = 0, descq = qdev->h2c_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ONLINE)
			qdma_queue_stop((unsigned long int)xdev, i, NULL, 0);
	}

	for (i = 0, descq = qdev->c2h_descq; i < qdev->qmax; i++, descq++) {
		if (descq->q_state == Q_STATE_ONLINE)
			qdma_queue_stop((unsigned long int)xdev,
					i + qdev->qmax, NULL, 0);
	}

	intr_teardown(xdev);

	if ((xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		pr_info("dev %s teardown interrupt coalescing ring\n",
					dev_name(&xdev->conf.pdev->dev));
		intr_ring_teardown(xdev);
	}

#ifndef __QDMA_VF__
	if (xdev->func_id == 0) {
		for (i = 0; i < xdev->mm_channel_max; i++) {
			hw_mm_channel_disable(xdev, i, DMA_TO_DEVICE);
			hw_mm_channel_disable(xdev, i, DMA_FROM_DEVICE);
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
			if (buf && buflen)  {
				snprintf(buf, buflen,
					"%s, q idx too big 0x%lx > 0x%x.\n",
					xdev->conf.name, idx, qdev->qmax);
			}
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
			if (buf && buflen) {
				snprintf(buf, buflen,
				"%s, idx 0x%lx, q 0x%p state invalid.\n",
					xdev->conf.name, idx, descq);
			}
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
			if (buf && buflen)  {
				snprintf(buf, buflen,
					"%s, q idx too big 0x%lx > 0x%x.\n",
					xdev->conf.name, idx, qdev->qmax);
			}
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
			if (buf && buflen) {
				snprintf(buf, buflen,
				"%s, idx 0x%lx, q 0x%p state invalid.\n",
					xdev->conf.name, idx, pair_descq);
			}
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
