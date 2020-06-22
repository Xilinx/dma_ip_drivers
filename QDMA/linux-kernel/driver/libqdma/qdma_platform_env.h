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
#ifndef LIBQDMA_QDMA_PLATFORM_ENV_H_
#define LIBQDMA_QDMA_PLATFORM_ENV_H_

#include <linux/types.h>
#include <linux/kernel.h>

#define QDMA_SNPRINTF_S(arg1, arg2, arg3, ...) \
		snprintf(arg1, arg3, ##__VA_ARGS__)

#define qdma_log_info(x_, ...) pr_info(x_, ##__VA_ARGS__)
#define qdma_log_warning(x_, ...) pr_warn(x_, ##__VA_ARGS__)
#define qdma_log_error(x_, ...) pr_err(x_, ##__VA_ARGS__)
#define qdma_log_debug(x_, ...) pr_debug(x_, ##__VA_ARGS__)

#endif /* LIBQDMA_QDMA_PLATFORM_ENV_H_ */
