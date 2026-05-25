/*
 * This file is part of the Xilinx MDB5 DMA IP Core driver for Linux
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
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



#ifndef __AMD_MDB5_DMA_CLIENT_MOD_H
#define __AMD_MDB5_DMA_CLIENT_MOD_H

#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/sched/task.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/aio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock_types.h>

#include "mdb5-dmaclient-cdev.h"

#define mdb5_dma_dbg(dev, fmt, a...) \
	dev_dbg(dev, fmt "\n", ##a)

#define mdb5_dma_err(dev, fmt, a...) \
	dev_err(dev, fmt "\n", ##a)

enum amd_mdb5_dma_chan_dir {
	AMD_MDB5_DMA_CHAN_WR = 1,
	AMD_MDB5_DMA_CHAN_RD
};


struct amd_mdb5_dma_channel;
struct amd_mdb5_dma_channel_stats;
struct amd_mdb5_dma_cdev_chan;
struct amd_mdb5_dma_cdev_ctrl;
struct amd_mdb5_dma_kthread;

/*
 * amd_mdb5_dma_channel
 * This is per channel struct, all the data path and control path
 * information can be derived from this structure.
 * This structure is mainly helpful in data path operations.
 */
struct amd_mdb5_dma_channel {
	struct list_head			link;
	char					chan_name[AMD_MDB5_DMA_NAME_SZ];
	struct dma_chan				*dchan;
	enum amd_mdb5_dma_chan_dir		dir;
	struct device				*dev;
	u16					index;
	enum amd_mdb5_dma_mode			mode;
	unsigned int				aperture_sz;
	struct amd_mdb5_dma_cdev_chan		cdev_chan;
	struct amd_mdb5_dma_cdev_ctrl		*cdev_ctrl;
	struct amd_mdb5_dma_channel_stats	stats;
	struct amd_mdb5_dma_kthread		kth;
	struct list_head			pend_result;
	u32					pend_count;
	spinlock_t				pend_lock;
	struct mutex				mutex;
	int					error;
};

/*
 * amd_mdb5_dma_client
 * This is book keeping struct to keep the information
 * related to each controller that has been found.
 */
struct amd_mdb5_dma_client {
	struct list_head			channels;
	struct device				*dev;
	unsigned int				rd_max_chan;
	unsigned int				rd_chan_available;
	unsigned int				wr_max_chan;
	unsigned int				wr_chan_available;
	struct amd_mdb5_dma_cdev_ctrl		cdev_ctrl;
	struct mutex				lock;
};

int amd_mdb5_dma_probe_channels(struct amd_mdb5_dma_client *mdb5_dma_client,
				u16 dev_id, u16 max_rchan, u16 max_wchan);
void amd_mdb5_dma_free_channel(struct amd_mdb5_dma_channel *hchan);
#endif /* __AMD_CLIENT_MOD_H */
