/*
 * This file is part of the XVSEC userspace library which provides the
 * userspace APIs to enable the XSEC driver functionality
 *
 * Copyright (c) 2018,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#ifndef __XVSEC_H__
#define __XVSEC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <byteswap.h>

/**
 * @file
 * @brief This file contains the interface declarations of XVSEC User Space
 *        Library
 *
 */

/** XVSEC API Return Value : Indicates Success of API call */
#define XVSEC_SUCCESS				(0)
/** XVSEC API Return Value : Indicates Failure of API call */
#define XVSEC_FAILURE				(-1)
/** XVSEC API Return Value : Indicates one or more NULL pointer passed
  * as parameter
  */
#define XVSEC_ERR_NULL_POINTER			(-2)
/** XVSEC API Return Value : Indicates one or more invalid parameters
  * passed
  */
#define XVSEC_ERR_INVALID_PARAM			(-3)
/** XVSEC API Return Value : Indicates mutex lock failed
  */
#define XVSEC_ERR_MUTEX_LOCK_FAIL		(-4)
/** XVSEC API Return Value : Indicates mutex unlock failed
  * while trying to release
  */
#define XVSEC_ERR_MUTEX_UNLOCK_FAIL		(-5)
/** XVSEC API Return Value : Indicates memory allocation failed */
#define XVSEC_ERR_MEM_ALLOC_FAILED		(-6)
/** XVSEC API Return Value : Indicates the requested operation is
  * not supported */
#define XVSEC_ERR_OPERATION_NOT_SUPPORTED	(-7)
/** XVSEC API Return Value : Indicates BitStream programming Failed */
#define XVSEC_ERR_BITSTREAM_PROGRAM		(-8)
/** XVSEC API Return Value : Indicates one of the linux system call Failed */
#define XVSEC_ERR_LINUX_SYSTEM_CALL		(-9)
/** XVSEC API Return Value : Indicates No more devices present to open */
#define XVSEC_MAX_DEVICES_LIMIT_REACHED		(-10)
/** XVSEC API Return Value : Indicates unsupported file format provided */
#define XVSEC_ERR_INVALID_FILE_FORMAT		(-11)
/** XVSEC API Return Value : Indicates Offset and Access combination
 *  is incorrect (Ex : u16 needs an even offset, u32 needs an offset divisible
 *  by 4 */
#define XVSEC_ERR_INVALID_OFFSET_ACCESS_COMBO	(-12)
/** XVSEC API Return Value : Indicates Capability ID is not provided */
#define XVSEC_ERR_CAPABILITY_ID_MISSING		(-13)
/** XVSEC API Return Value : Indicates Capability ID is not supported */
#define XVSEC_ERR_CAPABILITY_NOT_SUPPORTED	(-13)
/** XVSEC API Return Value : Indicates FPGA CFG Register number provided
 *  is invalid (or) access to the requested register is prohibited */
#define XVSEC_ERR_INVALID_FPGA_REG_NUM		(-14)
/** XVSEC API Return Value : Indicates Provided VSEC Offset is invalid */
#define XVSEC_ERR_INVALID_OFFSET		(-15)

/** Maximum supported Capability ID by the Library */
#define MAX_CAPS_SUPPORTED			(10)

/** Unique XVSEC handle per device.
  * This handle is needed for all XVSEC operations */
typedef uint64_t	xvsec_handle_t;

/**
 * @enum - access_type_t
 * @brief	Register/Offset Access Type(Byte/Short/Word)
 */
typedef enum access_type_t
{
	/** 8 bits access (read/write) */
	ACCESS_BYTE = 0,
	/** 16 bits access (read/write) */
	ACCESS_SHORT,
	/** 32 bits access (read/write) */
	ACCESS_WORD,
}access_type_t;

/**
 * @struct - xvsec_cap_t
 * @brief	Capability Information
 */
typedef struct xvsec_cap_t
{
	/** Capability Identifier */
	uint16_t	cap_id;
	/** Capability Name */
	char		cap_name[10];
}xvsec_cap_t;

/**
 * @struct - xvsec_cap_list_t
 * @brief	Capability List
 */
typedef struct xvsec_cap_list_t
{
	/** No of capabilities supported */
	uint16_t	no_of_caps;
	/** Capabilities Information */
	xvsec_cap_t	cap_info[MAX_CAPS_SUPPORTED];
}xvsec_cap_list_t;

/**
 * @struct - xvsec_mcap_sts_reg_t
 * @brief	MCAP Status Register Fields
 */
typedef struct xvsec_mcap_sts_reg_t
{
	/** Error */
	uint32_t err		: 1;
	/** End of Startup Signal */
	uint32_t eos		: 1;
	uint32_t reserved01	: 2;
	/** MCAP Read complete flag */
	uint32_t read_complete	: 1;
	/** MCAP Read word count */
	uint32_t read_count	: 3;
	/** MCAP Write Buffer FIFO overflow */
	uint32_t fifo_ovfl	: 1;
	uint32_t reserved02	: 3;
	/** MCAP Write Buffer FIFO Occupency */
	uint32_t fifo_occu	: 4;
	uint32_t reserved03	: 8;
	/** MCAP Request for Release Flag */
	uint32_t req4mcap_rel	: 1;
	uint32_t reserved04	: 7;
}xvsec_mcap_sts_reg_t;

/**
 * @struct - xvsec_mcap_ctl_reg_t
 * @brief	MCAP Control Register Fields
 */
typedef struct xvsec_mcap_ctl_reg_t
{
	/** MCAP Module Enable */
	uint32_t enable		: 1;
	/** MCAP Read Enable to perform FPGA CFG Read operation */
	uint32_t rd_enable	: 1;
	uint32_t reserved01	: 2;
	/** MCAP Configurable Region Reset */
	uint32_t reset		: 1;
	/** MCAP Module Reset */
	uint32_t module_reset	: 1;
	uint32_t reserved02	: 2;
	/** Request for gaining access to configurable region */
	uint32_t req4mcap_pcie	: 1;
	uint32_t reserved03	: 3;
	/** MCAP Design Switch : Must be SET after loading bitstream */
	uint32_t cfg_desgn_sw	: 1;
	uint32_t reserved04	: 3;
	/** MCAP Write Register Enable */
	uint32_t wr_reg_enable	: 1;
	uint32_t reserved05	: 15;
}xvsec_mcap_ctl_reg_t;

/**
 * @struct - xvsec_mcap_regs_t
 * @brief	MCAP Register set
 */
typedef struct xvsec_mcap_regs_t
{
	/** Extended capability header register */
	uint32_t cap_header;
	/** Vendor Specific header register */
	uint32_t vendor_header;
	/** FPGA JTAG ID register */
	uint32_t fpga_jtag_id;
	/** FPGA bit-stream version register */
	uint32_t fpga_bitstream_ver;
	/** Status Register */
	uint32_t status_reg;
	/** Control Register */
	uint32_t control_reg;
	/** Write Data Register */
	uint32_t write_data_reg;
	/** Read Data Register: 4 data words */
	uint32_t read_data_reg[4];
}xvsec_mcap_regs_t;

/**
 * @struct - xvsec_fpga_cfg_regs_t
 * @brief	FPGA Configuration Register set
 */
typedef struct xvsec_fpga_cfg_regs_t
{
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
}xvsec_fpga_cfg_regs_t;

/** Error codes in human readable form */
extern const char *error_codes[];

/*****************************************************************************/
/**
 * xvsec_lib_init() -	Initializes the XVSEC Library by allocating memory
 *			to support requested number of devices
 *
 * @param[in]	max_devices	Maximum devices to support
 *
 * @return	XVSEC_SUCCESS			: Success
 * @return	XVSEC_ERR_MEM_ALLOC_FAILED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL	: Filure
 *
 *****************************************************************************/
int xvsec_lib_init(int max_devices);

/*****************************************************************************/
/**
 * xvsec_lib_deinit() - De-initializes the XVSEC Library by
 *			freeing allocated memory and clearing the context
 *
 * @param	none
 *
 * @return	XVSEC_SUCCESS			: Success
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL	: Failure
 *****************************************************************************/
int xvsec_lib_deinit(void);

/*****************************************************************************/
/**
 * xvsec_open() - Opens XVSEC character device which is dedicated to the given
 *                bus number and device number and returns a unique handle to
 *                access the device
 *
 *
 * @param[in]	bus_no		PCIe bus number on which device sits
 * @param[in]	dev_no		Device number in the PCIe bus
 * @param[out]	handle		Unique handle returned to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_MAX_DEVICES_LIMIT_REACHED		: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 *****************************************************************************/
int xvsec_open(uint16_t bus_no, uint16_t dev_no, xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_close() - Closes XVSEC character device of provided handle
 *
 * @param[in]	handle		Unique handle to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_NULL_POINTER			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_close(xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_get_cap_list() - Returns the supported VSEC capabilities
 * 			  of the given handle
 *
 * @param[in]		handle		Unique handle to access the device
 * @param[out]		cap_list	Supported capability list
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_get_cap_list(xvsec_handle_t *handle, xvsec_cap_list_t *cap_list);

/*****************************************************************************/
/**
 * xvsec_show_device() - Shows the device information of the given handle
 *
 * @param[in]	handle		Unique handle to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_show_device(xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_mcap_reset() - Resets the configuration logic of the given handle
 *
 * @param[in]	handle		Unique handle to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_reset(xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_mcap_module_reset() - Resets the MCAP module of the given handle
 *
 * @param[in]	handle		Unique handle to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_module_reset(xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_mcap_full_reset() - Resets bothconfiguration logic & MCAP module
 *                           of the given handle
 *
 * @param[in]	handle		Unique handle to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_full_reset(xvsec_handle_t *handle);

/*****************************************************************************/
/**
 * xvsec_mcap_get_data_registers() - Returns the MCAP read data registers
 *
 * @param[in]	handle		Unique handle to access the device
 * @param[out]	data[]		MCAP read data register values
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_get_data_registers(xvsec_handle_t *handle, uint32_t data[4]);

/*****************************************************************************/
/**
 * xvsec_mcap_get_registers() - Returns the MCAP register set
 *
 * @param[in]	handle		Unique handle to access the device
 * @param[out]	mcap_regs	MCAP register values
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_get_registers(xvsec_handle_t *handle,
				xvsec_mcap_regs_t *mcap_regs);

/*****************************************************************************/
/**
 * xvsec_mcap_get_fpga_registers() - Returns the FPGA configuration register set
 *
 * @param[in]	handle		Unique handle to access the device
 * @param[out]	fpga_cfg_regs	FPGA configuration register values
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_get_fpga_registers(xvsec_handle_t *handle,
					xvsec_fpga_cfg_regs_t *fpga_cfg_regs);

/*****************************************************************************/
/**
 * xvsec_mcap_access_config_reg() - Performs read/write operations on
 *                                  the MCAP register set
 *
 * @param[in]		handle	Unique handle to access the device
 * @param[in]		offset	Register address offset from MCAP Base address
 * @param[in/out]	data	Data pointer which holds the data to write /
 *                              the data after read
 * @param[in]		access	Control specifier which specifies type of
 *                              access (8 bits/16 bits/32 bits)
 * @param[in]		write	Control specifier which specifies write/read
 *                                operation
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_INVALID_OFFSET		: Failure
 * @return	XVSEC_ERR_INVALID_OFFSET_ACCESS_COMBO	: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 *****************************************************************************/
int xvsec_mcap_access_config_reg(xvsec_handle_t *handle, uint16_t offset,
					void *data, access_type_t access,
					bool write);

/*****************************************************************************/
/**
 * xvsec_mcap_access_fpga_config_reg() - Performs read/write operations on
 *                                       the FPGA configuration register set
 *
 * @param[in]		handle	Unique handle to access the device
 * @param[in]		offset	Register address offset from MCAP Base address
 * @param[in/out]	data	Data pointer which holds the data to write /
 *                              the data after read
 * @param[in]		write	Control specifier which specifies write/read
 *                              operation
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_ERR_INVALID_FPGA_REG_NUM		: Failure
 *****************************************************************************/
int xvsec_mcap_access_fpga_config_reg(xvsec_handle_t *handle, uint16_t offset,
					void *data, bool write);

/*****************************************************************************/
/**
 * xvsec_mcap_configure_fpga() - Performs bitstream programming on FPGA
 *
 *
 * @param[in]	handle			Unique handle to access the device
 * @param[in]	partial_cfg_file	Partial Clear bitstream file
 * @param[in]	bitfile			Bitstream file
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_ERR_INVALID_FPGA_REG_NUM		: Failure
 *****************************************************************************/
int xvsec_mcap_configure_fpga(xvsec_handle_t *handle,
				char *partial_cfg_file, char *bitfile);

#endif /* __XVSEC_H__ */
