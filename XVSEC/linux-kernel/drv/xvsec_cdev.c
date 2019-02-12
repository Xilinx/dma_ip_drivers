/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018,  Xilinx, Inc.
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

#include "xvsec_cdev.h"
#include "xvsec_cdev_int.h"


struct xvsec_context *xvsec_ctx;

const uint16_t fpga_valid_addr[] = {
		0x00, 0x01, 0x02, 0x03, 0x04,
		0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
		0x10, 0x11, 0x14, 0x16, 0x18,
		0x1F
	};


static struct file *fopen(const char *path, int flags, int rights);
static void fclose(struct file *filep);
static int fread(struct file *filep,
	uint64_t offset, uint8_t *data, uint32_t size);
static int get_file_size(const char *fname, loff_t *size);

static void xvsec_fpga_cfg_setup(struct context *xvsec_ctx,
	enum oper operation);
static void xvsec_fpga_cfg_teardown(struct context *xvsec_ctx);
static void xvsec_fpga_cfg_write_cmd(struct context *xvsec_ctx,
	enum oper operation, uint8_t offset, uint16_t word_count);
static void xvsec_fpga_cfg_write_data(struct context *xvsec_ctx, uint32_t data);
static int xvsec_open(struct inode *inode, struct file *filep);
static int xvsec_close(struct inode *inode, struct file *filep);
static long xvsec_ioctl(struct file *filep, uint32_t cmd, unsigned long arg);

static long xvsec_ioc_get_cap_list(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_mcap_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_mcap_module_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_mcap_full_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_get_data_regs(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_get_regs(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_get_fpga_regs(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_prog_bitstream(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_rd_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_wr_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_rd_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_wr_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_get_device_info(struct file *filep,
	uint32_t cmd, unsigned long arg);


static int xvsec_get_vsec_capabilities(struct context *xvsec_ctx);
static int xvsec_mcap_req_access(struct context *xvsec_ctx, uint32_t *restore);
static int xvsec_mcap_reset(struct context *xvsec_ctx);
static int xvsec_mcap_module_reset(struct context *xvsec_ctx);
static int xvsec_mcap_full_reset(struct context *xvsec_ctx);
static int xvsec_mcap_get_data_regs(struct context *xvsec_ctx,
	uint32_t regs[4]);
static int xvsec_mcap_get_regs(struct context *xvsec_ctx,
	struct mcap_regs *regs);
static int xvsec_mcap_get_fpga_regs(struct context *xvsec_ctx,
	struct fpga_cfg_regs *regs);
static int xvsec_mcap_program_bitstream(struct context *xvsec_ctx,
	struct bitstream_file *bit_files);
static int xvsec_mcap_program(struct context *xvsec_ctx, char *fname);
static int xvsec_write_rbt(struct context *xvsec_ctx,
	struct file *filep, loff_t size);
static int xvsec_write_bit(struct context *xvsec_ctx,
	struct file *filep, loff_t size);
static int xvsec_write_bin(struct context *xvsec_ctx,
	struct file *filep, loff_t size);
static int xvsec_mcap_rd_cfg_addr(struct context *xvsec_ctx,
	struct cfg_data *data);
static int xvsec_mcap_wr_cfg_addr(struct context *xvsec_ctx,
	struct cfg_data *data);
static int xvsec_fpga_rd_cfg_addr(struct context *xvsec_ctx,
	struct fpga_cfg_reg *cfg_reg);
static int xvsec_fpga_wr_cfg_addr(struct context *xvsec_ctx,
	struct fpga_cfg_reg *cfg_reg);
static int xvsec_get_dev_info(struct context *xvsec_ctx,
	struct device_info *dev_info);
static int check_for_completion(struct context *xvsec_ctx,
	uint32_t *ret);

static int find_file_type(char *fname, const char *suffix)
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

static struct file *fopen(const char *path, int flags, int rights)
{
	struct file *filep = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filep = filp_open(path, (flags | O_LARGEFILE), rights);
	set_fs(oldfs);
	if (IS_ERR(filep) != 0) {
		err = PTR_ERR(filep);
		pr_err("filp_open failed with err : 0x%X\n", err);
		return NULL;
	}
	return filep;
}

static void fclose(struct file *filep)
{
	filp_close(filep, NULL);
}

static int fread(struct file *filep, uint64_t offset,
	uint8_t *data, uint32_t size)
{
	int ret = 0;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(filep, data, size, &offset);
	filep->f_pos = offset;
	set_fs(oldfs);

	return ret;
}

static int get_file_size(const char *fname, loff_t *size)
{
	int ret = 0;
	mm_segment_t oldfs;
	struct kstat stat;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_stat(fname, &stat);
	set_fs(oldfs);
	if (ret < 0)
		return -(EIO);

	*size = stat.size;

	return ret;
}

static void xvsec_fpga_cfg_setup(struct context *xvsec_ctx, enum oper operation)
{
	int wr_offset;
	int index;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	switch (operation) {
	case FPGA_WR_CMD:
		for (index = 0; index < 16; index++)
			pci_write_config_dword(pdev, wr_offset, DUMMY_WORD);
		pci_write_config_dword(pdev, wr_offset, BUS_WIDTH_SYNC);
		pci_write_config_dword(pdev, wr_offset, BUS_WIDTH_DETECT);
		pci_write_config_dword(pdev, wr_offset, DUMMY_WORD);
		pci_write_config_dword(pdev, wr_offset, DUMMY_WORD);
		pci_write_config_dword(pdev, wr_offset, SYNC_WORD);
		pci_write_config_dword(pdev, wr_offset, NOOP);
		pci_write_config_dword(pdev, wr_offset, NOOP);
		break;

	case FPGA_RD_CMD:
	default:

		pci_write_config_dword(pdev, wr_offset, DUMMY_WORD);
		pci_write_config_dword(pdev, wr_offset, BUS_WIDTH_SYNC);
		pci_write_config_dword(pdev, wr_offset, BUS_WIDTH_DETECT);
		pci_write_config_dword(pdev, wr_offset, DUMMY_WORD);
		pci_write_config_dword(pdev, wr_offset, SYNC_WORD);
		pci_write_config_dword(pdev, wr_offset, NOOP);

		break;
	}
}

static void xvsec_fpga_cfg_teardown(struct context *xvsec_ctx)
{
	int wr_offset;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	pci_write_config_dword(pdev, wr_offset, TYPE1_WR_CMD);
	pci_write_config_dword(pdev, wr_offset, DESYNC);
	pci_write_config_dword(pdev, wr_offset, NOOP);
	pci_write_config_dword(pdev, wr_offset, NOOP);
}

static void xvsec_fpga_cfg_write_cmd(struct context *xvsec_ctx,
	enum oper operation, uint8_t offset, uint16_t word_count)
{
	int wr_offset;
	union type1_header  header;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	header.data		= 0x0;

	header.header_type	= TYPE1_HEADER_TYPE;
	header.opcode		= (operation == FPGA_WR_CMD) ?
					FPGA_CFG_WRITE : FPGA_CFG_READ;
	header.address		= offset;
	header.word_count	= word_count;

	pci_write_config_dword(pdev, wr_offset, header.data);
	if (operation == FPGA_RD_CMD) {
		pci_write_config_dword(pdev, wr_offset, NOOP);
		pci_write_config_dword(pdev, wr_offset, NOOP);
	}
}

static void xvsec_fpga_cfg_write_data(struct context *xvsec_ctx, uint32_t data)
{
	int wr_offset;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	pci_write_config_dword(pdev, wr_offset, data);
	pci_write_config_dword(pdev, wr_offset, NOOP);
	pci_write_config_dword(pdev, wr_offset, NOOP);
}

static int xvsec_open(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct context		*xvsec_ctx;
	struct file_priv	*priv;

	xvsec_ctx = container_of(inode->i_cdev, struct context, cdev);

	if (xvsec_ctx->fopen_cnt != 0)
		return -(EBUSY);

	priv = kzalloc(sizeof(struct file_priv), GFP_KERNEL);
	if (priv == NULL)
		return -(ENOMEM);

	xvsec_ctx->fopen_cnt++;
	priv->ctx = (void *)xvsec_ctx;
	filep->private_data = priv;

	pr_debug("xvsec_open success\n");

	return	ret;
}

static int xvsec_close(struct inode *inode, struct file *filep)
{
	struct file_priv  *priv = filep->private_data;
	struct context    *xvsec_ctx = (struct context *)priv->ctx;

	if (xvsec_ctx->fopen_cnt == 0) {
		pr_warn("File Open/close mismatch\n");
	} else {
		xvsec_ctx->fopen_cnt--;
		pr_info("xvsec_close success\n");
	}
	kfree(priv);
	return 0;
}

static const struct ioctl_ops ioctl_ops[] = {
	{IOC_XVSEC_GET_CAP_LIST,	xvsec_ioc_get_cap_list},
	{IOC_MCAP_RESET,		xvsec_ioc_mcap_reset},
	{IOC_MCAP_MODULE_RESET,		xvsec_ioc_mcap_module_reset},
	{IOC_MCAP_FULL_RESET,		xvsec_ioc_mcap_full_reset},
	{IOC_MCAP_GET_DATA_REGISTERS,	xvsec_ioc_get_data_regs},
	{IOC_MCAP_GET_REGISTERS,	xvsec_ioc_get_regs},
	{IOC_MCAP_GET_FPGA_REGISTERS,	xvsec_ioc_get_fpga_regs},
	{IOC_MCAP_PROGRAM_BITSTREAM,	xvsec_ioc_prog_bitstream},
	{IOC_MCAP_READ_DEV_CFG_REG,	xvsec_ioc_rd_dev_cfg_reg},
	{IOC_MCAP_WRITE_DEV_CFG_REG,	xvsec_ioc_wr_dev_cfg_reg},
	{IOC_MCAP_READ_FPGA_CFG_REG,	xvsec_ioc_rd_fpga_cfg_reg},
	{IOC_MCAP_WRITE_FPGA_CFG_REG,	xvsec_ioc_wr_fpga_cfg_reg},
	{IOC_GET_DEVICE_INFO,		xvsec_ioc_get_device_info},
};


static long xvsec_ioc_get_cap_list(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;


	pr_debug("ioctl : IOC_XVSEC_GET_CAP_LIST\n");

	ret = xvsec_get_vsec_capabilities(xvsec_ctx);
	if (ret < 0) {
		pr_err("xvsec_get_vsec_capabilities ");
		pr_err("returned err : %d\n", ret);
		goto CLEANUP;
	}

	ret = copy_to_user((void __user *)arg,
		(void *)&xvsec_ctx->capabilities,
		sizeof(struct xvsec_capabilities));
CLEANUP:
	return ret;
}
static long xvsec_ioc_mcap_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;

	pr_debug("ioctl : IOC_MCAP_RESET\n");

	ret = xvsec_mcap_reset(xvsec_ctx);

	return ret;

}
static long xvsec_ioc_mcap_module_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;

	pr_debug("ioctl : IOC_MCAP_MODULE_RESET\n");

	ret = xvsec_mcap_module_reset(xvsec_ctx);

	return ret;
}
static long xvsec_ioc_mcap_full_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;

	pr_debug("ioctl : IOC_MCAP_FULL_RESET\n");

	ret = xvsec_mcap_full_reset(xvsec_ctx);

	return ret;
}
static long xvsec_ioc_get_data_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;
	uint32_t         read_data_reg[4];

	pr_debug("ioctl : IOC_MCAP_GET_DATA_REGISTERS\n");

	ret = xvsec_mcap_get_data_regs(xvsec_ctx, read_data_reg);
	if (ret == 0) {
		ret = copy_to_user((void __user *)arg,
			(void *)read_data_reg, sizeof(uint32_t)*4);
	}

	return ret;

}
static long xvsec_ioc_get_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv *priv = filep->private_data;
	struct context   *xvsec_ctx = (struct context *)priv->ctx;
	struct mcap_regs mcap_regs;

	pr_debug("ioctl : IOC_MCAP_GET_REGISTERS\n");

	memset(&mcap_regs, 0, sizeof(struct mcap_regs));
	ret = xvsec_mcap_get_regs(xvsec_ctx, &mcap_regs);
	if (ret == 0) {
		ret = copy_to_user((void __user *)arg,
			(void *)&mcap_regs, sizeof(struct mcap_regs));
	}

	return ret;
}
static long xvsec_ioc_get_fpga_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv      *priv = filep->private_data;
	struct context        *xvsec_ctx = (struct context *)priv->ctx;
	struct fpga_cfg_regs  fpga_cfg_regs;

	pr_debug("ioctl : IOC_MCAP_GET_FPGA_REGISTERS\n");

	memset(&fpga_cfg_regs, 0, sizeof(struct fpga_cfg_regs));
	ret = xvsec_mcap_get_fpga_regs(xvsec_ctx, &fpga_cfg_regs);
	if (ret == 0) {
		fpga_cfg_regs.valid = 1;
		fpga_cfg_regs.far = 1;
		ret = copy_to_user((void __user *)arg,
			(void *)&fpga_cfg_regs,
			sizeof(struct fpga_cfg_regs));
	}

	return ret;
}
static long xvsec_ioc_prog_bitstream(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct bitstream_file  bit_files;

	pr_debug("ioctl : IOC_MCAP_PROGRAM_BITSTREAM\n");
	ret = copy_from_user(&bit_files, (void __user *)arg,
		sizeof(struct bitstream_file));

	if (ret != 0)
		goto CLEANUP;

	bit_files.status = MCAP_BITSTREAM_PROGRAM_FAILURE;
	ret = xvsec_mcap_program_bitstream(xvsec_ctx, &bit_files);
	if (ret < 0)
		goto CLEANUP;

	bit_files.status = MCAP_BITSTREAM_PROGRAM_SUCCESS;

	ret = copy_to_user((void __user *)arg, (void *)&bit_files,
		sizeof(struct bitstream_file));

CLEANUP:
	return ret;
}
static long xvsec_ioc_rd_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct cfg_data        rw_cfg_data;

	pr_debug("ioctl : IOC_MCAP_READ_DEV_CFG_REG\n");

	ret = copy_from_user(&rw_cfg_data,
		(void __user *)arg, sizeof(struct cfg_data));

	if (ret != 0)
		goto CLEANUP;

	ret = xvsec_mcap_rd_cfg_addr(xvsec_ctx, &rw_cfg_data);
	if (ret < 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&rw_cfg_data, sizeof(struct cfg_data));

CLEANUP:
	return ret;
}
static long xvsec_ioc_wr_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct cfg_data        rw_cfg_data;

	pr_debug("ioctl : IOC_MCAP_WRITE_DEV_CFG_REG\n");

	ret = copy_from_user(&rw_cfg_data,
		(void __user *)arg, sizeof(struct cfg_data));

	if (ret != 0)
		goto CLEANUP;

	ret = xvsec_mcap_wr_cfg_addr(xvsec_ctx, &rw_cfg_data);

CLEANUP:
	return ret;

}
static long xvsec_ioc_rd_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct fpga_cfg_reg    fpga_cfg_data;

	pr_debug("ioctl : IOC_MCAP_READ_FPGA_CFG_REG\n");
	ret = copy_from_user(&fpga_cfg_data,
		(void __user *)arg, sizeof(struct fpga_cfg_reg));

	if (ret != 0)
		goto CLEANUP;

	ret = xvsec_fpga_rd_cfg_addr(xvsec_ctx, &fpga_cfg_data);
	if (ret != 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&fpga_cfg_data, sizeof(struct fpga_cfg_reg));
CLEANUP:
	return ret;
}
static long xvsec_ioc_wr_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct fpga_cfg_reg    fpga_cfg_data;

	pr_debug("ioctl : IOC_MCAP_WRITE_FPGA_CFG_REG\n");

	ret = copy_from_user(&fpga_cfg_data,
		(void __user *)arg, sizeof(struct fpga_cfg_reg));

	if (ret != 0)
		goto CLEANUP;

	ret = xvsec_fpga_wr_cfg_addr(xvsec_ctx, &fpga_cfg_data);
CLEANUP:
	return ret;
}
static long xvsec_ioc_get_device_info(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv       *priv = filep->private_data;
	struct context         *xvsec_ctx = (struct context *)priv->ctx;
	struct device_info     dev_info;

	pr_debug("ioctl : IOC_GET_DEVICE_INFO\n");

	ret = xvsec_get_dev_info(xvsec_ctx, &dev_info);
	if (ret < 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&dev_info, sizeof(struct device_info));
CLEANUP:
	return ret;
}



static long xvsec_ioctl(struct file *filep, uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	int index, cmd_cnt;
	bool cmd_found = false;

	cmd_cnt = ARRAY_SIZE(ioctl_ops);
	for (index = 0; index < cmd_cnt; index++) {
		if (ioctl_ops[index].cmd == cmd) {
			cmd_found = true;
			ret = ioctl_ops[index].fpfunction(filep, cmd, arg);
			break;
		}
	}
	if (cmd_found == false)
		ret = -(EINVAL);

	return ret;
}

static int check_for_completion(struct context *xvsec_ctx, uint32_t *ret)
{
	unsigned long retry_count = 0;
	int i;
	uint16_t sts_offset, data_off;
	uint32_t sts_data;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	sts_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_STATUS_REGISTER;
	data_off = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	pci_read_config_dword(pdev, sts_offset, &sts_data);

	while ((sts_data & XVSEC_MCAP_STATUS_EOS) == 0x0) {
		msleep(20);
		for (i = 0 ; i < EMCAP_EOS_LOOP_COUNT; i++)
			pci_write_config_dword(pdev, data_off, EMCAP_NOOP_VAL);

		pci_read_config_dword(pdev, sts_offset, &sts_data);
		retry_count++;
		if (retry_count > EMCAP_EOS_RETRY_COUNT) {
			pr_err("Error: The MCAP EOS bit did not assert after");
			pr_err(" programming the specified programming file\n");
			pr_err("Status Reg : 0x%X\n", sts_data);
			*ret = sts_data;
			return -EIO;
		}
	}
	*ret = sts_data;
	return 0;
}

static const struct file_operations xvsec_fops = {
	.owner			= THIS_MODULE,
	.open			= xvsec_open,
	.release		= xvsec_close,
	.unlocked_ioctl		= xvsec_ioctl,
};

static int xvsec_get_vsec_capabilities(struct context *xvsec_ctx)
{
	int ret = 0;
	int offset = 0;
	int nxt_offset;
	int index = 0;
	uint16_t vsec_id = 0x0;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	xvsec_ctx->mcap_cap_offset = INVALID_OFFSET;
	index = 0;
	do {
		nxt_offset = pci_find_next_ext_capability(pdev,
			offset, XVSEC_EXT_CAP_VSEC_ID);
		if (nxt_offset == 0)
			break;

		ret = pci_read_config_word(pdev, (nxt_offset + 4), &vsec_id);
		if (ret != 0) {
			pr_warn("pci_read_config_dword failed with error");
			pr_warn(" %d, offset : 0x%X\n", ret, offset);
			break;
		}

		xvsec_ctx->capabilities.
			capability_offset[index] = nxt_offset;
		xvsec_ctx->capabilities.
			capability_id[index] = vsec_id;
		offset = nxt_offset;

		if (vsec_id == XVSEC_MCAP_VSEC_ID)
			xvsec_ctx->mcap_cap_offset = nxt_offset;

		index = index + 1;
	} while (nxt_offset != 0);

	xvsec_ctx->capabilities.no_of_caps = index;

	pr_debug("xvsec_ctx->capabilities.no_of_caps : %d\n", index);

	return ret;
}

static int xvsec_mcap_req_access(struct context *xvsec_ctx, uint32_t *restore)
{
	int ret = 0;
	int delay = XVSEC_MCAP_LOOP_COUNT;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, sts_offset;
	uint32_t ctrl_data, sts_data;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	ctrl_offset = mcap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	sts_offset = mcap_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	*restore = ctrl_data;

	pci_read_config_dword(pdev, sts_offset, &sts_data);

	if ((sts_data & XVSEC_MCAP_STATUS_ACCESS) != 0x0) {

		ctrl_data = ctrl_data |
			(XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS);

		pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

		do {
			pci_read_config_dword(pdev, sts_offset, &sts_data);
			if ((sts_data & XVSEC_MCAP_STATUS_ACCESS) == 0x0)
				break;

			delay = delay - 1;
		} while (delay != 0);

		if (delay == 0) {
			pr_err("Unable to get the FPGA CFG Access\n");

			ctrl_data = ctrl_data &
				~(XVSEC_MCAP_CTRL_ENABLE |
				XVSEC_MCAP_CTRL_REQ_ACCESS);
			pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

			ret = -(EBUSY);
		}
	}

	return ret;
}

static int xvsec_mcap_reset(struct context *xvsec_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	ctrl_offset = mcap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Reset */
	ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_RESET |
		XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data & XVSEC_MCAP_CTRL_RESET) == 0x0)
		ret = -(EBUSY);

	status_offset = mcap_offset + XVSEC_MCAP_STATUS_REGISTER;
	pci_read_config_dword(pdev, status_offset, &status_data);
	if ((status_data & XVSEC_MCAP_STATUS_ERR) == 0x1)
		ret = -(EBUSY);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_mcap_module_reset(struct context *xvsec_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	ctrl_offset = mcap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Module Reset */
	ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_MOD_RESET |
		XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data & XVSEC_MCAP_CTRL_MOD_RESET) == 0x0)
		ret = -(EBUSY);

	status_offset = mcap_offset + XVSEC_MCAP_STATUS_REGISTER;
	pci_read_config_dword(pdev, status_offset, &status_data);
	if ((status_data & XVSEC_MCAP_STATUS_ERR) == 0x1)
		ret = -(EBUSY);


	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_mcap_full_reset(struct context *xvsec_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	ctrl_offset = mcap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Module Reset */
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_RESET | XVSEC_MCAP_CTRL_MOD_RESET |
		XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data &
		(XVSEC_MCAP_CTRL_RESET | XVSEC_MCAP_CTRL_MOD_RESET)) == 0x0)
		ret = -(EBUSY);

	status_offset = mcap_offset + XVSEC_MCAP_STATUS_REGISTER;
	pci_read_config_dword(pdev, status_offset, &status_data);
	if ((status_data & XVSEC_MCAP_STATUS_ERR) == 0x1)
		ret = -(EBUSY);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_mcap_get_data_regs(struct context *xvsec_ctx, uint32_t regs[4])
{
	uint16_t mcap_offset;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	pci_read_config_dword(pdev,
		(mcap_offset + XVSEC_MCAP_READ_DATA_REG0), &regs[0]);
	pci_read_config_dword(pdev,
		(mcap_offset + XVSEC_MCAP_READ_DATA_REG1), &regs[1]);
	pci_read_config_dword(pdev,
		(mcap_offset + XVSEC_MCAP_READ_DATA_REG2), &regs[2]);
	pci_read_config_dword(pdev,
		(mcap_offset + XVSEC_MCAP_READ_DATA_REG3), &regs[3]);

	return 0;
}
static int xvsec_mcap_get_regs(struct context *xvsec_ctx,
	struct mcap_regs *regs)
{
	uint16_t mcap_offset;
	uint16_t index;
	uint32_t *ptr;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	regs->valid = 0;

	mcap_offset = xvsec_ctx->mcap_cap_offset;
	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	ptr = &regs->ext_cap_header;
	for (index = 0; index <= (XVSEC_MCAP_READ_DATA_REG3 / 4); index++) {
		pci_read_config_dword(pdev, mcap_offset, &ptr[index]);
		mcap_offset = mcap_offset + 4;
	}

	regs->valid = 1;

	return 0;
}

static int xvsec_mcap_get_fpga_regs(struct context *xvsec_ctx,
	struct fpga_cfg_regs *regs)
{
	int ret = 0;
	int index, reg_count;
	int wr_offset, ctrl_offset, sts_offset;
	int rd_offset;
	uint32_t rd_data, ctrl_data, sts_data, restore;
	uint16_t count;
	uint32_t *reg_dump;
	struct pci_dev *pdev = xvsec_ctx->pdev;


	pr_info("In %s\n", __func__);

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;
	rd_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_READ_DATA_REG0;
	ctrl_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	sts_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(xvsec_ctx, FPGA_RD_CMD);

	reg_count = ARRAY_SIZE(fpga_valid_addr);
	/* sizeof(fpga_valid_addr)/sizeof(fpga_valid_addr[0]); */
	reg_dump = &regs->crc;
	for (index = 0; index < reg_count; index++) {
		xvsec_fpga_cfg_write_cmd(xvsec_ctx, FPGA_RD_CMD,
					fpga_valid_addr[index], 1);

		ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_RD_ENABLE;
		pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

		count = 0x0;
		while (count < 20) {
			pci_read_config_dword(pdev, sts_offset, &sts_data);
			if ((sts_data & 0x10) != 0x0)
				break;
			count = count + 1;
			msleep(20);
		}

		if (count >= 20) {
			/* FIXME : should discuss with brain martin
			 * and close this teardown
			 */
			pr_err("Time out happened while ");
			pr_err("reading FPGA CFG Register\n");
			ret = -(EBUSY);
			goto CLEANUP;
		}

		pci_read_config_dword(pdev, rd_offset, &rd_data);

		reg_dump[index] = rd_data;

		ctrl_data = ctrl_data & (~XVSEC_MCAP_CTRL_RD_ENABLE);
		pci_write_config_dword(pdev, ctrl_offset, ctrl_data);
	}

CLEANUP:
	xvsec_fpga_cfg_teardown(xvsec_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_mcap_program_bitstream(struct context *xvsec_ctx,
	struct bitstream_file *bit_files)
{
	int ret = 0;
	uint16_t ctrl_offset;
	uint32_t ctrl_data, restore;
	char bitfile[MAX_FLEN];
	uint16_t len;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	if ((bit_files->partial_clr_file == NULL) &&
		(bit_files->bitstream_file == NULL)) {
		pr_err("Both Bit files are NULL\n");
		return -(EINVAL);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Reset */
	ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_WR_ENABLE |
			XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS;


	ctrl_data = ctrl_data &
		~(XVSEC_MCAP_CTRL_RESET | XVSEC_MCAP_CTRL_MOD_RESET |
		XVSEC_MCAP_CTRL_RD_ENABLE | XVSEC_MCAP_CTRL_CFG_SWICTH);

	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	pr_info("Ctrl Data : 0x%X, 0x%X\n", ctrl_offset, ctrl_data);

	if (bit_files->partial_clr_file != NULL) {
		len = strnlen_user(bit_files->partial_clr_file, MAX_FLEN);
		if (len > MAX_FLEN) {
			pr_err("File Name too long\n");
			goto CLEANUP;
		}

		ret = strncpy_from_user(bitfile,
			bit_files->partial_clr_file, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			goto CLEANUP;
		}
		pr_info("Clear File Name : %s\n", bitfile);

		ret = xvsec_mcap_program(xvsec_ctx, bitfile);
		if (ret < 0) {
			pr_err("[xvsec_mcap] : xvsec_mcap_program ");
			pr_err("failed for partial clear file with err : ");
			pr_err("%d\n", ret);

			goto CLEANUP;
		}
	}

	if (bit_files->bitstream_file != NULL) {
		len = strnlen_user(bit_files->bitstream_file, MAX_FLEN);
		if (len > MAX_FLEN) {
			pr_err("File Name too long\n");
			goto CLEANUP;
		}

		ret = strncpy_from_user(bitfile,
			bit_files->bitstream_file, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			goto CLEANUP;
		}

		pr_info("Bit File Name : %s\n", bitfile);

		ret = xvsec_mcap_program(xvsec_ctx, bitfile);
		if (ret < 0) {
			pr_err("[xvsec_mcap] : xvsec_mcap_program ");
			pr_err("failed for bit file with err : %d\n", ret);
			goto CLEANUP;
		}
		restore = restore | XVSEC_MCAP_CTRL_CFG_SWICTH;
	}

CLEANUP:
	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_parse_rbt_file(struct file *filep, int file_size, int *offset)
{
	uint8_t chunk_size;
	char raw_buf[RBT_WORD_LEN];
	int search_off = 0, len;
	int size, index;
	uint8_t count;

	size = file_size;
	count = 0x0;
	while (size != 0) {
		chunk_size = (size > RBT_WORD_LEN) ? RBT_WORD_LEN : size;
		len = fread(filep, search_off, raw_buf, chunk_size);

		for (index = 0; index < len ; index++) {
			if ((raw_buf[index] == '1') ||
				(raw_buf[index] == '0')) {
				count = count + 1;
			} else {
				count = 0x0;
			}

			if (count >= RBT_WORD_LEN)
				break;
		}

		if (count >= RBT_WORD_LEN) {
			*offset = search_off + (index + 1) - chunk_size;
			len = fread(filep, *offset, raw_buf, RBT_WORD_LEN);
			count = 0x0;
			for (index = 0; index < len ; index++) {
				if ((raw_buf[index] == '1') ||
					(raw_buf[index] == '0')) {
					count = count + 1;
				}
			}
			if (count == RBT_WORD_LEN)
				break;
			return -(EAGAIN);

		}

		search_off = search_off + len;
		size = size - len;
	}

	if (size == 0)
		return -(ENOEXEC);

	return 0;
}

static int xvsec_write_rbt(struct context *xvsec_ctx,
	struct file *filep, loff_t size)
{
	int status;
	char bitdata[RBT_WORD_LEN + 1], dummy_data;
	uint16_t wr_offset;
	uint32_t *buf, buf_index;
	int i, len, offset = 0, remain_size, dummy_len;
	uint32_t worddata, chunk_size;
	bool done;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	status = xvsec_parse_rbt_file(filep, size, &offset);
	if (status < 0)
		return -(ENOEXEC);

	pr_info("File Parsed Successfully..Offset found : 0x%X\n", offset);

	buf = kmalloc(DMA_HWICAP_BITFILE_BUFFER_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -(ENOMEM);

	remain_size = size - offset;
	worddata = 0x0;
	buf_index = 0x0;
	done = false;
	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;

	while (done == false) {
		chunk_size = (remain_size > (RBT_WORD_LEN + 1)) ?
			(RBT_WORD_LEN + 1) : remain_size;
		len = fread(filep, offset, bitdata, chunk_size);
		/* Discarding Comments */
		if (bitdata[0] == '#') {
			offset = offset + chunk_size;
			remain_size = remain_size - chunk_size;
			continue;
		}

		if (len != chunk_size)
			pr_warn("Len and chunk_size mismatch\n");

		for (i = 0; i < (len - 1) ; i++) {
			if ((bitdata[i] == '1') || (bitdata[i] == '0')) {
				worddata = (worddata << 1) |
						(bitdata[i] - 0x30);
			} else {
				pr_info("Corrupted Character : %c, 0x%X\n",
					bitdata[i], bitdata[i]);
				break;
			}
		}

		if (i != (len - 1)) {
			pr_err("Corrupted rbt file..Found ASCII character\n");
			pr_err("in middle of the bits\n");
			pr_info("i : %d, len : %d\n", i, len);

			kfree(buf);
			return -(EFAULT);
		}

		buf[buf_index] = worddata;
		worddata = 0x0;
		offset = offset + chunk_size;
		remain_size = remain_size - chunk_size;

		buf_index = buf_index + 1;
		/* Check whether complete buffer filled up */
		if (((buf_index * 4) == DMA_HWICAP_BITFILE_BUFFER_SIZE) ||
			(remain_size == 0x0)) {

			/* Write the Data to MCAP */
			for (i = 0; i < buf_index; i++) {
				pci_write_config_dword(pdev, wr_offset, buf[i]);

				/* FROM SDAccel Code:
				* This delay resolves the MIG calibration
				* issues we have been seeing with
				* Tandem Stage 2 Loading
				*/
				udelay(1);
			}
			buf_index = 0x0;
		}

		/* More than one word in a same line..just ignore that data */
		if (bitdata[len - 1] != '\n') {
			pr_info("Found multiple words in single line\n");
			pr_info("Character : %c\n", bitdata[len - 1]);
			while (remain_size != 0) {
				dummy_len =
					fread(filep, offset, &dummy_data, 1);
				offset = offset + 1;
				remain_size = remain_size - 1;
				if ((dummy_data == '\n') || (dummy_len != 1))
					break;
			}
		}

		if (remain_size == 0)
			done = true;
	}

	kfree(buf);
	return offset;
}

static int xvsec_write_bit(struct context *xvsec_ctx,
	struct file *filep, loff_t size)
{
	int err;
	uint8_t val, len = 0;
	uint32_t index, loop;
	uint64_t offset = 0x0;
	uint32_t *buf = NULL;
	uint16_t chunk = 0;
	uint16_t wr_offset;
	loff_t remain_size = 0;
	bool	sync_found = false;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	/*
	 * .bit files are not guaranteed to be aligned with
	 * the bitstream sync word on a 32-bit boundary. So,
	 * we need to check every byte here.
	 */
	while (fread(filep, offset, &val, 1) == 1) {
		if (offset >= size) {
			pr_err("[xvsec_cdev] : Reached End of BIT file");
			pr_err(" Failed to find the sync word\n");
			return -(EINVAL);
		}
		len++; offset++;
		if (val == MCAP_SYNC_BYTE0)
		if (fread(filep, offset, &val, 1) == 1) {
			len++; offset++;
			if (val == MCAP_SYNC_BYTE1)
			if (fread(filep, offset, &val, 1) == 1) {
				len++; offset++;
				if (val == MCAP_SYNC_BYTE2)
				if (fread(filep, offset, &val, 1) == 1) {
					len++; offset++;
					if (val == MCAP_SYNC_BYTE3) {
						sync_found = true;
						break;
					}
				}
			}
		}
	}

	if (sync_found != true) {
		pr_err("[xvsec_cdev] : Failed to Read BIT file\n");
		return -(EINVAL);
	}

	pr_info("found sync pattern : %d\n", len);

	buf = kmalloc(DMA_HWICAP_BITFILE_BUFFER_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -(ENOMEM);

	buf[0] = cpu_to_be32(MCAP_SYNC_DWORD);

	remain_size = size - len;
	index = 4;
	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;
	while (remain_size != 0) {
		chunk = (remain_size > DMA_HWICAP_BITFILE_BUFFER_SIZE) ?
			DMA_HWICAP_BITFILE_BUFFER_SIZE : remain_size;

		chunk = (index == 0) ? chunk : (chunk - 4);

		err = fread(filep, offset, (uint8_t *)&buf[index/4], chunk);
		if (err < 0)
			goto CLEANUP;

		for (loop = 0; loop < ((chunk+index) / 4); loop++) {
			pci_write_config_dword(pdev, wr_offset,
						cpu_to_be32(buf[loop]));

			/* FROM SDAccel Code:
			* This delay resolves the MIG calibration issues
			* we have been seeing with Tandem Stage 2 Loading
			*/
			udelay(1);
		}


		index = 0;
		offset = offset + chunk;
		remain_size = remain_size - chunk;
	}

CLEANUP:
	kfree(buf);
	return offset;
}

static int xvsec_write_bin(struct context *xvsec_ctx,
	struct file *filep, loff_t size)
{
	int err;
	uint32_t loop;
	uint64_t offset = 0x0;
	uint32_t *buf = NULL;
	uint16_t chunk = 0;
	uint16_t wr_offset;
	loff_t remain_size = 0;
	struct pci_dev *pdev = xvsec_ctx->pdev;


	buf = kmalloc(DMA_HWICAP_BITFILE_BUFFER_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -(ENOMEM);

	remain_size = size;
	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;
	while (remain_size != 0) {
		chunk = (remain_size > DMA_HWICAP_BITFILE_BUFFER_SIZE) ?
			DMA_HWICAP_BITFILE_BUFFER_SIZE : remain_size;


		err = fread(filep, offset, (uint8_t *)&buf[0], chunk);
		if (err < 0)
			goto CLEANUP;

		for (loop = 0; loop < (chunk / 4); loop++) {
			pci_write_config_dword(pdev, wr_offset,
						cpu_to_be32(buf[loop]));

			/* FROM SDAccel Code:
			* This delay resolves the MIG calibration issues
			* we have been seeing with Tandem Stage 2 Loading
			*/
			udelay(1);
		}

		offset = offset + chunk;
		remain_size = remain_size - chunk;
	}
CLEANUP:
	kfree(buf);
	return err;
}

static int xvsec_mcap_program(struct context *xvsec_ctx, char *fname)
{
	int ret = 0;
	loff_t file_size;
	struct file *filep;
	uint32_t sts_data;

	pr_info("Before fopen\n");
	pr_info("file name : %p\n", fname);
	filep = fopen(fname, O_RDONLY, 0);
	if (filep == NULL)
		return -(ENOENT);

	pr_info("After fopen\n");

	ret = get_file_size(fname, &file_size);
	if (ret < 0)
		goto CLEANUP;

	pr_info("After getsize\n");

	if (file_size <= 0) {
		ret = -(EINVAL);
		goto CLEANUP;
	}

	if (find_file_type(fname, MCAP_RBT_FILE) == 0) {
		ret = xvsec_write_rbt(xvsec_ctx, filep, file_size);
		pr_info("xvsec_write_rbt : output : 0x%X\n", ret);
		if (ret < 0)
			goto CLEANUP;
	} else if (find_file_type(fname, MCAP_BIT_FILE) == 0) {
		ret = xvsec_write_bit(xvsec_ctx, filep, file_size);
		if (ret < 0)
			goto CLEANUP;
	} else if (find_file_type(fname, MCAP_BIN_FILE) == 0) {
		ret = xvsec_write_bin(xvsec_ctx, filep, file_size);
		if (ret < 0)
			goto CLEANUP;
	}

	ret = check_for_completion(xvsec_ctx, &sts_data);
	if ((ret != 0) ||
		((sts_data & XVSEC_MCAP_STATUS_ERR) != 0x0) ||
		((sts_data & XVSEC_MCAP_STATUS_FIFO_OVFL) != 0x0)) {
		pr_err("Performing Full Reset\n");
		xvsec_mcap_full_reset(xvsec_ctx);
		ret = -(EIO);
	}

CLEANUP:
	fclose(filep);
	return ret;

}

static int xvsec_mcap_rd_cfg_addr(struct context *xvsec_ctx,
	struct cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	address = xvsec_ctx->mcap_cap_offset + data->offset;
	switch (data->access) {
	case 'b':
		ret = pci_read_config_byte(pdev, address, &byte_data);
		if (ret == 0)
			data->data = byte_data;
		break;
	case 'h':
		ret = pci_read_config_word(pdev, address, &short_data);
		if (ret == 0)
			data->data = short_data;
		break;
	case 'w':
		ret = pci_read_config_dword(pdev, address, &word_data);
		if (ret == 0)
			data->data = word_data;
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

static int xvsec_mcap_wr_cfg_addr(struct context *xvsec_ctx,
	struct cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	address = xvsec_ctx->mcap_cap_offset + data->offset;

	switch (data->access) {
	case 'b':
		byte_data = (uint8_t)data->data;
		ret = pci_write_config_byte(pdev, address, byte_data);
		break;
	case 'h':
		short_data = (uint16_t)data->data;
		ret = pci_write_config_word(pdev, address, short_data);
		break;
	case 'w':
		word_data = (uint32_t)data->data;
		ret = pci_write_config_dword(pdev, address, word_data);
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

static int xvsec_fpga_rd_cfg_addr(struct context *xvsec_ctx,
	struct fpga_cfg_reg *cfg_reg)
{
	int ret = 0;
	int wr_offset, ctrl_offset, status_offset;
	int rd_offset;
	int i, n;
	uint32_t rd_data, ctrl_data, status_data, restore;
	uint16_t count;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	i = 0x0;
	n = ARRAY_SIZE(fpga_valid_addr);

	while ((i < n) && (cfg_reg->offset != fpga_valid_addr[i++]))
		;

	if ((i == n) && (cfg_reg->offset != fpga_valid_addr[n - 1])) {
		pr_err("Invalid FPGA Register Access : ");
		pr_err("Addr : 0x%X\n", cfg_reg->offset);
		return -(EACCES);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;
	rd_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_READ_DATA_REG0;
	ctrl_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	status_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(xvsec_ctx, FPGA_RD_CMD);

	xvsec_fpga_cfg_write_cmd(xvsec_ctx, FPGA_RD_CMD, cfg_reg->offset, 1);

	ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_RD_ENABLE;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	count = 0x0;
	while (count < 20) {
		pci_read_config_dword(pdev, status_offset, &status_data);
		if ((status_data & 0x10) != 0x0)
			break;
		count = count + 1;
		msleep(20);
	}

	if (count >= 20) {
		pr_err("Time out happened while reading FPGA CFG Register\n");
		ret = -(EBUSY);
		goto CLEANUP;
	}

	pci_read_config_dword(pdev, rd_offset, &rd_data);

	pr_info("%s : data : 0x%X\n", __func__, rd_data);

	cfg_reg->data = rd_data;

	ctrl_data = ctrl_data & (~XVSEC_MCAP_CTRL_RD_ENABLE);
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);


CLEANUP:
	xvsec_fpga_cfg_teardown(xvsec_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_fpga_wr_cfg_addr(struct context *xvsec_ctx,
	struct fpga_cfg_reg *cfg_reg)
{
	int ret = 0;
	int wr_offset, ctrl_offset, status_offset;
	int rd_offset;
	int i, n;
	uint32_t ctrl_data, restore;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	i = 0x0;
	n = ARRAY_SIZE(fpga_valid_addr);

	while ((i < n) && (cfg_reg->offset != fpga_valid_addr[i++]))
		;

	if ((i == n) && (cfg_reg->offset != fpga_valid_addr[n - 1])) {
		pr_err("Invalid FPGA Register Access : ");
		pr_err("Addr : 0x%X\n", cfg_reg->offset);
		return -(EACCES);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(xvsec_ctx, &restore);
	if (ret < 0)
		return ret;

	wr_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_WRITE_DATA_REG;
	rd_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_READ_DATA_REG0;
	ctrl_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_CONTROL_REGISTER;
	status_offset = xvsec_ctx->mcap_cap_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(xvsec_ctx, FPGA_WR_CMD);

	xvsec_fpga_cfg_write_cmd(xvsec_ctx, FPGA_WR_CMD, cfg_reg->offset, 1);

	xvsec_fpga_cfg_write_data(xvsec_ctx, cfg_reg->data);

	xvsec_fpga_cfg_teardown(xvsec_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

static int xvsec_get_dev_info(struct context *xvsec_ctx,
	struct device_info *dev_info)
{
	int ret = 0;
	struct pci_dev *pdev = xvsec_ctx->pdev;

	dev_info->vendor_id		= pdev->vendor;
	dev_info->device_id		= pdev->device;
	dev_info->device_no		= PCI_SLOT(pdev->devfn);
	dev_info->device_fn		= pdev->devfn & 0x7;
	dev_info->subsystem_vendor	= pdev->subsystem_vendor;
	dev_info->subsystem_device	= pdev->subsystem_device;
	dev_info->class_id		= pdev->class;
	dev_info->cfg_size		= pdev->cfg_size;
	dev_info->is_msi_enabled	= pdev->msi_enabled;
	dev_info->is_msix_enabled	= pdev->msix_enabled;

	return ret;
}

static int xvsec_cdev_create(struct context *xvsec_ctx)
{
	int ret = 0;
	struct pci_dev *pdev = xvsec_ctx->pdev;
	uint8_t bus_no = pdev->bus->number;
	uint8_t dev_no = PCI_SLOT(pdev->devfn);
	char cdev_name[10];

	ret = alloc_chrdev_region(&xvsec_ctx->dev_no,
		XVSEC_MINOR_BASE, XVSEC_MINOR_COUNT, XVSEC_NODE_NAME);
	if (ret < 0) {
		pr_err("Major number allocation is failed\n");
		goto CLEANUP1;
	}

	xvsec_ctx->major_no =
		MAJOR(xvsec_ctx->dev_no);

	pr_info("The major number is %d, bus no : %d, dev no : %d\n",
		xvsec_ctx->major_no, bus_no, dev_no);

	cdev_init(&xvsec_ctx->cdev, &xvsec_fops);

	ret = cdev_add(&xvsec_ctx->cdev, xvsec_ctx->dev_no, 1);
	if (ret < 0) {
		pr_err("Unable to add cdev\n");
		goto CLEANUP2;
	}

	snprintf(cdev_name, 10, "%s%02X%02X", XVSEC_NODE_NAME, bus_no, dev_no);
	kobject_set_name(&xvsec_ctx->cdev.kobj, cdev_name);

	xvsec_ctx->sys_device =
		device_create(g_xvsec_class, NULL,
			xvsec_ctx->dev_no, NULL, cdev_name);

	if (IS_ERR(xvsec_ctx->sys_device)) {
		pr_err("failed to create device");
		ret = -(PTR_ERR(xvsec_ctx->sys_device));
		goto CLEANUP3;
	}

	return ret;

CLEANUP3:
	cdev_del(&xvsec_ctx->cdev);
CLEANUP2:
	unregister_chrdev_region(xvsec_ctx->dev_no,
		XVSEC_MINOR_COUNT);
CLEANUP1:
	return ret;

}


int xvsec_initialize(struct pci_dev *pdev, uint32_t dev_idx)
{
	int ret = 0;

	if ((pdev == NULL) || (dev_idx >= xvsec_dev.dev_cnt))
		return -(EINVAL);

	xvsec_dev.ctx[dev_idx].pdev = pdev;

	ret = xvsec_get_vsec_capabilities(&xvsec_dev.ctx[dev_idx]);
	if (ret < 0) {
		pr_err("Error In retrieving VSEC capabilities :");
		pr_err(" err code : %d\n", ret);
		return ret;
	}

	ret = xvsec_cdev_create(&xvsec_dev.ctx[dev_idx]);
	if (ret < 0)
		return ret;



	return ret;
}
EXPORT_SYMBOL_GPL(xvsec_initialize);

int xvsec_deinitialize(uint32_t dev_idx)
{
	int ret = 0;
	struct context *xvsec_ctx = &xvsec_dev.ctx[dev_idx];

	if (dev_idx >= xvsec_dev.dev_cnt)
		return -(EINVAL);

	device_destroy(g_xvsec_class, xvsec_ctx->dev_no);
	cdev_del(&xvsec_ctx->cdev);
	unregister_chrdev_region(xvsec_ctx->dev_no, 1);

	return ret;
}
EXPORT_SYMBOL_GPL(xvsec_deinitialize);

MODULE_LICENSE("Dual BSD/GPL");
