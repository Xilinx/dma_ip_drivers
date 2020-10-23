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

#ifndef __XVSEC_MCAP_VERSAL_H__
#define __XVSEC_MCAP_VERSAL_H__

/**
 * @file xvsec_mcap_versal.h
 *
 * Xilinx XVSEC Driver header file for versal MCAP register definations.
 *
 * Header file *xvsec_mcap_versal.h* declares mcap register macros and
 * functions to use internal to driver to configure MCAP and AXI
 *
 */

#define MCAPV2_PDI_FILE		".pdi"
#define MAX_FILE_LEN		300

#define MAX_FRAG_SZ			512 /** in bytes */
/**
 * Versal S80 ES2 HW Limitation:
 * Increased Polling timout as a workaround due to HW limitation for SBI.
 * SBI is back pressuring at certain data locations in PDI
 * due to PLM decoding/processing involved. Due to this it's getting
 * timeout for rw_complete status for some of the data locations
 *
 */
#define MAX_OP_POLL_CNT		100000
#define MAX_FIFO_OCCUPANCY	16

#define MIN_LEN_32B			4
#define MIN_LEN_128B		16
#define truncate_len_32b_mode(val)	\
		do { \
			if (val % 4) \
				val = (val / 4) * 4 ;\
		} while (0)
#define truncate_len_128b_mode(val)	\
	do { \
		if (val % 16) \
			val = (val / 16) * 16; \
	} while (0)

/* MCAP Versal Register Offsets */
#define XVSEC_MCAPV2_EXTENDED_HEADER	(0x0000)
#define XVSEC_MCAPV2_VENDOR_HEADER	(0x0004)
#define XVSEC_MCAPV2_STATUS_REG		(0x0008)
#define XVSEC_MCAPV2_CONTROL_REG	(0x000c)
#define XVSEC_MCAPV2_RW_ADDRESS		(0x0010)
#define XVSEC_MCAPV2_WRITE_DATA_REG	(0x0014)
#define XVSEC_MCAPV2_READ_DATA_REG	(0x0018)

/* CTRL REG FIELDS */
#define XVSEC_MCAPV2_CTRL_READ_ENABLE	(1 << 0)
#define XVSEC_MCAPV2_CTRL_WRITE_ENABLE	(1 << 4)
#define XVSEC_MCAPV2_CTRL_128B_MODE	(1 << 5)
#define XVSEC_MCAPV2_CTRL_RESET		(1 << 8)
#define XVSEC_MCAPV2_CTRL_AXI_CACHE	(0xF << 16) /* AXI Cache 19:16 */
#define XVSEC_MCAPV2_CTRL_AXI_PROTECT	(0x7 << 20) /* AXI Protect 22:20 */

#define XVSEC_MCAPV2_CTRL_AXI_CACHE_MASK 0xF /* AXI Cache MASK */
#define XVSEC_MCAPV2_CTRL_AXI_CACHE_SHIFT 16 /* AXI Cache SHIFT */

#define XVSEC_MCAPV2_CTRL_AXI_PROTECT_MASK 0x7 /* AXI PROTECT MASK */
#define XVSEC_MCAPV2_CTRL_AXI_PROTECT_SHIFT 20 /* AXI PROTECT SHIFT */

/** STATUS REG FIELDS */

/** Read/Write Status 5:4 */
#define XVSEC_MCAPV2_STATUS_RW_STS			(0x3 << 4)
#define XVSEC_MCAPV2_STATUS_RW_COMPLETE			(0x1 << 8)
/* FIFO Occupancy 20:16 */
#define XVSEC_MCAPV2_STATUS_FIFO_OCCUPANCY		(0x1F << 16)
#define XVSEC_MCAPV2_STATUS_WRITE_FIFO_FULL		(0x1 << 21)
#define XVSEC_MCAPV2_STATUS_WRITE_FIFO_ALMOST_FULL	(0x1 << 22)
#define XVSEC_MCAPV2_STATUS_WRITE_FIFO_ALMOST_EMPTY	(0x1 << 23)
#define XVSEC_MCAPV2_STATUS_WRITE_FIFO_EMPTY		(0x1 << 24)
#define XVSEC_MCAPV2_STATUS_WRITE_FIFO_OVERFLOW		(0x1 << 25)

#define XVSEC_MCAPV2_STATUS_FIFO_OCCUPANCY_MASK		(0x1F)
#define XVSEC_MCAPV2_STATUS_FIFO_OCCUPANCY_SHIFT	(16)

#define XVSEC_MCAPV2_STATUS_RW_STS_MASK			(0x3)
#define XVSEC_MCAPV2_STATUS_RW_STS_SHIFT		(4)

#define XVSEC_MCAPV2_IS_RW_COMPLETE(sts) \
	((sts & XVSEC_MCAPV2_STATUS_RW_COMPLETE) ? true : false)
#define XVSEC_MCAPV2_IS_FIFO_FULL(sts) \
	((sts & XVSEC_MCAPV2_STATUS_WRITE_FIFO_FULL) ? true : false)
#define XVSEC_MCAPV2_IS_FIFO_EMPTY(sts) \
	((sts & XVSEC_MCAPV2_STATUS_WRITE_FIFO_EMPTY) ? true : false)
#define XVSEC_MCAPV2_IS_FIFO_OVERFLOW(sts) \
	((sts & XVSEC_MCAPV2_STATUS_WRITE_FIFO_OVERFLOW) ? true : false)

#define XVSEC_MCAPV2_GET_FIFO_OCCUPANCY(sts, occupancy) { \
		occupancy = (uint8_t)(sts >> \
			XVSEC_MCAPV2_STATUS_FIFO_OCCUPANCY_SHIFT) & \
			XVSEC_MCAPV2_STATUS_FIFO_OCCUPANCY_MASK; \
	}

#define XVSEC_MCAPV2_GET_RW_STATUS(sts, rw_status) { \
		rw_status = (uint8_t)(sts >>  \
			XVSEC_MCAPV2_STATUS_RW_STS_SHIFT) & \
			XVSEC_MCAPV2_STATUS_RW_STS_MASK; \
	}

enum xvsec_mcapv2_rw_status {
	XVSEC_MCAPV2_RW_OK = 0,
	XVSEC_MCAPV2_SLVERR,
	XVSEC_MCAPV2_DECERR
};

int xvsec_mcapv2_module_reset(struct vsec_context *mcap_ctx);
int xvsec_mcapv2_get_regs(
	struct vsec_context *mcap_ctx, union mcap_regs *regs);
int xvsec_mcapv2_rd_cfg_addr(
	struct vsec_context *mcap_ctx, union cfg_data *data);
int xvsec_mcapv2_wr_cfg_addr(
	struct vsec_context *mcap_ctx, union cfg_data *data);
int xvsec_mcapv2_axi_rd_addr(
	struct vsec_context *mcap_ctx, union axi_reg_data *cfg_reg);
int xvsec_mcapv2_axi_wr_addr(
	struct vsec_context *mcap_ctx, union axi_reg_data *cfg_reg);
int xvsec_mcapv2_file_download(
	struct vsec_context *mcap_ctx, union file_download_upload *file_info);
int xvsec_mcapv2_file_upload(
	struct vsec_context *mcap_ctx, union file_download_upload *file_info);
int xvsec_mcapv2_set_axi_cache_attr(
	struct vsec_context *mcap_ctx, union axi_cache_attr *attr);


/*unsupported for versal devices */
int xvsec_mcapv2_reset(struct vsec_context *mcap_ctx);
int xvsec_mcapv2_full_reset(struct vsec_context *mcap_ctx);
int xvsec_mcapv2_get_data_regs(struct vsec_context *mcap_ctx, uint32_t regs[4]);
int xvsec_mcapv2_get_fpga_regs(
	struct vsec_context *mcap_ctx, union fpga_cfg_regs *regs);
int xvsec_mcapv2_program_bitstream(
	struct vsec_context *mcap_ctx, union bitstream_file *bit_files);
int xvsec_fpgav2_rd_cfg_addr(
	struct vsec_context *mcap_ctx, union fpga_cfg_reg *cfg_reg);
int xvsec_fpgav2_wr_cfg_addr(
	struct vsec_context *mcap_ctx, union fpga_cfg_reg *cfg_reg);

#endif /* __XVSEC_MCAP_VERSAL_H__ */
