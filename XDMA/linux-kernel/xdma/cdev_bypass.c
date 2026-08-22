/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
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

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "libxdma_api.h"
#include "xdma_cdev.h"

#define write_register(v, mem, off) iowrite32(v, mem)

/*
 * Copy a transfer's descriptors into the kernel staging buffer kbuf. Runs under
 * engine->lock, so it must not touch userspace (copy_to_user can fault and
 * sleep, which is illegal while holding a spinlock). The caller copies kbuf out
 * to userspace after dropping the lock.
 */
static int copy_desc_data(struct xdma_transfer *transfer, char *kbuf,
		size_t *buf_offset, size_t buf_size)
{
	int i;
	int rc = 0;

	if (!kbuf) {
		pr_err("Invalid kernel buffer\n");
		return -EINVAL;
	}

	if (!buf_offset) {
		pr_err("Invalid user buffer offset\n");
		return -EINVAL;
	}

	/* Stage descriptor data into the kernel buffer */
	for (i = 0; i < transfer->desc_num; i++) {
		if (*buf_offset + sizeof(struct xdma_desc) <= buf_size) {
			memcpy(kbuf + *buf_offset, transfer->desc_virt + i,
				sizeof(struct xdma_desc));
			*buf_offset += sizeof(struct xdma_desc);
		} else {
			rc = -ENOMEM;
		}
	}

	return rc;
}

static ssize_t char_bypass_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	struct xdma_dev *xdev;
	struct xdma_engine *engine;
	struct xdma_cdev *xcdev = (struct xdma_cdev *)file->private_data;
	struct xdma_transfer *transfer;
	struct list_head *idx;
	size_t buf_offset = 0;
	char *kbuf;
	int rc = 0;

	rc = xcdev_check(__func__, xcdev, 1);
	if (rc < 0)
		return rc;
	xdev = xcdev->xdev;
	engine = xcdev->engine;

	dbg_sg("In %s()\n", __func__);

	if (count & 3) {
		dbg_sg("Buffer size must be a multiple of 4 bytes\n");
		return -EINVAL;
	}

	if (!buf) {
		dbg_sg("Caught NULL pointer\n");
		return -EINVAL;
	}

	if (xdev->bypass_bar_idx < 0) {
		dbg_sg("Bypass BAR not present - unsupported operation\n");
		return -ENODEV;
	}

	/* Stage descriptors into a kernel buffer under the lock, then copy out
	 * to userspace after unlocking: copy_to_user may fault/sleep and must
	 * not run while holding engine->lock (a spinlock).
	 */
	kbuf = kvzalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	spin_lock(&engine->lock);

	if (!list_empty(&engine->transfer_list)) {
		list_for_each(idx, &engine->transfer_list) {
			transfer = list_entry(idx, struct xdma_transfer, entry);

			rc = copy_desc_data(transfer, kbuf, &buf_offset, count);
		}
	}

	spin_unlock(&engine->lock);

	if (rc == 0 && buf_offset) {
		if (copy_to_user(buf, kbuf, buf_offset))
			rc = -EFAULT;
	}

	kvfree(kbuf);

	if (rc < 0)
		return rc;
	else
		return buf_offset;
}

static ssize_t char_bypass_write(struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
	struct xdma_dev *xdev;
	struct xdma_engine *engine;
	struct xdma_cdev *xcdev = (struct xdma_cdev *)file->private_data;

	void __iomem *bypass_addr;
	size_t buf_offset = 0;
	u32 *kbuf;
	int rc = 0;

	rc = xcdev_check(__func__, xcdev, 1);
	if (rc < 0)
		return rc;
	xdev = xcdev->xdev;
	engine = xcdev->engine;

	if (count & 3) {
		dbg_sg("Buffer size must be a multiple of 4 bytes\n");
		return -EINVAL;
	}

	if (!buf) {
		dbg_sg("Caught NULL pointer\n");
		return -EINVAL;
	}

	if (xdev->bypass_bar_idx < 0) {
		dbg_sg("Bypass BAR not present - unsupported operation\n");
		return -ENODEV;
	}

	dbg_sg("In %s()\n", __func__);

	/* Copy the whole user buffer in before taking the lock: copy_from_user
	 * may fault/sleep and must not run while holding engine->lock (a
	 * spinlock). The MMIO writes below then run lock-held against kbuf.
	 */
	kbuf = kvmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
	if (copy_from_user(kbuf, buf, count)) {
		dbg_sg("Error reading data from userspace buffer\n");
		kvfree(kbuf);
		return -EFAULT;
	}

	spin_lock(&engine->lock);

	/* Write descriptor data to the bypass BAR. engine->bypass_offset is a
	 * BYTE offset into the bypass BAR (engines_num * BYPASS_MODE_SPACING),
	 * so add it to the void __iomem * base directly. The previous code cast
	 * the base to (u32 __iomem *) before adding, which scaled the offset by
	 * 4 and sent every engine past channel 0 to bar + bypass_offset*4. The
	 * per-DWORD streaming to this fixed bypass address is unchanged. */
	bypass_addr = xdev->bar[xdev->bypass_bar_idx] + engine->bypass_offset;
	while (buf_offset < count) {
		write_register(kbuf[buf_offset / sizeof(u32)], bypass_addr,
				engine->bypass_offset);
		buf_offset += sizeof(u32);
		rc = buf_offset;
	}

	spin_unlock(&engine->lock);

	kvfree(kbuf);

	return rc;
}


/*
 * character device file operations for bypass operation
 */

static const struct file_operations bypass_fops = {
	.owner = THIS_MODULE,
	.open = char_open,
	.release = char_close,
	.read = char_bypass_read,
	.write = char_bypass_write,
	.mmap = bridge_mmap,
};

void cdev_bypass_init(struct xdma_cdev *xcdev)
{
	cdev_init(&xcdev->cdev, &bypass_fops);
}
