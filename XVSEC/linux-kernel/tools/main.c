/*
 * This file is part of the XVSEC userspace application
 * to enable the user to execute the XVSEC functionality
 *
 * Copyright (c) 2018-2020  Xilinx, Inc.
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
#include "xvsec_parser.h"

static char version[] =
	XVSEC_TOOL_MODULE_DESC "\t: v" XVSEC_TOOL_VERSION "\n";

static int execute_xvsec_help_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_xvsec_verbose_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_xvsec_list_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int validate_capability(xvsec_handle_t *xvsec_handle,
	struct args *args);

/*extern def */
extern int get_mcap_version(xvsec_handle_t *handle, enum mcap_revision *mrev);

struct xvsec_ops xvsec_ops[] = {
	{XVSEC_HELP,		execute_xvsec_help_cmd},
	{XVSEC_VERBOSE,		execute_xvsec_verbose_cmd},
	{XVSEC_LIST_VSECS,	execute_xvsec_list_cmd},
	{XVSEC_OP_END,		NULL}
};

void SignalHandler(int SignalNum)
{
	int j, nptrs;
	void *buffer[100];
	char **strings;

	fprintf(stderr, "Received Signal : %d\n", SignalNum);

	nptrs = backtrace(buffer, 100);
	fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	would produce similar output to the following: */

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
	{
		printf("%s\n", strings[j]);
	}

	free(strings);

	exit(-1);
}

static void xvsec_common_help(FILE *fp)
{
	fprintf(fp, "Usage: xvsec -b <Bus No> -F <Device No> "
			"-c <Capability ID> [Capability Supported Options]\n");
	fprintf(fp, "     : xvsec -b <Bus No> -F <Device No> -l\n");
	fprintf(fp, "     : xvsec -h/-H\n");
	fprintf(fp, "     : xvsec -v\n\n");
	fprintf(fp, "Options:\n");
	fprintf(fp,
			"\t-h/H\t\t\tHelp\n"
			"\t-b    <bus_no>\tSpecify PCI bus no on which device sits\n"
			"\t-F    <dev_no>\tSpecify PCI device no on the bus\n"
			"\t-c    <cap_id>\tSpecify the capability ID\n"
			"\t-l    \t\tList the supported Xilinx VSECs\n"
			"\t-v    \t\tVerbose information of Device\n"
			"\n");
}

static void ultrascale_help(FILE *fp)
{
	fprintf(fp,
			"US/US+ MCAP options(Ext Cap ID : 0xB, VSEC ID:0x0001, VSEC Rev : 0 or 1):\n"
			"\t-p    <file>\t Program Bitstream(.bin/.bit/.rbt)\n"
			"\t-C    <file>\t Partial Reconfig Clear File(.bin/.bit/.rbt)\n"
			"\t-r\t\t Performs Simple Reset\n"
			"\t-m\t\t Performs Module Reset\n"
			"\t-f\t\t Performs Full Reset\n"
			"\t-D\t\t Read Data Registers\n"
			"\t-d\t\t Dump all the MCAP Registers\n"
			"\t-o\t\t Dump FPGA Config Registers\n"
			"\t-a <byte offset> [type [data]]  Access the MCAP Registers\n"
			"\t\t      here type[data] - b for byte data [8 bits]\n"
			"\t\t      here type[data] - h for half word data [16 bits]\n"
			"\t\t      here type[data] - w for word data [32 bits]\n"
			"\t-s <register no> [w [data]]  Access FPGA Config Registers\n"
			"\t\t      here [w [data]] - Write operation with data\n"
			"\t\t                      - Read Operation if 'w' not given\n"
			"\t\t     "
			"\n");
}

static void versal_help(FILE *fp)
{
	fprintf(fp,
			"VERSAL MCAP options(Ext Cap ID : 0xB, VSEC ID:0x0001, VSEC Rev : 2):\n"
			"\t-m\t\t Performs Module Reset\n"
			"\t-d\t\t Print the contents of the MCAP Registers\n"
			"\t-p  mode <32b/128b> type <fixed/incr> <address> <file> [tr_mode <slow/fast>]   Download the specified File (.pdi) at given address\n"
			"\t    \tmode <32b>   - 32-bit mode should be used\n"
			"\t    \tmode <128b>  - 128-bit mode should be used\n"
			"\t    \ttype <fixed> - Address is fixed\n"
			"\t    \ttype <incr>  - Address should be incremented based on specified mode\n"
			"\t    \ttr_mode      - optional slow/fast download mode option\n"
			"\t-t  type <fixed/incr> <address> <len> <file>   Read the contents at given address for the given len into given file\n"
			"\t    \ttype <fixed> - Address is fixed\n"
			"\t    \ttype <incr>  - Address should be incremented based on specified mode\n"
			"\t-a  <byte offset> [type [data]]                   Access the MCAP Registers\n"
			"\t    \there type[data] - b for byte data [8 bits]\n"
			"\t    \there type[data] - h for half word data [16 bits]\n"
			"\t    \there type[data] - w for word data [32 bits]\n"
			"\t-x  [mode <32b/128b>] <address> [w [data]]         Access the AXI Registers\n"
			"\t    \t[mode <32b/128b>] - here mode is valid only for 32b/128b write\n"
			"\t    \t                  - Not valid for Read Operation\n"
			"\t    \there [w [data]]   - Write operation with data\n"
			"\t    \t                  - Read Operation if 'w' not given\n"
			"\t-q  [axi_cache <data> axi_prot <data>]	Set the AXI cache and protections bits\n"
			"\t                        - axi_cache valid range 0 to 15, axi_prot valid range 0 to 7\n"
			"\t\t     "
			"\n");
}

static void usage(FILE *fp)
{
	xvsec_common_help(fp);
	ultrascale_help(fp);
	versal_help(fp);
}

static int execute_xvsec_help_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{

	if(args == NULL)
		return XVSEC_FAILURE;

	if(args->help.flag == false)
		return XVSEC_FAILURE;

	usage(stdout);

	return 0;
}

static int execute_xvsec_verbose_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

	if((xvsec_handle == NULL) || (args == NULL))
		return XVSEC_FAILURE;

	if(args->verbose.flag == false)
		return XVSEC_FAILURE;


	fprintf(stdout, "%s", version);
	ret = xvsec_show_device(xvsec_handle);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_show_device failed "
			"with error %d(%s) handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
	}

	return 0;
}

static int execute_xvsec_list_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	uint16_t index;

	if((xvsec_handle == NULL) || (args == NULL))
		return XVSEC_FAILURE;

	if(args->list_caps.flag == false)
		return XVSEC_FAILURE;

	memset(&args->list_caps.cap_list, 0, sizeof(xvsec_cap_list_t));
	ret = xvsec_get_cap_list(xvsec_handle, &args->list_caps.cap_list);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_get_cap_list failed with error %d(%s) "
			"handle : 0x%lX\n", ret, error_codes[-ret],
			*xvsec_handle);
		return 0;
	}

	fprintf(stdout, "No of Supported Extended capabilities : %d\n",
			args->list_caps.cap_list.no_of_caps);


	fprintf(stdout, "VSEC ID\t\tVSEC Rev\tVSEC Name\tDriver Support\n");
	fprintf(stdout, "-------\t\t--------\t---------\t--------------\n");
	for(index = 0; index < args->list_caps.cap_list.no_of_caps; index++)
	{
		fprintf(stdout, "0x%04X\t\t0x%04X\t\t%-9s\t%-14s\n",
			args->list_caps.cap_list.cap_info[index].cap_id,
			args->list_caps.cap_list.cap_info[index].rev_id,
			args->list_caps.cap_list.cap_info[index].cap_name,
			(args->list_caps.cap_list.cap_info[index].is_supported ? "Yes" : "No"));
	}

	return 0;
}

static int validate_capability(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;
	uint16_t index;
	bool is_capable;
	xvsec_cap_list_t cap_list;

	if(args->cap_id == 0xFFFF)
	{
		fprintf(stderr, "ERROR: Capability ID(-c) "
			"argument is missing\n");
		return XVSEC_ERR_CAPABILITY_ID_MISSING;
	}

	memset(&cap_list, 0, sizeof(xvsec_cap_list_t));
	ret = xvsec_get_cap_list(xvsec_handle, &cap_list);
	if(ret < 0)
	{
		fprintf(stderr, "xvsec_get_cap_list failed with error %d(%s) "
			"handle : 0x%lX\n",
			ret, error_codes[-ret], *xvsec_handle);
		return ret;
	}

	is_capable = false;
	for(index = 0; index < cap_list.no_of_caps; index++)
	{
		if(cap_list.cap_info[index].cap_id == args->cap_id)
		{
			is_capable = true;
		}
	}

	if(is_capable == false)
	{
		fprintf(stderr, "ERROR: Capability ID(-c) %d "
			"is not supported\n", args->cap_id);
		return XVSEC_ERR_CAPABILITY_NOT_SUPPORTED;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int			ret = 0, sts = 0;
	int                     no_of_devices = 2; /* no_of_devices: Need 2 user_ctx to handle two character dev for each target */
	xvsec_handle_t		xvsec_handle = -1;
	xvsec_handle_t		xvsec_handle_mcap = -1;
	struct args		args;
	enum xvsec_operation	op;
	enum mcap_operation	mcap_op;

	if (getuid() != 0)
	{
		fprintf(stderr, "%s : Please run in sudo mode\n", argv[0]);
		return XVSEC_FAILURE;
	}

	/* Check for arguments validity */
	if(argc < 2)
	{
		usage(stderr);
		return XVSEC_FAILURE;
	}

	signal(SIGSEGV, SignalHandler);

	memset(&args, 0, sizeof(struct args));
	args.bus_no = 0xFFFF;
	args.dev_no = 0xFFFF;
	args.cap_id = 0xFFFF;
	args.rev_id.mrev = XVSEC_INVALID_MCAP_REVISION;
	/* Parse the arguments */
	sts = parse_arguments(argc, argv, &args);
	if(sts < 0) {
		fprintf(stderr, "parse_arguments failed\n");
		return XVSEC_FAILURE;
	}

	if (args.parse_err == true) {
		fprintf(stderr, "Given Invalid Argument combination\n\n");
		usage(stderr);
		return XVSEC_FAILURE;
	}

	sts = xvsec_ops[XVSEC_HELP].execute(NULL, &args);
	if(sts == 0)
	{
		return 0;
	}

	/* Check for mandatory options provided in arguments list */
	if((args.bus_no == 0xFFFF) || (args.dev_no == 0xFFFF))
	{
		fprintf(stderr, "ERROR: bus number(-b), device number(-F), "
			"options are mandatory\n");
		return XVSEC_FAILURE;
	}

	sts = xvsec_lib_init(no_of_devices);
	if(sts < 0)
	{
		ret = sts;
		fprintf(stderr, "xvsec_lib_init failed with error %d(%s)\n",
			ret, error_codes[-ret]);
		return ret;
	}

	/* open system call for XVSEC character driver */
	memset(&xvsec_handle, 0, sizeof(xvsec_handle));
	sts = xvsec_open(args.bus_no, args.dev_no, &xvsec_handle, XVSEC_DEV_STR);
	if(sts < 0)
	{
		ret = sts;
		fprintf(stderr, "xvsec_handle: xvsec_open failed with error %d(%s) "
			"for bus no : %d, device no : %d\n",
			ret, error_codes[-ret], args.bus_no, args.dev_no);
		goto CLEANUP0;
	}

	/* Execute commonly Supported XVSEC Operations if requested */
	for(op = XVSEC_VERBOSE; xvsec_ops[op].execute != NULL; op++)
	{
		sts = xvsec_ops[op].execute(&xvsec_handle, &args);
		if(sts == 0)
		{
			goto CLEANUP1;
		}
	}

	sts = validate_capability(&xvsec_handle, &args);
	if(sts < 0)
	{
		ret = sts;
		goto CLEANUP1;
	}

	if(args.cap_id == MCAP_CAP_ID)
	{
		/* open system call for MCAP character driver */
		memset(&xvsec_handle_mcap, 0, sizeof(xvsec_handle_mcap));
		sts = xvsec_open(args.bus_no, args.dev_no, &xvsec_handle_mcap, XVSEC_MCAP_DEV_STR);
		if(sts < 0)
		{
			ret = sts;
			fprintf(stderr, "xvsec_handle_mcap: MCAP xvsec_open failed with error %d(%s) "
					"for bus no : %d, device no : %d\n",
					ret, error_codes[-ret], args.bus_no, args.dev_no);

			sts = xvsec_close(&xvsec_handle);
			if(sts < 0)
			{
				ret = sts;
				fprintf(stderr, "xvsec_handle: xvsec_close failed with error %d(%s) "
					"for bus no : %d, device no : %d\n", ret,
					error_codes[-ret], args.bus_no, args.dev_no);
			}

			goto CLEANUP1;
		}

		sts = xvsec_lib_get_mcap_revision(&xvsec_handle_mcap, (uint8_t *)&args.rev_id.mrev);

		if( sts != XVSEC_SUCCESS )
		{
			fprintf(stderr, "Failed to get MCAP version with sts: %d\n", sts);
			goto CLEANUP1;
		}
		else
		{
			fprintf(stdout, "MCAP version: %d\n", args.rev_id.mrev);
		}

		/*parse args dependent on mcap version*/
		sts = parse_mcap_arguments(argc, argv, &args);
		if(sts != XVSEC_SUCCESS)
		{
			ret = sts;
			goto CLEANUP1;
		}

		for(mcap_op = MCAP_RESET;
			mcap_ops[mcap_op].execute != NULL; mcap_op++)
		{
			sts = mcap_ops[mcap_op].execute(&xvsec_handle_mcap, &args);
			if(sts == 0)
			{
				break;
			}
		}

		if(mcap_ops[mcap_op].execute == NULL)
		{
			fprintf(stderr, "ERR : Invalid Command Given\n");
			goto CLEANUP1;
		}
	}

CLEANUP1:
	if (xvsec_handle != (xvsec_handle_t)(-1)) {
		sts = xvsec_close(&xvsec_handle);
		if(sts < 0)
		{
			ret = sts;
			fprintf(stderr, "xvsec_close failed with error %d(%s) "
				"for bus no : %d, device no : %d\n", ret,
				error_codes[-ret], args.bus_no, args.dev_no);
		}
	}

	if (xvsec_handle_mcap != (xvsec_handle_t)(-1)) {
		sts = xvsec_close(&xvsec_handle_mcap);
		if(sts < 0)
		{
			ret = sts;
			fprintf(stderr, "xvsec_close for mcap failed with "
				"error %d(%s) for bus no : %d, device no : %d\n",
				 ret, error_codes[-ret], args.bus_no, args.dev_no);
		}
	}


CLEANUP0:
	sts = xvsec_lib_deinit();
	if(sts < 0)
	{
		ret = sts;
		fprintf(stderr, "xvsec_lib_deinit failed with error %d(%s)\n",
			ret, error_codes[-ret]);
	}
	return ret;
}
