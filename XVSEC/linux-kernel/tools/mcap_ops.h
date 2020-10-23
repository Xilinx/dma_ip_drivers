/*
 * This file is part of the XVSEC userspace application
 * to enable the user to execute the XVSEC functionality
 *
 * Copyright (c) 2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#ifndef __MCAP_OPS_H__
#define __MCAP_OPS_H__

#define MCAP_CAP_ID		(0x0001)
#define MCAP_STS_REG_OFFSET	(0x10)
#define MCAP_CTL_REG_OFFSET	(0x14)
#define MCAP_VSEC_HEADER_OFFSET	(0x04)
#define MCAP_VSEC_REV_MASK	(0xF0000)
#define MCAP_VSEC_REV_SHIFT	(0x10)

#define MCAPV2_STS_REG_OFFSET   (0x08)
#define MCAPV2_CTL_REG_OFFSET   (0x0C)

enum mcap_operation {
	MCAP_RESET = 0,
	MCAP_MODULE_RESET,
	MCAP_FULL_RESET,
	MCAP_GET_REVISION,
	MCAP_GET_DATA_REGS,
	MCAP_GET_REGS,
	MCAP_GET_FPGA_CFG_REGS,
	MCAP_ACCESS_REG,
	MCAP_ACCESS_FPGA_CFG_REG,
	MCAP_PROGRAM_BITSTREAM,
	MCAP_ACCESS_AXI_REG,
	MCAP_FILE_DOWNLOAD,
	MCAP_FILE_UPLOAD,
	MCAP_SET_AXI_CACHE_ATTR,
	MCAP_OP_END
};

struct mcap_ops {
	enum mcap_operation op;
	int (*execute)(xvsec_handle_t *xvsec_handle, struct args *args);
};

extern struct mcap_ops mcap_ops[];

#endif /* __MCAP_OPS_H__ */
