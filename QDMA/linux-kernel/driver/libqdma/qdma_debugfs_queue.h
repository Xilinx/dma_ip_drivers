/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#ifndef __QDMA_DEBUGFS_QUEUE_H__
#define __QDMA_DEBUGFS_QUEUE_H__

#include "qdma_debugfs.h"
#include "qdma_descq.h"


int dbgfs_queue_init(struct qdma_queue_conf *conf,
		struct qdma_descq *pairq,
		struct dentry *dbgfs_queues_root);
void dbgfs_queue_exit(struct qdma_queue_conf *conf,
		struct qdma_descq *pairq);

#endif
