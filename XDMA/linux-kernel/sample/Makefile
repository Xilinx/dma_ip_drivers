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

include $(TOPDIR)/Makefile.inc

APP_TSNV1_XDMA_OBJS = receiver_thread.o  sender_thread.o  stats_thread.o  buffer_handler.o tsn-app.o 
APP_TSNV1_XDMA_OBJS += 

OBJS = $(APP_TSNV1_XDMA_OBJS)

LIBS = $(TOPDIR)/libxdma/libxdma.a $(TOPDIR)/libcom/libcommon.a
LIBS_INCLUDE = -L.. -L../libxdma -L../libcom -lcommon -lxdma -lpthread -lreadline -lm

CPPFLAGS += -I.. -I../include/libcom -D_GNU_SOURCE
CFLAGS += -I.. -I../include/libcom -D_GNU_SOURCE

all: tsn-app

tsn-app: .depend $(LIBS) $(APP_TSNV1_XDMA_OBJS)
	$(CC) $(CFLAGS) $(APP_TSNV1_XDMA_OBJS) -o $@ $(LIBS_INCLUDE)

clean:
	rm -rf *.o *~ core .depend
	rm -rf tsn-app 

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
