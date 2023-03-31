#/*
# * This file is part of the Xilinx DMA IP Core driver for Linux
# *
# * Copyright (c) 2017-2022, Xilinx, Inc. All rights reserved.
# * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
# *
# * This source code is free software; you can redistribute it and/or modify it
# * under the terms and conditions of the GNU General Public License,
# * version 2, as published by the Free Software Foundation.
# *
# * This program is distributed in the hope that it will be useful, but WITHOUT
# * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# * more details.
# *
# * The full GNU General Public License is included in this distribution in
# * the file called "COPYING".
# */

# Kernel directories.
KERNELRELEASE := $(shell uname -r)

# If KSRC=<path> is specified on the command line, KOBJ=<path> must
# also be specified. This is to avoid mixups if the kernel object path
# differs from the source path. A shortcut (KSRC=KOBJ) is to use KDIR.
ifeq ($(KDIR),)
  ifeq ($(KSRC),)
    ifneq ($(KOBJ),)
      $(warning When using KOBJ=<path>, the KSRC=<path> must also be defined.)
      $(warning Use KDIR=<path> when KSRC and KOBJ are the same.)
      $(error ERROR: kernel source path not specified)
    endif
  else
    ifeq ($(KOBJ),)
      $(warning When using KSRC=<path>, the KOBJ=<path> must also be defined.)
      $(warning Use KDIR=<path> when KSRC and KOBJ are the same.)
      $(error ERROR: KOBJ path not specified)
    endif
  endif
else
  override KSRC := $(KDIR)
  override KOBJ := $(KDIR)
endif

# Only if KSRC/KOBJ were not defined on the command line.
KSRC ?= $(wildcard /lib/modules/$(KERNELRELEASE)/source)
KOBJ ?= $(wildcard /lib/modules/$(KERNELRELEASE)/build)
KINC  = $(KSRC)/include

#$(warning KDIR: $(KDIR).)

ifeq ($(KSRC),)
   KSRC = $(KOBJ)
endif

# Define kernel files.
VERSION_H  := $(KOBJ)/include/linux/version.h
AUTOCONF_H := $(KOBJ)/include/generated/autoconf.h
UTSRELEASE_H  := $(KOBJ)/include/generated/utsrelease.h

ifeq ($(wildcard $(VERSION_H)),)
  VERSION_H := $(KOBJ)/include/generated/uapi/linux/version.h
endif
ifeq ($(wildcard $(AUTOCONF_H)),)
  AUTOCONF_H := $(KOBJ)/include/linux/autoconf.h
endif
ifeq ($(wildcard $(UTSRELEASE_H)),)
  UTSRELEASE_H := $(KOBJ)/include/linux/utsrelease.h
endif
#ifeq ($(wildcard $(UTSRELEASE_H)),)
#  $(error NO utsrelease)
#endif

# Define architecture and target(for RPM).
ARCH := $(shell uname -m)
target := $(ARCH)
override ARCH := $(shell echo $(ARCH) | sed 's/i.86/i386/')
ifeq ($(USER_ARCH),)

  ifeq ($(ARCH),ppc64le)
    ifeq ($(wildcard $(KOBJ)/arch/$(ARCH)),)
      override ARCH := powerpc
    endif
  endif

  ifeq ($(ARCH),ppc64)
    # Check if the kernel wants ppc64 or powerpc.
    ifeq ($(wildcard $(KOBJ)/arch/$(ARCH)),)
      override ARCH := powerpc
    endif
  endif
else
  # Honor the value of ARCH if specified by user.
  override ARCH := $(USER_ARCH)
endif

# Functions.
define path_check
$(if $(wildcard $(1)),$(1),)
endef
define reverse_sort
$(shell echo -e `echo "$(strip $(1))" |\
                 sed 's/[[:space:]]/\\\n/g'` | sort -r)
endef
define version_code
$(shell let x=`sed '/^\#define[[:space:]]*LINUX_VERSION_CODE/!d;\
                    s/.*LINUX_VERSION_CODE[[:space:]]*//' < $(1)\
               2>/dev/null`;\
        let a="$$x >> 16";\
        let x="$$x - ($$a << 16)";\
        let b="$$x >> 8";\
        let x="$$x - ($$b << 8)";\
        echo "$$a $$b $$x")
endef

# Checks for kernel source and object directories.
ifeq ($(call path_check,$(KSRC)),)
  $(warning Be sure the kernel source is properly installed or \
            try specifying the kernel source tree using 'make KSRC=<path>')
  $(error ERROR: missing kernel source)
endif
ifeq ($(call path_check,$(KOBJ)),)
  $(warning Try specifying the kernel build tree using 'make KOBJ=<path>'.)
  $(error ERROR: missing kernel build)
endif

# Check kernel source and build directories are somewhat likely to be correct.
ifneq ($(notdir $(wildcard $(KSRC)/Makefile)),Makefile)
  $(warning There seems to be a problem with the kernel \
            source [$(KSRC)] directory.)
  $(error ERROR: missing kernel Makefile)
endif
ifneq ($(notdir $(wildcard $(KOBJ)/Makefile)),Makefile)
  $(warning There seems to be a problem with the kernel \
            build [$(KOBJ)] directory.)
  $(error ERROR: missing kernel Makefile)
endif

# Get kernel version code info.
KERNELVERSION := $(strip $(call version_code,$(VERSION_H)))
ifneq ($(words $(KERNELVERSION)), 3)
  $(error ERROR: unexpected kernel version \
          '$(shell echo $(KERNELVERSION) | sed 's/[[:space:]]/./g')')
endif

# Define kernel version details.
kversion       := $(word 1, $(KERNELVERSION))
kpatchlevel    := $(word 2, $(KERNELVERSION))
ksublevel      := $(word 3, $(KERNELVERSION))

# The kernel base version, excluding the EXTRAVERSION string.
kbaseversion   := $(kversion).$(kpatchlevel).$(ksublevel)

# The kernel series version.
kseries        := $(kversion).$(kpatchlevel)

# Fix for variation of Module.symvers naming (thanks 2.6.17!).
# I need to know the file name of the module symver generated by the kernel
# during an external module build (MODPOST). Also used for kernels that don't
# automatically generate the module symver file during MODPOST (2.6.0-2.6.17?).
ifeq ($(shell $(grep) -c '^modulesymfile[[:space:]]*:\?=' \
                $(KSRC)/scripts/Makefile.modpost),1)
  modulesymfile := $(shell $(grep) '^modulesymfile[[:space:]]*:\?=' \
                             $(KSRC)/scripts/Makefile.modpost)
  kernelsymfile := $(shell $(grep) '^kernelsymfile[[:space:]]*:\?=' \
                             $(KSRC)/scripts/Makefile.modpost)
else
  ifeq ($(shell $(grep) -c '^symverfile[[:space:]]*:\?=' \
                $(KSRC)/scripts/Makefile.modpost),1)
    symverfile    := $(shell $(grep) '^symverfile[[:space:]]*:\?=' \
                             $(KSRC)/scripts/Makefile.modpost)
    kernelsymfile := $(subst symverfile,kernelsymfile,$(symverfile))
  endif
endif
modulesymfile ?= $(symverfile)
ifeq ($(modulesymfile),)
  $(warning The parsing of $(KSRC)/scripts/Makefile.modpost \
            is not making sense.)
  $(error ERROR cannot determine module symvers file)
endif

# Gnu make (3.80) bug #1516, $(eval ...) inside conditionals causes errors.
# This is fixed in v3.81 and some v3.80 (RHEL4/5) but not on SLES10.
# Workaround: include a separate makefile that does the eval.
ifeq ($(shell echo '$(modulesymfile)' | $(grep) -c '^[[:alnum:]_]\+[[:space:]]*:\?=[[:space:]]*.\+'),1)
  $(shell echo '$$(eval $$(modulesymfile))' > eval.mak)
  include eval.mak
else
  modulesymfile =
endif
ifeq ($(shell echo '$(kernelsymfile)' | $(grep) -c '^[[:alnum:]_]\+[[:space:]]*:\?=[[:space:]]*.\+'),1)
  $(shell echo '$$(eval $$(kernelsymfile))' > eval.mak)
  include eval.mak
else
  kernelsymfile =
endif
modulesymfile := $(notdir $(modulesymfile))
kernelsymfile := $(notdir $(kernelsymfile))
$(shell [ -f eval.mak ] && /bin/rm -f eval.mak)

ifneq ($(words $(modulesymfile)),1)
  $(warning The parsing of $(KSRC)/scripts/Makefile.modpost \
            is not making sense.)
  $(warning You can try passing 'modulesymfile=Module.symvers' or \
            similar to make.)
  $(error ERROR cannot determine module symvers file)
endif


# Check for configured kernel.
ifeq ($(wildcard $(AUTOCONF_H)),)
  $(warning The kernel is not properly configured, try running \
            'make menuconfig' on your kernel.)
  $(error ERROR: kernel missing autoconf.h)
endif
# Check for built kernel.
ifeq ($(wildcard $(VERSION_H)),)
  $(warning The kernel has not been compiled. Try building your kernel \
            before building this driver.)
  $(error ERROR: kernel missing version.h)
endif

# Check that kernel supports modules.
ifneq ($(shell $(grep) -c '^\#define[[:space:]]\+CONFIG_MODULES[[:space:]]\+1' $(AUTOCONF_H)),1)
  $(warning The kernel has not been configured for module support.)
  $(warning Try configuring the kernel to allow external modules and \
            recompile.)
  $(error ERROR: kernel CONFIG_MODULES not defined)
endif

# Get kernel UTS_RELEASE info.
ifneq ($(wildcard $(UTSRELEASE_H)),)
  ifneq ($(shell $(grep) -c '^\#define[[:space:]]\+UTS_RELEASE' \
                  $(UTSRELEASE_H)),0)
    utsrelease := $(UTSRELEASE_H)
  endif
else
  ifneq ($(wildcard $(KOBJ)/include/linux/version.h),)
    ifneq ($(shell $(grep) -c '^\#define[[:space:]]\+UTS_RELEASE' \
                    $(KOBJ)/include/linux/version.h),0)
      utsrelease := $(KOBJ)/include/linux/version.h
    endif
  endif
endif
ifeq ($(utsrelease),)
  $(error ERROR: cannot locate kernel UTS_RELEASE)
endif
# Getting the UTS_RELEASE on RHEL3 had problems due to the multiple defines
# within the file. I can run this file through the C pre-processor and get 
# the actual UTS_RELEASE definition. This has only been tested on gcc, other
# compilers may not work.
utsrelease := $(strip $(shell $(CC) -E -dM -I $(KSRC)/include $(utsrelease) \
                        2>/dev/null| sed '/^\#define[[:space:]]*UTS_RELEASE/!d;\
		                          s/^\#define UTS_RELEASE[[:space:]]*"//;\
					  s/"//g'))

# The kernel local version string if defined in config.
klocalversion  := $(shell sed '/^CONFIG_LOCALVERSION=/!d;\
                               s/^CONFIG_LOCALVERSION="//;s/"//g'\
                            2>/dev/null < $(KOBJ)/.config)
# The complete kernel EXTRAVERSION string.
kextraversion  := $(subst $(kbaseversion),,$(utsrelease))
# The full kernel version should be the same as uts_release.
kernelversion  := $(utsrelease)

# The kernel EXTRAVERSION creates a unique problem, especially since
# kernel versioning extended into the EXTRAVERSION and distributions 
# add strings such as smp, largesmp, xen, etc or when additional minor
# version numbers are appended.
# Some code that we supply is dependent on the kernel version and
# parts of the EXTRAVERSION, but not dependent on some of the additional
# flags. This requires that I have a list of kernel version strings that
# could map to the source version we require. For example, if the
# kernel version is 2.6.9-67.ELsmp, we only care about the "2.6.9-67"
# part, therefore, I need to split the EXTRAVERSION accordingly.
# Another problem is when a user builds their own kernel, say 2.6.21.4
# and adds an additional string to EXTRAVERSION. The EXTRAVERSION is
# now ".4-custom" and I have to parse this with hopes of extracting
# only the ".4" part, resulting in the needed "2.6.21.4" version.
# Adding a BUGFIX version (int) field would be very helpfull!

# EXTRAVERSION as defined only in the Makefile.
extraversion1 := $(strip $(shell sed '/^EXTRAVERSION/!d;\
                                      s/^EXTRAVERSION[[:space:]]*=//;s/"//g'\
                                    < $(KSRC)/Makefile 2>/dev/null))
# SLES9 likes to put make code in their EXTRAVERSION define. Let the
# variables expand out to nothing, because the code will cause problems.
extraversion1 := $(shell echo $(extraversion1))
# EXTRAVERSION without local version.
extraversion2 := $(strip $(subst $(klocalversion),,$(kextraversion)))
# EXTRAVERSION with only the kernel .version (hopefully).
extraversion3 := $(strip $(shell echo $(kextraversion) |\
				  sed 's/\(^\.[0-9]*\).*/\1/'))
# EXTRAVERSION without the Redhat EL tag.
extraversion4 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\.EL.*//i'))
# EXTRAVERSION with the Redhat EL tag, but nothing else after.
extraversion5 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\(\.EL\).*/\1/i'))
# EXTRAVERSION with the Redhat EL tag, including a number (el5).
extraversion6 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\(\.EL[[:digit:]]*\).*/\1/i'))
# EXTRAVERSION without the Redhat hotfix/update kernel version number.
extraversion7 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\(.*\-[[:digit:]]*\)\..*\(\.EL\).*/\1\2/i'))
# EXTRAVERSION without the Redhat hotfix/update kernel version number with Redhat EL tag, including the number (el5).
extraversion8 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\(.*\-[[:digit:]]*\)\..*\(\.EL[[:digit:]]\).*/\1\2/i'))
# EXTRAVERSION with only the RHEL distro version
extraversion9 := $(strip $(shell echo $(kextraversion) |\
			          sed 's/\(.*\-[[:digit:]]*\)\..*\.EL[[:digit:]].*/\1/i'))

# All known EXTRAVERSION strings, duplicates removed.
extraversions  := $(strip $(sort $(kextraversion) \
			         $(extraversion1) \
			         $(extraversion2) \
			         $(extraversion3) \
			         $(extraversion4) \
			         $(extraversion5) \
			         $(extraversion6) \
			         $(extraversion7) \
			         $(extraversion8) \
			         $(extraversion9)))

# List of all possible kernel version names for target kernel.
all_kernels    := $(sort $(kbaseversion) \
		         $(foreach a,$(extraversions),$(kbaseversion)$(a)))

# A reverse ordered list. This is used primarily to search source code
# directory names to match the target kernel version.
kversions := $(call reverse_sort, $(all_kernels))

# Special cases for 2.4 series kernels.
ifeq ($(kseries),2.4)
  $(error ERROR:  2.4 kernel NOT supported.)
endif
# Note: Define only FLAGS here. These will convert to CFLAGS in the sub-make.
# If the environment variable FLAGS is defined with make, things will break,
# use CFLAGS instead.
# General compiler flags.
# Kernel version 3.3+ moved include/asm to include/generated, which breaks
# outbox kernel drivers. Fix to include the new generated without code change.
ifeq ($(kversion),3)
  ifeq ($(shell [ $(kpatchlevel) -gt 2 ] && echo 1),1)
    override CARCH := $(ARCH)
    ifeq ($(CARCH),x86_64)
      override CARCH := x86
    endif
    FLAGS += -I$(KSRC)/arch/$(CARCH)/include/generated
  endif
endif
