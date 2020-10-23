/*
 *This file is part of the XVSEC driver for Linux
 *
 *Copyright (c) 2020  Xilinx, Inc.
 *All rights reserved.
 *
 *This source code is free software; you can redistribute it and/or modify it
 *under the terms and conditions of the GNU General Public License,
 *version 2, as published by the Free Software Foundation.
 *
 *This program is distributed in the hope that it will be useful, but WITHOUT
 *ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *more details.
 *
 *The full GNU General Public License is included in this distribution in
 *the file called "COPYING".
 *
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
#include "xvsec_mcap.h"
#include "xvsec_mcap_versal.h"
#include "xvsec_util.h"

static int xvsec_mcapv2_wr_enable(struct vsec_context *mcap_ctx);
static int xvsec_mcapv2_rd_enable(struct vsec_context *mcap_ctx);
static int xvsec_mcapv2_wr_disable(struct vsec_context *mcap_ctx);
static int xvsec_mcapv2_rd_disable(struct vsec_context *mcap_ctx);
static int xvsec_mcapv2_set_mode(
	struct vsec_context *mcap_ctx, enum axi_access_mode mode);
static int xvsec_mcapv2_set_address(
	struct vsec_context *mcap_ctx, uint32_t address);
static int xvsec_mcapv2_set_axi_cache_prot(
	struct vsec_context *mcap_ctx, uint8_t axi_cache, uint8_t axi_prot);
static int xvsec_mcapv2_write_data_reg(
	struct vsec_context *mcap_ctx, uint32_t data);
static int xvsec_mcapv2_read_data_reg(
	struct vsec_context *mcap_ctx, uint32_t *data);
static int xvsec_mcapv2_read_status_reg(
	struct vsec_context *mcap_ctx, uint32_t *sts);
static int xvsec_mcapv2_wait_for_write_FIFO_empty(
	struct vsec_context *mcap_ctx);
static int xvsec_mcapv2_check_mcap_rw_status(
	struct vsec_context *mcap_ctx, uint32_t sts,
	enum file_operation_status *op_status);
static int xvsec_mcapv2_read_fifo_capacity(
	struct vsec_context *mcap_ctx, uint8_t *fifo_capacity);
static int xvsec_mcapv2_wait_for_rw_complete(
	struct vsec_context *mcap_ctx);

int xvsec_mcapv2_module_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint16_t mcap_offset;
	uint16_t ctrl_offset;
	uint32_t ctrl_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	mcap_offset = mcap_ctx->vsec_offset;
	ctrl_offset = mcap_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	restore = ctrl_data;

	/* Asserting the Module Reset */
	ctrl_data = ctrl_data | XVSEC_MCAPV2_CTRL_RESET;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data & XVSEC_MCAPV2_CTRL_RESET) == 0x0)
		ret = -(EBUSY);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

int xvsec_mcapv2_get_regs(
	struct vsec_context *mcap_ctx, union mcap_regs *regs)
{
	uint16_t mcap_offset;
	uint16_t index;
	uint32_t *ptr;
	struct pci_dev *pdev = mcap_ctx->pdev;

	regs->v2.valid = 0;

	mcap_offset = mcap_ctx->vsec_offset;
	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	ptr = &regs->v2.ext_cap_header;
	for (index = 0; index <= (XVSEC_MCAPV2_READ_DATA_REG / 4); index++) {
		pci_read_config_dword(pdev, mcap_offset, &ptr[index]);
		mcap_offset = mcap_offset + 4;
	}

	regs->v2.valid = 1;

	return 0;
}

int xvsec_mcapv2_rd_cfg_addr(
	struct vsec_context *mcap_ctx, union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	address = mcap_ctx->vsec_offset + data->v2.offset;
	switch (data->v2.access) {
	case 'b':
		ret = pci_read_config_byte(pdev, address, &byte_data);
		if (ret == 0)
			data->v2.data = byte_data;
		break;
	case 'h':
		ret = pci_read_config_word(pdev, address, &short_data);
		if (ret == 0)
			data->v2.data = short_data;
		break;
	case 'w':
		ret = pci_read_config_dword(pdev, address, &word_data);
		if (ret == 0)
			data->v2.data = word_data;
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

int xvsec_mcapv2_wr_cfg_addr(
	struct vsec_context *mcap_ctx, union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	address = mcap_ctx->vsec_offset + data->v2.offset;

	switch (data->v2.access) {
	case 'b':
		byte_data = (uint8_t)data->v2.data;
		ret = pci_write_config_byte(pdev, address, byte_data);
		break;
	case 'h':
		short_data = (uint16_t)data->v2.data;
		ret = pci_write_config_word(pdev, address, short_data);
		break;
	case 'w':
		word_data = (uint32_t)data->v2.data;
		ret = pci_write_config_dword(pdev, address, word_data);
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

int xvsec_mcapv2_axi_rd_addr(
	struct vsec_context *mcap_ctx, union axi_reg_data *cfg_reg)
{
	int ret = 0, wcnt = 0;
	uint32_t data = 0, sts = 0;
	uint8_t rw_status;
	uint32_t address = cfg_reg->v2.address;

	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	if (XVSEC_MCAPV2_IS_RW_COMPLETE(sts) == true) {
		/* TO Clear the MCAP Read Complete bit by asserting reset */
		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
	}

	xvsec_mcapv2_set_mode(mcap_ctx, MCAP_AXI_MODE_32B);

	xvsec_mcapv2_set_address(mcap_ctx, address);

	xvsec_mcapv2_rd_enable(mcap_ctx);

	/*Poll the MCAP Status Register MCAP Read Complete bit */
	wcnt = 0;
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	while (XVSEC_MCAPV2_IS_RW_COMPLETE(sts) != true) {
		udelay(1);
		if (wcnt++ >= MAX_OP_POLL_CNT) {
			pr_err("%s: timeout on rw_complete.\n", __func__);
			ret = -(ETIME);
			goto CLEANUP;
		}
		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	}

	/*If the MCAP rw_status != OK, report the error to the user.*/
	XVSEC_MCAPV2_GET_RW_STATUS(sts, rw_status);
	if (rw_status != XVSEC_MCAPV2_RW_OK) {
		pr_err("%s: AXI write transaction failed.\n", __func__);
		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
		ret = -(EIO);
		goto CLEANUP;
	}

	/*read 32bit data*/
	xvsec_mcapv2_read_data_reg(mcap_ctx, &data);

	/*return data*/
	cfg_reg->v2.data[0] = data;

CLEANUP:
	/*disable the read*/
	xvsec_mcapv2_rd_disable(mcap_ctx);
	return ret;
}

int xvsec_mcapv2_axi_wr_addr(
	struct vsec_context *mcap_ctx, union axi_reg_data *cfg_reg)
{
	int ret = 0, reg_cnt = 0, wcnt = 0, i = 0;
	uint8_t rw_status;
	uint32_t address = cfg_reg->v2.address;
	uint32_t *data = (uint32_t *)&cfg_reg->v2.data[0];
	enum axi_access_mode mode = cfg_reg->v2.mode;
	uint32_t sts = 0;

	if (mode == MCAP_AXI_MODE_128B)
		reg_cnt = 4; /* four 32bit writes */
	else
		reg_cnt = 1; /* one 32bit writes */

	xvsec_mcapv2_wr_enable(mcap_ctx);
	xvsec_mcapv2_set_mode(mcap_ctx, mode);

	xvsec_mcapv2_set_address(mcap_ctx, address);

	for (i = 0; i < reg_cnt; i++) {
		/* Wait if FIFO is full for a while */
		wcnt = 0;
		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
		while (XVSEC_MCAPV2_IS_FIFO_FULL(sts) == true) {
			udelay(1);
			if (wcnt++ >= MAX_OP_POLL_CNT) {
				pr_err("%s: Timeout, FIFO FULL\n", __func__);
				ret = -(ETIME);
				goto CLEANUP;
			}
			xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
		}

		xvsec_mcapv2_write_data_reg(mcap_ctx, data[i]);
	}

	/*If FIFO != empty and time-out reached, report error to user.*/
	wcnt = 0;
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	while (XVSEC_MCAPV2_IS_FIFO_EMPTY(sts) != true) {
		udelay(1);
		if (wcnt++ >= MAX_OP_POLL_CNT) {
			pr_err("%s: Timeout, FIFO not Empty\n", __func__);
			ret = -(ETIME);
			goto CLEANUP;
		}
		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	}

	XVSEC_MCAPV2_GET_RW_STATUS(sts, rw_status);
	/*If rw_status != OK, report error to user.*/
	if (rw_status != XVSEC_MCAPV2_RW_OK) {
		pr_err("%s: AXI write transaction failed.\n", __func__);
		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
		ret = -(EIO);
		goto CLEANUP;
	}

	/*If FIFO Overflow bit != set, report error to user */
	if (XVSEC_MCAPV2_IS_FIFO_OVERFLOW(sts) == true) {
		pr_err("%s: Write FIFO overflow error occured.\n", __func__);
		ret = -(EIO);
		goto CLEANUP;
	}

CLEANUP:
	return ret;
}

int xvsec_mcapv2_file_download(
	struct vsec_context *mcap_ctx, union file_download_upload *file_info)
{
	int ret = 0;
	char *fname;
	loff_t file_size = 0;
	struct file *filep;
	uint32_t dev_address;
	loff_t offset;
	loff_t frag_size;
	loff_t rem_len = 0;
	int index = 0;
	int rd_len = 0;
	uint32_t data_buf[MAX_FRAG_SZ / 4];
	enum axi_access_mode mode;
	uint32_t sts = 0;
	char pdifile[MAX_FILE_LEN];
	int len = 0;
	enum data_transfer_mode tr_mode;
	uint8_t fifo_capacity = 0;
	uint8_t min_len = 0;

	pr_debug("In %s\n", __func__);

	file_info->v2.op_status = FILE_OP_FAILED;

	if ((file_info->v2.mode > MCAP_AXI_MODE_128B) ||
		(file_info->v2.addr_type > INCREMENT_ADDRESS) ||
		(file_info->v2.file_name == NULL) ||
		(file_info->v2.tr_mode > DATA_TRANSFER_MODE_SLOW)) {
		pr_err("%s: Invalid Params : mode : %d, type : %d, tr_mode : %d, name : %p",
			__func__, file_info->v2.mode, file_info->v2.addr_type,
			file_info->v2.tr_mode, file_info->v2.file_name);
		return -(EINVAL);
	}


	if (file_info->v2.file_name != NULL) {
		len = strnlen_user(
			(char __user *)file_info->v2.file_name,
			MAX_FILE_LEN);
		if (len > MAX_FILE_LEN) {
			pr_err("File Name too long\n");
			file_info->v2.op_status = FILE_PATH_TOO_LONG;
			return -(ENAMETOOLONG);
		}

		ret = strncpy_from_user(pdifile,
				(char __user *)file_info->v2.file_name, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			return -(EINVAL);
		}
		pr_info("pdi File Name : %s, tr_mode: %d\n", pdifile,
			file_info->v2.tr_mode);
	}

	fname = pdifile;
	dev_address = file_info->v2.address;
	mode = file_info->v2.mode;
	tr_mode = file_info->v2.tr_mode;

	/** At present only PDI file format is implemented */
	if (xvsec_util_find_file_type(fname, MCAPV2_PDI_FILE) < 0) {
		pr_err("Only PDI files supported for a while\n");
		return -(EINVAL);
	}

	filep = xvsec_util_fopen(fname, O_RDONLY, 0);
	if (filep == NULL)
		return -(ENOENT);

	ret = xvsec_util_get_file_size(fname, &file_size);
	if (ret < 0)
		goto CLEANUP_EXIT;

	if (file_size <= 0) {
		file_info->v2.op_status = FILE_OP_ZERO_FSIZE;
		ret = -(EINVAL);
		goto CLEANUP_EXIT;
	}

	/** Check the file size is proper (multiple of 128bit/16Bytes) */
	if ((mode == MCAP_AXI_MODE_128B) && ((file_size % MIN_LEN_128B) != 0)) {
		file_info->v2.op_status = FILE_OP_INVALID_FSIZE;
		pr_err("%s: file size is not multiple of 128b/16B\n", __func__);
		ret = -(EINVAL);
		goto CLEANUP_EXIT;
	}

	ret = xvsec_mcapv2_wait_for_write_FIFO_empty(mcap_ctx);
	if (ret != 0)
		goto CLEANUP_EXIT;

	/** FIXME: pprereps, does mcap needs a reset here? */

	/** Enable write mode */
	xvsec_mcapv2_wr_enable(mcap_ctx);
	xvsec_mcapv2_set_mode(mcap_ctx, mode);
	if (file_info->v2.addr_type == FIXED_ADDRESS)
		xvsec_mcapv2_set_address(mcap_ctx, dev_address);

	if (mode == MCAP_AXI_MODE_32B)
		min_len = MIN_LEN_32B;
	else
		min_len = MIN_LEN_128B;

	offset = 0;
	rem_len = file_size;
	file_info->v2.err_index = 0;
	memset(data_buf, 0, sizeof(data_buf));

	while (rem_len != 0) {
		frag_size = (rem_len >= MAX_FRAG_SZ) ? MAX_FRAG_SZ : rem_len;

		rd_len = xvsec_util_fread(filep,
				offset, (uint8_t *)data_buf, frag_size);

		if (mode == MCAP_AXI_MODE_128B) {
			/** Truncates the length for 128b mode */
			truncate_len_128b_mode(rd_len);
		} else {
			/** Truncates the length for 32b mode */
			truncate_len_32b_mode(rd_len);
		}

		index = 0;
		while (index < rd_len) {
			/*read the status*/
			xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
			/*
			 * Fast download
			 * - no condition check except FIFO occupancy
			 * - FIFO occupancy, Need to check inside loop
			 *   if the FIFO occupancy is 16 will wait for a while
			 * - fifo_capacity set to 1 for slow download and
			 *   for fast download mode based on fifo occupancy
			 */
			fifo_capacity = 1;
			if (tr_mode == DATA_TRANSFER_MODE_FAST) {
				fifo_capacity = 0;
				ret = xvsec_mcapv2_read_fifo_capacity(
					mcap_ctx, &fifo_capacity);
				if (ret != 0)
					goto CLEANUP;
			}
			/*slow download - Fifo should be empty*/
			else if (tr_mode == DATA_TRANSFER_MODE_SLOW) {
				/** Wait until the Write FIFO is empty */
				/* for 128B, check after evry 4 dwords*/
				if ((index % min_len) == 0) {
					ret =
					xvsec_mcapv2_wait_for_write_FIFO_empty(
						mcap_ctx);
					if (ret != 0)
						goto CLEANUP;
				}
			}

			while (fifo_capacity--) {
				/** Continue when FIFO has some room */
				if (
				(file_info->v2.addr_type == INCREMENT_ADDRESS)
				&& ((index % min_len) == 0)) {
					xvsec_mcapv2_set_address(mcap_ctx,
							dev_address);
					dev_address = dev_address + min_len;
				}

				xvsec_mcapv2_write_data_reg(mcap_ctx,
						data_buf[index / 4]);
				index = index + 4;

				/*
				 * - Break This loop if remaining data is
				 *   less then the fifo capacity and it gets
				 *   transferred to the HW
				 */
				if (index >= rd_len) {
					pr_debug("%s: Fifo cap: %d, index: %d, rd_len: %d\n",
						__func__, fifo_capacity,
						index, rd_len);
					break;
				}
			}

			/*slow download:
			 * sts_ok and rw_complete for every dword
			 */
			if (tr_mode == DATA_TRANSFER_MODE_SLOW) {
				if ((index % min_len) == 0) {
					ret = xvsec_mcapv2_wait_for_rw_complete(
						mcap_ctx);
					if (ret != 0)
						goto CLEANUP;

					xvsec_mcapv2_read_status_reg(
						mcap_ctx, &sts);
					ret = xvsec_mcapv2_check_mcap_rw_status(
						mcap_ctx, sts,
						&file_info->v2.op_status);
					if (ret != 0)
						goto CLEANUP;
				}
			}
		}

		offset = offset + rd_len;
		rem_len = rem_len - rd_len;
	}

	/** common for all modes:
	 ** to check all the conditions and report to user if error **/

	/** Wait for write transactions in FIFO are complete */
	ret = xvsec_mcapv2_wait_for_write_FIFO_empty(mcap_ctx);
	if (ret != 0)
		goto CLEANUP;

	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	ret = xvsec_mcapv2_check_mcap_rw_status(mcap_ctx, sts,
		&file_info->v2.op_status);
	if (ret != 0)
		goto CLEANUP;

	/* If the MCAP FIFO Overflow bit is set report the error to the user. */
	if (XVSEC_MCAPV2_IS_FIFO_OVERFLOW(sts) == true) {
		file_info->v2.op_status = FILE_OP_HW_BUSY;
		pr_err("%s: Write FIFO overflow error occured.\n", __func__);
		ret = -(EIO);
		goto CLEANUP;
	}

	/** Finally, Update op_status to SUCCESS */
	file_info->v2.op_status = FILE_OP_SUCCESS;

CLEANUP:
	if (ret != 0) {
		file_info->v2.err_index = (file_size - rem_len) + index;
		pr_err("%s: file_size: %lld, rem_len: %lld, rd_len: %d, index: %d, sts: 0x%X\n",
			__func__, file_size, rem_len, rd_len, index, sts);

		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
	}

CLEANUP_EXIT:
	xvsec_util_fclose(filep);

	return ret;
}

int xvsec_mcapv2_file_upload(
	struct vsec_context *mcap_ctx, union file_download_upload *file_info)
{
	int ret = 0;
	char *fname;
	struct file *filep;
	uint32_t dev_address;
	loff_t offset;
	loff_t frag_size = 0;
	loff_t rem_len = 0;
	int index = 0;
	int written_len;
	uint32_t data_buf[MAX_FRAG_SZ / 4];
	uint32_t wait_cnt;
	bool is_rw_done;
	uint8_t rw_status;
	uint8_t *ptr;
	loff_t file_size = 0;
	uint32_t sts = 0;
	char pdifile[MAX_FILE_LEN];
	int len = 0;

	pr_debug("In %s\n", __func__);

	file_info->v2.op_status = FILE_OP_FAILED;

	if ((file_info->v2.addr_type > INCREMENT_ADDRESS) ||
		(file_info->v2.file_name == NULL) ||
		(file_info->v2.length == 0)) {
		pr_err("%s: Invalid Params : type : %d, name : %p", __func__,
			file_info->v2.addr_type, file_info->v2.file_name);
		return -(EINVAL);
	}

	if (file_info->v2.file_name != NULL) {
		len = strnlen_user(
			(char __user *)file_info->v2.file_name,
			MAX_FILE_LEN);
		if (len > MAX_FILE_LEN) {
			pr_err("File Name too long\n");
			file_info->v2.op_status = FILE_PATH_TOO_LONG;
			return -(ENAMETOOLONG);
		}

		ret = strncpy_from_user(pdifile,
				(char __user *)file_info->v2.file_name, len);
		if (ret < 0) {
			pr_err("File Name Copy Failed\n");
			return -(EINVAL);
		}
		pr_info("pdi File Name : %s\n", pdifile);
	}

	fname = pdifile;
	dev_address = file_info->v2.address;

	/** Check for any previous read/operation completed */
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	is_rw_done = XVSEC_MCAPV2_IS_RW_COMPLETE(sts);
	if (is_rw_done == true) {
		/* Reset MCAP module to clear rw_complete bit */
		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
	}

	/** FIXME : pprerepa, check for file permissions during testing */
	filep = xvsec_util_fopen(fname, O_WRONLY|O_CREAT, 0);
	if (filep == NULL)
		return -(ENOENT);

	xvsec_mcapv2_set_mode(mcap_ctx, MCAP_AXI_MODE_32B);
	if (file_info->v2.addr_type == FIXED_ADDRESS)
		xvsec_mcapv2_set_address(mcap_ctx, dev_address);

	offset = 0;
	rem_len = file_info->v2.length;
	file_info->v2.err_index = 0;

	while (rem_len != 0) {
		index = 0;
		frag_size = (rem_len >= MAX_FRAG_SZ) ? MAX_FRAG_SZ : rem_len;
		while (index < frag_size) {
			if (file_info->v2.addr_type == INCREMENT_ADDRESS) {
				xvsec_mcapv2_set_address(mcap_ctx, dev_address);
				dev_address = dev_address + 4;
			}
			/** Enable read mode */
			xvsec_mcapv2_rd_enable(mcap_ctx);

			/** Wait for write transactions in FIFO are complete */
			wait_cnt = 0;
			/** Wait until the Write FIFO is empty */
			xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
			while (((is_rw_done =
				XVSEC_MCAPV2_IS_RW_COMPLETE(sts)) != true)
					&& (wait_cnt++ < MAX_OP_POLL_CNT)) {
				/* Sleep for a while */
				udelay(1);
				xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
			}
			if (is_rw_done != true) {
				pr_err("%s: Err file upload: RW Cmplt : %d\n",
					__func__, is_rw_done);
				file_info->v2.op_status = FILE_OP_HW_BUSY;
				ret = -(EIO);
				goto CLEANUP;
			}

			XVSEC_MCAPV2_GET_RW_STATUS(sts, rw_status);
			if (rw_status != XVSEC_MCAPV2_RW_OK) {
				file_info->v2.op_status =
					(enum file_operation_status)rw_status;
				pr_err("%s: Err file upload: RW Status : %d\n",
					__func__, rw_status);
				ret = -(EIO);
				goto CLEANUP;
			}
			xvsec_mcapv2_read_data_reg(mcap_ctx,
					&data_buf[index / 4]);
			index = index + 4;

			/** Check for any previous read/operation completed */
			xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
			xvsec_mcapv2_rd_disable(mcap_ctx);

			is_rw_done = XVSEC_MCAPV2_IS_RW_COMPLETE(sts);
			if (is_rw_done == true) {
				/* Reset MCAP module to clear rw_complete bit */
				xvsec_mcapv2_module_reset(mcap_ctx);
				pr_debug("%s: MCAP Reset is issued.\n",
					__func__);
			}

		}

		written_len = 0;
		ptr = (uint8_t *)data_buf;
		while (written_len < frag_size) {
			written_len += xvsec_util_fwrite(filep, offset,
				&ptr[written_len], (frag_size - written_len));
		}

		offset = offset + frag_size;
		rem_len = rem_len - frag_size;
	}

	/* Make sure the file contents written to disk */
	xvsec_util_fsync(filep);

	/** Compare the requested size with file size */
	ret = xvsec_util_get_file_size(fname, &file_size);
	if (ret < 0)
		goto CLEANUP;

	if (file_size != file_info->v2.length) {
		pr_err("%s: Could not read complete requested length\n",
			__func__);
		/** Replace the requested length with read length */
		file_info->v2.length = file_size;
	}

	file_info->v2.op_status = FILE_OP_SUCCESS;
CLEANUP:

	if (ret != 0) {
		file_info->v2.err_index = (file_size - rem_len) + index;
		pr_err("%s: file_size: %lld, rem_len: %lld, frag_size: %lld, index: %d, sts: 0x%X\n",
			__func__, file_size, rem_len, frag_size, index, sts);

		xvsec_mcapv2_module_reset(mcap_ctx);
		pr_debug("%s: MCAP Reset is issued.\n", __func__);
	}
	/*disable the read*/
	xvsec_mcapv2_rd_disable(mcap_ctx);
	xvsec_util_fclose(filep);

	return ret;
}

int xvsec_mcapv2_set_axi_cache_attr(
	struct vsec_context *mcap_ctx, union axi_cache_attr *attr)
{
	int ret = 0;
	uint8_t axi_cache;
	uint8_t axi_prot;

	axi_cache = attr->v2.axi_cache;
	axi_prot = attr->v2.axi_prot;

	pr_debug("%s: axi_cache:%d, axi_prot:%d\n",
		__func__, (int)axi_cache, (int)axi_prot);
	ret = xvsec_mcapv2_set_axi_cache_prot(mcap_ctx, axi_cache, axi_prot);

	return ret;
}

static int xvsec_mcapv2_wait_for_write_FIFO_empty(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	bool is_fifo_empty;
	uint32_t wait_cnt;
	uint32_t sts = 0;

	wait_cnt = 0;
	/** Wait until the Write FIFO is empty */
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	while (((is_fifo_empty = XVSEC_MCAPV2_IS_FIFO_EMPTY(sts)) != true)
			&& (wait_cnt++ < MAX_OP_POLL_CNT)) {
		/* Sleep for a while */
		udelay(1);
		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	}
	if (is_fifo_empty != true) {
		pr_err("%s: Timeout on FIFO Empty\n", __func__);
		ret = -(ETIME);
	}

	return ret;
}

static int xvsec_mcapv2_wait_for_rw_complete(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	bool is_rw_done;
	uint32_t wait_cnt;
	uint32_t sts = 0;

	/** Wait until the rw_complete bit sets */
	wait_cnt = 0;
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	while (((is_rw_done = XVSEC_MCAPV2_IS_RW_COMPLETE(sts)) != true)
			&& (wait_cnt++ < MAX_OP_POLL_CNT)) {
		/* Sleep for a while */
		udelay(1);
		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	}
	if (is_rw_done != true) {
		pr_err("%s: Timeout on rw_complete\n", __func__);
		ret = -(ETIME);
	}

	/*reset the module to clear the rw_complete bit*/
	xvsec_mcapv2_module_reset(mcap_ctx);
	pr_debug("%s: MCAP Reset is issued.\n", __func__);

	return ret;
}

static int xvsec_mcapv2_read_fifo_capacity(struct vsec_context *mcap_ctx,
	uint8_t *fifo_capacity)
{
	int ret = 0;
	uint32_t wait_cnt;
	uint8_t fifo_occupancy;
	uint32_t sts = 0;

	wait_cnt = 0;
	/** Wait if the Write FIFO occupancy reached to MAX */
	xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
	XVSEC_MCAPV2_GET_FIFO_OCCUPANCY(sts, fifo_occupancy);
	while ((fifo_occupancy >= MAX_FIFO_OCCUPANCY)) {

		if (wait_cnt++ >= MAX_OP_POLL_CNT) {
			pr_err("%s: Timeout on FIFO occupancy.\n",
				__func__);
			ret = -(ETIME);
			break;
		}
		/* Sleep for a while */
		udelay(1);

		xvsec_mcapv2_read_status_reg(mcap_ctx, &sts);
		XVSEC_MCAPV2_GET_FIFO_OCCUPANCY(sts, fifo_occupancy);
	}

	*fifo_capacity = (MAX_FIFO_OCCUPANCY - fifo_occupancy);

	return ret;
}



static int xvsec_mcapv2_check_mcap_rw_status(struct vsec_context *mcap_ctx,
	uint32_t sts, enum file_operation_status *op_status)
{
	int ret = 0;
	uint8_t rw_status;

	XVSEC_MCAPV2_GET_RW_STATUS(sts, rw_status);
	if (rw_status != XVSEC_MCAPV2_RW_OK) {
		*op_status =
			(enum file_operation_status)rw_status;
		pr_err("%s: Err :file download : RW Status : %d\n",
				__func__, rw_status);
		ret = -(EIO);
	}

	return ret;
}

static int xvsec_mcapv2_wr_enable(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data = 0;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);
	/* MCAP Read Enable = 0 */
	ctrl_data = ctrl_data & ~(XVSEC_MCAPV2_CTRL_READ_ENABLE);
	/* MCAP Write Enable = 1 */
	ctrl_data = ctrl_data | XVSEC_MCAPV2_CTRL_WRITE_ENABLE;

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data:0x%X\n",
		__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}

__attribute__((unused))
static int xvsec_mcapv2_wr_disable(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data = 0;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);
	/* MCAP Write Enable = 0 */
	ctrl_data = ctrl_data & ~XVSEC_MCAPV2_CTRL_WRITE_ENABLE;

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data:0x%X\n",
			__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}

static int xvsec_mcapv2_rd_enable(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data = 0;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);
	/* MCAP Read Enable = 1 */
	ctrl_data = ctrl_data | XVSEC_MCAPV2_CTRL_READ_ENABLE;
	/* MCAP Write Enable = 0 */
	ctrl_data = ctrl_data & ~(XVSEC_MCAPV2_CTRL_WRITE_ENABLE);

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data:0x%X\n",
		__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}

static int xvsec_mcapv2_rd_disable(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data = 0;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);
	/* MCAP Read Enable = 0 */
	ctrl_data = ctrl_data & ~XVSEC_MCAPV2_CTRL_READ_ENABLE;

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data:0x%X\n",
			__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}


static int xvsec_mcapv2_set_mode(
	struct vsec_context *mcap_ctx, enum axi_access_mode mode)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data = 0;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);

	if (mode == MCAP_AXI_MODE_32B) {
		/* 0 for 32-bit transactions  */
		ctrl_data = ctrl_data & ~XVSEC_MCAPV2_CTRL_128B_MODE;
	} else if (mode == MCAP_AXI_MODE_128B) {
		/* 1 for 128-bit transactions */
		ctrl_data = ctrl_data | XVSEC_MCAPV2_CTRL_128B_MODE;
	}

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data:0x%X\n",
			__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}

static int xvsec_mcapv2_set_address(
	struct vsec_context *mcap_ctx, uint32_t address)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t rw_addr_reg;

	rw_addr_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_RW_ADDRESS;

	pr_debug("In %s: addr_reg:0x%X, addr:0x%X\n",
		__func__, rw_addr_reg, address);

	ret = pci_write_config_dword(pdev, rw_addr_reg, address);

	return ret;
}

static int xvsec_mcapv2_set_axi_cache_prot(struct vsec_context *mcap_ctx,
	uint8_t axi_cache, uint8_t axi_prot)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t ctrl_reg;
	uint32_t ctrl_data;

	ctrl_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_reg, &ctrl_data);

	ctrl_data &= ~XVSEC_MCAPV2_CTRL_AXI_CACHE;
	ctrl_data &= ~XVSEC_MCAPV2_CTRL_AXI_PROTECT;

	/*configure AXI Cache*/

	ctrl_data =
		ctrl_data | ((axi_cache & XVSEC_MCAPV2_CTRL_AXI_CACHE_MASK) <<
		XVSEC_MCAPV2_CTRL_AXI_CACHE_SHIFT);

	/*configure AXI Protect*/
	ctrl_data =
		ctrl_data | ((axi_prot & XVSEC_MCAPV2_CTRL_AXI_PROTECT_MASK) <<
		XVSEC_MCAPV2_CTRL_AXI_PROTECT_SHIFT);

	pr_debug("In %s: ctrl_reg:0x%X, ctrl_data_cp:0x%X\n",
		__func__, ctrl_reg, ctrl_data);
	ret = pci_write_config_dword(pdev, ctrl_reg, ctrl_data);

	return ret;
}

static int xvsec_mcapv2_write_data_reg(
	struct vsec_context *mcap_ctx, uint32_t data)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t wr_data_reg;

	wr_data_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_WRITE_DATA_REG;

	pr_debug("In %s, wr_data_reg: 0x%X, data:0x%X\n",
		__func__, wr_data_reg, data);

	ret = pci_write_config_dword(pdev, wr_data_reg, data);

	return ret;
}

static int xvsec_mcapv2_read_data_reg(
	struct vsec_context *mcap_ctx, uint32_t *data)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t rd_data_reg;

	rd_data_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_READ_DATA_REG;
	ret = pci_read_config_dword(pdev, rd_data_reg, data);

	pr_debug("In %s, rd_data_reg: 0x%X, data:0x%X\n",
		__func__, rd_data_reg, *data);

	return ret;
}

static int xvsec_mcapv2_read_status_reg(
	struct vsec_context *mcap_ctx, uint32_t *sts)
{
	int ret = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;
	uint32_t sts_reg;

	sts_reg = mcap_ctx->vsec_offset + XVSEC_MCAPV2_STATUS_REG;
	ret = pci_read_config_dword(pdev, sts_reg, sts);

	pr_debug("In %s, sts_reg: 0x%X, data:0x%X\n", __func__, sts_reg, *sts);

	return ret;
}

/*unsupported for versal devices */
int xvsec_mcapv2_reset(struct vsec_context *mcap_ctx)
{
	pr_err("reset is not supported for versal\n");
	return -(EPERM);
}

int xvsec_mcapv2_full_reset(struct vsec_context *mcap_ctx)
{
	pr_err("Full reset is not supported for versal\n");
	return -(EPERM);
}

int xvsec_mcapv2_get_data_regs(struct vsec_context *mcap_ctx, uint32_t regs[4])
{
	pr_err("get_data_regs is not supported for versal\n");
	return -(EPERM);
}

int xvsec_mcapv2_get_fpga_regs(
	struct vsec_context *mcap_ctx, union fpga_cfg_regs *regs)
{
	pr_err("fpga read is not supported for versal\n");
	return -(EPERM);
}

int xvsec_mcapv2_program_bitstream(
	struct vsec_context *mcap_ctx, union bitstream_file *bit_files)
{
	pr_err("unsupported ioctl call for versal\n");
	return -(EPERM);
}

int xvsec_fpgav2_rd_cfg_addr(
	struct vsec_context *mcap_ctx, union fpga_cfg_reg *cfg_reg)
{
	pr_err("fpga read is not supported for versal\n");
	return -(EPERM);
}

int xvsec_fpgav2_wr_cfg_addr(
	struct vsec_context *mcap_ctx, union fpga_cfg_reg *cfg_reg)
{
	pr_err("fpga write is not supported for versal\n");
	return -(EPERM);
}

MODULE_LICENSE("Dual BSD/GPL");
