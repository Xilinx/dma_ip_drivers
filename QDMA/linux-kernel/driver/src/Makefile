SHELL = /bin/bash

export topdir
export bin_dir

# The top-level makefile defines required variables and flags.
ifneq ($(shell [[ $(MAKELEVEL) -ge 1 ]] && echo 1),1)
  $(error Please use the top-level Makefile to build this driver)
endif

ifneq ($(wildcard /etc/lsb-release),)
  ifneq ($(shell $(grep) "Ubuntu" /etc/lsb-release),)
    FLAGS += -DUBUNTU_VERSION_CODE
  endif
endif

include $(srcdir)/../make_rules/common_flags.mk

$(info srcdir = $(srcdir).)
$(info KSRC = $(KSRC).)
$(info VF = $(VF).)
$(info CROSS_COMPILE_FLAG = $(CROSS_COMPILE_FLAG).)
$(info ARCH = $(ARCH).)


MOD_NAME := qdma$(PFVF_TYPE)

EXTRA_CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall -Werror
EXTRA_CFLAGS += $(FLAGS) $(CPPFLAGS) $(EXTRA_FLAGS)
EXTRA_CFLAGS += -I$(srcdir)/../include
EXTRA_CFLAGS += -I$(KSRC)/../include
EXTRA_CFLAGS += -I$(srcdir)/../libqdma/qdma_access/qdma_soft_access
EXTRA_CFLAGS += -I$(srcdir)/../libqdma/qdma_access/eqdma_soft_access
EXTRA_CFLAGS += -I$(srcdir)/../libqdma/qdma_access/qdma_s80_hard_access
EXTRA_CFLAGS += -I.

# linux >= 3.13 genl_ops become part of the genl_family. And although
# genl_register_family_with_ops() is still retained until kernel 4.10,
# its prototype changed from a extern function to a define.
#
ifneq ($(shell $(grep) -c 'int genl_register_family_with_ops' $(KINC)/net/genetlink.h),0)
  ccflags-y += -D__GENL_REG_FAMILY_OPS_FUNC__
endif

# linux <= 3.13 pci_msix_vec_count is not defined.
#
ifeq ($(shell $(grep) -c 'int pci_msix_vec_count' $(KINC)/linux/pci.h),0)
  ccflags-y += -D__PCI_MSI_VEC_COUNT__
endif

# linux < 3.13 list_next_entry is not defined.
#
ifeq ($(shell $(grep) -c 'list_next_entry' $(KINC)/linux/list.h),0)
  ccflags-y += -D__LIST_NEXT_ENTRY__
endif

# linux < 3.18.13 READ_ONCE is not defined.
#
ifneq ($(shell $(grep) -c 'READ_ONCE' $(KINC)/linux/compiler.h),0)
  ccflags-y += -D__READ_ONCE_DEFINED__
endif

$(info EXTRA_FLAGS = $(EXTRA_FLAGS).)
$(info ccflags-y = $(ccflags-y).)
#EXTRA_CFLAGS += -DDEBUG

#For Kernel Images > 4.9.199 with CONFIG_STACK_VALIDATION=y and GCC version > 8 compiler throws spurious warnings
#related to sibling calls and frame pointer save/setup. To supress these warnings
#enable the OBJECT_FILES_NON_STANDARD option below
#If in case, the flag name changes in future linux kernel releases, please refer
#https://github.com/torvalds/linux/blob/master/tools/objtool/Documentation/stack-validation.txt
#to get the correct flag name
#OBJECT_FILES_NON_STANDARD := y

ifneq ($(modulesymfile),)
  override symverfile = symverfile="$(topdir)/$(modulesymfile) \
					-o $(drvdir)/$(modulesymfile)"
else
  override symverfile =
endif

DRV_OBJS := nl.o cdev.o qdma_mod.o

LIBQDMA_OBJS := libqdma/qdma_mbox.o libqdma/qdma_intr.o libqdma/qdma_st_c2h.o \
	libqdma/qdma_thread.o libqdma/libqdma_export.o libqdma/qdma_context.o \
	libqdma/qdma_sriov.o libqdma/qdma_platform.o libqdma/qdma_descq.o libqdma/qdma_regs.o \
	libqdma/qdma_debugfs.o libqdma/qdma_debugfs_dev.o libqdma/qdma_debugfs_queue.o \
	libqdma/libqdma_config.o libqdma/qdma_device.o libqdma/xdev.o libqdma/thread.o

QDMA_ACCESS_OBJS := libqdma/qdma_access/qdma_mbox_protocol.o libqdma/qdma_access/qdma_list.o \
	libqdma/qdma_access/qdma_access_common.o libqdma/qdma_access/qdma_resource_mgmt.o \
	libqdma/qdma_access/qdma_s80_hard_access/qdma_s80_hard_access.o \
	libqdma/qdma_access/qdma_s80_hard_access/qdma_s80_hard_reg_dump.o \
	libqdma/qdma_access/qdma_soft_access/qdma_soft_access.o \
	libqdma/qdma_access/eqdma_soft_access/eqdma_soft_access.o \
	libqdma/qdma_access/eqdma_soft_access/eqdma_soft_reg_dump.o



obj-m += $(MOD_NAME).o
$(MOD_NAME)-objs += $(DRV_OBJS)
$(MOD_NAME)-objs += $(LIBQDMA_OBJS)
$(MOD_NAME)-objs += $(QDMA_ACCESS_OBJS)

ifeq ($(ARCH),aarch64)
    ARCH=arm64
endif

.PHONY: bin
bin:
	@mkdir -p -m 755 $(bin_dir)
	@rm -f $(MOD_NAME).ko $(bin_dir)/$(MOD_NAME).ko
ifeq ($(CROSS_COMPILE_FLAG),)
	@$(MAKE) symverfile=$(symverfile) KBUILD_EXTRA_SYMBOLS=$(extra_symb) -C $(KOBJ) M=$(shell pwd) modules
else
	@$(MAKE) symverfile=$(symverfile) KBUILD_EXTRA_SYMBOLS=$(extra_symb) -C $(KOBJ)  CROSS_COMPILE=$(CROSS_COMPILE_FLAG) SUBDIRS=$(shell pwd) modules
endif
	@cp $(MOD_NAME).ko $(bin_dir)

.PHONY: clean
clean:
	@-/bin/rm -rf *.ko* ?odule* .tmp_versions *.mod.* *.o *.o.* .*.o.* .*.cmd
	@-/bin/rm -rf */*.o */*.o.* */.*.o.* */.*.cmd
	@-/bin/rm -rf $(bin_dir)/*.ko
	@-/bin/rm -f $(srcdir)/drv/libqdma
