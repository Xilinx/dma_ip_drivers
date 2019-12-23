/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2019,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */
#include "qdmautils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../include/qdma_nl.h"
#include "dmaxfer.h"
#include "version.h"

#define QDMA_Q_NAME_LEN 100

enum qdma_reg_op {
	QDMA_Q_REG_DUMP,
	QDMA_Q_REG_RD,
	QDMA_Q_REG_WR,
};

int qdmautils_ioctl_nomemcpy(char *filename)
{
	unsigned char no_memcpy = 1;
	int fd;
	int ret;

	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("Error: Cannot find %s\n", filename);
		return -EINVAL;
	}

	ret = ioctl(fd, 0, &no_memcpy);
	if (ret != 0) {
		printf("failed to set non memcpy\n");
		return -EINVAL;
	}
	close(fd);

	return 0;
}

void qdmautils_get_version(struct qdmautils_version *ver)
{
	memset(ver->version, '\0', QDMAUTILS_VERSION_LEN);
	memcpy(ver->version, QDMATUILS_VERSION, QDMAUTILS_VERSION_LEN);
}

int qdmautils_sync_xfer(char *filename, enum qdmautils_io_dir dir, void *buf,
			unsigned int xfer_len)
{
	return dmaxfer_iosubmit(filename, dir, DMAXFER_IO_SYNC, buf, xfer_len);
}

int qdmautils_async_xfer(char *filename, enum qdmautils_io_dir dir, void *buf,
			unsigned int xfer_len)
{
	return dmaxfer_iosubmit(filename, dir, DMAXFER_IO_ASYNC, buf, xfer_len);
}
