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
#include <linux/string.h>
#include <linux/errno.h>

#include "xdev.h"
#include "qdma_mbox.h"

#ifdef __QDMA_VF__
int xdev_sriov_vf_offline(struct xlnx_dma_dev *xdev, u8 func_id)
{
	struct mbox_msg *m;
	int rv;

	m = qdma_mbox_msg_alloc(xdev, MBOX_OP_BYE);
	if (!m)
		return -ENOMEM;

	rv = qdma_mbox_msg_send(xdev, m, 0, 0, 0);
	if (rv < 0) {
		pr_info("%s, send bye failed %d.\n", xdev->conf.name, rv);
		return rv;
	}

	return 0;
}

int xdev_sriov_vf_online(struct xlnx_dma_dev *xdev, u8 func_id)
{
	struct mbox_msg *m;
	int rv;

	m = qdma_mbox_msg_alloc(xdev, MBOX_OP_HELLO);
	if (!m)
		return -ENOMEM;

	rv = qdma_mbox_msg_send(xdev, m, 0, 0, 0);
	if (rv < 0) {
		pr_info("%s, send hello failed %d.\n",  xdev->conf.name, rv);
		return rv;
	}

	/* initial value should always be qmax =1, qbase=0 */
	xdev->conf.qsets_max = 1;
	xdev->conf.qsets_base = 0;
	qdma_device_set_cfg_state((unsigned long)xdev, QMAX_CFG_INITIAL);

	return 0;
}

#elif defined(CONFIG_PCI_IOV)

struct qdma_vf_info {
	unsigned short func_id;
	unsigned short qbase;
	unsigned short qmax;
	unsigned short filler;
};

void xdev_sriov_disable(struct xlnx_dma_dev *xdev)
{
	struct pci_dev *pdev = xdev->conf.pdev;

	pci_disable_sriov(pdev);

	kfree(xdev->vf_info);
	xdev->vf_info = NULL;
	xdev->vf_count = 0;

	qdma_mbox_stop(xdev);
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
		pr_info("%s OOM, %d * %ld.\n",
			xdev->conf.name, num_vfs, sizeof(struct qdma_vf_info));
		return -ENOMEM;
	}

	for (i = 0; i < num_vfs; i++)
		vf[i].func_id = QDMA_FUNC_ID_INVALID;

	xdev->vf_count = num_vfs;
	xdev->vf_info = vf;

	pr_debug("%s: req %d, current %d, assigned %d.\n",
		xdev->conf.name, num_vfs, current_vfs, pci_vfs_assigned(pdev));

	rv = pci_enable_sriov(pdev, num_vfs);
	if (rv) {
		pr_info("%s, enable sriov %d failed %d.\n",
			xdev->conf.name, num_vfs, rv);
		xdev_sriov_disable(xdev);
		return 0;
	}

	qdma_mbox_start(xdev);

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

	/* if zeror disable sriov */
	if (!num_vfs) {
		xdev_sriov_disable(xdev);
		return 0;
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

	for (i = 0; i < xdev->vf_count; i++, vf++) {
		if (vf->func_id == func_id) {
			vf->func_id = QDMA_FUNC_ID_INVALID;
			vf->qbase = 0;
			vf->qmax = 0;
		}
	}
}

int xdev_sriov_vf_online(struct xlnx_dma_dev *xdev, u8 func_id)
{
	struct qdma_vf_info *vf = (struct qdma_vf_info *)xdev->vf_info;
	int i;

	for (i = 0; i < xdev->vf_count; i++, vf++) {
		if (vf->func_id == QDMA_FUNC_ID_INVALID) {
			vf->func_id = func_id;
			return 0;
		}
	}

	pr_info("%s, func 0x%x, NO free slot.\n", xdev->conf.name, func_id);
	return -EINVAL;
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
