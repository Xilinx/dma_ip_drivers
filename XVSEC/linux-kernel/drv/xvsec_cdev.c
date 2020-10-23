/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
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
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "xvsec_drv.h"
#include "xvsec_drv_int.h"
#include "xvsec_cdev.h"


int xvsec_cdev_create(
	struct pci_dev *pdev,
	struct cdev_info *char_dev,
	const struct file_operations *fops,
	const char *dev_name)
{
	int ret = 0;
	uint8_t bus_no;
	uint8_t dev_no;

	if ((pdev == NULL) || (char_dev == NULL) ||
		(fops == NULL)) {
		pr_err(__FILE__": Invalid Parameters\n");
		return -EINVAL;
	}

	bus_no = pdev->bus->number;
	dev_no = PCI_SLOT(pdev->devfn);

	ret = alloc_chrdev_region(&char_dev->dev_no,
		XVSEC_MINOR_BASE, XVSEC_MINOR_COUNT, XVSEC_NODE_NAME);
	if (ret < 0) {
		pr_err("Major number allocation is failed\n");
		goto CLEANUP1;
	}

	char_dev->major_no =
		MAJOR(char_dev->dev_no);

	pr_info("The major number is %d, bus no : %d, dev no : %d\n",
		char_dev->major_no, bus_no, dev_no);

	cdev_init(&char_dev->cdev, fops);

	ret = cdev_add(&char_dev->cdev, char_dev->dev_no, 1);
	if (ret < 0) {
		pr_err("Unable to add cdev\n");
		goto CLEANUP2;
	}

	if (dev_name != NULL) {
		snprintf(char_dev->name,
			XVSEC_CDEV_NAME_MAX_LEN, "%s%02X%02X_%s",
			XVSEC_NODE_NAME, bus_no, dev_no, dev_name);
	} else {
		snprintf(char_dev->name,
			XVSEC_CDEV_NAME_MAX_LEN, "%s%02X%02X",
			XVSEC_NODE_NAME, bus_no, dev_no);
	}

	kobject_set_name(&char_dev->cdev.kobj, "%s", char_dev->name);

	char_dev->sys_device =
		device_create(g_xvsec_class, NULL,
			char_dev->dev_no, NULL, "%s", char_dev->name);

	if (IS_ERR(char_dev->sys_device)) {
		pr_err("failed to create device");
		ret = -(PTR_ERR(char_dev->sys_device));
		goto CLEANUP3;
	}

	pr_info("%s : %s\n", __func__, char_dev->name);

	return ret;

CLEANUP3:
	cdev_del(&char_dev->cdev);
CLEANUP2:
	unregister_chrdev_region(char_dev->dev_no,
		XVSEC_MINOR_COUNT);
CLEANUP1:
	return ret;

}
EXPORT_SYMBOL_GPL(xvsec_cdev_create);

void xvsec_cdev_remove(
	struct cdev_info *char_dev)
{
	if (char_dev == NULL)
		return;

	pr_info("%s : %s\n", __func__, char_dev->name);
	device_destroy(g_xvsec_class, char_dev->dev_no);
	cdev_del(&char_dev->cdev);
	unregister_chrdev_region(char_dev->dev_no, 1);
}
EXPORT_SYMBOL_GPL(xvsec_cdev_remove);

MODULE_LICENSE("Dual BSD/GPL");
