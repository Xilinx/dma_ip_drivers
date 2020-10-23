/*
 * This file is part of the XVSEC userspace library which provides the
 * userspace APIs to enable the XSEC driver functionality
 *
 * Copyright (c) 2018-2020  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#include "xvsec.h"
#include "xvsec_int.h"
#include "xvsec_drv.h"	/* kernel character Driver layer API's*/
#include "xvsec_mcap.h"	/* kernel layer MCAP API's */

#define MCAP_LOOP_COUNT		1000000

#define MCAP_SYNC_DWORD		0xFFFFFFFF
#define MCAP_SYNC_BYTE0		((MCAP_SYNC_DWORD & 0xFF000000) >> 24)
#define MCAP_SYNC_BYTE1		((MCAP_SYNC_DWORD & 0x00FF0000) >> 16)
#define MCAP_SYNC_BYTE2		((MCAP_SYNC_DWORD & 0x0000FF00) >> 8)
#define MCAP_SYNC_BYTE3		((MCAP_SYNC_DWORD & 0x000000FF) >> 0)

#define MCAP_RBT_FILE		".rbt"
#define MCAP_BIT_FILE		".bit"
#define MCAP_BIN_FILE		".bin"

int check_error_code(int errcode, const char* fstr)
{
	int ret = XVSEC_ERR_LINUX_SYSTEM_CALL;


	if(errcode == XVSEC_EPERM)
	{
		fprintf(stderr, "[XVSEC] :Operation is not supported\n");
		ret = XVSEC_ERR_OPERATION_NOT_SUPPORTED;
	}
	else
	{
		fprintf(stderr, "[XVSEC] : ioctl failed for %s ,"
			"failed with error %d(%s)\n", fstr, errno,
			strerror(errno));
	}

	return ret;
}

/* MCAP specific APIs */
int xvsec_lib_get_mcap_revision(xvsec_handle_t *handle, uint8_t *mrev)
{
	int status = 0;
	/* Parameter Validation */
	if((handle == NULL) || (mrev == NULL))
	{
		status = XVSEC_ERR_INVALID_PARAM;
		goto CLEANUP;
	}

	status = xvsec_validate_handle(handle);
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : handle corrupted!");
		goto CLEANUP;
	}

	/*return the mcap revision*/
	if(((handle_t *)handle)->mrev == VSEC_REV_UNKNOWN)
	{
		status = xvsec_mcap_get_revision(handle, (uint32_t*)mrev);
		if(status != XVSEC_SUCCESS)
		{
			fprintf(stderr, "[XVSEC] : ioctl for get mcap revision failed. ret: %d", status);
			status = XVSEC_FAILURE;
			goto CLEANUP;
		}
		((handle_t *)handle)->mrev = *mrev;
		goto CLEANUP;
	}

	*mrev = ((handle_t *)handle)->mrev;

CLEANUP:
	return status;
}

int xvsec_mcap_reset(xvsec_handle_t *handle)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if(handle == NULL)
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd, IOC_MCAP_RESET);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_module_reset(xvsec_handle_t *handle)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if(handle == NULL)
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd, IOC_MCAP_MODULE_RESET);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_full_reset(xvsec_handle_t *handle)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if(handle == NULL)
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd, IOC_MCAP_FULL_RESET);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_get_revision(xvsec_handle_t *handle, uint32_t *rev)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if((handle == NULL) || (rev == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_GET_REVISION, rev);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;
}

int xvsec_mcap_set_axi_cache_attr(xvsec_handle_t *handle, axi_cache_attr_t *user_attr)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if((handle == NULL) || (user_attr == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_SET_AXI_ATTR, user_attr);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;
}


int xvsec_mcap_get_data_registers(xvsec_handle_t *handle, uint32_t data[4])
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;

	/* Parameter Validation */
	if(handle == NULL)
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(data, 0, sizeof(uint32_t)*4);
	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_GET_DATA_REGISTERS, data);
	if(status != XVSEC_SUCCESS)
	{
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;
}

int xvsec_mcap_get_registers(xvsec_handle_t *handle,
	xvsec_mcap_regs_t *mcap_regs)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;
	union mcap_regs regs;
	uint8_t mrev;

	/* Parameter Validation */
	if((handle == NULL) || (mcap_regs == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_lib_get_mcap_revision(handle, &mrev);
	if((status != XVSEC_SUCCESS) || (mrev >= INVALID_MCAP_REVISION))
		return XVSEC_FAILURE;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&regs, 0, sizeof(union mcap_regs));
	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_GET_REGISTERS, &regs);
	if((status != XVSEC_SUCCESS) || ( (mrev != MCAP_VERSAL) & (regs.v1.valid == 0)) || ( (mrev == MCAP_VERSAL) & (regs.v2.valid == 0)))
	{
		fprintf(stderr, "[XVSEC] : %s, mrev: %d, v1.valid:%d, v2.valid:%d\n", __func__, mrev, regs.v1.valid, regs.v2.valid);
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

	if( mrev == MCAP_VERSAL )
	{
		mcap_regs->v2.ext_cap_header      = regs.v2.ext_cap_header;
		mcap_regs->v2.vendor_header       = regs.v2.vendor_header;
		mcap_regs->v2.status_reg          = regs.v2.status_reg;
		mcap_regs->v2.control_reg         = regs.v2.control_reg;
		mcap_regs->v2.address_reg         = regs.v2.address_reg;
		mcap_regs->v2.wr_data_reg         = regs.v2.wr_data_reg;
		mcap_regs->v2.rd_data_reg         = regs.v2.rd_data_reg;
	}
	else
	{
		mcap_regs->v1.cap_header			= regs.v1.ext_cap_header;
		mcap_regs->v1.vendor_header			= regs.v1.vendor_header ;
		mcap_regs->v1.fpga_jtag_id			= regs.v1.fpga_jtag_id  ;
		mcap_regs->v1.fpga_bitstream_ver		= regs.v1.fpga_bit_ver  ;
		mcap_regs->v1.status_reg			= regs.v1.status_reg    ;
		mcap_regs->v1.control_reg			= regs.v1.control_reg   ;
		mcap_regs->v1.write_data_reg			= regs.v1.wr_data_reg   ;
		mcap_regs->v1.read_data_reg[0]			= regs.v1.rd_data_reg[0];
		mcap_regs->v1.read_data_reg[1]			= regs.v1.rd_data_reg[1];
		mcap_regs->v1.read_data_reg[2]			= regs.v1.rd_data_reg[2];
		mcap_regs->v1.read_data_reg[3]			= regs.v1.rd_data_reg[3];
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_get_fpga_registers(xvsec_handle_t *handle,
	xvsec_fpga_cfg_regs_t *fpga_cfg_regs)
{
	int ret = XVSEC_SUCCESS;
	int status;
	int device_index;
	union fpga_cfg_regs  regs;
	uint8_t mrev;

	/* Parameter Validation */
	if((handle == NULL) || (fpga_cfg_regs == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_lib_get_mcap_revision(handle, &mrev);
	if((status != XVSEC_SUCCESS) || (mrev >= INVALID_MCAP_REVISION))
		return XVSEC_FAILURE;


	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;
	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&regs, 0, sizeof(union fpga_cfg_regs));

	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_GET_FPGA_REGISTERS, &regs);

	if( (status != XVSEC_SUCCESS) || ( (mrev != MCAP_VERSAL) & (regs.v1.valid == 0)))
	{
		fprintf(stderr, "[XVSEC] : %s, valid : %d, mrev: %d\n", __func__, mrev, regs.v1.valid);
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

	if( (mrev == MCAP_US) || (mrev == MCAP_USPLUS) )
	{

		fpga_cfg_regs->v1.crc        =  regs.v1.crc    ;
		fpga_cfg_regs->v1.far        =  regs.v1.far    ;
		fpga_cfg_regs->v1.fdri       =  regs.v1.fdri   ;
		fpga_cfg_regs->v1.fdro       =  regs.v1.fdro   ;
		fpga_cfg_regs->v1.cmd        =  regs.v1.cmd    ;
		fpga_cfg_regs->v1.ctl0       =  regs.v1.ctl0   ;
		fpga_cfg_regs->v1.mask       =  regs.v1.mask   ;
		fpga_cfg_regs->v1.stat       =  regs.v1.stat   ;
		fpga_cfg_regs->v1.lout       =  regs.v1.lout   ;
		fpga_cfg_regs->v1.cor0       =  regs.v1.cor0   ;
		fpga_cfg_regs->v1.mfwr       =  regs.v1.mfwr   ;
		fpga_cfg_regs->v1.cbc        =  regs.v1.cbc    ;
		fpga_cfg_regs->v1.idcode     =  regs.v1.idcode ;
		fpga_cfg_regs->v1.axss       =  regs.v1.axss   ;
		fpga_cfg_regs->v1.cor1       =  regs.v1.cor1   ;
		fpga_cfg_regs->v1.wbstar     =  regs.v1.wbstar ;
		fpga_cfg_regs->v1.timer      =  regs.v1.timer  ;
		fpga_cfg_regs->v1.scratchpad =  regs.v1.scratchpad  ;
		fpga_cfg_regs->v1.bootsts    =  regs.v1.bootsts;
		fpga_cfg_regs->v1.ctl1       =  regs.v1.ctl1   ;
		fpga_cfg_regs->v1.bspi       =  regs.v1.bspi   ;
	}
	else
	{
		fprintf(stderr, "[XVSEC] : mcap version is not valid: %d\n", mrev);
		regs.v1.valid = 0;
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_access_config_reg(xvsec_handle_t *handle, uint16_t offset,
	void *data, access_type_t access, bool write)
{
	int             ret = XVSEC_SUCCESS;
	int             status;
	int             device_index;
	union cfg_data  cfg_data;
	cfg_data_t      usr_cfg_data;
	uint8_t         mrev;

	/* Parameter Validation */
	if((handle == NULL) || (data == NULL) || (access > ACCESS_WORD))
		return XVSEC_ERR_INVALID_PARAM;


	status = xvsec_lib_get_mcap_revision(handle, &mrev);
	if((status != XVSEC_SUCCESS) || (mrev >= INVALID_MCAP_REVISION))
		return XVSEC_FAILURE;


	if(( (mrev == MCAP_US) || (mrev == MCAP_USPLUS)) && (offset >= MAX_MCAP_REG_OFFSET))
	{
		fprintf(stderr, "[XVSEC] : %s : Invalid Offset Provided for US/US+ device\n", __func__);
		return XVSEC_ERR_INVALID_OFFSET;
	}
	else if((mrev == MCAP_VERSAL) && (offset >= MAX_MCAPV2_REG_OFFSET))
	{
		fprintf(stderr, "[XVSEC] : %s : Invalid Offset Provided for Versal device\n", __func__);
		return XVSEC_ERR_INVALID_OFFSET;
	}

	if(((access == ACCESS_SHORT) && (offset % 2 != 0)) ||
		((access == ACCESS_WORD) && (offset % 4 != 0)))
		return XVSEC_ERR_INVALID_OFFSET_ACCESS_COMBO;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	usr_cfg_data.offset = offset;
	usr_cfg_data.data = 0x0;
	if(write == true)
	{
		if(access == ACCESS_BYTE)
		{
			usr_cfg_data.access = 'b';
			usr_cfg_data.data = (uint32_t)(*(uint8_t *)data);
		}
		else if(access == ACCESS_SHORT)
		{
			usr_cfg_data.access = 'h';
			usr_cfg_data.data = (uint32_t)(*(uint16_t *)data);
		}
		else
		{
			usr_cfg_data.access = 'w';
			usr_cfg_data.data = (uint32_t)(*(uint32_t *)data);
		}

		if( mrev == MCAP_VERSAL ) /* cfg data for Versal devices */
		{
			cfg_data.v2.offset = usr_cfg_data.offset;
			cfg_data.v2.access = usr_cfg_data.access;
			cfg_data.v2.data   = usr_cfg_data.data;
		}
		else			  /* cfg data for US/US+ devices */ 
		{
			cfg_data.v1.offset = usr_cfg_data.offset;
			cfg_data.v1.access = usr_cfg_data.access;
			cfg_data.v1.data   = usr_cfg_data.data;
		}

		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_WRITE_DEV_CFG_REG, &cfg_data);

		if(status != XVSEC_SUCCESS)
		{
			ret = check_error_code(status, __func__);
			goto CLEANUP;
		}

		fprintf(stdout, "[XVSEC] : %s : write operation completed\n", __func__);
	}
	else
	{
		if(access == ACCESS_BYTE)
		{
			usr_cfg_data.access = 'b';
		}
		else if(access == ACCESS_SHORT)
		{
			usr_cfg_data.access = 'h';
		}
		else
		{
			usr_cfg_data.access = 'w';
		}
		if( mrev == MCAP_VERSAL )
		{
			cfg_data.v2.offset = usr_cfg_data.offset;
			cfg_data.v2.access = usr_cfg_data.access;
		}
		else
		{
			cfg_data.v1.offset = usr_cfg_data.offset;
			cfg_data.v1.access = usr_cfg_data.access;
		}


		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_READ_DEV_CFG_REG, &cfg_data);

		if(status != XVSEC_SUCCESS)
		{
			ret = check_error_code(status, __func__);
			goto CLEANUP;
		}

		if(mrev == MCAP_VERSAL)
			usr_cfg_data.data   = cfg_data.v2.data;
		else
			usr_cfg_data.data   = cfg_data.v1.data;

		if(access == ACCESS_BYTE)
		{
			*(uint8_t *)data = (uint8_t)usr_cfg_data.data;
		}
		else if(access == ACCESS_SHORT)
		{
			*(uint16_t *)data = (uint16_t)usr_cfg_data.data;
		}
		else
		{
			*(uint32_t *)data = (uint32_t)usr_cfg_data.data;
		}

		fprintf(stdout, "[XVSEC] : %s : read operation "
			"completed\n", __func__);
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}


int xvsec_mcap_access_axi_reg(xvsec_handle_t *handle, uint32_t address,
	void *value, bool write, axi_access_mode_t mode)

{
	int		ret = XVSEC_SUCCESS;
	int		status;
	int		device_index;
	axi_reg_data_t	axi_data;
	uint32_t* data = (uint32_t*)value;

	/* Parameter Validation */
	if((handle == NULL) || (data == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	axi_data.v2.mode = mode;
	axi_data.v2.address = address;
	axi_data.v2.data[0] = 0;;

	if(write == true)
	{
		axi_data.v2.data[0] = (uint32_t)data[0];
		axi_data.v2.data[1] = (uint32_t)data[1];
		axi_data.v2.data[2] = (uint32_t)data[2];
		axi_data.v2.data[3] = (uint32_t)data[3];

		status = ioctl(xvsec_user_ctx[device_index].fd,
				IOC_MCAP_WRITE_AXI_REG, &axi_data);

		if(status != XVSEC_SUCCESS)
		{
			ret = check_error_code(status, __func__);
			goto CLEANUP;
		}

		fprintf(stdout, "[XVSEC] : %s : write operation "
				"completed\n", __func__);
	}
	else
	{
		status = ioctl(xvsec_user_ctx[device_index].fd,
				IOC_MCAP_READ_AXI_REG, &axi_data);

		if(status != XVSEC_SUCCESS)
		{
			ret = check_error_code(status, __func__);
			goto CLEANUP;
		}

		data[0] = (uint32_t)axi_data.v2.data[0];

		fprintf(stdout, "[XVSEC] : %s : read operation "
				"completed\n", __func__);
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}


int xvsec_mcap_access_fpga_config_reg(xvsec_handle_t *handle, uint16_t offset,
		void *data, bool write)

{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	union fpga_cfg_reg	cfg_data;
	uint8_t mrev;

	/* Parameter Validation */
	if((handle == NULL) || (data == NULL))
		return XVSEC_ERR_INVALID_PARAM;


	status = xvsec_lib_get_mcap_revision(handle, &mrev);
	if((status != XVSEC_SUCCESS) || (mrev >= INVALID_MCAP_REVISION))
		return XVSEC_FAILURE;


	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	cfg_data.v1.offset = offset;
	cfg_data.v1.data = 0;

	if(write == true)
	{
		cfg_data.v1.data = (uint32_t)(*(uint32_t *)data);

		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_WRITE_FPGA_CFG_REG, &cfg_data);

		if(status != XVSEC_SUCCESS)
		{
			if(errno == EACCES)
				ret = XVSEC_ERR_INVALID_FPGA_REG_NUM;
			else
				ret = check_error_code(status, __func__);

			goto CLEANUP;
		}

		fprintf(stdout, "[XVSEC] : %s : write operation "
			"completed\n", __func__);
	}
	else
	{
		status = ioctl(xvsec_user_ctx[device_index].fd,
				IOC_MCAP_READ_FPGA_CFG_REG, &cfg_data);

		if(status != XVSEC_SUCCESS)
		{
			if(errno == EACCES)
				ret = XVSEC_ERR_INVALID_FPGA_REG_NUM;
			else
				ret = check_error_code(status, __func__);

			goto CLEANUP;
		}

		*(uint32_t *)data = (uint32_t)cfg_data.v1.data;

		fprintf(stdout, "[XVSEC] : %s : read operation "
			"completed\n", __func__);
	}

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_configure_fpga(xvsec_handle_t *handle,
	char *partial_cfg_file, char *bitfile)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	union bitstream_file	bit_files;

	if((handle == NULL) ||
		((partial_cfg_file == NULL) && (bitfile == NULL)))
		return XVSEC_ERR_INVALID_PARAM;


	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	/* V1 arguementes for US/US+ devices to program bitstreasm */
	bit_files.v1.partial_clr_file = partial_cfg_file;
	bit_files.v1.bitstream_file = bitfile;
	bit_files.v1.status = MCAP_BITSTREAM_PROGRAM_FAILURE;
	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_PROGRAM_BITSTREAM, &bit_files);
	
	if((status != XVSEC_SUCCESS) ||
		(bit_files.v1.status != MCAP_BITSTREAM_PROGRAM_SUCCESS))
	{
		fprintf(stderr, "[XVSEC] : %s : err status : %d\n", __func__, bit_files.v1.status);
		ret = check_error_code(status, __func__);
		goto CLEANUP;
	}

	fprintf(stdout, "[XVSEC] : %s : Bitstream Program successful\n", __func__);


CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;

}

int xvsec_mcap_file_download(xvsec_handle_t *handle,
	bool fixed_address, bool mode_128_bit,
	char *file_name, uint32_t dev_address,
	data_transfer_mode_t tr_mode,
	file_operation_status_t  *op_status, size_t *err_index)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	union file_download_upload download_info;

	if ((handle == NULL) || (file_name == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&download_info, 0, sizeof(union file_download_upload));

	download_info.v2.mode =
		(mode_128_bit == true) ? MCAP_AXI_MODE_128B : MCAP_AXI_MODE_32B;
	download_info.v2.addr_type =
		(fixed_address == true) ? FIXED_ADDRESS : INCREMENT_ADDRESS;
	download_info.v2.address = dev_address;
	download_info.v2.file_name = file_name;
	download_info.v2.op_status = FILE_OP_FAILED;
	download_info.v2.tr_mode =
		(tr_mode == XVSEC_MCAP_DATA_TR_MODE_SLOW) ? DATA_TRANSFER_MODE_SLOW : DATA_TRANSFER_MODE_FAST;

	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_FILE_DOWNLOAD, &download_info);

	*op_status = (file_operation_status_t)download_info.v2.op_status;
	*err_index = download_info.v2.err_index;

	if((status < 0) ||
		(download_info.v2.op_status != FILE_OP_SUCCESS))
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_FILE_DOWNLOAD failed with error %d(%s), "
			"status : %d, failed at index: %ld\n", __func__,
			errno, strerror(errno),
			download_info.v2.op_status,
			download_info.v2.err_index);

		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	fprintf(stdout, "[XVSEC] : %s : File Download successful\n", __func__);

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;
}

int xvsec_mcap_file_upload(xvsec_handle_t *handle,
	bool fixed_address, char *file_name,
	uint32_t dev_address, size_t length,
	file_operation_status_t  *op_status, size_t *err_index)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	union file_download_upload upload_info;

	if ((handle == NULL) || (file_name == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&upload_info, 0, sizeof(union file_download_upload));

	upload_info.v2.addr_type =
		(fixed_address == true) ? FIXED_ADDRESS : INCREMENT_ADDRESS;
	upload_info.v2.address = dev_address;
	upload_info.v2.file_name = file_name;
	upload_info.v2.length = length;
	upload_info.v2.op_status = FILE_OP_FAILED;

	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_FILE_UPLOAD, &upload_info);

	*op_status = (file_operation_status_t)upload_info.v2.op_status;
	*err_index = upload_info.v2.err_index;

	if((status < 0) ||
		(upload_info.v2.op_status != FILE_OP_SUCCESS))
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_FILE_UPLOAD failed with error %d(%s), "
			"status : %d, failed at index: %ld\n", __func__,
			errno, strerror(errno),
			upload_info.v2.op_status,
			upload_info.v2.err_index);

		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	if (upload_info.v2.length != length) {
		fprintf(stderr, "[XVSEC] : %s : Partial file uploaded, "
				"Requested len : %zu, Uploaded len: %ld\n",
				__func__, length, upload_info.v2.length);
		ret = XVSEC_FAILURE;
		goto CLEANUP;
	}

	fprintf(stdout, "[XVSEC] : %s : File Upload successful\n", __func__);

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;

}
