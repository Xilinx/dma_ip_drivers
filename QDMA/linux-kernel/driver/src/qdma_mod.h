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

#ifndef __QDMA_MODULE_H__
#define __QDMA_MODULE_H__
/**
 * @file
 * @brief This file contains the declarations for qdma pcie kernel module
 *
 */
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <net/genetlink.h>

#include "libqdma/libqdma_export.h"
#include "cdev.h"


/**
 * enum qdma_drv_mod_param_type - Indicate the module parameter type
 *
 * QDMA PF/VF drivers takes module parameters mode and config bar is the same
 * format, This enum is unsed to identify the parameter type
 *
 */
enum qdma_drv_mod_param_type {
	/** @DRV_MODE : Driver mode mod param */
	DRV_MODE,
	/** @CONFIG_BAR : Config Bar mod param */
	CONFIG_BAR,
	/** @MASTER_PF : Master PF mod param */
	MASTER_PF,
};

/**
 * @struct - xlnx_qdata
 * @brief	queue data variables send while read/write request
 */
struct xlnx_qdata {
	unsigned long qhndl;		/**< Queue handle */
	struct qdma_cdev *xcdev;	/**< qdma character device details */
};

#define XNL_EBUFLEN     256
struct xlnx_nl_work_q_ctrl {
	unsigned short qidx;
	unsigned short qcnt;
	u8 is_qp:1;
	u8 q_type:2;
};

struct xlnx_nl_work {
	struct work_struct work;
	struct xlnx_pci_dev *xpdev;
	wait_queue_head_t wq;
	unsigned int q_start_handled;
	unsigned int buflen;
	char *buf;
	struct xlnx_nl_work_q_ctrl qctrl;
	int ret;
};

/**
 * @struct - xlnx_pci_dev
 * @brief	xilinx pcie device data members
 */
struct xlnx_pci_dev {
	struct list_head list_head;	/**< device list */
	struct pci_dev *pdev;		/**< pointer to struct pci_dev */
	unsigned long dev_hndl;		/**< device handle*/
	struct workqueue_struct *nl_task_wq; /**< netlink request work queue */
	struct qdma_cdev_cb cdev_cb;	/**< character device call back data*/
	spinlock_t cdev_lock;		/**< character device lock*/
	unsigned int qmax;		/**< max number of queues for device*/
	unsigned int idx;		/**< device index*/
	void __iomem *user_bar_regs;	/**< PCIe AXI Master Lite bar */
	void __iomem *bypass_bar_regs;  /**< PCIe AXI Bridge Master bar*/
	struct xlnx_qdata *qdata;	/**< queue data*/
};

/*****************************************************************************/
/**
 * xpdev_list_dump() - list the qdma devices
 *
 * @param[in]	buflen:		buffer length
 * @param[out]	buf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xpdev_list_dump(char *buf, int buflen);

/*****************************************************************************/
/**
 * xpdev_find_by_idx() - qdma pcie kernel module api to
 *						find the qdma device by index
 *
 * @param[in]	idx:		qdma device index
 * @param[in]	buflen:		buffer length
 * @param[out]	buf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: pointer to xlnx_pci_dev
 * @return	NULL: failure
 *****************************************************************************/
struct xlnx_pci_dev *xpdev_find_by_idx(unsigned int idx, char *buf,
			int buflen);

/*****************************************************************************/
/**
 * xpdev_queue_get() - qdma pcie kernel module api to get a queue information
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	qidx:		queue index
 * @param[in]	c2h:		flag to indicate the queue direction (c2h/h2c)
 * @param[in]	check_qhndl:	flag for validating the data
 * @param[in]	ebuflen:	buffer length
 * @param[out]	ebuf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: queue information
 * @return	NULL: failure
 *****************************************************************************/
struct xlnx_qdata *xpdev_queue_get(struct xlnx_pci_dev *xpdev,
			unsigned int qidx, u8 q_type, bool check_qhndl,
			char *ebuf, int ebuflen);

/*****************************************************************************/
/**
 * xpdev_queue_add() - qdma pcie kernel module api to add a queue
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	qconf:		queue configuration
 * @param[in]	ebuflen:	buffer length
 * @param[out]	ebuf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xpdev_queue_add(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf,
			char *ebuf, int ebuflen);

/*****************************************************************************/
/**
 * xpdev_queue_delete() - qdma pcie kernel module api to delete a queue
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	qidx:		queue index
 * @param[in]	c2h:		flag to indicate the queue direction (c2h/h2c)
 * @param[in]	ebuflen:	buffer length
 * @param[out]	ebuf:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xpdev_queue_delete(struct xlnx_pci_dev *xpdev, unsigned int qidx,
		u8 q_type, char *ebuf, int ebuflen);

int xpdev_nl_queue_start(struct xlnx_pci_dev *xpdev, void *nl_info, u8 is_qp,
			u8 q_type, unsigned short qidx, unsigned short qcnt);

/*****************************************************************************/
/**
 * qdma_device_read_user_register() - read AXI Master Lite bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[out]	value:	pointer to hold the register value
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_read_user_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 *value);

/*****************************************************************************/
/**
 * qdma_device_write_user_register() - write AXI Master Lite bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[in]	value:		register value to be written
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_write_user_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 value);

/*****************************************************************************/
/**
 * qdma_device_read_bypass_register() - read AXI Bridge Master bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[out]	value:	pointer to hold the register value
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_read_bypass_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 *value);

/*****************************************************************************/
/**
 * qdma_device_write_bypass_register() - write AXI Bridge Master bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[in]	value:		register value to be written
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_device_write_bypass_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 value);

#endif /* ifndef __QDMA_MODULE_H__ */
