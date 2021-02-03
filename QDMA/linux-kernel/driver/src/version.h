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

#ifndef __QDMA_VERSION_H__
#define __QDMA_VERSION_H__

#include "libqdma/version.h"

#ifdef __QDMA_VF__
#define DRV_MODULE_NAME		"qdma-vf"
#define DRV_MODULE_DESC		"Xilinx QDMA VF Reference Driver"
#else
#define DRV_MODULE_NAME		"qdma-pf"
#define DRV_MODULE_DESC		"Xilinx QDMA PF Reference Driver"
#endif /* #ifdef __QDMA_VF__ */
#define DRV_MODULE_RELDATE	"Dec 2020"

#define DRV_MOD_MAJOR		2020
#define DRV_MOD_MINOR		2
#define DRV_MOD_PATCHLEVEL	1

#define DRV_MODULE_VERSION      \
	__stringify(DRV_MOD_MAJOR) "." \
	__stringify(DRV_MOD_MINOR) "." \
	__stringify(DRV_MOD_PATCHLEVEL) "." \
	__stringify(LIBQDMA_VERSION_PATCH) "." \

#define DRV_MOD_VERSION_NUMBER  \
	((DRV_MOD_MAJOR)*10000 + (DRV_MOD_MINOR)*1000 + DRV_MOD_PATCHLEVEL)

#endif /* ifndef __QDMA_VERSION_H__ */
