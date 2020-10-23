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

#ifndef __XVSEC_CDEV_H__
#define __XVSEC_CDEV_H__

/**
 * @file xvsec_cdev.h
 *
 * Xilinx XVSEC Driver Library Definitions
 *
 * Header file *xvsec_cdev.h* defines data structures neeed for
 * character device implementation by the driver
 *
 * These data structures are purely internal to the driver
 * and not meant to user
 */


/** @defgroup xvsec_enums Enumerations
 */
/** @defgroup xvsec_struct Data Structures
 */
/**
 * @defgroup xvsec_defines Definitions
 * @{
 */


#define XVSEC_CDEV_NAME_MAX_LEN		(20)

#define XVSEC_NODE_NAME			"xvsec"

/**
 * XVSEC char device first minor number
 */
#define XVSEC_MINOR_BASE			(0)
/**
 * XVSEC char device total minor numbers count
 */
#define XVSEC_MINOR_COUNT			(1)


/** @} */

struct cdev_info {
	dev_t		dev_no;
	int		major_no;
	char		name[XVSEC_CDEV_NAME_MAX_LEN];
	struct cdev	cdev;
	struct device	*sys_device;
};

int xvsec_cdev_create(
	struct pci_dev *pdev,
	struct cdev_info *char_dev,
	const struct file_operations *fops,
	const char *dev_name);

void xvsec_cdev_remove(
	struct cdev_info *char_dev);

#endif /* __XVSEC_CDEV_H__ */
