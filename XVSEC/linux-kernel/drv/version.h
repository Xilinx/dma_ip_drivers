/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018-2022,  Xilinx, Inc.
 * Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __XVSEC_VERSION_H__
#define __XVSEC_VERSION_H__

#define XVSEC_MODULE_NAME	"xvsec"
#define XVSEC_MODULE_DESC	"Xilinx VSEC Library"

#define XVSEC_VERSION_MAJOR	2026
#define XVSEC_VERSION_MINOR	1
#define XVSEC_VERSION_PATCH	0

#define XVSEC_DRV_VERSION	\
	__stringify(XVSEC_VERSION_MAJOR) "." \
	__stringify(XVSEC_VERSION_MINOR) "." \
	__stringify(XVSEC_VERSION_PATCH)

#define XVSEC_DRV_VERSION_NUM  \
	((XVSEC_VERSION_MAJOR)*1000 + \
	 (XVSEC_VERSION_MINOR)*100 + \
	  XVSEC_VERSION_PATCH)

#endif /* ifndef __XVSEC_VERSION_H__ */
