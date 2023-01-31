#
#/*
# * This file is part of the QDMA userspace application
# * to enable the user to execute the QDMA functionality
# *
# * Copyright (c) 2018-2022, Xilinx, Inc. All rights reserved.
# * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
# *
# * This source code is licensed under BSD-style license (found in the
# * LICENSE file in the root directory of this source tree)
# */

CC ?= gcc

CFLAGS += -g
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I../include
CFLAGS += $(EXTRA_FLAGS)

all: dmautils.o dmactl.o dmactl_reg.o dmaxfer.o dma_xfer_utils.o

%.o: %.c
	$(CC) $(CFLAGS) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	rm -rf *.o
