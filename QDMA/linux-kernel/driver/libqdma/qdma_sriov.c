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
#include <linux/string.h>
#include <linux/errno.h>

#include "xdev.h"
#include "qdma_mbox.h"
#include <linux/sched.h>

#ifdef __QDMA_VF__
int xdev_sriov_vf_offline(struct xlnx_dma_dev *xdev, u8 func_id)
{
	int rv;
	struct mbox_msg *m = qdma_mbox_msg_alloc();

	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_offline(xdev->func_id, m->raw);
	/** For sending BYE message, retry to send multiple
	 * times before giving up by giving non-zero timeout value
	 */
	rv = qdma_mbox_msg_send(xdev, m, 0, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0)
		pr_info("%s, send bye failed %d.\n", xdev->conf.name, rv);

	return rv;
}

int xdev_sriov_vf_reset_offline(struct xlnx_dma_dev *xdev)
{
	int rv;
	struct mbox_msg *m = qdma_mbox_msg_alloc();

	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_reset_offline(xdev->func_id, m->raw);

	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0)
		pr_err("%s, send reset bye failed %d.\n", xdev->conf.name, rv);

	qdma_mbox_msg_free(m);
	return rv;
}
int xdev_sriov_vf_online(struct xlnx_dma_dev *xdev, u8 func_id)
{
	int rv;
	int qbase = -1;
	struct mbox_msg *m = qdma_mbox_msg_alloc();

	if (!m)
		return -ENOMEM;

	qmda_mbox_compose_vf_online(xdev->func_id, 0, &qbase, m->raw);

	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0) {
		pr_err("%s, send hello failed %d.\n",  xdev->conf.name, rv);
		qdma_mbox_msg_free(m);
		return rv;
	}

	rv = qdma_mbox_vf_dev_info_get(m->raw, &xdev->dev_cap,
			&xdev->dma_device_index);
	if (rv < 0) {
		pr_info("%s, failed to get dev info %d.\n",
			xdev->conf.name, rv);
		rv = -EINVAL;
	} else {
		pr_info("%s: num_pfs:%d, num_qs:%d, flr_present:%d, st_en:%d, mm_en:%d, mm_cmpt_en:%d, mailbox_en:%d, mm_channel_max:%d, qid2vec_ctx:%d, cmpt_ovf_chk_dis:%d, mailbox_intr:%d, sw_desc_64b:%d, cmpt_desc_64b:%d, dynamic_bar:%d, legacy_intr:%d, cmpt_trig_count_timer:%d",
				xdev->conf.name,
				xdev->dev_cap.num_pfs,
				xdev->dev_cap.num_qs,
				xdev->dev_cap.flr_present,
				xdev->dev_cap.st_en,
				xdev->dev_cap.mm_en,
				xdev->dev_cap.mm_cmpt_en,
				xdev->dev_cap.mailbox_en,
				xdev->dev_cap.mm_channel_max,
				xdev->dev_cap.qid2vec_ctx,
				xdev->dev_cap.cmpt_ovf_chk_dis,
				xdev->dev_cap.mailbox_intr,
				xdev->dev_cap.sw_desc_64b,
				xdev->dev_cap.cmpt_desc_64b,
				xdev->dev_cap.dynamic_bar,
				xdev->dev_cap.legacy_intr,
				xdev->dev_cap.cmpt_trig_count_timer);
	}

	qdma_mbox_msg_free(m);
	return rv;
}

#elif defined(CONFIG_PCI_IOV)

void xdev_sriov_disable(struct xlnx_dma_dev *xdev)
{
	struct pci_dev *pdev = xdev->conf.pdev;
	unsigned int sleep_timeout = (50 * xdev->vf_count); /* 50ms per vf */

	if (!xdev->vf_count)
		return;

	pci_disable_sriov(pdev);

	qdma_waitq_wait_event_timeout(xdev->wq, (xdev->vf_count == 0),
					 msecs_to_jiffies(sleep_timeout));

	qdma_mbox_stop(xdev);
	kfree(xdev->vf_info);
	xdev->vf_info = NULL;
	xdev->vf_count = 0;

}

int xdev_sriov_enable(struct xlnx_dma_dev *xdev, int num_vfs)
{
	struct pci_dev *pdev = xdev->conf.pdev;
	int current_vfs = pci_num_vf(pdev);
	struct qdma_vf_info *vf;
	int i;
	int rv;

	if (current_vfs) {
		dev_err(&pdev->dev, "%d VFs already enabled!n", current_vfs);
		return current_vfs;
	}

	vf = kmalloc(num_vfs * (sizeof(struct qdma_vf_info)), GFP_KERNEL);
	if (!vf) {
		pr_info("%s failed to allocate memory for VFs, %d * %ld.\n",
			xdev->conf.name, num_vfs, sizeof(struct qdma_vf_info));
		return -ENOMEM;
	}

	for (i = 0; i < num_vfs; i++)
		vf[i].func_id = QDMA_FUNC_ID_INVALID;

	qdma_waitq_init(&xdev->wq);
	xdev->vf_count = num_vfs;
	xdev->vf_info = vf;

	pr_debug("%s: req %d, current %d, assigned %d.\n",
		xdev->conf.name, num_vfs, current_vfs, pci_vfs_assigned(pdev));

	qdma_mbox_start(xdev);

	rv = pci_enable_sriov(pdev, num_vfs);
	if (rv) {
		pr_info("%s, enable sriov %d failed %d.\n",
			xdev->conf.name, num_vfs, rv);
		xdev_sriov_disable(xdev);
		return 0;
	}

	pr_debug("%s: done, req %d, current %d, assigned %d.\n",
		xdev->conf.name, num_vfs, pci_num_vf(pdev),
		pci_vfs_assigned(pdev));

	return num_vfs;
}

int qdma_device_sriov_config(struct pci_dev *pdev, unsigned long dev_hndl,
				int num_vfs)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv;

	if (!dev_hndl)
		return -EINVAL;

	rv = xdev_check_hndl(__func__, pdev, dev_hndl);
	if (rv < 0)
		return rv;

	/* if zero disable sriov */
	if (!num_vfs) {
		xdev_sriov_disable(xdev);
		return 0;
	}

	if (!xdev->dev_cap.mailbox_en) {
		dev_err(&pdev->dev, "Mailbox not enabled in this device");
		return -EPERM;
	}

	rv = xdev_sriov_enable(xdev, num_vfs);
	if (rv < 0)
		return rv;

	return xdev->vf_count;
}

void xdev_sriov_vf_offline(struct xlnx_dma_dev *xdev, u8 func_id)
{
	struct qdma_vf_info *vf = (struct qdma_vf_info *)xdev->vf_info;
	int i;

	xdev->vf_count_online--;
	for (i = 0; i < xdev->vf_count; i++, vf++) {
		if (vf->func_id == func_id) {
			/** func_id cannot be marked invalid in the PF FLR flow.
			 *  This is because, after PF comes back up
			 *  it should have valid func_id list to be able to
			 *  send out the RESET_DONE msg
			 */
			if (xdev->reset_state == RESET_STATE_IDLE)
				vf->func_id = QDMA_FUNC_ID_INVALID;
			vf->qbase = 0;
			vf->qmax = 0;
		}
	}

	qdma_waitq_wakeup(&xdev->wq);
}

int xdev_sriov_vf_online(struct xlnx_dma_dev *xdev, u8 func_id)
{
	struct qdma_vf_info *vf = (struct qdma_vf_info *)xdev->vf_info;
	int i;

	xdev->vf_count_online++;

	if (xdev->reset_state == RESET_STATE_IDLE) {
		for (i = 0; i < xdev->vf_count; i++, vf++) {
			if (vf->func_id == QDMA_FUNC_ID_INVALID) {
				vf->func_id = func_id;
				return 0;
			}
		}
		pr_info("%s, func 0x%x, NO free slot.\n", xdev->conf.name,
				func_id);
		qdma_waitq_wakeup(&xdev->wq);
		return -EINVAL;
	} else
		return 0;
}

int xdev_sriov_vf_fmap(struct xlnx_dma_dev *xdev, u8 func_id,
			unsigned short qbase, unsigned short qmax)
{
	struct qdma_vf_info *vf = (struct qdma_vf_info *)xdev->vf_info;
	int i;

	for (i = 0; i < xdev->vf_count; i++, vf++) {
		if (vf->func_id == func_id) {
			vf->qbase = qbase;
			vf->qmax = qmax;
			return 0;
		}
	}

	pr_info("%s, func 0x%x, NO match.\n", xdev->conf.name, func_id);
	return -EINVAL;
}

#endif /* if defined(CONFIG_PCI_IOV) && !defined(__QDMA_VF__) */
