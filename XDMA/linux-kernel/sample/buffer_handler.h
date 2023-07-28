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

#ifndef __BUFFER_HANDLER_H__
#define __BUFFER_HANDLER_H__

#include "xdma_common.h"

void relese_buffers(int count);

int buffer_pool_free(BUF_POINTER element);
BUF_POINTER buffer_pool_alloc();
int initialize_buffer_allocation();

void buffer_release();

#endif     // __BUFFER_HANDLER_H__
