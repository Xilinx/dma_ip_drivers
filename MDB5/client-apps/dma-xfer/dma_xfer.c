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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>

#include "version.h"
#include "dma_common_utils.h"

static bool verbose = false;
static char input_file[FILE_NAME_SZ];
static char output_file[FILE_NAME_SZ];

static const struct option long_opts[] = {
	{"config", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{"verbose", no_argument, NULL, 'V'},
	{0, 0, 0, 0}
};

static void usage(const char *name)
{
	int i = 0;

	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "Usage: %s [OPTIONS]\n\n", name);

	fprintf(stdout,
		"  -%c (--%s) config file that has configuration for IO\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,	"  -%c (--%s) print usage help and exit\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,	"  -%c (--%s) to print version name\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,	"  -%c (--%s) enable verbose\n",
		long_opts[i].val, long_opts[i].name);
}

static int parse_options(int argc, char *argv[], char **cfg_fname,
			 struct mdb5_dma_common *h)
{
	int ret = 0, cmd_opt = 0;

	if (argc == 1) {
		usage(argv[0]);
		ret = -EINVAL;
		goto error;
	}

	while ((cmd_opt = getopt_long(argc, argv, "vVhxc:", long_opts,
				      NULL)) != -1) {
		switch (cmd_opt) {
		case 'c':
			/* config file name */
			*cfg_fname = strdup(optarg);
			break;
		case 'v':
			printf("%s version %s\n", PROGNAME, VERSION);
			printf("%s\n", COPYRIGHT);
			exit(EXIT_SUCCESS);
		case 'V':
			verbose = true;
			break;
		case 'h':
		case 0:
			/* long option */
		default:
			usage(argv[0]);
			exit(0);
		}
	}

	if (!cfg_fname || *cfg_fname == NULL) {
		mdb5_dma_err("Config file required.\n");
		usage(argv[0]);
		ret = -EINVAL;
		goto error;
	}

error:
	return ret;
}

/**
 * This function reads and parses the configuration file specified by
 * @c cfg_fname. It extracts various configuration parameters and populates
 * the @c mdb5_dma_common structure.
 *
 * @param cfg_fname The name of the configuration file to be parsed.
 *
 * @param h Pointer to a structure of type @c mdb5_dma_common where the parsed
 *          configuration will be stored.
 *
 * @return 0 on success, -EINVAL if an error occurs.
 */
static int parse_config_file(char *cfg_fname, struct mdb5_dma_common *h)
{
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	size_t numread, numblanks, linelen = 0;
	uint32_t linenum = 0;
	char *realbuf, *linebuf = NULL;
	char *config, *value;
	enum mdb5_dma_transfer_mode xfer_mode;
	FILE *fp;

	fp = fopen(cfg_fname, "r");
	if (!fp) {
		mdb5_dma_err("Failed to open Config File [%s]: %s\n",
			     cfg_fname, mdb5_dma_strerror(errno));
		return -errno;
	}

	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		linenum++;
		linebuf = strip_comments(linebuf);
		if (!linebuf)
			continue;
		realbuf = strip_blanks(linebuf, &numblanks);
		linelen -= numblanks;
		if (linelen == 0)
			continue;
		config = strtok(realbuf, "=");
		value = strtok(NULL, "=");
		if (!strncmp(config, "name", 4)) {
			copy_value(value, h->cfg_name, 64);
		} else if (!strncmp(config, "chan_mode", 9)) {
			if (!strncmp(value, "mm", 2)) {
				h->chan_mode = MDB5_CHAN_MODE_MM;
			} else {
				mdb5_dma_err("Invalid channel mode: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "address", 7)) {
			if (arg_read_int_ull(value, &h->address)) {
				mdb5_dma_err("Invalid address: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_rd_channels", 15)) {
			if (arg_read_int(value, &h->num_rd_chan) ||
			    is_negative((int8_t)h->num_rd_chan)) {
				mdb5_dma_err("Invalid num of read channels:"
					     "%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_wr_channels", 15)) {
			if (arg_read_int(value, &h->num_wr_chan) ||
			    is_negative((int8_t)h->num_wr_chan)) {
				mdb5_dma_err("Invalid num of write channels:"
					     "%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "transfer_mode", 13)) {
			if (!strncmp(value, "sg", 2)) {
				xfer_mode = MDB5_MODE_SG;
			} else if (!strncmp(value, "simple", 6)) {
				xfer_mode = MDB5_MODE_SIMPLE;
			} else {
				mdb5_dma_err("Unknown transfer mode %s\n",
					     value);
				goto parse_cleanup;
			}
			hioc->cmode.mode = xfer_mode;
		} else if (!strncmp(config, "pkt_sz", 6)) {
			if (arg_read_int_ull(value, &h->pkt_sz) ||
			    is_negative((long long)h->pkt_sz)) {
				mdb5_dma_err("Invalid pkt_sz: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "io_type", 6)) {
			if (!strncmp(value, "io_sync", 6)) {
				h->io_type = MDB5_IO_SYNC;
			} else if (!strncmp(value, "io_async", 6)) {
				h->io_type = MDB5_IO_ASYNC;
			} else {
				mdb5_dma_err("Unknown io_type: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "inputfile", 9)) {
			copy_value(value, input_file, 128);
		} else if (!strncmp(config, "outputfile", 10)) {
			copy_value(value, output_file, 128);
		}
	}
	if (linebuf)
		free(linebuf);
	fclose(fp);

	return 0;

parse_cleanup:
	fclose(fp);
	return -EINVAL;
}

/**
 * This function validates the input parameters provided in the configuration
 * file. It checks for the presence and correctness of various parameters such
 * as input and output files, number of read and write channels, packet size,
 * and other configuration settings.
 *
 * @param h Pointer to a structure of type @c mdb5_dma_common containing the
 *          configuration parameters.
 *
 * @param cfg_fname The name of the configuration file being validated.
 *
 * @return 0 on success, -EINVAL if an error occurs.
 */
static int validate_input_params(struct mdb5_dma_common *h, char *cfg_fname)
{
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	struct stat st;
	uint32_t num_rd_chan = h->num_rd_chan;
	uint32_t num_wr_chan = h->num_wr_chan;
	int ret = 0;

	if (!num_wr_chan && !num_rd_chan) {
		mdb5_dma_err("Invalid number of write and read channels\n");
		ret = -EINVAL;
		goto error;
	}

	if (num_wr_chan > 0) {
		if (*input_file == '\0') {
			mdb5_dma_err("Data input file is not provided\n");
			ret = -EINVAL;
			goto error;
		}

		ret = stat(input_file, &st);
		if (ret < 0) {
			mdb5_dma_err("Failed to stat file '%s': %s\n",
				     input_file, mdb5_dma_strerror(errno));
			goto error;
		}

		ret = validate_pkt_size(h->pkt_sz);
		if (ret < 0) {
			goto error;
		}

		if (h->pkt_sz > st.st_size) {
			mdb5_dma_err("File [%s] content size is lesser than "
				     "pkt_sz %lu\n", input_file, h->pkt_sz);
			ret = -EINVAL;
			goto error;
		}
	}

	if (num_rd_chan > 0 && *output_file == '\0') {
		mdb5_dma_err("Data output file is not provided\n");
		ret = -EINVAL;
		goto error;
	}

	if (num_wr_chan > MDB5_MAX_WR_CHAN ||
	    num_rd_chan > MDB5_MAX_RD_CHAN) {
		mdb5_dma_err("Input read or write channels exceeds %d or "
			     "%d\n", MDB5_MAX_RD_CHAN, MDB5_MAX_WR_CHAN);
		ret = -EINVAL;
		goto error;
	}

	if (verbose) {
		/* Config file contents dump */
		printf("\n%s contents..\n", cfg_fname);
		printf("Configuration name=%s\n", h->cfg_name);
		printf("Channel mode=%s\n", get_chan_mode(h->chan_mode));
		printf("Num of read channels=%d\n", num_rd_chan);
		printf("Num of write channels=%d\n", num_wr_chan);
		printf("Address=0x%lx\n", h->address);
		printf("Transfer mode=%s\n", get_transfer_mode(hioc->cmode.mode));
		printf("Packet size=%ld\n", h->pkt_sz);
		printf("IO type=%s\n", get_io_type(h->io_type));
		printf("Input file=%s\n", input_file);
		printf("Output file=%s\n\n", output_file);
	}

error:
	return ret;
}

/**
 * This function configures the MDB5 channels for read and write operations.
 * It prepares the node names, checks file availability, configures transfer
 * modes for each channel.
 *
 * @param h Pointer to a structure of type @c mdb5_dma_common containing the
 *          configuration.
 *
 * @param chan Pointer to an array of @c mdb5_dma_channel structures where the
 *        channel information will be stored.
 *
 * @return Number of configured channels on success, or a negative error code
 *         on failure.
 */
static int mdb5_dma_config_channels(struct mdb5_dma_common *h,
				    struct mdb5_dma_channel *chan)
{
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	uint32_t num_rd_chan = h->num_rd_chan;
	uint32_t num_wr_chan = h->num_wr_chan;
	uint32_t ch_count, i = 0, j = 0;
	int ret = 0;

	hioc->h = h;
	ch_count = num_rd_chan + num_wr_chan;

	h->set_mode = true;

	prepare_node_name(h->ctrl_name, "ctrl", 0);
	if (!is_file_available(h->ctrl_name)) {
		mdb5_dma_err("Invalid controller name:%s\n", h->ctrl_name);
		return -ENOENT;
	}

	for (i = 0; i < num_wr_chan; i++) {
		chan[i].ch_id = i;
		chan[i].dir = MDB5_CHAN_DIR_TO_DEV;
		chan[i].address = h->address;

		prepare_node_name(chan[i].name, "write", i);

		if (!is_file_available(chan[i].name)) {
			mdb5_dma_err("Invalid controller name: %s\n",
				     h->ctrl_name);
			return -ENOENT;
		}

		memcpy(&hioc->cmode.name, &chan[i].name, MDB5_NODE_CHAN_SZ);
		ret = mdb5_dma_transfer_mode((void *)hioc);
		if (ret < 0)
			return ret;
	}

	for (i = 0; i < num_rd_chan; i++) {
		j = i + num_wr_chan;
		chan[j].ch_id = i;
		chan[j].dir = MDB5_CHAN_DIR_FROM_DEV;
		chan[j].address = h->address;

		prepare_node_name(chan[j].name, "read", i);

		if (!is_file_available(chan[j].name)) {
			mdb5_dma_err("Invalid controller name: %s\n",
				     h->ctrl_name);
			return -ENOENT;
		}

		memcpy(&hioc->cmode.name, &chan[j].name, MDB5_NODE_CHAN_SZ);
		ret = mdb5_dma_transfer_mode((void *)hioc);
		if (ret < 0)
			return ret;
	}

	return ch_count;
}

/**
 * @brief Perform a DMA write operation on the specified channel.
 *
 * This function performs a DMA write operation on the specified MDB5 channel by
 * reading data from the provided input file into a buffer and then transferring
 * it to the device memory. It supports both synchronous and asynchronous DMA
 * transfers. The function measures the time taken for each write operation and
 * calculates the average bandwidth.
 *
 * @param chan Pointer to the MDB5 channel structure.
 *
 * @param input_file Path to the input file containing data to be written.
 *
 * @param io_type Type of I/O operation (MDB5_IO_SYNC for synchronous,
 *        MDB5_IO_ASYNC for asynchronous).
 *
 * @param pkt_sz Size of the data packet to be transferred.
 *
 * @return 0 on success, negative error code on failure.
 */
static int mdb5_dmautils_write(struct mdb5_dma_channel *chan, char *input_file,
			       int io_type, size_t pkt_sz)
{
	size_t size;
	int infile_fd = -1, ret = 0;
	char *buffer = NULL, *allocated = NULL;

	if (!chan || !input_file) {
		mdb5_dma_err("Invalid input params\n");
		return -EINVAL;
	}

	size = pkt_sz;

	infile_fd = open(input_file, O_RDONLY | O_NONBLOCK);
	if (infile_fd < 0) {
		mdb5_dma_err("Unable to open input file %s, ret: %d\n",
			     input_file, infile_fd);
		return infile_fd;
	}

	posix_memalign((void **)&allocated, ALIGN_4K, size + ALIGN_4K);
	if (!allocated) {
		mdb5_dma_err("OOM %lu.\n", size + ALIGN_4K);
		ret = -ENOMEM;
		goto out;
	}

	buffer = allocated;
	ret = read_to_buffer(input_file, infile_fd, buffer, size, 0);
	if (ret < 0)
		goto out;

	if (io_type == MDB5_IO_SYNC) {
		ret = mdb5_dmautils_sync_xfer(chan->name, chan->dir, buffer,
					  size, chan->address);
		if (ret < 0)
			mdb5_dma_err("Channel %s: Sync transfer to device failed, "
				     "ret: %d\n", chan->name, ret);
		else
			mdb5_dma_info("Channel %s: Sync transfer to device "
				      "success\n", chan->name);
	} else if (io_type == MDB5_IO_ASYNC) {
		ret = mdb5_dmautils_async_xfer(chan->name, chan->dir, buffer,
					   size, chan->address);
		if (ret < 0)
			mdb5_dma_err("Channel %s: Async transfer to device failed, "
				     "ret: %d\n", chan->name, ret);
		else
			mdb5_dma_info("Channel %s: Async transfer to device "
				      "success\n", chan->name);
	}

out:
	free(allocated);
	close(infile_fd);

	return ret;
}

/**
 * @brief Perform a DMA read operation on the specified channel.
 *
 * This function performs a read operation on the specified MDB5 channel.
 * It reads data from the device memory and writes it to the specified output
 * file. The function supports both synchronous and asynchronous transfer modes.
 * It measures the time taken for each transfer and handles memory allocation
 * for the buffer used in the transfer.
 *
 * @param chan Pointer to the MDB5 channel structure.
 *
 * @param output_file Path to the output file where the read data will be
 *        written.
 *
 * @param io_type Type of I/O operation (MDB5_IO_SYNC for synchronous,
 *        MDB5_IO_ASYNC for asynchronous).
 *
 * @param pkt_sz Size of the data packet to be read.
 *
 * @return 0 on success, negative error code on failure.
 */
static int mdb5_dmautils_read(struct mdb5_dma_channel *chan, char *output_file,
			      int io_type, size_t pkt_sz)
{
	size_t size;
	int outfile_fd = -1, ret = 0;
	char *buffer = NULL, *allocated = NULL;

	if (!chan || !output_file) {
		mdb5_dma_err("Invalid input params\n");
		return -EINVAL;
	}

	size = pkt_sz;

	outfile_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
			  0666);
	if (outfile_fd < 0) {
		mdb5_dma_err("Unable to open/create output file %s, ret: %d\n",
			     output_file, outfile_fd);
		return outfile_fd;
	}

	posix_memalign((void **)&allocated, ALIGN_4K, size + ALIGN_4K);
	if (!allocated) {
		mdb5_dma_err("OOM %lu.\n", size + ALIGN_4K);
		ret = -ENOMEM;
		goto out;
	}
	buffer = allocated;

	if (io_type == MDB5_IO_SYNC) {
		ret = mdb5_dmautils_sync_xfer(chan->name, chan->dir, buffer,
					      size, chan->address);
		if (ret < 0)
			mdb5_dma_err("Channel %s: Sync transfer from device "
				     "failed, ret: %d\n", chan->name, ret);
		else
			mdb5_dma_info("Channel %s: Sync transfer from device "
				      "success\n", chan->name);
	} else if (io_type == MDB5_IO_ASYNC) {
		ret = mdb5_dmautils_async_xfer(chan->name, chan->dir, buffer,
					   size, chan->address);
		if (ret < 0)
			mdb5_dma_err("Channel %s: Async transfer from device "
				     "failed, ret: %d\n", chan->name, ret);
		else
			mdb5_dma_info("Channel %s: Async transfer from device "
				      "success\n", chan->name);
	}
	if (ret < 0)
		goto out;

	ret = write_from_buffer(output_file, outfile_fd, buffer, size, 0);
	if (ret < 0)
		mdb5_dma_err("Write from buffer to %s failed\n", output_file);
out:
	free(allocated);
	close(outfile_fd);

	return ret;
}

/**
 * This function performs DMA transfers based on the configuration provided
 * in the @c mdb5_dma_channel structure. It iterates over the specified number
 * of channels and performs read or write operations depending on the direction
 * of each channel.
 *
 * @param h Pointer to a structure of type @c mdb5_dma_common containing the
 *        configuration.
 *
 * @param count The number of channels to be processed.
 *
 * @return 0 on success, a negative error code on failure.
 */
static int mdb5_dmautils_xfer(struct mdb5_dma_common *h, uint32_t ch_count)
{
	struct mdb5_dma_channel *chan = h->chan;
	size_t pkt_sz = h->pkt_sz;
	int ret = 0, i;
	int io_type = h->io_type;

	if (!chan || ch_count == 0) {
		mdb5_dma_err("Invalid input params\n");
		return -EINVAL;
	}

	for (i = 0; i < ch_count; i++) {
		if (chan[i].dir == MDB5_CHAN_DIR_TO_DEV) {
			/* Transfer DATA from inputfile to Device */
			ret = mdb5_dmautils_write(chan + i, input_file, io_type,
						  pkt_sz);
			if (ret < 0)
				goto error;
		} else if (chan[i].dir == MDB5_CHAN_DIR_FROM_DEV) {
			/* Reads data from Device and writes into output file*/
			ret = mdb5_dmautils_read(chan + i, output_file, io_type,
						 pkt_sz);
			if (ret < 0)
				goto error;
		}
	}

error:
	return ret;
}

int main(int argc, char *argv[])
{
	struct mdb5_dma_common h;
	uint8_t max_chan = MDB5_MAX_RD_CHAN + MDB5_MAX_WR_CHAN;
	struct mdb5_dma_channel chan[max_chan];
	int ch_count = 0, ret = 0;
	char *cfg_fname = NULL;

	memset(&h, 0, sizeof(struct mdb5_dma_common));
	memset(chan, 0, max_chan * sizeof(struct mdb5_dma_channel));
	h.chan = chan;

	ret = parse_options(argc, argv, &cfg_fname, &h);
	if (ret < 0)
		goto error;

	ret = parse_config_file(cfg_fname, &h);
	if (ret < 0) {
		mdb5_dma_err("Config File: %s has invalid parameters\n", cfg_fname);
		goto error;
	}

	ret = validate_input_params(&h, cfg_fname);
	if (ret < 0) {
		mdb5_dma_err("Invalid Input parameters in Config File: %s\n",
			     cfg_fname);
		goto error;
	}

	ch_count = mdb5_dma_config_channels(&h, chan);
	if (ch_count < 0) {
		mdb5_dma_err("Channel configuration failed\n");
		goto error;
	}

	ret = mdb5_dmautils_xfer(&h, ch_count);
	if (ret < 0) {
		mdb5_dma_err("DMA transfer failed\n");
		goto error;
	}

error:
	if (cfg_fname)
		free(cfg_fname);
	return ret;
}
