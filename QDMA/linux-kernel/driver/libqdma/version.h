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

#ifndef __LIBQDMA_VERSION_H__
#define __LIBQDMA_VERSION_H__

#define LIBQDMA_MODULE_NAME	"libqdma"
#define LIBQDMA_MODULE_DESC	"Xilinx QDMA Library"

#define LIBQDMA_VERSION_MAJOR	2020
#define LIBQDMA_VERSION_MINOR	2
#define LIBQDMA_VERSION_PATCH	1

#define LIBQDMA_VERSION_STR	\
	__stringify(LIBQDMA_VERSION_MAJOR) "." \
	__stringify(LIBQDMA_VERSION_MINOR) "." \
	__stringify(LIBQDMA_VERSION_PATCH)

#define LIBQDMA_VERSION  \
	((LIBQDMA_VERSION_MAJOR)*10000 + \
	 (LIBQDMA_VERSION_MINOR)*1000 + \
	  LIBQDMA_VERSION_PATCH)

#endif /* ifndef __LIBQDMA_VERSION_H__ */
