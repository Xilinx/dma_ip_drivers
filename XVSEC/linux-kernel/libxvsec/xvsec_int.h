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


#ifndef __XVSEC_INT_H__
#define __XVSEC_INT_H__

#define MAX_MCAP_REG_OFFSET	(0x2C)
#define MAX_MCAPV2_REG_OFFSET	(0x1C)

/* Internal APIs and structures */
typedef struct handle_t
{
	uint16_t	xvsec_magic_no;	/* XVSEC Magic Number */
	uint8_t		bus_no;		/* PCI bus number */
	uint8_t		dev_no;		/* Device Number on PCI bus */
	uint16_t	index;		/* Array Index of handle info */
	uint8_t		mrev;		/* mcap rev*/
	bool		valid;		/* Validity of the handle */
}handle_t;

typedef struct xvsec_user_context_t
{
	xvsec_handle_t	*handle;
	int		fd;
	pthread_mutex_t	mutex;
}xvsec_user_context_t;


extern int no_of_devs;
extern xvsec_user_context_t    *xvsec_user_ctx;

extern int xvsec_validate_handle(xvsec_handle_t *handle);

#endif /* __XVSEC_INT_H__ */
