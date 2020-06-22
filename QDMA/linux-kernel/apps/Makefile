SHELL = /bin/bash

CFLAGS += -g
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += $(EXTRA_FLAGS)

srcdir := $(shell pwd)
topdir := $(shell cd $(srcdir)/.. && pwd)
bin_dir := $(topdir)/bin
dma-ctl_dir = $(srcdir)/dma-ctl
dma-from-device_dir = $(srcdir)/dma-from-device
dma-to-device_dir = $(srcdir)/dma-to-device
dma-xfer_dir = $(srcdir)/dma-xfer
dma-perf_dir = $(srcdir)/dma-perf
dma-latency_dir = $(srcdir)/dma-latency

export topdir
export bin_dir

all: clean apps

.PHONY: dma-from-device
dma-from-device:
	@echo "#############################";
	@echo "####  dma-from-device    ####";
	@echo "#############################";
	$(MAKE) -C dma-from-device
	@cp -f $(dma-from-device_dir)/dma-from-device $(bin_dir)	

.PHONY: dma-to-device
dma-to-device:
	@echo "###########################";
	@echo "####  dma-to-device    ####";
	@echo "###########################";
	$(MAKE) -C dma-to-device
	@cp -f $(dma-to-device_dir)/dma-to-device $(bin_dir)	
	
.PHONY: dma-xfer
dma-xfer:
	@echo "###########################";
	@echo "####  dma-xfer         #####";
	@echo "###########################";
	$(MAKE) -C dma-xfer
	@cp -f $(dma-xfer_dir)/dma-xfer $(bin_dir)		

.PHONY: dma-ctl
dma-ctl:
	@echo "#######################";
	@echo "####  dma-ctl    ######";
	@echo "#######################";
	@mkdir -p -m 755 $(bin_dir)
	$(MAKE) -C dma-ctl
	@cp -f $(dma-ctl_dir)/dma-ctl $(bin_dir)	


.PHONY: dma-perf
dma-perf:
	@echo "########################";
	@echo "####  dma-perf    #######";
	@echo "########################";
	$(MAKE) -C dma-perf
	@cp -f $(dma-perf_dir)/dma-perf $(bin_dir)	

.PHONY: dma-latency
dma-latency:
	@echo "########################";
	@echo "####  dma-latency   #######";
	@echo "########################";
	$(MAKE) -C dma-latency
	@cp -f $(dma-latency_dir)/dma-latency $(bin_dir)	
	
.PHONY: apps
apps: dma-ctl dma-from-device dma-to-device dma-xfer dma-perf dma-latency


.PHONY: clean
clean:
	@echo "#############################";
	@echo "####  dma-from-device    ####";
	@echo "#############################";
	$(MAKE) -C dma-from-device clean;
	@echo "#############################";
	@echo "####  dma-to-device      ####";
	@echo "#############################";
	$(MAKE) -C dma-to-device clean;
	@echo "#############################";
	@echo "####  dma-xfer            ####";
	@echo "#############################";
	$(MAKE) -C dma-xfer clean;
	@echo "#############################";
	@echo "####  dma-ctl             ####";
	@echo "#############################";
	$(MAKE) -C dma-ctl clean;
	@echo "#############################";
	@echo "####  dma-perf            ####";
	@echo "#############################";
	$(MAKE) -C dma-perf clean;
	@echo "#############################";
	@echo "####  dma-latency         ####";
	@echo "#############################";
	$(MAKE) -C dma-latency clean;
	@rm -f $(bin_dir)/dma-ctl $(bin_dir)/dma-from-device $(bin_dir)/dma-to-device $(bin_dir)/dma-xfer $(bin_dir)/dma-perf $(bin_dir)/dma-latency
	@for dir in $(ALLSUBDIRS); do \
	   echo "#######################";\
	   printf "####  %-8s%5s####\n" $$dir;\
	   echo "#######################";\
	  drvdir=$(shell pwd)/$$dir $(MAKE) -C $$dir clean;\
	done;
	@-/bin/rm -f *.symvers eval.mak 2>/dev/null;

