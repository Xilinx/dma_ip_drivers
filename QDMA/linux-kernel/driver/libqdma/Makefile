SHELL = /bin/bash

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

EXTRA_CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall -Werror
EXTRA_CFLAGS += $(FLAGS) $(CPPFLAGS)
EXTRA_CFLAGS += -I$(srcdir)/../include
EXTRA_CFLAGS += -I$(KSRC)/../include
EXTRA_CFLAGS += -I.

#EXTRA_CFLAGS += -DDEBUG

ifneq ($(modulesymfile),)
  override symverfile = symverfile="$(topdir)/$(modulesymfile) \
					-o $(drvdir)/$(modulesymfile)"
else
  override symverfile =
endif

MOD_NAME := libqdma
ifneq ($(SUBDIRS),)
  BASE_OBJS := $(patsubst $(SUBDIRS)/%.c,%.o,$(wildcard $(SUBDIRS)/*.c))
endif
obj-m := $(MOD_NAME).o
$(MOD_NAME)-objs := $(BASE_OBJS)

.PHONY: build
build:
	@$(MAKE) symverfile=$(symverfile) KBUILD_EXTRA_SYMBOLS=$(extra_symb) -C $(KOBJ) SUBDIRS=$(shell pwd) modules

.PHONY: clean
clean:
	@-/bin/rm -rf *.ko* ?odule* .tmp_versions *.mod.* *.o .*.o.* .*.cmd
