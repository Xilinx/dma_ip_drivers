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



#ifndef __AMD_MDB5_DMA_CLIENT_CDEV_H
#define __AMD_MDB5_DMA_CLIENT_CDEV_H

#include <linux/cdev.h>
#include <linux/cdev.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/version.h>

#define AMD_MDB5_DMA_NODE_NAME			"mdb5"

#define AMD_MDB5_DMA_MAX_RD_CHAN		(8)
#define AMD_MDB5_DMA_MAX_WR_CHAN		(8)
#define AMD_MDB5_DMA_NAME_SZ			(32)
#define AMD_MDB5_DMA_THREAD_NAME_FMT		"mdb5_%s%d_poll"
#define AMD_MDB5_DMA_MAX_BURST_SZ		(0x100000000L)

#define AMD_MDB5_DMA_CHAN_APERTURE_MODE		(0x1)
#define AMD_MDB5_DMA_CHAN_ASYNC_MODE		(0x2)

#define AMD_MDB5_DMA_NODE_CTRL_MINOR		1
#define AMD_MDB5_DMA_NODE_CHAN_MINOR		1
#define AMD_MDB5_DMA_REQ_TIMEOUT_MS		(5000)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,4,0))
#define	iter_iov(iter)		(iter)->iov
#endif

struct amd_mdb5_dma_channel;

enum amd_mdb5_dma_req_status {
	AMD_MDB5_DMA_REQ_NONE = 0x100,
	AMD_MDB5_DMA_REQ_SUBMIT,
	AMD_MDB5_DMA_REQ_INTR_RECV,
	AMD_MDB5_DMA_REQ_COMPLETED,

	AMD_MDB5_DMA_REQ_TIMEDOUT = 0x200,
	AMD_MDB5_DMA_REQ_ERROR
};

enum amd_mdb5_dma_mode {
	AMD_MDB5_DMA_MODE_SG = 0x100,
	AMD_MDB5_DMA_MODE_SIMPLE = 0x300,
};


struct ctrl_mode {
	char name[AMD_MDB5_DMA_NAME_SZ];
	enum amd_mdb5_dma_mode mode;
};

struct ctrl_aperture {
	char name[AMD_MDB5_DMA_NAME_SZ];
	u32  aperture;
};

struct ctrl_stats {
	char name[AMD_MDB5_DMA_NAME_SZ];
	u64  stat_addr;
};


#define AMD_MDB5_DMA_CTRL_CMD_MAGIC		'm'
#define AMD_MDB5_DMA_CTRL_CMD_SET_APERTURE_SZ	\
_IOW(AMD_MDB5_DMA_CTRL_CMD_MAGIC, 1, struct ctrl_aperture)

#define AMD_MDB5_DMA_CTRL_CMD_STATS		\
_IOR(AMD_MDB5_DMA_CTRL_CMD_MAGIC, 2, struct ctrl_stats)

#define AMD_MDB5_DMA_CTRL_CMD_MODE		\
_IOW(AMD_MDB5_DMA_CTRL_CMD_MAGIC, 3, struct ctrl_mode)

#define AMD_MDB5_DMA_CTRL_CMD_GET_MODE	\
_IOR(AMD_MDB5_DMA_CTRL_CMD_MAGIC, 4, struct ctrl_mode)

#define AMD_MDB5_DMA_CTRL_CMD_GET_APERTURE_SZ	\
_IOR(AMD_MDB5_DMA_CTRL_CMD_MAGIC, 5, struct ctrl_aperture)


/*
 * amd_mdb5_dma_cdev_ctrl
 * This is the struct holding information related to
 * control path and its own device node.
 */
struct amd_mdb5_dma_cdev_ctrl {
	int					major;
	dev_t					cdevno;
	struct cdev				cdev;
	struct amd_mdb5_dma_channel		*rchan[AMD_MDB5_DMA_MAX_RD_CHAN];
	struct amd_mdb5_dma_channel		*wchan[AMD_MDB5_DMA_MAX_WR_CHAN];
	char					cdev_name[AMD_MDB5_DMA_NAME_SZ];
	int					dev_id;
	struct device				*sys_dev;
	struct device				*dev;
	int					nref;
	int					ctrl_open;
};

/*
 * amd_mdb5_dma_cdev_chan
 * This struct holds the per channel char device related
 * information. With the help of this struct members
 * the device nodes in /dev are created.
 */
struct amd_mdb5_dma_cdev_chan {
	int					major;
	dev_t					cdevno;
	struct cdev				cdev;
	struct device				*sys_dev;
	int					nref;
	u16					dev_id;
	char					cdev_name[AMD_MDB5_DMA_NAME_SZ];
	unsigned int				name_hash;
	int					chan_open;
};

/*
 * amd_mdb5_dma_kthread
 * This struct holds the members required to run a poll
 * thread. It is a kernel thread.
 */ 
struct amd_mdb5_dma_kthread {
	u32					timeout;
	u8					running;
	u8					schedule;
	u32					cpu;
	char					kthread_name[AMD_MDB5_DMA_NAME_SZ];
	struct task_struct			*task;
	struct amd_mdb5_dma_channel		*hchan;
	wait_queue_head_t			poll_wq;
	spinlock_t				lock;
};

/*
 * amd_mdb5_dma_channel_stats
 * Various stats related to channel operation can be tracked
 * using the members of this struct.
 */
struct amd_mdb5_dma_channel_stats {
	atomic64_t				req_rcvd;
	atomic64_t				req_success;
	atomic64_t				req_failed;
	atomic64_t				intr_rcvd;
	atomic64_t				read_data_processed;
	atomic64_t				write_data_processed;
	atomic64_t				total_data_processed;
};

/*
 * amd_mdb5_dma_io_completion
 * This struct holds the vital information to get the
 * completion status of a submitted request.
 * This has the members using which a request can be tracked
 * in the interrupt and poll thread contexts.
 */
struct amd_mdb5_dma_io_completion {
	struct list_head			link;
	struct dma_async_tx_descriptor		*txdesc;
	dma_cookie_t				cookie;
	unsigned int				flags;
	bool					comp_handled;
	struct amd_mdb5_dma_kthread		*kth;
	enum amd_mdb5_dma_req_status		req_status;
	enum dma_status				status;
	wait_queue_head_t			*wq;
	unsigned long				tstamp;
	unsigned long				timeout; /* in MS */
	spinlock_t				lock;
};

/*
 * amd_mdb5_dma_io_request
 * This is the struct which contains src, dst and the details of
 * the inputs and dma-able transformations of the addresses required
 * for submission to the dma controller callback.
 *
 * This struct can be used for SYNC and ASYNC requests alike.
 */
struct amd_mdb5_dma_io_request {
	struct amd_mdb5_dma_io_completion	comp;
	void					*private;
	void __user				*buf;
	size_t					buflen;
	unsigned int				pages_nr;
	struct sg_table				sgt;
	bool					sgt_dma_mapped;
	struct page				**pages;
	bool					write;
	u64					ep_addr;
	wait_queue_head_t			wq;
};

/*
 * amd_mdb5_dma_aio_request
 *
 * This struct is used for tracking, deriving the overall
 * status of the each iovec request and sending that status
 * to the user application.
 *
 * This struct is used only for ASYNC requests.
 */
struct amd_mdb5_dma_aio_request {
	u32					req_count;
	u32					err_count;
	u32					cmp_count;
	int					err;
	struct kiocb				*iocb;
	struct amd_mdb5_dma_io_request		*io_reqs;
	struct amd_mdb5_dma_channel		*hchan;
	void					(*aio_done)(void *);
};


/*
 **********************************************************************
 *              Function Declarations                                 *
 **********************************************************************
 */
int amd_mdb5_dma_cdev_init(void);
void amd_mdb5_dma_cdev_destroy(void);

int amd_mdb5_dma_cdev_ctrl_create(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				  struct device *device);
int amd_mdb5_dma_cdev_ctrl_destroy(struct amd_mdb5_dma_cdev_ctrl *ctrl);
struct amd_mdb5_dma_cdev_ctrl *
amd_mdb5_dma_cdev_ctrl_get(struct amd_mdb5_dma_cdev_ctrl *ctrl);
int amd_mdb5_dma_cdev_ctrl_put(struct amd_mdb5_dma_cdev_ctrl *ctrl);

void
amd_mdb5_dma_tx_direction(struct amd_mdb5_dma_channel *hchan,
			  enum dma_data_direction *data_dir,
			  enum dma_transfer_direction *tx_dir);

int  amd_mdb5_dma_cdev_chan_create(struct amd_mdb5_dma_cdev_chan *chan,
				   struct amd_mdb5_dma_cdev_ctrl *cdev_ctrl,
				   struct amd_mdb5_dma_channel *hchan);
void amd_mdb5_dma_cdev_chan_destroy(struct amd_mdb5_dma_cdev_chan *chan);

ssize_t amd_mdb5_dma_sgdma_read_write(struct amd_mdb5_dma_cdev_chan *chan,
				      void __user *,
				      size_t count, loff_t *pos, bool read_write);

ssize_t amd_mdb5_dma_aio_read_write(struct amd_mdb5_dma_cdev_chan *chan,
				    void __user *,
				    unsigned long count, loff_t pos,
				    bool read_write);

ssize_t amd_mdb5_dma_simple_read_write(struct amd_mdb5_dma_cdev_chan *chan,
				       void __user *,
				       size_t count, loff_t *pos,
				       bool read_write);

ssize_t amd_mdb5_dma_interleaved_read_write(struct amd_mdb5_dma_cdev_chan *chan,
					    struct dma_interleaved_template *xt,
					    bool read_write);

int amd_mdb5_dma_chan_poll_thread_create(struct amd_mdb5_dma_cdev_chan *chan);
void amd_mdb5_dma_chan_poll_thread_destroy(struct amd_mdb5_dma_cdev_chan *chan);

#endif /* __AMD_CLIENT_CDEV_H */
