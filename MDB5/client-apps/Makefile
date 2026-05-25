#
# This file is part of the MDB5 userspace application
# to enable the user to execute the MDB5 functionality
#
# Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
#
# This source code is licensed under BSD-style license (found in the
# LICENSE file in the root directory of this source tree)
#

SHELL = /bin/bash

srcdir := $(shell pwd)
bin_dir := $(srcdir)/bin
dma-ctl_dir = $(srcdir)/dma-ctl
dma-from-device_dir = $(srcdir)/dma-from-device
dma-to-device_dir = $(srcdir)/dma-to-device
dma-xfer_dir = $(srcdir)/dma-xfer

export bin_dir

all: clean apps

.PHONY: dma-ctl
dma-ctl:
	@echo "#######################";
	@echo "####  dma-ctl    ######";
	@echo "#######################";
	@mkdir -p -m 755 $(bin_dir)
	$(MAKE) -C dma-ctl
	@cp -f $(dma-ctl_dir)/dma-ctl $(bin_dir)

.PHONY: dma-from-device
dma-from-device:
	@echo "#######################";
	@echo "### dma-from-device ###";
	@echo "#######################";
	@mkdir -p -m 755 $(bin_dir)
	$(MAKE) -C dma-from-device
	@cp -f $(dma-from-device_dir)/dma-from-device $(bin_dir)

.PHONY: dma-to-device
dma-to-device:
	@echo "#######################";
	@echo "###  dma-to-device  ###";
	@echo "#######################";
	@mkdir -p -m 755 $(bin_dir)
	$(MAKE) -C dma-to-device
	@cp -f $(dma-to-device_dir)/dma-to-device $(bin_dir)

.PHONY: dma-xfer
dma-xfer:
	@echo "#######################";
	@echo "###  dma-xfer  ###";
	@echo "#######################";
	@mkdir -p -m 755 $(bin_dir)
	$(MAKE) -C dma-xfer
	@cp -f $(dma-xfer_dir)/dma-xfer $(bin_dir)

.PHONY: apps
apps: dma-ctl dma-from-device dma-to-device dma-xfer


.PHONY: clean
clean:
	@echo "#############################";
	@echo "####       dma-ctl       ####";
	@echo "#############################";
	$(MAKE) -C dma-ctl clean;
	@rm -f $(bin_dir)/dma-ctl
	@echo "#############################";
	@echo "####  dma-from-device    ####";
	@echo "#############################";
	$(MAKE) -C dma-from-device clean;
	@rm -f $(bin_dir)/dma-from-device
	@echo "#############################";
	@echo "####   dma-to-device     ####";
	@echo "#############################";
	$(MAKE) -C dma-to-device clean;
	@rm -f $(bin_dir)/dma-to-device
	@echo "#############################";
	@echo "####       dma-xfer      ####";
	@echo "#############################";
	$(MAKE) -C dma-xfer clean;
	@rm -rf $(bin_dir)
