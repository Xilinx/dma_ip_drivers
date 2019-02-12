/*
 * This file is part of the XVSEC userspace application
 * to enable the user to execute the XVSEC functionality
 *
 * Copyright (c) 2018,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */



#ifndef __MAIN_H__
#define __MAIN_H__

struct help_args {
	bool flag;
};

struct verbose_args {
	bool flag;
};

struct list_caps_args {
	bool flag;
	xvsec_cap_list_t cap_list;
};

struct mcap_reset_args {
	bool flag;
};

struct mcap_module_reset_args {
	bool flag;
};

struct mcap_full_reset_args {
	bool flag;
};

struct mcap_data_dump_args {
	bool flag;
	uint32_t mcap_data_reg[4];
};

struct mcap_reg_dump_args {
	bool flag;
	xvsec_mcap_regs_t mcap_regs;
};

struct fpga_cfg_reg_dump_args {
	bool flag;
	xvsec_fpga_cfg_regs_t fpga_cfg_regs;
};

struct mcap_access_reg {
	bool flag;
	bool write;
	char access_type;
	uint16_t offset;
	uint32_t data;
};

struct fpga_cfg_access_reg {
	bool flag;
	bool write;
	char cmd;
	uint16_t offset;
	uint32_t data;
};

struct mcap_program_bitstream {
	bool flag;
	char *abs_clr_file;
	char *abs_bit_file;
};

struct args {
	uint16_t			bus_no;
	uint16_t			dev_no;
	uint16_t			cap_id;
	bool				parse_err;
	struct help_args		help;
	struct verbose_args		verbose;
	struct list_caps_args		list_caps;
	struct mcap_reset_args		reset;
	struct mcap_module_reset_args	module_reset;
	struct mcap_full_reset_args	full_reset;
	struct mcap_data_dump_args	data_dump;
	struct mcap_reg_dump_args	reg_dump;
	struct fpga_cfg_reg_dump_args	fpga_reg_dump;
	struct mcap_access_reg		access_reg;
	struct fpga_cfg_access_reg	fpga_access_reg;
	struct mcap_program_bitstream	program;
};

enum xvsec_operation {
	XVSEC_HELP = 0,
	XVSEC_VERBOSE,
	XVSEC_LIST_VSECS,
	XVSEC_OP_END
};

struct xvsec_ops {
	enum xvsec_operation op;
	int (*execute)(xvsec_handle_t *xvsec_handle, struct args *args);
};

#endif /* __MAIN_H__ */
