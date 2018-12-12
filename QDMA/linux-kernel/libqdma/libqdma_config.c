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

/*****************************************************************************/
/**
 * qdma_set_qmax() -  Handler function to set the qmax configuration value
 *
 * @param[in]	dev_hndl:	qdma device handle
 * @param[in]	qsets_max:	qmax configuration value
 * @param[in]	forced:	whether to force set the value
 *
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	< 0 on failure
 *****************************************************************************/
int qdma_set_qmax(unsigned long dev_hndl, u32 qsets_max, bool forced)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev;
	int rv = -1;

	/**
	 *  If xdev is NULL or qdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	qdev = xdev_2_qdev(xdev);

	if (!qdev)
		return -EINVAL;

	/** If qdev->init_qrange is set,
	 *  it indicates that FMAP programming is done
	 *	That means at least one queue is added in the system.
	 *  qmax is not allowed to change after FMAP programming is done.
	 *  Hence, If qdev->init_qrange is set, return error as qmax
	 *  cannot be changed now.
	 */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, can not modify qmax [%d]\n",
						xdev,
						qdev->qmax);
		return rv;
	}

	/** If the input qsets_max is same as the current xdev->conf.qsets_max,
	 *	return, as there is nothing to be changed
	 */
	if ((qsets_max == xdev->conf.qsets_max) && !forced) {
		pr_err("xdev 0x%p, Current qsets_max is same as [%d].Nothing to be done\n",
				xdev, xdev->conf.qsets_max);
		return rv;
	}


	/** FMAP programming is not done yet
	 *  remove the already created qdev and recreate it using the
	 *  newly configured size
	 */
	qdma_device_cleanup(xdev);
	xdev->conf.qsets_max = qsets_max;
	rv = qdma_device_init(xdev);
	if (rv < 0) {
		pr_warn("qdma_init failed %d.\n", rv);
		qdma_device_cleanup(xdev);
	}

	return QDMA_OPERATION_SUCCESSFUL;
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
	if (!xdev)
		return -EINVAL;

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
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_intr_rngsz(unsigned long dev_hndl, u32 intr_rngsz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev;
	int rv = -1;

	/**
	 *  If xdev is NULL or qdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	qdev = xdev_2_qdev(xdev);

	if (!qdev)
		return -EINVAL;

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

	/** If qdev->init_qrange is set,
	 *  it indicates that FMAP programming is done
	 *	That means at least one queue is added in the system.
	 *  intr_rngsz is not allowed to change after FMAP programming is done.
	 *  Hence, If qdev->init_qrange is set, return error as intr_rngsz
	 *  cannot be changed now.
	 */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify intr ring size [%d]\n",
				xdev,
				xdev->conf.intr_rngsz);
		return rv;
	}

	/**
	 *  FMAP programming is not done yet, update the intr_rngsz
	 */
	xdev->conf.intr_rngsz = intr_rngsz;

	return QDMA_OPERATION_SUCCESSFUL;
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
	if (!xdev)
		return -EINVAL;

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
/*****************************************************************************/
/**
 * qdma_set_buf_sz() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	buf_sz:		interrupt aggregation ring size
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_buf_sz(unsigned long dev_hndl, u32 *buf_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = -1;

	/**
	 *  If xdev is NULL or qdev is NULL,
	 *  return error as Invalid parameter
	 */
	if (!xdev || !qdev)
		return -EINVAL;


	/** If qdev->init_qrange is set,
	 *  it indicates that FMAP programming is done
	 *	That means at least one queue is added in the system.
	 *  intr_rngsz is not allowed to change after FMAP programming is done.
	 *  Hence, If qdev->init_qrange is set, return error as buf_sz
	 *  cannot be changed now.
	 */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify buf size\n",
				xdev);
		return rv;
	}

	/**
	 * Write the given buf sizes to the registers
	 */
	rv = qdma_csr_write_bufsz(xdev, buf_sz);

	return rv;
}

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
	if (!xdev)
		return -EINVAL;

	qdma_csr_read_bufsz(xdev, buf_sz);

	return QDMA_OPERATION_SUCCESSFUL;
}

/*****************************************************************************/
/**
 * qdma_set_glbl_rng_sz() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	buf_sz:		interrupt aggregation ring size
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_glbl_rng_sz(unsigned long dev_hndl, u32 *glbl_rng_sz)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = -1;

	/**
	 *  If xdev is NULL or qdev is NULL,
	 *  return error as Invalid parameter
	 */
	if (!xdev || !qdev)
		return -EINVAL;


	/** If qdev->init_qrange is set,
	 *  it indicates that FMAP programming is done
	 *	That means at least one queue is added in the system.
	 *  intr_rngsz is not allowed to change after FMAP programming is done.
	 *  Hence, If qdev->init_qrange is set, return error as glbl_rng_sz
	 *  cannot be changed now.
	 */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify glbl_rng_sz\n",
				xdev);
		return rv;
	}

	/**
	 * Write the given buf sizes to the registers
	 */
	rv = qdma_csr_write_rngsz(xdev, glbl_rng_sz);

	return rv;
}

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
	if (!xdev)
		return -EINVAL;

	qdma_csr_read_rngsz(xdev, glbl_rng_sz);

	return QDMA_OPERATION_SUCCESSFUL;
}

/*****************************************************************************/
/**
 * qdma_set_timer_cnt() - Handler function to set the buf_sz value
 *
 * @param[in]	dev_hndl:		qdma device handle
 * @param[in]	tmr_cnt:		Array of 16 timer count values
 *
 * @return	QDMA_OPERATION_SUCCESSFUL on success
 * @return	<0 on failure
 *****************************************************************************/
int qdma_set_timer_cnt(unsigned long dev_hndl, u32 *tmr_cnt)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = -1;

	if (!xdev || !qdev)
		return -EINVAL;

   /**  If qdev->init_qrange is set,
    *  it indicates that FMAP programming is done
    *      That meansat least one queue is added in the system.
    *  qmax is not allowed to change after FMAP programming is done.
    *  Hence, If qdev->init_qrange is set, return error as qmax
    *  cannot be changed now.
    */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, can not modify timer count\n",
						xdev);
		return rv;
	}

	rv = qdma_csr_write_timer_cnt(xdev, tmr_cnt);

	return rv;
}

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

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	qdma_csr_read_timer_cnt(xdev, tmr_cnt);

	return QDMA_OPERATION_SUCCESSFUL;
}

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
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	int rv = -1;

	if (!xdev || !qdev)
		return -EINVAL;

   /** If qdev->init_qrange is set,
    *  it indicates that FMAP programming is done
    *  That means at least one queue is added in the system.
    *  csr count threshold is not allowed to change after FMAP
    *  programming is done.
    *  Hence, If qdev->init_qrange is set, return error as csr count threshold
    *  cannot be changed now.
    */
	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, can not modify threshold count\n",
						xdev);
		return rv;
	}

	rv = qdma_csr_write_cnt_thresh(xdev, cnt_th);

	return rv;
}

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

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	qdma_csr_read_cnt_thresh(xdev, cnt_th);

	return QDMA_OPERATION_SUCCESSFUL;
}

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
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	unsigned int reg;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	if (qdev->init_qrange) {
		pr_err("xdev 0x%p, FMAP prog done, cannot modify cmpt acc\n",
				xdev);
		return -EINVAL;
	}
	/**
	 * Write the given cmpl_status_acc value to the register
	 */
	reg = __read_reg(xdev, QDMA_REG_GLBL_DSC_CFG);
	reg &= ~QDMA_REG_GLBL_DSC_CFG_CMPL_STATUS_ACC_MASK;
	reg |= cmpl_status_acc;
	__write_reg(xdev, QDMA_REG_GLBL_DSC_CFG, reg);

	return QDMA_OPERATION_SUCCESSFUL;
}

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
unsigned int qdma_get_cmpl_status_acc(unsigned long dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	unsigned int cs_acc;

	/**
	 * If xdev is NULL, return error as Invalid parameter
	 */
	if (!xdev)
		return -EINVAL;

	/**
	 * Read the current cmpl_status_acc value from the register and return
	 */
	cs_acc = __read_reg(xdev, QDMA_REG_GLBL_DSC_CFG) &
			QDMA_REG_GLBL_DSC_CFG_CMPL_STATUS_ACC_MASK;

	return cs_acc;
}
#endif
