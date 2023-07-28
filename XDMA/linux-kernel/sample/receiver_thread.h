/*
* TSNv1 XDMA :
* -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
*/

#ifndef __RECEIVER_THREAD_H__
#define __RECEIVER_THREAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "xdma_common.h"
#include "buffer_handler.h"

#include "../libxdma/api_xdma.h"

void xbuffer_enqueue(QueueElement element);
QueueElement xbuffer_dequeue();
int getQueueCount();

void initialize_statistics(stats_t* p_stats);

#endif    // __RECEIVER_THREAD_H__
