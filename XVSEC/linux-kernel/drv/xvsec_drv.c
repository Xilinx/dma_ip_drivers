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


#include "version.h"
#include "xvsec_drv.h"
#include "xvsec_drv_int.h"
#include "xvsec_cdev.h"
#include "xvsec_mcap.h"

static char version[] = XVSEC_MODULE_DESC " v" XVSEC_DRV_VERSION "\n";
static struct xvsec_dev	xvsec_dev;

/** Global context information to store VSEC information of all pcie devices */
struct class		*g_xvsec_class;

/** ioctl function to retrieve capability list of a pcie-device */
static long xvsec_ioc_get_cap_list(struct file *filep,
	uint32_t cmd, unsigned long arg);

/** ioctl function to retrieve the pcie-device info */
static long xvsec_ioc_get_device_info(struct file *filep,
	uint32_t cmd, unsigned long arg);

/** This structure holds the vsec information supported by this driver
 *
 *  All supported VSECs by this driver should be added here.
 *
 *  This is the master VSEC list supported by this driver
 */
static struct vsec_ops xvsec_supported_ops[] = {
	/** MCAP VSEC info */
	{
		.vsec_id = XVSEC_MCAP_VSEC_ID,
		.vsec_module_init = xvsec_mcap_module_init,
		.vsec_module_exit = xvsec_mcap_module_exit,
	}
};


static int xvsec_populate_vsec_capabilities(struct context *dev_ctx,
	int index, uint32_t vendor_data)
{
	int ret = 0;
	uint16_t vsec_rev_id = 0x0;
	uint16_t vsec_id = 0x0;

	dev_ctx->capabilities.vsec_info[index].is_supported = false;

	vsec_id = (uint16_t)(vendor_data & XVSEC_VSEC_ID_POS) >>
		XVSEC_VSEC_ID_SHIFT;

	dev_ctx->capabilities.vsec_info[index].cap_id = vsec_id;

	vsec_rev_id = (vendor_data & XVSEC_REV_ID_POS) >>
		XVSEC_REV_ID_SHIFT;

	dev_ctx->capabilities.vsec_info[index].rev_id = vsec_rev_id;

	pr_debug("%s: vsec_id: %d, vsec_rev_id: %d\n", __func__,
			dev_ctx->capabilities.vsec_info[index].cap_id,
			dev_ctx->capabilities.vsec_info[index].rev_id);

	switch (dev_ctx->capabilities.vsec_info[index].cap_id) {
	case XVSEC_MCAP_VSEC_ID:
		dev_ctx->capabilities.vsec_info[index].is_supported = true;
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_MCAP_VSEC     ");
			break;
	case XVSEC_XVC_DEBUG_VSEC_ID:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_XVC_DEBUG_VSEC");
			break;
	case XVSEC_SCID_VSEC_ID:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_SCID_VSEC     ");
			break;
	case XVSEC_ALF_VSEC_ID:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_ALF_VSEC      ");
			break;
	case XVSEC_SWITCH_VSEC_ID:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_SWITCH_VSEC   ");
			break;
	case XVSEC_NULL_VSEC_ID:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"PCIe_NULL_VSEC     ");
			break;
	default:
		snprintf(dev_ctx->capabilities.vsec_info[index].name,
			MAX_VSEC_STR_LEN,
			"UNKNOWN            ");
			break;
	}

	return ret;
}

static int xvsec_retrieve_vsec_capabilities(struct context *dev_ctx)
{
	int ret = 0;
	int offset = 0;
	int nxt_offset;
	int index = 0;
	uint16_t i, j;
	uint16_t count = 0;
	struct pci_dev *pdev = dev_ctx->pdev;
	uint32_t vendor_data;

	index = 0;
	do {
		nxt_offset = pci_find_next_ext_capability(pdev,
			offset, XVSEC_EXT_CAP_VSEC_ID);
		if (nxt_offset == 0)
			break;

		ret = pci_read_config_dword(pdev,
				(nxt_offset + XVSEC_VENDOR_HEADER_OFFSET),
				&vendor_data);

		if (ret != 0) {
			pr_warn("pci_read_config_dword failed with error");
			pr_warn(" %d, offset : 0x%X\n", ret, offset);
			break;
		}

		dev_ctx->capabilities.vsec_info[index].offset = nxt_offset;
		xvsec_populate_vsec_capabilities(dev_ctx, index, vendor_data);
		offset = nxt_offset;

		for (i = 0; i < ARRAY_SIZE(xvsec_supported_ops); i++) {
			if (dev_ctx->capabilities.vsec_info[index].cap_id ==
				xvsec_supported_ops[i].vsec_id)
				count++;
		}

		index = index + 1;
	} while (nxt_offset != 0);

	dev_ctx->capabilities.no_of_caps = index;

	pr_debug("dev_ctx->capabilities.no_of_caps : %d\n", index);

	/** Allocate memory for supported vsec count to store its context */
	dev_ctx->vsec_supported_cnt = count;
	dev_ctx->vsec_ctx =
		kzalloc((count * sizeof(struct vsec_context)), GFP_KERNEL);
	if (dev_ctx->vsec_ctx == NULL) {
		pr_err("Memory Allocation failed for dev_ctx->vsec_ctx");
		return -(ENOMEM);
	}


	/** Scan through supported vsec ids */
	index = 0;
	for (i = 0; i < dev_ctx->capabilities.no_of_caps; i++) {
		for (j = 0; j < ARRAY_SIZE(xvsec_supported_ops); j++) {
			if (dev_ctx->capabilities.vsec_info[i].cap_id ==
				xvsec_supported_ops[j].vsec_id) {
				dev_ctx->vsec_ctx[index].vsec_offset =
				dev_ctx->capabilities.vsec_info[i].offset;
				dev_ctx->vsec_ctx[index].vsec_ops =
					&xvsec_supported_ops[j];
				dev_ctx->vsec_ctx[index].pdev = dev_ctx->pdev;
				index++;
				break;
			}
		}
	}

	return ret;
}

static int xvsec_gen_open(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct context		*dev_ctx;
	struct file_priv	*priv;

	dev_ctx = container_of(inode->i_cdev,
			struct context, generic_cdev.cdev);

	spin_lock(&dev_ctx->lock);

	if (dev_ctx->fopen_cnt != 0) {
		ret = -(EBUSY);
		goto CLEANUP;
	}

	priv = kzalloc(sizeof(struct file_priv), GFP_KERNEL);
	if (priv == NULL) {
		ret = -(ENOMEM);
		goto CLEANUP;
	}

	dev_ctx->fopen_cnt++;

	priv->dev_ctx = (void *)dev_ctx;
	filep->private_data = priv;

	pr_info("%s success\n", __func__);

CLEANUP:
	spin_unlock(&dev_ctx->lock);
	return ret;
}

static int xvsec_gen_close(struct inode *inode, struct file *filep)
{
	struct file_priv *priv = filep->private_data;
	struct context   *dev_ctx = (struct context *)priv->dev_ctx;

	spin_lock(&dev_ctx->lock);
	if (dev_ctx->fopen_cnt == 0) {
		pr_warn("File Open/close mismatch\n");
	} else {
		dev_ctx->fopen_cnt--;
		pr_info("%s success\n", __func__);
	}
	kfree(priv);
	spin_unlock(&dev_ctx->lock);

	return 0;
}

static long xvsec_ioc_get_cap_list(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *dev_ctx = (struct context *)priv->dev_ctx;

	pr_debug("ioctl : IOC_XVSEC_GET_CAP_LIST\n");

	spin_lock(&dev_ctx->lock);
	ret = copy_to_user((void __user *)arg,
		(void *)&dev_ctx->capabilities,
		sizeof(struct xvsec_capabilities));
	spin_unlock(&dev_ctx->lock);

	return ret;
}

static int xvsec_get_dev_info(struct context *dev_ctx,
	union device_info *dev_info)
{
	int ret = 0;
	struct pci_dev *pdev = dev_ctx->pdev;

	dev_info->vendor_id		= pdev->vendor;
	dev_info->device_id		= pdev->device;
	dev_info->device_no		= PCI_SLOT(pdev->devfn);
	dev_info->device_fn		= PCI_FUNC(pdev->devfn);
	dev_info->subsystem_vendor	= pdev->subsystem_vendor;
	dev_info->subsystem_device	= pdev->subsystem_device;
	dev_info->class_id		= pdev->class;
	dev_info->cfg_size		= pdev->cfg_size;
	dev_info->is_msi_enabled	= pdev->msi_enabled;
	dev_info->is_msix_enabled	= pdev->msix_enabled;

	return ret;
}

static long xvsec_ioc_get_device_info(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv	*priv = filep->private_data;
	struct context		*dev_ctx = (struct context *)priv->dev_ctx;
	union device_info	dev_info;

	pr_debug("ioctl : IOC_GET_DEVICE_INFO\n");

	spin_lock(&dev_ctx->lock);
	ret = xvsec_get_dev_info(dev_ctx, &dev_info);
	spin_unlock(&dev_ctx->lock);

	if (ret < 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&dev_info, sizeof(union device_info));
CLEANUP:
	return ret;
}


static const struct xvsec_ioctl_ops xvsec_gen_ioctl_ops[] = {
	{IOC_XVSEC_GET_CAP_LIST,	xvsec_ioc_get_cap_list},
	{IOC_XVSEC_GET_DEVICE_INFO,	xvsec_ioc_get_device_info},
};

static long xvsec_gen_ioctl(struct file *filep, uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	int index, cmd_cnt;
	bool cmd_found = false;

	cmd_cnt = ARRAY_SIZE(xvsec_gen_ioctl_ops);
	for (index = 0; index < cmd_cnt; index++) {
		if (xvsec_gen_ioctl_ops[index].cmd == cmd) {
			cmd_found = true;
			ret = xvsec_gen_ioctl_ops[index].fpfunction(
				filep, cmd, arg);
			break;
		}
	}
	if (cmd_found == false)
		ret = -(EINVAL);

	return ret;
}

/** pcie device generic fops structure */
static const struct file_operations xvsec_gen_fops = {
	.owner		= THIS_MODULE,
	.open		= xvsec_gen_open,
	.release	= xvsec_gen_close,
	.unlocked_ioctl	= xvsec_gen_ioctl,
};


static int xvsec_initialize(struct pci_dev *pdev, struct context *dev_ctx)
{
	int ret = 0;
	int status;
	uint16_t index;
	struct vsec_ops *vsec_ops;

	if ((pdev == NULL) || (dev_ctx == NULL))
		return -(EINVAL);

	dev_ctx->pdev = pdev;

	pr_info("%s : dev_ctx address : %p\n", __func__, dev_ctx);

	ret = xvsec_retrieve_vsec_capabilities(dev_ctx);
	if (ret < 0) {
		pr_err("Error In retrieving VSEC capabilities :");
		pr_err(" err code : %d\n", ret);
		return ret;
	}

	spin_lock_init(&dev_ctx->lock);
	ret = xvsec_cdev_create(pdev,
		&dev_ctx->generic_cdev, &xvsec_gen_fops, NULL);
	if (ret < 0) {
		pr_err("xvsec_cdev_create failed for generic cdev, err : %d\n",
			ret);
	}

	/** Initialize the supported VSEC IDs */
	for (index = 0; index < dev_ctx->vsec_supported_cnt; index++) {
		vsec_ops = dev_ctx->vsec_ctx[index].vsec_ops;
		status = vsec_ops->vsec_module_init(&dev_ctx->vsec_ctx[index]);
		if (status < 0) {
			pr_warn("vsec_module_init failed");
			pr_warn("    Index : %d, VSEC ID : %d\n", index,
				dev_ctx->vsec_ctx[index].vsec_ops->vsec_id);
		}
	}

	return ret;
}
EXPORT_SYMBOL_GPL(xvsec_initialize);

static int xvsec_deinitialize(struct context *dev_ctx)
{
	int ret = 0;
	uint16_t index;
	struct vsec_ops *vsec_ops;

	if (dev_ctx == NULL)
		return -(EINVAL);

	pr_err("%s : cnt : %d\n", __func__, dev_ctx->vsec_supported_cnt);

	xvsec_cdev_remove(&dev_ctx->generic_cdev);

	for (index = 0; index < dev_ctx->vsec_supported_cnt; index++) {
		vsec_ops = dev_ctx->vsec_ctx[index].vsec_ops;
		vsec_ops->vsec_module_exit(&dev_ctx->vsec_ctx[index]);
	}

	/** Checkpatch : kfree(NULL) is safe */
	kfree(dev_ctx->vsec_ctx);
	dev_ctx->vsec_ctx = NULL;

	return ret;
}
EXPORT_SYMBOL_GPL(xvsec_deinitialize);

static int __init xvsec_drv_init(void)
{
	int ret = 0;
	int index, loop;
	uint32_t dev_count, count;
	bool duplicate;
	struct pci_dev *pdev;
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
	if (pdev_list == NULL) {
		class_destroy(g_xvsec_class);
		g_xvsec_class = NULL;
		return -(ENOMEM);
	}

	count = 0;
	pdev = NULL;
	for (index = 0; index < dev_count; index++) {
		duplicate = false;
		pdev = pci_get_device(XILINX_VENDOR_ID, PCI_ANY_ID, pdev);
		if (pdev == NULL) {
			ret = -(EBUSY);
			goto CLEANUP_PDEV_MEM;
		}
		for (loop = 0; loop < count; loop++) {
			if ((pdev_list[loop]->bus->number == pdev->bus->number)
				&& (PCI_SLOT(pdev_list[loop]->devfn) ==
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
		ret = -(ENOMEM);
		goto CLEANUP_PDEV_MEM;
	}

	dev_count = count;
	xvsec_dev.dev_cnt = dev_count;
	for (index = 0; index < dev_count; index++) {
		ret = xvsec_initialize(pdev_list[index], &xvsec_dev.ctx[index]);
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
		xvsec_deinitialize(&xvsec_dev.ctx[index - 1]);

	kfree(xvsec_dev.ctx);
	xvsec_dev.ctx = NULL;

CLEANUP_PDEV_MEM:
	kfree(pdev_list);
	class_destroy(g_xvsec_class);
	g_xvsec_class = NULL;
	return ret;
}

static void __exit xvsec_drv_exit(void)
{
	int index;
	int ret;

	for (index = 0; index < xvsec_dev.dev_cnt; index++) {
		ret = xvsec_deinitialize(&xvsec_dev.ctx[index]);
		if (ret < 0) {
			pr_err("xvsec_deinitialize() failed with error ");
			pr_err("%d for device %d\n", ret, (index + 1));
		} else {
			pr_info("xvsec_deinitialize() success for device %d\n",
				(index + 1));
		}
	}

	pr_info("In %s : dev_cnt : %d\n", __func__, xvsec_dev.dev_cnt);

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
