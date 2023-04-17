SHELL = /bin/bash

#
# makefile parameters:
# - KDIR/KSRC/KOBJ=, optional
# - install_path=,			override all install directories
# - kernel_install_path=,	override install directory for kernel module
# - dev_install_path=,		override install directory for development headers
# - apps_install_path=,		override install directory for applications
# - docs_install_path=,		override install directory for man pages
#
# - enable_cmpt_immediate_data=<0|1>	enable immediate data in writeback desc.
# - disable_st_c2h_completion=<0|1>	disable completion
# - CROSS_COMPILE=,  gcc compiler prefix for architecture eg. aarch64-linux-gnu-

# Define grep error output to NULL, since -s is not portable.
grep = grep 2>/dev/null

# Define paths.
srcdir := $(shell pwd)
topdir := $(shell cd $(srcdir)/.. && pwd)
bin_dir := $(srcdir)/bin
apps_dir := $(srcdir)/apps


# ALL subdirectories
ALLSUBDIRS := driver
DRIVER_SRC_DIR := driver

# Exports.
export grep
export srcdir
export topdir
export bin_dir
export verbose

# evaluate install paths
ifeq ($(install_path),)
	# defaults
	kernel_install_path ?= $(PREFIX)/lib/modules/$(utsrelease)/updates/kernel/drivers/qdma
	dev_install_path ?= /usr/local/include/qdma
	apps_install_path ?= /usr/local/sbin
	docs_install_path ?= /usr/share/man/man8
else # bundled install
	kernel_install_path ?= $(install_path)/modules
	dev_install_path ?= $(install_path)/include/qdma
	apps_install_path ?= $(install_path)/bin
	docs_install_path ?= $(install_path)/doc
endif

.PHONY: eval.mak

.PHONY: default
default: apps driver post

.PHONY: install
install: install-apps install-dev install-mods

.PHONY: uninstall
uninstall: uninstall-apps uninstall-dev uninstall-mods

.PHONY: apps
apps:
	@echo "#######################";
	@echo "####  apps        ####";
	@echo "#######################";
	$(MAKE) -C apps

.PHONY: driver
driver:
	@echo "#######################";
	@echo "####  Driver       ####";
	@echo "#######################";
	$(MAKE) $(MODULE) -C driver modulesymfile=$(srcdir)/driver/src/Module.symvers

.PHONY: post
post:
	@if [ -n "$(post_msg)" ]; then \
	   echo -e "\nWARNING:\n $(post_msg)";\
	 fi;

.PHONY: clean
clean:
	@echo "#######################";
	@echo "####  apps         ####";
	@echo "#######################";
	$(MAKE) -C apps clean;
	@for dir in $(ALLSUBDIRS); do \
	   echo "#######################";\
	   printf "####  %-8s%5s####\n" $$dir;\
	   echo "#######################";\
	  drvdir=$(shell pwd)/$$dir $(MAKE) -C $$dir clean;\
	done;
	@-/bin/rm -f *.symvers eval.mak 2>/dev/null;
	@rm -rf $(bin_dir)

.PHONY: install-mods
install-mods:
	@echo "installing kernel modules to /lib/modules/$(shell uname -r)/qdma ..."
	@mkdir -p -m 755 /lib/modules/$(shell uname -r)/qdma
	@install -v -m 644 $(bin_dir)/*.ko /lib/modules/$(shell uname -r)/qdma
	@depmod -a || true


.PHONY: install-apps
install-apps:
	@echo "installing apps to $(apps_install_path) ..."
	@mkdir -p -m 755 $(apps_install_path)
	@install -v -m 755 bin/dma-ctl* $(apps_install_path)
	@install -v -m 755 bin/dma-xfer* $(apps_install_path)
	@install -v -m 755 bin/dma-from-device $(apps_install_path)
	@install -v -m 755 bin/dma-to-device $(apps_install_path)
	@install -v -m 755 bin/dma-perf $(apps_install_path)
	@install -v -m 755 bin/dma-latency $(apps_install_path)
	@echo "MAN PAGES:"
	@mkdir -p -m 755 $(docs_install_path)
	@install -v -m 644 docs/dmactl.8.gz $(docs_install_path)

.PHONY: install-dev
install-dev:
	@echo "installing development headers to $(dev_install_path) ..."
	@mkdir -p -m 755 $(dev_install_path)
	@install -v -m 755 driver/include/* $(dev_install_path)

.PHONY: uninstall-mods
uninstall-mods:
	@echo "Un-installing /lib/modules/$(shell uname -r)/qdma ..."
	@/bin/rm -rf /lib/modules/$(shell uname -r)/qdma
	@depmod -a

.PHONY: uninstall-apps
uninstall-apps:
	@echo "Un-installing apps under $(apps_install_path) ..."
	@/bin/rm -f $(apps_install_path)/dma-ctl
	@/bin/rm -f $(apps_install_path)/dma-xfer
	@/bin/rm -f $(apps_install_path)/dma-from-device
	@/bin/rm -f $(apps_install_path)/dma-to-device
	@/bin/rm -f $(apps_install_path)/dma-perf
	@/bin/rm -f $(apps_install_path)/dma-latency

.PHONY: uninstall-dev
uninstall-dev:
	@echo "Un-installing development headers under $(dev_install_path) ..."
	@/bin/rm -r $(dev_install_path)

.PHONY: help
help:
	@echo "Build Targets:";\
	 echo " install             - Installs apps and development headers.";\
	 echo " uninstall           - Uninstalls apps and development headers.";\
	 echo " install-mods        - Installs drivers.";\
	 echo " uninstall-mods      - Uninstalls drivers.";\
	 echo;\
	 echo "Build Options:";\
	 echo " KOBJ=<path>         - Kernel build (object) path.";\
	 echo " KSRC=<path>         - Kernel source path.";\
	 echo "                     - Note: When using KSRC or KOBJ, both";\
	 echo "                             variables must be specified.";\
	 echo " KDIR=<path>         - Kernel build and source path. Shortcut";\
	 echo "                       for KOBJ=KSRC=<path>.";\
	 echo " kernel_install_path=<path>";\
	 echo "                     - kernel module install path.";\
	 echo " apps_install_path=<path>";\
	 echo "                     - user cli tool install path.";
