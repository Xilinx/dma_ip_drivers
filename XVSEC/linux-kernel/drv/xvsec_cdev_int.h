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

#ifndef __XVSEC_CDEV_INT_H__
#define __XVSEC_CDEV_INT_H__

#define XVSEC_NODE_NAME			"xvsec"
#define INVALID_DEVICE_INDEX		(uint8_t)(255)
#define INVALID_OFFSET			(0xFFFF)
#define XVSEC_EXT_CAP_VSEC_ID		(0x000B)
#define XVSEC_MCAP_VSEC_ID		(0x0001)

#define MAX_FLEN			(300)
#define RBT_WORD_LEN			(32)

#define XVSEC_MCAP_LOOP_COUNT		(1000000)
#define EMCAP_EOS_LOOP_COUNT		(100)
#define EMCAP_NOOP_VAL			0x2000000
#define EMCAP_EOS_RETRY_COUNT		10
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

/* CTRL REG FIELDS */
#define XVSEC_MCAP_CTRL_ENABLE		(1 << 0)
#define XVSEC_MCAP_CTRL_RD_ENABLE	(1 << 1)
#define XVSEC_MCAP_CTRL_RESET		(1 << 4)
#define XVSEC_MCAP_CTRL_MOD_RESET	(1 << 5)
#define XVSEC_MCAP_CTRL_REQ_ACCESS	(1 << 8)
#define XVSEC_MCAP_CTRL_CFG_SWICTH	(1 << 12)
#define XVSEC_MCAP_CTRL_WR_ENABLE	(1 << 16)

/* STATUS REG FIELDS */
#define XVSEC_MCAP_STATUS_ERR		(1 << 0)
#define XVSEC_MCAP_STATUS_EOS		(1 << 1)
#define XVSEC_MCAP_STATUS_RD_COMPLETE	(1 << 4)
#define XVSEC_MCAP_STATUS_RD_COUNT	(0x7 << 5)
#define XVSEC_MCAP_STATUS_FIFO_OVFL	(1 << 8)
#define XVSEC_MCAP_STATUS_FIFO_LEVEL	(0xF << 12)
#define XVSEC_MCAP_STATUS_ACCESS	(1 << 24)

/* FPGA CFG RD/WR SEQUENCE WORDS */
#define DUMMY_WORD			0xFFFFFFFF
#define BUS_WIDTH_SYNC			0x000000BB
#define BUS_WIDTH_DETECT		0x11220044
#define SYNC_WORD			0xAA995566
#define NOOP				0x20000000
#define TYPE1_WR_CMD			0x30008001
#define DESYNC				0x0000000D

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

struct context {
	dev_t				dev_no;
	int				major_no;
	char				name[20];
	struct class			*class;
	struct cdev			cdev;
	struct device			*sys_device;

	struct pci_dev			*pdev;
	int				mcap_cap_offset;
	int				fopen_cnt;
	struct xvsec_capabilities	capabilities;
};

struct xvsec_dev {
	uint32_t	dev_cnt;
	struct context	*ctx;
};

struct file_priv {
	void *ctx;
};

struct ioctl_ops {
	uint32_t cmd;
	long (*fpfunction)(struct file *filep, uint32_t cmd, unsigned long arg);
};


extern struct xvsec_dev xvsec_dev;
extern struct class *g_xvsec_class;
extern int xvsec_initialize(struct pci_dev *pdev, uint32_t dev_idx);
extern int xvsec_deinitialize(uint32_t dev_idx);

#endif /* __XVSEC_CDEV_INT_H__ */
