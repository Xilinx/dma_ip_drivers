/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018,  Xilinx, Inc.
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

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>

#include "version.h"
#include "xvsec_cdev.h"
#include "xvsec_cdev_int.h"

static char version[] =
	XVSEC_MODULE_DESC " v" XVSEC_DRV_VERSION "\n";

struct xvsec_dev	xvsec_dev;
struct class		*g_xvsec_class;
static int __init xvsec_drv_init(void)
{
	int ret = 0;
	int index, loop;
	uint32_t dev_count, count;
	bool duplicate;
	struct pci_dev *pdev = NULL;
	struct pci_dev **pdev_list = NULL;


	pr_info("%s", version);

	dev_count = 0x0;
	pdev = NULL;
	do {
		pdev = pci_get_device(XILINX_VENDOR_ID, PCI_ANY_ID, pdev);
		if (pdev == NULL)
			break;

		dev_count = dev_count + 1;
	} while (pdev != NULL);

	if (dev_count == 0)
		return 0;

	g_xvsec_class = class_create(THIS_MODULE, XVSEC_NODE_NAME);
	if (IS_ERR(g_xvsec_class)) {
		pr_err("failed to create class");
		ret = -(PTR_ERR(g_xvsec_class));
		return ret;
	}


	pdev_list = kzalloc((dev_count * sizeof(struct pci_dev *)),
		GFP_KERNEL);
	if (pdev_list == NULL)
		return -(ENOMEM);

	count = 0;
	pdev = NULL;
	for(index = 0; index < dev_count; index++) {
		duplicate = false;
		pdev = pci_get_device(XILINX_VENDOR_ID, PCI_ANY_ID, pdev);
		if (pdev == NULL) {
			kfree(pdev_list);
			return -(EBUSY);
		}
		for (loop = 0; loop < count; loop++) {
			if ((pdev_list[loop]->bus->number ==
				pdev->bus->number) &&
				(PCI_SLOT(pdev_list[loop]->devfn) ==
				PCI_SLOT(pdev->devfn))) {
					duplicate = true;
					break;
				}
		}

		if (duplicate == false) {
			pdev_list[count] = pdev;
			count = count + 1;
		}
	}

	xvsec_dev.ctx = kzalloc((count * sizeof(struct context)),
		GFP_KERNEL);
	if (xvsec_dev.ctx == NULL) {
		kfree(pdev_list);
		return -(ENOMEM);
	}

	dev_count = count;
	xvsec_dev.dev_cnt = dev_count;
	for (index = 0; index < dev_count; index++) {
		ret = xvsec_initialize(pdev_list[index], index);
		if (ret < 0) {
			pr_err("xvsec_initialize() failed with error ");
			pr_err("%d for device %d\n", ret, (index + 1));
			goto CLEANUP;
		} else {
			pr_info("xvsec_initialize() success for device %d\n",
				(index + 1));
		}
	}
	xvsec_dev.dev_cnt = index;

	kfree(pdev_list);
	pr_info("%s : Success\n", __func__);

	return ret;
CLEANUP:
	for (; index > 0; index--)
		xvsec_deinitialize(index - 1);

	kfree(xvsec_dev.ctx);
	xvsec_dev.ctx = NULL;
	return ret;
}

static void __exit xvsec_drv_exit(void)
{
	int index;

	pr_info("In xvsec_drv_exit : dev_cnt : %d\n", xvsec_dev.dev_cnt);

	for (index = 0; index < xvsec_dev.dev_cnt; index++)
		xvsec_deinitialize(index);

	class_destroy(g_xvsec_class);
	g_xvsec_class = NULL;
	kfree(xvsec_dev.ctx);
	xvsec_dev.ctx = NULL;
}

module_init(xvsec_drv_init);
module_exit(xvsec_drv_exit);

MODULE_INFO(intree, "Y");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(XVSEC_DRV_VERSION);
MODULE_AUTHOR("Xilinx Inc.");
MODULE_DESCRIPTION("XVSEC Device Driver");
