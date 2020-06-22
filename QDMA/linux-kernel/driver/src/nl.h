/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
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

#ifndef __QDMA_DRV_NL_H__
#define __QDMA_DRV_NL_H__
/**
 * @file
 * @brief This file contains the declarations for qdma netlink helper
 * funnctions kernel module
 *
 */
#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <net/genetlink.h>

/*****************************************************************************/
/**
 * xnl_respond_buffer() - send a netlink string message
 *
 * @param[in]	nl_info:	pointer to netlink genl_info
 * @param[in]	buf:		string buffer
 * @param[in]	buflen:		length of the string buffer
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xnl_respond_buffer(struct genl_info *info, char *buf, int buflen,
		int error);

int xlnx_nl_init(void);
void  xlnx_nl_exit(void);

#endif /* ifndef __QDMA_DRV_NL_H__ */
