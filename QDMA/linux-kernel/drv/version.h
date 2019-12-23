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

#ifndef __QDMA_VERSION_H__
#define __QDMA_VERSION_H__

#include "libqdma/version.h"

#ifdef __QDMA_VF__
#define DRV_MODULE_NAME		"qdma_vf"
#define DRV_MODULE_DESC		"Xilinx QDMA VF Reference Driver"
#else
#define DRV_MODULE_NAME		"qdma_pf"
#define DRV_MODULE_DESC		"Xilinx QDMA PF Reference Driver"
#endif /* #ifdef __QDMA_VF__ */
#define DRV_MODULE_RELDATE	"Nov 2019"

#define DRV_MOD_MAJOR		2019
#define DRV_MOD_MINOR		2
#define DRV_MOD_PATCHLEVEL	125

#define DRV_MODULE_VERSION      \
	__stringify(DRV_MOD_MAJOR) "." \
	__stringify(DRV_MOD_MINOR) "." \
	__stringify(DRV_MOD_PATCHLEVEL) "." \
	__stringify(LIBQDMA_VERSION_PATCH) "." \

#define DRV_MOD_VERSION_NUMBER  \
	((DRV_MOD_MAJOR)*10000 + (DRV_MOD_MINOR)*1000 + DRV_MOD_PATCHLEVEL)

#endif /* ifndef __QDMA_VERSION_H__ */
