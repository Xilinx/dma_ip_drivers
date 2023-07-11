#
# DLA-VVTB : Deep Learning Accelerator - Vulnerability Verification Test Board
# -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
#

LIB	= libcommon.a

OBJS	= common.o lib_menu.o helper.o log.o

INCLUDES = 

CFLAGS = -I./ -I../ -I../include/ -I../include/libcom 

all:	$(LIB) $(OBJS)

$(LIB):	.depend $(OBJS)
	$(AR) crv $@ $(OBJS)

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(INCLUDES) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################

clean :
	rm -rf *.o *.a
