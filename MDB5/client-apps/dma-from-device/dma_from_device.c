/*
 * This file is part of the MDB5 userspace application
 * to enable the user to execute the MDB5 functionality
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "dma_common_utils.h"

#define DEVICE_NAME_DEFAULT		"/dev/mdb5_read00"
#define SIZE_DEFAULT			(32)
#define CONTROLLER_INDEX_DEFAULT	(0)
#define CHANNEL_INDEX_DEFAULT		(0)
#define NSEC_DIV			1000000000

static int verbose;

static const struct option long_opts[] = {
	{"channel", required_argument, NULL, 'c'},
	{"address", required_argument, NULL, 'a'},
	{"size", required_argument, NULL, 's'},
	{"offset", required_argument, NULL, 'o'},
	{"data_output_file", required_argument, NULL, 'w'},
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

static int no_write;

static void usage(const char *name)
{
	int i = 0;

	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "Usage: %s [OPTIONS]\n\n", name);
	fprintf(stdout, "Read via Scatter-Gather / Simple DMA.\n\n");

	fprintf(stdout, "  -%c (--%s) Channel Index, default %d\n",
		long_opts[i].val, long_opts[i].name, CHANNEL_INDEX_DEFAULT);
	i++;
	fprintf(stdout, "  -%c (--%s) Address on the device memory\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,
		"  -%c (--%s) size of a single transfer in bytes, default %d\n",
		long_opts[i].val, long_opts[i].name, SIZE_DEFAULT);
	i++;
	fprintf(stdout, "  -%c (--%s) page offset of transfer, default 0\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,
		"  -%c (--%s) Optional output file to write transfer data\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout, "  -%c (--%s) print usage help and exit\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout, "  -%c (--%s) verbose output\n",
		long_opts[i].val, long_opts[i].name);
}

/**
 * @brief Perform DMA test on the specified device.
 *
 * This function performs DMA FROM DEVICE on device node of the specified
 * channel by reading data from the device memory address to a buffer and
 * optionally saves output to a file. It measures the time taken for each
 * read operation and calculates the average bandwidth.
 *
 * @param devname Name of the device to test.
 *
 * @param addr Address on the device memory.
 *
 * @param size Size of the data to write in bytes.
 *
 * @param offset Read data offset.
 *
 * @param count Number of iterations for the test.
 *
 * @param ofname Name of the output file (optional).
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int test_dma(char *devname, uint64_t addr, size_t size,
		    size_t offset, char *ofname)
{
	ssize_t rc;
	char *buffer = NULL;
	char *allocated = NULL;
	struct timespec ts_start, ts_end;
	int out_fd = -1;
	int fpga_fd = open(devname, O_RDWR);
	double total_time = 0;
	double result;
	double avg_time = 0;

	if (fpga_fd < 0) {
		mdb5_dma_err("unable to open device %s, %d.\n", devname, fpga_fd);
		return -EINVAL;
	}

	/* create file to write data to */
	if (ofname) {
		out_fd = open(ofname, O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
			      0666);
		if (out_fd < 0) {
			mdb5_dma_err("unable to open output file %s, %d.\n",
				     ofname, out_fd);
			rc = -EINVAL;
			goto out;
		}
	}

	posix_memalign((void **)&allocated, ALIGN_4K, size + ALIGN_4K);
	if (!allocated) {
		mdb5_dma_err("OOM %lu.\n", size + ALIGN_4K);
		rc = -ENOMEM;
		goto out;
	}

	buffer = allocated + offset;
	if (verbose)
		fprintf(stdout, "host buffer 0x%lx, %p.\n", size + ALIGN_4K,
			buffer);

	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	/* read data from device memory address to buffer */
	rc = read_to_buffer(devname, fpga_fd, buffer, size, addr);
	if (rc < 0)
		goto out;
	clock_gettime(CLOCK_MONOTONIC, &ts_end);

	/* subtract the start time from the end time */
	timespec_sub(&ts_end, &ts_start);
	total_time +=
	    (ts_end.tv_sec + ((double)ts_end.tv_nsec / NSEC_DIV));
	/* a bit less accurate but side-effects are accounted for */
	if (verbose)
		mdb5_dma_info("#: CLOCK_MONOTONIC %ld.%09ld sec."
			      "read %lu bytes\n",
			      ts_end.tv_sec, ts_end.tv_nsec, size);

	/* file argument given? */
	if ((out_fd >= 0) & (!no_write)) {
		rc = write_from_buffer(ofname, out_fd, buffer,
				       size, 0);
		if (rc < 0)
			goto out;
	}

	avg_time = (double)total_time;
	result = ((double)size) / avg_time;
	if (verbose)
		mdb5_dma_info("** Avg time device %s, total time %f nsec, "
			      "avg_time = %f, size = %lu, BW = %f bytes/sec\n",
			      devname, total_time, avg_time, size, result);
	dump_throughput_result(size, result);

	rc = 0;

out:
	close(fpga_fd);
	if (out_fd >= 0)
		close(out_fd);
	free(allocated);

	return rc;
}

/**
 * @brief Main entry point for the MDB5 test application.
 *
 * This function tests the DMA FROM DEVICE functionality by parsing
 * command-line arguments to configure various parameters such as
 * controller index, channel index, device memory address, size,
 * offset and output file name.
 *
 * @param argc Number of command-line arguments.
 *
 * @param argv Array of command-line arguments.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int main(int argc, char *argv[])
{
	uint64_t address = 0;
	size_t offset = 0;
	size_t size = SIZE_DEFAULT;
	uint8_t ch_id = CHANNEL_INDEX_DEFAULT;
	char name[MDB5_NODE_CHAN_SZ];
	char *ofname = NULL;
	int cmd_opt, ret = 0;

	if (argc == 1) {
		usage(argv[0]);
		exit(0);
	}

	while ((cmd_opt = getopt_long(argc, argv, "vhc:w:a:s:o:",
				      long_opts, NULL)) != -1) {
		switch (cmd_opt) {
		case 0:
			/* long option */
			break;
		case 'c':
			/* Channel Index */
			ch_id = getopt_integer(optarg);
			if (is_negative((int8_t)ch_id)) {
				mdb5_dma_err("Invalid channel index: %s\n",
					     optarg);
				ret = -EINVAL;
				goto error;
			}
			if (ch_id >= MDB5_MAX_RD_CHAN ||
			    ch_id >= MDB5_MAX_WR_CHAN) {
				mdb5_dma_err("Channel index: %s is not in range "
					     "RD: 0-%d, WR: 0-%d)\n",
					     optarg, MDB5_MAX_RD_CHAN - 1,
					     MDB5_MAX_WR_CHAN - 1);
				ret = -EINVAL;
				goto error;
			}
			break;
		case 'a':
			/* address on the device memory in bytes */
			address = getopt_integer(optarg);
			break;
		case 's':
			/* size in bytes */
			size = getopt_integer(optarg);
			if (is_negative((ssize_t)size)) {
				mdb5_dma_err("Invalid size: %s\n", optarg);
				ret = -EINVAL;
				goto error;
			}
			break;
		case 'o':
			/* Read data offset */
			offset = getopt_integer(optarg) & 4095;
			if (is_negative((int64_t)getopt_integer(optarg))) {
				mdb5_dma_err("Invalid offset: %s\n", optarg);
				ret = -EINVAL;
				goto error;
			}
			break;
		case 'w':
			ofname = strdup(optarg);
			break;
			/* print usage help and exit */
		case 'v':
			verbose = 1;
			break;
		case 'h':
			/* print usage help and exit */
		default:
			usage(argv[0]);
			exit(0);
			break;
		}
	}

	prepare_node_name(name, "read", ch_id);
	if (!is_file_available(name)) {
		mdb5_dma_err("Invalid controller name: %s\n", name);
		ret = -ENOENT;
		goto error;
	}

	if (verbose)
		mdb5_dma_info("dev: %s, addr: 0x%lx, size: 0x%lx, offset: 0x%lx\n",
			      name, address, size, offset);

	ret = test_dma(name, address, size, offset, ofname);

error:
	if (ofname)
		free(ofname);
	return ret;
}
