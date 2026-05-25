#
# This file is part of the MDB5 userspace application
# to enable the user to execute the MDB5 functionality
#
# Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
#
# This source code is licensed under BSD-style license (found in the
# LICENSE file in the root directory of this source tree)
#

CC ?= gcc

CFLAGS += -g
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I.
CFLAGS += $(EXTRA_FLAGS) -std=c99

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
endif

all: clean dma_common_utils.o dma_config.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	rm -f *.o

