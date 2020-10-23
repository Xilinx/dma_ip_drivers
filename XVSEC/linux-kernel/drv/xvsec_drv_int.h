/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2020,  Xilinx, Inc.
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

#ifndef __XVSEC_DRV_INT_H__
#define __XVSEC_DRV_INT_H__

#include "xvsec_cdev.h"

/**
 * @file xvsec_drv_int.h
 *
 * Xilinx XVSEC Driver Library Definitions
 *
 * Header file *xvsec_drv_int.h* defines data structures
 * needed for driver implementation
 *
 * These data structures are purely internal to the driver
 * and not meant to user
 */

/**
 * XILINX PCIe vendor ID
 */
#define XILINX_VENDOR_ID		(uint16_t)(0x10ee)

#define XVSEC_EXT_CAP_VSEC_ID		(0x000B)

#define XVSEC_MCAP_VSEC_ID		(0x0001)
#define XVSEC_XVC_DEBUG_VSEC_ID		(0x0008)
#define XVSEC_SCID_VSEC_ID		(0x0010)
#define XVSEC_ALF_VSEC_ID		(0x0020)
#define XVSEC_SWITCH_VSEC_ID		(0x0040)
#define XVSEC_NULL_VSEC_ID		(0xFFFF)


#define INVALID_DEVICE_INDEX		(uint8_t)(255)
#define INVALID_OFFSET			(0xFFFF)

/**
 * Extended Capability Header Offset
 */
#define XVSEC_EXTENDED_HEADER_OFFSET	(0x0000)
/**
 * Vendor Specific Header Offset
 *
 * provides details about the VSEC information
 */
#define XVSEC_VENDOR_HEADER_OFFSET	(0x0004)
#define XVSEC_VSEC_ID_SHIFT		0
#define XVSEC_REV_ID_SHIFT		16
#define XVSEC_VSEC_ID_POS		(0xFFFF << XVSEC_VSEC_ID_SHIFT)
#define XVSEC_REV_ID_POS		(0xF << XVSEC_REV_ID_SHIFT)
#define XVSEC_MCAP_ID			(0x0001)


struct file_priv {
	void *dev_ctx;
};

struct xvsec_ioctl_ops {
	uint32_t cmd;
	long (*fpfunction)(struct file *filep, uint32_t cmd, unsigned long arg);
};

struct vsec_context {
	struct pci_dev		*pdev;
	spinlock_t		lock;
	struct cdev_info	char_dev;
	int			fopen_cnt;
	uint16_t		vsec_offset;
	struct vsec_ops		*vsec_ops;
	void			*vsec_priv;
};

struct vsec_ops {
	uint32_t	vsec_id;
	int (*vsec_module_init)(struct vsec_context *vsec_ctx);
	void (*vsec_module_exit)(struct vsec_context *vsec_ctx);
};

struct context {
	struct pci_dev			*pdev;
	int				fopen_cnt;
	spinlock_t			lock;
	struct cdev_info		generic_cdev;
	uint16_t			vsec_supported_cnt;
	struct vsec_context		*vsec_ctx;
	struct xvsec_capabilities	capabilities;
};

struct xvsec_dev {
	uint32_t	dev_cnt;
	struct context	*ctx;
};

extern struct class		*g_xvsec_class;

int xvsec_mcap_module_init(struct vsec_context *dev_ctx);
void xvsec_mcap_module_exit(struct vsec_context *dev_ctx);

#endif /* __XVSEC_DRV_INT_H__ */
