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

#include <asm/cacheflush.h>
#include <linux/stringhash.h>

#include "mdb5-dmaclient-cdev.h"
#include "mdb5-dmaclient-mod.h"

#define AMD_BURST_SZ_1GB				(0x040000000)
#define AMD_MAX_BURST_SZ				(0x100000000)
#define AMD_MDB5_DMA_NODE_CTRL_NAME_FMT			"mdb5_ctrl"
#define AMD_MDB5_DMA_NODE_CHAN_NAME_FMT			"mdb5_%s%02d"
#define AMD_MDB5_DMA_SALT				0x6D646235

/*********************************************************************/

static struct class *g_mdb5_dma_class = NULL;
static struct kmem_cache *mdb5_dma_cdev_cache;
typedef ssize_t (*rw_ops_t)(struct amd_mdb5_dma_cdev_chan *,
			    void __user *, size_t,
			    loff_t *, bool);

struct io_req_param {
	void __user		*user_buf;
	size_t			user_bufsz;
	u64			ep_addr;
	bool			read_write;
	bool			is_result_cb;
	void			(*cb)(void *);
	void			(*cb_result)(void *,
					const struct dmaengine_result *);
	void			*cb_param;
	wait_queue_head_t	*wq;
	unsigned int		flags;
	void			*pconfig;
	size_t			pconfig_sz;
	void			*private;
};

/*********************************************************************/
static int mdb5_dma_io_req_prepare(struct amd_mdb5_dma_channel *hchan,
				   struct amd_mdb5_dma_io_request *io_req,
				   struct io_req_param *io_req_param);
static void
amd_mdb5_dma_comp_cb(void *args);

static void
amd_mdb5_dma_comp_result_cb(void *args,
			    const struct dmaengine_result *res);

static unsigned int
mdb5_dma_map_page(struct amd_mdb5_dma_channel *hchan,
		      struct amd_mdb5_dma_io_request *io_req);
static void
mdb5_dma_unmap_page(struct amd_mdb5_dma_channel *hchan,
			struct amd_mdb5_dma_io_request *io_req);

static void mdb5_dma_sgdma_unmap_user_buf(struct amd_mdb5_dma_channel *hchan,
					  struct amd_mdb5_dma_io_request *io_req,
					  bool read_write);
static void
amd_mdb5_dma_comp_status_set(struct amd_mdb5_dma_io_completion *comp,
			     enum amd_mdb5_dma_req_status status);
static enum amd_mdb5_dma_req_status
amd_mdb5_dma_comp_status_get(struct amd_mdb5_dma_io_completion *comp);

static void
amd_mdb5_dma_aio_done(void *args);

static int
mdb5_dma_async_tx_status_check(struct amd_mdb5_dma_channel *hchan,
			       struct amd_mdb5_dma_io_completion *comp);

static inline int
mdb5_dma_check_timeout(unsigned long tstamp, unsigned long timeout);

static inline int
mdb5_dma_check_req_pending(struct amd_mdb5_dma_kthread *kth);

static inline int
mdb5_dma_comp_update(struct amd_mdb5_dma_io_completion *comp);

static inline void
mdb5_dma_wake_up_poll_thread(struct amd_mdb5_dma_kthread *kth);

/*********************************************************************/

static inline struct amd_mdb5_dma_io_request *
amd_mdb5_dma_comp2ioreq(struct amd_mdb5_dma_io_completion *io_comp)
{
	return container_of(io_comp,
			    struct amd_mdb5_dma_io_request,
			    comp);
}

static inline struct amd_mdb5_dma_aio_request *
amd_mdb5_dma_ioreq2aioreq(struct amd_mdb5_dma_io_request *io_req)
{
	return io_req->private;
}

static inline struct amd_mdb5_dma_channel *
amd_mdb5_dma_cdev2channel(struct amd_mdb5_dma_cdev_chan *chan)
{
	return container_of(chan,
			    struct amd_mdb5_dma_channel,
			    cdev_chan);
}

static void
amd_mdb5_dma_stats_init(struct amd_mdb5_dma_channel_stats *stats)
{
	if (!stats)
		return;

	atomic64_set(&stats->req_rcvd, 0);
	atomic64_set(&stats->req_success, 0);
	atomic64_set(&stats->req_failed, 0);
	atomic64_set(&stats->intr_rcvd, 0);
	atomic64_set(&stats->read_data_processed, 0);
	atomic64_set(&stats->write_data_processed, 0);
	atomic64_set(&stats->total_data_processed, 0);
}

/**
 * mdb5_dma_ops_open()
 *
 * open call for per channel data path operations
 */
static int mdb5_dma_ops_open(struct inode *inode, struct file *filep)
{
	struct amd_mdb5_dma_cdev_chan *hcdev;
	struct amd_mdb5_dma_channel *hchan;

	hcdev = container_of(inode->i_cdev, struct amd_mdb5_dma_cdev_chan, cdev);
	if (!hcdev)
		return -EINVAL;

	hchan = amd_mdb5_dma_cdev2channel(hcdev);
	mdb5_dma_dbg(hchan->dev, "opening the device node");
	if (hchan->error)
		return hchan->error;
	if (hcdev->chan_open > 0)
		return -EBUSY;

	filep->private_data = hcdev;
	amd_mdb5_dma_stats_init(&hchan->stats);
	hcdev->chan_open = 1;
	mdb5_dma_dbg(hchan->dev, "opened the device node: %s", hcdev->cdev_name);
	return 0;
}

/**
 * mdb5_dma_ops_release()
 *
 * @inode pointer to inode struct
 * @file pointer to file pointer
 *
 * File operations support release call the channel
 * data path operations
 *
 * @return '0' on success, negative error on failure
 */
static int mdb5_dma_ops_release(struct inode *inode, struct file *filep)
{
	struct amd_mdb5_dma_cdev_chan *hcdev;
	struct amd_mdb5_dma_channel *hchan;

	hcdev = container_of(inode->i_cdev, struct amd_mdb5_dma_cdev_chan, cdev);

	hchan = amd_mdb5_dma_cdev2channel(hcdev);
	if (!hcdev->chan_open)
		return -EINVAL;

	mdb5_dma_dbg(hchan->dev, "closing the device node");

	if (hchan) {
		mdb5_dma_dbg(hchan->dev, "Dumping Stats\n");
		mdb5_dma_dbg(hchan->dev, "\n\trequest received = %lld\n"
		       "\t\trequest succeed = %lld\n"
		       "\t\trequest failed = %lld\n"
		       "\t\tinterrupt received = %lld\n"
		       "\t\tread data (bytes) = %lld\n"
		       "\t\twrite data (bytes) = %lld\n"
		       "\t\ttotal data (bytes) = %lld\n",
		       atomic64_read(&hchan->stats.req_rcvd),
		       atomic64_read(&hchan->stats.req_success),
		       atomic64_read(&hchan->stats.req_failed),
		       atomic64_read(&hchan->stats.intr_rcvd),
		       atomic64_read(&hchan->stats.read_data_processed),
		       atomic64_read(&hchan->stats.write_data_processed),
		       atomic64_read(&hchan->stats.total_data_processed));
	}

	hcdev->chan_open = 0;
	filep->private_data = NULL;
	mdb5_dma_dbg(hchan->dev, "closed the device node: %s", hcdev->cdev_name);
	return 0;
}

/**
 * mdb5_dma_simple_read_write()
 *
 * @chan pointer having channel struct
 * @buf user provided buffer
 * @count size of the data in the user buffer
 * @pos position to be used in the user buffer
 * @read_write true for 'write' and false for 'read'
 *
 * Function supports the simple mode of read / write
 * DMA transfer
 *
 * @return number of bytes transferred on success, negative error on failure
 */
static ssize_t mdb5_dma_simple_read_write(struct amd_mdb5_dma_cdev_chan *chan,
				      void __user *buf, size_t count,
				      loff_t *pos, bool read_write)
{
	struct amd_mdb5_dma_channel *hchan = amd_mdb5_dma_cdev2channel(chan);
	struct amd_mdb5_dma_io_completion *comp = NULL;
	struct amd_mdb5_dma_io_request *io_req = NULL;
	struct dma_chan *dchan = hchan->dchan;
	struct io_req_param io_req_param;
	enum dma_status status;
	ssize_t ret = -EINVAL;
	int non_ll = 1;

	mdb5_dma_dbg(hchan->dev, "mdb5_dma_simple_read_write start");
	if (( read_write && hchan->dir == AMD_MDB5_DMA_CHAN_WR) ||
	    (!read_write && hchan->dir == AMD_MDB5_DMA_CHAN_RD)) {
		mdb5_dma_err(hchan->dev, "%s r/w mismatch. W %d, dir %d.\n",
			chan->cdev_name, read_write, hchan->dir);
		return -EINVAL;
	}

	io_req = kzalloc(sizeof(*io_req), GFP_KERNEL);
	if (!io_req) {
		mdb5_dma_err(hchan->dev, "out of memory");
		return -ENOMEM;
	}

	io_req->buf = buf;
	comp = &io_req->comp;
	init_waitqueue_head(&io_req->wq);

	memset(&io_req_param, 0, sizeof(io_req_param));
	io_req_param.user_buf	= buf;
	io_req_param.user_bufsz	= count;
	io_req_param.ep_addr	= *pos;
	io_req_param.read_write	= read_write;
	io_req_param.is_result_cb = true;
	io_req_param.cb_result	= amd_mdb5_dma_comp_result_cb;
	io_req_param.cb_param	= &io_req->comp;
	io_req_param.wq		= &io_req->wq;
	io_req_param.private	= (void *) hchan;
	io_req_param.pconfig	= (void *) &non_ll;
	io_req_param.pconfig_sz	= sizeof(non_ll);


	ret = mdb5_dma_io_req_prepare(hchan, io_req, &io_req_param);
	if (ret) {
		mdb5_dma_err(hchan->dev, "mdb5_dma_io_req_prepare() failed");
		goto end_simple;
	}

	comp = &io_req->comp;
	/* This call starts the DMA tx */
	dma_async_issue_pending(dchan);

	/* Wait for the result */
	wait_event_interruptible_timeout(*comp->wq,
		amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_INTR_RECV,
		comp->timeout);

	if (hchan->error) {
		ret = hchan->error;
		goto end_simple;
	}

	status = dma_async_is_tx_complete(dchan, comp->cookie, NULL, NULL);
	if (amd_mdb5_dma_comp_status_get(comp) != AMD_MDB5_DMA_REQ_INTR_RECV) {
		ret = -ETIMEDOUT;
		goto end_simple;
	} else if (status != DMA_COMPLETE) {
		if (status == DMA_ERROR) {
			ret = -EFAULT;
			goto end_simple;
		}
	} else {
		ret = count;
	}

	mdb5_dma_dbg(hchan->dev, "mdb5_dma_simple_read_write ret=%ld", ret);
end_simple:
	if (io_req) {
		if (io_req->sgt.sgl)
			mdb5_dma_unmap_page(hchan, io_req);
		mdb5_dma_sgdma_unmap_user_buf(hchan, io_req, read_write);
		kfree(io_req);
	}
	return ret;
}

/**
 * amd_mdb5_dma_comp_status_set()
 *
 * @comp pointer to the completion structure
 * @status status that needs to be set
 *
 * Update the status of the request
 *
 * @return void
 */
static void amd_mdb5_dma_comp_status_set(struct amd_mdb5_dma_io_completion *comp,
					 enum amd_mdb5_dma_req_status status)
{
	if (comp) {
		spin_lock(&comp->lock);
		comp->req_status = status;
		spin_unlock(&comp->lock);
	}
}

/**
 * amd_mdb5_dma_comp_status_get()
 *
 * @comp pointer to the completion structure
 *
 * Get the status of a request
 *
 * @return return the status of the request
 */
static enum amd_mdb5_dma_req_status
amd_mdb5_dma_comp_status_get(struct amd_mdb5_dma_io_completion *comp)
{
	enum amd_mdb5_dma_req_status status = AMD_MDB5_DMA_REQ_NONE;

	if (comp) {
		spin_lock(&comp->lock);
		status = comp->req_status;
		spin_unlock(&comp->lock);
	}
	return status;
}

static void amd_mdb5_dma_handle_abort(struct amd_mdb5_dma_channel *hchan,
				      bool async,
				      struct amd_mdb5_dma_io_completion *comp_single)
{
	struct amd_mdb5_dma_io_completion *comp, *_comp;
	struct amd_mdb5_dma_aio_request *aio_req = NULL;
	struct amd_mdb5_dma_io_request *io_req = NULL;

	/* Set the error on channel */
	hchan->error = -EIO;

	if (async) {
		list_for_each_entry_safe(comp, _comp,
					 &hchan->pend_result, link) {
			if (!comp)
				continue;

			if ((io_req = amd_mdb5_dma_comp2ioreq(comp)) == NULL ||
			    (aio_req = amd_mdb5_dma_ioreq2aioreq(io_req)) == NULL)
				continue;

			aio_req->aio_done((void *) aio_req);
		}
	} else {
		if (comp_single)
			wake_up_interruptible(comp_single->wq);
	}
}

static void amd_mdb5_dma_comp_result_cb(void *args,
					const struct dmaengine_result *res)
{
	struct amd_mdb5_dma_aio_request *aio_req = NULL;
	struct amd_mdb5_dma_io_request *io_req = NULL;
	struct amd_mdb5_dma_channel *hchan = NULL;
	struct amd_mdb5_dma_io_completion *comp;

	comp = (struct amd_mdb5_dma_io_completion *) args;
	if (!comp || comp->comp_handled)
		return;

	if ((io_req = amd_mdb5_dma_comp2ioreq(comp)) != NULL) {
		if ((comp->flags &
		     (AMD_MDB5_DMA_CHAN_ASYNC_MODE |
		      AMD_MDB5_DMA_CHAN_APERTURE_MODE)) &&
		    (aio_req = amd_mdb5_dma_ioreq2aioreq(io_req)) != NULL) {
			hchan = aio_req->hchan;
		} else {
			hchan = (struct amd_mdb5_dma_channel *) io_req->private;
		}
	}

	/* An abort occurred */
	if (res->result == DMA_TRANS_ABORTED) {
		amd_mdb5_dma_handle_abort(hchan,
				comp->flags & AMD_MDB5_DMA_CHAN_ASYNC_MODE, comp);
		return;
	}

	if (amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_ERROR)
		return;

	if (hchan)
		atomic64_inc(&hchan->stats.intr_rcvd);

	amd_mdb5_dma_comp_status_set(comp, AMD_MDB5_DMA_REQ_INTR_RECV);
	if (comp->kth && (comp->flags & AMD_MDB5_DMA_CHAN_ASYNC_MODE))
		comp->kth->schedule = 1;

	if (comp->wq)
		wake_up_interruptible(comp->wq);
}

/**
 * amd_mdb5_dma_comp_cb()
 *
 * @args args pointer
 *
 * Callback invoked from the controller interrupt
 * context.
 *
 * @return void
 */
static void amd_mdb5_dma_comp_cb(void *args)
{
	struct amd_mdb5_dma_aio_request *aio_req = NULL;
	struct amd_mdb5_dma_io_request *io_req = NULL;
	struct amd_mdb5_dma_channel *hchan = NULL;
	struct amd_mdb5_dma_io_completion *comp;

	comp = (struct amd_mdb5_dma_io_completion *) args;
	if (!comp)
		return;

	if ((io_req = amd_mdb5_dma_comp2ioreq(comp)) != NULL) {
		if (comp->flags &&
		    (aio_req = amd_mdb5_dma_ioreq2aioreq(io_req)) != NULL) {
			hchan = aio_req->hchan;
		} else {
			hchan = (struct amd_mdb5_dma_channel *) io_req->private;
		}
	}

	if (amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_ERROR)
		return;

	if (hchan)
		atomic64_inc(&hchan->stats.intr_rcvd);

	amd_mdb5_dma_comp_status_set(comp, AMD_MDB5_DMA_REQ_INTR_RECV);
	if (comp->kth && (comp->flags & AMD_MDB5_DMA_CHAN_ASYNC_MODE))
		comp->kth->schedule = 1;

	if (comp->wq)
		wake_up_interruptible(comp->wq);
}

/**
 * amd_mdb5_dma_tx_direction()
 *
 * @hchan channel structure
 * @data_dir pointer to dma_data_direction type
 * @tx_dir pointer to dma_transfer_direction type
 *
 * Based on the channel direction provide the dir required
 * for DMAEngine specific APIs and dma_map*() operations.
 *
 * @return void
 */
void
amd_mdb5_dma_tx_direction(struct amd_mdb5_dma_channel *hchan,
			  enum dma_data_direction *data_dir,
			  enum dma_transfer_direction *tx_dir)
{
	if (hchan->dir == AMD_MDB5_DMA_CHAN_WR) {
		if (data_dir)
			*data_dir = DMA_FROM_DEVICE;
		if (tx_dir)
			*tx_dir = DMA_DEV_TO_MEM;
	}
	if (hchan->dir == AMD_MDB5_DMA_CHAN_RD) {
		if (data_dir)
			*data_dir = DMA_TO_DEVICE;
		if (tx_dir)
			*tx_dir = DMA_MEM_TO_DEV;
	}
}

/**
 * mdb5_dma_map_page()
 *
 * @hchan channel pointer
 * @io_req DMA request by the user
 *
 * Map the pages provided by the user in to a scatter-gather list.
 *
 * @return number of pages mapped on sucess, '0' on error
 */
static unsigned int mdb5_dma_map_page(struct amd_mdb5_dma_channel *hchan,
					  struct amd_mdb5_dma_io_request *io_req)
{
	struct sg_table *sgt = &io_req->sgt;
	unsigned int nents = sgt->nents;
	enum dma_data_direction dir;
	unsigned int ret;

	amd_mdb5_dma_tx_direction(hchan, &dir, NULL);

	ret = dma_map_sg(hchan->dev, sgt->sgl, nents, dir);
	if (!ret) {
		mdb5_dma_err(hchan->dev, "dma map sg failed");
		return ret;
	}
	sgt->nents = ret;
	io_req->sgt_dma_mapped = true;
	return ret;
}

/**
 * mdb5_dma_unmap_page()
 *
 * @hchan channel structure
 * @io_req io request sent by user
 *
 * Unmap the scatter-gather list.
 *
 * @return void
 */
static void mdb5_dma_unmap_page(struct amd_mdb5_dma_channel *hchan,
				    struct amd_mdb5_dma_io_request *io_req)
{
	struct sg_table *sgt = &io_req->sgt;
	enum dma_data_direction dir;

	if (!io_req->sgt_dma_mapped)
		return;

	amd_mdb5_dma_tx_direction(hchan, &dir, NULL);
	dma_unmap_sg(hchan->dev, sgt->sgl, sgt->nents, dir);
	io_req->sgt_dma_mapped = false;
}

/**
 * mdb5_dma_sgdma_unmap_user_buf()
 *
 * @io_req io request issued by the user
 * @read_write type of DMA operation
 *
 * Unpins the pages for the addresses provided by the user-space.
 *
 * @return void
 */
static void mdb5_dma_sgdma_unmap_user_buf(struct amd_mdb5_dma_channel *hchan,
					  struct amd_mdb5_dma_io_request *io_req,
					  bool read_write)
{
	int i = 0;

	if (!io_req)
		return;

	sg_free_table(&io_req->sgt);

	if (!io_req->pages)
		return;
	/*
 	 * This code will 'put' down the pinned pages.
 	 * The 'read_write' op corresponds to a write to the
 	 * device memory. So for the read_write=0, aka read
 	 * ops the pages given by the user are pinned
 	 * and written by the read op from device to
 	 * host which should be set to dirty, hence
 	 * set_page_dirty_lock(). For read_write=1 aka write
 	 * to device memory the pages are read from host
 	 * hence no set_page_dirty_lock() require.
 	 */
	for (i = 0; i < io_req->pages_nr; i++) {
		if (io_req->pages[i]) {
			if (!read_write)
				set_page_dirty_lock(io_req->pages[i]);
			put_page(io_req->pages[i]);
		} else {
			break;
		}
	}

	if (i != io_req->pages_nr)
		mdb5_dma_dbg(hchan->dev, "sgl pages %d/%u.\n", i, io_req->pages_nr);

	kfree(io_req->pages);
	io_req->pages = NULL;
}

/**
 * mdb5_dma_sgdma_map_user_buf_to_sgl()
 *
 * @io_req pointer to the user request
 *
 * Pins the pages for the addresses provided by the user-space.
 *
 * @return '0' on succes, negative on error
 */
static int mdb5_dma_sgdma_map_user_buf_to_sgl(struct amd_mdb5_dma_channel *hchan,
					      struct amd_mdb5_dma_io_request *io_req)
{
	struct sg_table *sgt = &io_req->sgt;
	unsigned long len = io_req->buflen;
	void __user *buf = io_req->buf;
	unsigned int i, pages_nr;
	struct scatterlist *sgl;
	int ret;

	pages_nr = (len + offset_in_page(buf) + PAGE_SIZE - 1) >> PAGE_SHIFT;
	if (!pages_nr)
		return -EINVAL;

	if (sg_alloc_table(sgt, pages_nr, GFP_KERNEL)) {
		mdb5_dma_err(hchan->dev, "sgl OOM.\n");
		return -ENOMEM;
	}

	io_req->pages = kcalloc(pages_nr, sizeof(struct page *), GFP_KERNEL);
	if (!io_req->pages) {
		mdb5_dma_err(hchan->dev, "pages OOM.\n");
		ret = -ENOMEM;
		goto err_out;
	}

	ret = get_user_pages_fast((unsigned long)buf, pages_nr, FOLL_WRITE,
				io_req->pages);
	/* No pages were pinned */
	if (ret < 0) {
		mdb5_dma_err(hchan->dev, "unable to pin down %u user pages, %d.\n",
			     pages_nr, ret);
		goto err_out;
	}
	/* Less pages pinned than wanted */
	if (ret != pages_nr) {
		mdb5_dma_err(hchan->dev, "unable to pin down all %u user pages, %d.\n",
			     pages_nr, ret);
		io_req->pages_nr = ret;
		ret = -EFAULT;
		goto err_out;
	}

	sgl = sgt->sgl;
	for (i = 0; i < pages_nr; i++, sgl = sg_next(sgl)) {
		unsigned int offset = offset_in_page(buf);
		unsigned int nbytes =
			min_t(unsigned int, PAGE_SIZE - offset, len);

		flush_dcache_page(io_req->pages[i]);
		sg_set_page(sgl, io_req->pages[i], nbytes, offset);

		buf += nbytes;
		len -= nbytes;
	}

	io_req->pages_nr = i;
	if (len) {
		mdb5_dma_err(hchan->dev,
			     "Invalid user buffer length. Cannot map to sgl\n");
		ret = -EINVAL;
		goto err_out;
	}


	return 0;

err_out:
	mdb5_dma_sgdma_unmap_user_buf(hchan, io_req, io_req->write);

	return ret;
}

/**
 * mdb5_dma_interleaved_read_write()
 *
 * @chan channel structure
 * @xt pointer to the template
 * @read_write direction of the DMA operation
 *
 * Performs the interleaved mode of transfer as per the inputs given
 * in the template 'xt'
 *
 * @return bytes transferred on success, negative value on error
 */
static ssize_t mdb5_dma_interleaved_read_write(struct amd_mdb5_dma_cdev_chan *chan,
					       struct dma_interleaved_template *xt,
					       bool read_write)
{
	struct amd_mdb5_dma_io_completion *comp = NULL;
	struct dma_async_tx_descriptor *txdesc;
	struct amd_mdb5_dma_io_request io_req;
	struct amd_mdb5_dma_channel *hchan;
	enum dma_transfer_direction dir;
	struct dma_slave_config sconf;
	struct dma_chan *dchan;
	int ret = -EINVAL;

	hchan = amd_mdb5_dma_cdev2channel(chan);
	dchan = hchan->dchan;

	if (( read_write && hchan->dir == AMD_MDB5_DMA_CHAN_WR) ||
	    (!read_write && hchan->dir == AMD_MDB5_DMA_CHAN_RD)) {
		mdb5_dma_err(hchan->dev, "%s r/w mismatch. W %d, dir %d.\n",
			     chan->cdev_name, read_write, hchan->dir);
		return -EINVAL;
	}

	if (!xt)
		return ret;

	memset(&io_req, 0, sizeof(io_req));
	init_waitqueue_head(&io_req.wq);
	amd_mdb5_dma_tx_direction(hchan, NULL, &dir);
	xt->dir = dir;

	memset(&sconf, 0, sizeof(sconf));
	sconf.src_addr = xt->src_start;
	sconf.dst_addr = xt->dst_start;

	mdb5_dma_dbg(hchan->dev, "src: 0x%lx, dst: 0x%lx",
		(unsigned long) sconf.src_addr,
		(unsigned long) sconf.dst_addr);
	dmaengine_slave_config(dchan, &sconf);

	txdesc = dmaengine_prep_interleaved_dma(dchan, xt,
						DMA_PREP_INTERRUPT);
	if (!txdesc) {
		mdb5_dma_err(hchan->dev, "dma engine prep failed");
		ret = -EFAULT;
		goto end_intl;
	}

	comp          = &io_req.comp;
	comp->wq      = &io_req.wq;
	comp->txdesc  = txdesc;
	comp->timeout = msecs_to_jiffies(AMD_MDB5_DMA_REQ_TIMEOUT_MS);

	amd_mdb5_dma_comp_status_set(comp, AMD_MDB5_DMA_REQ_SUBMIT);
	txdesc->callback       = amd_mdb5_dma_comp_cb;
	txdesc->callback_param = comp;

	comp->cookie = dmaengine_submit(txdesc);
	if (dma_submit_error(comp->cookie))
		goto end_intl;

	/* This call starts the DMA tx */
	dma_async_issue_pending(dchan);

	/* Wait for the result */
	wait_event_interruptible_timeout(*comp->wq,
		amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_INTR_RECV,
		comp->timeout);

	ret = mdb5_dma_async_tx_status_check(hchan, comp);

end_intl:
	return ret;
}

/**
 * mdb5_dma_io_req_prepare() - Prepare an I/O request for DMA transfer
 * @hchan: Pointer to DMA channel
 * @io_req: Pointer to I/O request structure
 * @io_req_param: Pointer to I/O request parameters
 *
 * Prepares the I/O request structure for a DMA transfer by mapping user buffer,
 * setting up the scatter-gather list, and configuring DMA parameters.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int mdb5_dma_io_req_prepare(struct amd_mdb5_dma_channel *hchan,
				   struct amd_mdb5_dma_io_request *io_req,
				   struct io_req_param *io_req_param)
{
	bool read_write = io_req_param->read_write;
	struct amd_mdb5_dma_io_completion *comp = NULL;
	struct dma_async_tx_descriptor *txdesc;
	enum dma_transfer_direction dir;
	struct dma_slave_config sconf;
	struct dma_chan *dchan;
	int ret = 0;
	bool async = false;

	dchan = hchan->dchan;
	amd_mdb5_dma_tx_direction(hchan, NULL, &dir);

	io_req->private = io_req_param->private;
	comp = &io_req->comp;
	INIT_LIST_HEAD(&comp->link);

	io_req->buf    = io_req_param->user_buf;
	io_req->buflen = io_req_param->user_bufsz;
	io_req->write  = read_write;

	if (io_req_param->flags & AMD_MDB5_DMA_CHAN_ASYNC_MODE)
		async = true;

	ret = mdb5_dma_sgdma_map_user_buf_to_sgl(hchan, io_req);
	if (ret) {
		ret = -EFAULT;
		goto end_ops;
	}

	io_req->ep_addr = io_req_param->ep_addr;

	ret = mdb5_dma_map_page(hchan, io_req);
	if (!ret) {
		ret = -EFAULT;
		goto end_ops;
	}

	mdb5_dma_dbg(hchan->dev, "buf:0x%lx, count: %lu, AXI: 0x%lx, w: %d",
		     (unsigned long) io_req->buf, io_req->buflen,
		     (unsigned long) io_req->ep_addr, io_req->write);

	memset(&sconf, 0, sizeof(sconf));
	if (dir == DMA_DEV_TO_MEM) {
		sconf.src_addr = (u64) io_req->ep_addr;
		sconf.dst_addr = sg_dma_address(io_req->sgt.sgl);
	} else {
		sconf.src_addr = sg_dma_address(io_req->sgt.sgl);
		sconf.dst_addr = (u64) io_req->ep_addr;
	}

	mdb5_dma_dbg(hchan->dev, "src: 0x%lx, dst: 0x%lx",
		     (unsigned long) sconf.src_addr,
		     (unsigned long) sconf.dst_addr);

	if (io_req_param->pconfig_sz > 0 &&
	    io_req_param->pconfig &&
	    !async &&
	    hchan->mode == AMD_MDB5_DMA_MODE_SIMPLE) {
		sconf.peripheral_config = io_req_param->pconfig;
		sconf.peripheral_size   = io_req_param->pconfig_sz;
	}

	dmaengine_slave_config(dchan, &sconf);

	txdesc = dmaengine_prep_slave_sg(dchan,
					 io_req->sgt.sgl,
					 io_req->sgt.nents,
					 dir,
					 DMA_PREP_INTERRUPT);
	if (!txdesc) {
		mdb5_dma_err(hchan->dev, "dma prep failed");
		ret = -EFAULT;
		goto end_ops;
	}

	spin_lock_init(&comp->lock);
	comp->tstamp  = jiffies;
	comp->timeout = msecs_to_jiffies(AMD_MDB5_DMA_REQ_TIMEOUT_MS);
	comp->wq      = io_req_param->wq;
	comp->flags   = io_req_param->flags;
	comp->kth     = async ? &hchan->kth : NULL;
	comp->comp_handled = false;

	amd_mdb5_dma_comp_status_set(comp, AMD_MDB5_DMA_REQ_SUBMIT);

	if (!io_req_param->is_result_cb)
		txdesc->callback = io_req_param->cb;
	else
		txdesc->callback_result = io_req_param->cb_result;

	txdesc->callback_param = (void *) io_req_param->cb_param;
	comp->txdesc = txdesc;

	comp->cookie = dmaengine_submit(comp->txdesc);
	if (dma_submit_error(comp->cookie)) {
		ret = -EFAULT;
		goto end_ops;
	}

	return 0;
end_ops:
	mdb5_dma_unmap_page(hchan, io_req);
	mdb5_dma_sgdma_unmap_user_buf(hchan, io_req, read_write);
	return ret;
}

/**
 * mdb5_dma_sgdma_single_xfer() - Perform a single SGDMA transfer
 * @hchan: Pointer to DMA channel
 * @buf: User buffer for transfer
 * @count: Number of bytes to transfer
 * @pos: Endpoint address (offset)
 * @read_write: Direction of transfer (read/write)
 *
 * Initiates a single scatter-gather DMA transfer for the specified buffer and direction.
 *
 * Return: Number of bytes transferred on success, negative error code on failure.
 */
static ssize_t mdb5_dma_sgdma_single_xfer(struct amd_mdb5_dma_channel *hchan,
					  void __user *buf, size_t count,
					  loff_t *pos, bool read_write)
{
	struct amd_mdb5_dma_io_request *io_req = NULL;
	struct amd_mdb5_dma_io_completion *comp = NULL;
	struct io_req_param io_req_param;
	enum dma_status status;
	ssize_t ret = 0;

	io_req = kzalloc(sizeof(*io_req), GFP_KERNEL);
	if (!io_req)
		return -ENOMEM;

	init_waitqueue_head(&io_req->wq);

	memset(&io_req_param, 0, sizeof(io_req_param));
	io_req_param.user_buf	= buf;
	io_req_param.user_bufsz	= count;
	io_req_param.ep_addr	= *pos;
	io_req_param.read_write	= read_write;
	io_req_param.is_result_cb = true;
	io_req_param.cb_result	= amd_mdb5_dma_comp_result_cb;
	io_req_param.cb_param	= &io_req->comp;
	io_req_param.wq		= &io_req->wq;
	io_req_param.private	= (void *)hchan;

	ret = mdb5_dma_io_req_prepare(hchan, io_req, &io_req_param);
	if (ret) {
		mdb5_dma_err(hchan->dev, "mdb5_dma_io_req_prepare() failed\n");
		goto err_ops;
	}

	comp = &io_req->comp;
	dma_async_issue_pending(hchan->dchan);

	/* Wait for the result */
	wait_event_interruptible_timeout(io_req->wq,
	    amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_INTR_RECV,
	    comp->timeout);

	status = hchan->error ?
		 hchan->error :
		 mdb5_dma_async_tx_status_check(hchan, comp);

	ret = status ? status : count;
err_ops:
	if (io_req) {
		if (io_req->sgt.sgl)
			mdb5_dma_unmap_page(hchan, io_req);
		mdb5_dma_sgdma_unmap_user_buf(hchan, io_req, read_write);
		kfree(io_req);
	}
	return ret;
}

/**
 * mdb5_dma_sgdma_aperture_xfer() - Perform an SGDMA transfer using aperture
 * @hchan: Pointer to DMA channel
 * @buf: User buffer for transfer
 * @count: Number of bytes to transfer
 * @pos: Endpoint address (offset)
 * @read_write: Direction of transfer (read/write)
 *
 * Initiates a scatter-gather DMA transfer using the aperture mechanism, splitting
 * the transfer into segments as needed for the aperture size.
 *
 * Return: Number of bytes transferred on success, negative error code on failure.
 */
static ssize_t mdb5_dma_sgdma_aperture_xfer(struct amd_mdb5_dma_channel *hchan,
					    void __user *buf, size_t count,
					    loff_t *pos, bool read_write)
{
	struct amd_mdb5_dma_aio_request *aio_req = NULL;
	struct amd_mdb5_dma_io_completion *comp, *_comp;
	struct amd_mdb5_dma_io_request *io_req = NULL;
	u32 aperture_sz = hchan->aperture_sz;
	struct io_req_param io_req_param;
	struct list_head aperture_list;
	wait_queue_head_t aperture_wq;
	unsigned int i, segments, len;
	size_t tx_count = count;
	ssize_t ret = 0;

	segments = DIV_ROUND_UP(count, aperture_sz);
	INIT_LIST_HEAD(&aperture_list);

	aio_req = kmem_cache_zalloc(mdb5_dma_cdev_cache, GFP_KERNEL);
	if (!aio_req)
		return -ENOMEM;

	aio_req->io_reqs = kcalloc(segments, sizeof(struct amd_mdb5_dma_io_request),
				   GFP_KERNEL);
	if (!aio_req->io_reqs) {
		ret = -ENOMEM;
		goto err_ops;
	}

	aio_req->aio_done = amd_mdb5_dma_aio_done;
	aio_req->hchan = hchan;
	init_waitqueue_head(&aperture_wq);

	for (i = 0; i < segments; i++) {
		io_req = &aio_req->io_reqs[i];
		comp = &io_req->comp;

		len = min_t(unsigned int, count, aperture_sz);

		memset(&io_req_param, 0, sizeof(io_req_param));
		io_req_param.user_buf =
				(char __user *)buf + (i * aperture_sz);
		io_req_param.user_bufsz	= len;
		io_req_param.ep_addr	= *pos;
		io_req_param.read_write	= read_write;
		io_req_param.is_result_cb = true;
		io_req_param.cb_result  = amd_mdb5_dma_comp_result_cb;
		io_req_param.cb_param   = &io_req->comp;
		io_req_param.wq		= &aperture_wq;
		io_req_param.flags	= AMD_MDB5_DMA_CHAN_APERTURE_MODE;
		io_req_param.private	= (void *)aio_req;

		ret = mdb5_dma_io_req_prepare(hchan, io_req, &io_req_param);
		if (ret) {
			mdb5_dma_err(hchan->dev, "segment %u of %u prepare failed\n",
			       i, segments);
			goto err_ops;
		}

		count -= len;
		++aio_req->req_count;
		list_add_tail(&comp->link, &aperture_list);
	}

	dma_async_issue_pending(hchan->dchan);

	list_for_each_entry_safe(comp, _comp, &aperture_list, link) {
		wait_event_interruptible_timeout(*comp->wq,
						 amd_mdb5_dma_comp_status_get(comp)
						 == AMD_MDB5_DMA_REQ_INTR_RECV,
						 comp->timeout);

		if (amd_mdb5_dma_comp_status_get(comp) == AMD_MDB5_DMA_REQ_INTR_RECV) {
			if (mdb5_dma_async_tx_status_check(hchan, comp) == 0)
				++aio_req->cmp_count;
			else
				++aio_req->err_count;
		} else {
			++aio_req->err_count;
		}
		list_del(&comp->link);
	}

	if (hchan->error)
		aio_req->err = hchan->error;
	else if (aio_req->err_count > 0)
		aio_req->err = -EIO;

	ret = aio_req->err ? aio_req->err : tx_count;

 err_ops:
	if (aio_req)
		aio_req->aio_done((void *) aio_req);

	return ret;
}

/**
 * mdb5_dma_sgdma_read_write()
 *
 * @chan channel structure
 * @buf user buffer
 * @count size of the user buffer
 * @pos pointer to the offset within the user buffer
 * @read_write direction of the DMA transfer
 *
 * Perform the scatter-gather based DMA transfer, based on the
 * inputs provided by the user
 *
 * @return bytes transferred successfully, negative error on error
 */
static ssize_t mdb5_dma_sgdma_read_write(struct amd_mdb5_dma_cdev_chan *chan,
					 void __user *buf,
					 size_t count, loff_t *pos, bool read_write)
{
	struct amd_mdb5_dma_channel *hchan = amd_mdb5_dma_cdev2channel(chan);
	ssize_t ret;

	mdb5_dma_dbg(hchan->dev, "sgdma %s: count=%zu, pos=0x%llx\n",
	       read_write ? "write" : "read", count, (u64) * pos);

	if (( read_write && hchan->dir == AMD_MDB5_DMA_CHAN_WR) ||
	    (!read_write && hchan->dir == AMD_MDB5_DMA_CHAN_RD)) {
		mdb5_dma_err(hchan->dev, "%s r/w mismatch. W %d, dir %d.\n",
			     chan->cdev_name, read_write, hchan->dir);
		return -EINVAL;
	}

	if (count <= 0 || count > AMD_MAX_BURST_SZ) {
		mdb5_dma_err(hchan->dev, "buffer size not supported: %zu\n", count);
		return -EINVAL;
	}

	/* If aperture_sz is set, use the multi-segment transfer path */
	if (!hchan->aperture_sz) {
		ret = mdb5_dma_sgdma_single_xfer(hchan, buf, count, pos,
						 read_write);
	} else {
		ret = mdb5_dma_sgdma_aperture_xfer(hchan, buf, count, pos,
						   read_write);
	}

	mdb5_dma_dbg(hchan->dev, "mdb5_dma_sgdma_read_write ret=%ld\n", ret);
	return ret;
}

/**
 * mdb5_dma_ops_read()
 *
 * @filep pointer to the file structure
 * @buf pointer to the user buffer
 * @size size of the user buffer in bytes
 * @ppos offset within the user buffer
 *
 * Read operation, supports the scatter-gather and simple mode
 *
 * @return bytes successfully transferred, negative on error
 */
static ssize_t mdb5_dma_ops_read(struct file *filep, char __user *buf,
				 size_t size, loff_t *ppos)
{
	struct amd_mdb5_dma_cdev_chan *chan;
	struct amd_mdb5_dma_channel *hchan;
	ssize_t ret = -EINVAL;
	rw_ops_t rw_ops;

	chan = (struct amd_mdb5_dma_cdev_chan *) filep->private_data;
	if (!chan)
		goto end_read;
	hchan = amd_mdb5_dma_cdev2channel(chan);

	atomic64_inc(&hchan->stats.req_rcvd);

	switch (hchan->mode) {
	default:
	case AMD_MDB5_DMA_MODE_SG:
		rw_ops = mdb5_dma_sgdma_read_write;
		break;
	case AMD_MDB5_DMA_MODE_SIMPLE:
		rw_ops = mdb5_dma_simple_read_write;
		break;
	}

	ret = rw_ops(chan, buf, size, ppos, false);
	if (ret == size) {
		atomic64_add((s64) size, &hchan->stats.total_data_processed);
		atomic64_add((s64) size, &hchan->stats.read_data_processed);
		atomic64_inc(&hchan->stats.req_success);
	} else {
		atomic64_inc(&hchan->stats.req_failed);
	}
end_read:
	return ret;
}

/**
 * mdb5_dma_ops_write()
 *
 * @filep pointer to the file structure
 * @buf pointer to the user buffer
 * @size size of the user buffer in bytes
 * @ppos offset within the user buffer
 *
 * Write operation, supports the scatter-gather and simple mode
 *
 * @return bytes successfully transferred, negative on error
 */
static ssize_t mdb5_dma_ops_write(struct file *filep, const char __user *buf,
			      size_t size, loff_t *ppos)
{
	struct amd_mdb5_dma_cdev_chan *chan;
	struct amd_mdb5_dma_channel *hchan;
	ssize_t ret = -EINVAL;
	rw_ops_t rw_ops;

	chan = (struct amd_mdb5_dma_cdev_chan *) filep->private_data;
	if (!chan)
		goto end_write;
	hchan = amd_mdb5_dma_cdev2channel(chan);

	atomic64_inc(&hchan->stats.req_rcvd);

	switch (hchan->mode) {
	default:
	case AMD_MDB5_DMA_MODE_SG:
		rw_ops = mdb5_dma_sgdma_read_write;
		break;
	case AMD_MDB5_DMA_MODE_SIMPLE:
		rw_ops = mdb5_dma_simple_read_write;
		break;
	}

	ret = rw_ops(chan, (void __user *) buf, size, ppos, true);
	if (ret == size) {
		atomic64_add((s64) size, &hchan->stats.total_data_processed);
		atomic64_add((s64) size, &hchan->stats.write_data_processed);
		atomic64_inc(&hchan->stats.req_success);
	} else {
		atomic64_inc(&hchan->stats.req_failed);
	}

end_write:
	return ret;
}

/**
 * mdb5_dma_append_to_pend_list()
 *
 * @hchan pointer to the channel structure
 * @comp pointer to the completion structure
 *
 * Append an ASYNC request to the pending list.
 *
 * @return void
 */
static void mdb5_dma_append_to_pend_list(struct amd_mdb5_dma_channel *hchan,
					 struct amd_mdb5_dma_io_completion *comp)
{
	spin_lock(&hchan->pend_lock);
	list_add_tail(&comp->link, &hchan->pend_result);
	hchan->pend_count++;
	spin_unlock(&hchan->pend_lock);
}

/**
 * mdb5_dma_remove_from_pend_list()
 *
 * @hchan pointer to the channel structure
 * @comp pointer to the completion structure
 *
 * Remove an ASYNC request from the pending list
 * after its status is delivered.
 *
 * @return void
 */
static void mdb5_dma_remove_from_pend_list(struct amd_mdb5_dma_channel *hchan,
					   struct amd_mdb5_dma_io_completion *comp)
{
	spin_lock(&hchan->pend_lock);
	list_del(&comp->link);
	hchan->pend_count--;
	spin_unlock(&hchan->pend_lock);
}

/**
 * mdb5_dma_wake_up_poll_thread()
 *
 * @kth pointer to the thread structure
 *
 * Wakes up the poll thread
 *
 * @return void
 */
static inline void mdb5_dma_wake_up_poll_thread(struct amd_mdb5_dma_kthread *kth)
{
	if (!kth || !mdb5_dma_check_req_pending(kth))
		return;

	kth->schedule = 1;
	wake_up_interruptible(&kth->poll_wq);
}

/**
 * amd_mdb5_dma_aio_done()
 *
 * @args pointer of void type
 *
 * Callback invoked upon completion of async requests
 *
 * @return void
 */
static void amd_mdb5_dma_aio_done(void *args)
{
	struct amd_mdb5_dma_io_completion *comp = NULL;
	struct amd_mdb5_dma_channel *hchan = NULL;
	struct amd_mdb5_dma_aio_request *aio_req;
	struct amd_mdb5_dma_io_request *io_req;
	enum amd_mdb5_dma_req_status status;
	struct kiocb *iocb = NULL;
	bool async = false;
	long result = 0;
	u32 i;

	aio_req = (struct amd_mdb5_dma_aio_request *) args;
	if (!aio_req)
		return;

	if (!aio_req->iocb)
		return;

	hchan = aio_req->hchan;
	iocb = aio_req->iocb;
	result = (long) (aio_req->cmp_count);
	aio_req->iocb = NULL;

	for (i = 0; i < aio_req->req_count; i++) {
		io_req = &aio_req->io_reqs[i];
		comp   = &io_req->comp;
		status = amd_mdb5_dma_comp_status_get(comp);

		async = comp->flags & AMD_MDB5_DMA_CHAN_ASYNC_MODE;
		if (hchan && async && status == AMD_MDB5_DMA_REQ_COMPLETED) {
			atomic64_add((s64) io_req->buflen,
					&hchan->stats.total_data_processed);
			atomic64_add((s64) io_req->buflen,
					io_req->write ?
					   &hchan->stats.write_data_processed :
					   &hchan->stats.read_data_processed);
		}

		mdb5_dma_unmap_page(hchan, io_req);
		mdb5_dma_sgdma_unmap_user_buf(hchan, io_req, io_req->write);

		if (async && !comp->comp_handled)
			mdb5_dma_remove_from_pend_list(aio_req->hchan, comp);
	}

	if (async) {
		atomic64_add((s64) aio_req->cmp_count, &hchan->stats.req_success);
		atomic64_add((s64) aio_req->err_count, &hchan->stats.req_failed);
	}

	kfree(aio_req->io_reqs);
	aio_req->io_reqs = NULL;

	if (iocb)
		iocb->ki_complete(iocb, result);

	kmem_cache_free(mdb5_dma_cdev_cache, aio_req);
}

/**
 * mdb5_dma_ops_aio_read_write()
 *
 * @ctx context pointer
 * @iovec pointer to iovec type
 * @count count of iovec
 * @pos position in the file worked upon
 * @read_write type of DMA transter
 *
 * For async transactions based on the iovec based
 * inputs. Each iovec entry is an SG list in itself and
 * submitted to the controller, collectively.
 */
static ssize_t mdb5_dma_ops_aio_read_write(void *ctx, void *iovec,
					   unsigned long count,
					   loff_t pos, bool read_write)
{
	const struct iovec *io = (const struct iovec *) iovec;
	struct amd_mdb5_dma_aio_request *aio_req = NULL;
	struct amd_mdb5_dma_io_request *io_req = NULL;
	struct amd_mdb5_dma_channel *hchan = NULL;
	struct amd_mdb5_dma_io_completion *comp;
	struct amd_mdb5_dma_cdev_chan *chan;
	struct io_req_param io_req_param;
	enum dma_transfer_direction dir;
	struct kiocb *iocb = NULL;
	struct dma_chan *dchan;
	ssize_t ret = -EINVAL;
	long i = 0, j = 0;
	size_t idx = 0;
	size_t sz;

	iocb = (struct kiocb *) ctx;
	chan = (struct amd_mdb5_dma_cdev_chan *) iocb->ki_filp->private_data;

	if (!chan || !(hchan = amd_mdb5_dma_cdev2channel(chan))) {
		return ret;
	}

	mdb5_dma_dbg(hchan->dev, "mdb5_dma_aio_read_write start");
	atomic64_inc(&hchan->stats.req_rcvd);

	if (( read_write && hchan->dir == AMD_MDB5_DMA_CHAN_WR) ||
	    (!read_write && hchan->dir == AMD_MDB5_DMA_CHAN_RD)) {
		mdb5_dma_err(hchan->dev, "%s r/w mismatch. W %d, dir %d.\n",
			     chan->cdev_name, read_write, hchan->dir);
		return -EINVAL;
	}

	dchan = hchan->dchan;
	amd_mdb5_dma_tx_direction(hchan, NULL, &dir);

	aio_req = kmem_cache_zalloc(mdb5_dma_cdev_cache, GFP_KERNEL);
	if (!aio_req) {
		mdb5_dma_err(hchan->dev, "out of memory");
		return -ENOMEM;
	}

	sz = count * sizeof(struct amd_mdb5_dma_io_request);
	aio_req->io_reqs = kzalloc(sz, GFP_KERNEL);
	if (!aio_req->io_reqs) {
		mdb5_dma_err(hchan->dev, "out of memory: %lu bytes", sz);
		ret = -ENOMEM;
		goto end_out;
	}
	iocb->private = aio_req;
	aio_req->aio_done = amd_mdb5_dma_aio_done;

	aio_req->iocb = iocb;
	aio_req->hchan = hchan;

	mutex_lock(&hchan->mutex);
	for (i = 0; i < count; i++) {
		io_req = &aio_req->io_reqs[i];

		if ((io[i].iov_len == 0) ||
		    (io[i].iov_len > AMD_MDB5_DMA_MAX_BURST_SZ)) {
			mdb5_dma_err(hchan->dev, "invalid input length");
			ret = -EFAULT;
			mutex_unlock(&hchan->mutex);
			goto end_out;
		}

		memset(&io_req_param, 0, sizeof(io_req_param));

		io_req_param.user_buf	= io[i].iov_base;
		io_req_param.user_bufsz	= io[i].iov_len;
		io_req_param.ep_addr	= pos + idx;
		io_req_param.read_write	= read_write;
		io_req_param.is_result_cb = true;
		io_req_param.cb_result	= amd_mdb5_dma_comp_result_cb;
		io_req_param.cb_param	= &io_req->comp;
		io_req_param.wq		= &hchan->kth.poll_wq;
		io_req_param.flags	= AMD_MDB5_DMA_CHAN_ASYNC_MODE;
		io_req_param.private	= aio_req;

		ret = mdb5_dma_io_req_prepare(hchan, io_req, &io_req_param);
		if (ret < 0) {
			mdb5_dma_err(hchan->dev, "mdb5_dma_io_req_prepare() failed");
			mutex_unlock(&hchan->mutex);
			goto end_out;
		}

		comp = &io_req->comp;
		idx += io[i].iov_len; 

		mdb5_dma_append_to_pend_list(hchan, comp);
		++aio_req->req_count;
	}
	mutex_unlock(&hchan->mutex);

	/* start the dma transfer */
	dma_async_issue_pending(dchan);

	/* Wake up the poll thread */
	mdb5_dma_wake_up_poll_thread(&hchan->kth);

	mdb5_dma_dbg(hchan->dev, "mdb5_dma_aio_read_write exit");
	return -EIOCBQUEUED;

end_out:
	atomic64_inc(&hchan->stats.req_failed);
	if (aio_req) {
		if (aio_req->io_reqs) {
			for (j = 0; j <= i; j++) {
				io_req = &aio_req->io_reqs[j];
				comp   = &io_req->comp;
				mdb5_dma_unmap_page(hchan, io_req);
				mdb5_dma_sgdma_unmap_user_buf(hchan, io_req,
							  read_write);

				if (!list_empty(&comp->link))
					mdb5_dma_remove_from_pend_list(hchan, comp);
			}
			kfree(aio_req->io_reqs);
		}
		kmem_cache_free(mdb5_dma_cdev_cache, aio_req);
	}
	mdb5_dma_dbg(hchan->dev, "mdb5_dma_aio_read_write ret=%ld", ret);
	return ret;
}

static ssize_t mdb5_dma_ops_read_aio(struct kiocb *iocb,
	 			     struct iov_iter *iter)
{
	return mdb5_dma_ops_aio_read_write((void *) iocb,
					   (void *) iter_iov(iter),
					   iter->nr_segs,
					   iocb->ki_pos, false);
}

static ssize_t mdb5_dma_ops_write_aio(struct kiocb *iocb,
				  struct iov_iter *iter)
{
	return mdb5_dma_ops_aio_read_write((void *) iocb,
					   (void *) iter_iov(iter),
					   iter->nr_segs,
					   iocb->ki_pos, true);
}

/**
 * mdb5_dma_memhole_sz()
 *
 * Provide the size for a burst is supported.
 */
static loff_t mdb5_dma_memhole_sz(void)
{
	return AMD_MAX_BURST_SZ;
}

/**
 * mdb5_dma_ops_llseek()
 *
 * @filep file pointer
 * @off offset within the file worked upon
 * @whence direction where offset need to be added
 *
 * API implementation to change the offset at device location
 *
 * @return new offset
 */
static loff_t mdb5_dma_ops_llseek(struct file *filep, loff_t off, int whence)
{
	struct amd_mdb5_dma_cdev_chan *mdb5_dma_cdev;
	loff_t newpos = 0;

	mdb5_dma_cdev = (struct amd_mdb5_dma_cdev_chan *) filep->private_data;

        switch (whence) {
        case SEEK_SET:
                newpos = off;
                break;
        case SEEK_CUR:
                newpos = filep->f_pos + off;
                break;
        case SEEK_END:
                newpos = mdb5_dma_memhole_sz();
		if (off < 0)
			newpos += off;
                break;
        default: /* can't happen */
                return -EINVAL;
        }

        if (newpos < 0)
                return -EINVAL;

        filep->f_pos = newpos;

        return newpos;
}

/**
 * mdb5_dma_chan_fops
 *
 * Per channel bases file_operation struct,
 * the ops mentioned are for the data-path
 * operations.
 */
static struct file_operations mdb5_dma_chan_fops = {
	.owner			= THIS_MODULE,
	.open			= mdb5_dma_ops_open,
	.release		= mdb5_dma_ops_release,
	.read			= mdb5_dma_ops_read,
	.write			= mdb5_dma_ops_write,
	.read_iter		= mdb5_dma_ops_read_aio,
	.write_iter		= mdb5_dma_ops_write_aio,
	.llseek			= mdb5_dma_ops_llseek,
};

/**
 * amd_mdb5_dma_dir2char()
 *
 * @dir direction of the channel
 *
 * Provide the string version of the direction of the channel.
 *
 * @return string version of the type of channel
 */
static char *amd_mdb5_dma_dir2char(enum amd_mdb5_dma_chan_dir dir)
{
	switch (dir) {
	case AMD_MDB5_DMA_CHAN_WR:
		return "read";
	case AMD_MDB5_DMA_CHAN_RD:
		return "write";
	default:
	}
	return "";
}

/**
 * mdb5_dma_check_timeout()
 *
 * @tstamp current timestamp
 * @timeout duration of the timeout
 *
 * Provide whether a req has timed out based on the time it was submitted.
 *
 * @return 'true' if timeout occurs, 'false' if no timeout occurred
 */
static inline int mdb5_dma_check_timeout(unsigned long tstamp,
					 unsigned long timeout)
{
	return time_after_eq(jiffies, tstamp + timeout);
}

/**
 * mdb5_dma_async_tx_status_check()
 *
 * @hchan channel structure
 * @comp completion strucute
 *
 * Check the status of the sub-requests and put out the status
 *
 * @return '0' on success, negative on error
 */
static int mdb5_dma_async_tx_status_check(struct amd_mdb5_dma_channel *hchan,
					  struct amd_mdb5_dma_io_completion *comp)
{
	struct dma_chan *dchan = hchan->dchan;
	enum dma_status status;
	int ret = 0;

	if (!comp)
		return 0;

	status = dma_async_is_tx_complete(dchan, comp->cookie, NULL, NULL);
	if (amd_mdb5_dma_comp_status_get(comp) != AMD_MDB5_DMA_REQ_INTR_RECV) {
		mdb5_dma_err(hchan->dev, "[%s] dma tx error status: %d",
			     hchan->cdev_chan.cdev_name, (int) status);
		ret = -ETIMEDOUT;
	} else if (status != DMA_COMPLETE) {
		if (status == DMA_ERROR) {
			mdb5_dma_err(hchan->dev, "[%s] dma tx error status: %d",
				     hchan->cdev_chan.cdev_name, (int) status);
			ret = -EFAULT;
		}
		mdb5_dma_dbg(hchan->dev, "dma tx in progress status: %d", (int) status);
	} else {
		mdb5_dma_dbg(hchan->dev, "transfer successful");
		ret = 0;
	}
	return ret;
}

/**
 * mdb5_dma_comp_update()
 *
 * @comp completion structure
 *
 * Update the status of the request based on the following factors
 * - interrupt received from the controller
 * - in case interrupt didn't happen then request timed out.
 *
 * @return '0' if status is changed, '1' is no change in status
 */
static inline int mdb5_dma_comp_update(struct amd_mdb5_dma_io_completion *comp)
{
	enum amd_mdb5_dma_req_status status, new_status;
	struct amd_mdb5_dma_aio_request *aio_req;
	struct amd_mdb5_dma_io_request *io_req;
	struct amd_mdb5_dma_channel *hchan;
	bool timedout;
	u32 req_count;
	int ret;

	if (!comp || comp->comp_handled)
		return 1;

	status = amd_mdb5_dma_comp_status_get(comp);

	io_req   = amd_mdb5_dma_comp2ioreq(comp);
	aio_req  = amd_mdb5_dma_ioreq2aioreq(io_req);
	timedout = mdb5_dma_check_timeout(comp->tstamp, comp->timeout);
	hchan    = aio_req->hchan;
	new_status = status;

	if (status == AMD_MDB5_DMA_REQ_INTR_RECV) {
		new_status = AMD_MDB5_DMA_REQ_COMPLETED;

		ret = mdb5_dma_async_tx_status_check(hchan, comp);
		if (unlikely(timedout || ret)) {
			++aio_req->err_count;
			aio_req->err = ret ? ret : -ETIMEDOUT;
			new_status = AMD_MDB5_DMA_REQ_TIMEDOUT;
		} else {
			++aio_req->cmp_count;
		}
		comp->comp_handled = true;
	} else if ((status == AMD_MDB5_DMA_REQ_SUBMIT) && timedout) {
		aio_req->err = -EIO;
		++aio_req->err_count;
		new_status = AMD_MDB5_DMA_REQ_ERROR;
		comp->comp_handled = true;
	}

	if (status == new_status)
		return 1;

	amd_mdb5_dma_comp_status_set(comp, new_status);
	mdb5_dma_remove_from_pend_list(hchan, comp);

	mdb5_dma_dbg(hchan->dev,
		     "aio_req=%p, completed: %u, request count: %u, error: %u",
		     aio_req, aio_req->cmp_count, aio_req->req_count,
		     aio_req->err_count);

	req_count = aio_req->cmp_count + aio_req->err_count;
	if (aio_req->req_count != req_count)
		return 1;

	aio_req->aio_done((void *) aio_req);
	return 0;
}

/**
 * mdb5_dma_kthread_resched()
 *
 * @kth pointer to poll thread
 *
 * Make the thread inactive / active based on the reqs pending
 * on the per channel pending list.
 *
 * @return void
 */
static inline void mdb5_dma_kthread_resched(struct amd_mdb5_dma_kthread *kth)
{
	if (kth->timeout) {
		mdb5_dma_dbg(kth->hchan->dev, "%s: rescheduling for %u secs",
		       kth->kthread_name, kth->timeout);
		wait_event_interruptible_timeout(kth->poll_wq,
					 	 kth->schedule,
					 	 kth->timeout);
	} else {
		mdb5_dma_dbg(kth->hchan->dev, "%s: rescheduling", kth->kthread_name);
		wait_event_interruptible(kth->poll_wq, kth->schedule);
	}
}

/**
 * mdb5_dma_check_req_pending()
 *
 * @kth pointer to polling thread
 *
 * Check whether reqs are pending on the per channel pending list.
 *
 * @return number of requests pending
 */
static inline int mdb5_dma_check_req_pending(struct amd_mdb5_dma_kthread *kth)
{
	struct amd_mdb5_dma_channel *hchan;
	int pending = 0;

	hchan = container_of(kth, struct amd_mdb5_dma_channel, kth);

	spin_lock(&hchan->pend_lock);
	pending = !list_empty(&hchan->pend_result);
	spin_unlock(&hchan->pend_lock);

	return pending;
}

/**
 * amd_mdb5_dma_poll_thread_task()
 *
 * @args pointer to void type
 *
 * Kernel thread that keeps the track of status of
 * the pending reqs.
 *
 * @return '0' on successful completion
 */
static int amd_mdb5_dma_poll_thread_task(void *args)
{
	struct amd_mdb5_dma_io_completion *comp, *_comp;
	struct amd_mdb5_dma_channel *hchan;
	struct amd_mdb5_dma_kthread *kth;

	kth   = (struct amd_mdb5_dma_kthread *) args;
	hchan = kth->hchan;

	disallow_signal(SIGPIPE);

	mdb5_dma_dbg(hchan->dev, "poll thread function for %s",
		     hchan->cdev_chan.cdev_name);

	while (!kthread_should_stop()) {
		spin_lock(&kth->lock);
		if (!mdb5_dma_check_req_pending(kth)) {
			spin_unlock(&kth->lock);
			mdb5_dma_kthread_resched(kth);
			spin_lock(&kth->lock);
		}
		kth->schedule = 0;

		list_for_each_entry_safe(comp, _comp,
					 &hchan->pend_result, link) {
			mdb5_dma_comp_update(comp);
		}

		spin_unlock(&kth->lock);
		schedule();
	}
	return 0;
}

/**
 * amd_mdb5_dma_chan_poll_thread_create()
 *
 * @chan pointer to channel structure
 *
 * Creates a poll thread on per channel basis.
 *
 * @return '0' on success, negative on error
 */
int amd_mdb5_dma_chan_poll_thread_create(struct amd_mdb5_dma_cdev_chan *chan)
{
	struct amd_mdb5_dma_channel *hchan;
	struct amd_mdb5_dma_kthread *kth;
	unsigned int num_cpus = 0;
	int ret = -EINVAL;

	hchan = amd_mdb5_dma_cdev2channel(chan);
	kth = &hchan->kth;

	mdb5_dma_dbg(hchan->dev, "starting poll thread creation");

	if (kth->task) {
		mdb5_dma_err(hchan->dev, "poll thread already initialized");
		return ret;
	}

	memset(kth, 0, sizeof(struct amd_mdb5_dma_kthread));

	spin_lock_init(&kth->lock);
	init_waitqueue_head(&kth->poll_wq);
	kth->timeout = 0;
	num_cpus = num_online_cpus();

	if (num_cpus)
		kth->cpu = hchan->index % num_cpus;

	snprintf(kth->kthread_name, sizeof(kth->kthread_name),
		 AMD_MDB5_DMA_THREAD_NAME_FMT,
		 amd_mdb5_dma_dir2char(hchan->dir), hchan->index);

	kth->task = kthread_create_on_node(amd_mdb5_dma_poll_thread_task,
					   (void *) kth,
					   cpu_to_node(kth->cpu),
					   "%s", kth->kthread_name);
	if (IS_ERR(kth->task)) {
		mdb5_dma_err(hchan->dev, "kthread %s, create failed: 0x%lx",
			     kth->kthread_name,
			     (unsigned long) IS_ERR(kth->task));
		kth->task = NULL;
		ret = -EFAULT;
		goto end_kthread;
	}

	kth->hchan = hchan;
	kthread_bind(kth->task, kth->cpu);

	mdb5_dma_dbg(hchan->dev, "poll thread created for %s", chan->cdev_name);

	wake_up_process(kth->task);
	kth->running = 1;

	return 0;

end_kthread:
	return ret;
}

/**
 * amd_mdb5_dma_chan_poll_thread_destroy()
 *
 * @chan pointer to channel structure
 *
 * Destroy the poll thread on per channel basis.
 *
 * @return void
 */
void amd_mdb5_dma_chan_poll_thread_destroy(struct amd_mdb5_dma_cdev_chan *chan)
{
	struct amd_mdb5_dma_channel *hchan;
	struct amd_mdb5_dma_kthread *kth;
	int ret;

	hchan = amd_mdb5_dma_cdev2channel(chan);
	kth   = &hchan->kth;

	if (!kth->running || !kth->task)
		return;

	kth->schedule = 1;
	ret = kthread_stop(kth->task);
	if (ret < 0) {
		mdb5_dma_err(hchan->dev, "ERROR: kthread:%s err:%d\n",
			     kth->kthread_name, ret);
		return;
	}

	kth->task = NULL;
}

/**
 * amd_mdb5_dma_cdev_chan_create()
 *
 * @chan pointer to char device channel
 * @ctrl pointer to char control node
 * @hchan pointer to channel structure
 *
 * Create and initialize further the per-channel device nodes
 * and kernel poll threads.
 *
 * Format of the device nodes is /dev/mdb5YY_readXX
 *
 * @return '0' on success, negative value on error
 */
int amd_mdb5_dma_cdev_chan_create(struct amd_mdb5_dma_cdev_chan *chan,
				  struct amd_mdb5_dma_cdev_ctrl *ctrl,
				  struct amd_mdb5_dma_channel *hchan)
{
	u16 minor = AMD_MDB5_DMA_NODE_CHAN_MINOR;
	int ret = -EINVAL;
	dev_t dev;
	int len;

	if (!chan || !hchan || !ctrl)
		return ret;

	mdb5_dma_dbg(hchan->dev, "create channel nodes");
	if (!chan->major) {
		ret = alloc_chrdev_region(&dev, 0, minor, AMD_MDB5_DMA_NODE_NAME);
		if (ret) {
			mdb5_dma_err(hchan->dev, "char dev allocation failed");
			goto end_chan_create;
		}
		chan->major = MAJOR(dev);
	}

	chan->cdev.owner = THIS_MODULE;

	len = snprintf(chan->cdev_name, sizeof(chan->cdev_name),
		       AMD_MDB5_DMA_NODE_CHAN_NAME_FMT,
		       amd_mdb5_dma_dir2char(hchan->dir), hchan->index);

	ret = kobject_set_name(&chan->cdev.kobj, chan->cdev_name);
	if (ret) {
		mdb5_dma_err(hchan->dev, "object name set failed");
		goto unreg_cdev;
	}
	chan->name_hash = full_name_hash((const void *) AMD_MDB5_DMA_SALT,
					 chan->cdev_name, len);

	cdev_init(&chan->cdev, &mdb5_dma_chan_fops);

	chan->cdevno = MKDEV(chan->major, 0);

	ret = cdev_add(&chan->cdev, chan->cdevno, minor);
	if (ret < 0)
		goto unreg_cdev;

	chan->chan_open = 0;
	mdb5_dma_dbg(hchan->dev, "channel cdev is initialized: %s", chan->cdev_name);
	if (g_mdb5_dma_class) {
		chan->sys_dev = device_create(g_mdb5_dma_class, hchan->dev,
					      chan->cdevno, NULL,
					      "%s", chan->cdev_name);
		if (IS_ERR(chan->sys_dev)) {
			mdb5_dma_err(hchan->dev, "device create failed");
			goto del_cdev;
		}
	}

	/* Get the reference of the control device node */
	hchan->cdev_ctrl = amd_mdb5_dma_cdev_ctrl_get(ctrl);
	if (!hchan->cdev_ctrl)
		goto del_cdev;

	if (hchan->dir == AMD_MDB5_DMA_CHAN_WR) {
		ctrl->wchan[hchan->index % AMD_MDB5_DMA_MAX_WR_CHAN] = hchan;
	} else {
		ctrl->rchan[hchan->index % AMD_MDB5_DMA_MAX_RD_CHAN] = hchan;
	}

	mdb5_dma_dbg(hchan->dev, "creating poll thread for channel: %s", chan->cdev_name);

	ret = amd_mdb5_dma_chan_poll_thread_create(chan);
	if (ret < 0) {
		mdb5_dma_err(hchan->dev, "poll thread create failed");
		goto del_cdev;
	}

	spin_lock_init(&hchan->pend_lock);
	INIT_LIST_HEAD(&hchan->pend_result);
	mutex_init(&hchan->mutex);

	mdb5_dma_dbg(hchan->dev, "channel %s creation successful", chan->cdev_name);

	return 0;

del_cdev:
	if (hchan->cdev_ctrl)
		amd_mdb5_dma_cdev_ctrl_put(ctrl);
	if (chan->sys_dev)
		device_destroy(g_mdb5_dma_class, chan->cdevno);
	cdev_del(&chan->cdev);
unreg_cdev:
	unregister_chrdev_region(chan->cdevno, minor);
end_chan_create:
	return ret;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_chan_create);

/**
 * amd_mdb5_dma_cdev_chan_destroy()
 *
 * @chan pointer to char channel structure
 *
 * Destroy the following entities related to a channel device node:
 * - device node
 * - poll thread
 * - remove the file_operations associated with this device.
 *
 * @return void
 */
void amd_mdb5_dma_cdev_chan_destroy(struct amd_mdb5_dma_cdev_chan *chan)
{
	struct amd_mdb5_dma_channel *hchan = NULL;

	if (!chan) {
		mdb5_dma_err(NULL, "device is null");
		goto end_destroy;
	}

	hchan = amd_mdb5_dma_cdev2channel(chan);

	if (!g_mdb5_dma_class) {
		mdb5_dma_err(hchan->dev, "sys device class is null");
		goto end_destroy;
	}

	if (!chan->sys_dev) {
		mdb5_dma_err(hchan->dev, "sys device is null");
		goto end_destroy;
	}


	amd_mdb5_dma_chan_poll_thread_destroy(chan);
	amd_mdb5_dma_cdev_ctrl_put(hchan->cdev_ctrl);
	device_destroy(g_mdb5_dma_class, chan->cdevno);
	cdev_del(&chan->cdev);
	unregister_chrdev_region(chan->cdevno, AMD_MDB5_DMA_NODE_CHAN_MINOR);
	chan->chan_open = 0;

end_destroy:
	return;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_chan_destroy);


/**
 * mdb5_dma_cdev_ctrl_open()
 *
 * @inode inode pointer
 * @filep file pointer
 *
 * Open call for the control node to configure all the
 * channels
 *
 * @return '0' on success, negative value on error
 */
static int mdb5_dma_cdev_ctrl_open(struct inode *inode, struct file *filep)
{
	struct amd_mdb5_dma_cdev_ctrl *ctrl;

	ctrl = container_of(inode->i_cdev, struct amd_mdb5_dma_cdev_ctrl, cdev);
	if (!ctrl)
		return -EINVAL;

	if (ctrl->ctrl_open)
		return -EBUSY;

	ctrl->ctrl_open = 1;
	filep->private_data = ctrl;
	return 0;
}

/**
 * mdb5_dma_cdev_ctrl_release()
 *
 * @inode inode pointer
 * @filep file pointer
 *
 * release call for the control node
 *
 * @return '0' on success, negative value on error
 */
static int mdb5_dma_cdev_ctrl_release(struct inode *inode, struct file *filep)
{
	struct amd_mdb5_dma_cdev_ctrl *ctrl;

	ctrl = container_of(inode->i_cdev, struct amd_mdb5_dma_cdev_ctrl, cdev);
	if (!ctrl)
		return -EINVAL;

	ctrl->ctrl_open = 0;
	filep->private_data = NULL;
	return 0;
}

/**
 * mdb5_dma_ctrl_match_channel_byname()
 *
 * @ctrl pointer to char control node
 * @chan_name channel name
 *
 * Query and return the per-channel struct based on the
 * name of the device node.
 *
 * @return pointer to the channel struct, NULL otherwise
 */
static struct amd_mdb5_dma_channel *
mdb5_dma_ctrl_match_channel_byname(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				   const char *chan_name)
{
	struct amd_mdb5_dma_channel *chan;
	unsigned int name_hash = 0;
	const char *p = NULL;
	int i;

	if (!chan_name || *chan_name == '\0')
		return NULL;

	if ((p = strstr(chan_name, "mdb5")) == NULL)
		return NULL;


	mdb5_dma_dbg(ctrl->dev, "chan name:%s, name:%s", chan_name, p);
	name_hash = full_name_hash((const void *) AMD_MDB5_DMA_SALT,
					p, strlen(p));

	for (i = 0; i < AMD_MDB5_DMA_MAX_RD_CHAN; i++) {
		chan = ctrl->rchan[i];
		mdb5_dma_dbg(ctrl->dev, "ph:0x%08X, ch:0x%08X", name_hash,
			     chan ? chan->cdev_chan.name_hash : 0x0);
		if (chan &&
		    chan->cdev_chan.name_hash == name_hash) {
			return chan;
		}
	}

	for (i = 0; i < AMD_MDB5_DMA_MAX_WR_CHAN; i++) {
		chan = ctrl->wchan[i];
		mdb5_dma_dbg(ctrl->dev, "ph:0x%08X, ch:0x%08X", name_hash,
			     chan ? chan->cdev_chan.name_hash : 0x0);
		if (chan &&
		    chan->cdev_chan.name_hash == name_hash) {
			return chan;
		}
	}
	return NULL;
}

/**
 * mdb5_dma_ctrl_cmd_mode()
 *
 * @ctrl pointer to char control structure
 * @mode pointer to the mode structure
 * @set to set or retrieve the mode of the requested channel
 *
 * * Changes the mode of the channel based on
 * the user-space inputs.
 *
 * @return '0' on success, negative value on error
 */
static int mdb5_dma_ctrl_cmd_mode(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				  struct ctrl_mode *mode,
				  bool set)
{
	struct amd_mdb5_dma_channel *chan;
	int ret = -EINVAL;

	mdb5_dma_dbg(ctrl->dev, "name:%s, mode:0x%x", mode->name, mode->mode);

	chan = mdb5_dma_ctrl_match_channel_byname(ctrl, mode->name);
	if (!chan) {
		mdb5_dma_err(ctrl->dev, "could not find channel: %s",
			     mode->name);
		goto end_mode_proc;
	}

	if (set) {
		switch (mode->mode) {
		case AMD_MDB5_DMA_MODE_SG:
		case AMD_MDB5_DMA_MODE_SIMPLE:
			chan->mode = mode->mode;
			chan->aperture_sz = 0;
			break;
		default:
			goto end_mode_proc;
		}
	} else {
		mode->mode = chan->mode;
	}

	return 0;
end_mode_proc:
	return ret;
}

/**
 * mdb5_dma_ctrl_cmd_aperture()
 *
 * @ctrl pointer to char control structure
 * @mode pointer to the aperture structure
 * @set to set or retrieve the aperture of the requested channel
 *
 * Changes the mode of the channel based on
 * the user-space inputs.
 *
 * @return '0' on success, negative value on error
 */
static int mdb5_dma_ctrl_cmd_aperture(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				      struct ctrl_aperture *aperture,
				      bool set)
{
	struct amd_mdb5_dma_channel *chan;
	int ret = -EINVAL;

	mdb5_dma_dbg(ctrl->dev, "name:%s, aperture_sz:0x%x",
		     aperture->name, aperture->aperture);

	if (!aperture)
		goto end_aperture;

	chan = mdb5_dma_ctrl_match_channel_byname(ctrl, aperture->name);
	if (!chan) {
		mdb5_dma_err(ctrl->dev, "could not find channel: %s",
			     aperture->name);
		goto end_aperture;
	}

	if (chan->mode != AMD_MDB5_DMA_MODE_SG)
		goto end_aperture;

	if (set) {
		if (aperture->aperture >= AMD_MAX_BURST_SZ)
			goto end_aperture;

		chan->aperture_sz = aperture->aperture;
	} else {
		aperture->aperture = chan->aperture_sz;
	}

	return 0;
end_aperture:
	return ret;
}

/**
 * mdb5_dma_ctrl_cmd_stats()
 *
 * @ctrl pointer to char control structure
 * @stats pointer to stats
 *
 * Retrieve the stats related to channel
 *
 * @return '0' on success, negative value on error
 */
static int mdb5_dma_ctrl_cmd_stats(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				   struct ctrl_stats *stats)
{
	struct amd_mdb5_dma_channel *chan;
	void __user *buf;
	size_t sz;
	struct mdb5_dma_channel_stats_user {
		s64	req_rcvd;
		s64	req_success;
		s64	req_failed;
		s64	intr_rcvd;
		s64	read_data_processed;
		s64	write_data_processed;
		s64	total_data_processed;
	} ch_stats;


	chan = mdb5_dma_ctrl_match_channel_byname(ctrl, stats->name);
	if (!chan)
		return -EINVAL;

	memset(&ch_stats, 0, sizeof(ch_stats));

	ch_stats.req_rcvd	      = atomic64_read(&chan->stats.req_rcvd);
	ch_stats.req_success	      = atomic64_read(&chan->stats.req_success);
	ch_stats.req_failed	      = atomic64_read(&chan->stats.req_failed);
	ch_stats.intr_rcvd	      = atomic64_read(&chan->stats.intr_rcvd);
	ch_stats.read_data_processed  = atomic64_read(&chan->stats.read_data_processed);
	ch_stats.write_data_processed = atomic64_read(&chan->stats.write_data_processed);
	ch_stats.total_data_processed = atomic64_read(&chan->stats.total_data_processed);

	buf = (void __user *)(u64) stats->stat_addr;
	sz  = sizeof(struct mdb5_dma_channel_stats_user);

	if (copy_to_user(buf, &ch_stats, sz))
		return -EFAULT;

	return 0;
}

/**
 * mdb5_dma_cdev_ctrl_ioctl()
 *
 * @filep pointer to the file
 * @cmd command
 * @args args provided
 *
 * Configure / retrived the information via control node related
 * to the channels such as aperture / mode / stats
 *
 * @return '0' on success, negative value on error
 */
static long mdb5_dma_cdev_ctrl_ioctl(struct file *filep,
				     unsigned int cmd, unsigned long args)
{
	void __user *buf = (void __user *) args;
	struct amd_mdb5_dma_cdev_ctrl *ctrl;
	struct ctrl_aperture caperture;
	struct ctrl_stats cstats;
	struct ctrl_mode cmode;
	bool set_cmd = false;
	int ret = -1;
	size_t sz;

	ctrl = (struct amd_mdb5_dma_cdev_ctrl *) filep->private_data;

	if (!buf)
		goto end_ioctl;

	mdb5_dma_dbg(ctrl->dev, "cmd: 0x%x, args: 0x%lx", cmd, args);
	switch (cmd) {
	case AMD_MDB5_DMA_CTRL_CMD_MODE:
		set_cmd = true;
		fallthrough;
	case AMD_MDB5_DMA_CTRL_CMD_GET_MODE:
		sz = sizeof(struct ctrl_mode);
		memset(&cmode, 0, sz);

		if (copy_from_user(&cmode, buf, sz)) {
			ret = -EFAULT;
			goto end_ioctl;
		}

		ret = mdb5_dma_ctrl_cmd_mode(ctrl, &cmode, set_cmd);
		if (ret)
			goto end_ioctl;

		if (!set_cmd && copy_to_user(buf, &cmode, sz)) {
			ret = -EFAULT;
			goto end_ioctl;
		}
		ret = 0;
		break;
	case AMD_MDB5_DMA_CTRL_CMD_SET_APERTURE_SZ:
		set_cmd = true;
		fallthrough;
	case AMD_MDB5_DMA_CTRL_CMD_GET_APERTURE_SZ:
		sz = sizeof(struct ctrl_aperture);
		if (copy_from_user(&caperture, buf, sz)) {
			ret = -EFAULT;
			goto end_ioctl;
		}

		ret = mdb5_dma_ctrl_cmd_aperture(ctrl, &caperture, set_cmd);
		if (ret)
			goto end_ioctl;

		if (!set_cmd && copy_to_user(buf, &caperture, sz)) {
			ret = -EFAULT;
			goto end_ioctl;
		}
		ret = 0;
		break;
	case AMD_MDB5_DMA_CTRL_CMD_STATS:
		sz = sizeof(struct ctrl_stats);
		if (copy_from_user(&cstats, buf, sz)) {
			ret = -EFAULT;
			goto end_ioctl;
		}

		ret = mdb5_dma_ctrl_cmd_stats(ctrl, &cstats);
		if (ret)
			goto end_ioctl;
		break;
	default:
		ret = -EINVAL;
		goto end_ioctl;
	}

	ret = 0;
end_ioctl:
	return ret;
}

/**
 * mdb5_dma_fops_ctrl
 *
 * File operations related to the control path operations.
 */
static struct file_operations mdb5_dma_fops_ctrl = {
	.owner			= THIS_MODULE,
	.open			= mdb5_dma_cdev_ctrl_open,
	.release		= mdb5_dma_cdev_ctrl_release,
	.unlocked_ioctl		= mdb5_dma_cdev_ctrl_ioctl,
};

/**
 * amd_mdb5_dma_cdev_ctrl_create()
 *
 * @ctrl pointer to char control node
 * @device pointer to device
 *
 * Creates the device node and intializes the file operations
 * for control path operations.
 *
 * @return '0' on success, negative value on error
 */
int amd_mdb5_dma_cdev_ctrl_create(struct amd_mdb5_dma_cdev_ctrl *ctrl,
				  struct device *device)
{
	u16 minor = AMD_MDB5_DMA_NODE_CTRL_MINOR;
	int ret = -1;
	dev_t dev;
	u16 i;

	mdb5_dma_dbg(NULL, "cdev ctrl init");
	if (!ctrl) {
		ret = -EINVAL;
		goto end_ctrl;
	}

	if (!ctrl->major) {
		ret = alloc_chrdev_region(&dev, 0, minor, AMD_MDB5_DMA_NODE_NAME);
		if (ret)
			goto end_ctrl;
		ctrl->major = MAJOR(dev);
	}

	ctrl->cdev.owner = THIS_MODULE;
	ctrl->ctrl_open  = 0;

	for (i = 0; i < AMD_MDB5_DMA_MAX_RD_CHAN; i++)
		ctrl->rchan[i] = NULL;

	for (i = 0; i < AMD_MDB5_DMA_MAX_WR_CHAN; i++)
		ctrl->wchan[i] = NULL;

	ret = kobject_set_name(&ctrl->cdev.kobj, AMD_MDB5_DMA_NODE_CTRL_NAME_FMT);
	if (ret)
		goto unreg_cdev;

	cdev_init(&ctrl->cdev, &mdb5_dma_fops_ctrl);

	ctrl->cdevno = MKDEV(ctrl->major, 0);

	ret = cdev_add(&ctrl->cdev, ctrl->cdevno, minor);
	if (ret < 0)
		goto unreg_cdev;

	if (g_mdb5_dma_class) {
		ctrl->sys_dev = device_create(g_mdb5_dma_class, device,
					      ctrl->cdevno, NULL,
				    	      AMD_MDB5_DMA_NODE_CTRL_NAME_FMT);
		if (IS_ERR(ctrl->sys_dev))
			goto del_cdev;
		ctrl->dev = device;
	}

	mdb5_dma_dbg(ctrl->dev, "cdev ctrl create successful");
	return 0;

del_cdev:
	cdev_del(&ctrl->cdev);
unreg_cdev:
	unregister_chrdev_region(ctrl->cdevno, minor);
end_ctrl:
	return ret;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_ctrl_create);

/**
 * amd_mdb5_dma_cdev_ctrl_destroy()
 *
 * @ctrl pointer to char control node
 *
 * Deletes the device node and de-intializes the file operations
 * for control path operations.
 *
 * @return '0' on success, negative value on error
 */
int amd_mdb5_dma_cdev_ctrl_destroy(struct amd_mdb5_dma_cdev_ctrl *ctrl)
{
	int ret = -EINVAL;

	if (!ctrl) {
		mdb5_dma_err(ctrl->dev, "device is null");
		goto end_destroy;
	}

	if (!g_mdb5_dma_class) {
		mdb5_dma_err(ctrl->dev, "sys device class is null");
		goto end_destroy;
	}

	if (!ctrl->sys_dev) {
		mdb5_dma_err(ctrl->dev, "sys device is null");
		goto end_destroy;
	}

	device_destroy(g_mdb5_dma_class, ctrl->cdevno);
	cdev_del(&ctrl->cdev);
	unregister_chrdev_region(ctrl->cdevno, AMD_MDB5_DMA_NODE_CTRL_MINOR);
	ret = 0;

end_destroy:
	return ret;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_ctrl_destroy);


/**
 * amd_mdb5_dma_cdev_ctrl_get()
 *
 * Provide count based reference of the struct
 *
 * @return reference to the char control node
 */
struct amd_mdb5_dma_cdev_ctrl *
amd_mdb5_dma_cdev_ctrl_get(struct amd_mdb5_dma_cdev_ctrl *ctrl)
{
	if (!ctrl)
		return NULL;

	++ctrl->nref;
	return ctrl;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_ctrl_get);

/**
 * amd_mdb5_dma_cdev_ctrl_put()
 *
 * Free up the reference based on the count of the struct
 *
 *
 * @return current reference
 */
int amd_mdb5_dma_cdev_ctrl_put(struct amd_mdb5_dma_cdev_ctrl *ctrl)
{
	int ret;

	if (!ctrl)
		return 0;

	if (ctrl->nref > 0) {
		ret = --ctrl->nref;
		if (!ret)
			amd_mdb5_dma_cdev_ctrl_destroy(ctrl);
	}
	return ret;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_ctrl_put);

ssize_t amd_mdb5_dma_interleaved_read_write(struct amd_mdb5_dma_cdev_chan *chan,
					    struct dma_interleaved_template *xt,
					    bool read_write)
{
	return mdb5_dma_interleaved_read_write(chan, xt, read_write);
}
EXPORT_SYMBOL(amd_mdb5_dma_interleaved_read_write);

/**
 * amd_mdb5_dma_cdev_init()
 *
 * Create the base class struct for the
 * device nodes need to be created for the channels.
 *
 * Create the kmem_cache area for the frequently used
 * memory allocations.
 *
 * @return '0' on success, negative value on error
 */
int amd_mdb5_dma_cdev_init(void)
{
	int ret;

	if (g_mdb5_dma_class)
		return -EINVAL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0))
        g_mdb5_dma_class = class_create(AMD_MDB5_DMA_NODE_NAME);
#else
        g_mdb5_dma_class = class_create(THIS_MODULE, AMD_MDB5_DMA_NODE_NAME);
#endif
        if (IS_ERR(g_mdb5_dma_class)) {
                mdb5_dma_err(NULL,
			     AMD_MDB5_DMA_NODE_NAME ": failed to create class");
                return -EINVAL;
        }

	mdb5_dma_cdev_cache = kmem_cache_create("mdb5_dma_cache",
						sizeof(struct amd_mdb5_dma_aio_request),
						0, SLAB_HWCACHE_ALIGN, NULL);
	if (!mdb5_dma_cdev_cache) {
		ret = -ENOMEM;
                mdb5_dma_err(NULL,
			     AMD_MDB5_DMA_NODE_NAME ": failed to create cache");
		goto init_err;
	}

	return 0;
init_err:
	if (g_mdb5_dma_class)
		class_destroy(g_mdb5_dma_class);

	return ret;
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_init);

/**
 * amd_mdb5_dma_cdev_destroy()
 *
 * Delete the kmem_cache and class struct.
 *
 * @return void
 */
void amd_mdb5_dma_cdev_destroy(void)
{
	if (mdb5_dma_cdev_cache)
		kmem_cache_destroy(mdb5_dma_cdev_cache);

	if (g_mdb5_dma_class)
		class_destroy(g_mdb5_dma_class);
}
EXPORT_SYMBOL(amd_mdb5_dma_cdev_destroy);
