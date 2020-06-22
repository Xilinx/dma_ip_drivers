/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2019 - 2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#ifndef _QDMACTL_INTERNAL_H_
#define _QDMACTL_INTERNAL_H_

#include <stdint.h>

#include "dmautils.h"

int proc_reg_cmd(struct xcmd_info *xcmd);
int is_valid_addr(unsigned char bar_no, unsigned int reg_addr);
int xnl_common_msg_send(struct xcmd_info *cmd, uint32_t *attrs);

#endif /* _QDMACTL_INTERNAL_H_ */
