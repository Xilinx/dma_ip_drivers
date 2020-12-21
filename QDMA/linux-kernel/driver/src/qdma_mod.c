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

#include "qdma_mod.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/vmalloc.h>

#include "nl.h"
#include "libqdma/xdev.h"

/* include early, to verify it depends only on the headers above */
#include "version.h"

#define QDMA_DEFAULT_TOTAL_Q 2048

static char version[] =
	DRV_MODULE_DESC " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

static char mode[500] = {0};
module_param_string(mode, mode, sizeof(mode), 0);
MODULE_PARM_DESC(mode, "Load the driver in different modes, dflt is auto mode, format is \"<bus_num>:<pf_num>:<mode>\" and multiple comma separated entries can be specified");

static char config_bar[500] = {0};
module_param_string(config_bar, config_bar, sizeof(config_bar), 0);
MODULE_PARM_DESC(config_bar, "specify the config bar number, dflt is 0, format is \"<bus_num>:<pf_num>:<bar_num>\" and multiple comma separated entries can be specified");

static char master_pf[500] = {0};
module_param_string(master_pf, master_pf, sizeof(master_pf), 0);
MODULE_PARM_DESC(master_pf, "specify the master_pf, dflt is 0, format is \"<bus_num>:<master_pf>\" and multiple comma separated entries can be specified");

static unsigned int num_threads;
module_param(num_threads, uint, 0644);
MODULE_PARM_DESC(num_threads,
"Number of threads to be created each for request and writeback processing");


#include "pci_ids.h"

/*
 * xpdev helper functions
 */
static LIST_HEAD(xpdev_list);
static DEFINE_MUTEX(xpdev_mutex);

static int xpdev_qdata_realloc(struct xlnx_pci_dev *xpdev, unsigned int qmax);

static int xpdev_map_bar(struct xlnx_pci_dev *xpdev,
		void __iomem **regs, u8 bar_num);
static void xpdev_unmap_bar(struct xlnx_pci_dev *xpdev, void __iomem **regs);

#ifdef __QDMA_VF__
void qdma_flr_resource_free(unsigned long dev_hndl);
#endif

/*****************************************************************************/
/**
 * funcname() -  handler to show the intr_rngsz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   intr_rngsz configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the intr_rngsz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t show_intr_rngsz(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len;
	unsigned int rngsz = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	rngsz = qdma_get_intr_rngsz(xpdev->dev_hndl);
	len = scnprintf(buf, PAGE_SIZE, "%u\n", rngsz);
	if (len <= 0)
		pr_err("copying rngsz to buffer failed with err: %d\n", len);

	return len;
}

/*****************************************************************************/
/**
 * funcname() -  handler to set the intr_rngsz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   intr_rngsz configuration value
 * @buf :   buffer to hold the configured value
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the intr_rngsz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t set_intr_rngsz(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct xlnx_pci_dev *xpdev;
	unsigned int rngsz = 0;
	int err = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	err = kstrtouint(buf, 0, &rngsz);
	if (err < 0) {
		pr_err("failed to set interrupt ring size\n");
		return err;
	}


	err = qdma_set_intr_rngsz(xpdev->dev_hndl, (u32)rngsz);
	return err ? err : count;
}

/*****************************************************************************/
/**
 * funcname() -  handler to show the qmax configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   qmax configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the qmax
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t show_qmax(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len;
	unsigned int qmax = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qmax = qdma_get_qmax(xpdev->dev_hndl);
	len = scnprintf(buf, PAGE_SIZE, "%u\n", qmax);
	if (len <= 0)
		pr_err("copying qmax to buf failed with err: %d\n", len);

	return len;
}

#ifndef __QDMA_VF__
static ssize_t set_qmax(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct xlnx_pci_dev *xpdev;
	unsigned int qmax = 0;
	int err = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;
	err = kstrtouint(buf, 0, &qmax);
	if (err < 0) {
		pr_err("failed to set qmax to %d\n", qmax);
		return err;
	}
	err = qdma_set_qmax(xpdev->dev_hndl, -1, qmax);

	if (!err)
		xpdev_qdata_realloc(xpdev, qmax);

	return err ? err : count;
}

/*****************************************************************************/
/**
 * funcname() -  handler to show the cmpl_status_acc configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   cmpl_status_acc configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the cmpl_status_acc
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t show_cmpl_status_acc(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len;
	unsigned int cmpl_status_acc = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	cmpl_status_acc = qdma_get_wb_intvl(xpdev->dev_hndl);
	len = scnprintf(buf, PAGE_SIZE,
			"%u\n", cmpl_status_acc);
	if (len <= 0)
		pr_err("copying cmpl status acc value to buf failed with err: %d\n",
				len);

	return len;
}

/*****************************************************************************/
/**
 * funcname() -  handler to set the cmpl_status_acc configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   cmpl_status_acc configuration value
 * @buf :   buffer to hold the configured value
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the cmpl_status_acc
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t set_cmpl_status_acc(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef QDMA_CSR_REG_UPDATE
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	unsigned int cmpl_status_acc = 0;
	int err = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	err = kstrtoint(buf, 0, &cmpl_status_acc);
	if (err < 0) {
		pr_err("failed to set cmpl status accumulator to %d\n",
				cmpl_status_acc);
		return err;
	}

	err = qdma_set_cmpl_status_acc(xpdev->dev_hndl, cmpl_status_acc);
	return err ? err : count;
#else
	pr_warn("QDMA CSR completion status accumulation update is not allowed\n");
	return -EPERM;
#endif
}

/*****************************************************************************/
/**
 * funcname() -  handler to show the buf_sz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   buf_sz configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the buf_sz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/

static ssize_t show_c2h_buf_sz(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

	len += scnprintf(buf + len, PAGE_SIZE - len, "%u", c2h_buf_sz[0]);
	for (i = 1; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += scnprintf(buf + len,
					PAGE_SIZE - len, " %u", c2h_buf_sz[i]);
	len += scnprintf(buf + len, PAGE_SIZE - len, "%s", "\n\0");

	return len;
}

/*****************************************************************************/
/**
 * funcname() -  handler to set the buf_sz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   buf_sz configuration value
 * @buf :   buffer to hold the configured value
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the buf_sz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/

static ssize_t set_c2h_buf_sz(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef QDMA_CSR_REG_UPDATE
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int err = 0;
	char *s = (char *)buf, *p = NULL;
	const char *tc = " ";   /* token character here is a "space" */
	unsigned int c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
	int i = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	/* First read the values in to the register
	 * This helps to restore the values of entries if
	 * user configures lesser than 16 values
	 */
	qdma_get_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

	while (((p = strsep(&s, tc)) != NULL) &&
			(i < QDMA_GLOBAL_CSR_ARRAY_SZ)) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_buf_sz[i]);
		if (err < 0)
			goto input_err;

		i++;
	}

	if (p) {
		/*
		 * check if the number of entries are more than 16 !
		 * if yes, ignore the extra values
		 */
		pr_warn("Found more than 16 buffer size entries. Ignoring extra entries\n");
	}

	err = qdma_set_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

input_err:
	return err ? err : count;
#else
	pr_warn("QDMA CSR C2H buffer size update is not allowed\n");
	return -EPERM;
#endif
}

/*****************************************************************************/
/**
 * funcname() -  handler to show the ring_sz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   ring_sz configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the ring_sz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/

static ssize_t show_glbl_rng_sz(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	struct xlnx_dma_dev *xdev = NULL;
	int len = 0;
	int i;
	unsigned int glbl_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);
	qdma_get_ring_sizes(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ, glbl_ring_sz);
	len += scnprintf(buf + len, PAGE_SIZE - len, "%u", glbl_ring_sz[0]);
	for (i = 1; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += scnprintf(buf + len,
					PAGE_SIZE - len,
					" %u", glbl_ring_sz[i]);
	len += scnprintf(buf + len, PAGE_SIZE - len, "%s", "\n\0");

	return len;
}

/*****************************************************************************/
/**
 * funcname() -  handler to set the glbl_ring_sz configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   buf_sz configuration value
 * @buf :   buffer to hold the configured value
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the glbl_ring_sz
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/

static ssize_t set_glbl_rng_sz(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef QDMA_CSR_REG_UPDATE
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int err = 0;
	char *s = (char *)buf, *p = NULL;
	const char *tc = " ";   /* token character here is a "space" */
	unsigned int glbl_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
	int i = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	/* First read the values in to the register
	 * This helps to restore the values of entries if
	 * user configures lesser than 16 values
	 */
	qdma_get_glbl_rng_sz(xpdev->dev_hndl, glbl_ring_sz);

	while (((p = strsep(&s, tc)) != NULL) &&
			(i < QDMA_GLOBAL_CSR_ARRAY_SZ)) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &glbl_ring_sz[i]);
		if (err < 0)
			goto input_err;

		i++;
	}

	if (p) {
		/*
		 * check if the number of entries are more than 16 !
		 * if yes, ignore the extra values
		 */
		pr_warn("Found more than 16 ring size entries. Ignoring extra entries");
	}

	err = qdma_set_glbl_rng_sz(xpdev->dev_hndl, glbl_ring_sz);

input_err:
	return err ? err : count;
#else
	pr_warn("QDMA CSR global ring size update is not allowed\n");
	return -EPERM;
#endif
}

/*****************************************************************************/
/**
 * funcname() -  handler to show global csr c2h_timer_cnt configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   c2h_timer_cnt configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the global csr c2h_timer_cnt
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t show_c2h_timer_cnt(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

	len += scnprintf(buf + len, PAGE_SIZE - len, "%u", c2h_timer_cnt[0]);
	for (i = 1; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += scnprintf(buf + len,
					PAGE_SIZE - len,
					" %u", c2h_timer_cnt[i]);
	len += scnprintf(buf + len, PAGE_SIZE - len, "%s", "\n\0");

	return len;
}

/*****************************************************************************/
/**
 * set_c2h_timer_cnt() -  handler to set global csr c2h_timer_cnt config
 *
 * @dev :   PCIe device handle
 * @attr:   c2h_timer_cnt configuration value
 * @buf :   buffer containing new configuration
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the global csr c2h_timer_cnt
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t set_c2h_timer_cnt(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef QDMA_CSR_REG_UPDATE
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int err = 0;
	char *s = (char *)buf, *p = NULL;
	const char *tc = " ";	/* token character here is a "space" */
	unsigned int c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
	int i = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	/* First read the values in to the register
	 * This helps to restore the values of entries if
	 * user configures lesser than 16 values
	 */
	qdma_get_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

	while (((p = strsep(&s, tc)) != NULL) &&
			(i < QDMA_GLOBAL_CSR_ARRAY_SZ)) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_timer_cnt[i]);
		if (err < 0)
			goto input_err;

		if (c2h_timer_cnt[i] > 255) {
			pr_warn("timer cnt at index %d is %d - out of range [0-255]\n",
				i, c2h_timer_cnt[i]);
			err = -EINVAL;
			goto input_err;
		}
		i++;
	}

	if (p) {
		/*
		 * check if the number of entries are more than 16 !
		 * if yes, ignore the extra values
		 */
		pr_warn("Found more than 16 timer entries. Ignoring extra entries\n");
	}
	err = qdma_set_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

input_err:
	return err ? err : count;
#else
	pr_warn("QDMA CSR C2H timer counter update is not allowed\n");
	return -EPERM;
#endif
}

/*****************************************************************************/
/**
 * funcname() -  handler to show global csr c2h_cnt_th_ configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   c2h_cnt_th configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to show the global csr c2h_cnt_th
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t show_c2h_cnt_th(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

	len += scnprintf(buf + len,
				PAGE_SIZE - len, "%u", c2h_cnt_th[0]);
	for (i = 1; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += scnprintf(buf + len,
				PAGE_SIZE - len, " %u", c2h_cnt_th[i]);
	len += scnprintf(buf + len,
				PAGE_SIZE - len, "%s", "\n\0");

	return len;
}

/*****************************************************************************/
/**
 * set_c2h_cnt_th() -  handler to set global csr c2h_cnt_th configuration
 *
 * @dev :   PCIe device handle
 * @attr:   c2h_cnt_th configuration value
 * @buf :   buffer containing new configuration
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the global csr c2h_cnt_th
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t set_c2h_cnt_th(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef QDMA_CSR_REG_UPDATE
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int err = 0;
	char *s = (char *)buf, *p = NULL;
	const char *tc = " ";	/* token character here is a "space" */
	unsigned int c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
	int i = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	/* First read the values in to the register
	 * This helps to restore the values of entries if
	 * user configures lesser than 16 values
	 */
	qdma_get_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

	while (((p = strsep(&s, tc)) != NULL) &&
			(i < QDMA_GLOBAL_CSR_ARRAY_SZ)) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_cnt_th[i]);
		if (err < 0)
			goto input_err;

		if (c2h_cnt_th[i] > 255) {
			pr_warn("counter threshold at index %d is %d - out of range [0-255]\n",
				i, c2h_cnt_th[i]);
			err = -EINVAL;
			goto input_err;
		}
		i++;
	}

	if (p) {
		/*
		 * check if the number of entries are more than 16 !
		 * if yes, ignore the extra values
		 */
		pr_warn("Found more than 16 counter entries. Ignoring extra entries\n");
	}
	err = qdma_set_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

input_err:
	return err ? err : count;
#else
	pr_warn("QDMA CSR C2H counter threshold update is not allowed\n");
	return -EPERM;
#endif
}
#else /** For VF #ifdef __QDMA_VF__ */
/*****************************************************************************/
/**
 * funcname() -  handler to set the qmax configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   qmax configuration value
 * @buf :   buffer to hold the configured value
 *
 * Handler function to set the qmax
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/

static ssize_t set_qmax(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct xlnx_pci_dev *_xpdev;
	unsigned int qmax = 0;
	int err = 0;

	_xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!_xpdev)
		return -EINVAL;

	err = kstrtouint(buf, 0, &qmax);
	if (err < 0) {
		pr_err("Failed to set qmax\n");
		return err;
	}

	err = qdma_vf_qconf(_xpdev->dev_hndl, qmax);
	if (err < 0)
		return err;

	if (!err) {
		pr_debug("Succesfully reconfigured qdmavf%05x\n", _xpdev->idx);
		xpdev_qdata_realloc(_xpdev, qmax);
	}

	return err ? err : count;
}
#endif

static DEVICE_ATTR(qmax, S_IWUSR | S_IRUGO, show_qmax, set_qmax);
static DEVICE_ATTR(intr_rngsz, S_IWUSR | S_IRUGO,
			show_intr_rngsz, set_intr_rngsz);
#ifndef __QDMA_VF__
static DEVICE_ATTR(buf_sz, S_IWUSR | S_IRUGO,
		show_c2h_buf_sz, set_c2h_buf_sz);
static DEVICE_ATTR(glbl_rng_sz, S_IWUSR | S_IRUGO,
		show_glbl_rng_sz, set_glbl_rng_sz);
static DEVICE_ATTR(c2h_timer_cnt, S_IWUSR | S_IRUGO,
			show_c2h_timer_cnt, set_c2h_timer_cnt);
static DEVICE_ATTR(c2h_cnt_th, S_IWUSR | S_IRUGO,
			show_c2h_cnt_th, set_c2h_cnt_th);
static DEVICE_ATTR(cmpl_status_acc, S_IWUSR | S_IRUGO,
		show_cmpl_status_acc, set_cmpl_status_acc);
#endif

static struct attribute *pci_device_attrs[] = {
		&dev_attr_qmax.attr,
		&dev_attr_intr_rngsz.attr,
		NULL,
};

static struct attribute *pci_master_device_attrs[] = {
		&dev_attr_qmax.attr,
		&dev_attr_intr_rngsz.attr,
#ifndef __QDMA_VF__
		&dev_attr_buf_sz.attr,
		&dev_attr_glbl_rng_sz.attr,
		&dev_attr_c2h_timer_cnt.attr,
		&dev_attr_c2h_cnt_th.attr,
		&dev_attr_cmpl_status_acc.attr,
#endif
		NULL,
};

static struct attribute_group pci_device_attr_group = {
		.name  = "qdma",
		.attrs = pci_device_attrs,

};

static struct attribute_group pci_master_device_attr_group = {
		.name  = "qdma",
		.attrs = pci_master_device_attrs,

};

static inline void xpdev_list_remove(struct xlnx_pci_dev *xpdev)
{
	mutex_lock(&xpdev_mutex);
	list_del(&xpdev->list_head);
	mutex_unlock(&xpdev_mutex);
}

static inline void xpdev_list_add(struct xlnx_pci_dev *xpdev)
{
	mutex_lock(&xpdev_mutex);
	list_add_tail(&xpdev->list_head, &xpdev_list);
	mutex_unlock(&xpdev_mutex);
}

int xpdev_list_dump(char *buf, int buflen)
{
	struct xlnx_pci_dev *xpdev, *tmp;
	char *cur = buf;
	char *const end = buf + buflen;
	int base_end = 0;
	int qmax_val = 0;

	if (!buf || !buflen)
		return -EINVAL;

	mutex_lock(&xpdev_mutex);
	list_for_each_entry_safe(xpdev, tmp, &xpdev_list, list_head) {
		struct pci_dev *pdev;
		struct qdma_dev_conf conf;
		int rv;

		rv = qdma_device_get_config(xpdev->dev_hndl, &conf, NULL, 0);
		if (rv < 0) {
			cur += snprintf(cur, cur - end,
			"ERR! unable to get device config for idx %05x\n",
			xpdev->idx);
			if (cur >= end)
				goto handle_truncation;
			break;
		}

		pdev = conf.pdev;

		base_end = (int)(conf.qsets_base + conf.qsets_max - 1);
		if (base_end < 0)
			base_end = 0;
		qmax_val = conf.qsets_max;

		if (qmax_val) {
			cur += snprintf(cur, end - cur,
#ifdef __QDMA_VF__
					"qdmavf%05x\t%s\tmax QP: %d, %d~%d\n",
#else
					"qdma%05x\t%s\tmax QP: %d, %d~%d\n",
#endif
					xpdev->idx, dev_name(&pdev->dev),
					qmax_val, conf.qsets_base,
					base_end);
		} else {
			cur += snprintf(cur, end - cur,
#ifdef __QDMA_VF__
					"qdmavf%05x\t%s\tmax QP: 0, -~-\n",
#else
					"qdma%05x\t%s\tmax QP: 0, -~-\n",
#endif
					xpdev->idx, dev_name(&pdev->dev));
		}
		if (cur >= end)
			goto handle_truncation;
	}
	mutex_unlock(&xpdev_mutex);

	return cur - buf;

handle_truncation:
	mutex_unlock(&xpdev_mutex);
	pr_warn("ERR! str truncated. req=%lu, avail=%u", cur - buf, buflen);
	*buf = '\0';
	return cur - buf;
}

/**
 * Function to find the first PF device available in the card
 */
static bool is_first_pfdev(u8 bus_number)
{
	struct xlnx_pci_dev *_xpdev, *tmp;

	mutex_lock(&xpdev_mutex);
	if (list_empty(&xpdev_list)) {
		mutex_unlock(&xpdev_mutex);
		return 1;
	}

	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		struct pci_dev *pdev = _xpdev->pdev;
		/** find first bus and device are matching */
		if (pdev->bus->number == bus_number) {
			mutex_unlock(&xpdev_mutex);
			/** if func matches, it returns 1, else 0*/
			return 0;
		}
	}
	mutex_unlock(&xpdev_mutex);

	return 1;
}
/*****************************************************************************/
/**
 * extract_mod_param() - extract the device mode and config bar per function
 *
 * @param[in]	pdev:		pcie device
 * @param[in]	qdma_drv_mod_param_type:		mod param type
 *
 * @return	device mode
 *****************************************************************************/
static u8 extract_mod_param(struct pci_dev *pdev,
		enum qdma_drv_mod_param_type param_type)
{
	char p[600];
	char *ptr, *mod;

	u8 dev_fn = PCI_FUNC(pdev->devfn);
#ifdef __QDMA_VF__
	u16 device_id = pdev->device;
#endif


	/* Fetch param specified in the module parameter */
	ptr = p;
	if (param_type == DRV_MODE) {
		if (mode[0] == '\0')
			return 0;
		strncpy(p, mode, sizeof(p) - 1);
	} else if (param_type == CONFIG_BAR) {
		if (config_bar[0] == '\0')
			return 0;
		strncpy(p, config_bar, sizeof(p) - 1);
	} else if (param_type == MASTER_PF) {
		if (master_pf[0] == '\0')
			return is_first_pfdev(pdev->bus->number);
		strncpy(p, master_pf, sizeof(p) - 1);
	} else {
		pr_err("Invalid module param type received\n");
		return -EINVAL;
	}

	while ((mod = strsep(&ptr, ","))) {
		unsigned int bus_num, func_num, param = 0;
		int fields;

		if (!strlen(mod))
			continue;

		if (param_type == MASTER_PF) {
			fields = sscanf(mod, "%x:%x", &bus_num, &param);

			if (fields != 2) {
				pr_warn("invalid mode string \"%s\"\n", mod);
				continue;
			}

			if ((bus_num == pdev->bus->number) &&
					(dev_fn == param))
				return 1;
		} else {
			fields = sscanf(mod, "%x:%x:%x",
					&bus_num, &func_num, &param);

			if (fields != 3) {
				pr_warn("invalid mode string \"%s\"\n", mod);
				continue;
			}

#ifndef __QDMA_VF__
			if ((bus_num == pdev->bus->number) &&
					(func_num == dev_fn))
				return param;
#else
			if ((bus_num == pdev->bus->number) &&
				(((device_id >> VF_PF_IDENTIFIER_SHIFT) &
					VF_PF_IDENTIFIER_MASK) == func_num))
				return param;
#endif
		}
	}

	return 0;
}

struct xlnx_pci_dev *xpdev_find_by_idx(unsigned int idx, char *buf, int buflen)
{
	struct xlnx_pci_dev *xpdev, *tmp;

	mutex_lock(&xpdev_mutex);
	list_for_each_entry_safe(xpdev, tmp, &xpdev_list, list_head) {
		if (xpdev->idx == idx) {
			mutex_unlock(&xpdev_mutex);
			return xpdev;
		}
	}
	mutex_unlock(&xpdev_mutex);

	if (buf && buflen)
		snprintf(buf, buflen, "NO device found at index %05x!\n", idx);

	return NULL;
}

struct xlnx_qdata *xpdev_queue_get(struct xlnx_pci_dev *xpdev,
			unsigned int qidx, u8 q_type, bool check_qhndl,
			char *ebuf, int ebuflen)
{
	struct xlnx_qdata *qdata;

	if (qidx >= xpdev->qmax) {
		pr_debug("qdma%05x QID %u too big, %05x.\n",
			xpdev->idx, qidx, xpdev->qmax);
		if (ebuf && ebuflen) {
			snprintf(ebuf, ebuflen, "QID %u too big, %u.\n",
					 qidx, xpdev->qmax);
		}
		return NULL;
	}

	qdata = xpdev->qdata + qidx;
	if (q_type == Q_C2H)
		qdata += xpdev->qmax;
	if (q_type == Q_CMPT)
		qdata += (2 * xpdev->qmax);

	if (check_qhndl && (!qdata->qhndl && !qdata->xcdev)) {
		pr_debug("qdma%05x QID %u NOT configured.\n", xpdev->idx, qidx);
		if (ebuf && ebuflen) {
			snprintf(ebuf, ebuflen,
					"QID %u NOT configured.\n", qidx);
		}

		return NULL;
	}

	return qdata;
}

int xpdev_queue_delete(struct xlnx_pci_dev *xpdev, unsigned int qidx, u8 q_type,
			char *ebuf, int ebuflen)
{
	struct xlnx_qdata *qdata = xpdev_queue_get(xpdev, qidx, q_type, 1, ebuf,
						ebuflen);
	int rv = 0;

	if (!qdata)
		return -EINVAL;

	if (q_type != Q_CMPT) {
		if (!qdata->xcdev)
			return -EINVAL;
	}

	if (qdata->qhndl != QDMA_QUEUE_IDX_INVALID)
		rv = qdma_queue_remove(xpdev->dev_hndl, qdata->qhndl,
					ebuf, ebuflen);
	else
		pr_err("qidx %u/%u, type %d, qhndl invalid.\n",
			qidx, xpdev->qmax, q_type);
	if (rv < 0)
		goto exit;

	if (q_type != Q_CMPT) {
		spin_lock(&xpdev->cdev_lock);
		qdata->xcdev->dir_init &= ~(1 << (q_type ? 1 : 0));

		if (!qdata->xcdev->dir_init)
			qdma_cdev_destroy(qdata->xcdev);
		spin_unlock(&xpdev->cdev_lock);
	}

	memset(qdata, 0, sizeof(*qdata));
exit:
	return rv;
}

#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static void xpdev_queue_delete_all(struct xlnx_pci_dev *xpdev)
{
	int i;

	for (i = 0; i < xpdev->qmax; i++) {
		xpdev_queue_delete(xpdev, i, 0, NULL, 0);
		xpdev_queue_delete(xpdev, i, 1, NULL, 0);
	}
}
#endif

int xpdev_queue_add(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf,
			char *ebuf, int ebuflen)
{
	struct xlnx_qdata *qdata;
	struct qdma_cdev *xcdev = NULL;
	struct xlnx_qdata *qdata_tmp;
	struct qdma_dev_conf dev_config;
	u8 dir;
	unsigned long qhndl;
	int rv;

	rv = qdma_queue_add(xpdev->dev_hndl, qconf, &qhndl, ebuf, ebuflen);
	if (rv < 0)
		return rv;

	pr_debug("qdma%05x idx %u, st %d, q_type %s, added, qhndl 0x%lx.\n",
		xpdev->idx, qconf->qidx, qconf->st,
		q_type_list[qconf->q_type].name, qhndl);

	qdata = xpdev_queue_get(xpdev, qconf->qidx, qconf->q_type, 0, ebuf,
				ebuflen);
	if (!qdata) {
		pr_err("q added 0x%lx, get failed, idx 0x%x.\n",
			qhndl, qconf->qidx);
		return rv;
	}

	if (qconf->q_type != Q_CMPT) {
		dir = (qconf->q_type == Q_C2H) ? 0 : 1;
		spin_lock(&xpdev->cdev_lock);
		qdata_tmp = xpdev_queue_get(xpdev, qconf->qidx,
				dir, 0, NULL, 0);
		if (qdata_tmp) {
			/* only 1 cdev per queue pair */
			if (qdata_tmp->xcdev) {
				unsigned long *priv_data;

				qdata->qhndl = qhndl;
				qdata->xcdev = qdata_tmp->xcdev;
				priv_data = (qconf->q_type == Q_C2H) ?
						&qdata->xcdev->c2h_qhndl :
						&qdata->xcdev->h2c_qhndl;
				*priv_data = qhndl;
				qdata->xcdev->dir_init |= (1 << qconf->q_type);

				spin_unlock(&xpdev->cdev_lock);
				return 0;
			}
		}
		spin_unlock(&xpdev->cdev_lock);
	}

	rv = qdma_device_get_config(xpdev->dev_hndl, &dev_config, NULL, 0);
	if (rv < 0) {
		pr_err("Failed to get conf for qdma device '%05x'\n",
				xpdev->idx);
		return rv;
	}

	/* always create the cdev
	 * Give HW QID as minor number with qsets_base calculation
	 */
	if (qconf->q_type != Q_CMPT) {
		rv = qdma_cdev_create(&xpdev->cdev_cb, xpdev->pdev, qconf,
				(dev_config.qsets_base + qconf->qidx),
				qhndl, &xcdev, ebuf, ebuflen);

		qdata->xcdev = xcdev;
	}

	qdata->qhndl = qhndl;

	return rv;
}

static void nl_work_handler_q_start(struct work_struct *work)
{
	struct xlnx_nl_work *nl_work = container_of(work, struct xlnx_nl_work,
						work);
	struct xlnx_pci_dev *xpdev = nl_work->xpdev;
	struct xlnx_nl_work_q_ctrl *qctrl = &nl_work->qctrl;
	unsigned int qidx = qctrl->qidx;
	u8 is_qp = qctrl->is_qp;
	u8 q_type = qctrl->q_type;
	int i;
	char *ebuf = nl_work->buf;
	int rv = 0;

	for (i = 0; i < qctrl->qcnt; i++, qidx++) {
		struct xlnx_qdata *qdata;

q_start:
		qdata = xpdev_queue_get(xpdev, qidx, q_type, 1, ebuf,
					nl_work->buflen);
		if (!qdata) {
			pr_err("%s, idx %u, q_type %s, get failed.\n",
				dev_name(&xpdev->pdev->dev), qidx,
				q_type_list[q_type].name);
			snprintf(ebuf, nl_work->buflen,
				"Q idx %u, q_type %s, get failed.\n",
				qidx, q_type_list[q_type].name);
			goto send_resp;
		}

		rv = qdma_queue_start(xpdev->dev_hndl, qdata->qhndl, ebuf,
				      nl_work->buflen);
		if (rv < 0) {
			pr_err("%s, idx %u, q_type %s, start failed %d.\n",
				dev_name(&xpdev->pdev->dev), qidx,
				q_type_list[q_type].name, rv);
			snprintf(ebuf, nl_work->buflen,
				"Q idx %u, q_type %s, start failed %d.\n",
				qidx, q_type_list[q_type].name, rv);
			goto send_resp;
		}
		if (qctrl->q_type != Q_CMPT) {
			if (is_qp && q_type == qctrl->q_type) {
				q_type = !qctrl->q_type;
				goto q_start;
			}

			q_type = qctrl->q_type;
		}
	}

	snprintf(ebuf, nl_work->buflen,
		 "%u Queues started, idx %u ~ %u.\n",
		qctrl->qcnt, qctrl->qidx, qidx - 1);

send_resp:
	nl_work->q_start_handled = 1;
	nl_work->ret = rv;
	wake_up_interruptible(&nl_work->wq);
}

static struct xlnx_nl_work *xpdev_nl_work_alloc(struct xlnx_pci_dev *xpdev)
{
	struct xlnx_nl_work *nl_work;

	/* allocate work struct */
	nl_work = kzalloc(sizeof(*nl_work), GFP_ATOMIC);
	if (!nl_work) {
		pr_err("qdma%05x %s: OOM.\n",
			xpdev->idx, dev_name(&xpdev->pdev->dev));
		return NULL;
	}

	nl_work->xpdev = xpdev;

	return nl_work;
}

int xpdev_nl_queue_start(struct xlnx_pci_dev *xpdev, void *nl_info, u8 is_qp,
			u8 q_type, unsigned short qidx, unsigned short qcnt)
{
	struct xlnx_nl_work *nl_work = xpdev_nl_work_alloc(xpdev);
	struct xlnx_nl_work_q_ctrl *qctrl;
	char ebuf[XNL_EBUFLEN];
	int rv = 0;

	if (!nl_work)
		return -ENOMEM;

	qctrl = &nl_work->qctrl;
	qctrl->is_qp = is_qp;
	qctrl->q_type = q_type;
	qctrl->qidx = qidx;
	qctrl->qcnt = qcnt;

	INIT_WORK(&nl_work->work, nl_work_handler_q_start);
	init_waitqueue_head(&nl_work->wq);
	nl_work->q_start_handled = 0;
	nl_work->buf = ebuf;
	nl_work->buflen = XNL_EBUFLEN;
	queue_work(xpdev->nl_task_wq, &nl_work->work);
	wait_event_interruptible(nl_work->wq, nl_work->q_start_handled);
	rv = nl_work->ret;
	kfree(nl_work);
	xnl_respond_buffer(nl_info, ebuf, strlen(ebuf), rv);

	return rv;
}

static void xpdev_free(struct xlnx_pci_dev *p)
{
	xpdev_list_remove(p);

	if (p->nl_task_wq)
		destroy_workqueue(p->nl_task_wq);

	kfree(p->qdata);
	kfree(p);
}

static int xpdev_qdata_realloc(struct xlnx_pci_dev *xpdev, unsigned int qmax)
{
	if (!xpdev)
		return -EINVAL;

	kfree(xpdev->qdata);
	xpdev->qdata = NULL;

	if (!qmax)
		return 0;
	xpdev->qdata = kzalloc(qmax * 3 * sizeof(struct xlnx_qdata),
			       GFP_KERNEL);
	if (!xpdev->qdata) {
		pr_err("OMM, xpdev->qdata, sz %u.\n", qmax);
		return -ENOMEM;
	}
	xpdev->qmax = qmax;

	return 0;
}

static struct xlnx_pci_dev *xpdev_alloc(struct pci_dev *pdev, unsigned int qmax)
{
	int sz = sizeof(struct xlnx_pci_dev);
	struct xlnx_pci_dev *xpdev = kzalloc(sz, GFP_KERNEL);
	char name[80];

	if (!xpdev) {
		xpdev = vmalloc(sz);
		if (xpdev)
			memset(xpdev, 0, sz);
	}

	if (!xpdev) {
		pr_err("OMM, qmax %u, sz %u.\n", qmax, sz);
		return NULL;
	}
	spin_lock_init(&xpdev->cdev_lock);
	xpdev->pdev = pdev;
	xpdev->qmax = qmax;
	xpdev->idx = 0xFF;
	if (qmax && (xpdev_qdata_realloc(xpdev, qmax) < 0))
		goto free_xpdev;

	snprintf(name, 80, "qdma_%s_nl_wq", dev_name(&pdev->dev));
	xpdev->nl_task_wq = create_singlethread_workqueue(name);
	if (!xpdev->nl_task_wq) {
		pr_err("%s failed to allocate nl_task_wq.\n",
				dev_name(&pdev->dev));
		goto free_xpdev;
	}

	xpdev_list_add(xpdev);
	return xpdev;

free_xpdev:
	xpdev_free(xpdev);
	return NULL;
}

static int xpdev_map_bar(struct xlnx_pci_dev *xpdev,
		void __iomem **regs, u8 bar_num)
{
	int map_len;

	/* map the AXI Master Lite bar */
	map_len = pci_resource_len(xpdev->pdev, (int)bar_num);
	if (map_len > QDMA_MAX_BAR_LEN_MAPPED)
		map_len = QDMA_MAX_BAR_LEN_MAPPED;

	*regs = pci_iomap(xpdev->pdev, bar_num, map_len);
	if (!(*regs)) {
		pr_err("unable to map bar %d.\n", bar_num);
		return -ENOMEM;
	}

	return 0;
}

static void xpdev_unmap_bar(struct xlnx_pci_dev *xpdev, void __iomem **regs)
{
	/* unmap BAR */
	if (*regs) {
		pci_iounmap(xpdev->pdev, *regs);
		/* mark as unmapped */
		*regs = NULL;
	}
}

int qdma_device_read_user_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 *value)
{
	struct xlnx_dma_dev *xdev = NULL;
	int rv = 0;

	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	if (xdev->conf.bar_num_user < 0) {
		pr_err("AXI Master Lite bar is not present\n");
		return -EINVAL;
	}

	/* map the AXI Master Lite bar */
	rv = xpdev_map_bar(xpdev, &xpdev->user_bar_regs,
			xdev->conf.bar_num_user);
	if (rv < 0)
		return rv;

	*value = readl(xpdev->user_bar_regs + reg_addr);

	/* unmap the AXI Master Lite bar after accessing it */
	xpdev_unmap_bar(xpdev, &xpdev->user_bar_regs);

	return 0;
}

int qdma_device_write_user_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 value)
{
	struct xlnx_dma_dev *xdev = NULL;
	int rv = 0;

	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	if (xdev->conf.bar_num_user < 0) {
		pr_err("AXI Master Lite bar is not present\n");
		return -EINVAL;
	}

	/* map the AXI Master Lite bar */
	rv = xpdev_map_bar(xpdev, &xpdev->user_bar_regs,
			xdev->conf.bar_num_user);
	if (rv < 0)
		return rv;


	writel(value, xpdev->user_bar_regs + reg_addr);

	/* unmap the AXI Master Lite bar after accessing it */
	xpdev_unmap_bar(xpdev, &xpdev->user_bar_regs);

	return 0;
}

int qdma_device_read_bypass_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 *value)
{
	struct xlnx_dma_dev *xdev = NULL;
	int rv = 0;

	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	if (xdev->conf.bar_num_bypass < 0) {
		pr_err("AXI Bridge Master bar is not present\n");
		return -EINVAL;
	}

	/* map the AXI Bridge Master bar */
	rv = xpdev_map_bar(xpdev, &xpdev->bypass_bar_regs,
			xdev->conf.bar_num_bypass);
	if (rv < 0)
		return rv;

	*value = readl(xpdev->bypass_bar_regs + reg_addr);

	/* unmap the AXI Bridge Master bar after accessing it */
	xpdev_unmap_bar(xpdev, &xpdev->bypass_bar_regs);

	return 0;
}

int qdma_device_write_bypass_register(struct xlnx_pci_dev *xpdev,
		u32 reg_addr, u32 value)
{
	struct xlnx_dma_dev *xdev = NULL;
	int rv = 0;

	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	if (xdev->conf.bar_num_bypass < 0) {
		pr_err("AXI Bridge Master bar is not present\n");
		return -EINVAL;
	}

	/* map the AXI Bridge Master bar */
	rv = xpdev_map_bar(xpdev, &xpdev->bypass_bar_regs,
			xdev->conf.bar_num_bypass);
	if (rv < 0)
		return rv;

	writel(value, xpdev->bypass_bar_regs + reg_addr);

	/* unmap the AXI Bridge Master bar after accessing it */
	xpdev_unmap_bar(xpdev, &xpdev->bypass_bar_regs);

	return 0;
}

static int probe_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct qdma_dev_conf conf;
	struct xlnx_pci_dev *xpdev = NULL;
	unsigned long dev_hndl;
	int rv;
#ifdef __x86_64__
	pr_info("%s: func 0x%x, p/v %d/%d,0x%p.\n",
		dev_name(&pdev->dev), PCI_FUNC(pdev->devfn),
		pdev->is_physfn, pdev->is_virtfn, pdev->physfn);
#endif
	memset(&conf, 0, sizeof(conf));

	conf.qdma_drv_mode = (enum qdma_drv_mode)extract_mod_param(pdev,
			DRV_MODE);
	conf.vf_max = 0;	/* enable via sysfs */

#ifndef __QDMA_VF__
	conf.master_pf = extract_mod_param(pdev, MASTER_PF);
	if (conf.master_pf)
		pr_info("Configuring '%02x:%02x:%x' as master pf\n",
				pdev->bus->number,
				PCI_SLOT(pdev->devfn),
				PCI_FUNC(pdev->devfn));

#endif /* #ifdef __QDMA_VF__ */
	pr_info("Driver is loaded in %s(%d) mode\n",
				mode_name_list[conf.qdma_drv_mode].name,
				conf.qdma_drv_mode);

	if (conf.qdma_drv_mode == LEGACY_INTR_MODE)
		intr_legacy_init();

	conf.intr_rngsz = QDMA_INTR_COAL_RING_SIZE;
	conf.pdev = pdev;

	/* initialize all the bar numbers with -1 */
	conf.bar_num_config = -1;
	conf.bar_num_user = -1;
	conf.bar_num_bypass = -1;

	conf.bar_num_config = extract_mod_param(pdev, CONFIG_BAR);
	conf.qsets_max = 0;
	conf.qsets_base = -1;
	conf.msix_qvec_max = 32;
	conf.user_msix_qvec_max = 1;
#ifdef __QDMA_VF__
	conf.fp_flr_free_resource = qdma_flr_resource_free;
#endif
	if (conf.master_pf)
		conf.data_msix_qvec_max = 5;
	else
		conf.data_msix_qvec_max = 6;

	rv = qdma_device_open(DRV_MODULE_NAME, &conf, &dev_hndl);
	if (rv < 0)
		return rv;

	xpdev = xpdev_alloc(pdev, conf.qsets_max);
	if (!xpdev) {
		rv = -EINVAL;
		goto close_device;
	}

	xpdev->dev_hndl = dev_hndl;
	xpdev->idx = conf.bdf;

	xpdev->cdev_cb.xpdev = xpdev;
	rv = qdma_cdev_device_init(&xpdev->cdev_cb);
	if (rv < 0)
		goto close_device;

	/* Create the files for attributes in sysfs */
	if (conf.master_pf) {
		rv = sysfs_create_group(&pdev->dev.kobj,
				&pci_master_device_attr_group);
		if (rv < 0)
			goto close_device;
	} else {
		rv = sysfs_create_group(&pdev->dev.kobj,
				&pci_device_attr_group);
		if (rv < 0)
			goto close_device;
	}

	dev_set_drvdata(&pdev->dev, xpdev);

	return 0;

close_device:
	qdma_device_close(pdev, dev_hndl);

	if (xpdev)
		xpdev_free(xpdev);

	return rv;
}

static void xpdev_device_cleanup(struct xlnx_pci_dev *xpdev)
{
	struct xlnx_qdata *qdata = xpdev->qdata;
	struct xlnx_qdata *qmax = qdata + (xpdev->qmax * 2); /* h2c and c2h */

	spin_lock(&xpdev->cdev_lock);
	for (; qdata != qmax; qdata++) {
		if (qdata->xcdev) {
			/* if either h2c(1) or c2h(2) bit set, but not both */
			if (qdata->xcdev->dir_init == 1 ||
				qdata->xcdev->dir_init == 2) {
				qdma_cdev_destroy(qdata->xcdev);
			} else { /* both bits are set so remove one */
				qdata->xcdev->dir_init >>= 1;
			}
		}
		memset(qdata, 0, sizeof(*qdata));
	}
	spin_unlock(&xpdev->cdev_lock);
}

static void remove_one(struct pci_dev *pdev)
{
	struct xlnx_dma_dev *xdev = NULL;
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	if (!xpdev) {
		pr_info("%s NOT attached.\n", dev_name(&pdev->dev));
		return;
	}

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	pr_info("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);

	if (xdev->conf.master_pf)
		sysfs_remove_group(&pdev->dev.kobj,
				&pci_master_device_attr_group);
	else
		sysfs_remove_group(&pdev->dev.kobj, &pci_device_attr_group);

	qdma_cdev_device_cleanup(&xpdev->cdev_cb);

	xpdev_device_cleanup(xpdev);

	qdma_device_close(pdev, xpdev->dev_hndl);

	xpdev_free(xpdev);

	dev_set_drvdata(&pdev->dev, NULL);
}

#if defined(CONFIG_PCI_IOV) && !defined(__QDMA_VF__)
static int sriov_config(struct pci_dev *pdev, int num_vfs)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);
	int total_vfs = pci_sriov_get_totalvfs(pdev);

	if (!xpdev) {
		pr_info("%s NOT attached.\n", dev_name(&pdev->dev));
		return -EINVAL;
	}

	pr_debug("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);

	if (num_vfs > total_vfs) {
		pr_info("%s, clamp down # of VFs %d -> %d.\n",
			dev_name(&pdev->dev), num_vfs, total_vfs);
		num_vfs = total_vfs;
	}

	return qdma_device_sriov_config(pdev, xpdev->dev_hndl, num_vfs);
}
#endif

static pci_ers_result_t qdma_error_detected(struct pci_dev *pdev,
					pci_channel_state_t state)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	switch (state) {
	case pci_channel_io_normal:
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		pr_warn("dev 0x%p,0x%p, frozen state error, reset controller\n",
			pdev, xpdev);
		pci_disable_device(pdev);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		pr_warn("dev 0x%p,0x%p, failure state error, req. disconnect\n",
			pdev, xpdev);
		return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t qdma_slot_reset(struct pci_dev *pdev)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	if (!xpdev) {
		pr_info("%s NOT attached.\n", dev_name(&pdev->dev));
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pr_info("0x%p restart after slot reset\n", xpdev);
	if (pci_enable_device_mem(pdev)) {
		pr_info("0x%p failed to renable after slot reset\n", xpdev);
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pci_set_master(pdev);
	pci_restore_state(pdev);
	pci_save_state(pdev);

	return PCI_ERS_RESULT_RECOVERED;
}

static void qdma_error_resume(struct pci_dev *pdev)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	if (!xpdev) {
		pr_info("%s NOT attached.\n", dev_name(&pdev->dev));
		return;
	}

	pr_info("dev 0x%p,0x%p.\n", pdev, xpdev);
#if KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE
	pci_aer_clear_nonfatal_status(pdev);
#else
	pci_cleanup_aer_uncorrect_error_status(pdev);
#endif
}


#ifdef __QDMA_VF__
void qdma_flr_resource_free(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)(dev_hndl);
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&xdev->conf.pdev->dev);

	xpdev_device_cleanup(xpdev);
	xdev->conf.qsets_max = 0;
	xdev->conf.qsets_base = -1;
}
#endif

#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
static void qdma_reset_prepare(struct pci_dev *pdev)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);
	struct xlnx_dma_dev *xdev = NULL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);
	pr_info("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);

	qdma_device_offline(pdev, xpdev->dev_hndl, XDEV_FLR_ACTIVE);

	/* FLR setting is required for Versal Hard IP */
	if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
		qdma_device_flr_quirk_set(pdev, xpdev->dev_hndl);
	xpdev_queue_delete_all(xpdev);
	xpdev_device_cleanup(xpdev);
	xdev->conf.qsets_max = 0;
	xdev->conf.qsets_base = -1;

	if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
		qdma_device_flr_quirk_check(pdev, xpdev->dev_hndl);
}

static void qdma_reset_done(struct pci_dev *pdev)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	pr_info("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);
	qdma_device_online(pdev, xpdev->dev_hndl, XDEV_FLR_ACTIVE);
}

#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static void qdma_reset_notify(struct pci_dev *pdev, bool prepare)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);
	struct xlnx_dma_dev *xdev = NULL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	pr_info("%s prepare %d, pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), prepare, pdev, xpdev, xpdev->dev_hndl,
		xpdev->idx);

	if (prepare) {
		qdma_device_offline(pdev, xpdev->dev_hndl, XDEV_FLR_ACTIVE);
		/* FLR setting is not required for 2018.3 IP */
		if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
			qdma_device_flr_quirk_set(pdev, xpdev->dev_hndl);
		xpdev_queue_delete_all(xpdev);
		xpdev_device_cleanup(xpdev);
		xdev->conf.qsets_max = 0;
		xdev->conf.qsets_base = -1;

		if (xdev->version_info.ip_type == QDMA_VERSAL_HARD_IP)
			qdma_device_flr_quirk_check(pdev, xpdev->dev_hndl);
	} else
		qdma_device_online(pdev, xpdev->dev_hndl, XDEV_FLR_ACTIVE);
}
#endif
static const struct pci_error_handlers qdma_err_handler = {
	.error_detected	= qdma_error_detected,
	.slot_reset	= qdma_slot_reset,
	.resume		= qdma_error_resume,
#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
	.reset_prepare  = qdma_reset_prepare,
	.reset_done     = qdma_reset_done,
#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	.reset_notify   = qdma_reset_notify,
#endif
};

static struct pci_driver pci_driver = {
	.name = DRV_MODULE_NAME,
	.id_table = pci_ids,
	.probe = probe_one,
	.remove = remove_one,
/*	.shutdown = shutdown_one, */
#if defined(CONFIG_PCI_IOV) && !defined(__QDMA_VF__)
	.sriov_configure = sriov_config,
#endif
	.err_handler = &qdma_err_handler,
};

static int __init qdma_mod_init(void)
{
	int rv;

	pr_info("%s", version);

	rv = libqdma_init(num_threads, NULL);
	if (rv < 0)
		return rv;

	rv = xlnx_nl_init();
	if (rv < 0)
		return rv;

	rv = qdma_cdev_init();
	if (rv < 0)
		return rv;

	return pci_register_driver(&pci_driver);
}

static void __exit qdma_mod_exit(void)
{
	/* unregister this driver from the PCI bus driver */
	pci_unregister_driver(&pci_driver);

	xlnx_nl_exit();

	qdma_cdev_cleanup();

	libqdma_exit();
}

module_init(qdma_mod_init);
module_exit(qdma_mod_exit);
