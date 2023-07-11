#
# TSNv1 XDMA : 
# -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
#

HOSTARCH := $(shell uname -m | \
	sed -e s/i.86/i386/ \
	    -e s/sun4u/sparc64/ \
	    -e s/arm.*/arm/ \
	    -e s/sa110/arm/ \
	    -e s/macppc/ppc/)

HOSTOS := $(shell uname -s | tr A-Z a-z)

CROSS_COMPILE =
export	CROSS_COMPILE HOSTARCH

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export	TOPDIR
SRCDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export	SRCDIR


REVISION = $(shell cat ./include/version.h | grep VERSION_STRING | awk '{print $$4}' | sed -e s/version-//g | sed -e s/\"//g)
BUILDATE = $(shell cat ./include/version.h | grep TSNA_BUILD_DATE | awk '{print $$3}' | sed -e s/0x//g | sed -e s/\r//g)

REV = "tsn-app-$(REVISION)-$(BUILDATE)"

include $(TOPDIR)/Makefile.inc

SUBDIRS	= libcom \
		  libxdma \
		  sample

SRCDIRS	= include \
	include/libcom \
	sample \
	libxdma \
	libcom

CLEANDIR = $(SUBDIRS) \
	xdma \


#########################################################################
# objects....order is important (i.e. start must be first)

OBJS  =	libcom/libcom.a libxdma/libxdma.a

EXE = tsn-app

all: $(EXE)

install: all

driver:	depend subdirs $(OBJS) $(LDSCRIPT)
	$(MAKE) -C driver

$(EXE):
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir || exit 1 ; done

subdirs:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir || exit 1 ; done

depend dep:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir .depend ; done

touch:
	@for dir in $(SRCDIRS) ; do touch $$dir/* ; done

clean:
	rm -f `find . -type f \
		\( -name 'core' -o -name '*.bak' -o -name '*~' \
		-o -name '*.o'  -o -name '*.a' -o -name '.depend' \) -print`
	rm -rf release
	@for d in $(CLEANDIR); do $(MAKE) -C $$d clean; done

