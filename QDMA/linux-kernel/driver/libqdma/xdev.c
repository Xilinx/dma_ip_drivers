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

/**
 * @file
 * @brief This file contains the declarations for QDMA PCIe device
 *
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

#include "qdma_regs.h"
#include "xdev.h"
#include "qdma_mbox.h"
#include "qdma_intr.h"
#include "qdma_resource_mgmt.h"
#include "qdma_access_common.h"
#ifdef DEBUGFS
#include "qdma_debugfs_dev.h"
#endif

#ifdef __LIST_NEXT_ENTRY__
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)
#endif

#ifndef __QDMA_VF__
#ifndef QDMA_QBASE
#define QDMA_QBASE 0
#endif
#ifndef QDMA_TOTAL_Q
#define QDMA_TOTAL_Q 2048
#endif
#endif

/**
 * qdma device management
 * maintains a list of the qdma devices
 */
static LIST_HEAD(xdev_list);

/**
 * mutex defined for qdma device management
 */
static DEFINE_MUTEX(xdev_mutex);

#ifndef list_last_entry
#define list_last_entry(ptr, type, member) \
		list_entry((ptr)->prev, type, member)
#endif

struct qdma_resource_lock {
	struct list_head node;
	struct mutex lock;
};

/*****************************************************************************/
/**
 * pci_dma_mask_set() - check the pci capability of the dma device
 *
 * @param[in]	pdev:	pointer to struct pci_dev
 *
 *
 * @return	0: on success
 * @return	<0: on failure
 *****************************************************************************/
static int pci_dma_mask_set(struct pci_dev *pdev)
{
	/** 64-bit addressing capability for XDMA? */
	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		/** use 64-bit DMA for descriptors */
		pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
		/** use 64-bit DMA, 32-bit for consistent */
	} else if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
		pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
		/** use 32-bit DMA */
		dev_info(&pdev->dev, "Using a 32-bit DMA mask.\n");
	} else {
		/** use 32-bit DMA */
		dev_info(&pdev->dev, "No suitable DMA possible.\n");
		return -EINVAL;
	}

	return 0;
}

#if KERNEL_VERSION(3, 5, 0) <= LINUX_VERSION_CODE
static void pci_enable_relaxed_ordering(struct pci_dev *pdev)
{
	pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_RELAX_EN);
}

static void pci_disable_relaxed_ordering(struct pci_dev *pdev)
{
	pcie_capability_clear_word(pdev, PCI_EXP_DEVCTL,
			PCI_EXP_DEVCTL_RELAX_EN);
}

static void pci_enable_extended_tag(struct pci_dev *pdev)
{
	pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_EXT_TAG);
}

static void pci_disable_extended_tag(struct pci_dev *pdev)
{
	pcie_capability_clear_word(pdev, PCI_EXP_DEVCTL,
			PCI_EXP_DEVCTL_EXT_TAG);
}

#else
static void pci_enable_relaxed_ordering(struct pci_dev *pdev)
{
	u16 v;
	int pos;

	pos = pci_pcie_cap(pdev);
	if (pos > 0) {
		pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &v);
		v |= PCI_EXP_DEVCTL_RELAX_EN;
		pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, v);
	}
}

static void pci_disable_relaxed_ordering(struct pci_dev *pdev)
{
	u16 v;
	int pos;

	pos = pci_pcie_cap(pdev);
	if (pos > 0) {
		pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &v);
		v &= ~(PCI_EXP_DEVCTL_RELAX_EN);
		pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, v);
	}
}

static void pci_enable_extended_tag(struct pci_dev *pdev)
{
	u16 v;
	int pos;

	pos = pci_pcie_cap(pdev);
	if (pos > 0) {
		pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &v);
		v |= PCI_EXP_DEVCTL_EXT_TAG;
		pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, v);
	}
}

static void pci_disable_extended_tag(struct pci_dev *pdev)
{
	u16 v;
	int pos;

	pos = pci_pcie_cap(pdev);
	if (pos > 0) {
		pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &v);
		v &= ~(PCI_EXP_DEVCTL_EXT_TAG);
		pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, v);
	}
}
#endif


#if defined(__QDMA_VF__)
static void xdev_reset_work(struct work_struct *work)
{
	struct xlnx_dma_dev *xdev = container_of(work, struct xlnx_dma_dev,
								reset_work);
	struct pci_dev *pdev = xdev->conf.pdev;
	int rv = 0;

	if (xdev->reset_state == RESET_STATE_RECV_PF_RESET_REQ) {

		qdma_device_offline(pdev, (unsigned long)xdev, XDEV_FLR_ACTIVE);
		pci_disable_extended_tag(pdev);
		pci_disable_relaxed_ordering(pdev);
		pci_release_regions(pdev);
		pci_disable_device(pdev);

		rv = pci_request_regions(pdev, "qdma-vf");
		if (rv) {
			pr_err("cannot obtain PCI resources\n");
			return;
		}

		rv = pci_enable_device(pdev);
		if (rv) {
			pr_err("cannot enable PCI device\n");
			pci_release_regions(pdev);
			return;
		}

		/* enable relaxed ordering */
		pci_enable_relaxed_ordering(pdev);

		/* enable extended tag */
		pci_enable_extended_tag(pdev);

		/* enable bus master capability */
		pci_set_master(pdev);

		pci_dma_mask_set(pdev);

		pcie_set_readrq(pdev, 512);

		qdma_device_online(pdev, (unsigned long)xdev, XDEV_FLR_ACTIVE);

		if (xdev->reset_state == RESET_STATE_RECV_PF_RESET_DONE)
			xdev->reset_state = RESET_STATE_IDLE;
	}  else if (xdev->reset_state == RESET_STATE_RECV_PF_OFFLINE_REQ) {
		qdma_device_offline(pdev, (unsigned long)xdev,
							XDEV_FLR_INACTIVE);
	}

}
#endif

/*****************************************************************************/
/**
 * xdev_list_first() - handler to return the first xdev entry from the list
 *
 * @return	pointer to first xlnx_dma_dev on success
 * @return	NULL on failure
 *****************************************************************************/
struct xlnx_dma_dev *xdev_list_first(void)
{
	struct xlnx_dma_dev *xdev;

	mutex_lock(&xdev_mutex);
	xdev = list_first_entry(&xdev_list, struct xlnx_dma_dev, list_head);
	mutex_unlock(&xdev_mutex);

	return xdev;
}

/*****************************************************************************/
/**
 * xdev_list_next() - handler to return the next xdev entry from the list
 *
 * @param[in]	xdev:	pointer to current xdev
 *
 * @return	pointer to next xlnx_dma_dev on success
 * @return	NULL on failure
 *****************************************************************************/
struct xlnx_dma_dev *xdev_list_next(struct xlnx_dma_dev *xdev)
{
	struct xlnx_dma_dev *next;

	mutex_lock(&xdev_mutex);
	next = list_next_entry(xdev, list_head);
	mutex_unlock(&xdev_mutex);

	return next;
}

/*****************************************************************************/
/**
 * xdev_list_dump() - list the dma device details
 *
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	pointer to next xlnx_dma_dev on success
 * @return	NULL on failure
 *****************************************************************************/
int xdev_list_dump(char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev, *tmp;
	int len = 0;

	mutex_lock(&xdev_mutex);
	list_for_each_entry_safe(xdev, tmp, &xdev_list, list_head) {
		len += snprintf(buf + len, buflen - len,
				"qdma%05x\t%02x:%02x.%02x\n",
				xdev->conf.bdf, xdev->conf.pdev->bus->number,
				PCI_SLOT(xdev->conf.pdev->devfn),
				PCI_FUNC(xdev->conf.pdev->devfn));
		if (len >= buflen)
			break;
	}
	mutex_unlock(&xdev_mutex);

	buf[len] = '\0';
	return len;
}

/*****************************************************************************/
/**
 * xdev_list_add() - add a new node to the xdma device lsit
 *
 * @param[in]	xdev:	pointer to current xdev
 *
 * @return	none
 *****************************************************************************/
static inline void xdev_list_add(struct xlnx_dma_dev *xdev)
{
	u32 bdf = 0;
	struct xlnx_dma_dev *_xdev, *tmp;
	u32 last_bus = 0;
	u32 last_dev = 0;

	mutex_lock(&xdev_mutex);
	bdf = ((xdev->conf.pdev->bus->number << PCI_SHIFT_BUS) |
			(PCI_SLOT(xdev->conf.pdev->devfn) << PCI_SHIFT_DEV) |
			PCI_FUNC(xdev->conf.pdev->devfn));
	xdev->conf.bdf = bdf;
	list_add_tail(&xdev->list_head, &xdev_list);

	/*
	 * Iterate through the list of devices. Increment cfg_done, to
	 * get the mulitplier for initial configuration of queues. A
	 * '0' indicates queue is already configured. < 0, indicates
	 * config done using sysfs entry
	 */

	list_for_each_entry_safe(_xdev, tmp, &xdev_list, list_head) {

		/*are we dealing with a different card?*/
#ifdef __QDMA_VF__
		/** for VF check only bus number, as dev number can change
		 * in a single card
		 */
		if (last_bus != _xdev->conf.pdev->bus->number)
#else
		if ((last_bus != _xdev->conf.pdev->bus->number) ||
				(last_dev != PCI_SLOT(_xdev->conf.pdev->devfn)))
#endif
			xdev->conf.idx = 0;
		xdev->conf.idx++;
		last_bus = _xdev->conf.pdev->bus->number;
		last_dev = PCI_SLOT(xdev->conf.pdev->devfn);
	}
	mutex_unlock(&xdev_mutex);
}


#undef list_last_entry
/*****************************************************************************/
/**
 * xdev_list_add() - remove a node from the xdma device lsit
 *
 * @param[in]	xdev:	pointer to current xdev
 *
 * @return	none
 *****************************************************************************/
static inline void xdev_list_remove(struct xlnx_dma_dev *xdev)
{
	mutex_lock(&xdev_mutex);
	list_del(&xdev->list_head);
	mutex_unlock(&xdev_mutex);
}

/*****************************************************************************/
/**
 * xdev_find_by_pdev() - find the xdev using struct pci_dev
 *
 * @param[in]	pdev:	pointer to struct pci_dev
 *
 * @return	pointer to xlnx_dma_dev on success
 * @return	NULL on failure
 *****************************************************************************/
struct xlnx_dma_dev *xdev_find_by_pdev(struct pci_dev *pdev)
{
	struct xlnx_dma_dev *xdev, *tmp;

	mutex_lock(&xdev_mutex);
	list_for_each_entry_safe(xdev, tmp, &xdev_list, list_head) {
		if (xdev->conf.pdev == pdev) {
			mutex_unlock(&xdev_mutex);
			return xdev;
		}
	}
	mutex_unlock(&xdev_mutex);
	return NULL;
}

/*****************************************************************************/
/**
 * xdev_find_by_idx() - find the xdev using the index value
 *
 * @param[in]	idx:	index value in the xdev list
 *
 * @return	pointer to xlnx_dma_dev on success
 * @return	NULL on failure
 *****************************************************************************/
struct xlnx_dma_dev *xdev_find_by_idx(int idx)
{
	struct xlnx_dma_dev *xdev, *tmp;

	mutex_lock(&xdev_mutex);
	list_for_each_entry_safe(xdev, tmp, &xdev_list, list_head) {
		if (xdev->conf.bdf == idx) {
			mutex_unlock(&xdev_mutex);
			return xdev;
		}
	}
	mutex_unlock(&xdev_mutex);
	return NULL;
}

/*****************************************************************************/
/**
 * xdev_check_hndl() - helper function to validate the device handle
 *
 * @param[in]	pdev:	pointer to struct pci_dev
 * @param[in]	hndl:	device handle
 *
 * @return	0: success
 * @return	<0: on failure
 *****************************************************************************/
int xdev_check_hndl(const char *fname, struct pci_dev *pdev, unsigned long hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)hndl;

	if (!pdev)
		return -EINVAL;

	if (xdev->magic != QDMA_MAGIC_DEVICE) {
		pr_err("%s xdev->magic %ld  != %ld\n",
			fname, xdev->magic, QDMA_MAGIC_DEVICE);
		return -EINVAL;
	}

	if (xdev->conf.pdev != pdev) {
		pr_err("pci_dev(0x%lx) != pdev(0x%lx)\n",
				(unsigned long)xdev->conf.pdev,
				(unsigned long)pdev);
		return -EINVAL;
	}

	return 0;
}

/**********************************************************************
 * PCI-level Functions
 **********************************************************************/

/*****************************************************************************/
/**
 * xdev_unmap_bars() - Unmap the BAR regions that had been mapped
 *						earlier using map_bars()
 *
 * @param[in]	xdev:	pointer to current xdev
 * @param[in]	pdev:	pointer to struct pci_dev
 *
 * @return	none
 *****************************************************************************/
static void xdev_unmap_bars(struct xlnx_dma_dev *xdev, struct pci_dev *pdev)
{
	if (xdev->regs) {
		/* unmap BAR */
		pci_iounmap(pdev, xdev->regs);
		/* mark as unmapped */
		xdev->regs = NULL;
	}
}

/*****************************************************************************/
/**
 * xdev_map_bars() - map device regions into kernel virtual address space
 *						earlier using map_bars()
 *
 * @param[in]	xdev:	pointer to current xdev
 * @param[in]	pdev:	pointer to struct pci_dev
 *
 * Map the device memory regions into kernel virtual address space after
 * verifying their sizes respect the minimum sizes needed
 *
 * @return	length of the bar on success
 * @return	0 on failure
 *****************************************************************************/
static int xdev_map_bars(struct xlnx_dma_dev *xdev, struct pci_dev *pdev)
{
	int map_len;

	map_len = pci_resource_len(pdev, (int)xdev->conf.bar_num_config);
	if (map_len > QDMA_MAX_BAR_LEN_MAPPED)
		map_len = QDMA_MAX_BAR_LEN_MAPPED;

	xdev->regs = pci_iomap(pdev, xdev->conf.bar_num_config, map_len);
	if (!xdev->regs ||
		map_len < QDMA_MIN_BAR_LEN_MAPPED) {
		pr_err("%s unable to map config bar %d.\n", xdev->conf.name,
				xdev->conf.bar_num_config);
		return -EINVAL;
	}

	return 0;
}

/*****************************************************************************/
/**
 * xdev_identify_bars() - identifies the AXI Master Lite bar
 *			and AXI Bridge Master bar
 *
 * @param[in]	xdev:	pointer to current xdev
 * @param[in]	pdev:	pointer to struct pci_dev\
 *
 * @return	0 on success, -ve on failure
 *****************************************************************************/
static int xdev_identify_bars(struct xlnx_dma_dev *xdev, struct pci_dev *pdev)
{
	int bar_idx = 0;
	u8 num_bars_present = 0;
	int bar_id_list[QDMA_BAR_NUM];
	int bar_id_idx = 0;

	/* Find out the number of bars present in the design */
	for (bar_idx = 0; bar_idx < QDMA_BAR_NUM; bar_idx++) {
		int map_len = 0;

		map_len = pci_resource_len(pdev, bar_idx);
		if (!map_len)
			continue;

		bar_id_list[bar_id_idx] = bar_idx;
		bar_id_idx++;
		num_bars_present++;
	}

	if (num_bars_present > 1) {
		int rv = 0;

		/* AXI Master Lite BAR IDENTIFICATION */
		if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
			xdev->conf.bar_num_user = DEFAULT_USER_BAR;
		else {
#ifndef __QDMA_VF__
			rv = xdev->hw.qdma_get_user_bar(xdev, 0,
					xdev->func_id,
					(uint8_t *)&xdev->conf.bar_num_user);
#else
			rv = xdev->hw.qdma_get_user_bar(xdev, 1,
					xdev->func_id_parent,
					(uint8_t *)&xdev->conf.bar_num_user);
#endif
		}

		if (rv < 0) {
			pr_err("get AXI Master Lite bar failed with error = %d",
					rv);
			return xdev->hw.qdma_get_error_code(rv);
		}

		pr_info("AXI Master Lite BAR %d.\n",
				xdev->conf.bar_num_user);

		/* AXI Bridge Master BAR IDENTIFICATION */
		if (num_bars_present > 2) {
			for (bar_idx = 0; bar_idx < num_bars_present;
								bar_idx++) {
				if ((bar_id_list[bar_idx] !=
						xdev->conf.bar_num_user) &&
						(bar_id_list[bar_idx] !=
						xdev->conf.bar_num_config)) {
					xdev->conf.bar_num_bypass =
						bar_id_list[bar_idx];
					pr_info("AXI Bridge Master BAR %d.\n",
						xdev->conf.bar_num_bypass);
					break;
				}
			}
		}
	}
	return 0;
}

/*****************************************************************************/
/**
 * xdev_map_bars() - allocate the dma device
 *
 * @param[in]	conf:	qdma device configuration
 *
 *
 * @return	pointer to dma device
 * @return	NULL on failure
 *****************************************************************************/
static struct xlnx_dma_dev *xdev_alloc(struct qdma_dev_conf *conf)
{
	struct xlnx_dma_dev *xdev;

	/* allocate zeroed device book keeping structure */
	xdev = kzalloc(sizeof(struct xlnx_dma_dev), GFP_KERNEL);
	if (!xdev)
		return NULL;

	spin_lock_init(&xdev->hw_prg_lock);
	spin_lock_init(&xdev->lock);

	/* create a driver to device reference */
	memcpy(&xdev->conf, conf, sizeof(*conf));

	xdev->magic = QDMA_MAGIC_DEVICE;

	/* !! FIXME default to enabled for everything */
	xdev->dev_cap.flr_present = 1;
	xdev->dev_cap.st_en = 1;
	xdev->dev_cap.mm_en = 1;
	xdev->dev_cap.mm_channel_max = 1;

	return xdev;
}

#ifndef __QDMA_VF__
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
		xdev->hw.qdma_hw_error_process(xdev);
		schedule_delayed_work(dwork, msecs_to_jiffies(1000));/* 1 sec */
	}
	spin_unlock(&xdev->err_lock);
}
#endif



/*****************************************************************************/
/**
 * qdma_device_offline() - set the dma device in offline mode
 *
 * @param[in]	pdev:		pointer to struct pci_dev
 * @param[in]	dev_hndl:	device handle
 * @param[in]	reset:		0/1 function level reset active or not
 *
 * @return	0: on success
 * @return	<0: on failure
 *****************************************************************************/
int qdma_device_offline(struct pci_dev *pdev, unsigned long dev_hndl,
						 int reset)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	if (xdev->conf.pdev != pdev) {
		pr_err("Invalid pdev passed: pci_dev(0x%lx) != pdev(0x%lx)\n",
			(unsigned long)xdev->conf.pdev, (unsigned long)pdev);
		return -EINVAL;
	}

	if (xlnx_dma_device_flag_check(xdev, XDEV_FLAG_OFFLINE)) {
#ifdef __QDMA_VF__
		if (xdev->workq != NULL) {
			pr_debug("destroy workq\n");
			destroy_workqueue(xdev->workq);
			xdev->workq = NULL;
		}
#endif
		return 0;
	}

#ifdef __QDMA_VF__
	if (xdev->reset_state == RESET_STATE_PF_OFFLINE_REQ_PROCESSING) {
		int retry_cnt = 10;

		while (!xlnx_dma_device_flag_check(xdev, XDEV_FLAG_OFFLINE)) {
			mdelay(100);
			if (retry_cnt == 0)
				break;
			retry_cnt--;
		}
		if (xdev->workq != NULL) {
			destroy_workqueue(xdev->workq);
			xdev->workq = NULL;
		}
		return 0;
	}
#endif
	/* Canceling the error poll thread which was started
	 * in the poll mode
	 */
#ifndef __QDMA_VF__
	if ((xdev->conf.master_pf) &&
		(xdev->conf.qdma_drv_mode == POLL_MODE)) {
		pr_debug("Cancelling delayed work");
		spin_lock(&xdev->err_lock);
		xdev->err_mon_cancel = 1;
		cancel_delayed_work_sync(&xdev->err_mon);
		spin_unlock(&xdev->err_lock);
	}
#endif

	qdma_device_cleanup(xdev);
	qdma_device_interrupt_cleanup(xdev);
	qdma_mbox_stop(xdev);
	intr_teardown(xdev);
	xdev->flags &= ~(XDEV_FLAG_IRQ);

	/*
	 * When the FLR is done to parent PF , it's associated VFs
	 * and it's resources are no more active. The
	 * interrupt state of the VF goes bad. That's why switching
	 * from mbox's interrupt mode to poll mode
	 */
	qdma_mbox_poll_start(xdev);
#ifdef __QDMA_VF__
	if (reset) {
		if (xdev->reset_state == RESET_STATE_RECV_PF_RESET_REQ) {

			xdev_sriov_vf_reset_offline(xdev);

			/** Wait for the PF to send the PF Reset Done*/
			qdma_waitq_wait_event_timeout(xdev->wq,
				(xdev->reset_state ==
				 RESET_STATE_RECV_PF_RESET_DONE),
				10 * QDMA_MBOX_MSG_TIMEOUT_MS);

			if (xdev->reset_state != RESET_STATE_RECV_PF_RESET_DONE)
				xdev->reset_state = RESET_STATE_INVALID;
		} else
			xdev_sriov_vf_offline(xdev, 0);

	} else {
		if (xdev->reset_state == RESET_STATE_RECV_PF_OFFLINE_REQ) {
			xdev_sriov_vf_offline(xdev, 0);
			xdev->reset_state =
				RESET_STATE_PF_OFFLINE_REQ_PROCESSING;
		} else {
			xdev_sriov_vf_offline(xdev, 0);
			destroy_workqueue(xdev->workq);
			xdev->workq = NULL;
		}
	}
	qdma_mbox_stop(xdev);
#elif defined(CONFIG_PCI_IOV)
	if (!reset) {
		qdma_pf_trigger_vf_offline((unsigned long)xdev);
		xdev_sriov_disable(xdev);
	} else if (xdev->vf_count_online != 0) {
		qdma_pf_trigger_vf_reset((unsigned long)xdev);
		qdma_mbox_stop(xdev);
	}

#endif

	if (reset) {
		/* Free the allocated resources if FLR process running*/
		if (xdev->conf.fp_flr_free_resource)
			xdev->conf.fp_flr_free_resource((unsigned long)xdev);
	}

	if (xdev->conf.qdma_drv_mode != POLL_MODE)
		xdev->mbox.rx_poll = 0;

	xdev_flag_set(xdev, XDEV_FLAG_OFFLINE);
	if (xdev->dev_cap.mailbox_en)
		qdma_mbox_cleanup(xdev);

	return 0;
}

/*****************************************************************************/
/**
 * qdma_device_online() - set the dma device in online mode
 *
 * @param[in]	pdev:		pointer to struct pci_dev
 * @param[in]	dev_hndl:	device handle
 * @param[in]	reset:		0/1 function level reset active or not
 *
 * @return	0: on success
 * @return	<0: on failure
 *****************************************************************************/
int qdma_device_online(struct pci_dev *pdev, unsigned long dev_hndl, int reset)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv;
#if !defined(__QDMA_VF__)
	struct qdma_vf_info *vf;
#endif
	if (!xdev) {
		pr_err("Invalid device handle received");
		return -EINVAL;
	}


	if (xdev_check_hndl(__func__, pdev, dev_hndl) < 0) {
		pr_err("Invalid device");
		return -EINVAL;
	}

	if (xdev->conf.pdev != pdev) {
		pr_err("pci_dev(0x%lx) != pdev(0x%lx)\n",
			(unsigned long)xdev->conf.pdev, (unsigned long)pdev);
	}

#if defined(__QDMA_VF__)
	pr_info("reset_state = %d", xdev->reset_state);
	if (reset && xdev->reset_state == RESET_STATE_INVALID) {
		pr_info("returning");
		return -EINVAL;
	}
#endif

	if (xdev->conf.qdma_drv_mode != POLL_MODE &&
			xdev->conf.qdma_drv_mode != LEGACY_INTR_MODE) {

		if ((xdev->flags & XDEV_FLAG_IRQ) == 0x0) {
			rv = intr_setup(xdev);
			if (rv) {
				pr_err("Failed to setup interrupts, err %d",
					rv);
				return -EINVAL;
			}
		}
		xdev->flags |= XDEV_FLAG_IRQ;
	}

#ifndef __QDMA_VF__
	if (xdev->dev_cap.mailbox_en)
		qdma_mbox_init(xdev);
#else
	qdma_mbox_init(xdev);
	if (!reset) {
		qdma_waitq_init(&xdev->wq);
		INIT_WORK(&xdev->reset_work, xdev_reset_work);
		xdev->workq = create_singlethread_workqueue("Reset Work Queue");
	}
#endif
	rv = qdma_device_init(xdev);
	if (rv < 0) {
		pr_warn("qdma_init failed %d.\n", rv);
		return rv;
	}
	xdev_flag_clear(xdev, XDEV_FLAG_OFFLINE);
#ifdef __QDMA_VF__
	qdma_mbox_start(xdev);
	/* PF mbox will start when vf > 0 */
	rv = xdev_sriov_vf_online(xdev, 0);
	if (rv < 0)
		return rv;
#endif
	rv = qdma_device_interrupt_setup(xdev);
	if (rv < 0) {
		pr_err("Failed to setup device interrupts");
		return rv;
	}

	/* Starting a error poll thread in Poll mode */
#ifndef __QDMA_VF__
	if ((xdev->conf.master_pf) &&
			(xdev->conf.qdma_drv_mode == POLL_MODE)) {

		rv = xdev->hw.qdma_hw_error_enable(xdev,
				xdev->hw.qdma_max_errors);
		if (rv < 0) {
			pr_err("Failed to enable error interrupts");
			return -EINVAL;
		}

		spin_lock_init(&xdev->err_lock);
		xdev->err_mon_cancel = 0;
		INIT_DELAYED_WORK(&xdev->err_mon, qdma_err_mon);
		schedule_delayed_work(&xdev->err_mon,
				      msecs_to_jiffies(1000));
	}

	/**
	 * Send the RESET_DONE message to VF
	 */
	if (reset && xdev->vf_count != 0) {
		int i = 0;

		vf = (struct qdma_vf_info *)xdev->vf_info;

		if (!vf) {
			pr_err("Invalid vf handle received");
			return -EINVAL;
		}

		qdma_mbox_start(xdev);
		for (i = 0; i < xdev->vf_count; i++) {
			struct mbox_msg *m = NULL;
			u8 vf_count_online = xdev->vf_count_online;

			m = qdma_mbox_msg_alloc();
			if (!m) {
				pr_err("Failed to allocate mbox msg\n");
				return -ENOMEM;
			}
			qdma_mbox_compose_pf_reset_done_message(m->raw,
						xdev->func_id, vf[i].func_id);
			qdma_mbox_msg_send(xdev, m, 1,
						QDMA_MBOX_MSG_TIMEOUT_MS);

			qdma_waitq_wait_event_timeout(xdev->wq,
				(xdev->vf_count_online ==
				(vf_count_online + 1)),
				QDMA_MBOX_MSG_TIMEOUT_MS);
		}
	}

#endif

	return 0;
}

/*****************************************************************************/
/**
 * qdma_device_open() - open the dma device
 *
 * @param[in]	mod_name:	name of the dma device
 * @param[in]	conf:		device configuration
 * @param[in]	dev_hndl:	device handle
 *
 *
 * @return	0: on success
 * @return	<0: on failure
 *****************************************************************************/
int qdma_device_open(const char *mod_name, struct qdma_dev_conf *conf,
			unsigned long *dev_hndl)
{
	struct pci_dev *pdev = NULL;
	struct xlnx_dma_dev *xdev = NULL;
	int rv = 0;
#ifndef __QDMA_VF__
	int qbase = QDMA_QBASE;
	int qmax = QDMA_TOTAL_Q;
#endif

	*dev_hndl = 0UL;

	if (!mod_name) {
		pr_err("%s: mod_name is NULL.\n", __func__);
		return -EINVAL;
	}

	if (!conf) {
		pr_err("%s: queue_conf is NULL.\n", mod_name);
		return -EINVAL;
	}

	if (conf->qdma_drv_mode > LEGACY_INTR_MODE) {
		pr_err("%s: driver mode passed in Invalid.\n", mod_name);
		return -EINVAL;
	}

	pdev = conf->pdev;

	if (!pdev) {
		pr_err("%s: pci device NULL.\n", mod_name);
		return -EINVAL;
	}
	pr_info("%s, %02x:%02x.%02x, pdev 0x%p, 0x%x:0x%x.\n",
		mod_name, pdev->bus->number, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn), pdev, pdev->vendor, pdev->device);

	xdev = xdev_find_by_pdev(pdev);
	if (xdev) {
		pr_warn("%s, device %s already attached!\n",
			mod_name, dev_name(&pdev->dev));
		return -EINVAL;
	}

	rv = pci_request_regions(pdev, mod_name);
	if (rv) {
		/* Just info, some other driver may have claimed the device. */
		dev_info(&pdev->dev, "cannot obtain PCI resources\n");
		return rv;
	}

	rv = pci_enable_device(pdev);
	if (rv) {
		dev_err(&pdev->dev, "cannot enable PCI device\n");
		goto release_regions;
	}

	/* enable relaxed ordering */
	pci_enable_relaxed_ordering(pdev);

	/* enable extended tag */
	pci_enable_extended_tag(pdev);

	/* enable bus master capability */
	pci_set_master(pdev);

	rv = pci_dma_mask_set(pdev);
	if (rv) {
		pr_err("Failed to set the dma mask");
		goto disable_device;
	}

	if (pcie_get_readrq(pdev) < 512)
		pcie_set_readrq(pdev, 512);

	/* allocate zeroed device book keeping structure */
	xdev = xdev_alloc(conf);
	if (!xdev) {
		pr_err("Failed to allocate xdev");
		goto disable_device;
	}

	strncpy(xdev->mod_name, mod_name, QDMA_DEV_NAME_MAXLEN - 1);

	xdev_flag_set(xdev, XDEV_FLAG_OFFLINE);
	xdev_list_add(xdev);

	rv = snprintf(xdev->conf.name, QDMA_DEV_NAME_MAXLEN,
		"qdma%05x-p%s",
		xdev->conf.bdf, dev_name(&xdev->conf.pdev->dev));
	xdev->conf.name[rv] = '\0';

	/* Mapping bars */
	rv = xdev_map_bars(xdev, pdev);
	if (rv) {
		pr_err("Failed to map the bars");
		goto unmap_bars;
	}

	/* Get HW access */
#ifndef __QDMA_VF__
	rv = qdma_hw_access_init(xdev, 0, &xdev->hw);
	if (rv != QDMA_SUCCESS) {
		rv = -EINVAL;
		goto unmap_bars;
	}

	rv = xdev->hw.qdma_get_version(xdev, 0, &xdev->version_info);
	if (rv != QDMA_SUCCESS) {
		rv = xdev->hw.qdma_get_error_code(rv);
		pr_err("Failed to get the HW Version");
		goto unmap_bars;
	}

	/* get the device attributes */
	qdma_device_attributes_get(xdev);
	if (pdev->bus->parent)
		rv = qdma_master_resource_create(pdev->bus->number,
				pci_bus_max_busnr(pdev->bus->parent), qbase,
				qmax, &xdev->dma_device_index);
	else
		rv = qdma_master_resource_create(pdev->bus->number,
				pdev->bus->number, qbase,
				qmax, &xdev->dma_device_index);

	if (rv == -QDMA_ERR_NO_MEM) {
		pr_err("master_resource_create failed, err = %d", rv);
		rv = -ENOMEM;
		goto unmap_bars;
	}
#else
	rv = qdma_hw_access_init(xdev, 1, &xdev->hw);
	if (rv != QDMA_SUCCESS)
		goto unmap_bars;
	rv = xdev->hw.qdma_get_version(xdev, 1, &xdev->version_info);
	if (rv != QDMA_SUCCESS)
		goto unmap_bars;
#endif

	pr_info("Vivado version = %s\n",
			xdev->version_info.qdma_vivado_release_id_str);

#ifndef __QDMA_VF__
	rv = xdev->hw.qdma_get_function_number(xdev, &xdev->func_id);
	if (rv < 0) {
		pr_err("get function number failed, err = %d", rv);
		rv = -EINVAL;
		goto unmap_bars;
	}

	rv = qdma_dev_qinfo_get(xdev->dma_device_index, xdev->func_id,
				&xdev->conf.qsets_base,
				(uint32_t *) &xdev->conf.qsets_max);
	if (rv < 0) {
		rv = qdma_dev_entry_create(xdev->dma_device_index,
				xdev->func_id);
		if (rv < 0) {
			pr_err("Failed to create device entry, err = %d", rv);
			rv = -ENODEV;
			goto unmap_bars;
		}
	}

	rv = qdma_dev_update(xdev->dma_device_index, xdev->func_id,
			     xdev->conf.qsets_max, &xdev->conf.qsets_base);
	if (rv < 0) {
		pr_err("qdma_dev_update function call failed, err = %d\n", rv);
		rv = xdev->hw.qdma_get_error_code(rv);
		goto unmap_bars;
	}

	if (!xdev->dev_cap.mm_en && !xdev->dev_cap.st_en) {
		pr_err("None of the modes ( ST or MM) are enabled\n");
		rv = -EINVAL;
		goto unmap_bars;
	}
#endif

#ifdef __QDMA_VF__
	if ((conf->qdma_drv_mode != POLL_MODE) &&
		(xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)) {
		pr_warn("VF is not supported in %s mode\n",
				mode_name_list[conf->qdma_drv_mode].name);
		pr_info("Switching VF to poll mode\n");
		xdev->conf.qdma_drv_mode = POLL_MODE;
	}
#endif

	if ((conf->qdma_drv_mode == LEGACY_INTR_MODE) &&
			(!xdev->dev_cap.legacy_intr)) {
		dev_err(&pdev->dev, "Legacy mode interrupts are not supported\n");
		goto unmap_bars;
	}

	memcpy(conf, &xdev->conf, sizeof(*conf));

	rv = qdma_device_online(pdev, (unsigned long)xdev, XDEV_FLR_INACTIVE);
	if (rv < 0) {
		pr_warn("Failed to set the dma device  online, err = %d", rv);
		goto cleanup_qdma;
	}

	rv = xdev_identify_bars(xdev, pdev);
	if (rv) {
		pr_err("Failed to identify bars, err %d", rv);
		goto unmap_bars;
	}

	pr_info("%s, %05x, pdev 0x%p, xdev 0x%p, ch %u, q %u, vf %u.\n",
		dev_name(&pdev->dev), xdev->conf.bdf, pdev, xdev,
		xdev->dev_cap.mm_channel_max, conf->qsets_max, conf->vf_max);

#ifdef DEBUGFS
	/** time to clean debugfs */
	dbgfs_dev_init(xdev);
#endif

	*dev_hndl = (unsigned long)xdev;

	return 0;

cleanup_qdma:
	qdma_device_offline(pdev, (unsigned long)xdev, XDEV_FLR_INACTIVE);

unmap_bars:
	xdev_unmap_bars(xdev, pdev);
	xdev_list_remove(xdev);
	kfree(xdev);

disable_device:
	pci_disable_extended_tag(pdev);
	pci_disable_relaxed_ordering(pdev);
	pci_disable_device(pdev);

release_regions:
	pci_release_regions(pdev);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_device_close() - close the dma device
 *
 * @param[in]	pdev:		pointer to struct pci_dev
 * @param[in]	dev_hndl:	device handle
 *
 * @return	0: on success
 * @return	<0: on failure
 *****************************************************************************/
int qdma_device_close(struct pci_dev *pdev, unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!dev_hndl) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	if (xdev->conf.pdev != pdev) {
		pr_err("Invalid pdev passed: pci_dev(0x%lx) != pdev(0x%lx)\n",
			(unsigned long)xdev->conf.pdev, (unsigned long)pdev);
		return -EINVAL;
	}

	qdma_device_offline(pdev, dev_hndl, XDEV_FLR_INACTIVE);

#ifdef DEBUGFS
	/** time to clean debugfs */
	dbgfs_dev_exit(xdev);
#endif
#ifndef __QDMA_VF__
	qdma_dev_entry_destroy(xdev->dma_device_index, xdev->func_id);
	qdma_master_resource_destroy(xdev->dma_device_index);
#endif

	xdev_unmap_bars(xdev, pdev);

	pci_disable_relaxed_ordering(pdev);
	pci_disable_extended_tag(pdev);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

	xdev_list_remove(xdev);

	kfree(xdev);

	return 0;
}

/*****************************************************************************/
/**
 * qdma_device_get_config() - get the device configuration
 *
 * @param[in]	dev_hndl:	device handle
 * @param[out]	conf:		dma device configuration
 * @param[out]	buf, buflen:
 *			error message buffer, can be NULL/0 (i.e., optional)
 *
 *
 * @return	none
 *****************************************************************************/
int qdma_device_get_config(unsigned long dev_hndl, struct qdma_dev_conf *conf,
					char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		if (buf && buflen)
			snprintf(buf, buflen, "dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		if (buf && buflen)
			snprintf(buf, buflen, "Invalid dev_hndl passed");
		return -EINVAL;
	}

	memcpy(conf, &xdev->conf, sizeof(*conf));

	if (buf && buflen)
		snprintf(buf, buflen,
			"Device %s configuration is stored in conf param",
			xdev->conf.name);

	return 0;
}

int qdma_device_clear_stats(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	xdev->total_mm_h2c_pkts = 0;
	xdev->total_mm_c2h_pkts = 0;
	xdev->total_st_h2c_pkts = 0;
	xdev->total_st_c2h_pkts = 0;
	xdev->ping_pong_lat_max = 0;
	xdev->ping_pong_lat_min = 0;
	xdev->ping_pong_lat_total = 0;

	return 0;
}

int qdma_device_get_mmh2c_pkts(unsigned long dev_hndl,
				unsigned long long *mmh2c_pkts)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*mmh2c_pkts = xdev->total_mm_h2c_pkts;

	return 0;
}

int qdma_device_get_mmc2h_pkts(unsigned long dev_hndl,
				unsigned long long *mmc2h_pkts)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*mmc2h_pkts = xdev->total_mm_c2h_pkts;

	return 0;
}

int qdma_device_get_sth2c_pkts(unsigned long dev_hndl,
				unsigned long long *sth2c_pkts)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*sth2c_pkts = xdev->total_st_h2c_pkts;

	return 0;
}

int qdma_device_get_stc2h_pkts(unsigned long dev_hndl,
				unsigned long long *stc2h_pkts)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*stc2h_pkts = xdev->total_st_c2h_pkts;

	return 0;
}

int qdma_device_get_ping_pong_min_lat(unsigned long dev_hndl,
				unsigned long long *min_lat)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (!min_lat) {
		pr_err("Min Lat is NULL\n");
		return -EINVAL;
	}
	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*min_lat = xdev->ping_pong_lat_min;

	return 0;
}

int qdma_device_get_ping_pong_max_lat(unsigned long dev_hndl,
				unsigned long long *max_lat)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (!max_lat) {
		pr_err("Max Lat is NULL\n");
		return -EINVAL;
	}
	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*max_lat = xdev->ping_pong_lat_max;

	return 0;
}

int qdma_device_get_ping_pong_tot_lat(unsigned long dev_hndl,
				unsigned long long *lat_total)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *) dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (!lat_total) {
		pr_err("Total Lat is NULL\n");
		return -EINVAL;
	}
	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*lat_total = xdev->ping_pong_lat_total;

	return 0;
}
/*****************************************************************************/
/**
 * qdma_device_set_config() - set the device configuration
 *
 * @param[in]	dev_hndl:	device handle
 * @param[in]	conf:		dma device configuration to set
 *
 * @return	0 on success ,<0 on failure
 *****************************************************************************/
int qdma_device_set_config(unsigned long dev_hndl, struct qdma_dev_conf *conf)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (!conf) {
		pr_err("conf is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0)
		return -EINVAL;

	memcpy(&xdev->conf, conf, sizeof(*conf));

	return 0;
}

