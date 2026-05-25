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

#include "mdb5-dmaclient-mod.h"
#include "mdb5-dmaclient-cdev.h"

#define AMD_MDB5_DMA_CHANNEL_NAME_FMT		"dma%uchan%u"
#define AMD_MDB5_DMA_DEVICE_NAME		"0000:00:00.0"
#define AMD_MDB5_DMA_DEV_ID			0
#define AMD_MDB5_DMA_DEV_ID_NONE		0xffff

static char *device_dbdf = NULL;
module_param(device_dbdf, charp, 0000);
MODULE_PARM_DESC(device_dbdf, "Device Domain:Bus:Device.Function <DOMA:BB:DD.F>");

static u16 dma_dev_id = AMD_MDB5_DMA_DEV_ID_NONE;
module_param(dma_dev_id, short, 0000);
MODULE_PARM_DESC(dma_dev_id, "DMA Chan Device ID dma<ID>chan<NUM>");

static struct amd_mdb5_dma_client mdb5_dma_client;

/**
 * amd_mdb5_dma_filter_comp() -  filter function to match the device & channels
 *
 * @chan : dma_chan structure
 * @filter : filter string
 *
 * Callback function to help selecting the correct DMA channel
 * 
 * @note    This function will be called as a callback
 *
 * @return: 'true' or 'false' if channels match filter.
 *
 */
static bool amd_mdb5_dma_filter_comp(struct dma_chan *chan, void *filter)
{
	if (strcmp(dev_name(chan->device->dev), device_dbdf) ||
	    strcmp(dma_chan_name(chan), filter))
		return false;

	return true;
}

/**
 * amd_mdb5_dma_probe_channels() - probe the channels enabled by controller
 *
 * @mdb5_dma_client : structure to the data related to all the channels
 * @dev_id : device id to be used for dma filter string
 * @max_rchan : maximum number of read channels supported
 * @max_wchan : maximum number of write channels 
 *
 * Probes the DMA channels available
 * 
 * @note    none
 *
 * @return:  returns '0' on success, 'negative error' on failure
 *
 */
int amd_mdb5_dma_probe_channels(struct amd_mdb5_dma_client *mdb5_dma_client,
				u16 dev_id, u16 max_rchan, u16 max_wchan)
{
	struct amd_mdb5_dma_channel *hc = NULL, *_hc;
	struct dma_slave_caps caps;
	struct dma_chan *chan;
	dma_cap_mask_t mask;
	char filter[24];
	int ret = -1;
	u16 i;

	mdb5_dma_dbg(NULL, "probing channels");
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);

	for (i = 0; i < (max_rchan + max_wchan); i++) {
		snprintf(filter, sizeof(filter), AMD_MDB5_DMA_CHANNEL_NAME_FMT,
			 dev_id, i);

		chan = dma_request_channel(mask, amd_mdb5_dma_filter_comp, filter);
		if (!chan)
			break;

		hc = kzalloc(sizeof(struct amd_mdb5_dma_channel), GFP_KERNEL);
		if (!hc) {
			dma_release_channel(chan);
			goto probe_done;
		}

		memset(&caps, 0, sizeof(caps));
		caps.directions = DMA_TRANS_NONE;
		if (dma_get_slave_caps(chan, &caps) < 0) {
			dma_release_channel(chan);
			goto probe_done;
		}

		strncpy(hc->chan_name, filter, strlen(filter));
		hc->dchan = chan;
		hc->dev   = chan->device->dev;
		hc->mode  = AMD_MDB5_DMA_MODE_SG;

		if (!mdb5_dma_client->dev)
			mdb5_dma_client->dev = hc->dev;

		switch (caps.directions) {
		case BIT(DMA_DEV_TO_MEM):
			hc->dir   = AMD_MDB5_DMA_CHAN_WR;
			hc->index = mdb5_dma_client->wr_chan_available++;
			break;
		case BIT(DMA_MEM_TO_DEV):
			hc->dir   = AMD_MDB5_DMA_CHAN_RD;
			hc->index = mdb5_dma_client->rd_chan_available++;
			break;
		default:
			goto probe_done;
		}

		mutex_lock(&mdb5_dma_client->lock);
		list_add_tail(&hc->link, &mdb5_dma_client->channels);
		mutex_unlock(&mdb5_dma_client->lock);
		hc = NULL;
	}

	mdb5_dma_dbg(NULL, "channels found: W: %u, R: %u",
		     mdb5_dma_client->wr_chan_available,
		     mdb5_dma_client->rd_chan_available);

	if (list_empty(&mdb5_dma_client->channels)) {
		mdb5_dma_err(NULL, "no read/write channels found for <%s> "
			     "and dma_dev_id=%u\n", device_dbdf, dma_dev_id);
		goto probe_done;
	}

	ret = 0;

probe_done:
	if (ret < 0) {
		if (hc)
			kfree(hc);

		list_for_each_entry_safe(hc, _hc,
					 &mdb5_dma_client->channels, link) {
			mutex_lock(&mdb5_dma_client->lock);
			amd_mdb5_dma_free_channel(hc);
			mutex_unlock(&mdb5_dma_client->lock);
			kfree(hc);
		}
	}
	return ret;
}

/**
 * amd_mdb5_dma_free_channel() - free all the DMA channels and corresponding
 *                          structures.
 *
 * @hchan : pointer to channel referred to.
 *
 * Free up the channel related data
 * 
 * @return: void
 *
 */
void amd_mdb5_dma_free_channel(struct amd_mdb5_dma_channel *hchan)
{
	if (!hchan)
		return;

	if (hchan->dchan) {
		dmaengine_terminate_all(hchan->dchan);
		dma_release_channel(hchan->dchan);
	}
	list_del(&hchan->link);
}


/**
 * amd_mdb5_dma_client_init() - initialize the members of amd_mdb5_dma_client
 *
 * @hcl : pointer to client referred to
 * 
 * Initializes the client and channel related data
 *
 * @return: void
 *
 */
static void amd_mdb5_dma_client_init(struct amd_mdb5_dma_client *hcl)
{
	if (!hcl)
		return;

	memset(hcl, 0, sizeof(struct amd_mdb5_dma_client));

	mutex_init(&hcl->lock);

	INIT_LIST_HEAD(&hcl->channels);

	hcl->rd_max_chan = AMD_MDB5_DMA_MAX_RD_CHAN;
	hcl->wr_max_chan = AMD_MDB5_DMA_MAX_WR_CHAN;
}

/**
 * amd_mdb5_dma_client_mod_init() - module init function
 *
 * Module init function
 *
 * @return: returns '0' on success and negative value on failure
 *
 */
static int __init amd_mdb5_dma_client_mod_init(void)
{
	struct amd_mdb5_dma_channel *hc, *_hc;
	struct list_head flist;
	int ret = -1;

	INIT_LIST_HEAD(&flist);
	amd_mdb5_dma_client_init(&mdb5_dma_client);

	mdb5_dma_dbg(NULL, "mod init");
	if (amd_mdb5_dma_cdev_init() < 0)
		goto init_err;

	if (!device_dbdf)
		device_dbdf = AMD_MDB5_DMA_DEVICE_NAME;
	if (dma_dev_id == AMD_MDB5_DMA_DEV_ID_NONE)
		dma_dev_id = AMD_MDB5_DMA_DEV_ID;

	mdb5_dma_dbg(NULL, "Device Dom:B:D.F=<%s> dma_dev_id=%u",
		     device_dbdf, dma_dev_id);

	ret = amd_mdb5_dma_probe_channels(&mdb5_dma_client, dma_dev_id,
				     mdb5_dma_client.rd_max_chan,
				     mdb5_dma_client.wr_max_chan);
	if (ret)
		goto init_err;

	ret = amd_mdb5_dma_cdev_ctrl_create(&mdb5_dma_client.cdev_ctrl,
				       mdb5_dma_client.dev);
	if (ret)
		goto init_err;


	/* Create the interfaces for all the available channels */
	list_for_each_entry_safe(hc, _hc, &mdb5_dma_client.channels, link) {
		mutex_lock(&mdb5_dma_client.lock);
		ret = amd_mdb5_dma_cdev_chan_create(&hc->cdev_chan,
					       &mdb5_dma_client.cdev_ctrl,
					       hc);
		if (ret < 0) {
			mdb5_dma_err(NULL, "channel cdev failed\n");
			list_del(&hc->link);
			list_add_tail(&hc->link, &flist);
			mutex_unlock(&mdb5_dma_client.lock);
			continue;
		}
		mutex_unlock(&mdb5_dma_client.lock);
	}
	mdb5_dma_dbg(NULL, "mod init success");
	ret = 0;

init_err:
	if (ret < 0) {
		list_for_each_entry_safe(hc, _hc,
					 &mdb5_dma_client.channels, link) {
			amd_mdb5_dma_cdev_chan_destroy(&hc->cdev_chan);
			mutex_lock(&mdb5_dma_client.lock);
			amd_mdb5_dma_free_channel(hc);
			mutex_unlock(&mdb5_dma_client.lock);
			kfree(hc);
		}

		amd_mdb5_dma_cdev_ctrl_put(&mdb5_dma_client.cdev_ctrl);
		amd_mdb5_dma_cdev_destroy();
	} else if (!list_empty(&flist)) {
		list_for_each_entry_safe(hc, _hc,
					 &flist, link) {
			amd_mdb5_dma_free_channel(hc);
			kfree(hc);
		}
	}
	return ret;
}

/*
 * amd_mdb5_dma_client_mod_exit() - clean up module function
 *
 * module exit function
 *
 * @return:  void
 *
 */
static void __exit amd_mdb5_dma_client_mod_exit(void)
{
	struct amd_mdb5_dma_channel *hc, *_hc;

	list_for_each_entry_safe(hc, _hc, &mdb5_dma_client.channels, link) {
		amd_mdb5_dma_cdev_chan_destroy(&hc->cdev_chan);
		mutex_lock(&mdb5_dma_client.lock);
		amd_mdb5_dma_free_channel(hc);
		mutex_unlock(&mdb5_dma_client.lock);
		kfree(hc);
	}

	amd_mdb5_dma_cdev_ctrl_put(&mdb5_dma_client.cdev_ctrl);
	amd_mdb5_dma_cdev_destroy();
}

module_init(amd_mdb5_dma_client_mod_init);
module_exit(amd_mdb5_dma_client_mod_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("AMD Client Module");
