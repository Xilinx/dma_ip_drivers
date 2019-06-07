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
#include "qdma_regs.h"
#include <linux/delay.h>

/**
 * mutex  used for resource management APIs
 */
static DEFINE_MUTEX(res_mutex);

void *qdma_calloc(uint32_t num_blocks, uint32_t size)
{
	return kzalloc(num_blocks * size, GFP_KERNEL);
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

void qdma_hw_error_handler(void *dev_hndl, enum qdma_error_idx err_idx)
{
	pr_err("%s detected", qdma_get_error_name(err_idx));
}

void qdma_get_device_attr(void *dev_hndl, struct qdma_dev_attributes **dev_cap)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	*dev_cap = &xdev->dev_cap;
}
