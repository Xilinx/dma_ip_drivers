/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2020,  Xilinx, Inc.
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

#include "xvsec_drv.h"
#include "xvsec_drv_int.h"
#include "xvsec_cdev.h"
#include "xvsec_mcap.h"
#include "xvsec_mcap_us.h"
#include "xvsec_mcap_versal.h"

/**
 * XVSEC-MCAP character device name
 */
#define XVSEC_MCAP_CDEV_NAME		"mcap"

enum mcap_revison {
	XVSEC_MCAP_US_REV = 0,
	XVSEC_MCAP_USPLUS_REV = 1,
	XVSEC_MCAP_VERSAL = 2,
	XVSEC_MCAP_MAX_REV
};

struct file_priv_mcap {
	void *ctx;
};

struct mcap_ioctl_ops {
	uint32_t cmd;
	long (*fpfunction)(struct file *filep,
		uint32_t cmd, unsigned long arg);
};

struct mcap_fops {
	int (*reset)(struct vsec_context *mcap_ctx);
	int (*module_reset)(struct vsec_context *mcap_ctx);
	int (*full_reset)(struct vsec_context *mcap_ctx);
	int (*get_revision)(struct vsec_context *mcap_ctx,
		uint16_t *vsec_id, uint16_t *rev_id);
	int (*get_data_regs)(struct vsec_context *mcap_ctx,
		uint32_t regs[4]);
	int (*get_regs)(struct vsec_context *mcap_ctx,
		union mcap_regs *regs);
	int (*get_fpga_regs)(struct vsec_context *mcap_ctx,
		union fpga_cfg_regs *regs);
	int (*program_bitstream)(struct vsec_context *mcap_ctx,
		union bitstream_file *bit_files);
	int (*rd_cfg_addr)(struct vsec_context *mcap_ctx,
		union cfg_data *data);
	int (*wr_cfg_addr)(struct vsec_context *mcap_ctx,
		union cfg_data *data);
	int (*fpga_rd_cfg_addr)(struct vsec_context *mcap_ctx,
		union fpga_cfg_reg *cfg_reg);
	int (*fpga_wr_cfg_addr)(struct vsec_context *mcap_ctx,
		union fpga_cfg_reg *cfg_reg);
	int (*axi_rd_addr)(struct vsec_context *mcap_ctx,
		union axi_reg_data *cfg_reg);
	int (*axi_wr_addr)(struct vsec_context *mcap_ctx,
		union axi_reg_data *cfg_reg);
	int (*file_download)(struct vsec_context *mcap_ctx,
		union file_download_upload *file);
	int (*file_upload)(struct vsec_context *mcap_ctx,
		union file_download_upload *file);
	int (*set_axi_cache_attr)(struct vsec_context *mcap_ctx,
		union axi_cache_attr *attr);
};

struct mcap_priv_ctx {
	struct mcap_fops fops;
	uint16_t vsec_id;
	uint16_t rev_id;
};

static int xvsec_mcap_open(struct inode *inode,
	struct file *filep);
static int xvsec_mcap_close(struct inode *inode,
	struct file *filep);
static long xvsec_mcap_ioctl(struct file *filep,
	uint32_t cmd, unsigned long arg);

static long xvsec_ioc_mcap_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_mcap_module_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_mcap_full_reset(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_get_mcap_revision(struct file *filep,
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
static long xvsec_ioc_read_axi_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_write_axi_reg(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_file_download(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_file_upload(struct file *filep,
	uint32_t cmd, unsigned long arg);
static long xvsec_ioc_set_axi_attr(struct file *filep,
	uint32_t cmd, unsigned long arg);

static int xvsec_mcap_get_revision(struct vsec_context *mcap_ctx,
	uint16_t *vsec_id, uint16_t *rev_id);

static int xvsec_mcap_open(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct vsec_context	*mcap_ctx;
	struct file_priv_mcap	*priv;

	mcap_ctx = container_of(inode->i_cdev,
			struct vsec_context, char_dev.cdev);

	pr_info("%s: mcap_ctx address : %p\n", __func__, mcap_ctx);

	spin_lock(&mcap_ctx->lock);

	if (mcap_ctx->fopen_cnt != 0) {
		ret = -(EBUSY);
		goto CLEANUP;
	}

	priv = kzalloc(sizeof(struct file_priv_mcap), GFP_KERNEL);
	if (priv == NULL) {
		ret = -(ENOMEM);
		goto CLEANUP;
	}

	mcap_ctx->fopen_cnt++;
	priv->ctx = (void *)mcap_ctx;
	filep->private_data = priv;

	pr_debug("%s success\n", __func__);

CLEANUP:
	spin_unlock(&mcap_ctx->lock);
	return	ret;
}

static int xvsec_mcap_close(struct inode *inode, struct file *filep)
{
	struct file_priv_mcap	*priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;

	spin_lock(&mcap_ctx->lock);

	if (mcap_ctx->fopen_cnt == 0) {
		pr_warn("File Open/close mismatch\n");
	} else {
		mcap_ctx->fopen_cnt--;
		pr_debug("%s success\n", __func__);
	}
	kfree(priv);

	spin_unlock(&mcap_ctx->lock);

	return 0;
}

static const struct mcap_ioctl_ops mcap_ioctl_ops[] = {
	{IOC_MCAP_RESET,		xvsec_ioc_mcap_reset},
	{IOC_MCAP_MODULE_RESET,		xvsec_ioc_mcap_module_reset},
	{IOC_MCAP_FULL_RESET,		xvsec_ioc_mcap_full_reset},
	{IOC_MCAP_GET_REVISION,		xvsec_ioc_get_mcap_revision},
	{IOC_MCAP_GET_DATA_REGISTERS,	xvsec_ioc_get_data_regs},
	{IOC_MCAP_GET_REGISTERS,	xvsec_ioc_get_regs},
	{IOC_MCAP_GET_FPGA_REGISTERS,	xvsec_ioc_get_fpga_regs},
	{IOC_MCAP_PROGRAM_BITSTREAM,	xvsec_ioc_prog_bitstream},
	{IOC_MCAP_READ_DEV_CFG_REG,	xvsec_ioc_rd_dev_cfg_reg},
	{IOC_MCAP_WRITE_DEV_CFG_REG,	xvsec_ioc_wr_dev_cfg_reg},
	{IOC_MCAP_READ_FPGA_CFG_REG,	xvsec_ioc_rd_fpga_cfg_reg},
	{IOC_MCAP_WRITE_FPGA_CFG_REG,	xvsec_ioc_wr_fpga_cfg_reg},
	{IOC_MCAP_READ_AXI_REG,		xvsec_ioc_read_axi_reg},
	{IOC_MCAP_WRITE_AXI_REG,	xvsec_ioc_write_axi_reg},
	{IOC_MCAP_FILE_DOWNLOAD,	xvsec_ioc_file_download},
	{IOC_MCAP_FILE_UPLOAD,		xvsec_ioc_file_upload},
	{IOC_MCAP_SET_AXI_ATTR,		xvsec_ioc_set_axi_attr},
};

static long xvsec_ioc_mcap_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;

	pr_debug("ioctl : IOC_MCAP_RESET\n");

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->reset(mcap_ctx);
	spin_unlock(&mcap_ctx->lock);

	return ret;

}

static long xvsec_ioc_mcap_module_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;

	pr_debug("ioctl : IOC_MCAP_MODULE_RESET\n");

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->module_reset(mcap_ctx);
	spin_unlock(&mcap_ctx->lock);

	return ret;
}

static long xvsec_ioc_mcap_full_reset(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;

	pr_debug("ioctl : IOC_MCAP_FULL_RESET\n");

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->full_reset(mcap_ctx);
	spin_unlock(&mcap_ctx->lock);

	return ret;
}

static long xvsec_ioc_get_mcap_revision(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	uint16_t vsec_id = 0;
	uint16_t rev_id = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;

	pr_debug("ioctl : IOC_MCAP_GET_REVISION\n");

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->get_revision(mcap_ctx, &vsec_id, &rev_id);
	spin_unlock(&mcap_ctx->lock);

	if (ret == 0) {
		pr_debug("vsec_id: %d, rev_id: %d\n", vsec_id, rev_id);
		ret = copy_to_user((void __user *)arg,
			(void *)&rev_id, sizeof(uint16_t));
	}

	return ret;
}

static long xvsec_ioc_get_data_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	uint32_t read_data_reg[XVSEC_MCAP_DATA_REG_CNT];

	pr_debug("ioctl : IOC_MCAP_GET_DATA_REGISTERS\n");

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->get_data_regs(mcap_ctx, read_data_reg);
	spin_unlock(&mcap_ctx->lock);

	memset(read_data_reg, 0, sizeof(read_data_reg));
	if (ret == 0) {
		ret = copy_to_user((void __user *)arg, (void *)read_data_reg,
			sizeof(uint32_t)*XVSEC_MCAP_DATA_REG_CNT);
	}

	return ret;

}

static long xvsec_ioc_get_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union mcap_regs mcap_regs;

	pr_debug("ioctl : IOC_MCAP_GET_REGISTERS\n");

	memset(&mcap_regs, 0, sizeof(union mcap_regs));

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->get_regs(mcap_ctx, &mcap_regs);
	spin_unlock(&mcap_ctx->lock);

	if (ret == 0) {
		ret = copy_to_user((void __user *)arg,
			(void *)&mcap_regs, sizeof(union mcap_regs));
	}

	return ret;
}

static long xvsec_ioc_get_fpga_regs(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union fpga_cfg_regs	fpga_cfg_regs;

	pr_debug("ioctl : IOC_MCAP_GET_FPGA_REGISTERS\n");

	memset(&fpga_cfg_regs, 0, sizeof(union fpga_cfg_regs));

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->get_fpga_regs(mcap_ctx, &fpga_cfg_regs);
	spin_unlock(&mcap_ctx->lock);

	if (ret == 0) {
		ret = copy_to_user((void __user *)arg,
			(void *)&fpga_cfg_regs,
			sizeof(union fpga_cfg_regs));
	}

	return ret;
}

static long xvsec_ioc_prog_bitstream(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union bitstream_file	bit_files;

	pr_debug("ioctl : IOC_MCAP_PROGRAM_BITSTREAM\n");
	ret = copy_from_user(&bit_files, (void __user *)arg,
		sizeof(union bitstream_file));

	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->program_bitstream(mcap_ctx, &bit_files);
	spin_unlock(&mcap_ctx->lock);

	if (ret < 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg, (void *)&bit_files,
		sizeof(union bitstream_file));

CLEANUP:
	return ret;
}

static long xvsec_ioc_rd_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union cfg_data	rw_cfg_data;

	pr_debug("ioctl : IOC_MCAP_READ_DEV_CFG_REG\n");

	ret = copy_from_user(&rw_cfg_data,
		(void __user *)arg, sizeof(union cfg_data));

	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->rd_cfg_addr(mcap_ctx, &rw_cfg_data);
	spin_unlock(&mcap_ctx->lock);

	if (ret < 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&rw_cfg_data, sizeof(union cfg_data));

CLEANUP:
	return ret;
}

static long xvsec_ioc_wr_dev_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union cfg_data	rw_cfg_data;

	pr_debug("ioctl : IOC_MCAP_WRITE_DEV_CFG_REG\n");

	ret = copy_from_user(&rw_cfg_data,
		(void __user *)arg, sizeof(union cfg_data));

	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->wr_cfg_addr(mcap_ctx, &rw_cfg_data);
	spin_unlock(&mcap_ctx->lock);

CLEANUP:
	return ret;

}

static long xvsec_ioc_rd_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
		(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union fpga_cfg_reg fpga_cfg_data;

	pr_debug("ioctl : IOC_MCAP_READ_FPGA_CFG_REG\n");
	ret = copy_from_user(&fpga_cfg_data,
		(void __user *)arg, sizeof(union fpga_cfg_reg));

	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->fpga_rd_cfg_addr(mcap_ctx, &fpga_cfg_data);
	spin_unlock(&mcap_ctx->lock);

	if (ret != 0)
		goto CLEANUP;

	ret = copy_to_user((void __user *)arg,
		(void *)&fpga_cfg_data, sizeof(union fpga_cfg_reg));
CLEANUP:
	return ret;
}

static long xvsec_ioc_wr_fpga_cfg_reg(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union fpga_cfg_reg fpga_cfg_data;

	pr_debug("ioctl : IOC_MCAP_WRITE_FPGA_CFG_REG\n");

	ret = copy_from_user(&fpga_cfg_data,
		(void __user *)arg, sizeof(union fpga_cfg_reg));

	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->fpga_wr_cfg_addr(mcap_ctx, &fpga_cfg_data);
	spin_unlock(&mcap_ctx->lock);
CLEANUP:
	return ret;
}

static long xvsec_ioc_read_axi_reg(struct file *filep,
		uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union axi_reg_data axi_rd_info;

	pr_debug("ioctl : IOC_MCAP_READ_AXI_REG\n");

	ret = copy_from_user(&axi_rd_info,
		(void __user *)arg, sizeof(union axi_reg_data));
	if (ret != 0)
		goto CLEANUP;


	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->axi_rd_addr(mcap_ctx, &axi_rd_info);
	spin_unlock(&mcap_ctx->lock);

	if (ret == 0) {
		ret = copy_to_user((void __user *)arg,
			(void *)&axi_rd_info, sizeof(union axi_reg_data));
	}

CLEANUP:
	return ret;
}

static long xvsec_ioc_write_axi_reg(struct file *filep,
		uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx = (
			struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union axi_reg_data axi_wr_info;

	pr_debug("ioctl : IOC_MCAP_WRITE_AXI_REG\n");

	ret = copy_from_user(&axi_wr_info,
		(void __user *)arg, sizeof(union axi_reg_data));
	if (ret != 0)
		goto CLEANUP;


	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->axi_wr_addr(mcap_ctx, &axi_wr_info);
	spin_unlock(&mcap_ctx->lock);

CLEANUP:
	return ret;
}

static long xvsec_ioc_file_download(struct file *filep,
		uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	int rv = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union file_download_upload file_args;

	pr_debug("ioctl : IOC_MCAP_FILE_DOWNLOAD\n");

	ret = copy_from_user(&file_args,
			(void __user *)arg, sizeof(union file_download_upload));
	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->file_download(mcap_ctx, &file_args);
	spin_unlock(&mcap_ctx->lock);

	rv = copy_to_user((void __user *)arg, (void *)&file_args,
			sizeof(union file_download_upload));

	if (rv != 0)
		ret = rv;

CLEANUP:
	return ret;
}

static long xvsec_ioc_file_upload(struct file *filep,
		uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	int rv = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union file_download_upload file_args;

	pr_debug("ioctl : IOC_MCAP_FILE_UPLOAD\n");

	ret = copy_from_user(&file_args,
		(void __user *)arg, sizeof(union file_download_upload));
	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->file_upload(mcap_ctx, &file_args);
	spin_unlock(&mcap_ctx->lock);

	rv = copy_to_user((void __user *)arg, (void *)&file_args,
			sizeof(union file_download_upload));

	if (rv != 0)
		ret = rv;

CLEANUP:
	return ret;
}

static long xvsec_ioc_set_axi_attr(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	struct file_priv_mcap *priv = filep->private_data;
	struct vsec_context *mcap_ctx = (struct vsec_context *)priv->ctx;
	struct mcap_priv_ctx *mcap_priv_ctx =
		(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;
	struct mcap_fops *mcap_fops = (struct mcap_fops *)&mcap_priv_ctx->fops;
	union axi_cache_attr axi_attr_info;

	pr_debug("ioctl : IOC_MCAP_SET_AXI_ATTR\n");

	ret = copy_from_user(&axi_attr_info,
			(void __user *)arg, sizeof(union axi_cache_attr));
	if (ret != 0)
		goto CLEANUP;

	spin_lock(&mcap_ctx->lock);
	ret = mcap_fops->set_axi_cache_attr(mcap_ctx, &axi_attr_info);
	spin_unlock(&mcap_ctx->lock);

	if (ret != 0)
		goto CLEANUP;

CLEANUP:
	return ret;
}

static long xvsec_mcap_ioctl(struct file *filep,
	uint32_t cmd, unsigned long arg)
{
	int ret = 0;
	int index, cmd_cnt;
	bool cmd_found = false;

	cmd_cnt = ARRAY_SIZE(mcap_ioctl_ops);
	for (index = 0; index < cmd_cnt; index++) {
		if (mcap_ioctl_ops[index].cmd == cmd) {
			cmd_found = true;
			ret = mcap_ioctl_ops[index].fpfunction(filep, cmd, arg);
			break;
		}
	}
	if (cmd_found == false)
		ret = -(EINVAL);

	return ret;
}

static int xvsec_mcap_get_revision(struct vsec_context *mcap_ctx,
	uint16_t *vsec_id, uint16_t *rev_id)
{
	int rv = 0;
	uint16_t vendor_offset;
	uint32_t vendor_data;
	struct pci_dev *pdev = mcap_ctx->pdev;
	static bool is_rev_avail;
	struct mcap_priv_ctx *mcap_priv_ctx =
			(struct mcap_priv_ctx *)mcap_ctx->vsec_priv;

	if (is_rev_avail == false) {
		vendor_offset =
			mcap_ctx->vsec_offset + XVSEC_MCAP_VENDOR_HEADER;

		rv = pci_read_config_dword(pdev, vendor_offset, &vendor_data);

		mcap_priv_ctx->vsec_id =
			(vendor_data & XVSEC_MCAP_VSEC_ID_POS) >>
			XVSEC_MCAP_VSEC_ID_SHIFT;
		mcap_priv_ctx->rev_id =
			(vendor_data & XVSEC_MCAP_REV_ID_POS) >>
			XVSEC_MCAP_REV_ID_SHIFT;

		is_rev_avail = true;
		pr_info("%s: Version details vsec_id:%d, rev_id: %d\n",
			__func__, *vsec_id, *rev_id);
	} else {
		*vsec_id = mcap_priv_ctx->vsec_id;
		*rev_id = mcap_priv_ctx->rev_id;

		pr_info("%s: vsec_id:%d, rev_id: %d\n",
			__func__, *vsec_id, *rev_id);
	}

	return rv;
}

static const struct file_operations xvsec_mcap_fops = {
	.owner		= THIS_MODULE,
	.open		= xvsec_mcap_open,
	.release	= xvsec_mcap_close,
	.unlocked_ioctl	= xvsec_mcap_ioctl,
};

int xvsec_mcap_module_init(struct vsec_context *mcap_ctx)
{
	int rv;
	struct mcap_priv_ctx *mcap_priv_ctx;
	struct mcap_fops *mcap_fops;

	pr_info("%s: mcap_ctx address : %p\n", __func__, mcap_ctx);

	spin_lock_init(&mcap_ctx->lock);
	mcap_priv_ctx = kzalloc(sizeof(struct mcap_priv_ctx), GFP_KERNEL);
	if (mcap_priv_ctx == NULL)
		return -(ENOMEM);

	/* Private context is getting used by
	 * xvsec_mcap_get_revision to save version info
	 */
	mcap_ctx->vsec_priv = (void *)mcap_priv_ctx;
	rv = xvsec_mcap_get_revision(mcap_ctx,
		&mcap_priv_ctx->vsec_id, &mcap_priv_ctx->rev_id);
	if (rv < 0) {
		pr_err("xvsec_mcap_get_version failed with error : %d\n", rv);
		goto CLEANUP;
	}

	if (mcap_priv_ctx->vsec_id != mcap_ctx->vsec_ops->vsec_id) {
		pr_err("VSEC ID Mismatch : Context VSEC %d, Actual VSEC : %d\n",
			mcap_ctx->vsec_ops->vsec_id, mcap_priv_ctx->vsec_id);
		rv = -(EPERM);
		goto CLEANUP;
	}

	if (mcap_priv_ctx->rev_id >= XVSEC_MCAP_MAX_REV) {
		pr_err("Valid MCAP Rev ID not found, rev_id : %d\n",
			mcap_priv_ctx->rev_id);
		rv = -(ENXIO);
		goto CLEANUP;
	}

	mcap_fops = &mcap_priv_ctx->fops;
	if ((mcap_priv_ctx->rev_id == XVSEC_MCAP_US_REV) ||
		(mcap_priv_ctx->rev_id == XVSEC_MCAP_USPLUS_REV)) {
		mcap_fops->reset = xvsec_mcap_reset;
		mcap_fops->module_reset = xvsec_mcap_module_reset;
		mcap_fops->full_reset = xvsec_mcap_full_reset;
		mcap_fops->get_revision = xvsec_mcap_get_revision;
		mcap_fops->get_data_regs = xvsec_mcap_get_data_regs;
		mcap_fops->get_regs = xvsec_mcap_get_regs;
		mcap_fops->get_fpga_regs = xvsec_mcap_get_fpga_regs;
		mcap_fops->program_bitstream = xvsec_mcap_program_bitstream;
		mcap_fops->rd_cfg_addr = xvsec_mcap_rd_cfg_addr;
		mcap_fops->wr_cfg_addr = xvsec_mcap_wr_cfg_addr;
		mcap_fops->fpga_rd_cfg_addr = xvsec_fpga_rd_cfg_addr;
		mcap_fops->fpga_wr_cfg_addr = xvsec_fpga_wr_cfg_addr;
		mcap_fops->axi_rd_addr = xvsec_mcapv1_axi_rd_addr;
		mcap_fops->axi_wr_addr = xvsec_mcapv1_axi_wr_addr;
		mcap_fops->file_download = xvsec_mcapv1_file_download;
		mcap_fops->file_upload	= xvsec_mcapv1_file_upload;
		mcap_fops->set_axi_cache_attr = xvsec_mcapv1_set_axi_cache_attr;


	} else if (mcap_priv_ctx->rev_id == XVSEC_MCAP_VERSAL) {
		mcap_fops->reset = xvsec_mcapv2_reset;
		mcap_fops->module_reset = xvsec_mcapv2_module_reset;
		mcap_fops->full_reset = xvsec_mcapv2_full_reset;
		mcap_fops->get_revision = xvsec_mcap_get_revision;
		mcap_fops->get_data_regs = xvsec_mcapv2_get_data_regs;
		mcap_fops->get_regs = xvsec_mcapv2_get_regs;
		mcap_fops->get_fpga_regs = xvsec_mcapv2_get_fpga_regs;
		mcap_fops->program_bitstream = xvsec_mcapv2_program_bitstream;
		mcap_fops->rd_cfg_addr = xvsec_mcapv2_rd_cfg_addr;
		mcap_fops->wr_cfg_addr = xvsec_mcapv2_wr_cfg_addr;
		mcap_fops->fpga_rd_cfg_addr = xvsec_fpgav2_rd_cfg_addr;
		mcap_fops->fpga_wr_cfg_addr = xvsec_fpgav2_wr_cfg_addr;
		mcap_fops->axi_rd_addr = xvsec_mcapv2_axi_rd_addr;
		mcap_fops->axi_wr_addr = xvsec_mcapv2_axi_wr_addr;
		mcap_fops->file_download = xvsec_mcapv2_file_download;
		mcap_fops->file_upload	= xvsec_mcapv2_file_upload;
		mcap_fops->set_axi_cache_attr = xvsec_mcapv2_set_axi_cache_attr;
	}

	rv = xvsec_cdev_create(mcap_ctx->pdev, &mcap_ctx->char_dev,
		&xvsec_mcap_fops, XVSEC_MCAP_CDEV_NAME);
	if (rv < 0) {
		pr_err("xvsec_cdev_create failed with error : %d\n", rv);
		goto CLEANUP;
	}

	return 0;

CLEANUP:
	kfree(mcap_ctx->vsec_priv);
	mcap_ctx->vsec_priv = NULL;
	return rv;
}

void xvsec_mcap_module_exit(struct vsec_context *mcap_ctx)
{
	pr_debug("%s\n", __func__);

	xvsec_cdev_remove(&mcap_ctx->char_dev);

	if (mcap_ctx->vsec_priv == NULL)
		pr_err("mcap_ctx->vsec_priv is NULL\n");

	/** Checkpatch : kfree(NULL) is safe */
	kfree(mcap_ctx->vsec_priv);
	mcap_ctx->vsec_priv = NULL;
}

MODULE_LICENSE("Dual BSD/GPL");
