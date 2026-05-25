#
# This file is part of the Xilinx DMA IP Core driver for Linux
#
# Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
#
# This source code is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#

INSMOD=insmod
MAKE=make
RM=rm
CP=cp
FILE=file
MKDIR=mkdir
MACHINE=`uname -p`
KDIR = /lib/modules/`uname -r`/build
MOD_DIR=bin/$(MACHINE)

DRV_MOD_NAME := mdb5-dmaclient-drv
DRV_OBJS := mdb5-dmaclient-cdev.o mdb5-dmaclient-mod.o

CFLAGS :=

obj-m += $(DRV_MOD_NAME).o
$(DRV_MOD_NAME)-y := $(DRV_OBJS)

.PHONY: all

all: driver

.PHONY: driver
driver:
	if [ ! -d bin/$(MACHINE) ]; then $(MKDIR) -p bin/$(MACHINE); fi
	$(MAKE) $(CFLAGS) -C $(KDIR) M=$(PWD) modules
	$(CP) -rf *.ko bin/$(MACHINE)/
	$(FILE) $(MOD_DIR)/*.ko

.PHONY: clean
clean:
	$(MAKE) $(CFLAGS) -C $(KDIR) M=$(PWD) clean
	$(RM) -rf bin *.mod

.PHONY: install
install:
	$(INSMOD) bin/$(MACHINE/$(DRV_MOD_NAME).ko
