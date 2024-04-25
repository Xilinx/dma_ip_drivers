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

#ifndef __XVSEC_PARSER_H__
#define __XVSEC_PARSER_H__

#define MODE_STR		"mode"
#define CACHE_STR		"axi_cache"
#define PROT_STR		"axi_prot"

#define MODE_STR_32B	"32b"
#define MODE_STR_128B	"128b"
#define MAX_FILE_LENGTH 300

#define MAX_NO_OF_P_ARGS_FOR_US		9
#define MAX_NO_OF_P_ARGS_FOR_VERSAL	14

/* main parser function */
int parse_arguments(int argc, char *argv[], struct args *args);

/* mcap related options parser function */
int parse_mcap_arguments(int argc, char *argv[], struct args *args);

/* access-mcap-regs Option */
int parse_opt_a_arguments(int argc, char *argv[], struct args *args);

/* access-fpga-regs */
int parse_opt_s_arguments(int argc, char *argv[], struct args *args);

/* program option */
int parse_opt_p_arguments(int argc, char *argv[], struct args *args);

/* Partial Reconfig Clear */
int parse_opt_C_arguments(int argc, char *argv[], struct args *args);

/* retrieve arguement */
int parse_opt_t_arguments(int argc, char *argv[], struct args *args);

/* versal axi-regs access */
int parse_opt_x_arguments(int argc, char *argv[], struct args *args);

/* sub function for program options for versal */
int parse_arguments_for_versal(int argc, char *argv[], struct args *args);

/* sub function for program options for ultrscale */
int parse_arguments_for_us(int argc, char *argv[], struct args *args);

#endif /* __XVSEC_PARSER_H__ */
