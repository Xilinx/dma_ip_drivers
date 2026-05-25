/*
 * This file is part of the MDB5 userspace application
 * to enable the user to execute the MDB5 functionality
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "dma_common_utils.h"

/**
 * This function reads a register offset of BAR0 using memory-mapped I/O.
 * It logs an error message if the read operation fails and logs the register
 * offset and data if the read operation is successful.
 *
 * @param   arg Pointer to a @c mdb5_dma_reg structure.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int mdb5_dma_reg_read(void *arg)
{
	struct mdb5_dma_reg *reg = (struct mdb5_dma_reg *)arg;
	int ret;

	ret = reg_read_mmap(reg);
	if (ret < 0) {
		mdb5_dma_err("Register read failed\n");
		goto error;
	}

	mdb5_dma_info("Register read: offset = 0x%x, data = 0x%x\n",
		      reg->offset, reg->data);

error:
	return ret;
}

/**
 * This function writes data to register offset of BAR0 using memory-mapped I/O.
 * It then reads back the data to verify the write operation. If the write
 * operation fails, it logs an error message. If successful, it logs the
 * register offset and data.
 *
 * @param   arg Pointer to a @c mdb5_dma_reg structure.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int mdb5_dma_reg_write(void *arg)
{
	struct mdb5_dma_reg *reg = (struct mdb5_dma_reg *)arg;
	int ret;

	ret = reg_write_mmap(reg);
	if (ret < 0) {
		mdb5_dma_err("Register write failed\n");
		goto error;
	}

	ret = reg_read_mmap(reg);
	if (ret < 0) {
		mdb5_dma_err("Register read back failed\n");
		goto error;
	}

	mdb5_dma_info("Register read back: offset = 0x%x, data = 0x%x\n",
		      reg->offset, reg->data);
error:
	return ret;
}

/**
 * @brief   Reads MDB5 read/write channel statistics.
 *
 * This function reads the statistics of MDB5 read/write channel.
 * It passes @c stat_addr physical address to kernel through ioctl operation
 * to retrieve the statistics. If any operation fails, it logs an error
 * message and returns a negative error code.
 *
 * @param   arg Pointer to a @c mdb5_dma_ioctl structure.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int mdb5_dma_stats_read(void *arg)
{
	struct mdb5_dma_ioctl *hioc = (struct mdb5_dma_ioctl *)arg;
	struct ctrl_stats *cstats = &hioc->cstats;
	struct mdb5_dma_channel_stats stats;
	int fd = 0, ret = 0;

	memset(&stats, 0, sizeof(struct mdb5_dma_channel_stats));
	cstats->stat_addr = (uint64_t)(void *)&stats;

	fd = open(hioc->h->ctrl_name, O_RDWR);
	if (fd < 0) {
		mdb5_dma_err("%s: Failed to open file '%s': %s\n", __func__,
			     hioc->h->ctrl_name, mdb5_dma_strerror(errno));
		ret = -errno;
		goto open_error;
	}

	/*TODO: Pass stat_addr from here via ioctl*/
	if (ioctl(fd, IOCTL_MDB5_STATS, cstats)) {
		mdb5_dma_err("%s: ioctl operation failed on fd %d: %s\n",
			     __func__, fd, mdb5_dma_strerror(errno));
		ret = -1;
		goto error;
	}

	mdb5_dma_info("Channel %s stats:\n"
		  "\tNumber of packets received = %lu\n"
		  "\tNumber of packets success = %lu\n"
		  "\tNumber of packets failed = %lu\n"
		  "\tNumber of interrupts received = %lu\n"
		  "\tRead data processed = %lu\n"
		  "\tWrite data processed = %lu\n"
		  "\tTotal data processed = %lu\n",
		  cstats->name, stats.req_rcvd, stats.req_success,
		  stats.req_failed, stats.intr_rcvd,
		  stats.read_data_processed,
		  stats.write_data_processed,
		  stats.total_data_processed);

error:
	close(fd);
open_error:
	return ret;
}

/**
 * @brief   Sets the transfer mode for an MDB5 channel.
 *
 * This function configures the transfer mode for a MDB5 read/write channel to
 * Scatter Gather or Simple DMA mode. If any operation fails, it logs an error
 * message and returns a negative error code.
 *
 * @param   arg Pointer to a @c mdb5_dma_ioctl structure.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int mdb5_dma_transfer_mode(void *arg)
{
	struct mdb5_dma_ioctl *hioc = (struct mdb5_dma_ioctl *)arg;
	struct mdb5_dma_common *h = hioc->h;
	struct ctrl_mode *cmode = &hioc->cmode;
	uint32_t cmd = 0;
	int fd = 0, ret = 0;

	fd = open(h->ctrl_name, O_RDWR);
	if (fd < 0) {
		mdb5_dma_err("%s: Failed to open '%s': %s\n", __func__,
			     hioc->h->ctrl_name, mdb5_dma_strerror(errno));
		ret = -errno;
		goto open_error;
	}

	cmd = h->set_mode ? IOCTL_MDB5_SET_TRANSFER_MODE
			  : IOCTL_MDB5_GET_TRANSFER_MODE;
	if (ioctl(fd, cmd, cmode)) {
		mdb5_dma_err("%s: ioctl operation failed on file descriptor "
			     "%d: %s\n", __func__, fd, mdb5_dma_strerror(errno));
		ret = -1;
		goto error;
	}

	mdb5_dma_info("%s mode configured on channel %s\n",
		      get_transfer_mode(cmode->mode), cmode->name);

error:
	close(fd);
open_error:
	return ret;
}

/**
 * @brief   Sets the aperture size for an MDB5 channel.
 *
 * This function configures the aperture size for a MDB5 read/write channel.
 * Assumes that transfer mode is set to Scatter Gather DMA before performing the
 * operation. If any operation fails, it logs an error message
 * and returns a negative error code.
 *
 * @param   arg Pointer to a @c mdb5_dma_ioctl structure.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int mdb5_dma_aperture_sz(void *arg)
{
	struct mdb5_dma_ioctl *hioc = (struct mdb5_dma_ioctl *)arg;
	struct mdb5_dma_common *h = hioc->h;
	struct ctrl_aperture *caperture = &hioc->caperture;
	uint32_t cmd = 0;
	int fd = 0, ret = 0;

	fd = open(h->ctrl_name, O_RDWR);
	if (fd < 0) {
		mdb5_dma_err("%s: Failed to open '%s': %s\n", __func__,
			     hioc->h->ctrl_name, mdb5_dma_strerror(errno));
		ret = -errno;
		goto open_error;
	}

	cmd = h->set_aperture ? IOCTL_MDB5_SET_APERTURE_SIZE
			      : IOCTL_MDB5_GET_APERTURE_SIZE;
	if (ioctl(fd, cmd, caperture)) {
		mdb5_dma_err("%s: ioctl operation failed on file descriptor %d: %s\n",
			     __func__, fd, mdb5_dma_strerror(errno));
		ret = -1;
		goto error;
	}

	mdb5_dma_info("Aperture size = %d is configured on channel %s\n",
		      caperture->aperture, caperture->name);

error:
	close(fd);
open_error:
	return ret;
}

