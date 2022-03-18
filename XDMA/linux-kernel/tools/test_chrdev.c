/*
 * This file is part of the Xilinx DMA IP Core driver tools for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

	int fd;
	char *filename;

	if ( (argc < 2) || (argc >= 3))
	{
		printf("usage %s <device file>\n",argv[0]);
		return -1;
	}
	filename = argv[1];
	fd = open(filename,O_RDWR);
	if (fd < 0)
	{
		perror("Device open Failed");
		return fd;
	}
	printf("%s Device open successfull\n",argv[1]);

	if ( close(fd) )
	{
		perror("Device Close Failed");
		return -1;
	}
	printf("%s Device close successfull\n",argv[1]);

	return 0;
}
