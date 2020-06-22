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

#ifndef __QDMA_DEBUGFS_H__
#define __QDMA_DEBUGFS_H__

#include <linux/pci.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define DBGFS_DBG_FNAME_SZ  (64)
#define QDMA_DEV_NAME_SZ	(64)

/*****************************************************************************/
/**
 * qdma_debugfs_init() - function to initialize debugfs
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_debugfs_init(struct dentry **qdma_debugfs_root);

/*****************************************************************************/
/**
 * qdma_debugfs_exit() - function to cleanup debugfs
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
void qdma_debugfs_exit(struct dentry *qdma_debugfs_root);

#endif
