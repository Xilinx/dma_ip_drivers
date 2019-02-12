/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2018,  Xilinx, Inc.
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

#ifndef __XVSEC_CDEV_H__
#define __XVSEC_CDEV_H__

/**
 * @file
 * @brief This file contains the interface declarations of XVSEC driver
 *
 */

/**
 * XVSEC ioctl magic character
 */
#define XVSEC_IOC_MAGIC				'm'
/**
 * XVSEC char device first minor number
 */
#define XVSEC_MINOR_BASE			(0)
/**
 * XVSEC char device total minor numbers count
 */
#define XVSEC_MINOR_COUNT			(1)
/**
 * XILINX PCIe vendor ID
 */
#define XILINX_VENDOR_ID			(uint16_t)(0x10ee)
/**
 * XILINX PCIe vendor specific capability ID
 */
#define MCAP_EXT_CAP_ID				(uint16_t)(0x000B)
/**
 * Maximum Supported capabilities by the driver
 */
#define MAX_CAPABILITIES_SUPPORTED		10

/**
 * @enum - bitstream_program_status
 * @brief	program status(Success/Failure)
 */
enum bitstream_program_status {
	/** Programming Success Indication */
	MCAP_BITSTREAM_PROGRAM_SUCCESS = 0,
	/** Programming Failure Indication */
	MCAP_BITSTREAM_PROGRAM_FAILURE = 1
};

/**
 * @struct - mcap_regs
 * @brief	MCAP register set
 */
struct mcap_regs {
	/** Valid flag to indicate registers validity */
	uint32_t	valid;
	/** Extended capability header register */
	uint32_t	ext_cap_header;
	/** Vendor Specific header register */
	uint32_t	vendor_header;
	/** FPGA JTAG ID register */
	uint32_t	fpga_jtag_id;
	/** FPGA bit-stream version register */
	uint32_t	fpga_bit_ver;
	/** Status Register */
	uint32_t	status_reg;
	/** Control Register */
	uint32_t	control_reg;
	/** Write Data Register */
	uint32_t	wr_data_reg;
	/** Read Data Register: 4 data words */
	uint32_t	rd_data_reg[4];
};

/**
 * @struct - bitstream_file
 * @brief	MCAP bitstream parameters for programming
 */
struct bitstream_file {
	/** Partial clear bitstream file to program ultrascale devices */
	char *partial_clr_file;
	/** bitstream file to program */
	char *bitstream_file;
	/** Status of the bitstream programming */
	enum bitstream_program_status status;
};

/**
 * @struct - cfg_data
 * @brief	MCAP configuration parameters to perform read and writes
 */
struct cfg_data {
	/** access field. 'b' for byte access, 'h'for half word access,
	 *  'w' for word access
	 */
	char		access;
	/** VSEC address offset */
	uint16_t	offset;
	/** data field holds the information to write into the provided offset
	 *  for Write operation.
	 *  Holds the information at the provided offset for read operation
	 */
	uint32_t	data;
};

/**
 * @struct - fpga_cfg_reg
 * @brief	FPGA configuration parameters to perform read and writes
 */
struct fpga_cfg_reg {
	/** FPGA configuration register number */
	uint16_t	offset;
	/** data field holds the information to write into the provided offset
	 *  for Write operation.
	 *  Holds the information at the provided offset for read operation
	 */
	uint32_t	data;
};

/**
 * @struct - fpga_cfg_regs
 * @brief	FPGA configuration register set(See UG570 for more information)
 */
struct fpga_cfg_regs {
	/** Valid flag to indicate registers validity */
	uint32_t valid;
	/** CRC Register */
	uint32_t crc;
	/** Frame Address Register */
	uint32_t far;
	/** Frame Data Register, Input Register (write configuration data) */
	uint32_t fdri;
	/** Frame Data Register, Output Register (read configuration data) */
	uint32_t fdro;
	/** Command Register */
	uint32_t cmd;
	/** Control Register 0 */
	uint32_t ctl0;
	/** Mask Register for CTL0 and CTL1 Registers */
	uint32_t mask;
	/** Status Register */
	uint32_t stat;
	/** Legacy Output Register for Daisy Chain */
	uint32_t lout;
	/** Configuration Option Register 0 */
	uint32_t cor0;
	/** Multi Frame Write Register */
	uint32_t mfwr;
	/** Initial CBC Value Register */
	uint32_t cbc;
	/** Device ID Register */
	uint32_t idcode;
	/** User Access Register */
	uint32_t axss;
	/** Configuration Option Register 1 */
	uint32_t cor1;
	/** Warm Boot Start Address Register */
	uint32_t wbstar;
	/** Watchdog Timer Register */
	uint32_t timer;
	/** Scratch Pad Register for Dummy Read and Writes */
	uint32_t scratchpad;
	/** Boot History Status Register */
	uint32_t bootsts;
	/** Control Register 1 */
	uint32_t ctl1;
	/** BPI/SPI Configuration Options Register */
	uint32_t bspi;
};

/**
 * @struct - device_info
 * @brief	PCIe device information for verbose option
 */
struct device_info {
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

/**
 * @struct - xvsec_capabilities
 * @brief	Xilinx Vendor Specific Capabilities
 */
struct xvsec_capabilities {
	/** Number of VSEC capabilities supported by the device */
	uint16_t	no_of_caps;
	/** Capability ID Info */
	uint16_t	capability_id[MAX_CAPABILITIES_SUPPORTED];
	/** Capability Offset Info in PCIe configuration space */
	uint16_t	capability_offset[MAX_CAPABILITIES_SUPPORTED];
};

/**
 * ioctl code for retrieving the XVSEC capability list
 */
#define IOC_XVSEC_GET_CAP_LIST			\
	_IOW(XVSEC_IOC_MAGIC, 0, struct xvsec_capabilities *)
/**
 * ioctl code for performing MCAP configuration logic reset
 */
#define IOC_MCAP_RESET					\
	_IO(XVSEC_IOC_MAGIC, 1)
/**
 * ioctl code for performing MCAP Module reset
 */
#define IOC_MCAP_MODULE_RESET			\
	_IO(XVSEC_IOC_MAGIC, 2)
/**
 * ioctl code for performing both configuration logic & Module reset
 */
#define IOC_MCAP_FULL_RESET				\
	_IO(XVSEC_IOC_MAGIC, 3)
/**
 * ioctl code for retrieving the MCAP Read Data Registers
 */
#define IOC_MCAP_GET_DATA_REGISTERS		\
	_IOR(XVSEC_IOC_MAGIC, 4, uint32_t *)
/**
 * ioctl code for retrieving the MCAP Registers
 */
#define IOC_MCAP_GET_REGISTERS			\
	_IOR(XVSEC_IOC_MAGIC, 5, struct mcap_regs *)
/**
 * ioctl code for retrieving the FPGA configuration Registers
 */
#define IOC_MCAP_GET_FPGA_REGISTERS			\
	_IOR(XVSEC_IOC_MAGIC, 6, struct mcap_regs *)
/**
 * ioctl code for programming the bitstream
 */
#define IOC_MCAP_PROGRAM_BITSTREAM		\
	_IOWR(XVSEC_IOC_MAGIC, 7, struct bitstream_file *)
/**
 * ioctl code for reading an MCAP VSEC register
 */
#define IOC_MCAP_READ_DEV_CFG_REG		\
	_IOWR(XVSEC_IOC_MAGIC, 8, struct cfg_data *)
/**
 * ioctl code for Writing to an MCAP VSEC register
 */
#define IOC_MCAP_WRITE_DEV_CFG_REG	\
	_IOWR(XVSEC_IOC_MAGIC, 9, struct cfg_data *)
/**
 * ioctl code for reading an FPGA CFG register
 */
#define IOC_MCAP_READ_FPGA_CFG_REG \
	_IOWR(XVSEC_IOC_MAGIC, 10, struct fpga_cfg_reg *)
/**
 * ioctl code for writing to an FPGA CFG register
 */
#define IOC_MCAP_WRITE_FPGA_CFG_REG \
	_IOWR(XVSEC_IOC_MAGIC, 11, struct fpga_cfg_reg *)
/**
 * ioctl code for retrieving the Device information
 */
#define IOC_GET_DEVICE_INFO \
	_IOWR(XVSEC_IOC_MAGIC, 12, struct device_info *)


#endif /* __XVSEC_CDEV_H__ */
