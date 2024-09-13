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

SHELL = /bin/bash

CFLAGS += -g -Wall
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I../include -I../dma-utils
CFLAGS += $(EXTRA_FLAGS)

DMA-UTILS_OBJS := $(notdir $(patsubst %.c,%.o,$(wildcard ../dma-utils/*.c)))

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
endif
 
all: clean qdma_api.so qdma_api3.so

%.o: ../dma-utils/%.c
	echo $(DMA-UTILS_OBJS)
	$(CC) $(CFLAGS) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE -fpic

qdma_api.so: python_bindings.c ${DMA-UTILS_OBJS}
	$(CC) $^ -o $@ -laio -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_REENTRANT -fpic -shared -I/usr/include/python2.7 ${CFLAGS}
	
qdma_api3.so: python_bindings.c ${DMA-UTILS_OBJS}
	$(CC) $^ -o $@ -laio -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_REENTRANT -fpic -shared  -I /export/crosstools/yocto-3.0/748/changeling-hypervisor/sysroots/corei7-64-spirent-linux/usr/include/python3.7m -I/export/crosstools/yocto-3.0/748/changeling-hypervisor/sysroots/corei7-64-spirent-linux/usr/include/ ${CFLAGS}
	
clean:
	@rm -f *.so *.o
