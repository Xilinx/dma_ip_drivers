/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2019,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef __LIBQDMA_VERSION_H__
#define __LIBQDMA_VERSION_H__

#define LIBQDMA_MODULE_NAME	"libqdma"
#define LIBQDMA_MODULE_DESC	"Xilinx QDMA Library"

#define LIBQDMA_VERSION_MAJOR	2019
#define LIBQDMA_VERSION_MINOR	2
#define LIBQDMA_VERSION_PATCH	213

#define LIBQDMA_VERSION_STR	\
	__stringify(LIBQDMA_VERSION_MAJOR) "." \
	__stringify(LIBQDMA_VERSION_MINOR) "." \
	__stringify(LIBQDMA_VERSION_PATCH)

#define LIBQDMA_VERSION  \
	((LIBQDMA_VERSION_MAJOR)*10000 + \
	 (LIBQDMA_VERSION_MINOR)*1000 + \
	  LIBQDMA_VERSION_PATCH)

#endif /* ifndef __LIBQDMA_VERSION_H__ */
