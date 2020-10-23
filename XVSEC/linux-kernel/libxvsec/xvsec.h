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


#ifndef __XVSEC_H__
#define __XVSEC_H__

#ifdef __cplusplus
}
#endif

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

/** @defgroup xvsec_api_enums Enumerations
 */
/** @defgroup xvsec_api_struct Data Structures
 */
/** @defgroup xvsec_api_union Data Structures
 */
/** @defgroup xvsec_api_func Exported Functions
 */
/** @defgroup xvsec_api_var Exported Variables
 */


/**
 * @defgroup xvsec_api_defines Definitions
 * @{
 */


/** Indicates Success of API call */
#define XVSEC_SUCCESS				(0)
/** Indicates Operation not permitted */
#define XVSEC_EPERM				(-1)
/** Indicates Failure of API call */
#define XVSEC_FAILURE				(-2)
/** Indicates one or more NULL pointer passed as parameter */
#define XVSEC_ERR_NULL_POINTER			(-3)
/** Indicates one or more invalid parameters passed */
#define XVSEC_ERR_INVALID_PARAM			(-4)
/** Indicates mutex lock failed */
#define XVSEC_ERR_MUTEX_LOCK_FAIL		(-5)
/** Indicates mutex unlock failed while trying to release */
#define XVSEC_ERR_MUTEX_UNLOCK_FAIL		(-6)
/** Indicates memory allocation failed */
#define XVSEC_ERR_MEM_ALLOC_FAILED		(-7)
/** Indicates the requested operation is not supported */
#define XVSEC_ERR_OPERATION_NOT_SUPPORTED	(-8)
/** Indicates BitStream programming Failed */
#define XVSEC_ERR_BITSTREAM_PROGRAM		(-9)
/** Indicates one of the linux system call Failed */
#define XVSEC_ERR_LINUX_SYSTEM_CALL		(-10)
/** Indicates No more devices present to open */
#define XVSEC_MAX_DEVICES_LIMIT_REACHED		(-11)
/** Indicates unsupported file format provided */
#define XVSEC_ERR_INVALID_FILE_FORMAT		(-12)
/** Indicates Offset and Access combination
 *  is incorrect (Ex : u16 needs an even offset, u32 needs an offset divisible
 *  by 4 */
#define XVSEC_ERR_INVALID_OFFSET_ACCESS_COMBO	(-13)
/** Indicates Capability ID is not provided */
#define XVSEC_ERR_CAPABILITY_ID_MISSING		(-14)
/** Indicates Capability ID is not supported */
#define XVSEC_ERR_CAPABILITY_NOT_SUPPORTED	(-15)
/** Indicates FPGA CFG Register number provided
 *  is invalid (or) access to the requested register is prohibited */
#define XVSEC_ERR_INVALID_FPGA_REG_NUM		(-16)
/** Indicates Provided VSEC Offset is invalid */
#define XVSEC_ERR_INVALID_OFFSET		(-17)

/** Maximum supported Capability ID by the Library */
#define MAX_CAPS_SUPPORTED			(10)

/** Maximum VSEC name String length */
#define XVSEC_MAX_VSEC_STR_LEN		(20)

/** @} */

/** Unique XVSEC handle per device.
  * This handle is needed for all XVSEC operations
  * @ingroup xvsec_api_var
  */
typedef uint64_t	xvsec_handle_t;

/**
 * @enum - access_type_t
 * @brief	Register/Offset Access Type(Byte/Short/Word)
 * @ingroup xvsec_api_enums
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
 * @ingroup xvsec_api_struct
 */
typedef struct xvsec_cap_t
{
	/** Capability Identifier */
	uint16_t	cap_id;
	/** MCAP Revision info */
	uint16_t	rev_id;
	/** Capability Name */
	char		cap_name[XVSEC_MAX_VSEC_STR_LEN];
	/** info to check capability supported by this drv or not*/
	bool		is_supported;
}xvsec_cap_t;

/**
 * @struct - xvsec_cap_list_t
 * @brief	Capability List
 * @ingroup xvsec_api_struct
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
 * @ingroup xvsec_api_struct
 */
typedef union xvsec_mcap_sts_reg_t {
	/** MCAP sts bits for US/US+ Devices */
	struct  {
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
	}v1;
	/** MCAP sts bits for Versal Devices */
	struct  {
		/** Reserved */
		uint32_t reserved01		: 4;
		/** MCAP Read/Write Status */
		uint32_t rw_status		: 2;
		/** Reserved */
		uint32_t reserved02		: 2;
		/** MCAP Read Complete */
		uint32_t rd_complete		: 1;
		/** Reserved */
		uint32_t reserved03		: 7;
		/** MCAP FIFO Occupancy */
		uint32_t fifo_occupancy		: 5;
		/** MCAP Write FIFO Full */
		uint32_t wr_fifo_full		: 1;
		/** Write FIFO Almost Full */
		uint32_t wr_fifo_almost_full	: 1;
		/** Write FIFO Almost Empty */
		uint32_t wr_fifo_almost_empty	: 1;
		/* MCAP Write FIFO Empty */
		uint32_t wr_fifo_empty		: 1;
		/** Write FIFO Overflow */
		uint32_t wr_fifo_overflow	: 1;
		/** Reserved */
		uint32_t reserved04		: 6;
	}v2;

}xvsec_mcap_sts_reg_t;

/**
 * @struct - xvsec_mcap_ctl_reg_t
 * @brief	MCAP Control Register Fields
 * @ingroup xvsec_api_struct
 */
typedef union xvsec_mcap_ctl_reg_t {
	/** MCAP ctrl bits for US/US+ */
	struct  {
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
	}v1;
	/** MCAP ctrl bits for Versal */
	struct  {
		/** MCAP Read Enable */
		uint32_t rd_enable	: 1;
		uint32_t reserved01	: 3;
		/** MCAP Write Enable */
		uint32_t wr_enable	: 1;
		/** MCAP 128-bit Mode */
		uint32_t mode		: 1;
		uint32_t reserved02	: 2;
		/** MCAP Module Reset */
		uint32_t reset		: 1;
		uint32_t reserved03	: 7;
		/** MCAP AXI Cache */
		uint32_t axi_cache	: 4;
		/** MCAP AXI Protect */
		uint32_t axi_protect	: 3;
		uint32_t reserved04	: 9;
	}v2;
}xvsec_mcap_ctl_reg_t;

/**
 * @struct - xvsec_mcap_regs_t
 * @brief	MCAP Register set
 * @ingroup xvsec_api_struct
 */
typedef union xvsec_mcap_regs_t {
	 /** MCAP cfg register set for US/US+ */
	struct	{
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
	}v1;
	/** MCAP cfg register set for Versal devices */
	struct {
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
	}v2;
}xvsec_mcap_regs_t;

/**
 * @struct - xvsec_fpga_cfg_regs_t
 * @brief	FPGA Configuration Register set
 * @ingroup xvsec_api_struct
 */
typedef union xvsec_fpga_cfg_regs_t {
	/** FPGA Configuration Register set for US/US+ devices */
	struct {
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
	}v1; /* fpga register set for US/US+ */
}xvsec_fpga_cfg_regs_t;

/** Error codes in human readable form
  * @ingroup xvsec_api_var
  */
extern const char *error_codes[];

/**
 * @enum - mcap_revision_t
 * @brief	Revision ID for MCAP IP
 * @ingroup xvsec_api_enums
 */
typedef enum mcap_revision_t {
	/** MCAP Revision for Ultrascale device */
	MCAP_US = 0,
	/** MCAP Revision for Ultrascale plus device */
	MCAP_USPLUS,
	/** MCAP Revision for Versal device */
	MCAP_VERSAL,
	/** Unsupported MCAP Revision by this Driver */
	INVALID_MCAP_REVISION,
}mcap_revision_t;

/**
 * @struct - _cfg_data
 * @brief	Parameters for accessing MCAP Registers(Read/Write)
 * @ingroup xvsec_api_struct
 */
typedef struct _cfg_data {
	/** access field. 'b' for byte access, 'h'for half word access,
	 * 'w' for word access
	 */
	char            access;
	/** VSEC address offset */
	uint16_t        offset;
	/** data field holds the information to write into the provided offset
	 * for Write operation.
	 * Holds the information at the provided offset for read operation
	 */
	uint32_t        data;
}cfg_data_t;

/**
 * @enum - _axi_access_mode
 * @brief  AXI Devices addressing mode (32 bit/128 bit)
 *
 * @ingroup xvsec_api_enums
 **/
typedef enum _axi_access_mode {
	/** 32-bit addressing mode */
	XVSEC_MCAP_AXI_MODE_32B = 0,
	/** 128-bit addressing mode */
	XVSEC_MCAP_AXI_MODE_128B,
}axi_access_mode_t;

/**
 * @enum - _data_transfer_mode
 * @brief	Indicates mode of data transfer for
 *		download functionality.
 *
 * @ingroup xvsec_api_enums
 **/
typedef enum _data_transfer_mode {
	/** Fast Transfer mode
	 *  and the default one when not specified */
	XVSEC_MCAP_DATA_TR_MODE_FAST = 0,
	/** Slow Transfer mode */
	XVSEC_MCAP_DATA_TR_MODE_SLOW,
}data_transfer_mode_t;

/**
 * @enum - _file_operation_status
 * @brief  Indicates File operation(Download/Upload) result
 *
 * @ingroup xvsec_api_enums
 **/
typedef enum _file_operation_status {
	/** File Operation Successful */
	XVSEC_MCAP_FILE_OP_SUCCESS = 0,
	/** File Operation failed with XVSEC-MCAP error SLVERR */
	XVSEC_MCAP_FILE_OP_FAIL_SLVERR = 1,
	/** File Operation failed with XVSEC-MCAP error DECERR */
	XVSEC_MCAP_FILE_OP_FAIL_DECERR = 2,
	XVSEC_MCAP_FILE_OP_RESERVED = 3,
	/** File operation failed */
	XVSEC_MCAP_FILE_OP_FAILED,
	/** Zero size file is provided as input */
	XVSEC_MCAP_FILE_OP_ZERO_FSIZE,
	/** Valid File size is not provided
	 *
	 * for 32-bit mode, file size must be multiple of 32-bits / 4 bytes
	 *
	 * for 128-bit mode, file size must be multiple of 128-bits / 16 bytes
	 */
	XVSEC_MCAP_FILE_OP_INVALID_FSIZE,
	/** File operation failed with XVSEC-MCAP HW timeout on completion */
	XVSEC_MCAP_FILE_OP_HW_BUSY,
	/** Provided file path is too long to fit in internal buffers */
	XVSEC_MCAP_FILE_PATH_TOO_LONG
}file_operation_status_t;


/**
 * @struct  -	_axi_reg_data
 * @brief	AXI register access structure (for Read & Writes)
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_api_union
 **/

typedef union _axi_reg_data{
	/** AXI register access parameters for Versal devices */
	struct {
		/** Holds the AXI sub-device operating mode (32 bit mode/128 bit mode),
		 *  128 bit mode only supported for write operation
		 **/
		axi_access_mode_t mode;
		/** AXI register address */
		uint32_t address;
		/** data field holds the data to write into the provided address
		 *  for Write operation.
		 *  Holds the data at the provided address for read operation
		 **/
		uint32_t data[4];
	}v2;
}axi_reg_data_t;
/**
 * @struct 	_axi_cache_attr
 * @brief	AXI cache and protection settings
 *		V2 corresponds to Versal devices
 *
 * @ingroup xvsec_api_union
 **/

typedef union _axi_cache_attr{
	/** AXI cache attributes for Versal devices */
	struct {
		/** AXI cache bits */
		uint8_t axi_cache;
		/** AXI protection bits */
		uint8_t axi_prot;
	}v2;
}axi_cache_attr_t;

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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @param[in]	dev_str		string to append to create character device. 
 * @param[out]	handle		Unique handle returned to access the device
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_MAX_DEVICES_LIMIT_REACHED		: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_open(uint16_t bus_no, uint16_t dev_no, xvsec_handle_t *handle, char *dev_str);

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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_get_data_registers(xvsec_handle_t *handle, uint32_t data[4]);

/*****************************************************************************/
/**
 * xvsec_mcap_get_revision() - Returns the MCAP read data registers
 * 
 * @param[in]   handle          Unique handle to access the device
 * @param[out]  rev[]          MCAP read data register values
 *  
 * @return      XVSEC_SUCCESS                           : Success
 * @return      XVSEC_ERR_INVALID_PARAM                 : Failure
 * @return      XVSEC_ERR_OPERATION_NOT_SUPPORTED       : Failure
 * @return      XVSEC_ERR_LINUX_SYSTEM_CALL             : Failure
 * @ingroup xvsec_api_func
 ******************************************************************************/
int xvsec_mcap_get_revision(xvsec_handle_t *handle, uint32_t *rev);

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
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
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
 * @param[inout]	data	Data pointer which holds the data to write /
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
 * @ingroup xvsec_api_func
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
 * @param[inout]	data	Data pointer which holds the data to write /
 *                              the data after read
 * @param[in]		write	Control specifier which specifies write/read
 *                              operation
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_ERR_INVALID_FPGA_REG_NUM		: Failure
 * @ingroup xvsec_api_func
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
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_configure_fpga(xvsec_handle_t *handle,
				char *partial_cfg_file, char *bitfile);


/*****************************************************************************/
/**
 * xvsec_mcap_access_axi_reg - Performs AXI read/write operations
 *
 * @param[in]		handle	Unique handle to access the device
 * @param[in]		address	axi address to be read/write
 * @param[inout]	value	Data pointer which holds the data to write/read
 *                          For Read Only 32b mode is supported. For Write 32b/128b modes are supported.
 *                          Data buffer size is assumed  4 bytes in case of 32b mode
 *                          Data buffer size is assumed 16 bytes in case of 128b mode
 *
 * @param[in]		write	Control specifier which specifies write/read
 *                              operation
 * @param[in]		mode	32bit or 128bit axi mode
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_access_axi_reg(xvsec_handle_t *handle, uint32_t address,
	void *value, bool write, axi_access_mode_t mode);

/*****************************************************************************/
/**
 * xvsec_mcap_file_download() - Performs File download at specified address
 *
 *
 * @param[in]	handle		Unique handle to access the device
 * @param[in]	fixed_address	Address Type (is it fixed/incr)
 * @param[in]	mode_128_bit	Access Mode (128 bit or 32 bit)
 * @param[in]	file_name	File to download
 * @param[in]	dev_address	The address to download the file
 * @param[in]	tr_mode		Data transfer mode(slow/fast)
 * @param[out]	op_status	file download status
 * @param[out]	err_index	file download error index
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_file_download(xvsec_handle_t *handle,
	bool fixed_address, bool mode_128_bit,
	char *file_name, uint32_t dev_address,
	data_transfer_mode_t tr_mode,
	file_operation_status_t  *op_status, size_t *err_index);

/*****************************************************************************/
/**
 * xvsec_mcap_file_upload() - Performs File Upload from specified address
 *
 *
 * @param[in]	handle			Unique handle to access the device
 * @param[in]	fixed_address	Address Type (is it fixed/incr)
 * @param[in]	file_name		File to download
 * @param[in]	dev_address		The address to download the file
 * @param[in]	length			Length of the file to upload
 * @param[out]	op_status		file upload status
 * @param[out]	err_index		file upload error index
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @return	XVSEC_FAILURE				: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_file_upload(xvsec_handle_t *handle,
	bool fixed_address, char *file_name,
	uint32_t dev_address, size_t length,
	file_operation_status_t  *op_status, size_t *err_index);

/*****************************************************************************/
/**
 * xvsec_mcap_set_axi_cache_attr - To set AXI cache and protection bits
 *
 * @param[in]		handle		Unique handle to access the device
 * @param[in]		user_attr	structure having axi cache and protection value to be set
 *
 * @return	XVSEC_SUCCESS				: Success
 * @return	XVSEC_ERR_INVALID_PARAM			: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @return	XVSEC_ERR_LINUX_SYSTEM_CALL		: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_mcap_set_axi_cache_attr(xvsec_handle_t *handle, axi_cache_attr_t *user_attr);

/*****************************************************************************/
/**
 * xvsec_lib_get_mcap_revision - To get the MCAP version info from Library
 *
 * @param[in]		handle	Unique handle to access the device
 * @param[in]		mrev	Pointer variable to get the MCAP revision
 *
 * @return	XVSEC_SUCCESS						: Success
 * @return	XVSEC_ERR_INVALID_PARAM				: Failure
 * @return	XVSEC_ERR_OPERATION_NOT_SUPPORTED	: Failure
 * @ingroup xvsec_api_func
 *****************************************************************************/
int xvsec_lib_get_mcap_revision(xvsec_handle_t *handle, uint8_t *mrev);

#ifdef __cplusplus
}
#endif

#endif /* __XVSEC_H__ */
