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

#ifndef __XVSEC_MCAP_US_H__
#define __XVSEC_MCAP_US_H__

/**
 * @file xvsec_mcap_us.h
 *
 * Xilinx XVSEC MCAP US/US+ Driver Library  Definitions
 *
 * Header file *xvsec_mcap_us.h* defines data structures & register
 * definitions needed to implement the MCAP version 1 driver
 *
 * These data structures and register definitions can be used by
 * XVSEC MCAP common driver
 */


#define MAX_FLEN						(300)
#define RBT_WORD_LEN					(32)

#define XVSEC_MCAP_LOOP_COUNT			(1000000)
#define EMCAP_EOS_LOOP_COUNT			(100)
#define EMCAP_NOOP_VAL					0x2000000
#define EMCAP_EOS_RETRY_COUNT			10
#define DMA_HWICAP_BITFILE_BUFFER_SIZE	1024 /* from xbar_sys_parameters.h */

#define MCAP_SYNC_DWORD	0xFFFFFFFF
#define MCAP_SYNC_BYTE0 ((MCAP_SYNC_DWORD & 0xFF000000) >> 24)
#define MCAP_SYNC_BYTE1 ((MCAP_SYNC_DWORD & 0x00FF0000) >> 16)
#define MCAP_SYNC_BYTE2 ((MCAP_SYNC_DWORD & 0x0000FF00) >> 8)
#define MCAP_SYNC_BYTE3 ((MCAP_SYNC_DWORD & 0x000000FF) >> 0)

#define MCAP_RBT_FILE	".rbt"
#define MCAP_BIT_FILE	".bit"
#define MCAP_BIN_FILE	".bin"

/* MCAP Register Offsets */
#define XVSEC_MCAP_EXTENDED_HEADER	(0x0000)
#define XVSEC_MCAP_VENDOR_HEADER	(0x0004)
#define XVSEC_MCAP_FPGA_JTAG_ID		(0x0008)
#define XVSEC_MCAP_FPGA_BIT_VER		(0x000c)
#define XVSEC_MCAP_STATUS_REGISTER	(0x0010)
#define XVSEC_MCAP_CONTROL_REGISTER	(0x0014)
#define XVSEC_MCAP_WRITE_DATA_REG	(0x0018)
#define XVSEC_MCAP_READ_DATA_REG0	(0x001c)
#define XVSEC_MCAP_READ_DATA_REG1	(0x0020)
#define XVSEC_MCAP_READ_DATA_REG2	(0x0024)
#define XVSEC_MCAP_READ_DATA_REG3	(0x0028)

#define XVSEC_MCAP_DATA_REG_CNT		4

#define XVSEC_MCAP_VSEC_ID_SHIFT	0
#define XVSEC_MCAP_REV_ID_SHIFT		16
#define XVSEC_MCAP_VSEC_ID_POS		(0xFFFF << XVSEC_MCAP_VSEC_ID_SHIFT)
#define XVSEC_MCAP_REV_ID_POS		(0xF << XVSEC_MCAP_REV_ID_SHIFT)

/* CTRL REG FIELDS */
#define XVSEC_MCAP_CTRL_ENABLE		(1 << 0)
#define XVSEC_MCAP_CTRL_RD_ENABLE	(1 << 1)
#define XVSEC_MCAP_CTRL_RESET		(1 << 4)
#define XVSEC_MCAP_CTRL_MOD_RESET	(1 << 5)
#define XVSEC_MCAP_CTRL_REQ_ACCESS	(1 << 8)
#define XVSEC_MCAP_CTRL_CFG_SWICTH	(1 << 12)
#define XVSEC_MCAP_CTRL_WR_ENABLE	(1 << 16)

/* STATUS REG FIELDS */
#define XVSEC_MCAP_STATUS_ERR			(1 << 0)
#define XVSEC_MCAP_STATUS_EOS			(1 << 1)
#define XVSEC_MCAP_STATUS_RD_COMPLETE	(1 << 4)
#define XVSEC_MCAP_STATUS_RD_COUNT		(0x7 << 5)
#define XVSEC_MCAP_STATUS_FIFO_OVFL		(1 << 8)
#define XVSEC_MCAP_STATUS_FIFO_LEVEL	(0xF << 12)
#define XVSEC_MCAP_STATUS_ACCESS		(1 << 24)

/* FPGA CFG RD/WR SEQUENCE WORDS */
#define DUMMY_WORD				0xFFFFFFFF
#define BUS_WIDTH_SYNC			0x000000BB
#define BUS_WIDTH_DETECT		0x11220044
#define SYNC_WORD				0xAA995566
#define NOOP					0x20000000
#define TYPE1_WR_CMD			0x30008001
#define DESYNC					0x0000000D

#define TYPE1_HEADER_TYPE		0x1
#define FPGA_CFG_READ			0x1
#define FPGA_CFG_WRITE			0x2


enum oper {
	FPGA_WR_CMD = 0x0,
	FPGA_RD_CMD,
};

union type1_header {
	struct {
		uint32_t    word_count  : 11;
		uint32_t    reserved02  : 2;
		uint32_t    address     : 5;
		uint32_t    reserved01  : 9;
		uint32_t    opcode      : 2;
		uint32_t    header_type : 3;
	};
	uint32_t data;
};


int xvsec_mcap_reset(struct vsec_context *mcap_ctx);
int xvsec_mcap_module_reset(struct vsec_context *mcap_ctx);
int xvsec_mcap_full_reset(struct vsec_context *mcap_ctx);
int xvsec_mcap_get_data_regs(struct vsec_context *mcap_ctx,
	uint32_t regs[4]);
int xvsec_mcap_get_regs(struct vsec_context *mcap_ctx,
	union mcap_regs *regs);
int xvsec_mcap_get_fpga_regs(struct vsec_context *mcap_ctx,
	union fpga_cfg_regs *regs);
int xvsec_mcap_program_bitstream(struct vsec_context *mcap_ctx,
	union bitstream_file *bit_files);
int xvsec_mcap_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data);
int xvsec_mcap_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data);
int xvsec_fpga_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg);
int xvsec_fpga_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg);

/*unsupported for US/US+ */
int xvsec_mcapv1_axi_rd_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg);
int xvsec_mcapv1_axi_wr_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg);
int xvsec_mcapv1_file_download(struct vsec_context *mcap_ctx,
	union file_download_upload *file);
int xvsec_mcapv1_file_upload(struct vsec_context *mcap_ctx,
	union file_download_upload *file);
int xvsec_mcapv1_set_axi_cache_attr(struct vsec_context *mcap_ctx,
	union axi_cache_attr *attr);


#endif /* __XVSEC_MCAP_US_H__ */
