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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "xvsec.h"
#include "main.h"
#include "mcap_ops.h"
#include "xvsec_parser.h"
#include <libgen.h>
#include <string.h>

static const char options[] =	":b:F:c:lp:C:rmfdvHhDoa:s:t:x:q:";

/*
 * this is common for US/US+ and Versal devices.
 * access-mcap-regs Option
 */
int parse_opt_a_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;

	if(argv[optind] == NULL)
	{
		fprintf(stderr, "Access option "
			"must be provided. Valid options are"
			" \'b/h/w\'\n");
		args->parse_err = true;
		goto CLEANUP;
	}
	if(argv[optind][1] != '\0')
	{
		fprintf(stderr, "Invalid Access option "
			"provided %s (\'b/h/w\' are valid)\n",
			argv[optind]);
		args->parse_err = true;
		goto CLEANUP;
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
		goto CLEANUP;
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

CLEANUP:	
	return ret;	
}

/* Access FPGA Config*/
int parse_opt_s_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;

	if (args->rev_id.mrev == XVSEC_MCAP_VERSAL) {
		fprintf(stderr, "Access FPGA Config option is not "
				"supported for Versal devices\n");
		args->parse_err = true;
		goto CLEANUP;
	}
	args->fpga_access_reg.offset =
		(uint16_t) strtoul(optarg, NULL, 0);

	if(argv[optind] == NULL)
	{
		args->fpga_access_reg.write = false;
		args->fpga_access_reg.cmd = '\0';
		args->fpga_access_reg.data = 0x0;
		args->fpga_access_reg.flag = true;
		goto CLEANUP;
	}
	if(argv[optind + 1] == NULL)
	{
		args->parse_err = true;
		goto CLEANUP;
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
		goto CLEANUP;
	}
	printf("In -s option\n");
	args->fpga_access_reg.flag = true;

CLEANUP:	
	return ret;	
}

/* program option */
int parse_opt_p_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	if(args->rev_id.mrev == XVSEC_MCAP_VERSAL)
	{
		ret = parse_arguments_for_versal(argc, argv, args);
	}
	else
	{
		ret = parse_arguments_for_us(argc, argv, args);
	}

	return ret;
}

int parse_arguments_for_versal(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	uint16_t str_len;
	char *file = NULL;

	if (argc < MAX_NO_OF_P_ARGS_FOR_VERSAL) {
		fprintf(stderr, "Invalid number of arguments passed "
				"for -p option: %d\n"
				"Please enter valid arguements!\n", argc);
		return XVSEC_FAILURE;
	}

	if((optarg == NULL) || (argv[optind] == NULL)) {
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}
	args->download.flag = false;

	if (strncmp(optarg, "mode", sizeof("mode")) == 0) {
		if (strncmp(argv[optind], "32b", sizeof("32b")) == 0) {
			args->download.is_128b_mode = false;
		}
		else if (strncmp(argv[optind], "128b", sizeof("128b")) == 0) {
			args->download.is_128b_mode = true;
		}
		else {
			fprintf(stderr, "Invalid Mode provided, "
					"Only 32b and 128b are valid\n");
			ret = XVSEC_FAILURE;
			goto CLEANUP;
		}
	}

	if((argv[optind + 1] == NULL) || (argv[optind + 2] == NULL)) {
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	if (strncmp(argv[optind + 1], "type", sizeof("type")) == 0) {
		if (strncmp(argv[optind + 2], "incr", sizeof("incr")) == 0) {
			args->download.is_fixed_addr = false;
		}
		else if (strncmp(argv[optind + 2], "fixed", sizeof("fixed")) == 0) {
			args->download.is_fixed_addr = true;
		}
		else {
			fprintf(stderr, "Invalid Address type provided, "
					"Only fixed and incr are valid\n");
			ret = XVSEC_FAILURE;
			goto CLEANUP;
		}
	}

	if(argv[optind + 3] == NULL) {
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	args->download.dev_addr = (uint32_t) strtoul(argv[optind + 3], NULL, 0);

	str_len = strlen(argv[optind + 4]) + 1;
	file = malloc(str_len * sizeof(char));
	if(file == NULL)
	{
		fprintf(stderr, "malloc failed while allocating"
			" for size : %d\n", str_len);
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	snprintf(file, str_len, "%s", argv[optind + 4]);

	args->download.file_name = realpath(file, NULL);
	if(args->download.file_name == NULL)
	{
		fprintf(stderr, "Absolute Path conversion "
			"Failed for file with error : "
			"%d(%s)\n", errno, strerror(errno));
		ret = XVSEC_FAILURE;
		free(file);
		goto CLEANUP;
	}

	/*optional transfer mode parameters*/
	if((argv[optind + 5] == NULL)) {
		args->download.tr_mode = XVSEC_MCAP_DATA_TR_MODE_FAST;
		goto EXIT;
	}

	if (strncmp(argv[optind + 5], "tr_mode", sizeof("tr_mode")) == 0) {

		if(argv[optind + 6] == NULL) {
			fprintf(stderr, "Please enter valid transfer mode, "
					"Only slow and fast are valid\n");
			ret = XVSEC_FAILURE;
			goto CLEANUP;
		}

		if (strncmp(argv[optind + 6], "slow", sizeof("slow")) == 0) {
			/*slow download mode*/
			args->download.tr_mode = XVSEC_MCAP_DATA_TR_MODE_SLOW;
		}
		else if (strncmp(argv[optind + 6], "fast", sizeof("fast")) == 0) {
			/*fast download mode*/
			args->download.tr_mode = XVSEC_MCAP_DATA_TR_MODE_FAST;
		}
		else {
			fprintf(stderr, "Invalid transfer mode provided, "
					"Only slow and fast are valid\n");
			ret = XVSEC_FAILURE;
			goto CLEANUP;
		}
		fprintf(stdout, "tr_mode: %d\n", args->download.tr_mode);
	}

EXIT:
	free(file);
	args->download.flag = true;

CLEANUP:
	return ret;
}

/* program option */
int parse_arguments_for_us(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	uint16_t len;
	char *bit_file = NULL;

	if (argc > MAX_NO_OF_P_ARGS_FOR_US) {
		fprintf(stderr, "Invalid number of arguments passed "
				"for -p option: %d\n"
				"Please enter valid arguements!\n", argc);
		return XVSEC_FAILURE;
	}

	len = strlen(optarg) + 1;
	bit_file = malloc(len*sizeof(char));
	if(bit_file == NULL)
	{
		fprintf(stderr, "malloc failed while allocating"
			" for size : %d\n", len);
		ret = XVSEC_FAILURE;
		goto CLEANUP;
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
		goto CLEANUP;
	}
	free(bit_file);
	args->program.flag = true;

CLEANUP:
	return ret;
}

/* Partial Reconfig Clear */
int parse_opt_C_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	uint16_t len;
	char *clear_file = NULL;

	len = strlen(optarg) + 1;
	clear_file = malloc(len*sizeof(char));
	if(clear_file == NULL)
	{
		fprintf(stderr, "malloc failed while "
			"allocating for size : %d\n", len);
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}
	snprintf(clear_file, len, "%s", optarg);

	args->program.abs_clr_file = realpath(clear_file, NULL);
	if(args->program.abs_clr_file == NULL)
	{
		fprintf(stderr, "Absolute Path conversion "
			"Failed for clear file");
		ret = XVSEC_FAILURE;
		free(clear_file);
		goto CLEANUP;
	}
	free(clear_file);
	args->program.flag = true;

CLEANUP:
	return ret;
}

bool get_realpath(char* path, char *rpath)
{
	bool ret = false;
	struct stat stats;
	char* dirptr = NULL;
	char* dirptr_abs_path = NULL;
	char* fileptr = NULL;
	char* dir = NULL;
	char* filename = NULL;
	int len = 0;

	if((path == NULL) || (rpath == NULL))
		return false;

	dirptr = strdup(path);
	fileptr = strdup(path);

	if((dirptr == NULL) || (fileptr == NULL))
	{
		if(dirptr != NULL)
			free(dirptr);

		if(fileptr != NULL)
			free(fileptr);

		return false;
	}

	dir = dirname(dirptr);
	filename = basename(fileptr);

	if((dir == NULL) || (filename == NULL))
	{
		if(dir != NULL)
			free(dir);

		if(filename != NULL)
			free(filename);

		ret = false;
		goto CLEANUP;
	}

	(void)stat(dir, &stats);
	len = 0;

	dirptr_abs_path = realpath(dirptr, NULL);
	if ( S_ISDIR(stats.st_mode) & (dirptr_abs_path != NULL))
	{
		len = strlen(dirptr_abs_path)+1;
		if( (len + strlen(filename) + 1) >= MAX_FILE_LENGTH)
		{
			fprintf(stderr, "File Name is too long\n");
			goto CLEANUP_ABS_PATH;
		}

		snprintf(rpath, len, "%s", dirptr_abs_path);
		strcat(rpath, "/");
		strcat(rpath, filename);
		ret = true;
	}

CLEANUP_ABS_PATH:
	free(dirptr_abs_path);

CLEANUP:
	free(dirptr);
	free(fileptr);
	return ret;
}


/*
 * retrieve arguement 
 *
 */
int parse_opt_t_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	uint16_t str_len;
	char *file = NULL;
	char rpath[MAX_FILE_LENGTH];

	if(args->rev_id.mrev != XVSEC_MCAP_VERSAL)
	{
		fprintf(stderr, "AXI --retrieve option is only "
				"supported for Versal devices\n");
		return XVSEC_ERR_OPERATION_NOT_SUPPORTED;
	}

	if((argv[optind] == NULL) || (argv[optind + 1] == NULL)) {
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	if (strncmp(optarg, "type", sizeof("type")) == 0) {
		if (strncmp(argv[optind], "incr", sizeof("incr")) == 0) {
			args->upload.is_fixed_addr = false;
		}
		else if (strncmp(argv[optind], "fixed", sizeof("fixed")) == 0) {
			args->upload.is_fixed_addr = true;
		}
		else {
			fprintf(stderr, "Invalid Address type provided, "
					"Only fixed and incr are valid\n");
			ret = XVSEC_FAILURE;
			goto CLEANUP;
		}
	}

	if(argv[optind+1] == NULL) {
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	args->upload.dev_addr = (uint32_t) strtoul(argv[optind+1], NULL, 0);
	args->upload.length = (size_t) strtoul(argv[optind+2], NULL, 0);

	str_len = strlen(argv[optind+3]) + 1;
	file = malloc(str_len * sizeof(char));
	if(file == NULL)
	{
		fprintf(stderr, "malloc failed while allocating"
			" for size : %d\n", str_len);
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	snprintf(file, str_len, "%s", argv[optind+3]);

	args->upload.file_name = NULL;
	memset(rpath, 0, MAX_FILE_LENGTH);
	if(get_realpath(file, rpath) == true)
	{
		args->upload.file_name = rpath;
	}

	if(args->upload.file_name == NULL)
	{
		fprintf(stderr, "Absolute Path conversion Failed"
			" for file: %s\n", file);
		ret = XVSEC_FAILURE;
		free(file);
		goto CLEANUP;
	}
	free(file);
	args->upload.flag = true;

CLEANUP:
	return ret;
}

/*
 * Access the AXI Registers
 * -x, --access-axi-regs [mode <32b/128b>] <address> [w [data]]  
 * 
 */
int parse_opt_x_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	bool write_mode = false;
	args->access_axi_reg.flag = false;

	if(args->rev_id.mrev != XVSEC_MCAP_VERSAL)
	{
		printf(" --access-axi-regs option is only supported for Versal devices\n");
		ret = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}

	args->access_axi_reg.write = false;

	if(optarg != NULL)
	{
		if( strncmp(optarg, MODE_STR, sizeof(MODE_STR)) == 0)
		{
			if(argv[optind] != NULL)
			{
				if(strncmp(argv[optind], MODE_STR_32B, sizeof(MODE_STR_32B)) == 0)
				{
					args->access_axi_reg.mode = XVSEC_MCAP_AXI_MODE_32B;
				}
				else if(strncmp(argv[optind], MODE_STR_128B, sizeof(MODE_STR_128B)) == 0)
				{
					args->access_axi_reg.mode = XVSEC_MCAP_AXI_MODE_128B;
				}
				else
				{
					printf("Invalid mode parameters.\n");
					ret = XVSEC_ERR_INVALID_PARAM;
					goto CLEANUP;
				}
				write_mode = true;
			}
			else
			{
				printf("Insufficient args for --access-axi-regs. Please enter valid mode parameters.\n");
				ret = XVSEC_ERR_INVALID_PARAM;
				goto CLEANUP;
			}
		}
		else
		{
			/*if mode is not given then it must be read operation*/
			if(optarg != NULL)
			{
				args->access_axi_reg.address = (int)strtol(optarg, NULL, 0);
				args->access_axi_reg.mode = XVSEC_MCAP_AXI_MODE_32B;
				goto READ_EXIT;
			}
			else
			{
				printf("Insufficient args for --access-axi-regs\n");
				ret = XVSEC_ERR_INVALID_PARAM;
				goto CLEANUP;
			}

		}
	}
	if(argv[optind + 1] == NULL)
	{
		printf("Insufficient args for --access-axi-regs\n");
		ret = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}

	if(argv[optind + 1] != NULL)
	{
		args->access_axi_reg.address = (int)strtol(argv[optind+1], NULL, 0);
	}
	else
	{
		printf("Error in address\n");
		ret = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}

	/* this is optional */
	if(argv[optind + 2] != NULL)
	{
		if( strcmp(argv[optind + 2], "w") == 0)
		{
			if(argv[optind + 3] != NULL)
			{
				args->access_axi_reg.data[0] = (int)strtol(argv[optind+3], NULL, 0);
				if(args->access_axi_reg.mode == XVSEC_MCAP_AXI_MODE_128B) /*expect 3 more args*/
				{
					 if((argv[optind + 4] != NULL) && (argv[optind + 5] != NULL) && (argv[optind + 6] != NULL))
					 {
						 args->access_axi_reg.data[1] = (int)strtol(argv[optind+4], NULL, 0);
						 args->access_axi_reg.data[2] = (int)strtol(argv[optind+5], NULL, 0);
						 args->access_axi_reg.data[3] = (int)strtol(argv[optind+6], NULL, 0);
					 }
					 else
					 {
						 printf("Insufficient data to write. Please provide valid 128bit data to write\n");
						 ret = XVSEC_ERR_INVALID_PARAM;
						 goto CLEANUP;
					 }

				}
				args->access_axi_reg.write = true;
			}
			else
			{
				printf("Insufficient args. Please provide valid data to write\n");
				ret = XVSEC_ERR_INVALID_PARAM;
				goto CLEANUP;
			}
		}
	}

	/*error check for mode and write parameters*/
	if((write_mode == true) && (args->access_axi_reg.write != true))
	{
		printf("Insufficient args for write mode\n");
		ret = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}
READ_EXIT:
	args->access_axi_reg.flag = true;

CLEANUP:
	return ret;
}


/*
 *Versal MCAP Cache and Protection settings
 *
 **/
int parse_opt_q_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	int axi_cache = 0;
	int axi_prot = 0;

	args->axi_cache_settings.flag = false;

	if(args->rev_id.mrev != XVSEC_MCAP_VERSAL)
	{
		printf(" --access-axi-regs option is only supported for Versal devices\n");
		ret = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}

	if((optarg != NULL) && (argv[optind] != NULL))
	{
		if( strncmp(optarg, CACHE_STR, sizeof(CACHE_STR)) == 0)
		{
			if(argv[optind] != NULL)
			{
				axi_cache = (int)strtol(argv[optind], NULL, 0);
				printf("axi_cache: 0x%X\n", axi_cache);
				if( axi_cache > 0xF )
				{
					printf("please enter axi_cache in range 0-15\n");
					ret = XVSEC_ERR_INVALID_PARAM;
					goto CLEANUP;
				}
			}
		}
		else
		{
			printf("Invalid args for --axi-cache-settings.\n");
			ret = XVSEC_ERR_INVALID_PARAM;
			goto CLEANUP;
		}

		if((argv[optind + 1] == NULL) || (argv[optind + 2] == NULL))
		{
			printf("Insufficient args for --axi-cache-settings.\n");
			ret = XVSEC_ERR_INVALID_PARAM;
			goto CLEANUP;
		}

		if( strncmp(argv[optind + 1], PROT_STR, sizeof(PROT_STR)) == 0)
		{
			if(argv[optind + 2] != NULL)
			{
				axi_prot = (int)strtol(argv[optind + 2], NULL, 0);
				printf("axi_prot: 0x%X\n", axi_prot);
				if( axi_prot > 0x7 )
				{
					printf("please enter axi_prot in range 0-7\n");
					ret = XVSEC_ERR_INVALID_PARAM;
					goto CLEANUP;
				}
			}
		}
		else
		{
			printf("Invalid args for --axi-cache-settings.\n");
			ret = XVSEC_ERR_INVALID_PARAM;
			goto CLEANUP;
		}

		args->axi_cache_settings.attr.v2.axi_cache = axi_cache;
		args->axi_cache_settings.attr.v2.axi_prot = axi_prot;
		args->axi_cache_settings.flag = true;
	}

CLEANUP:
	return ret;
}

/*
 *
 * mcap parser function for the arguements
 * dependent on the MCAP version
 *
 */

int parse_mcap_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	int opt;

	optind = 1; /* to reset the getopt*/

	if(args == NULL) {
		return XVSEC_FAILURE;
	}

	if(args->cap_id != MCAP_CAP_ID)
	{
		args->parse_err = true;
		goto CLEANUP;
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
			case 'a':
				ret = parse_opt_a_arguments(argc, argv, args);
				break;
			case 's':
				ret = parse_opt_s_arguments(argc, argv, args);
				break;
			case 'p':
				ret = parse_opt_p_arguments(argc, argv, args);
				break;
			case 't':
				ret = parse_opt_t_arguments(argc, argv, args);
				break;
			case 'x':
				ret = parse_opt_x_arguments(argc, argv, args);
				break;
			case 'C':
				ret = parse_opt_C_arguments(argc, argv, args);
				break;
			case 'q':
				ret = parse_opt_q_arguments(argc, argv, args);
				break;
			/*
			 * options already parsed
			 * in func parse_arguments()
			 * */
			case 'b':
			case 'F':
			case 'c':
			case 'h':
			case 'H':
			case 'v':
			case 'l':
			case 'r':
			case 'm':
			case 'f':
			case 'd':
			case 'D':
			case 'o':
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

CLEANUP:	
	return ret;
}

int parse_arguments(int argc, char *argv[], struct args *args)
{
	int ret = 0;
	int opt;

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
				/*MCAP specific args needs version info
				 * parsed in func parse_mcap_arguments*/
			case 'a':
			case 's':
			case 'p':
			case 't':
			case 'x':
			case 'C':
			case 'q':
				break;
			default:
				fprintf(stderr, "Invalid Option :%c\n", opt);
				args->help.flag = true;
				break;
		}
	}

	return ret;
}

