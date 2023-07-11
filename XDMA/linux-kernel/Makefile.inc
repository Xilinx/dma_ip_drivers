#
#
# TSNv1-XDMA : 
# -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
#

PLATFORM_RELFLAGS=
PLATFORM_CPPFLAGS=

KERNELDIR = /usr/src/linux-headers-$(shell uname -r)

CONFIG_SHELL	:= $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
		    else if [ -x /bin/bash ]; then echo /bin/bash; \
		    else echo sh; fi ; fi)

HOSTCC		= gcc
HOSTCFLAGS	= -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer

#
# Include the make variables (CC, etc...)
#
AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB	= $(CROSS_COMPILE)ranlib

RM	= rm
ECHO	= echo

RELFLAGS= $(PLATFORM_RELFLAGS)
DBGFLAGS= -g #-DDEBUG
OPTFLAGS= -Os
CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) 	\
	-I$(TOPDIR)/include
CFLAGS := $(CPPFLAGS) -Wall -I $(TOPDIR)/include 
AFLAGS := -D__ASSEMBLY__ $(CPPFLAGS)

LDFLAGS += -Bstatic -T $(LDSCRIPT) -Ttext $(TEXT_BASE)

KFLAGS = -D__KERNEL__ -DMODULE -I $(KERNELDIR)/include/ -I $(TOPDIR)/include -O2 -Wall -Wno-unused -fno-strict-aliasing

#########################################################################

export	CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE \
	AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP \
	MAKE
export	CPPFLAGS CFLAGS AFLAGS

#########################################################################

%.s:	%.S
	$(CPP) $(AFLAGS) -o $@ $<
%.o:	%.S
	$(CC) $(AFLAGS) -c -o $@ $<
%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

#########################################################################
