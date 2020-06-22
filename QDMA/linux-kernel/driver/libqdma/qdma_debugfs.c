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

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_debugfs.h"

/*****************************************************************************/
/**
 * qdma_debugfs_init() - function to initialize debugfs
 *
 * param[in]: qdma_debugfs_root - debugfs root
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_debugfs_init(struct dentry **qdma_debugfs_root)
{
	struct dentry *debugfs_root = NULL;
	/* create a directory by the name qdma in
	 * /sys/kernel/debugfs
	 */
#ifndef __QDMA_VF__
	debugfs_root = debugfs_create_dir("qdma-pf", NULL);
	if (!debugfs_root)
		return -ENOENT;
	pr_debug("created qdma-pf dir in Linux debug file system\n");

#else
	debugfs_root = debugfs_create_dir("qdma-vf", NULL);
	if (!debugfs_root)
		return -ENOENT;
	pr_debug("created qdma-vf dir in Linux debug file system\n");

#endif

	*qdma_debugfs_root = debugfs_root;
	return 0;
}

/*****************************************************************************/
/**
 * qdma_debugfs_exit() - function to cleanup debugfs
 *
 * param[in]: qdma_debugfs_root - debugfs root
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
void qdma_debugfs_exit(struct dentry *qdma_debugfs_root)
{
	debugfs_remove_recursive(qdma_debugfs_root);
#ifndef __QDMA_VF__
	pr_debug("removed qdma_pf directory from Linux debug file system\n");
#else
	pr_debug("removed qdma_vf directory from Linux debug file system\n");
#endif
}
