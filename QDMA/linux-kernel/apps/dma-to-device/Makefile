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
CFLAGS += -I. -I../include -I../dma-utils
CFLAGS += $(EXTRA_FLAGS)

DMA-TO-DEVICE = dma-to-device
DMA-UTILS_OBJS := $(patsubst %.c,%.o,$(wildcard ../dma-utils/*.c))
DMA-TO-DEVICE_OBJS := dma_to_device.o
DMA-TO-DEVICE_OBJS += $(DMA-UTILS_OBJS)

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
endif

all: clean dma-to-device

dma-to-device: $(DMA-TO-DEVICE_OBJS)
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

%.o: %.c
	$(CC) $(CFLAGS) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	@rm -f *.o */*.o ../dma-utils/*.o
	rm -rf *.o *.bin dma-to-device
