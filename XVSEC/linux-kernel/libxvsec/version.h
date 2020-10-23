/*
 * This file is part of the XVSEC userspace library which provides the
 * userspace APIs to enable the XSEC driver functionality
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#ifndef __XVSEC_LIB_VERSION_H__
#define __XVSEC_LIB_VERSION_H__

#define __stringify1(x...)	#x
#define __stringify(x...)	__stringify1(x)

#define XVSEC_LIB_MODULE_NAME	"xvsec"
#define XVSEC_LIB_MODULE_DESC	"Xilinx VSEC Library"

#define XVSEC_LIB_VERSION_MAJOR	2020
#define XVSEC_LIB_VERSION_MINOR	2
#define XVSEC_LIB_VERSION_PATCH	0

#define XVSEC_LIB_VERSION	\
	__stringify(XVSEC_LIB_VERSION_MAJOR) "." \
	__stringify(XVSEC_LIB_VERSION_MINOR) "." \
	__stringify(XVSEC_LIB_VERSION_PATCH)

#define XVSEC_LIB_VERSION_NUM  \
	((XVSEC_LIB_VERSION_MAJOR)*1000 + \
	 (XVSEC_LIB_VERSION_MINOR)*100 + \
	  XVSEC_LIB_VERSION_PATCH)

#endif /* ifndef __XVSEC_LIB_VERSION_H__ */
