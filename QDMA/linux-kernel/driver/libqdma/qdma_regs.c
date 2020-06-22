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

#include "qdma_regs.h"

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/stddef.h>
#include <linux/string.h>

#include "xdev.h"
#include "qdma_device.h"
#include "qdma_descq.h"
#include "qdma_st_c2h.h"
#include "qdma_access_common.h"
#include "qdma_resource_mgmt.h"

int qdma_set_ring_sizes(struct xlnx_dma_dev *xdev, u8 index,
		u8 count, u32 *glbl_rng_sz)
{
	int i = 0;

	if (!xdev || !glbl_rng_sz)
		return -EINVAL;

	/* Adding 1 for the wrap around descriptor and status descriptor */
	for (i = index; i < (index + count); i++)
		*(glbl_rng_sz + i) += 1;

	if (xdev->hw.qdma_global_csr_conf(xdev, index, count, glbl_rng_sz,
					QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_WRITE))
		return -EINVAL;

	return 0;
}

int qdma_get_ring_sizes(struct xlnx_dma_dev *xdev, u8 index,
		u8 count, u32 *glbl_rng_sz)
{
	int i = 0;

	if (!xdev || !glbl_rng_sz)
		return -EINVAL;

	if (xdev->hw.qdma_global_csr_conf(xdev, index, count, glbl_rng_sz,
					QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ))
		return -EINVAL;

	/* Subtracting 1 for the wrap around descriptor and status descriptor */
	for (i = index; i < (index + count); i++)
		*(glbl_rng_sz + i) -= 1;

	return 0;
}

/*****************************************************************************/
/**
 * qdma_device_read_config_register() - read dma config. register
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	reg_addr:	register address
 * @param[out]	value:          pointer to hold the read value
 *
 * Return:	0 for success and <0 for error
 *****************************************************************************/
int qdma_device_read_config_register(unsigned long dev_hndl,
					unsigned int reg_addr, u32 *value)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	*value = readl(xdev->regs + reg_addr);

	return 0;
}

/*****************************************************************************/
/**
 * qdma_device_write_config_register() - write dma config. register
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	reg_addr:	register address
 * @param[in]	value:		register value to be writen
 *
 * Return:	0 for success and <0 for error
 *****************************************************************************/
int qdma_device_write_config_register(unsigned long dev_hndl,
				unsigned int reg_addr, unsigned int val)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	pr_debug("%s reg 0x%x, w 0x%08x.\n", xdev->conf.name, reg_addr, val);
	writel(val, xdev->regs + reg_addr);

	return 0;
}


#ifdef __QDMA_VF__
#include "qdma_mbox.h"

int qdma_csr_read(struct xlnx_dma_dev *xdev, struct global_csr_conf *csr)
{
	struct mbox_msg *m = qdma_mbox_msg_alloc();
	int rv, i;
	struct qdma_csr_info csr_info;

	if (!m)
		return -ENOMEM;

	memset(&csr_info, 0, sizeof(csr_info));

	qdma_mbox_compose_csr_read(xdev->func_id, m->raw);
	rv = qdma_mbox_msg_send(xdev, m, 1, QDMA_MBOX_MSG_TIMEOUT_MS);
	if (rv < 0)
		goto free_msg;

	rv = qdma_mbox_vf_csr_get(m->raw, &csr_info);
	if (!rv) {
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
			csr->ring_sz[i] = (uint32_t)csr_info.ringsz[i];
			csr->c2h_buf_sz[i] = (uint32_t)csr_info.bufsz[i];
			csr->c2h_timer_cnt[i] = (uint32_t)csr_info.timer_cnt[i];
			csr->c2h_cnt_th[i] = (uint32_t)csr_info.cnt_thres[i];
		}
		csr->wb_intvl = csr_info.wb_intvl;
	} else {
		pr_err("csr info read failed, rv = %d", rv);
		rv = -EINVAL;
	}


free_msg:
	qdma_mbox_msg_free(m);
	return rv;
}

#ifdef QDMA_CSR_REG_UPDATE
int qdma_global_csr_set(unsigned long dev_hndl, u8 index, u8 count,
		struct global_csr_conf *csr)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0)
		return -EINVAL;

	pr_info("VF %d setting csr NOT allowed.\n", xdev->func_id);
	return -EINVAL;
}
#endif
#else /* ifdef __QDMA_VF__ */

static void qdma_sort_c2h_cntr_th_values(struct xlnx_dma_dev *xdev)
{
	uint8_t i, idx = 0, j = 0;
	uint8_t c2h_cntr_val = xdev->csr_info.c2h_cnt_th[0];
	uint8_t least_max = 0;
	int ref_idx = -1;

get_next_idx:
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if ((ref_idx >= 0) && (ref_idx == i))
			continue;
		if (xdev->csr_info.c2h_cnt_th[i] < least_max)
			continue;
		c2h_cntr_val = xdev->csr_info.c2h_cnt_th[i];
		idx = i;
		break;
	}
	for (; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if ((ref_idx >= 0) && (ref_idx == i))
			continue;
		if (xdev->csr_info.c2h_cnt_th[i] < least_max)
			continue;
		if (c2h_cntr_val >= xdev->csr_info.c2h_cnt_th[i]) {
			c2h_cntr_val = xdev->csr_info.c2h_cnt_th[i];
			idx = i;
		}
	}
	xdev->sorted_c2h_cntr_idx[j] = idx;
	ref_idx = idx;
	j++;
	idx = j;
	least_max = c2h_cntr_val;
	if (j < QDMA_GLOBAL_CSR_ARRAY_SZ)
		goto get_next_idx;
}

int qdma_csr_read(struct xlnx_dma_dev *xdev, struct global_csr_conf *csr)
{
	int rv = 0;

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, csr->ring_sz,
				QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		pr_err("Failed to read global ring sizes, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	rv = xdev->hw.qdma_global_writeback_interval_conf(xdev,
						&csr->wb_intvl,
						QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		if (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED) {
			pr_err("Failed to read write back interval, err = %d",
					rv);
			return xdev->hw.qdma_get_error_code(rv);
		}
		pr_warn("Hardware Feature not supported");
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, csr->c2h_buf_sz,
				QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		if (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED) {
			pr_err("Failed to read global buffer sizes, err = %d",
						rv);
			return xdev->hw.qdma_get_error_code(rv);
		}
		pr_warn("Hardware Feature not supported");
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, csr->c2h_timer_cnt,
				QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		if (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED) {
			pr_err("Failed to read global timer count, err = %d",
						rv);
			return xdev->hw.qdma_get_error_code(rv);
		}
		pr_warn("Hardware Feature not supported");
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, csr->c2h_cnt_th,
				QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		if (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED) {
			pr_err("Failed to read global counter threshold, err = %d",
					rv);
			return xdev->hw.qdma_get_error_code(rv);
		}
		pr_warn("Hardware Feature not supported");
	}
	qdma_sort_c2h_cntr_th_values(xdev);
	return 0;
}

#ifdef QDMA_CSR_REG_UPDATE
int qdma_global_csr_set(unsigned long dev_hndl, u8 index, u8 count,
		struct global_csr_conf *csr)
{
	int rv = 0;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0)
		return -EINVAL;

	/** If qdma_get_active_queue_count() > 0,
	 *  cannot modify global CSR.
	 */
	if (qdma_get_active_queue_count(xdev->dma_device_index)) {
		pr_err("xdev %s, FMAP prog done, cannot modify global CSR\n",
				xdev->mod_name);
		return -EINVAL;
	}
	if (xdev->hw.qdma_global_writeback_interval_conf(xdev, csr->wb_intvl,
							 QDMA_HW_ACCESS_WRITE))
		return -EINVAL;
	if (xdev->hw.qdma_global_csr_conf(xdev, index, count, csr->ring_sz,
				QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_WRITE))
		return -EINVAL;
	if (xdev->hw.qdma_global_csr_conf(xdev, index, count,
					  csr->c2h_timer_cnt,
				QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_WRITE))
		return -EINVAL;
	if (xdev->hw.qdma_global_csr_conf(xdev, index, count, csr->c2h_cnt_th,
					QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_WRITE))
		return -EINVAL;
	if (xdev->hw.qdma_global_csr_conf(xdev, index, count, csr->c2h_buf_sz,
					QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_WRITE))
		return -EINVAL;

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0))
		return rv;

	return 0;
}
#endif
#endif

int qdma_global_csr_get(unsigned long dev_hndl, u8 index, u8 count,
		struct global_csr_conf *csr)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int i = 0;

	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	if ((index + count) > QDMA_GLOBAL_CSR_ARRAY_SZ) {
		pr_err("%s: Invalid index=%u and count=%u > %d",
					   __func__, index, count,
					   QDMA_GLOBAL_CSR_ARRAY_SZ);
		return -EINVAL;
	}

	/** If qdma_get_active_queue_count() > 0,
	 *  read the stored xdev csr values.
	 */
	if (qdma_get_active_queue_count(xdev->dma_device_index))
		memcpy(csr, &xdev->csr_info, sizeof(struct global_csr_conf));
	else
		qdma_csr_read(xdev, csr);

	/** Subtracting 1 for the wrap around descriptor and status descriptor
	 *  This same logic is present in qdma_get/set_ring_sizes() API
	 *  In case of any changes to this number, make sure to update in
	 *  all these places
	 */
	for (i = index; i < (index + count); i++)
		csr->ring_sz[i]--;

	return 0;
}

int qdma_device_flr_quirk_set(struct pci_dev *pdev, unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv;

	if (!dev_hndl) {
		pr_err("Invalid dev_hndl passed");
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

	if (!xdev->dev_cap.flr_present) {
		pr_info("FLR not present, therefore skipping FLR reset\n");
		return 0;
	}

	if (!dev_hndl || xdev_check_hndl(__func__, pdev, dev_hndl) < 0)
		return -EINVAL;

#ifndef __QDMA_VF__
	rv = xdev->hw.qdma_initiate_flr(xdev, 0);
#else
	rv = xdev->hw.qdma_initiate_flr(xdev, 1);
#endif
	if (rv)
		return -EINVAL;

	return 0;
}

int qdma_device_flr_quirk_check(struct pci_dev *pdev, unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv;
	uint8_t flr_done = 0;

	if (!dev_hndl) {
		pr_err("Invalid dev_hndl passed");
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

	if (!xdev->dev_cap.flr_present) {
		pr_info("FLR not present, therefore skipping FLR reset status\n");
		return 0;
	}

#ifndef __QDMA_VF__
	rv = xdev->hw.qdma_is_flr_done(xdev, 0, &flr_done);
#else
	rv = xdev->hw.qdma_is_flr_done(xdev, 1, &flr_done);
#endif
	if (rv)
		return -EINVAL;

	if (!flr_done)
		pr_info("%s, flr status stuck\n", xdev->conf.name);

	return 0;
}

int qdma_device_version_info(unsigned long dev_hndl,
		struct qdma_version_info *version_info)
{
	int rv = 0;
	struct qdma_hw_version_info info;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (!version_info) {
		pr_err("version_info is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	memset(&info, 0, sizeof(info));
#ifndef __QDMA_VF__
	rv = xdev->hw.qdma_get_version(xdev, QDMA_DEV_PF, &info);
#else
	rv = xdev->hw.qdma_get_version(xdev, QDMA_DEV_VF, &info);
#endif
	if (rv < 0) {
		pr_err("failed to get version with error = %d", rv);
		return  xdev->hw.qdma_get_error_code(rv);
	}

	strncpy(version_info->ip_str, info.qdma_ip_type_str,
			sizeof(version_info->ip_str) - 1);
	strncpy(version_info->rtl_version_str, info.qdma_rtl_version_str,
			sizeof(version_info->rtl_version_str) - 1);
	strncpy(version_info->vivado_release_str,
			info.qdma_vivado_release_id_str,
			sizeof(version_info->vivado_release_str) - 1);
	strncpy(version_info->device_type_str,
			info.qdma_device_type_str,
			sizeof(version_info->device_type_str) - 1);
	return 0;
}

#ifndef __QDMA_VF__
void qdma_device_attributes_get(struct xlnx_dma_dev *xdev)
{
	struct qdma_dev_attributes dev_info;

	memset(&dev_info, 0, sizeof(dev_info));

	xdev->hw.qdma_get_device_attributes(xdev, &xdev->dev_cap);

	pr_info("%s: num_pfs:%d, num_qs:%d, flr_present:%d, st_en:%d, mm_en:%d, mm_cmpt_en:%d, mailbox_en:%d, mm_channel_max:%d, qid2vec_ctx:%d, cmpt_ovf_chk_dis:%d, mailbox_intr:%d, sw_desc_64b:%d, cmpt_desc_64b:%d, dynamic_bar:%d, legacy_intr:%d, cmpt_trig_count_timer:%d",
		xdev->conf.name,
		xdev->dev_cap.num_pfs,
		xdev->dev_cap.num_qs,
		xdev->dev_cap.flr_present,
		xdev->dev_cap.st_en,
		xdev->dev_cap.mm_en,
		xdev->dev_cap.mm_cmpt_en,
		xdev->dev_cap.mailbox_en,
		xdev->dev_cap.mm_channel_max,
		xdev->dev_cap.qid2vec_ctx,
		xdev->dev_cap.cmpt_ovf_chk_dis,
		xdev->dev_cap.mailbox_intr,
		xdev->dev_cap.sw_desc_64b,
		xdev->dev_cap.cmpt_desc_64b,
		xdev->dev_cap.dynamic_bar,
		xdev->dev_cap.legacy_intr,
		xdev->dev_cap.cmpt_trig_count_timer);
}
#endif

int qdma_queue_cmpl_ctrl(unsigned long dev_hndl, unsigned long id,
			struct qdma_cmpl_ctrl *cctrl, bool set)
{
	int rv = 0;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq;

	/** make sure that the dev_hndl passed is Valid */
	if (!xdev) {
		pr_err("dev_hndl is NULL");
		return -EINVAL;
	}

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0) {
		pr_err("Invalid dev_hndl passed");
		return -EINVAL;
	}

	descq = qdma_device_get_descq_by_id(xdev, id, NULL, 0, 1);
	if (!descq) {
		pr_err("Invalid qid: %ld", id);
		return -EINVAL;
	}

	if (set) {
		lock_descq(descq);

		descq->cmpt_cidx_info.trig_mode =
				descq->conf.cmpl_trig_mode =
						cctrl->trigger_mode;
		descq->cmpt_cidx_info.timer_idx =
				descq->conf.cmpl_timer_idx = cctrl->timer_idx;
		descq->cmpt_cidx_info.counter_idx =
				descq->conf.cmpl_cnt_th_idx = cctrl->cnt_th_idx;
		descq->cmpt_cidx_info.irq_en =
				descq->conf.cmpl_en_intr = cctrl->cmpl_en_intr;
		descq->cmpt_cidx_info.wrb_en =
				descq->conf.cmpl_stat_en = cctrl->en_stat_desc;

		descq->cmpt_cidx_info.wrb_cidx = descq->cidx_cmpt;
		rv = queue_cmpt_cidx_update(descq->xdev,
				descq->conf.qidx, &descq->cmpt_cidx_info);
		if (unlikely(rv < 0)) {
			pr_err("%s: Failed to update cmpt cidx\n",
					descq->conf.name);
			unlock_descq(descq);
			return rv;
		}

		unlock_descq(descq);

	} else {
		/* read the setting */
		rv = queue_cmpt_cidx_read(descq->xdev,
				descq->conf.qidx, &descq->cmpt_cidx_info);
		if (unlikely(rv < 0))
			return rv;

		cctrl->trigger_mode = descq->conf.cmpl_trig_mode =
				descq->cmpt_cidx_info.trig_mode;
		cctrl->timer_idx = descq->conf.cmpl_timer_idx =
				descq->cmpt_cidx_info.timer_idx;
		cctrl->cnt_th_idx = descq->conf.cmpl_cnt_th_idx =
				descq->cmpt_cidx_info.counter_idx;
		cctrl->en_stat_desc = descq->conf.cmpl_stat_en =
				descq->cmpt_cidx_info.wrb_en;
		cctrl->cmpl_en_intr = descq->conf.cmpl_en_intr =
				descq->cmpt_cidx_info.irq_en;
	}

	return 0;
}
