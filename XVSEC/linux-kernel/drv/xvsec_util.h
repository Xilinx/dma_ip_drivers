/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2020,  Xilinx, Inc.
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

#ifndef __XVSEC_UTIL_H__
#define __XVSEC_UTIL_H__

/**
 * @file xvsec_util.h
 *
 * Xilinx XVSEC Utility functions
 *
 * Header file *xvsec_util.h* defines the utility functions that can be
 * used by driver.
 *
 * These data structures & functions are for driver internal use
 *
 */

int xvsec_util_find_file_type(char *fname, const char *suffix);
struct file *xvsec_util_fopen(const char *path, int flags, int rights);
void xvsec_util_fclose(struct file *filep);
int xvsec_util_fread(struct file *filep, uint64_t offset,
	uint8_t *data, uint32_t size);
int xvsec_util_fwrite(struct file *filep, uint64_t offset,
	uint8_t *data, uint32_t size);
int xvsec_util_fsync(struct file *filep);
int xvsec_util_get_file_size(const char *fname, loff_t *size);

#endif /* __XVSEC_UTIL_H__  */
