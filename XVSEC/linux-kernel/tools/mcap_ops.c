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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <execinfo.h>

#include "version.h"
#include "xvsec.h"
#include "main.h"
#include "mcap_ops.h"

static const char *MCAP_reg_names[] = {
	"Ext Capability",
	"VSEC Header",
	"FPGA JTAG ID",
	"FPGA BitStream Ver",
	"Status",
	"Control",
	"FPGA Write Data",
	"FPGA Read Data[0]",
	"FPGA Read Data[1]",
	"FPGA Read Data[2]",
	"FPGA Read Data[3]",
};

static const char *MCAP_sts_fields[] = {
	"MCAP Error",
	"MCAP EOS",
	"MCAP Read Complete",
	"MCAP Read Count",
	"MCAP FIFO Overflow",
	"MCAP FIFO Occupancy",
	"Req for MCAP Release"
};

static const char *MCAP_ctl_fields[] = {
	"MCAP Enable",
	"MCAP Read Enable",
	"MCAP Reset",
	"MCAP Module Reset",
	"Req for MCAP by PCIe",
	"MCAP Design Switch",
	"Write Data Reg Enable"
};

static const char *fpga_cfg_reg_names[] = {
	"crc", "far", "fdri", "fdro",
	"cmd", "ctl0", "mask", "stat",
	"lout", "cor0", "mfwr", "cbc",
	"idcode", "axss", "cor1", "wbstar",
	"timer", "scratchpad", "bootsts", "ctl1",
	"bspi"
};

static const uint32_t fpga_cfg_reg_num[] = {
	0x00, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x10, 0x11, 0x14, 0x16, 0x18,
	0x1F
};

static int execute_mcap_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_module_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_full_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_data_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_fpga_cfg_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_access_reg_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_fpga_cfg_access_reg_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_mcap_program_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);

static void print_mcap_sts_fields(uint32_t val);
static void print_mcap_ctl_fields(uint32_t val);

struct mcap_ops mcap_ops[] = {
	{MCAP_RESET,			execute_mcap_reset_cmd              },
	{MCAP_MODULE_RESET,		execute_mcap_module_reset_cmd       },
	{MCAP_FULL_RESET,		execute_mcap_full_reset_cmd         },
	{MCAP_GET_DATA_REGS,		execute_mcap_data_regs_cmd          },
	{MCAP_GET_REGS,			execute_mcap_regs_cmd               },
	{MCAP_GET_FPGA_CFG_REGS,	execute_mcap_fpga_cfg_regs_cmd      },
	{MCAP_ACCESS_REG,		execute_mcap_access_reg_cmd         },
	{MCAP_ACCESS_FPGA_CFG_REG,	execute_mcap_fpga_cfg_access_reg_cmd},
	{MCAP_PROGRAM_BITSTREAM,	execute_mcap_program_cmd            },
	{MCAP_OP_END,			NULL                                }
};

static int execute_mcap_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if(args->reset.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_reset(xvsec_handle);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_reset failed "
			"with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		fprintf(stdout, "Configuration Logic Reset Successful\n");
	}

	return 0;
}

static int execute_mcap_module_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if(args->module_reset.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_module_reset(xvsec_handle);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_module_reset "
			"failed with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		fprintf(stdout, "MCAP Module Reset Successful\n");
	}

	return 0;
}

static int execute_mcap_full_reset_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if(args->full_reset.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_full_reset(xvsec_handle);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_full_reset failed "
			"with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		fprintf(stdout, "Both Configuration Logic & "
			"MCAP Module Reset Successful\n");
	}

	return 0;
}

static int execute_mcap_data_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if(args->data_dump.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_get_data_registers(xvsec_handle,
		&args->data_dump.mcap_data_reg[0]);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_get_data_registers "
			"failed with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		fprintf(stdout, "BYTE OFFSET\tRegister Name"
			"\t\tData Value\n");
		fprintf(stdout, "-----------\t----------------"
			"\t----------\n");
		fprintf(stdout, "0x001C\t\tFPGA Read Data[0]\t"
			"0x%08X\n", args->data_dump.mcap_data_reg[0]);
		fprintf(stdout, "0x0020\t\tFPGA Read Data[1]\t"
			"0x%08X\n", args->data_dump.mcap_data_reg[1]);
		fprintf(stdout, "0x0024\t\tFPGA Read Data[2]\t"
			"0x%08X\n", args->data_dump.mcap_data_reg[2]);
		fprintf(stdout, "0x0028\t\tFPGA Read Data[3]\t"
			"0x%08X\n", args->data_dump.mcap_data_reg[3]);
	}
	return 0;
}

static int execute_mcap_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	uint16_t index;
	uint16_t reg_count;
	uint32_t *reg_value;

	if(args->reg_dump.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_get_registers(xvsec_handle, &args->reg_dump.mcap_regs);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_get_registers "
			"failed with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		reg_count = sizeof(args->reg_dump.mcap_regs)/sizeof(uint32_t);
		reg_value = (uint32_t *)&args->reg_dump.mcap_regs;
		fprintf(stdout, "BYTE OFFSET\tRegister Name\t\tData Value\n");
		fprintf(stdout, "-----------\t----------------\t----------\n");
		for(index = 0; index < reg_count; index++)
		{
			fprintf(stdout, "0x%04X\t\t%-20s\t0x%08X\n",
				index*4, MCAP_reg_names[index], reg_value[index]);
			if((index*4) == MCAP_STS_REG_OFFSET)
				print_mcap_sts_fields(reg_value[index]);
			if((index*4) == MCAP_CTL_REG_OFFSET)
				print_mcap_ctl_fields(reg_value[index]);
		}
	}
	return 0;
}

static int execute_mcap_fpga_cfg_regs_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	uint16_t index;
	uint16_t reg_count;
	uint32_t *reg_value;

	if(args->fpga_reg_dump.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_get_fpga_registers(
		xvsec_handle, &args->fpga_reg_dump.fpga_cfg_regs);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_get_fpga_registers "
			"failed with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}
	else
	{
		reg_count = sizeof(fpga_cfg_reg_num)/sizeof(uint32_t);
		reg_value = (uint32_t *)&args->fpga_reg_dump.fpga_cfg_regs;

		fprintf(stdout, "FPGA CFG Registers Dump "
			"(see Configuration User Guide for "
			"more details)\n\n");
		fprintf(stdout, "Register No\tRegister Name\t\tData Value\n");
		fprintf(stdout, "-----------\t----------------\t----------\n");

		for(index = 0; index < reg_count; index++)
		{
			fprintf(stdout, "0x%04X\t\t%-20s\t0x%08X\n",
				fpga_cfg_reg_num[index],
				fpga_cfg_reg_names[index],
				reg_value[index]);
		}
	}

	return 0;
}

static int execute_mcap_access_reg_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	access_type_t access;
	char print_buf[200];

	if(args->access_reg.flag == false)
		return XVSEC_FAILURE;

	if(args->access_reg.access_type == 'b')
		access = ACCESS_BYTE;
	else if(args->access_reg.access_type == 'h')
		access = ACCESS_SHORT;
	else
		access = ACCESS_WORD;

	ret = xvsec_mcap_access_config_reg(xvsec_handle,
		args->access_reg.offset,
		(void *)&args->access_reg.data,
		access,
		args->access_reg.write);
	if(ret < 0)
	{
		if(ret == XVSEC_ERR_INVALID_OFFSET_ACCESS_COMBO)
		{
			fprintf(stderr, "Error : The Address "
				"specified is invalid for "
				"the access type \'%c\'\n",
				args->access_reg.access_type);
		}
		else if(ret == XVSEC_ERR_INVALID_OFFSET)
		{
			fprintf(stderr, "Error : The specified "
			"VSEC offset of 0x%X is "
			"not Valid.\nUse the -d option to "
			"dump supported VSEC "
			"registers.\n", args->access_reg.offset);
		}
		else
		{
			fprintf(stderr,
				"xvsec_mcap_access_config_reg "
				"failed with error %d(%s) handle : 0x%lX\n",
				ret, error_codes[-ret], *xvsec_handle);
		}
	}
	else
	{
		if(access == ACCESS_WORD)
		{
			snprintf(print_buf, 200, "0x%04X\t\t%-20s\t0x%08X\n",
				args->access_reg.offset,
				MCAP_reg_names[args->access_reg.offset/4],
				args->access_reg.data);
		}
		else if(access == ACCESS_SHORT)
		{
			snprintf(print_buf, 200, "0x%04X\t\t%-20s\t0x%04X\n",
				args->access_reg.offset,
				MCAP_reg_names[args->access_reg.offset/4],
				args->access_reg.data);
		}
		else if(access == ACCESS_BYTE)
		{
			snprintf(print_buf, 200, "0x%04X\t\t%-20s\t0x%02X\n",
				args->access_reg.offset,
				MCAP_reg_names[args->access_reg.offset/4],
				args->access_reg.data);
		}
		fprintf(stdout, "BYTE OFFSET\tRegister Name\t\tData Value\n");
		fprintf(stdout, "-----------\t----------------\t----------\n");
		fprintf(stdout, "%s", print_buf);
	}

	return 0;
}

static int execute_mcap_fpga_cfg_access_reg_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	uint16_t index;
	uint16_t reg_count;

	if(args->fpga_access_reg.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_access_fpga_config_reg(
		xvsec_handle, args->fpga_access_reg.offset,
		(void *)&args->fpga_access_reg.data,
		args->fpga_access_reg.write);
	if(ret < 0)
	{
		if(ret == XVSEC_ERR_INVALID_FPGA_REG_NUM)
		{
			fprintf(stderr, "Error : The specified "
				"configuration register offset of 0x%X is "
				"not Valid.\nUse the -o option to "
				"dump supported FPGA configuration "
				"registers.\n", args->fpga_access_reg.offset);
		}
		else
		{
			fprintf(stderr,
				"xvsec_mcap_access_fpga_config_reg "
				"failed with error %d(%s) handle : 0x%lX\n",
				ret, error_codes[-ret], *xvsec_handle);
		}
	}
	else
	{
		reg_count = sizeof(fpga_cfg_reg_num)/sizeof(uint32_t);
		for(index = 0; index < reg_count; index++)
		{
			if(fpga_cfg_reg_num[index] ==
				args->fpga_access_reg.offset)
			{
				break;
			}
		}
		if(index == reg_count)
		{
			fprintf(stderr, "Error : The specified "
				"configuration register offset of 0x%X is "
				"not Valid.\nUse the -o option to "
				"dump supported FPGA configuration "
				"registers.\n", args->fpga_access_reg.offset);
			ret = XVSEC_ERR_INVALID_FPGA_REG_NUM;
			goto CLEANUP;
		}

		fprintf(stdout, "Register No\tRegister Name\t\tData Value\n");
		fprintf(stdout, "-----------\t----------------\t----------\n");

		fprintf(stdout, "0x%04X\t\t%-20s\t0x%08X\n",
			fpga_cfg_reg_num[index],
			fpga_cfg_reg_names[index],
			args->fpga_access_reg.data);
	}
CLEANUP:
	return 0;
}

static int execute_mcap_program_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if(args->program.flag == false)
		return XVSEC_FAILURE;

	ret = xvsec_mcap_configure_fpga(xvsec_handle,
		args->program.abs_clr_file,
		args->program.abs_bit_file);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_mcap_configure_fpga "
			"failed with error %d(%s) clear_file:%s, "
			"bit_file:%s, handle : 0x%lX\n", ret, error_codes[-ret],
			args->program.abs_clr_file,
			args->program.abs_bit_file, *xvsec_handle);
	}
	else
	{
		fprintf(stdout, "FPGA configuration successful\n");
	}

	if(args->program.abs_clr_file != NULL)
		free(args->program.abs_clr_file);
	if(args->program.abs_bit_file != NULL)
		free(args->program.abs_bit_file);

	return 0;
}

static void print_mcap_sts_fields(uint32_t val)
{
	xvsec_mcap_sts_reg_t *reg = (xvsec_mcap_sts_reg_t*)&val;

	fprintf(stdout, "   bit  0\t%-20s\t%10d\n", MCAP_sts_fields[0], reg->err);
	fprintf(stdout, "   bit  1\t%-20s\t%10d\n", MCAP_sts_fields[1], reg->eos);
	fprintf(stdout, "   bit  4\t%-20s\t%10d\n", MCAP_sts_fields[2], reg->read_complete);
	fprintf(stdout, "   bit 5:7\t%-20s\t%10d\n", MCAP_sts_fields[3], reg->read_count);
	fprintf(stdout, "   bit  8\t%-20s\t%10d\n", MCAP_sts_fields[4], reg->fifo_ovfl);
	fprintf(stdout, "   bit 12:15\t%-20s\t%10d\n", MCAP_sts_fields[5], reg->fifo_occu);
	fprintf(stdout, "   bit 24\t%-20s\t%10d\n", MCAP_sts_fields[6], reg->req4mcap_rel);
}

static void print_mcap_ctl_fields(uint32_t val)
{
	xvsec_mcap_ctl_reg_t *reg = (xvsec_mcap_ctl_reg_t *)&val;

	fprintf(stdout, "   bit  0\t%-20s\t%10d\n", MCAP_ctl_fields[0], reg->enable);
	fprintf(stdout, "   bit  1\t%-20s\t%10d\n", MCAP_ctl_fields[1], reg->rd_enable);
	fprintf(stdout, "   bit  4\t%-20s\t%10d\n", MCAP_ctl_fields[2], reg->reset);
	fprintf(stdout, "   bit  5\t%-20s\t%10d\n", MCAP_ctl_fields[3], reg->module_reset);
	fprintf(stdout, "   bit  8\t%-20s\t%10d\n", MCAP_ctl_fields[4], reg->req4mcap_pcie);
	fprintf(stdout, "   bit 12\t%-20s\t%10d\n", MCAP_ctl_fields[5], reg->cfg_desgn_sw);
	fprintf(stdout, "   bit 16\t%-20s\t%10d\n", MCAP_ctl_fields[6], reg->wr_reg_enable);
}
