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

#ifndef __XVSEC_DRV_H__
#define __XVSEC_DRV_H__

/**
 * @file xvsec_drv.h
 *
 * Xilinx XVSEC Driver Library Interface Definitions
 *
 * Header file *xvsec_drv.h* defines data structures and ioctl codes
 * exported by Xilinx XVSEC driver for common VSEC operations.
 *
 * These data structures and ioctl codes can be used by user space
 * applications to carry-out the XVSEC functionality
 */

/** @defgroup xvsec_enums Enumerations
 */
/** @defgroup xvsec_struct Data Structures
 */
/** @defgroup xvsec_union Data Structures
 */
/**
 * @defgroup xvsec_defines Definitions
 * @{
 */

/**
 * XVSEC ioctl magic character
 */
#define XVSEC_IOC_MAGIC		'v'

/**
 * Maximum Supported capabilities by the driver
 */
#define MAX_CAPABILITIES_SUPPORTED		10

/**
 * Maximum Vsec string length
 */
#define MAX_VSEC_STR_LEN		20

/**
 * Unknown VSEC Revision ID
 */
#define VSEC_REV_UNKNOWN		(0xFF)

/** @} */

/**
 * @struct - device_info
 * @brief	PCIe device information for verbose option
 *
 * @ingroup xvsec_union
 */
union device_info {
	struct {
		/** PCIe Vendor Identifier */
		uint16_t vendor_id;
		/** PCIe Device Identifier */
		uint16_t device_id;
		/** PCIe Device number */
		uint16_t device_no;
		/** PCIe Device function */
		uint16_t device_fn;
		/** PCIe Subsystem Vendor Identifier */
		uint16_t subsystem_vendor;
		/** PCIe Subsystem Device Identifier */
		uint16_t subsystem_device;
		/** PCIe Class Identifier */
		uint16_t class_id;
		/** Flag which indicates MSI enabled status */
		uint32_t is_msi_enabled;
		/** Flag which indicates MSIx enabled status */
		uint32_t is_msix_enabled;
		/** Size of the PCIe Device configuration space */
		int      cfg_size;
	};
};

/**
 * @struct - xvsec_vsec_info
 * @brief       Xilinx Vendor Specific Capabilities device information
 *
 * @ingroup xvsec_struct
 */
struct xvsec_vsec_info {
	/** Capability ID Info */
	uint16_t	cap_id;
	/** VSEC revision Info */
	uint16_t	rev_id;
	/** Capability Offset Info in PCIe configuration space */
	uint16_t	offset;
	/** VSEC Capability name */
	char	name[MAX_VSEC_STR_LEN];
	/** info to check capability supported by this drv or not*/
	bool	is_supported;
};

/**
 * @struct - xvsec_capabilities
 * @brief	Xilinx Vendor Specific Capabilities present in the device
 *
 * @ingroup xvsec_struct
 */
struct xvsec_capabilities {
	/** Number of VSEC capabilities supported by the device */
	uint16_t	no_of_caps;
	/** Vsec inforamtion */
	struct xvsec_vsec_info vsec_info[MAX_CAPABILITIES_SUPPORTED];
};

/**
 * Capability list operation code to use in ioctls
 */
#define CODE_XVSEC_GET_CAP_LIST			0

/**
 * Device Info operation code to use in ioctls
 */
#define CODE_XVSEC_GET_DEV_INFO			1

/**
 * ioctl code for retrieving the XVSEC capability list
 */
#define IOC_XVSEC_GET_CAP_LIST \
	_IOW(XVSEC_IOC_MAGIC, CODE_XVSEC_GET_CAP_LIST, \
	struct xvsec_capabilities *)

/**
 * ioctl code for retrieving the Device information
 */
#define IOC_XVSEC_GET_DEVICE_INFO \
	_IOWR(XVSEC_IOC_MAGIC, CODE_XVSEC_GET_DEV_INFO, \
	union device_info *)

#endif /* __XVSEC_DRV_H__ */
