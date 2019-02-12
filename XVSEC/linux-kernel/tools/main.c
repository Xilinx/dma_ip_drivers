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



static char version[] =
	XVSEC_TOOL_MODULE_DESC "\t: v" XVSEC_TOOL_VERSION "\n";

static const char options[] =	":b:F:c:lp:C:rmfdvHhDoa:s:";

static int parse_arguments(int argc, char *argv[], struct args *args);

static int execute_xvsec_help_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_xvsec_verbose_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int execute_xvsec_list_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args);
static int validate_capability(xvsec_handle_t *xvsec_handle,
	struct args *args);

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

static void usage(FILE *fp)
{
	fprintf(fp, "Usage: xvsec -b <Bus No> -F <Device No> "
			"-c <Capability ID> [Capability Supported Options]\n");
	fprintf(fp, "     : xvsec -b <Bus No> -F <Device No> -l\n");
	fprintf(fp, "     : xvsec -h/-H\n");
	fprintf(fp, "     : xvsec -v\n\n");
	fprintf(fp, "Options:\n");
	fprintf(fp,
		"\t-h/H\t\t\tHelp\n"
		"\t-b	 <bus_no>\tSpecify PCI bus no on which device sits\n"
		"\t-F	 <dev_no>\tSpecify PCI device no on the bus\n"
		"\t-c	 <cap_id>\tSpecify the capability ID\n"
		"\t-l	 \t\tList the supported Xilinx VSECs\n"
		"\t-v	 \t\tVerbose information of Device\n"
		"\n"
		"MCAP options(Ext Cap ID : 0xB, VSEC ID:0x0001):\n"
		"\t-p	 <file>\t Program Bitstream(.bin/.bit/.rbt)\n"
		"\t-C	 <file>\t Partial Reconfig Clear File(.bin/.bit/.rbt)\n"
		"\t-r\t\t Performs Simple Reset\n"
		"\t-m\t\t Performs Module Reset\n"
		"\t-f\t\t Performs Full Reset\n"
		"\t-D\t\t Read Data Registers\n"
		"\t-d\t\t Dump all the MCAP Registers\n"
		"\t-o\t\t Dump FPGA Config Registers\n"
		"\t-a <byte offset> [type [data]]  Access the MCAP Registers\n"
		"\t\t	   here type[data] - b for byte data [8 bits]\n"
		"\t\t	   here type[data] - h for half word data [16 bits]\n"
		"\t\t	   here type[data] - w for word data [32 bits]\n"
		"\t-s <register no> [w [data]]  Access FPGA Config Registers\n"
		"\t\t	   here [w [data]] - Write operation with data\n"
		"\t\t	                   - Read Operation if 'w' not given\n"
		"\t\t	  "
		"\n");

}

static int parse_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	int opt;
	uint16_t len;
	char *bit_file = NULL;
	char *clear_file = NULL;

	if(args == NULL) {
		return XVSEC_FAILURE;
	}

	while ((opt = getopt(argc, argv, options)) != -1) {
		if(opt == ':')
		{
			fprintf(stderr, "Parameters missing for the "
				"Specified Option  : %c\n", optopt);
			args->help.flag = true;
			break;
		}
		else if(opt == '?')
		{
			fprintf(stderr, "Invalid Option : %c\n", optopt);
			args->help.flag = true;
			break;
		}

		switch (opt) {
		case 'b':
			args->bus_no = (uint16_t) strtol(optarg, NULL, 16);
			break;
		case 'F':
			args->dev_no = (uint16_t) strtol(optarg, NULL, 16);
			break;
		case 'c':
			args->cap_id = (uint16_t) strtol(optarg, NULL, 16);
			break;
		case 'h':
		case 'H':
			args->help.flag = true;
			break;
		case 'v':
			args->verbose.flag = true;
			break;
		case 'l':
			args->list_caps.flag = true;
			break;
		case 'r':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->reset.flag = true;
			break;
		case 'm':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->module_reset.flag = true;
			break;
		case 'f':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->full_reset.flag = true;
			break;
		case 'D':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->data_dump.flag = true;
			break;
		case 'd':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->reg_dump.flag = true;
			break;
		case 'o':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}
			args->fpga_reg_dump.flag = true;
			break;
		case 'a':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}

			if(argv[optind] == NULL)
			{
				fprintf(stderr, "Access option "
					"must be provided. Valid options are"
					" \'b/h/w\'\n");
				args->parse_err = true;
				break;
			}
			if(argv[optind][1] != '\0')
			{
				fprintf(stderr, "Invalid Access option "
					"provided %s (\'b/h/w\' are valid)\n",
					argv[optind]);
				args->parse_err = true;
				break;
			}
			args->access_reg.offset =
				(uint16_t) strtoul(optarg, NULL, 0);
			args->access_reg.access_type =
				(char)argv[optind][0];

			if((args->access_reg.access_type != 'b') &&
				(args->access_reg.access_type != 'h') &&
				(args->access_reg.access_type != 'w'))
			{
				fprintf(stderr, "Invalid Access option "
					"provided %s (\'b/h/w\' are valid)\n",
					argv[optind]);
				args->parse_err = true;
				break;
			}

			if(argv[optind + 1] != NULL)
			{
				args->access_reg.write = true;
				args->access_reg.data = (uint32_t)
					strtoul(argv[optind + 1], NULL, 0);
			}
			else
			{
				args->access_reg.write = false;
				args->access_reg.data = 0x0;
			}

			args->access_reg.flag = true;
			break;
		case 's':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}

			args->fpga_access_reg.offset =
				(uint16_t) strtoul(optarg, NULL, 0);

			if(argv[optind] == NULL)
			{
				args->fpga_access_reg.write = false;
				args->fpga_access_reg.cmd = '\0';
				args->fpga_access_reg.data = 0x0;
				args->fpga_access_reg.flag = true;
				break;
			}
			if(argv[optind + 1] == NULL)
			{
				args->parse_err = true;
				break;
			}
			args->fpga_access_reg.cmd = (char)argv[optind][0];
			if((args->fpga_access_reg.cmd == 'w') && \
				(argv[optind][1] == '\0'))
			{
				args->fpga_access_reg.write = true;
				args->fpga_access_reg.data = (uint32_t)
					strtoul(argv[optind + 1], NULL, 0);
			}
			else
			{
				fprintf(stderr, "Invalid option "
					"provided %s (\'w\' is valid "
					"option for write operation)\n",
					argv[optind]);
				args->parse_err = true;
				break;
			}
			printf("In -s option\n");
			args->fpga_access_reg.flag = true;
			break;
		case 'p':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}

			len = strlen(optarg) + 1;
			bit_file = malloc(len*sizeof(char));
			if(bit_file == NULL)
			{
				fprintf(stderr, "malloc failed while allocating"
					" for size : %d\n", len);
				ret = XVSEC_FAILURE;
				break;
			}
			snprintf(bit_file, len, "%s", optarg);

			args->program.abs_bit_file = realpath(bit_file, NULL);
			if(args->program.abs_bit_file == NULL)
			{
				fprintf(stderr, "Absolute Path conversion "
					"Failed for bit file with error : "
					"%d(%s)\n", errno, strerror(errno));
				ret = XVSEC_FAILURE;
				free(bit_file);
				bit_file = NULL;
				break;
			}
			free(bit_file);
			bit_file = NULL;
			args->program.flag = true;
			break;
		case 'C':
			if(args->cap_id != MCAP_CAP_ID)
			{
				args->parse_err = true;
				break;
			}

			len = strlen(optarg) + 1;
			clear_file = malloc(len*sizeof(char));
			if(clear_file == NULL)
			{
				fprintf(stderr, "malloc failed while "
					"allocating for size : %d\n", len);
				ret = XVSEC_FAILURE;
				break;
			}
			snprintf(clear_file, len, "%s", optarg);

			args->program.abs_clr_file = realpath(clear_file, NULL);
			if(args->program.abs_clr_file == NULL)
			{
				fprintf(stderr, "Absolute Path conversion "
					"Failed for clear file");
				ret = XVSEC_FAILURE;
				free(clear_file);
				clear_file = NULL;
				break;
			}
			free(clear_file);
			clear_file = NULL;
			args->program.flag = true;
			break;
		default:
			fprintf(stderr, "Invalid Option :%c\n", opt);
			args->help.flag = true;
			break;
		}
	}

	if((args->parse_err == true) || (ret != 0))
	{
		if(args->program.abs_clr_file != NULL)
		{
			free(args->program.abs_clr_file);
			args->program.abs_clr_file = NULL;
		}
		if(args->program.abs_bit_file != NULL)
		{
			free(args->program.abs_bit_file);
			args->program.abs_bit_file = NULL;
		}
	}

	return ret;
}

static int execute_xvsec_help_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	if(args->help.flag == false)
		return XVSEC_FAILURE;

	usage(stdout);

	return 0;
}

static int execute_xvsec_verbose_cmd(xvsec_handle_t *xvsec_handle,
	struct args *args)
{
	int ret = 0;

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
	for(index = 0; index < args->list_caps.cap_list.no_of_caps; index++)
	{
		fprintf(stdout, "Capability ID : 0x%04X, "
			"Capability Name : %s\n",
			args->list_caps.cap_list.cap_info[index].cap_id,
			args->list_caps.cap_list.cap_info[index].cap_name);
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
	int 			no_of_devices = 1;
	xvsec_handle_t		xvsec_handle;
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

	memset(&xvsec_handle, 0, sizeof(xvsec_handle));
	sts = xvsec_open(args.bus_no, args.dev_no, &xvsec_handle);
	if(sts < 0)
	{
		ret = sts;
		fprintf(stderr, "xvsec_open failed with error %d(%s) "
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
		for(mcap_op = MCAP_RESET;
			mcap_ops[mcap_op].execute != NULL; mcap_op++)
		{
			sts = mcap_ops[mcap_op].execute(&xvsec_handle, &args);
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
	sts = xvsec_close(&xvsec_handle);
	if(sts < 0)
	{
		ret = sts;
		fprintf(stderr, "xvsec_close failed with error %d(%s) "
			"for bus no : %d, device no : %d\n", ret,
			error_codes[-ret], args.bus_no, args.dev_no);
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
