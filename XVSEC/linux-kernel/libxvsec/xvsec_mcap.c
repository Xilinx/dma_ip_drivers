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


#include "xvsec.h"
#include "xvsec_int.h"
#include "xvsec_cdev.h"

#define MCAP_LOOP_COUNT		1000000

#define MCAP_SYNC_DWORD		0xFFFFFFFF
#define MCAP_SYNC_BYTE0		((MCAP_SYNC_DWORD & 0xFF000000) >> 24)
#define MCAP_SYNC_BYTE1		((MCAP_SYNC_DWORD & 0x00FF0000) >> 16)
#define MCAP_SYNC_BYTE2		((MCAP_SYNC_DWORD & 0x0000FF00) >> 8)
#define MCAP_SYNC_BYTE3		((MCAP_SYNC_DWORD & 0x000000FF) >> 0)

#define MCAP_RBT_FILE		".rbt"
#define MCAP_BIT_FILE		".bit"
#define MCAP_BIN_FILE		".bin"

/* MCAP specific APIs */
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
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for IOC_MCAP_RESET "
			"failed with error %d(%s)\n", __func__, errno,
			strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
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
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_MODULE_RESET failed with error %d(%s)\n",
			__func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
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
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_FULL_RESET failed with error %d(%s)\n",
			__func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
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
	if(status < 0)
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_GET_DATA_REGISTERS failed with error %d(%s)\n",
			__func__, errno, strerror(errno));
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
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
	struct mcap_regs regs;

	/* Parameter Validation */
	if((handle == NULL) || (mcap_regs == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&regs, 0, sizeof(struct mcap_regs));
	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_GET_REGISTERS, &regs);
	if((status < 0) || (regs.valid == 0))
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_GET_REGISTERS failed with error %d(%s), "
			"valid : %d\n", __func__, errno, strerror(errno),
			regs.valid);
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	mcap_regs->cap_header		= regs.ext_cap_header;
	mcap_regs->vendor_header	= regs.vendor_header ;
	mcap_regs->fpga_jtag_id		= regs.fpga_jtag_id  ;
	mcap_regs->fpga_bitstream_ver	= regs.fpga_bit_ver  ;
	mcap_regs->status_reg		= regs.status_reg    ;
	mcap_regs->control_reg		= regs.control_reg   ;
	mcap_regs->write_data_reg	= regs.wr_data_reg   ;
	mcap_regs->read_data_reg[0]	= regs.rd_data_reg[0];
	mcap_regs->read_data_reg[1]	= regs.rd_data_reg[1];
	mcap_regs->read_data_reg[2]	= regs.rd_data_reg[2];
	mcap_regs->read_data_reg[3]	= regs.rd_data_reg[3];

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
	struct fpga_cfg_regs  regs;

	/* Parameter Validation */
	if((handle == NULL) || (fpga_cfg_regs == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;
	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	memset(&regs, 0, sizeof(struct fpga_cfg_regs));

	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_GET_FPGA_REGISTERS, &regs);

	if((status < 0) || (regs.valid == 0))
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_GET_FPGA_REGISTERS failed with error %d(%s), "
			"valid : %d\n", __func__, errno, strerror(errno),
			regs.valid);
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}


	fpga_cfg_regs->crc        =  regs.crc    ;
	fpga_cfg_regs->far        =  regs.far    ;
	fpga_cfg_regs->fdri       =  regs.fdri   ;
	fpga_cfg_regs->fdro       =  regs.fdro   ;
	fpga_cfg_regs->cmd        =  regs.cmd    ;
	fpga_cfg_regs->ctl0       =  regs.ctl0   ;
	fpga_cfg_regs->mask       =  regs.mask   ;
	fpga_cfg_regs->stat       =  regs.stat   ;
	fpga_cfg_regs->lout       =  regs.lout   ;
	fpga_cfg_regs->cor0       =  regs.cor0   ;
	fpga_cfg_regs->mfwr       =  regs.mfwr   ;
	fpga_cfg_regs->cbc        =  regs.cbc    ;
	fpga_cfg_regs->idcode     =  regs.idcode ;
	fpga_cfg_regs->axss       =  regs.axss   ;
	fpga_cfg_regs->cor1       =  regs.cor1   ;
	fpga_cfg_regs->wbstar     =  regs.wbstar ;
	fpga_cfg_regs->timer      =  regs.timer  ;
	fpga_cfg_regs->scratchpad =  regs.scratchpad  ;
	fpga_cfg_regs->bootsts    =  regs.bootsts;
	fpga_cfg_regs->ctl1       =  regs.ctl1   ;
	fpga_cfg_regs->bspi       =  regs.bspi   ;

CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);

	return ret;
}

int xvsec_mcap_access_config_reg(xvsec_handle_t *handle, uint16_t offset,
                                void *data, access_type_t access, bool write)
{
	int			ret = XVSEC_SUCCESS;
	int			status;
	int			device_index;
	struct cfg_data		cfg_data;

	/* Parameter Validation */
	if((handle == NULL) || (data == NULL) || (access > ACCESS_WORD))
		return XVSEC_ERR_INVALID_PARAM;

	if(offset >= MAX_MCAP_REG_OFFSET)
	{
		fprintf(stderr, "[XVSEC] : %s : Invalid Offset Provided\n", __func__);
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

	cfg_data.offset = offset;
	cfg_data.data = 0x0;
	if(write == true)
	{
		if(access == ACCESS_BYTE)
		{
			cfg_data.access = 'b';
			cfg_data.data = (uint32_t)(*(uint8_t *)data);
		}
		else if(access == ACCESS_SHORT)
		{
			cfg_data.access = 'h';
			cfg_data.data = (uint32_t)(*(uint16_t *)data);
		}
		else
		{
			cfg_data.access = 'w';
			cfg_data.data = (uint32_t)(*(uint32_t *)data);
		}

		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_WRITE_DEV_CFG_REG, &cfg_data);
		if(status < 0)
		{
			fprintf(stderr, "[XVSEC] : %s : ioctl for "
				"IOC_MCAP_WRITE_DEV_CFG_REG failed with "
				"error %d(%s)\n", __func__, errno,
				strerror(errno));
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
			goto CLEANUP;
		}

		fprintf(stdout, "[XVSEC] : %s : write operation completed\n", __func__);
	}
	else
	{
		if(access == ACCESS_BYTE)
		{
			cfg_data.access = 'b';
		}
		else if(access == ACCESS_SHORT)
		{
			cfg_data.access = 'h';
		}
		else
		{
			cfg_data.access = 'w';
		}

		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_READ_DEV_CFG_REG, &cfg_data);
		if(status < 0)
		{
			fprintf(stderr, "[XVSEC] : %s : ioctl for "
				"IOC_MCAP_READ_DEV_CFG_REG failed "
				"with error %d(%s)\n", __func__, errno,
				strerror(errno));
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
			goto CLEANUP;
		}

		if(access == ACCESS_BYTE)
		{
			*(uint8_t *)data = (uint8_t)cfg_data.data;
		}
		else if(access == ACCESS_SHORT)
		{
			*(uint16_t *)data = (uint16_t)cfg_data.data;
		}
		else
		{
			*(uint32_t *)data = (uint32_t)cfg_data.data;
		}

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
	struct fpga_cfg_reg	cfg_data;

	/* Parameter Validation */
	if((handle == NULL) || (data == NULL))
		return XVSEC_ERR_INVALID_PARAM;

	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	cfg_data.offset = offset;
	cfg_data.data = 0x0;
	if(write == true)
	{
		cfg_data.data = (uint32_t)(*(uint32_t *)data);

		status = ioctl(xvsec_user_ctx[device_index].fd,
			IOC_MCAP_WRITE_FPGA_CFG_REG, &cfg_data);
		if(status < 0)
		{
			fprintf(stderr, "[XVSEC] : %s : ioctl for "
				"IOC_MCAP_WRITE_FPGA_CFG_REG failed with "
				"error %d(%s)\n", __func__, errno,
				strerror(errno));
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
			if(errno == EACCES)
				ret = XVSEC_ERR_INVALID_FPGA_REG_NUM;
			goto CLEANUP;
		}

		fprintf(stdout, "[XVSEC] : %s : write operation "
			"completed\n", __func__);
	}
	else
	{
		status = ioctl(xvsec_user_ctx[device_index].fd,
				IOC_MCAP_READ_FPGA_CFG_REG, &cfg_data);
		if(status < 0)
		{
			fprintf(stderr, "[XVSEC] : %s : ioctl for "
				"IOC_MCAP_READ_FPGA_CFG_REG failed with "
				"error %d(%s)\n", __func__, errno,
				strerror(errno));
			ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
			if(errno == EACCES)
				ret = XVSEC_ERR_INVALID_FPGA_REG_NUM;
			goto CLEANUP;
		}

		*(uint32_t *)data = (uint32_t)cfg_data.data;

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
	struct bitstream_file	bit_files;

	if((handle == NULL) ||
		((partial_cfg_file == NULL) && (bitfile == NULL)))
		return XVSEC_ERR_INVALID_PARAM;


	status = xvsec_validate_handle(handle);
	if(status < 0)
		return status;

	device_index = ((handle_t *)handle)->index;

	pthread_mutex_lock(&xvsec_user_ctx[device_index].mutex);

	bit_files.partial_clr_file = partial_cfg_file;
	bit_files.bitstream_file = bitfile;
	bit_files.status = MCAP_BITSTREAM_PROGRAM_FAILURE;
	status = ioctl(xvsec_user_ctx[device_index].fd,
		IOC_MCAP_PROGRAM_BITSTREAM, &bit_files);
	if((status < 0) ||
		(bit_files.status != MCAP_BITSTREAM_PROGRAM_SUCCESS))
	{
		fprintf(stderr, "[XVSEC] : %s : ioctl for "
			"IOC_MCAP_PROGRAM_BITSTREAM failed with error %d(%s), "
			"status : %d\n", __func__, errno, strerror(errno),
			bit_files.status);
		ret = XVSEC_ERR_LINUX_SYSTEM_CALL;
		goto CLEANUP;
	}

	fprintf(stdout, "[XVSEC] : %s : Bitstream Program successful\n", __func__);


CLEANUP:
	pthread_mutex_unlock(&xvsec_user_ctx[device_index].mutex);
	return ret;

}
