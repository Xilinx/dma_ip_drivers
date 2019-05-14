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

/* include early, to verify it depends only on the headers above */
#include "version.h"

static char version[] =
	DRV_MODULE_DESC " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

/* Module Parameters */
/** 0 - AUTO_MODE,
 *  1 - POLL_MODE,
 *  2 - DIRECT_INTR_MODE,
 *  3 - INDIRECT_INTR_MODE,
 *  4 - LEGACY_INTR_MODE
 */
static unsigned int mode = 0;
module_param(mode, uint, 0644);
MODULE_PARM_DESC(mode,
"use hw polling instead of interrupts for determining dma transfer completion");

static unsigned int master_pf[MAX_DMA_DEV] = {0};
static unsigned int master_cnt;
module_param_array(master_pf, uint, &master_cnt, 0644);
MODULE_PARM_DESC(master_pf, "Master PF for setting global CSRs, dflt PF 0");

static unsigned int config_bar[MAX_DMA_DEV] = {0};
static unsigned int config_bar_cnt;
module_param_array(config_bar, uint, &config_bar_cnt, 0644);
MODULE_PARM_DESC(config_bar, "Config bar number for all devices , dflt 0");

static unsigned int num_threads = 0;
module_param(num_threads, uint, 0644);
MODULE_PARM_DESC(num_threads,
"Number of threads to be created each for request and writeback processing");

static unsigned int tm_mode_en = 0;
module_param(tm_mode_en, uint, 0644);
MODULE_PARM_DESC(tm_mode_en,
        "Enable Traffic Manager mode for bypass ST H2C xmit. Default disabled");

static unsigned int tm_one_cdh_en = 0;
module_param(tm_one_cdh_en, uint, 0644);
MODULE_PARM_DESC(tm_one_cdh_en,
                "Enable 1 CDH for Traffic Manager mode. Default is Zero CDH");

#include "pci_ids.h"

/*
 * xpdev helper functions
 */
static LIST_HEAD(xpdev_list);
static DEFINE_MUTEX(xpdev_mutex);

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
	len = sprintf(buf, "%d\n", rngsz);
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
		pr_info("failed to set interrupt ring size\n");
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
	len = sprintf(buf, "%d\n", qmax);
	if (len <= 0)
		pr_err("copying qmax to buf failed with err: %d\n", len);

	return len;
}

#ifndef __QDMA_VF__
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
static ssize_t show_qmax_vfs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len;
	int vf_qmax = qconf_get_qmax(PCI_TYPE_VF, 0);

	len = sprintf(buf, "%d\n", vf_qmax);
	if (len <= 0)
		pr_err("copying qmax_vfs to buf failed with err: %d\n", len);

	return len;
}

/*****************************************************************************/
/**
 * qdma_reconfigure_qmax() -  handler to set the qmax configuration value
 *
 * @xpdev :   PCIe device handle for
 * @qmax_new :   new value for qmax
 * @conf_head: pointer to hold the device configuration
 *
 * Helper function to reconfigure qmax values using sysfs/qmax interface for
 * the xlnx_pci_dev passed as parameter.  Will change the starting of the
 * subsequent PF Qs based on the modified qmax value of the xpdev. For
 * preceeding devices, no changes will happen
 *
 * @note    none
 *
 * Return:  Returns number of devices succesfully configured on success,
 * 		<0 on failure
 *
 *****************************************************************************/
static int qdma_prep_reconfigure(struct xlnx_pci_dev *xpdev,
		int qmax_new, struct qdma_dev_conf **conf_head)
{
	int reconfig_ok = 0;
	unsigned int used_qs = 0;
	unsigned int cfg_pending = 0;
	unsigned int cfg_pending_qcnt = 0;
	int err = 0;
	int next_qbase = 0;
	int total_pf = 0;
	int count = 0;
	struct xlnx_pci_dev *_xpdev, *tmp;
	struct qdma_dev_conf *_conf_head = NULL;
	struct qdma_dev_conf *conf_p = NULL;
	int last_bus = 0;
	int last_dev = 0;
	int max_pf_qs = qconf_get_qmax(PCI_TYPE_PF, 0);

	mutex_lock(&xpdev_mutex);
	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		struct qdma_dev_conf conf;
		/** found a new card? */
		if ((last_bus | last_dev) &&
			((last_bus != _xpdev->pdev->bus->number) ||
			(last_dev !=  PCI_SLOT(_xpdev->pdev->devfn))))
			break;

		err = qdma_device_get_config(_xpdev->dev_hndl, &conf, NULL, 0);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%05x'\n",
					_xpdev->idx);
			reconfig_ok = err;
			goto errout_reconfig;
		}

		if (conf.cur_cfg_state == QMAX_CFG_USER) {
			if (xpdev != _xpdev)
				used_qs += conf.qsets_max;
			else {
				if (qmax_new == conf.qsets_max) {
					pr_info("WARN! New qmax(%d) same as current(%d)\n",
							qmax_new,
							conf.qsets_max);
					reconfig_ok = 0;
					goto errout_reconfig;
				}
			}
		} else if (conf.cur_cfg_state == QMAX_CFG_INITIAL)
			if (xpdev != _xpdev)
				cfg_pending++;
		total_pf++;
		last_bus = _xpdev->pdev->bus->number;
		last_dev = PCI_SLOT(_xpdev->pdev->devfn);
	}
	if ((used_qs + qmax_new) > max_pf_qs) {
		pr_err("Not enough qs to allocate for qdma%05x\n",
				_xpdev->idx);
		pr_err("Requested size = %d, remaining size=%d\n",
				qmax_new, max_pf_qs - used_qs);
		reconfig_ok = -EINVAL;
		goto errout_reconfig;
	}

	_conf_head = (struct qdma_dev_conf *)kzalloc(total_pf * sizeof(struct qdma_dev_conf),
			GFP_KERNEL);
	if (!_conf_head) {
		reconfig_ok = -ENOMEM;
		goto errout_reconfig;
	}

	if (cfg_pending)
		cfg_pending_qcnt = (max_pf_qs - (used_qs + qmax_new))/cfg_pending;

	count = total_pf;
	conf_p = _conf_head;
	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		err = qdma_device_get_config(_xpdev->dev_hndl, conf_p, NULL, 0);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%05x'\n", _xpdev->idx);
			reconfig_ok = -1;
			goto errout_reconfig;
		}

		conf_p->qsets_base = next_qbase;
		if (_xpdev == xpdev) {
			conf_p->qsets_max = qmax_new;
			conf_p->cur_cfg_state = QMAX_CFG_USER;
		} else {
			if (conf_p->cur_cfg_state == QMAX_CFG_INITIAL)
				conf_p->qsets_max = cfg_pending_qcnt;
		}
		next_qbase = conf_p->qsets_max + conf_p->qsets_base;
		/* if 0qs make qbase to 0 */
		if (!conf_p->qsets_max)
			conf_p->qsets_base = 0;
		conf_p++;
		/** we have to configure 'total_pf' PF devices  */
		if (!(count--))
			break;
	}

	*conf_head = _conf_head;

	reconfig_ok = total_pf;

errout_reconfig:
	mutex_unlock(&xpdev_mutex);

	if ((reconfig_ok < 0) && (_conf_head != NULL))
		kfree(_conf_head);

	return reconfig_ok;
}

static int qdma_sysfs_reconfigure(void)
{
	int reconfig_ok = 0;
	unsigned int qcnt = 0;
	int err = 0;
	int next_qbase = 0;
	int total_pf = 0;
	int cfgd_pf = 0;
	int count = 0;
	struct xlnx_pci_dev *_xpdev, *tmp;
	int last_bus = 0;
	int last_dev = 0;
	int max_pf_qs = qconf_get_qmax(PCI_TYPE_PF, 0);
	int total_qcnt_user = 0;
	int qcnt_remaining = 0;

	mutex_lock(&xpdev_mutex);
	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		struct qdma_dev_conf conf;
		/** found a new card? */
		if ((last_bus | last_dev) &&
			((last_bus != _xpdev->pdev->bus->number) ||
			(last_dev !=  PCI_SLOT(_xpdev->pdev->devfn))))
			break;

		err = qdma_device_get_config(_xpdev->dev_hndl, &conf, NULL, 0);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%d'\n",
					_xpdev->idx);
			reconfig_ok = err;
			goto errout_reconfig;
		}

		if (conf.cur_cfg_state == QMAX_CFG_USER) {
			total_qcnt_user += conf.qsets_max;
		} else
			cfgd_pf++;

		total_pf++;
		last_bus = _xpdev->pdev->bus->number;
		last_dev = PCI_SLOT(_xpdev->pdev->devfn);
	}

	qcnt_remaining = max_pf_qs - total_qcnt_user;
	if (qcnt_remaining < 0) {
		pr_err("Sysfs reconfiguration of vf-max failed!!!\n");
		reconfig_ok = -EPERM;
		goto errout_reconfig;
	}

	if (cfgd_pf)
		qcnt = qcnt_remaining / cfgd_pf;

	count = total_pf;

	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		struct qdma_dev_conf conf;

		err = qdma_device_get_config(_xpdev->dev_hndl, &conf, NULL, 0);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%d'\n",
					_xpdev->idx);
			reconfig_ok = err;
			goto errout_reconfig;
		}

		if (conf.cur_cfg_state == QMAX_CFG_INITIAL)
			conf.qsets_max = qcnt;

		if (conf.qsets_max)
			conf.qsets_base = next_qbase;
		else
			conf.qsets_base = 0;
		next_qbase += conf.qsets_max;

		err = qdma_device_set_config(_xpdev->dev_hndl, &conf);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%d'\n", _xpdev->idx);
			reconfig_ok = err;
			goto errout_reconfig;
		}

		err = qdma_set_qmax(_xpdev->dev_hndl, conf.qsets_max, 1);
		if (err < 0) {
			pr_err("Failed to alter qmax for qdma%u %s max QP: %u, %u~%u\n",
					_xpdev->idx, dev_name(&conf.pdev->dev),
					conf.qsets_max, conf.qsets_base,
					conf.qsets_base + conf.qsets_max - 1);
			reconfig_ok = err;
			goto errout_reconfig;
		}

		/** we have to configure 'total_pf' PF devices  */
		if (!(count--))
			break;
	}

errout_reconfig:
	mutex_unlock(&xpdev_mutex);

	return reconfig_ok;
}

static int qdma_reconfigure_qmax(struct xlnx_pci_dev *xpdev,
		struct qdma_dev_conf *conf_head)
{
	int err = 0, qmax = 0, count = 0;
	struct xlnx_pci_dev *_xpdev, *tmp;
	struct qdma_dev_conf *conf_p = conf_head;

	if (!conf_head)
		return -EINVAL;

	mutex_lock(&xpdev_mutex);
	/** start from the device we want to alter the qmax value
	 * TODO: failure will take the device to an inconsistant
	 * state and how to handle it.
	 */
	list_for_each_entry_safe(_xpdev, tmp,  &xpdev_list, list_head) {

		err = qdma_device_set_config(_xpdev->dev_hndl, conf_p);
		if (err < 0) {
			pr_err("Failed to get conf for qdma device '%05x'\n", _xpdev->idx);
			break;
		}
		qmax = conf_p->qsets_max;
		err = qdma_set_qmax(_xpdev->dev_hndl, qmax, 1);
		if (err < 0) {
			pr_err("Failed to alter qmax for qdma%05x %s max QP: %u, %u~%u\n",
					_xpdev->idx, dev_name(&conf_p->pdev->dev),
					conf_p->qsets_max, conf_p->qsets_base,
					conf_p->qsets_base + conf_p->qsets_max - 1);
			break;
		}
		if (_xpdev == xpdev)
			qdma_device_set_cfg_state(_xpdev->dev_hndl, QMAX_CFG_USER);

		conf_p++;
		count++;
	}
	mutex_unlock(&xpdev_mutex);

	if (!err)
		pr_debug("Succesfully reconfigured '%d' devices\n", count);

	kfree(conf_head);

	return err;
}

static ssize_t set_qmax(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_dev_conf *conf_head = NULL;
	unsigned int qmax = 0;
	int err = 0;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	err = kstrtouint(buf, 0, &qmax);
	if (err < 0) {
		pr_info("failed to set qmax to %d\n", qmax);
		return err;
	}

	if (qmax > qconf_get_qmax(PCI_TYPE_PF, 0)) {
		pr_err("Invalid qmax %u (max = %u)\n",
				qmax,
				qconf_get_qmax(PCI_TYPE_PF, 0));
		return -EINVAL;
	}

	err = qdma_prep_reconfigure(xpdev, qmax, &conf_head);
	if (!(err < 0))
		err = qdma_reconfigure_qmax(xpdev, conf_head);

	return err ? err : count;
}

/*****************************************************************************/
/**
 * funcname() -  handler to set the qmax_vf configuration value
 *
 * @dev :   PCIe device handle
 * @attr:   qmax_vf configuration value
 * @buf :   buffer to hold the configured value
 * @count : the number of bytes of data in the buffer
 *
 * Handler function to set the qmax_vf
 *
 * @note    none
 *
 * Return:  Returns length of the buffer on success, <0 on failure
 *
 *****************************************************************************/
static ssize_t set_qmax_vfs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int qmax = 0;
	int err = 0;
	int last_qmax = 0;

	if (!is_vfqmax_configurable()) {
		pr_err("Setting VF-qmax failed, VFs already registered.");
		return -EPERM;
	}

	err = kstrtoint(buf, 0, &qmax);
	if (err < 0) {
		pr_err("Failed to set qmax VFs\n");
		return err;
	}

	if (qmax > TOTAL_QDMA_QS) {
		pr_err("Invalid vf qmax %d\n", qmax);
		return -EINVAL;
	}

	last_qmax = set_vf_qmax(qmax);
	err = qdma_sysfs_reconfigure();
	if (err < 0)
		set_vf_qmax(last_qmax);

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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int len;
	unsigned int cmpl_status_acc = 0;

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	cmpl_status_acc = qdma_get_cmpl_status_acc(xpdev->dev_hndl);
	len = sprintf(buf, "%d\n", cmpl_status_acc);
	if (len <= 0)
		pr_err("copying cmpl status acc value to buf failed with err: %d\n", len);

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
		pr_err("failed to set cmpl status accumulator to %d\n", cmpl_status_acc);
		return err;
	}

	err = qdma_set_cmpl_status_acc(xpdev->dev_hndl, cmpl_status_acc);
	return err ? err : count;
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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += sprintf(buf + len, "%hu ", c2h_buf_sz[i]);
	len += sprintf(buf + len, "%s", "\n");

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
 	** This helps to restore the values of entries if
	** user configures lesser than 16 values
	*/
	qdma_get_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

	while ((p = strsep(&s, tc)) != NULL && i < QDMA_GLOBAL_CSR_ARRAY_SZ) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_buf_sz[i]);
		if (err < 0)
			goto input_err;

		/* Check if buf_sz is configured as power of 2 */
		if (!(c2h_buf_sz[i]) || ((c2h_buf_sz[i]) & (c2h_buf_sz[i]-1))) {
			pr_warn("buf_sz at index %d is not power of 2\n", i);
			err = -EINVAL;
			goto input_err;
		}
		i++;
	}

	if (p) {
		/*
		check if the number of entries are more than 16 !
		if yes, ignore the extra values
		*/
		pr_warn("Found more than 16 entries. Ignoring extra entries\n");
	}

	err = qdma_set_buf_sz(xpdev->dev_hndl, c2h_buf_sz);

input_err:
	return err ? err : count;
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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int glbl_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_glbl_rng_sz(xpdev->dev_hndl, glbl_ring_sz);

	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += sprintf(buf + len, "%hu ", glbl_ring_sz[i]);
	len += sprintf(buf + len, "%s", "\n");

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
 	** This helps to restore the values of entries if
	** user configures lesser than 16 values
	*/
	qdma_get_glbl_rng_sz(xpdev->dev_hndl, glbl_ring_sz);

	while ((p = strsep(&s, tc)) != NULL && i < QDMA_GLOBAL_CSR_ARRAY_SZ) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &glbl_ring_sz[i]);
		if (err < 0)
			goto input_err;

		/* Check if rng_sz is configured as power of 2 */
		if ((glbl_ring_sz[i]) & (glbl_ring_sz[i]-1)) {
			pr_warn("glbl_ring_sz at index %d is not power of 2\n", i);
			err = -EINVAL;
			goto input_err;
		}

		/* Always add one for the completion status */
		glbl_ring_sz[i] += 1;

		i++;
	}

	if (p) {
		/*
		check if the number of entries are more than 16 !
		if yes, ignore the extra values
		*/
		pr_warn("Found more than 16 entries. Ignoring extra entries\n");
	}

	err = qdma_set_glbl_rng_sz(xpdev->dev_hndl, glbl_ring_sz);

input_err:
	return err ? err : count;
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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += sprintf(buf + len, "%hhu ", c2h_timer_cnt[i]);
	len += sprintf(buf + len, "%s", "\n");

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
 	** This helps to restore the values of entries if
	** user configures lesser than 16 values
	*/
	qdma_get_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

	while ((p = strsep(&s, tc)) != NULL && i < QDMA_GLOBAL_CSR_ARRAY_SZ) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_timer_cnt[i]);
		if (err < 0)
			goto input_err;

		if (c2h_timer_cnt[i] < 0 || c2h_timer_cnt[i] > 255) {
			pr_warn("timer cnt at index %d is %d - out of range [0-255]\n",
				i, c2h_timer_cnt[i]);
			err = -EINVAL;
			goto input_err;
		}
		i++;
	}

	if (p) {
		/*
		check if the number of entries are more than 16 !
		if yes, ignore the extra values
		*/
		pr_warn("Found more than 16 entries. Ignoring extra entries\n");
	}
	err = qdma_set_timer_cnt(xpdev->dev_hndl, c2h_timer_cnt);

input_err:
	return err ? err : count;
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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *xpdev;
	int len = 0;
	int i;
	unsigned int c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};

	if (!pdev)
		return -EINVAL;

	xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!xpdev)
		return -EINVAL;

	qdma_get_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		len += sprintf(buf + len, "%hhu ", c2h_cnt_th[i]);
	len += sprintf(buf + len, "%s", "\n");

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
 	** This helps to restore the values of entries if
	** user configures lesser than 16 values
	*/
	qdma_get_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

	while ((p = strsep(&s, tc)) != NULL && i < QDMA_GLOBAL_CSR_ARRAY_SZ) {
		if (*p == 0)
			continue;

		err = kstrtoint(p, 0, &c2h_cnt_th[i]);
		if (err < 0)
			goto input_err;

		if (c2h_cnt_th[i] < 0 || c2h_cnt_th[i] > 255) {
			pr_warn("counter threshold at index %d is %d - out of range [0-255]\n",
				i, c2h_cnt_th[i]);
			err = -EINVAL;
			goto input_err;
		}
		i++;
	}

	if (p) {
		/*
		check if the number of entries are more than 16 !
		if yes, ignore the extra values
		*/
		pr_warn("Found more than 16 entries. Ignoring extra entries\n");
	}
	err = qdma_set_cnt_thresh(xpdev->dev_hndl, c2h_cnt_th);

input_err:
	return err ? err : count;
}

/**
 * Function to find the first PF device available in the card
 */
static bool is_first_pfdev(u32 bdf)
{
	struct xlnx_pci_dev *_xpdev, *tmp;
	u32 bdf_tmp = 0;

	mutex_lock(&xpdev_mutex);
	if (list_empty(&xpdev_list)) {
		mutex_unlock(&xpdev_mutex);
		return 1;
	}

	list_for_each_entry_safe(_xpdev, tmp, &xpdev_list, list_head) {
		struct pci_dev *pdev = _xpdev->pdev;
		bdf_tmp = (pdev->bus->number << PCI_SHIFT_BUS)
			| (PCI_SLOT(pdev->devfn) << PCI_SHIFT_DEV)
			| PCI_FUNC(pdev->devfn);
		/** find first bus and device are matching */
		if ((bdf & ~0xf) == (bdf_tmp & ~0xf)) {
			mutex_unlock(&xpdev_mutex);
			/** if func matches, it returns 1, else 0*/
			return (PCI_FUNC(pdev->devfn) == (bdf & 0xf));
		}
	}
	mutex_unlock(&xpdev_mutex);

	return 0;
}

/**
 * function to find out master pf, if the parameter master_pf is passed.
 * By default it will be PF0
 */
static u8 is_master_pf(struct pci_dev *pdev)
{
	int i = 0;
	u32 bdf = (pdev->bus->number << PCI_SHIFT_BUS)
		| (PCI_SLOT(pdev->devfn) << PCI_SHIFT_DEV)
		| PCI_FUNC(pdev->devfn);
	/**
	 * no arguments are passed for master_pf, default to PF0
	 * arguments passed but bus and device not in list,
	 * default to PF0
	 */
	if (!master_cnt) {
		if (is_first_pfdev(bdf))
			return 1;
	} else {
		for (i = 0; i < master_cnt; i++) {
			if ((bdf & ~0xf) == (master_pf[i] & ~0xf)) {
				if ((master_pf[i] & 0xf) == (bdf & 0xf))
					return 1;
				else
					return 0;
			}
		}

		if (is_first_pfdev(bdf))
			return 1;
	}

	return 0;
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
	struct pci_dev *pdev = to_pci_dev(dev);
	struct xlnx_pci_dev *_xpdev;
	unsigned int qmax = 0;
	int err = 0;

	if (!pdev)
		return -EINVAL;

	_xpdev = (struct xlnx_pci_dev *)dev_get_drvdata(dev);
	if (!_xpdev)
		return -EINVAL;
	err = kstrtoint(buf, 0, &qmax);
	if (err < 0) {
		pr_err("Failed to set qmax\n");
		return err;
	}


	if (qmax > TOTAL_QDMA_QS) {
		pr_err("Invalid vf qmax %d\n", qmax);
		return -EINVAL;
	}

	err = qdma_vf_qconf(_xpdev->dev_hndl, qmax);
	if (err < 0)
		return err;

	if (!err)
		pr_debug("Succesfully reconfigured qdmavf%05x\n", _xpdev->idx);

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
static DEVICE_ATTR(cmpl_status_acc, S_IWUSR | S_IRUGO, show_cmpl_status_acc, set_cmpl_status_acc);
static DEVICE_ATTR(qmax_vfs, S_IWUSR | S_IRUGO, show_qmax_vfs, set_qmax_vfs);
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
		&dev_attr_qmax_vfs.attr,
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
#ifdef __QDMA_VF__
		if ((conf.qsets_base == 0) 
				&& (conf.qsets_max == 1)
				&& (conf.cur_cfg_state == QMAX_CFG_INITIAL))
			qmax_val = 0;
		else
			qmax_val = conf.qsets_max;
#else
			qmax_val = conf.qsets_max;
#endif

		cur += snprintf(cur, end - cur,
#ifdef __QDMA_VF__
			"qdmavf%05x\t%s\tmax QP: %u, %u~%u\n",
#else
			"qdma%05x\t%s\tmax QP: %u, %u~%u\n",
#endif
			xpdev->idx, dev_name(&pdev->dev),
			qmax_val, conf.qsets_base,
			base_end);
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

static void xpdev_unmap_user_bypass_bars(struct xlnx_pci_dev *xpdev)
{
	if (xpdev->user_bar_regs) {
		/* unmap BAR */
		pci_iounmap(xpdev->pdev, xpdev->user_bar_regs);
		/* mark as unmapped */
		xpdev->user_bar_regs = NULL;
	}

	if (xpdev->bypass_bar_regs) {
		/* unmap BAR */
		pci_iounmap(xpdev->pdev, xpdev->bypass_bar_regs);
		/* mark as unmapped */
		xpdev->bypass_bar_regs = NULL;
	}

}

/*****************************************************************************/
/**
 * identify_config_bar() - Function to identify the config bar of the PCIE device
 *
 * @param[in]	pdev:		pcie device
 *
 * @return	config bar
 *****************************************************************************/
static u8 identify_config_bar(struct pci_dev *pdev)
{
	int i = 0;
	u8 bus_num = pdev->bus->number;
#ifndef __QDMA_VF__
	u8 dev_fn = PCI_FUNC(pdev->devfn);
#else
	u16 device_id = pdev->device;
#endif
#ifndef __QDMA_VF__
	if (config_bar_cnt) {
		for (i = 0; i < MAX_DMA_DEV; i++) {
			if (((config_bar[i] & BUS_NUM_MASK) >> BUS_NUM_SHIFT) == bus_num) {
				switch (dev_fn) {
					case PF_DEVICE_0:
						return ((config_bar[i] & PF_DEV_0_MASK)
								>> PF_DEV_0_SHIFT);
					case PF_DEVICE_1:
						return ((config_bar[i] & PF_DEV_1_MASK)
								>> PF_DEV_1_SHIFT);
					case PF_DEVICE_2:
						return ((config_bar[i] & PF_DEV_2_MASK)
								>> PF_DEV_2_SHIFT);
					case PF_DEVICE_3:
						return ((config_bar[i] & PF_DEV_3_MASK)
								>> PF_DEV_3_SHIFT);
				}
			}
		}
	}

	return QDMA_CONFIG_BAR;
#else
	if (config_bar_cnt) {
		for (i = 0; i < MAX_DMA_DEV; i++) {
			if (((config_bar[i] & BUS_NUM_MASK) >> BUS_NUM_SHIFT) == bus_num) {
				switch ((device_id >> VF_PF_IDENTIFIER_SHIFT) &
						VF_PF_IDENTIFIER_MASK) {
					case PF_DEVICE_0:
						return ((config_bar[i] & PF_DEV_0_MASK)
								>> PF_DEV_0_SHIFT);
					case PF_DEVICE_1:
						return ((config_bar[i] & PF_DEV_1_MASK)
								>> PF_DEV_1_SHIFT);
					case PF_DEVICE_2:
						return ((config_bar[i] & PF_DEV_2_MASK)
								>> PF_DEV_2_SHIFT);
					case PF_DEVICE_3:
						return ((config_bar[i] & PF_DEV_3_MASK)
								>> PF_DEV_3_SHIFT);
				}
			}
		}
	}

	return QDMA_CONFIG_BAR;
#endif
}

static int xpdev_map_user_bypass_bars(struct xlnx_pci_dev *xpdev)
{
	struct xlnx_dma_dev *xdev = NULL;
	int map_len;

	if (!xpdev)
		return -EINVAL;

	xdev = (struct xlnx_dma_dev *)(xpdev->dev_hndl);

	if (xdev->conf.bar_num_user < 0) {
		pr_err("User and bypass bar is not present \n");
		return -EINVAL;
	}

	map_len = pci_resource_len(xpdev->pdev, (int)xdev->conf.bar_num_user);
	if (map_len > QDMA_MAX_BAR_LEN_MAPPED)
		map_len = QDMA_MAX_BAR_LEN_MAPPED;

	xpdev->user_bar_regs = pci_iomap(xpdev->pdev, xdev->conf.bar_num_user, map_len);
	if (!xpdev->user_bar_regs) {
		pr_err("unable to map user bar %d.\n", xdev->conf.bar_num_user);
		return -EINVAL;
	}

	if (xdev->conf.bar_num_bypass >= 0) {
		/* Mapping bypass bar */
		map_len = pci_resource_len(xpdev->pdev, (int)xdev->conf.bar_num_bypass);
		if (map_len > QDMA_MAX_BAR_LEN_MAPPED)
			map_len = QDMA_MAX_BAR_LEN_MAPPED;

		xpdev->bypass_bar_regs = pci_iomap(xpdev->pdev, xdev->conf.bar_num_bypass, map_len);
		if (!xpdev->bypass_bar_regs) {
			pr_err("unable to map bypass bar %d.\n", xdev->conf.bar_num_bypass);
			return -EINVAL;
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
			unsigned int qidx, bool c2h, bool check_qhndl,
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
	if (c2h)
		qdata += xpdev->qmax;

	if (check_qhndl && (!qdata->qhndl && !qdata->xcdev)) {
		pr_debug("qdma%05x QID %u NOT configured.\n", xpdev->idx, qidx);
		if (ebuf && ebuflen) {
			snprintf(ebuf, ebuflen, "QID %u NOT configured.\n", qidx);
		}

		return NULL;
	}

	return qdata;
}

int xpdev_queue_delete(struct xlnx_pci_dev *xpdev, unsigned int qidx, bool c2h,
			char *ebuf, int ebuflen)
{
	struct xlnx_qdata *qdata = xpdev_queue_get(xpdev, qidx, c2h, 1, ebuf,
						ebuflen);
	int rv = 0;

	if (!qdata)
		return -EINVAL;

	if(!qdata->xcdev)
		return -EINVAL;

	spin_lock(&xpdev->cdev_lock);
	qdata->xcdev->dir_init &= ~(1 << (c2h ? 1 : 0));
	if (qdata->xcdev && !qdata->xcdev->dir_init)
		qdma_cdev_destroy(qdata->xcdev);
	spin_unlock(&xpdev->cdev_lock);

	if (qdata->qhndl != QDMA_QUEUE_IDX_INVALID)
		rv = qdma_queue_remove(xpdev->dev_hndl, qdata->qhndl,
					ebuf, ebuflen);
	else
		pr_debug("qidx %u/%u, c2h %d, qhndl invalid.\n",
			qidx, xpdev->qmax, c2h);
	if (rv < 0)
		goto exit;

	memset(qdata, 0, sizeof(*qdata));
exit:
	return rv;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
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
	struct qdma_cdev *xcdev;
	struct xlnx_qdata *qdata_tmp;
	struct qdma_dev_conf dev_config;
	u8 dir;
	unsigned long qhndl;
	int rv;

	rv = qdma_queue_add(xpdev->dev_hndl, qconf, &qhndl, ebuf, ebuflen);
	if (rv < 0)
		return rv;

	pr_debug("qdma%05x idx %u, st %d, c2h %d, added, qhndl 0x%lx.\n",
		xpdev->idx, qconf->qidx, qconf->st, qconf->c2h, qhndl);

	qdata = xpdev_queue_get(xpdev, qconf->qidx, qconf->c2h, 0, ebuf,
				ebuflen);
	if (!qdata) {
		pr_info("q added 0x%lx, get failed, idx 0x%x.\n",
			qhndl, qconf->qidx);
		return rv;
	}

	dir = qconf->c2h ? 0 : 1;
	spin_lock(&xpdev->cdev_lock);
	qdata_tmp = xpdev_queue_get(xpdev, qconf->qidx, dir, 0, NULL, 0);
	if (qdata_tmp) {
		/* only 1 cdev per queue pair */
		if (qdata_tmp->xcdev) {
			unsigned long *priv_data;

			qdata->qhndl = qhndl;
			qdata->xcdev = qdata_tmp->xcdev;
			priv_data = qconf->c2h ? &qdata->xcdev->c2h_qhndl :
					&qdata->xcdev->h2c_qhndl;
			*priv_data = qhndl;
			qdata->xcdev->dir_init |= (1 << qconf->c2h);

			spin_unlock(&xpdev->cdev_lock);
			return 0;
		}
	}
	spin_unlock(&xpdev->cdev_lock);

	rv = qdma_device_get_config(xpdev->dev_hndl, &dev_config, NULL, 0);
	if (rv < 0) {
		pr_err("Failed to get conf for qdma device '%05x'\n",
				xpdev->idx);
		return rv;
	}

	/* always create the cdev
	 * Give HW QID as minor number with qsets_base calculation */
	rv = qdma_cdev_create(&xpdev->cdev_cb, xpdev->pdev, qconf,
			(dev_config.qsets_base + qconf->qidx),
			qhndl, &xcdev, ebuf, ebuflen);

	qdata->qhndl = qhndl;
	qdata->xcdev = xcdev;

	return rv;
}

static void nl_work_handler_q_start(struct work_struct *work)
{
	struct xlnx_nl_work *nl_work = container_of(work, struct xlnx_nl_work,
						work);
	struct xlnx_pci_dev *xpdev = nl_work->xpdev;
	struct xlnx_nl_work_q_ctrl *qctrl = &nl_work->qctrl;
	char ebuf[XNL_EBUFLEN] = {'\0'};
	unsigned int qidx = qctrl->qidx;
	u8 is_qp = qctrl->is_qp;
	u8 c2h = qctrl->is_c2h;
	int i;

	for (i = 0; i < qctrl->qcnt; i++, qidx++) {
		int rv;
		struct xlnx_qdata *qdata;

q_start:
		qdata = xpdev_queue_get(xpdev, qidx, c2h, 1, ebuf, XNL_EBUFLEN);
		if (!qdata) {
			pr_info("%s, idx %u, c2h %u, get failed.\n",
				dev_name(&xpdev->pdev->dev), qidx, c2h);
			snprintf(ebuf, XNL_EBUFLEN,
				"Q idx %u, c2h %u, get failed.\n", qidx, c2h);
			goto send_resp;
		}

		rv = qdma_queue_start(xpdev->dev_hndl, qdata->qhndl, ebuf,
					XNL_EBUFLEN);
		if (rv < 0) {
			pr_info("%s, idx %u, c2h %u, start failed %d.\n",
				dev_name(&xpdev->pdev->dev), qidx, c2h, rv);
			snprintf(ebuf, XNL_EBUFLEN,
				"Q idx %u, c2h %u, start failed %d.\n",
				qidx, c2h, rv);
			goto send_resp;
		}

#ifndef __QDMA_VF__
		{
			struct xlnx_dma_dev *xdev =
				(struct xlnx_dma_dev *)(xpdev->dev_hndl);

			if (xdev->stm_en) {
				rv = qdma_queue_prog_stm(xpdev->dev_hndl,
							 qdata->qhndl,
							 ebuf, XNL_EBUFLEN);
				if (rv < 0) {
					pr_info("%s, idx %u, c2h %u, prog stm failed %d.\n",
						dev_name(&xpdev->pdev->dev),
						qidx, c2h, rv);
					snprintf(ebuf, XNL_EBUFLEN,
						 "Q idx %u, c2h %u, prog stm failed %d.\n",
						 qidx, c2h, rv);
					goto send_resp;
				}
			}
		}
#endif

		if (is_qp && c2h == qctrl->is_c2h) {
			c2h = !qctrl->is_c2h;
			goto q_start;
		}

		c2h = qctrl->is_c2h;
	}

	snprintf(ebuf, XNL_EBUFLEN,
		 "%u Queues started, idx %u ~ %u.\n",
		qctrl->qcnt, qctrl->qidx, qidx - 1);

send_resp:
	spin_lock(&nl_work->lock);
	nl_work->q_start_handled = 1;
	spin_unlock(&nl_work->lock);
	xnl_respond_buffer(&nl_work->nl_info, ebuf, strlen(ebuf));
	wake_up_interruptible(&nl_work->wq);
}

static struct xlnx_nl_work *xpdev_nl_work_alloc(struct xlnx_pci_dev *xpdev,
						struct genl_info *nl_info)
{
	struct xlnx_nl_work *nl_work;

	/* allocate work struct */
	nl_work = kmalloc(sizeof(*nl_work), GFP_ATOMIC);
	if (!nl_work) {
		pr_info("qdma%05x %s: OOM.\n",
			xpdev->idx, dev_name(&xpdev->pdev->dev));
		return NULL;
	}

	memcpy(&nl_work->nl_info, nl_info, sizeof(*nl_info));
	nl_work->xpdev = xpdev;

	return nl_work;
}

int xpdev_nl_queue_start(struct xlnx_pci_dev *xpdev, void *nl_info, u8 is_qp,
			u8 is_c2h, unsigned short qidx, unsigned short qcnt)
{
	struct xlnx_nl_work *nl_work = xpdev_nl_work_alloc(xpdev,
					(struct genl_info *)nl_info);
	struct xlnx_nl_work_q_ctrl *qctrl = &nl_work->qctrl;

	if (!nl_work)
		return -ENOMEM;

	qctrl->is_qp = is_qp;
	qctrl->is_c2h = is_c2h;
	qctrl->qidx = qidx;
	qctrl->qcnt = qcnt;

	INIT_WORK(&nl_work->work, nl_work_handler_q_start);
	init_waitqueue_head(&nl_work->wq);
	spin_lock_init(&nl_work->lock);
	nl_work->q_start_handled = 0;
	spin_lock(&nl_work->lock);
	queue_work(xpdev->nl_task_wq, &nl_work->work);
	spin_unlock(&nl_work->lock);
	wait_event_interruptible(nl_work->wq, nl_work->q_start_handled);
	kfree(nl_work);

	return 0;
}

static void xpdev_free(struct xlnx_pci_dev *p)
{
	xpdev_list_remove(p);

	if (p->nl_task_wq)
		destroy_workqueue(p->nl_task_wq);

	if (((unsigned long)p) >= VMALLOC_START &&
	    ((unsigned long)p) < VMALLOC_END)
		vfree(p);
	else
		kfree(p);
}

static struct xlnx_pci_dev *xpdev_alloc(struct pci_dev *pdev, unsigned int qmax)
{
	int sz = sizeof(struct xlnx_pci_dev) +
		qmax * 2 * sizeof(struct xlnx_qdata);
	struct xlnx_pci_dev *xpdev = kzalloc(sz, GFP_KERNEL);
	char name[80];

	if (!xpdev) {
		xpdev = vmalloc(sz);
		if (xpdev)
			memset(xpdev, 0, sz);
	}

	if (!xpdev) {
		pr_info("OMM, qmax %u, sz %u.\n", qmax, sz);
		return NULL;
	}
	spin_lock_init(&xpdev->cdev_lock);
	xpdev->pdev = pdev;
	xpdev->qmax = qmax;
	xpdev->idx = 0xFF;

	snprintf(name, 80, "qdma_%s_nl_wq", dev_name(&pdev->dev));
        xpdev->nl_task_wq = create_singlethread_workqueue(name);
        if (!xpdev->nl_task_wq) {
                pr_info("%s OOM, nl_task_wq.\n", dev_name(&pdev->dev));
                goto free_xpdev;
        }

	xpdev_list_add(xpdev);
	return xpdev;

free_xpdev:
	xpdev_free(xpdev);
	return NULL;
}

/*****************************************************************************/
/**
 * qdma_device_read_user_register() - read user bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 *
 * @return	value of the user bar register
 *****************************************************************************/
unsigned int qdma_device_read_user_register(struct xlnx_pci_dev *xpdev,
					unsigned int reg_addr)
{
	if (!xpdev)
		return -EINVAL;

	return readl(xpdev->user_bar_regs + reg_addr);
}

/*****************************************************************************/
/**
 * qdma_device_write_user_register() - write user bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[in]	val:		register value to be written
 *
 *****************************************************************************/
void qdma_device_write_user_register(struct xlnx_pci_dev *xpdev,
				unsigned int reg_addr, unsigned int val)
{
	if (!xpdev)
		return;

	writel(val, xpdev->user_bar_regs + reg_addr);
}

/*****************************************************************************/
/**
 * qdma_device_read_bypass_register() - read bypass bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 *
 * @return	value of the bypass bar register
 *****************************************************************************/
unsigned int qdma_device_read_bypass_register(struct xlnx_pci_dev *xpdev,
					unsigned int reg_addr)
{
	if (!xpdev)
		return -EINVAL;

	if (!xpdev->bypass_bar_regs) {
		pr_err("bypass bar is not present\n");
		return -EINVAL;
	}

	return readl(xpdev->bypass_bar_regs + reg_addr);
}

/*****************************************************************************/
/**
 * qdma_device_write_bypass_register() - write bypass bar register
 *
 * @param[in]	xpdev:		pointer to xlnx_pci_dev
 * @param[in]	reg_addr:	register address
 * @param[in]	val:		register value to be written
 *
 *****************************************************************************/
void qdma_device_write_bypass_register(struct xlnx_pci_dev *xpdev,
				unsigned int reg_addr, unsigned int val)
{
	if (!xpdev)
		return;

	if (!xpdev->bypass_bar_regs) {
		pr_err("bypass bar is not present\n");
		return;
	}

	writel(val, xpdev->bypass_bar_regs + reg_addr);
}

static int probe_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct qdma_dev_conf conf;
	struct xlnx_pci_dev *xpdev = NULL;
	unsigned long dev_hndl;
	int rv;

	pr_info("%s: func 0x%x/0x%x, p/v %d/%d,0x%p.\n",
		dev_name(&pdev->dev), PCI_FUNC(pdev->devfn), QDMA_PF_MAX,
		pdev->is_physfn, pdev->is_virtfn, pdev->physfn);

	memset(&conf, 0, sizeof(conf));
	conf.qdma_drv_mode = mode;
	conf.vf_max = 0;	/* enable via sysfs */

#ifdef __QDMA_VF__
	conf.qsets_max = TOTAL_QDMA_QS;
#else
	conf.master_pf = is_master_pf(pdev);
	if (conf.master_pf)
		pr_info("Configuring '%02x:%02x:%x' as master pf\n",
				pdev->bus->number,
				PCI_SLOT(pdev->devfn),
				PCI_FUNC(pdev->devfn));
#endif /* #ifdef __QDMA_VF__ */
	pr_info("Driver is loaded in %s mode\n",
				mode_name_list[conf.qdma_drv_mode].name);
	conf.intr_rngsz = QDMA_INTR_COAL_RING_SIZE;
	conf.tm_mode_en = tm_mode_en;
	conf.tm_one_cdh_en = tm_one_cdh_en;
	conf.pdev = pdev;

	/* initialize all the bar numbers with -1 */
	conf.bar_num_config = -1;
	conf.bar_num_user = -1;
	conf.bar_num_bypass = -1;

	conf.bar_num_config = identify_config_bar(pdev);

	rv = qdma_device_open(DRV_MODULE_NAME, &conf, &dev_hndl);
	if (rv < 0)
		return rv;

#ifndef __QDMA_VF__
	conf.qsets_max = TOTAL_PF_QS;
#endif

	xpdev = xpdev_alloc(pdev, conf.qsets_max);
	if (!xpdev) {
		rv = -EINVAL;
		goto close_device;
	}

	xpdev->dev_hndl = dev_hndl;
	xpdev->idx = conf.bdf;

	rv = xpdev_map_user_bypass_bars(xpdev);
	if (rv < 0)
		goto close_device;

	xpdev->cdev_cb.xpdev = xpdev;
	rv = qdma_cdev_device_init(&xpdev->cdev_cb);
	if (rv < 0)
		goto close_device;

	/* Create the files for attributes in sysfs */
	if (conf.master_pf) {
		rv = sysfs_create_group(&pdev->dev.kobj, &pci_master_device_attr_group);
		if (rv < 0)
			goto close_device;
	} else {
		rv = sysfs_create_group(&pdev->dev.kobj, &pci_device_attr_group);
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
		sysfs_remove_group(&pdev->dev.kobj, &pci_master_device_attr_group);
	else
		sysfs_remove_group(&pdev->dev.kobj, &pci_device_attr_group);

	qdma_cdev_device_cleanup(&xpdev->cdev_cb);

	xpdev_device_cleanup(xpdev);

	xpdev_unmap_user_bypass_bars(xpdev);

	qdma_device_close(pdev, xpdev->dev_hndl);

	xpdev_free(xpdev);

	dev_set_drvdata(&pdev->dev, NULL);
}

#if defined(CONFIG_PCI_IOV) && !defined(__QDMA_VF__)
static int sriov_config(struct pci_dev *pdev, int num_vfs)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	if (!xpdev) {
		pr_info("%s NOT attached.\n", dev_name(&pdev->dev));
		return -EINVAL;
	}

	pr_debug("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);

	if (num_vfs > QDMA_VF_MAX) {
		pr_info("%s, clamp down # of VFs %d -> %d.\n",
			dev_name(&pdev->dev), num_vfs, QDMA_VF_MAX);
		num_vfs = QDMA_VF_MAX;
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
	pci_cleanup_aer_uncorrect_error_status(pdev);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
static void qdma_reset_prepare(struct pci_dev *pdev)
{
	struct qdma_version_info version_info;
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	pr_info("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);

	qdma_device_version_info(xpdev->dev_hndl, &version_info);

	qdma_device_offline(pdev, xpdev->dev_hndl);

	/* FLR setting is not required for 2018.3 IP */
	if (version_info.vivado_release_id != VIVADO_RELEASE_2018_3)
		qdma_device_flr_quirk_set(pdev, xpdev->dev_hndl);
	xpdev_queue_delete_all(xpdev);
	if (version_info.vivado_release_id != VIVADO_RELEASE_2018_3)
	qdma_device_flr_quirk_check(pdev, xpdev->dev_hndl);
}

static void qdma_reset_done(struct pci_dev *pdev)
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	pr_info("%s pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), pdev, xpdev, xpdev->dev_hndl, xpdev->idx);
	qdma_device_online(pdev, xpdev->dev_hndl);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
static void qdma_reset_notify(struct pci_dev *pdev, bool prepare)
{
	struct qdma_version_info version_info;
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&pdev->dev);

	pr_info("%s prepare %d, pdev 0x%p, xdev 0x%p, hndl 0x%lx, qdma%05x.\n",
		dev_name(&pdev->dev), prepare, pdev, xpdev, xpdev->dev_hndl,
		xpdev->idx);

	qdma_device_version_info(xpdev->dev_hndl, &version_info);

	if (prepare) {
		qdma_device_offline(pdev, xpdev->dev_hndl);
		/* FLR setting is not required for 2018.3 IP */
		if (version_info.vivado_release_id != VIVADO_RELEASE_2018_3)
			qdma_device_flr_quirk_set(pdev, xpdev->dev_hndl);
		xpdev_queue_delete_all(xpdev);
		if (version_info.vivado_release_id != VIVADO_RELEASE_2018_3)
			qdma_device_flr_quirk_check(pdev, xpdev->dev_hndl);
	} else
		qdma_device_online(pdev, xpdev->dev_hndl);
}
#endif
static const struct pci_error_handlers qdma_err_handler = {
	.error_detected	= qdma_error_detected,
	.slot_reset	= qdma_slot_reset,
	.resume		= qdma_error_resume,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
	.reset_prepare  = qdma_reset_prepare,
	.reset_done     = qdma_reset_done,
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
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

int xlnx_nl_init(void);
void xlnx_nl_exit(void);

static int __init qdma_mod_init(void)
{
	int rv;

	pr_info("%s", version);
#ifdef __QDMA_VF__
	if (mode == LEGACY_INTR_MODE) {
		pr_err("Legacy interrupt not supported\n");
		return -EINVAL;
	}
#endif
	rv = libqdma_init(mode, num_threads);
	if (rv < 0)
		return rv;

	rv = xdev_qconf_init();
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

	xdev_qconf_cleanup();

	libqdma_exit();
}

module_init(qdma_mod_init);
module_exit(qdma_mod_exit);
