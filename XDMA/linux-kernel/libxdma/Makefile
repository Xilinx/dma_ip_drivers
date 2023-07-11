#
# TSN v1 PCIe XDMA 
# -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
#

CROSS_COMPILE =

AS  = $(CROSS_COMPILE)as
LD  = $(CROSS_COMPILE)ld
CC  = $(CROSS_COMPILE)gcc
CPP = $(CC) -E
AR  = $(CROSS_COMPILE)ar
NM  = $(CROSS_COMPILE)nm
STRIP   = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB  = $(CROSS_COMPILE)ranlib

LIB = libxdma.a

OBJS    = api_xdma.o

INCLUDES =

CFLAGS = -Wall -Werror -I./ -I../ -I../include/ -I../xdma/

all:    $(LIB) $(OBJS)

$(LIB): .depend $(OBJS)
		$(AR) crv $@ $(OBJS)

#########################################################################

.depend:    Makefile $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(INCLUDES) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################

clean :
	rm -rf *.o *.a
