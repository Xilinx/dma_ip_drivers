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

#ifndef __XVSEC_MCAP_H__
#define __XVSEC_MCAP_H__

/**
 * @file xvsec_mcap.h
 *
 * Xilinx XVSEC MCAP Library Interface Definitions
 *
 * Header file *xvsec_mcap.h* defines data structures and ioctl codes
 * exported by Xilinx XVSEC driver for MCAP functionality.
 *
 * These data structures and ioctl codes can be used by user space
 * applications to carryout the XVSEC-MCAP functionality
 *
 * XVSEC-MCAP IP has two versions V1 & V2.
 *     - Version 1 implementation of MCAP is targeted for Xilinx US/US+ devices
 *     - Version 2 implementation of MCAP is targeted for Xilinx Versal devices
 */

/** @defgroup xvsec_mcap_enums Enumerations
 */
/** @defgroup xvsec_mcap_union Data Structures
 */

/**
 * @defgroup xvsec_defines Definitions
 * @{
 */

/**
 * XVSEC-MCAP ioctl magic character
 */
#define XVSEC_MCAP_IOC_MAGIC		'm'

/** @} */

/**
 * @enum - bitstream_program_status
 * @brief	Enumeration for XVSEC-MCAP bitstream program Result
 *
 * @ingroup xvsec_mcap_enums
 */
enum bitstream_program_status {
	/** Programming Success Indication */
	MCAP_BITSTREAM_PROGRAM_SUCCESS = 0,
	/** Programming Failure Indication */
	MCAP_BITSTREAM_PROGRAM_FAILURE = 1
};

/**
 * @enum - axi_access_mode
 * @brief	AXI Address Access mode used to access
 *		AXI sub-devices via XVSEC-MCAP interface
 *
 * @ingroup xvsec_mcap_enums
 */
enum axi_access_mode {
	/** 32-bit AXI address mode */
	MCAP_AXI_MODE_32B = 0,
	/** 128-bit AXI address mode */
	MCAP_AXI_MODE_128B,
};

/**
 * @enum - axi_address_type
 * @brief	Indicates the AXI device access type.
 *
 *		FIFO type devices support fixed mode where the address is fixed.
 *
 *		Other types of device support increment address type
 *		where the address to be incremented for every transaction.
 *
 * @ingroup xvsec_mcap_enums
 */
enum axi_address_type {
	/** Fixed address type for FIFO type devices */
	FIXED_ADDRESS = 0,
	/** Increment address type for other devices */
	INCREMENT_ADDRESS
};

/**
 * @enum - data_transfer_mode
 * @brief	Indicates mode of data transfer for
 *		download functionality.
 *
 * @ingroup xvsec_mcap_enums
 **/
enum data_transfer_mode {
	/** Fast Transfer mode
	 *  and the default one when not specified
	 */
	DATA_TRANSFER_MODE_FAST = 0,
	/** Slow Transfer mode */
	DATA_TRANSFER_MODE_SLOW
};

/**
 * @enum - file_operation_status
 * @brief	Indicates File operation(Download/Upload) result
 *
 * @ingroup xvsec_mcap_enums
 **/
enum file_operation_status {
	/** File Operation Successful */
	FILE_OP_SUCCESS = 0,
	/** File Operation failed with XVSEC-MCAP error SLVERR */
	FILE_OP_FAIL_SLVERR = 1,
	/** File Operation failed with XVSEC-MCAP error DECERR */
	FILE_OP_FAIL_DECERR = 2,
	FILE_OP_RESERVED = 3,
	/** File operation failed */
	FILE_OP_FAILED,
	/** Zero size file is provided as input */
	FILE_OP_ZERO_FSIZE,
	/** Valid File size is not provided
	 *
	 * for 32-bit mode, file size must be multiple of 32-bits / 4 bytes
	 *
	 * for 128-bit mode, file size must be multiple of 128-bits / 16 bytes
	 */
	FILE_OP_INVALID_FSIZE,
	/** File operation failed with XVSEC-MCAP HW timeout on completion */
	FILE_OP_HW_BUSY,
	/** Provided file path is too long to fit in internal buffers */
	FILE_PATH_TOO_LONG
};

/*
 * FIXME:
 * There is issue with Sphinx document generation with union
 * To generate Sphinx document purposefully changed all @union to @struct
 * Need to be updated with @union when there is fix from Documentation team
 */

/**
 * @struct - mcap_regs
 * @brief	MCAP register set
 *		V1 corresponds to US/US+ devices
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_mcap_union
 */
union mcap_regs {
	/** MCAP register sets for US/US+ device*/
	struct {
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
	} v1;
	/** MCAP register sets for Versal device */
	struct {
		/** Valid flag to indicate registers validity */
		uint32_t	valid;
		/** Extended capability header register */
		uint32_t	ext_cap_header;
		/** Vendor Specific header register */
		uint32_t	vendor_header;
		/** Status Register */
		uint32_t	status_reg;
		/** Control Register */
		uint32_t	control_reg;
		/** RW Address register */
		uint32_t	address_reg;
		/** Write Data Register */
		uint32_t	wr_data_reg;
		/** Read Data Register */
		uint32_t	rd_data_reg;
	} v2;
};

/**
 * @struct - bitstream_file
 * @brief	XVSEC-MCAP bitstream parameters for programming
 *		V1 corresponds to US/US+ devices
 *
 * @ingroup xvsec_mcap_union
 */
union bitstream_file {
	/** MCAP bitstream parameters for US/US+ */
	struct {
		/** Partial clear bitstream file to
		 * program ultrascale devices
		 */
		char *partial_clr_file;
		/** bitstream file to program */
		char *bitstream_file;
		/** Status of the bitstream programming */
		enum bitstream_program_status status;
	} v1;
};

/**
 * @struct - cfg_data
 * @brief	Parameters needed to perform MCAP read and writes
 *		V1 corresponds to US/US+ devices
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_mcap_union
 */
union cfg_data {
	/** MCAP register read/write parameters for US/US+ and Versal */
	struct {
		/** access field. 'b' for byte access, 'h'for half word access,
		 *  'w' for word access
		 */
		char		access;
		/** VSEC address offset */
		uint16_t	offset;
		/** data field holds the info to write into the provided offset
		 *  for Write operation.
		 *  Holds the info at the provided offset for read operation
		 */
		uint32_t	data;
	} v1, v2;
};

/**
 * @struct - fpga_cfg_reg
 * @brief	FPGA configuration parameters to perform read and writes
 *		V1 corresponds to US/US+ devices
 *
 * @ingroup xvsec_mcap_union
 */
union fpga_cfg_reg {
	/**  FPGA configuration parameters for US/US+ Devices */
	struct {
		/** FPGA configuration register number */
		uint16_t	offset;
		/** data field holds the info to write into the provided offset
		 *  for Write operation.
		 *  Holds the info at the provided offset for read operation
		 */
		uint32_t	data;
	} v1;
};

/**
 * @struct - fpga_cfg_regs
 * @brief	FPGA configuration register set(See UG570 for more information)
 *		V1 corresponds to US/US+ devices
 *
 * @ingroup xvsec_mcap_union
 */
union fpga_cfg_regs {
	/**  FPGA configuration register for US/US+ Devices */
	struct {
		/** Valid flag to indicate registers validity */
		uint32_t valid;
		/** CRC Register */
		uint32_t crc;
		/** Frame Address Register */
		uint32_t far;
		/** Frame Data Register,
		 *  Input Register (write configuration data)
		 */
		uint32_t fdri;
		/** Frame Data Register,
		 *  Output Register (read configuration data)
		 */
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
	} v1;
};

/**
 * @struct - axi_reg_data
 * @brief	AXI register access structure (for Read & Writes)
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_mcap_union
 */
union axi_reg_data {
	/** AXI register access parameters for Versal Devices */
	struct {
		/** Holds AXI sub-device operating mode (32 bit /128 bit),
		 *  128 bit mode only supported for write operation
		 */
		enum axi_access_mode mode;
		/** AXI register address */
		uint32_t address;
		/** data field holds the data to write into the provided
		 *  address for Write operation.
		 *  Holds the data at the provided address for read operation
		 */
		uint32_t data[4];
	} v2;
};

/**
 * @struct - file_download_upload
 * @brief	File download & upload parameters for sub-devices
 *		connected via NOC
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_mcap_union
 */
union file_download_upload {
	/** File download & upload parameters for Versal Devices */
	struct {
		/** Holds  AXI sub-device operating mode (32 bit / 128 bit),
		 *  128 bit mode only supported for write operation
		 */
		enum axi_access_mode mode;
		/** Holds address type (fixed or increment ) */
		enum axi_address_type addr_type;
		/** Holds address from where download/upload should happen */
		uint32_t address;
		/** File to download/upload */
		char *file_name;
		/** Data length read in bytes (for upload option) */
		size_t length;
		/** data transfer mode */
		enum data_transfer_mode tr_mode;
		/** File Download/Upload Status */
		enum file_operation_status op_status;
		/** upload/download failed at byte index */
		size_t err_index;
	} v2;
};

/**
 * @struct - axi_cache_attr
 * @brief	AXI cache and protection settings
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_mcap_union
 */
union axi_cache_attr {
	/** AXI cache attributes for Versal Devices */
	struct {
		/** AXI cache bits */
		uint8_t axi_cache;
		/** AXI protection bits */
		uint8_t axi_prot;
	} v2;
};

/** MCAP operation codes to be used with ioctl codes */
#define CODE_MCAP_RESET			0
#define CODE_MCAP_MODULE_RESET		1
#define CODE_MCAP_FULL_RESET		2
#define CODE_MCAP_GET_DATA_REG		3
#define CODE_MCAP_GET_REG		4
#define CODE_MCAP_GET_FPGA_REG		5
#define CODE_MCAP_PROG_BITFILE		6
#define CODE_MCAP_RD_DEV_CFG_REG	7
#define	CODE_MCAP_WR_DEV_CFG_REG	8
#define CODE_MCAP_RD_FPGA_REG		9
#define CODE_MCAP_WR_FPGA_REG		10
#define CODE_MCAP_RD_AXI_REG		11
#define CODE_MCAP_WR_AXI_REG		12
#define CODE_MCAP_FILE_DOWNLOAD		13
#define CODE_MCAP_FILE_UPLOAD		14
#define CODE_MCAP_GET_REVISION		15
#define CODE_MCAP_SET_AXI_ATTR		16

/** Complete set of ioctls for MCAP VSEC */

/**
 * @defgroup xvsec_mcap_defines Definitions
 * @{
 */

/**
 * ioctl code for performing MCAP configuration logic reset
 */
#define IOC_MCAP_RESET					\
	_IO(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_RESET)
/**
 * ioctl code for performing MCAP Module reset
 */
#define IOC_MCAP_MODULE_RESET			\
	_IO(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_MODULE_RESET)
/**
 * ioctl code for performing both IOC_MCAP_RESET & IOC_MCAP_MODULE_RESET
 */
#define IOC_MCAP_FULL_RESET				\
	_IO(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_FULL_RESET)
/**
 * ioctl code for retrieving the MCAP Read Data Registers
 */
#define IOC_MCAP_GET_DATA_REGISTERS		\
	_IOR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_GET_DATA_REG, \
		uint32_t *)
/**
 * ioctl code for retrieving the MCAP Registers
 */
#define IOC_MCAP_GET_REGISTERS			\
	_IOR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_GET_REG, \
		union mcap_regs *)
/**
 * ioctl code for retrieving the FPGA configuration Registers
 */
#define IOC_MCAP_GET_FPGA_REGISTERS			\
	_IOR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_GET_FPGA_REG, \
		union fpga_cfg_regs *)
/**
 * ioctl code for programming the bitstream
 */
#define IOC_MCAP_PROGRAM_BITSTREAM		\
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_PROG_BITFILE, \
		union bitstream_file *)
/**
 * ioctl code for reading an MCAP VSEC register
 */
#define IOC_MCAP_READ_DEV_CFG_REG		\
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_RD_DEV_CFG_REG, \
		union cfg_data *)
/**
 * ioctl code for Writing to an MCAP VSEC register
 */
#define IOC_MCAP_WRITE_DEV_CFG_REG	\
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_WR_DEV_CFG_REG, \
		union cfg_data *)
/**
 * ioctl code for reading an FPGA CFG register
 */
#define IOC_MCAP_READ_FPGA_CFG_REG \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_RD_FPGA_REG, \
		union fpga_cfg_reg *)
/**
 * ioctl code for writing to an FPGA CFG register
 */
#define IOC_MCAP_WRITE_FPGA_CFG_REG \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_WR_FPGA_REG, \
		union fpga_cfg_reg *)
/**
 * ioctl code for reading the AXI register at given address
 */
#define IOC_MCAP_READ_AXI_REG \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_RD_AXI_REG, \
		union axi_reg_data *)
/**
 * ioctl code for writing the AXI register at given address with given data
 */
#define IOC_MCAP_WRITE_AXI_REG \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_WR_AXI_REG, \
		union axi_reg_data *)
/**
 * ioctl code for performing file download
 */
#define IOC_MCAP_FILE_DOWNLOAD \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_FILE_DOWNLOAD, \
		union file_download_upload *)
/**
 * ioctl code for performing file upload
 */
#define IOC_MCAP_FILE_UPLOAD \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_FILE_UPLOAD, \
		union file_download_upload *)
/**
 * ioctl code for retrieving the revision
 */
#define IOC_MCAP_GET_REVISION \
	_IOR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_GET_REVISION, \
		uint32_t *)

/**
 * ioctl code to configure axi cache & protection attributes
 */
#define IOC_MCAP_SET_AXI_ATTR \
	_IOWR(XVSEC_MCAP_IOC_MAGIC, CODE_MCAP_SET_AXI_ATTR, \
		union axi_cache_attr *)

/** @} */

#endif /* __XVSEC_MCAP_H__ */
