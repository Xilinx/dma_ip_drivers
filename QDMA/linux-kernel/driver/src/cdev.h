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

#ifndef __QDMA_CDEV_H__
#define __QDMA_CDEV_H__
/**
 * @file
 * @brief This file contains the declarations for qdma pcie kernel module
 *
 */
#include <linux/cdev.h>
#include "version.h"
#include <linux/spinlock_types.h>

#include "libqdma/libqdma_export.h"
#include <linux/workqueue.h>

/** QDMA character device class name */
#define QDMA_CDEV_CLASS_NAME  DRV_MODULE_NAME
/** QDMA character device max minor number*/
#define QDMA_MINOR_MAX (2048)

/* per pci device control */
/**
 * @struct - qdma_cdev_cb
 * @brief	QDMA character device call back data
 */
struct qdma_cdev_cb {
	/** pointer to xilinx pcie device */
	struct xlnx_pci_dev *xpdev;
	/** character device lock  */
	spinlock_t lock;
	/** character device major number  */
	int cdev_major;
	/** character device minor number count  */
	int cdev_minor_cnt;
};

/**
 * @struct - qdma_cdev
 * @brief	QDMA character device book keeping parameters
 */
struct qdma_cdev {
	/** lsit of qdma character devices */
	struct list_head list_head;
	/** minor number */
	int minor;
	/** character device number */
	dev_t cdevno;
	/** pointer to qdma character device call back data */
	struct qdma_cdev_cb *xcb;
	/** pointer to kernel device(struct device) */
	struct device *sys_device;
	/** pointer to kernel cdev(struct cdev) */
	struct cdev cdev;
	/** c2h queue handle */
	unsigned long c2h_qhndl;
	/** hec queue handle */
	unsigned long h2c_qhndl;
	/** direction */
	unsigned short dir_init;
	/* flag to indicate if memcpy is required */
	unsigned char no_memcpy;
	/** call back function for open a device */
	int (*fp_open_extra)(struct qdma_cdev *xcdev);
	/** call back function for close a device */
	int (*fp_close_extra)(struct qdma_cdev *xcdev);
	/** call back function to handle ioctl message */
	long (*fp_ioctl_extra)(struct qdma_cdev *xcdev, unsigned int cmd,
			unsigned long arg);
	/** call back function to handle read write request*/
	ssize_t (*fp_rw)(unsigned long dev_hndl, unsigned long qhndl,
			struct qdma_request *req);
	ssize_t (*fp_aiorw)(unsigned long dev_hndl, unsigned long qhndl,
			unsigned long count, struct qdma_request **reqv);
	/** name of the character device*/
	char name[0];
};

/**
 * @struct - qdma_io_cb
 * @brief	QDMA character device io call back book keeping parameters
 */
struct qdma_io_cb {
	void *private;
	/** user buffer */
	void __user *buf;
	/** length of the user buffer */
	size_t len;
	/** page number */
	unsigned int pages_nr;
	/** scatter gather list */
	struct qdma_sw_sg *sgl;
	/** pages allocated to accommodate the scatter gather list */
	struct page **pages;
	/** qdma request */
	struct qdma_request req;
};

/*****************************************************************************/
/**
 * qdma_cdev_destroy() - handler to destroy the character device
 *
 * @param[in]	xcdev: pointer to character device
 *
 * @return	none
 *****************************************************************************/
void qdma_cdev_destroy(struct qdma_cdev *xcdev);

/*****************************************************************************/
/**
 * qdma_cdev_create() - handler to create a character device
 *
 * @param[in]	xcb:	pointer to qdma character device call back data
 * @param[in]	pdev: pointer to struct pci_dev
 * @param[in]	qconf: queue configurations
 * @param[in]	minor: character device minor number
 * @param[in]	ebuflen:	buffer length
 * @param[in]	qhndl: queue handle
 * @param[out]	xcdev_pp: pointer to struct qdma_cdev
 * @param[out]	ebuf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_cdev_create(struct qdma_cdev_cb *xcb, struct pci_dev *pdev,
			struct qdma_queue_conf *qconf, unsigned int minor,
			unsigned long qhndl, struct qdma_cdev **xcdev_pp,
			char *ebuf, int ebuflen);

/*****************************************************************************/
/**
 * qdma_cdev_device_cleanup() - handler to clean up a character device
 *
 * @param[in]	xcb: pointer to qdma character device call back data
 *
 * @return	none
 *****************************************************************************/
void qdma_cdev_device_cleanup(struct qdma_cdev_cb *xcb);

/*****************************************************************************/
/**
 * qdma_cdev_device_init() - handler to initialize a character device
 *
 * @param[in]	xcb: pointer to qdma character device call back data
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_cdev_device_init(struct qdma_cdev_cb *xcb);

/*****************************************************************************/
/**
 * qdma_cdev_cleanup() - character device cleanup handler
 *
 *****************************************************************************/
void qdma_cdev_cleanup(void);

/*****************************************************************************/
/**
 * qdma_cdev_init() - character device initialization handler
 *
 *****************************************************************************/
int qdma_cdev_init(void);

#endif /* ifndef __QDMA_CDEV_H__ */
