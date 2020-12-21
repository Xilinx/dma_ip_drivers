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
#include "qdma_platform.h"
#include "qdma_regs.h"
#include "qdma_access_errors.h"
#include <linux/errno.h>
#include <linux/delay.h>

static struct err_code_map error_code_map_list[] = {
	{QDMA_SUCCESS,				0},
	{QDMA_ERR_INV_PARAM,			EINVAL},
	{QDMA_ERR_NO_MEM,			ENOMEM},
	{QDMA_ERR_HWACC_BUSY_TIMEOUT,		EBUSY},
	{QDMA_ERR_HWACC_INV_CONFIG_BAR,		EINVAL},
	{QDMA_ERR_HWACC_NO_PEND_LEGCY_INTR,	EINVAL},
	{QDMA_ERR_HWACC_BAR_NOT_FOUND,		EINVAL},
	{QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED,	EINVAL},
	{QDMA_ERR_RM_RES_EXISTS,		EPERM},
	{QDMA_ERR_RM_RES_NOT_EXISTS,		EINVAL},
	{QDMA_ERR_RM_DEV_EXISTS,		EPERM},
	{QDMA_ERR_RM_DEV_NOT_EXISTS,		EINVAL},
	{QDMA_ERR_RM_NO_QUEUES_LEFT,		EPERM},
	{QDMA_ERR_RM_QMAX_CONF_REJECTED,	EPERM},
	{QDMA_ERR_MBOX_FMAP_WR_FAILED,		EIO},
	{QDMA_ERR_MBOX_NUM_QUEUES,		EINVAL},
	{QDMA_ERR_MBOX_INV_QID,			EINVAL},
	{QDMA_ERR_MBOX_INV_RINGSZ,		EINVAL},
	{QDMA_ERR_MBOX_INV_BUFSZ,		EINVAL},
	{QDMA_ERR_MBOX_INV_CNTR_TH,		EINVAL},
	{QDMA_ERR_MBOX_INV_TMR_TH,		EINVAL},
	{QDMA_ERR_MBOX_INV_MSG,			EINVAL},
	{QDMA_ERR_MBOX_SEND_BUSY,		EBUSY},
	{QDMA_ERR_MBOX_NO_MSG_IN,		EINVAL},
	{QDMA_ERR_MBOX_REG_READ_FAILED,	EIO},
	{QDMA_ERR_MBOX_ALL_ZERO_MSG,		EINVAL},
};

/**
 * mutex  used for resource management APIs
 */
static DEFINE_MUTEX(res_mutex);

void *qdma_calloc(uint32_t num_blocks, uint32_t size)
{
	return kzalloc((num_blocks * (size_t)size), GFP_KERNEL);
}

void qdma_memfree(void *memptr)
{
	kfree(memptr);
}

void qdma_udelay(u32 delay_us)
{
	udelay(delay_us);
}

int qdma_reg_access_lock(void *dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	spin_lock(&xdev->hw_prg_lock);

	return 0;
}

int qdma_reg_access_release(void *dev_hndl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	spin_unlock(&xdev->hw_prg_lock);

	return 0;
}

u32 qdma_reg_read(void *dev_hndl, u32 reg_offst)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	return readl(xdev->regs + reg_offst);
}

void qdma_reg_write(void *dev_hndl, u32 reg_offst, u32 val)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	writel(val, xdev->regs + reg_offst);
}

void qdma_resource_lock_take(void)
{
	mutex_lock(&res_mutex);
}

void qdma_resource_lock_give(void)
{
	mutex_unlock(&res_mutex);
}

void qdma_get_hw_access(void *dev_hndl, struct qdma_hw_access **hw)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	*hw = &xdev->hw;
}

void qdma_strncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
}


int qdma_get_err_code(int acc_err_code)
{
	acc_err_code *= -1;
	return -(error_code_map_list[acc_err_code].err_code);
}
