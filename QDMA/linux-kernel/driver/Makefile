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

# ALL subdirectories
ALLSUBDIRS := src
DRIVER_SRC_DIR := src

# subdirectories to be build
SUBDIRS := $(ALLSUBDIRS)

# Honor the -s (silent) make option.
verbose := $(if $(filter s,$(MAKEFLAGS)),,-v)

# Define paths.
srcdir := $(shell pwd)
topdir := $(shell cd $(srcdir)/.. && pwd)
bin_dir := $(topdir)/bin

kernel_check = 1
distro_check = 1

ifeq ($(filter clean,$(MAKECMDGOALS)),clean)
  kernel_check = 0
  distro_check = 0
endif

ifeq ($(filter uninstall,$(MAKECMDGOALS)),uninstall)
  distro_check = 0
endif

ifeq ($(kernel_check),1)
  include make_rules/kernel_check.mk

  ifeq ($(distro_check),1)
    include make_rules/distro_check.mk
  endif
endif

ifneq ($(wildcard $(KINC)/linux/kconfig.h),)
  FLAGS += -DKERNEL_HAS_KCONFIG_H
endif
ifneq ($(wildcard $(KINC)/linux/export.h),)
  FLAGS += -DKERNEL_HAS_EXPORT_H
endif

# Debug flags.
ifeq ($(DEBUG),1)
  FLAGS += -g -DDEBUG
endif

# Debugfs flags
ifneq ($(DEBUGFS),0)
  FLAGS += -DDEBUGFS
endif

# LOOPBACK flags.
ifeq ($(LOOPBACK),1)
  FLAGS += -DLOOPBACK_TEST
endif

ifeq ($(DEBUG_THREADS),1)
  FLAGS += -DDEBUG -DDEBUG_THREADS
endif
 
ifeq ($(enable_cmpt_immediate_data),1)
	FLAGS += -DXNL_IMM_DATA_EN
endif

ifeq ($(disable_st_c2h_completion),1)
	FLAGS += -DXMP_DISABLE_ST_C2H_CMPL
endif

# 64B Descriptor Bypass flags.
ifeq ($(TEST_64B_DESC_BYPASS),1)
  FLAGS += -DTEST_64B_DESC_BYPASS_FEATURE
endif

ifeq ($(ERR_DEBUG),1)
  EXTRA_FLAGS += -DERR_DEBUG
#  FLAGS += -DERR_DEBUG
  export EXTRA_FLAGS
endif

# Don't allow ARCH to overwrite the modified variable when passed to
# the sub-makes.
MAKEOVERRIDES := $(filter-out ARCH=%,$(MAKEOVERRIDES))
# Don't allow CFLAGS/EXTRA_CFLAGS to clobber definitions in sub-make.
MAKEOVERRIDES := $(filter-out CFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out EXTRA_CFLAGS=%,$(MAKEOVERRIDES))

# Exports.
export grep
export srcdir
export topdir
export bin_dir
export KERNELRELEASE
export KSRC
export KOBJ
export KINC
# arm64 specific fix to include <ksrc>/arch/<karch> folder properly.
# This hack is motivated by the RHEL7.X/CentOS7.X release where the
# uname Architecture is indicated as "aarch64" but the real Architecture
# source directory is "arm64"
ifeq ($(ARCH),aarch64)
  ifeq ($(wildcard $(KOBJ)/arch/$(ARCH)/Makefile),)
    override MAKECMDGOALS = $(MAKECMDGOALS) "ARCH=arm64"
    export ARCH
  else
    export ARCH
  endif
else
  export ARCH
endif
CFLAGS += -I$(srcdir)/src/libqdma/qdma_access
CFLAGS += -I$(srcdir)/src/libqdma
export FLAGS += $(CFLAGS)
#export FLAGS += $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS)
export verbose
export utsrelease
export kversions
export kseries
export modulesymfile

#export enable_xvc

CROSS_COMPILE_FLAG = $(CROSS_COMPILE)
export CROSS_COMPILE_FLAG
$(info CROSS_COMPILE_FLAG = $(CROSS_COMPILE_FLAG).)
$(info ARCH = $(ARCH).)

# evaluate install paths
ifeq ($(install_path),)
	# defaults
	kernel_install_path ?= $(PREFIX)/lib/modules/$(utsrelease)/updates/kernel/drivers/qdma
	dev_install_path ?= /usr/local/include/qdma
	docs_install_path ?= /usr/share/man/man8
else # bundled install
	kernel_install_path ?= $(install_path)/modules
	dev_install_path ?= $(install_path)/include/qdma
	docs_install_path ?= $(install_path)/doc
endif

$(shell rm -f $(srcdir)/src/libqdma; ln -fs $(srcdir)/libqdma $(srcdir)/src;)

.PHONY: eval.mak

.PHONY: default
default: mod_pf mod_vf post

.PHONY: pf 
pf: mod_pf

.PHONY: vf 
vf: mod_vf

.PHONY: mods
mod: mod_pf mod_vf 

.PHONY: mod_pf
mod_pf:
	@if [ -n "$(verbose)" ]; then \
	   echo "#######################";\
	   printf "#### PF %-8s%5s####\n" $(DRIVER_SRC_DIR);\
	   echo "#######################";\
	 fi;
	@srcdir=$(shell pwd)/$(DRIVER_SRC_DIR) $(MAKE) VF=0 -C $(DRIVER_SRC_DIR);

.PHONY: mod_vf
mod_vf:
	@if [ -n "$(verbose)" ]; then \
	   echo "#######################";\
	   printf "#### VF %-8s%5s####\n" $(DRIVER_SRC_DIR);\
	   echo "#######################";\
	 fi;
	@srcdir=$(shell pwd)/$(DRIVER_SRC_DIR) $(MAKE) VF=1 -C $(DRIVER_SRC_DIR);

.PHONY: post
post:
	@if [ -n "$(post_msg)" ]; then \
	   echo -e "\nWARNING:\n $(post_msg)";\
	 fi;

.PHONY: clean
clean:
	@for dir in $(ALLSUBDIRS); do \
	   echo "#######################";\
	   printf "####  %-8s%5s####\n" $$dir;\
	   echo "#######################";\
	  srcdir=$(shell pwd)/$$dir $(MAKE) -C $$dir clean;\
	done;
	@-/bin/rm -f *.symvers eval.mak 2>/dev/null;
	@-/bin/rm -rf $(shell pwd)/libqdma/qdma_access/*.o $(shell pwd)/libqdma/qdma_access/.*.o.* $(shell pwd)/libqdma/qdma_access/.*.cmd $(shell pwd)/libqdma/qdma_access/.*.o.cmd
	@-/bin/rm -f $(shell pwd)/src/libqdma

.PHONY: install-mods
install-mods:
	@echo "installing kernel modules to $(kernel_install_path) ..."
	@mkdir -p -m 755 $(kernel_install_path)
	@install -v -m 644 $(bin_dir)/*.ko $(kernel_install_path)
	@depmod -a || true

.PHONY: uninstall-mods
uninstall-mods:
	@echo "Un-installing $(kernel_install_path) ..."
	@/bin/rm -rf $(kernel_install_path)/*
	@depmod -a

.PHONY: help
help:
	@echo "Build Targets:";\
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

