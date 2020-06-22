/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#ifndef USER_CLI_CMD_PARSE_H_
#define USER_CLI_CMD_PARSE_H_

#include "qdma_nl.h"
#include "dmautils.h"

int parse_cmd(int argc, char *argv[], struct xcmd_info *xcmd);

#endif /* USER_CLI_CMD_PARSE_H_ */
