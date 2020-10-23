/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
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

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "xvsec_util.h"

int xvsec_util_find_file_type(char *fname, const char *suffix)
{
	size_t	suffix_len;
	size_t	fname_len;
	int	ret = -(EINVAL);
	char	*fname_suffix;

	/* Parameter Validation */
	if ((fname == NULL) || (suffix == NULL))
		return ret;

	suffix_len = strlen(suffix);
	if (suffix_len == 0)
		return ret;
	fname_len = strlen(fname);

	fname_suffix = fname+(fname_len - suffix_len);
	if (strncasecmp(fname_suffix, suffix, suffix_len) == 0)
		ret = 0;

	return ret;
}

struct file *xvsec_util_fopen(const char *path, int flags, int rights)
{
	struct file *filep;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filep = filp_open(path, (flags | O_LARGEFILE), rights);
	set_fs(oldfs);
	if (IS_ERR(filep) != 0) {
		err = PTR_ERR(filep);
		pr_err("%s : filp_open failed, err : 0x%X\n", __func__, err);
		return NULL;
	}
	return filep;
}

void xvsec_util_fclose(struct file *filep)
{
	filp_close(filep, NULL);
}

int xvsec_util_fread(struct file *filep, uint64_t offset,
	uint8_t *data, uint32_t size)
{
	int ret = 0;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(filep, (char __user *)data, size, (loff_t *)&offset);
	filep->f_pos = offset;
	set_fs(oldfs);

	if (ret < 0) {
		pr_err("%s : vfs_read failed with error : %d\n", __func__, ret);
		return -(EIO);
	}

	return ret;
}

int xvsec_util_fwrite(struct file *filep, uint64_t offset,
	uint8_t *data, uint32_t size)
{
	int ret = 0;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_write(filep, (char __user *)data, size, (loff_t *)&offset);
	filep->f_pos = offset;
	set_fs(oldfs);

	if (ret < 0) {
		pr_err("%s : vfs_write failed, err : %d\n", __func__, ret);
		return -(EIO);
	}

	return ret;
}

int xvsec_util_fsync(struct file *filep)
{
	vfs_fsync(filep, 0);
	return 0;
}

int xvsec_util_get_file_size(const char *fname, loff_t *size)
{
	int ret = 0;
	mm_segment_t oldfs;
	struct kstat stat;

	oldfs = get_fs();
	set_fs(get_ds());

	memset(&stat, 0, sizeof(struct kstat));
	ret = vfs_stat((char __user *)fname, &stat);
	set_fs(oldfs);
	if (ret < 0) {
		pr_err("%s : vfs_stat failed with error : %d\n", __func__, ret);
		return -(EIO);
	}

	*size = stat.size;

	return ret;
}

