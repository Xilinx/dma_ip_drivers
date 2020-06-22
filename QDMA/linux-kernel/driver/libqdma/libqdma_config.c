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
 * @brief This file contains the definitions for qdma configuration apis
 *
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "libqdma_export.h"

#include "qdma_descq.h"
#include "qdma_device.h"
#include "qdma_thread.h"
#include "qdma_regs.h"
#include "qdma_context.h"
#include "qdma_intr.h"
#include "thread.h"
#include "version.h"
#include "qdma_resource_mgmt.h"

/*****************************************************************************/
/**
 * qdma_set_qmax() -  Handler function to set the qmax configuration value
 *
 * @param[in]	dev_hndl:	qdma device handle
 * @param[in]	qsets_max:	qmax configuration value
 * @param[in]	forced:	whether to force set the value
 *
 *
 * @return	0 on success
 * @return	< 0 on failure
 *****************************************************************************/
int qdma_set_qmax(unsigned long dev_hndl, int qbase, u32 qsets_max)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = 0;

	/**
	 *  If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}


	/* update the device with requested qmax and qbase */
	rv = qdma_dev_update(xdev->dma_device_index, xdev->func_id,
			qsets_max, &qbase);
	if (rv < 0) {
		pr_err("Failed to update dev entry, err = %d", rv);
		return -EINVAL;
	}
	qdma_device_cleanup(xdev);

	rv = qdma_dev_qinfo_get(xdev->dma_device_index, xdev->func_id,
			&qbase, &qsets_max);
	if (rv < 0) {
		pr_err("Failed to get qinfo, err = %d", rv);
		return -EINVAL;
	}
	xdev->conf.qsets_max = qsets_max;
	xdev->conf.qsets_base = qbase;
	rv = qdma_device_init(xdev);
	if (rv < 0) {
		pr_warn("qdma_init failed, err = %d", rv);
		qdma_device_cleanup(xdev);
		return -EINVAL;
	}

	return 0;
}
/*****************************************************************************/
/**
 * qdma_get_qmax() -  Handler function to get the qmax configuration value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 * @return	qmax value on success
 * @return	< 0 on failure
 *****************************************************************************/
unsigned int qdma_get_qmax(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/**
	 * If xdev is NULL return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/**
	 * Return the current qsets_max value of the device
	 */
	return xdev->conf.qsets_max;
}

/*****************************************************************************/
/**
 * qdma_set_intr_rngsz() - Handler function to set the intr_ring_size value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	intr_rngsz:		interrupt aggregation ring size
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_intr_rngsz(unsigned long dev_hndl, u32 intr_rngsz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	/**
	 *  If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/** If the input intr_rngsz is same as the
	 *  current xdev->conf.intr_rngsz,
	 *	return, as there is nothing to be changed
	 */
	if (intr_rngsz == xdev->conf.intr_rngsz) {
		pr_err("xdev 0x%p, Current intr_rngsz is same as [%d].Nothing to be done\n",
					xdev, intr_rngsz);
		return rv;
	}

	/** If interrupt aggregation is not enabled, then no need to change the
	 *  interrupt ring size. Retrun error in this case.
	 */
	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		pr_err("xdev 0x%p, interrupt aggregation is disabled\n", xdev);
		return rv;
	}

	/** If qdma_get_active_queue_count() > 0,
	 *  intr_rngsz is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify intr ring size [%d]\n",
				xdev,
				xdev->conf.intr_rngsz);
		return rv;
	}

	/** intr_rngsz > QDMA_INDIRECT_INTR_RING_SIZE_32KB,
	 *  is invalid.
	 */
	if (intr_rngsz > QDMA_INDIRECT_INTR_RING_SIZE_32KB) {
		pr_err("Invalid intr ring size\n");
		return rv;
	}

	/**
	 *  FMAP programming is not done yet, update the intr_rngsz
	 */
	qdma_device_interrupt_cleanup(xdev);
	xdev->conf.intr_rngsz = intr_rngsz;
	qdma_device_interrupt_setup(xdev);

	return 0;
}

/*****************************************************************************/
/**
 * qdma_get_intr_rngsz() - Handler function to get the intr_ring_size value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 *
 * @return	interrupt ring size on success
 * @return	<0 on failure
 *****************************************************************************/
unsigned int qdma_get_intr_rngsz(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/**
	 * If xdev is NULL return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/** If interrupt aggregation is not enabled, then return 0
	 *  As the intr_rngsz value is irrelevant in this case
	 */
	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		pr_info("xdev 0x%p, interrupt aggregation is disabled\n", xdev);
		return 0;
	}

	pr_info("xdev 0x%p, intr ring_size = %d\n",
				xdev,
				xdev->conf.intr_rngsz);
	/**
	 * Return the current intr_rngsz value of the device
	 */
	return xdev->conf.intr_rngsz;
}
#ifndef __QDMA_VF__
#ifdef QDMA_CSR_REG_UPDATE
/*****************************************************************************/
/**
 * qdma_set_buf_sz() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	buf_sz:		interrupt aggregation ring size
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_buf_sz(unsigned long dev_hndl, u32 *buf_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	/**
	 *  If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}


	/** If qdma_get_active_queue_count() > 0,
	 *  buf_sz is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify buf size\n",
				xdev);
		return rv;
	}

	/**
	 * Write the given buf sizes to the registers
	 */
	rv = xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			buf_sz, QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_WRITE);
	if (rv < 0) {
		pr_err("set global buffer size failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	qdma_csr_read(xdev, &xdev->csr_info);

	return rv;
}
#endif

/*****************************************************************************/
/**
 * qdma_get_buf_sz() - Handler function to get the buf_sz value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 *
 * @return	buffer size on success
 * @return	<0 on failure
 *****************************************************************************/
unsigned int qdma_get_buf_sz(unsigned long dev_hndl, u32 *buf_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	if (xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			buf_sz, QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ))
		return -EINVAL;

	return 0;
}

#ifdef QDMA_CSR_REG_UPDATE
/*****************************************************************************/
/**
 * qdma_set_glbl_rng_sz() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	buf_sz:		interrupt aggregation ring size
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_glbl_rng_sz(unsigned long dev_hndl, u32 *glbl_rng_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	/**
	 *  If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}


	/** If qdma_get_active_queue_count() > 0,
	 *  glbl_rng_sz is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify glbl_rng_sz\n",
				xdev);
		return rv;
	}

	/**
	 * Write the given ring sizes to the registers
	 */
	rv = xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			glbl_rng_sz, QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_WRITE);
	if (rv < 0) {
		pr_err("Failed to write glbl rng size, err = %d", rv);
		return -EINVAL;
	}

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0)) {
		pr_err("Failed to read glbl csr, err = %d", rv);
		return rv;
	}

	return 0;
}
#endif

/*****************************************************************************/
/**
 * qdma_get_glbl_rng_sz() - Handler function to get the glbl_rng_sz value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 *
 * @return	glbl_rng_sz size on success
 * @return	<0 on failure
 *****************************************************************************/
unsigned int qdma_get_glbl_rng_sz(unsigned long dev_hndl, u32 *glbl_rng_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	if (xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			glbl_rng_sz, QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ))
		return -EINVAL;

	return 0;
}

#ifdef QDMA_CSR_REG_UPDATE
/*****************************************************************************/
/**
 * qdma_set_timer_cnt() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	tmr_cnt:		Array of 16 timer count values
 *
 * @return	0 on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_timer_cnt(unsigned long dev_hndl, u32 *tmr_cnt)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/** If qdma_get_active_queue_count() > 0,
	 *  tmr_cnt is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, can not modify timer count\n",
						xdev);
		return rv;
	}


	rv = xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			tmr_cnt, QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_WRITE);
	if (unlikely(rv < 0)) {
		pr_err("global timer set failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0))
		return rv;

	return 0;
}
#endif

/*****************************************************************************/
/**
 * qdma_get_timer_cnt() - Handler function to get the timer_cnt value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 *
 * @return	timer_cnt  on success
 * @return	<0 on failure
 *****************************************************************************/
unsigned int qdma_get_timer_cnt(unsigned long dev_hndl, u32 *tmr_cnt)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0, QDMA_GLOBAL_CSR_ARRAY_SZ,
			tmr_cnt, QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		pr_err("get global timer failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	return 0;
}

#ifdef QDMA_CSR_REG_UPDATE
/*****************************************************************************/
/**
 * qdma_set_cnt_thresh() - Handler function to set the counter threshold value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	cnt_th:		Array of 16 timer count values
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_cnt_thresh(unsigned long dev_hndl, unsigned int *cnt_th)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/** If qdma_get_active_queue_count() > 0,
	 *  cnt_th is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, can not modify threshold count\n",
						xdev);
		return rv;
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, cnt_th,
			QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_WRITE);
	if (unlikely(rv < 0)) {
		pr_err("set global counter failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0))
		return rv;

	return 0;
}
#endif

/*****************************************************************************/
/**
 * qdma_get_cnt_thresh() - Handler function to get the counter thresh value
 *
 * @param[in]	dev_hndl:	qdma device handle
 *
 *
 * @return	counter threshold values  on success
 * @return	<0 on failure
 *****************************************************************************/
unsigned int qdma_get_cnt_thresh(unsigned long dev_hndl, u32 *cnt_th)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = -1;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	rv = xdev->hw.qdma_global_csr_conf(xdev, 0,
					QDMA_GLOBAL_CSR_ARRAY_SZ, cnt_th,
					QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		pr_err("get global counter failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	return 0;
}

#ifdef QDMA_CSR_REG_UPDATE
/*****************************************************************************/
/**
 * qdma_set_cmpl_status_acc() -  Handler function to set the cmpl_status_acc
 * configuration value
 *
 * @param[in]	dev_hndl:	qdma device handle
 * @param[in]	cmpl_status_acc:	Writeback Accumulation value
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_cmpl_status_acc(unsigned long dev_hndl, u32 cmpl_status_acc)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	int rv = 0;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/** If qdma_get_active_queue_count() > 0,
	 *  cmpl_status_acc is not allowed to change.
	 */
	if (qdma_get_active_queue_count(xdev->conf.pdev->bus->number)) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify cmpt acc\n",
				xdev);
		return -EINVAL;
	}
	/**
	 * Write the given cmpl_status_acc value to the register
	 */
	rv = xdev->hw.qdma_global_writeback_interval_conf(xdev, cmpl_status_acc,
							QDMA_HW_ACCESS_WRITE);
	if (unlikely(rv < 0)) {
		pr_err("set global writeback intvl failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	rv = qdma_csr_read(xdev, &xdev->csr_info);
	if (unlikely(rv < 0))
		return rv;

	return 0;
}
#endif

/*****************************************************************************/
/**
 * qdma_get_cmpl_status_acc() -  Handler function to get the cmpl_status_acc
 * configuration value
 *
 * @param[in] dev_hndl:		qdma device handle
 *
 * Handler function to get the writeback accumulation value
 *
 * @return	cmpl_status_acc on success
 * @return	<0 on failure
 *
 *****************************************************************************/
unsigned int qdma_get_wb_intvl(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	unsigned int wb_intvl;
	int rv = -1;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev) {
		pr_err("xdev is invalid");
		return -EINVAL;
	}

	/**
	 * Read the current cmpl_status_acc value from the register and return
	 */
	rv = xdev->hw.qdma_global_writeback_interval_conf(xdev, &wb_intvl,
							QDMA_HW_ACCESS_READ);
	if (unlikely(rv < 0)) {
		pr_err("read global writeback intvl failed, err = %d", rv);
		return xdev->hw.qdma_get_error_code(rv);
	}

	return wb_intvl;
}
#endif
