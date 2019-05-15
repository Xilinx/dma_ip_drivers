/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
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

#ifdef DEBUGFS
#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_debugfs_dev.h"
#include "xdev_regs.h"
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEBUGFS_DEV_INFO_SZ		(256)

#define DBGFS_ERR_BUFLEN	(64)
#define DEBGFS_LINE_SZ		(81)
#define DEBGFS_GEN_NAME_SZ	(32)
#define REG_DUMP_SIZE_PER_LINE  (256)


enum dbgfs_dev_dbg_file_type {
	DBGFS_DEV_DBGF_INFO = 0,
	DBGFS_DEV_DBGF_REGS = 1,
	DBGFS_DEV_DBGF_END,
};

struct dbgfs_dev_dbgf {
	char name[DBGFS_DBG_FNAME_SZ];
	struct file_operations fops;
};

struct dbgfs_dev_priv {
	unsigned long dev_hndl;
	char dev_name[QDMA_DEV_NAME_SZ];
	char *data;
	int datalen;
};

enum bar_type {
	DEBUGFS_BAR_CONFIG = 0,
	DEBUGFS_BAR_USER = 1,
	DEBUGFS_BAR_BYPASS = 2,
};

extern struct dentry *qdma_debugfs_root;

/** structure to hold file ops */
static struct dbgfs_dev_dbgf dbgf[DBGFS_DEV_DBGF_END];

/*****************************************************************************/
/**
 * dump_banner() - static helper function to dump a device banner
 *
 * @param[in]	dev_name:	qdma device name
 * @param[out]	buf:	buffer to which banner is dumped
 * @param[in]	buf_sz:	size of the buffer passed to func
 *
 * @return	len: length of the buffer printed
 *****************************************************************************/
static int dump_banner(char *dev_name, char *buf, int buf_sz)
{
	int len = 0;
	char seperator[81] = {0};

	memset(seperator, '#', 80);

	/** Banner consumes three lines, so size should be min 240 (80 * 3)
	 * If changed, check the minimum buffer size required
	 */
	if (buf_sz < (3 * DEBGFS_LINE_SZ))
		return -1;

	len += snprintf(buf + len, buf_sz - len, "%s\n", seperator);
	len += snprintf(buf + len, buf_sz - len,
			"###\t\t\t\tqdma%s, reg dump\n",
			dev_name);
	len += snprintf(buf + len, buf_sz - len, "%s\n", seperator);

	return len;
}

/*****************************************************************************/
/**
 * dump_reg() - static helper function to dump a specific reg
 *
 * @param[out]	buf:	buffer to dump the registers
 * @param[in]	buf_sz:	size of the buffer passed to func
 * @param[in]	rname:	register name
 * @param[in]	rval:	register value
 *
 * @return	len: length of the buffer printed
 *****************************************************************************/
static int dump_reg(char *buf, int buf_sz, unsigned int raddr, char *rname,
					unsigned int rval)
{
	int len = 0;

	/* length of the line should not exceed 80 chars, so, checking
	 * for min 80 chars. If below print pattern is changed, check for
	 * new the buffer size requirement
	 */
	if (buf_sz < DEBGFS_LINE_SZ)
		return -1;

	len += sprintf(buf + len, "[%#7x] %-47s %#-10x %u\n",
			raddr, rname, rval, rval);

	return len;
}

/*****************************************************************************/
/**
 * dump_bar_regs() - static helper function to dump a specific bar regs
 *
 * @param[in]	dev_hndl:	pointer to xdev device structure
 * @param[out]	buf:	buffer to dump the registers
 * @param[in]	buf_len:	size of the buffer passed
 * @param[in]	type:	bar type
 *
 * @return	len: returns the length buffer printed
 *****************************************************************************/
static int dump_bar_regs(unsigned long dev_hndl, char *buf, int buf_len,
		enum bar_type type)
{
	int num_regs = 0;
	int i = 0, j = 0;
	unsigned int val = 0;
	unsigned int addr = 0;
	int len = 0;
	int rv;
	char name[DEBGFS_GEN_NAME_SZ] = {0};
	char bar_name[DEBGFS_GEN_NAME_SZ] = {0};
	struct xreg_info *reg_info;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (type == DEBUGFS_BAR_CONFIG) {
		num_regs = sizeof(qdma_config_regs)/sizeof(qdma_config_regs[0]);
		reg_info = qdma_config_regs;
		snprintf(bar_name, DEBGFS_GEN_NAME_SZ, "%s #%d", "CONFIG BAR",
				xdev->conf.bar_num_config);
	} else if (type == DEBUGFS_BAR_USER) {
		num_regs = sizeof(qdma_user_regs)/sizeof(qdma_user_regs[0]);
		reg_info = qdma_user_regs;
		snprintf(bar_name, DEBGFS_GEN_NAME_SZ, "%s #%d", "USER BAR",
				xdev->conf.bar_num_user);
	}

	num_regs -= 1;  /* last entry is empty */

	/* print the bar name */
	len += snprintf(buf + len, buf_len - len, "\n%s\n", bar_name);
	if (len >= buf_len){
		pr_warn("insufficient space to dump reg vals\n");
		pr_warn("ERR! str truncated. req=%d, avail=%d", len, buf_len);
		return buf_len;
	}
	/* now print all the registers of the corresponding bar */
	for (i = 0; i < num_regs; i++) {
		/* if there are more than one register of same type
		 * loop and print 'repeat' number of times
		 */
		if (reg_info[i].repeat) {
			for (j = 0; j < reg_info[i].repeat; j++) {
				memset(name, 0, DEBGFS_GEN_NAME_SZ);
				snprintf(name, DEBGFS_GEN_NAME_SZ, "%s_%d",
						reg_info[i].name, j);

				addr = reg_info[i].addr + (j * 4);

				/* read the register value */
				val = qdma_device_read_config_register(dev_hndl,
						addr);

				/* num print address, name & value of reg */
				rv = dump_reg(buf + len, buf_len - len, addr,
						name, val);
				if (rv < 0) {
					pr_warn("insufficient space to dump reg vals\n");
					return len;
				}
				len += rv;
			}
		} else {
			addr = reg_info[i].addr;
			val = qdma_device_read_config_register(dev_hndl,
					addr);
			rv = dump_reg(buf + len, buf_len - len, addr,
					(char *)reg_info[i].name, val);
			if (rv < 0) {
				pr_warn("inusfficient space to dump register values\n");
				return len;
			}
			len += rv;
		}
	}

	return len;
}

/*****************************************************************************/
/**
 * dbgfs_dump_qdma_regs() - static function to dump qdma device registers
 *
 * @param[in]	xpdev:	pointer to qdma pci device structure
 * @param[in]	buf:	buffer to dump the registers
 * @param[in]	buf_len:size of the buffer passed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dbgfs_dump_qdma_regs(unsigned long dev_hndl, char *dev_name,
		char **data, int *data_len)
{
	int len = 0;
	int rv;
	char *buf = NULL;
	int num_elem =
			((sizeof(qdma_config_regs)/sizeof(struct xreg_info)) + 1) +
			((sizeof(qdma_user_regs)/sizeof(struct xreg_info)) + 1);
	int buflen = num_elem * REG_DUMP_SIZE_PER_LINE;

	/** allocate memory */
	buf = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* print the banner with device info */
	rv = dump_banner(dev_name, buf + len, buflen - len);
	if (rv < 0) {
		pr_warn("insufficient space to dump register banner\n");
		return len;
	}
	len += rv;

	rv = dump_bar_regs(dev_hndl, buf + len, buflen - len,
			   DEBUGFS_BAR_USER);
	if (rv < 0) {
		pr_warn("insufficient space to dump User bar register values\n");
		return len;
	}
	len += rv;

	rv = dump_bar_regs(dev_hndl, buf + len, buflen - len,
			   DEBUGFS_BAR_CONFIG);
	if (rv < 0) {
		pr_warn("insufficient space to dump Config Bar register values\n");
		return len;
	}
	len += rv;

	*data = buf;
	*data_len = buflen;

	return len;
}

/*****************************************************************************/
/**
 * dbgfs_dump_qdma_info() - static function to dump qdma device registers
 *
 * @xpdev:	pointer to qdma pci device structure
 * @buf:	buffer to dump the registers
 * @buf_len:size of the buffer passed
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dbgfs_dump_qdma_info(unsigned long dev_hndl, char *dev_name,
		char **data, int *data_len)
{
	int len = 0;
	char *buf = NULL;
	int buflen = DEBUGFS_DEV_INFO_SZ;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev_conf *conf = NULL;

	if (!xdev)
		return -EINVAL;

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0)
		return -EINVAL;

	conf = &xdev->conf;

	/** allocate memory */
	buf = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += snprintf(buf + len, buflen - len, "%-36s: %s\n", "Master PF",
			(conf->master_pf) ? "True" : "False");
	len += snprintf(buf + len, buflen - len, "%-36s: %d\n", "QBase",
			conf->qsets_base);
	len += snprintf(buf + len, buflen - len, "%-36s: %d\n", "Max Qsets",
			conf->qsets_max);
	len += snprintf(buf + len, buflen - len, "%-36s: %d\n",
			"Number of VFs", xdev->vf_count);
	len += snprintf(buf + len, buflen - len, "%-36s: %d\n",
			"Max number of VFs", conf->vf_max);
	len += snprintf(buf + len, buflen - len, "%-36s: %s mode\n",
			"Driver Mode",
			mode_name_list[conf->qdma_drv_mode].name);

	*data = buf;
	*data_len = buflen;

	return len;
}

/*****************************************************************************/
/**
 * dev_dbg_file_open() - static function that provides generic open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dev_dbg_file_open(struct inode *inode, struct file *fp)
{
	int dev_id = -1;
	int rv = 0;
	unsigned char dev_name[QDMA_DEV_NAME_SZ] = {0};
	unsigned char *lptr = NULL, *rptr = NULL;
	struct dentry *dev_dir = NULL;
	struct dbgfs_dev_priv *priv = NULL;
	struct xlnx_dma_dev *xdev = NULL;

	if (!inode || !fp)
		return -EINVAL;
	dev_dir = fp->f_path.dentry->d_parent;
	xdev = inode->i_private;
	if (!xdev)
		return -EINVAL;

	/* convert colon sepearted b:d:f to hex */
	rptr = dev_dir->d_iname;
	lptr = dev_name;
	while (*rptr) {
		if (*rptr == ':') {
			rptr++;
			continue;
		}
		*lptr++ = *rptr++;
	}

	/* convert this string as hex integer */
	rv = kstrtoint((const char *)dev_name, 16, &dev_id);
	if (rv < 0) {
		rv = -ENODEV;
		return rv;
	}


	priv = (struct dbgfs_dev_priv *) kzalloc(sizeof(struct dbgfs_dev_priv),
			GFP_KERNEL);
	if (!priv) {
		rv = -ENOMEM;
		return rv;
	}

	priv->dev_hndl = (unsigned long)xdev;
	snprintf(priv->dev_name, QDMA_DEV_NAME_SZ, "%s", dev_name);
	fp->private_data = priv;

	return 0;
}

/*****************************************************************************/
/**
 * dev_dbg_file_release() - static function that provides generic release
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dev_dbg_file_release(struct inode *inode, struct file *fp)
{
	kfree(fp->private_data);

	fp->private_data = NULL;

	return 0;
}

/*****************************************************************************/
/**
 * dev_dbg_file_read() - static function that provides common read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 * @param[in]	type: information type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t dev_dbg_file_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos, enum dbgfs_dev_dbg_file_type type)
{
	char *buf = NULL;
	int buf_len = 0;
	int len = 0;
	int rv = 0;
	struct dbgfs_dev_priv *dev_priv =
		(struct dbgfs_dev_priv *)fp->private_data;

	if (dev_priv->data == NULL && dev_priv->datalen == 0) {
		if (type == DBGFS_DEV_DBGF_INFO) {
			rv = dbgfs_dump_qdma_info(dev_priv->dev_hndl,
					dev_priv->dev_name, &buf, &buf_len);
		} else if (type == DBGFS_DEV_DBGF_REGS) {
			rv = dbgfs_dump_qdma_regs(dev_priv->dev_hndl,
					dev_priv->dev_name, &buf, &buf_len);
		}

		if (rv < 0)
			goto dev_dbg_file_read_exit;

		dev_priv->datalen = rv;
		dev_priv->data = buf;
	}

	buf = dev_priv->data;
	len = dev_priv->datalen;

	if (!buf)
		goto dev_dbg_file_read_exit;

	/** write to user buffer */
	if (*ppos >= len) {
		rv = 0;
		goto dev_dbg_file_read_exit;
	}

	if (*ppos + count > len)
		count = len - *ppos;

	if (copy_to_user(user_buffer, buf + *ppos, count)) {
		rv = -EFAULT;
		goto dev_dbg_file_read_exit;
	}

	*ppos += count;
	rv = count;

	pr_debug("nuber of bytes written to user space is %zu\n", count);

dev_dbg_file_read_exit:
	kfree(buf);
	dev_priv->data = NULL;
	dev_priv->datalen = 0;
	return rv;
}

/*****************************************************************************/
/**
 * dev_info_open() - static function that executes info open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dev_info_open(struct inode *inode, struct file *fp)
{
	return dev_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * dev_info_read() - static function that executes info read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t dev_info_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos)
{
	return dev_dbg_file_read(fp, user_buffer, count, ppos,
			DBGFS_DEV_DBGF_INFO);
}

/*****************************************************************************/
/**
 * dev_regs_open() - static function that opens regs debug file
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int dev_regs_open(struct inode *inode, struct file *fp)
{
	return dev_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * dev_regs_read() - static function that executes info read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t dev_regs_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos)
{
	return dev_dbg_file_read(fp, user_buffer, count, ppos,
			DBGFS_DEV_DBGF_REGS);
}

/*****************************************************************************/
/**
 * create_dev_dbg_files() - static function to create queue debug files
 *
 * @param[in]	dev_root:	debugfs root for the device
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int create_dev_dbg_files(struct xlnx_dma_dev *xdev, struct dentry *dev_root)
{
	struct dentry  *fp[DBGFS_DEV_DBGF_END] = { NULL };
	struct file_operations *fops = NULL;
	int i = 0;

	memset(dbgf, 0, sizeof(struct dbgfs_dev_dbgf) * DBGFS_DEV_DBGF_END);

	for (i = 0; i < DBGFS_DEV_DBGF_END; i++) {
		fops = &dbgf[i].fops;
		fops->owner = THIS_MODULE;
		switch (i) {
		case DBGFS_DEV_DBGF_INFO:
			snprintf(dbgf[i].name, 64, "%s", "info");
			fops->open = dev_info_open;
			fops->read = dev_info_read;
			fops->release = dev_dbg_file_release;
			break;
		case DBGFS_DEV_DBGF_REGS:
			snprintf(dbgf[i].name, 64, "%s", "regs");
			fops->open = dev_regs_open;
			fops->read = dev_regs_read;
			fops->release = dev_dbg_file_release;
			break;
		}
	}

	for (i = 0; i < DBGFS_DEV_DBGF_END; i++) {
		fp[i] = debugfs_create_file(dbgf[i].name, 0644, dev_root,
				    xdev, &dbgf[i].fops);
		if (!fp[i])
			return -1;
	}
	return 0;
}

/*****************************************************************************/
/**
 * dbgfs_dev_init() - function to initialize device debugfs files
 *
 * @param[in]	xdev:	Xilinx dma device
 * @param[in]	qdma_debugfs_root:	root file in debugfs
 *
 * @return	=0: success
 * @return	<0: error
 *****************************************************************************/
int dbgfs_dev_init(struct xlnx_dma_dev *xdev)
{
	char dname[QDMA_DEV_NAME_SZ] = {0};
	struct dentry *dbgfs_dev_root = NULL;
	struct dentry *dbgfs_queues_root = NULL;
	struct pci_dev *pdev = xdev->conf.pdev;
	int rv = 0;

	snprintf(dname, QDMA_DEV_NAME_SZ, "%02x:%02x:%x",
			pdev->bus->number,
			PCI_SLOT(pdev->devfn),
			PCI_FUNC(pdev->devfn));
	/* create a directory for the device in debugfs */
	dbgfs_dev_root = debugfs_create_dir(dname, qdma_debugfs_root);
	if (!dbgfs_dev_root) {
		pr_err("Failed to create device direcotry\n");
		return -1;
	}
	xdev->dbgfs_dev_root = dbgfs_dev_root;

	/* create debug files for qdma device */
	rv = create_dev_dbg_files(xdev, dbgfs_dev_root);
	if (rv < 0) {
		pr_err("Failed to create device debug files\n");
		goto dbgfs_dev_init_fail;
	}

	/* create a directory for the queues in debugfs */
	dbgfs_queues_root = debugfs_create_dir("queues", xdev->dbgfs_dev_root);
	if (!dbgfs_queues_root) {
		pr_err("Failed to create queueus directory under device directory\n");
		goto dbgfs_dev_init_fail;
	}

	xdev->dbgfs_queues_root = dbgfs_queues_root;
	spin_lock_init(&xdev->qidx_lock);

	return 0;

dbgfs_dev_init_fail:

	debugfs_remove_recursive(dbgfs_dev_root);
	return -1;
}

/*****************************************************************************/
/**
 * dbgfs_dev_exit() - function to cleanup device debugfs files
 *
 * @param[in]	xdev:	Xilinx dma device
 *
 *****************************************************************************/
void dbgfs_dev_exit(struct xlnx_dma_dev *xdev)
{
	if (xdev->dbgfs_dev_root)
		debugfs_remove_recursive(xdev->dbgfs_dev_root);
	xdev->dbgfs_dev_root = NULL;
	xdev->dbgfs_queues_root = NULL;
}

#endif
