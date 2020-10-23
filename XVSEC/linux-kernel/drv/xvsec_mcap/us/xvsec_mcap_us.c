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

#include "xvsec_util.h"
#include "xvsec_drv.h"
#include "xvsec_drv_int.h"
#include "xvsec_mcap.h"
#include "xvsec_mcap_us.h"



static const uint16_t fpga_valid_addr[] = {
		0x00, 0x01, 0x02, 0x03, 0x04,
		0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
		0x10, 0x11, 0x14, 0x16, 0x18,
		0x1F
	};


static void xvsec_fpga_cfg_setup(struct vsec_context *mcap_ctx,
	enum oper operation);
static void xvsec_fpga_cfg_teardown(struct vsec_context *mcap_ctx);
static void xvsec_fpga_cfg_write_cmd(struct vsec_context *mcap_ctx,
	enum oper operation, uint8_t offset, uint16_t word_count);
static void xvsec_fpga_cfg_write_data(struct vsec_context *mcap_ctx,
	uint32_t data);
static int xvsec_mcap_req_access(struct vsec_context *mcap_ctx,
	uint32_t *restore);
static int xvsec_mcap_program(struct vsec_context *mcap_ctx, char *fname);
static int xvsec_write_rbt(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size);
static int xvsec_write_bit(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size);
static int xvsec_write_bin(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size);

static int check_for_completion(struct vsec_context *mcap_ctx,
	uint32_t *ret);


static void xvsec_fpga_cfg_setup(struct vsec_context *mcap_ctx,
	enum oper operation)
{
	int wr_offset;
	int index;
	struct pci_dev *pdev = mcap_ctx->pdev;

	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

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

static void xvsec_fpga_cfg_teardown(struct vsec_context *mcap_ctx)
{
	int wr_offset;
	struct pci_dev *pdev = mcap_ctx->pdev;

	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

	pci_write_config_dword(pdev, wr_offset, TYPE1_WR_CMD);
	pci_write_config_dword(pdev, wr_offset, DESYNC);
	pci_write_config_dword(pdev, wr_offset, NOOP);
	pci_write_config_dword(pdev, wr_offset, NOOP);
}

static void xvsec_fpga_cfg_write_cmd(struct vsec_context *mcap_ctx,
	enum oper operation, uint8_t offset, uint16_t word_count)
{
	int wr_offset;
	union type1_header  header;
	struct pci_dev *pdev = mcap_ctx->pdev;

	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

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

static void xvsec_fpga_cfg_write_data(struct vsec_context *mcap_ctx,
	uint32_t data)
{
	int wr_offset;
	struct pci_dev *pdev = mcap_ctx->pdev;

	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

	pci_write_config_dword(pdev, wr_offset, data);
	pci_write_config_dword(pdev, wr_offset, NOOP);
	pci_write_config_dword(pdev, wr_offset, NOOP);
}

static int check_for_completion(struct vsec_context *mcap_ctx, uint32_t *ret)
{
	unsigned long retry_count = 0;
	int i;
	uint16_t sts_offset, data_off;
	uint32_t sts_data;
	struct pci_dev *pdev = mcap_ctx->pdev;

	sts_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_STATUS_REGISTER;
	data_off = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

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

static int xvsec_mcap_req_access(struct vsec_context *mcap_ctx,
	uint32_t *restore)
{
	int ret = 0;
	int delay = XVSEC_MCAP_LOOP_COUNT;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, sts_offset;
	uint32_t ctrl_data, sts_data;
	struct pci_dev *pdev = mcap_ctx->pdev;

	mcap_offset = mcap_ctx->vsec_offset;
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

int xvsec_mcap_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = mcap_ctx->vsec_offset;
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

int xvsec_mcap_module_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = mcap_ctx->vsec_offset;
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

int xvsec_mcap_full_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	mcap_offset = mcap_ctx->vsec_offset;
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

int xvsec_mcap_get_data_regs(struct vsec_context *mcap_ctx, uint32_t regs[4])
{
	uint16_t mcap_offset;
	struct pci_dev *pdev = mcap_ctx->pdev;

	mcap_offset = mcap_ctx->vsec_offset;
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

int xvsec_mcap_get_regs(struct vsec_context *mcap_ctx,
	union mcap_regs *regs)
{
	uint16_t mcap_offset;
	uint16_t index;
	uint32_t *ptr;
	struct pci_dev *pdev = mcap_ctx->pdev;

	regs->v1.valid = 0;

	mcap_offset = mcap_ctx->vsec_offset;
	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	ptr = &regs->v1.ext_cap_header;
	for (index = 0; index <= (XVSEC_MCAP_READ_DATA_REG3 / 4); index++) {
		pci_read_config_dword(pdev, mcap_offset, &ptr[index]);
		mcap_offset = mcap_offset + 4;
	}

	regs->v1.valid = 1;

	return 0;
}

int xvsec_mcap_get_fpga_regs(struct vsec_context *mcap_ctx,
	union fpga_cfg_regs *regs)
{
	int ret = 0;
	int index, reg_count;
	int ctrl_offset, sts_offset;
	int rd_offset;
	uint32_t rd_data, ctrl_data, sts_data, restore;
	uint16_t count;
	uint32_t *reg_dump;
	struct pci_dev *pdev = mcap_ctx->pdev;


	pr_info("In %s\n", __func__);

	regs->v1.valid = 0;
	regs->v1.far = 1;


	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	rd_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_READ_DATA_REG0;
	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_CONTROL_REGISTER;
	sts_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(mcap_ctx, FPGA_RD_CMD);

	reg_count = ARRAY_SIZE(fpga_valid_addr);
	/* sizeof(fpga_valid_addr)/sizeof(fpga_valid_addr[0]); */
	reg_dump = &regs->v1.crc;
	for (index = 0; index < reg_count; index++) {
		xvsec_fpga_cfg_write_cmd(mcap_ctx, FPGA_RD_CMD,
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

	if (ret == 0)
		regs->v1.valid = 1;

CLEANUP:
	xvsec_fpga_cfg_teardown(mcap_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

int xvsec_mcap_program_bitstream(struct vsec_context *mcap_ctx,
	union bitstream_file *bit_files)
{
	int ret = 0;
	uint16_t ctrl_offset;
	uint32_t ctrl_data, restore;
	char bitfile[MAX_FLEN];
	uint16_t len;
	struct pci_dev *pdev = mcap_ctx->pdev;

	bit_files->v1.status = MCAP_BITSTREAM_PROGRAM_FAILURE;

	if ((bit_files->v1.partial_clr_file == NULL) &&
		(bit_files->v1.bitstream_file == NULL)) {
		pr_err("Both Bit files are NULL\n");
		return -(EINVAL);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_CONTROL_REGISTER;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Reset */
	ctrl_data = ctrl_data | XVSEC_MCAP_CTRL_WR_ENABLE |
			XVSEC_MCAP_CTRL_ENABLE | XVSEC_MCAP_CTRL_REQ_ACCESS;


	ctrl_data = ctrl_data &
		~(XVSEC_MCAP_CTRL_RESET | XVSEC_MCAP_CTRL_MOD_RESET |
		XVSEC_MCAP_CTRL_RD_ENABLE | XVSEC_MCAP_CTRL_CFG_SWICTH);

	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	pr_info("Ctrl Data : 0x%X, 0x%X\n", ctrl_offset, ctrl_data);

	if (bit_files->v1.partial_clr_file != NULL) {
		len = strnlen_user(
			(char __user *)bit_files->v1.partial_clr_file,
			MAX_FLEN);
		if (len > MAX_FLEN) {
			pr_err("File Name too long\n");
			goto CLEANUP;
		}

		ret = strncpy_from_user(bitfile,
			(char __user *)bit_files->v1.partial_clr_file, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			goto CLEANUP;
		}
		pr_info("Clear File Name : %s\n", bitfile);

		ret = xvsec_mcap_program(mcap_ctx, bitfile);
		if (ret < 0) {
			pr_err("[xvsec_mcap] : xvsec_mcap_program ");
			pr_err("failed for partial clear file with err : ");
			pr_err("%d\n", ret);

			goto CLEANUP;
		}
	}

	if (bit_files->v1.bitstream_file != NULL) {
		len = strnlen_user(
			(char __user *)bit_files->v1.bitstream_file,
			MAX_FLEN);
		if (len > MAX_FLEN) {
			pr_err("File Name too long\n");
			goto CLEANUP;
		}

		ret = strncpy_from_user(bitfile,
			(char __user *)bit_files->v1.bitstream_file, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			goto CLEANUP;
		}

		pr_info("Bit File Name : %s\n", bitfile);

		ret = xvsec_mcap_program(mcap_ctx, bitfile);
		if (ret < 0) {
			pr_err("[xvsec_mcap] : xvsec_mcap_program ");
			pr_err("failed for bit file with err : %d\n", ret);
			goto CLEANUP;
		}
		restore = restore | XVSEC_MCAP_CTRL_CFG_SWICTH;
	}

	if (ret == 0)
		bit_files->v1.status = MCAP_BITSTREAM_PROGRAM_SUCCESS;

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
	memset(raw_buf, 0, RBT_WORD_LEN);
	while (size != 0) {
		chunk_size = (size > RBT_WORD_LEN) ? RBT_WORD_LEN : size;
		len = xvsec_util_fread(filep, search_off,
				(uint8_t *)raw_buf,
				chunk_size);

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
			len = xvsec_util_fread(filep,
				*offset, (uint8_t *)raw_buf, RBT_WORD_LEN);
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

static int xvsec_write_rbt(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size)
{
	int status;
	char bitdata[RBT_WORD_LEN + 1], dummy_data = 0;
	uint16_t wr_offset;
	uint32_t *buf, buf_index;
	int i, len, offset = 0, remain_size, dummy_len;
	uint32_t worddata, chunk_size;
	bool done;
	struct pci_dev *pdev = mcap_ctx->pdev;

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
	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;

	memset(bitdata, 0, (RBT_WORD_LEN + 1));
	while (done == false) {
		chunk_size = (remain_size > (RBT_WORD_LEN + 1)) ?
			(RBT_WORD_LEN + 1) : remain_size;
		len = xvsec_util_fread(filep, offset,
				(uint8_t *)bitdata,
				chunk_size);
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
				dummy_len = xvsec_util_fread(filep,
					offset, (uint8_t *)&dummy_data, 1);
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

static int xvsec_write_bit(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size)
{
	int err;
	uint8_t val = 0, len = 0;
	uint32_t index, loop;
	uint64_t offset = 0x0;
	uint32_t *buf = NULL;
	uint16_t chunk = 0;
	uint16_t wr_offset;
	loff_t remain_size = 0;
	bool	sync_found = false;
	struct pci_dev *pdev = mcap_ctx->pdev;

	/*
	 * .bit files are not guaranteed to be aligned with
	 * the bitstream sync word on a 32-bit boundary. So,
	 * we need to check every byte here.
	 */
	while (xvsec_util_fread(filep, offset, &val, 1) == 1) {
		if (offset >= size) {
			pr_err("[xvsec_cdev] : Reached End of BIT file");
			pr_err(" Failed to find the sync word\n");
			return -(EINVAL);
		}
		len++; offset++;
		if ((val == MCAP_SYNC_BYTE0) &&
			(xvsec_util_fread(filep,
				offset, &val, 1)) == 1) {
			len++; offset++;
			if ((val == MCAP_SYNC_BYTE1) &&
				(xvsec_util_fread(filep,
					offset, &val, 1)) == 1) {
				len++; offset++;
				if ((val == MCAP_SYNC_BYTE2) &&
					(xvsec_util_fread(filep,
						offset, &val, 1)) == 1) {
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

	buf[0] = (uint32_t)cpu_to_be32(MCAP_SYNC_DWORD);

	remain_size = size - len;
	index = 4;
	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;
	while (remain_size != 0) {
		chunk = (remain_size > DMA_HWICAP_BITFILE_BUFFER_SIZE) ?
			DMA_HWICAP_BITFILE_BUFFER_SIZE : remain_size;

		chunk = (index == 0) ? chunk : (chunk - 4);

		err = xvsec_util_fread(filep,
			offset, (uint8_t *)&buf[index/4], chunk);
		if (err < 0)
			goto CLEANUP;

		for (loop = 0; loop < ((chunk+index) / 4); loop++) {
			pci_write_config_dword(pdev, wr_offset,
				(uint32_t)cpu_to_be32(buf[loop]));

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

static int xvsec_write_bin(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size)
{
	int err = 0;
	uint32_t loop;
	uint64_t offset = 0x0;
	uint32_t *buf;
	uint16_t chunk = 0;
	uint16_t wr_offset;
	loff_t remain_size = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;


	buf = kmalloc(DMA_HWICAP_BITFILE_BUFFER_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -(ENOMEM);

	memset(buf, 0, DMA_HWICAP_BITFILE_BUFFER_SIZE);
	remain_size = size;
	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_WRITE_DATA_REG;
	while (remain_size != 0) {
		chunk = (remain_size > DMA_HWICAP_BITFILE_BUFFER_SIZE) ?
			DMA_HWICAP_BITFILE_BUFFER_SIZE : remain_size;


		err = xvsec_util_fread(filep,
			offset, (uint8_t *)&buf[0], chunk);
		if (err < 0)
			goto CLEANUP;

		for (loop = 0; loop < (chunk / 4); loop++) {
			pci_write_config_dword(pdev, wr_offset,
				(uint32_t)cpu_to_be32(buf[loop]));

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

static int xvsec_mcap_program(struct vsec_context *mcap_ctx, char *fname)
{
	int ret = 0;
	loff_t file_size;
	struct file *filep;
	uint32_t sts_data;

	pr_info("Before fopen\n");
	pr_info("file name : %p\n", fname);
	filep = xvsec_util_fopen(fname, O_RDONLY, 0);
	if (filep == NULL)
		return -(ENOENT);

	pr_info("After fopen\n");

	ret = xvsec_util_get_file_size(fname, &file_size);
	if (ret < 0)
		goto CLEANUP;

	pr_info("After getsize\n");

	if (file_size <= 0) {
		ret = -(EINVAL);
		goto CLEANUP;
	}

	if (xvsec_util_find_file_type(fname, MCAP_RBT_FILE) == 0) {
		ret = xvsec_write_rbt(mcap_ctx, filep, file_size);
		pr_info("xvsec_write_rbt : output : 0x%X\n", ret);
		if (ret < 0)
			goto CLEANUP;
	} else if (xvsec_util_find_file_type(fname, MCAP_BIT_FILE) == 0) {
		ret = xvsec_write_bit(mcap_ctx, filep, file_size);
		if (ret < 0)
			goto CLEANUP;
	} else if (xvsec_util_find_file_type(fname, MCAP_BIN_FILE) == 0) {
		ret = xvsec_write_bin(mcap_ctx, filep, file_size);
		if (ret < 0)
			goto CLEANUP;
	}

	ret = check_for_completion(mcap_ctx, &sts_data);
	if ((ret != 0) ||
		((sts_data & XVSEC_MCAP_STATUS_ERR) != 0x0) ||
		((sts_data & XVSEC_MCAP_STATUS_FIFO_OVFL) != 0x0)) {
		pr_err("Performing Full Reset\n");
		xvsec_mcap_full_reset(mcap_ctx);
		ret = -(EIO);
	}

CLEANUP:
	xvsec_util_fclose(filep);
	return ret;

}

int xvsec_mcap_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	address = mcap_ctx->vsec_offset + data->v1.offset;
	switch (data->v1.access) {
	case 'b':
		ret = pci_read_config_byte(pdev, address, &byte_data);
		if (ret == 0)
			data->v1.data = byte_data;
		break;
	case 'h':
		ret = pci_read_config_word(pdev, address, &short_data);
		if (ret == 0)
			data->v1.data = short_data;
		break;
	case 'w':
		ret = pci_read_config_dword(pdev, address, &word_data);
		if (ret == 0)
			data->v1.data = word_data;
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

int xvsec_mcap_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	address = mcap_ctx->vsec_offset + data->v1.offset;

	switch (data->v1.access) {
	case 'b':
		byte_data = (uint8_t)data->v1.data;
		ret = pci_write_config_byte(pdev, address, byte_data);
		break;
	case 'h':
		short_data = (uint16_t)data->v1.data;
		ret = pci_write_config_word(pdev, address, short_data);
		break;
	case 'w':
		word_data = (uint32_t)data->v1.data;
		ret = pci_write_config_dword(pdev, address, word_data);
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

int xvsec_fpga_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg)
{
	int ret = 0;
	int ctrl_offset, status_offset;
	int rd_offset;
	int i, n;
	uint32_t rd_data, ctrl_data, status_data, restore;
	uint16_t count;
	struct pci_dev *pdev = mcap_ctx->pdev;

	i = 0x0;
	n = ARRAY_SIZE(fpga_valid_addr);

	while ((i < n) && (cfg_reg->v1.offset != fpga_valid_addr[i++]))
		;

	if ((i == n) && (cfg_reg->v1.offset != fpga_valid_addr[n - 1])) {
		pr_err("Invalid FPGA Register Access : ");
		pr_err("Addr : 0x%X\n", cfg_reg->v1.offset);
		return -(EACCES);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	rd_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_READ_DATA_REG0;
	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_CONTROL_REGISTER;
	status_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_STATUS_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(mcap_ctx, FPGA_RD_CMD);

	xvsec_fpga_cfg_write_cmd(mcap_ctx, FPGA_RD_CMD, cfg_reg->v1.offset, 1);

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

	cfg_reg->v1.data = rd_data;

	ctrl_data = ctrl_data & (~XVSEC_MCAP_CTRL_RD_ENABLE);
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);


CLEANUP:
	xvsec_fpga_cfg_teardown(mcap_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

int xvsec_fpga_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg)
{
	int ret = 0;
	int ctrl_offset;
	int i, n;
	uint32_t ctrl_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	i = 0x0;
	n = ARRAY_SIZE(fpga_valid_addr);

	while ((i < n) && (cfg_reg->v1.offset != fpga_valid_addr[i++]))
		;

	if ((i == n) && (cfg_reg->v1.offset != fpga_valid_addr[n - 1])) {
		pr_err("Invalid FPGA Register Access : ");
		pr_err("Addr : 0x%X\n", cfg_reg->v1.offset);
		return -(EACCES);
	}

	/* Acquire the Access */
	ret = xvsec_mcap_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAP_CONTROL_REGISTER;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	ctrl_data = ctrl_data |
		XVSEC_MCAP_CTRL_WR_ENABLE | XVSEC_MCAP_CTRL_ENABLE |
		XVSEC_MCAP_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	xvsec_fpga_cfg_setup(mcap_ctx, FPGA_WR_CMD);

	xvsec_fpga_cfg_write_cmd(mcap_ctx, FPGA_WR_CMD, cfg_reg->v1.offset, 1);

	xvsec_fpga_cfg_write_data(mcap_ctx, cfg_reg->v1.data);

	xvsec_fpga_cfg_teardown(mcap_ctx);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

/*
 * unsupported v1 functions for US/US+
 */
int xvsec_mcapv1_axi_rd_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg)
{
	pr_err("AXI Read operation not supported for US/US+ devices\n");
	return -(EPERM);
}

int xvsec_mcapv1_axi_wr_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg)
{
	pr_err("AXI Write operation not supported for US/US+ devices\n");
	return -(EPERM);
}

int xvsec_mcapv1_file_download(struct vsec_context *mcap_ctx,
	union file_download_upload *file)
{
	pr_err("AXI File Download not supported for US/US+ devices\n");
	return -(EPERM);
}

int xvsec_mcapv1_file_upload(struct vsec_context *mcap_ctx,
	union file_download_upload *file)
{
	pr_err("AXI File upload not supported for US/US+ devices\n");
	return -(EPERM);
}
int xvsec_mcapv1_set_axi_cache_attr(
	struct vsec_context *mcap_ctx, union axi_cache_attr *attr)
{
	pr_err("AXI attributes settings not supported for US/US+ devices\n");
	return -(EPERM);
}

MODULE_LICENSE("Dual BSD/GPL");
