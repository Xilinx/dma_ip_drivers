/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qdma_access_common.h"
#include "qdma_platform.h"
#include "qdma_soft_reg.h"
#include "qdma_soft_access.h"
#include "qdma_s80_hard_access.h"
#include "eqdma_soft_access.h"
#include "qdma_reg_dump.h"

#ifdef ENABLE_WPP_TRACING
#include "qdma_access_common.tmh"
#endif

/* qdma version info */
#define RTL_BASE_VERSION                        2
#define RTL_PATCH_VERSION                       3

/**
 * enum qdma_ip - To hold ip type
 */
enum qdma_ip {
	QDMA_OR_VERSAL_IP,
	EQDMA_IP
};


/*
 * hw_monitor_reg() - polling a register repeatly until
 *	(the register value & mask) == val or time is up
 *
 * return -QDMA_BUSY_IIMEOUT_ERR if register value didn't match, 0 other wise
 */
int hw_monitor_reg(void *dev_hndl, uint32_t reg, uint32_t mask,
		uint32_t val, uint32_t interval_us, uint32_t timeout_us)
{
	int count;
	uint32_t v;

	if (!interval_us)
		interval_us = QDMA_REG_POLL_DFLT_INTERVAL_US;
	if (!timeout_us)
		timeout_us = QDMA_REG_POLL_DFLT_TIMEOUT_US;

	count = timeout_us / interval_us;

	do {
		v = qdma_reg_read(dev_hndl, reg);
		if ((v & mask) == val)
			return QDMA_SUCCESS;
		qdma_udelay(interval_us);
	} while (--count);

	v = qdma_reg_read(dev_hndl, reg);
	if ((v & mask) == val)
		return QDMA_SUCCESS;

	qdma_log_error("%s: Reg read=%u Expected=%u, err:%d\n",
				   __func__, v, val,
				   -QDMA_ERR_HWACC_BUSY_TIMEOUT);
	return -QDMA_ERR_HWACC_BUSY_TIMEOUT;
}

/*****************************************************************************/
/**
 * qdma_get_rtl_version() - Function to get the rtl_version in
 * string format
 *
 * @rtl_version: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_rtl_version(enum qdma_rtl_version rtl_version)
{
	switch (rtl_version) {
	case QDMA_RTL_PATCH:
		return "RTL Patch";
	case QDMA_RTL_BASE:
		return "RTL Base";
	default:
		qdma_log_error("%s: invalid rtl_version(%d), err:%d\n",
				__func__, rtl_version, -QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_ip_type() - Function to get the ip type in string format
 *
 * @ip_type: IP Type
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_ip_type(enum qdma_ip_type ip_type)
{
	switch (ip_type) {
	case QDMA_VERSAL_HARD_IP:
		return "Versal Hard IP";
	case QDMA_VERSAL_SOFT_IP:
		return "Versal Soft IP";
	case QDMA_SOFT_IP:
		return "QDMA Soft IP";
	case EQDMA_SOFT_IP:
		return "EQDMA Soft IP";
	default:
		qdma_log_error("%s: invalid ip type(%d), err:%d\n",
				__func__, ip_type, -QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_device_type() - Function to get the device type in
 * string format
 *
 * @device_type: Device Type
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_device_type(enum qdma_device_type device_type)
{
	switch (device_type) {
	case QDMA_DEVICE_SOFT:
		return "Soft IP";
	case QDMA_DEVICE_VERSAL:
		return "Versal S80 Hard IP";
	default:
		qdma_log_error("%s: invalid device type(%d), err:%d\n",
				__func__, device_type, -QDMA_ERR_INV_PARAM);
		return NULL;
	}
}

/*****************************************************************************/
/**
 * qdma_get_vivado_release_id() - Function to get the vivado release id in
 * string format
 *
 * @vivado_release_id: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
static const char *qdma_get_vivado_release_id(
				enum qdma_vivado_release_id vivado_release_id)
{
	switch (vivado_release_id) {
	case QDMA_VIVADO_2018_3:
		return "vivado 2018.3";
	case QDMA_VIVADO_2019_1:
		return "vivado 2019.1";
	case QDMA_VIVADO_2019_2:
		return "vivado 2019.2";
	case QDMA_VIVADO_2020_1:
		return "vivado 2020.1";
	case QDMA_VIVADO_2020_2:
		return "vivado 2020.2";
	default:
		qdma_log_error("%s: invalid vivado_release_id(%d), err:%d\n",
				__func__,
				vivado_release_id,
				-QDMA_ERR_INV_PARAM);
		return NULL;
	}
}


void qdma_write_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, const uint32_t *values)
{
	uint32_t index, reg_addr;

	for (index = idx; index < (idx + cnt); index++) {
		reg_addr = reg_offst + (index * sizeof(uint32_t));
		qdma_reg_write(dev_hndl, reg_addr, values[index - idx]);
	}
}

void qdma_read_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, uint32_t *values)
{
	uint32_t index, reg_addr;

	reg_addr = reg_offst + (idx * sizeof(uint32_t));
	for (index = 0; index < cnt; index++) {
		values[index] = qdma_reg_read(dev_hndl, reg_addr +
					      (index * sizeof(uint32_t)));
	}
}

void qdma_fetch_version_details(uint8_t is_vf, uint32_t version_reg_val,
		struct qdma_hw_version_info *version_info)
{
	uint32_t rtl_version, vivado_release_id, ip_type, device_type;
	const char *version_str;

	if (!is_vf) {
		rtl_version = FIELD_GET(QDMA_GLBL2_RTL_VERSION_MASK,
				version_reg_val);
		vivado_release_id =
			FIELD_GET(QDMA_GLBL2_VIVADO_RELEASE_MASK,
					version_reg_val);
		device_type = FIELD_GET(QDMA_GLBL2_DEVICE_ID_MASK,
				version_reg_val);
		ip_type = FIELD_GET(QDMA_GLBL2_VERSAL_IP_MASK,
				version_reg_val);
	} else {
		rtl_version =
			FIELD_GET(QDMA_GLBL2_VF_RTL_VERSION_MASK,
					version_reg_val);
		vivado_release_id =
			FIELD_GET(QDMA_GLBL2_VF_VIVADO_RELEASE_MASK,
					version_reg_val);
		device_type = FIELD_GET(QDMA_GLBL2_VF_DEVICE_ID_MASK,
				version_reg_val);
		ip_type =
			FIELD_GET(QDMA_GLBL2_VF_VERSAL_IP_MASK,
					version_reg_val);
	}

	switch (rtl_version) {
	case 0:
		version_info->rtl_version = QDMA_RTL_BASE;
		break;
	case 1:
		version_info->rtl_version = QDMA_RTL_PATCH;
		break;
	default:
		version_info->rtl_version = QDMA_RTL_NONE;
		break;
	}

	version_str = qdma_get_rtl_version(version_info->rtl_version);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_rtl_version_str,
				version_str,
				QDMA_HW_VERSION_STRING_LEN);

	switch (device_type) {
	case 0:
		version_info->device_type = QDMA_DEVICE_SOFT;
		break;
	case 1:
		version_info->device_type = QDMA_DEVICE_VERSAL;
		break;
	default:
		version_info->device_type = QDMA_DEVICE_NONE;
		break;
	}

	version_str = qdma_get_device_type(version_info->device_type);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_device_type_str,
				version_str,
				QDMA_HW_VERSION_STRING_LEN);


	if (version_info->device_type == QDMA_DEVICE_SOFT) {
		switch (ip_type) {
		case 0:
			version_info->ip_type = QDMA_SOFT_IP;
			break;
		case 1:
			version_info->ip_type = EQDMA_SOFT_IP;
			break;
		default:
			version_info->ip_type = QDMA_NONE_IP;
		}
	} else {
		switch (ip_type) {
		case 0:
			version_info->ip_type = QDMA_VERSAL_HARD_IP;
			break;
		case 1:
			version_info->ip_type = QDMA_VERSAL_SOFT_IP;
			break;
		default:
			version_info->ip_type = QDMA_NONE_IP;
		}
	}

	version_str = qdma_get_ip_type(version_info->ip_type);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_ip_type_str,
			version_str,
			QDMA_HW_VERSION_STRING_LEN);

	if (version_info->ip_type == QDMA_SOFT_IP) {
		switch (vivado_release_id) {
		case 0:
			version_info->vivado_release = QDMA_VIVADO_2018_3;
			break;
		case 1:
			version_info->vivado_release = QDMA_VIVADO_2019_1;
			break;
		case 2:
			version_info->vivado_release = QDMA_VIVADO_2019_2;
			break;
		default:
			version_info->vivado_release = QDMA_VIVADO_NONE;
			break;
		}
	} else if (version_info->ip_type == EQDMA_SOFT_IP) {
		switch (vivado_release_id) {
		case 0:
			version_info->vivado_release = QDMA_VIVADO_2020_1;
			break;
		case 1:
			version_info->vivado_release = QDMA_VIVADO_2020_2;
			break;
		default:
			version_info->vivado_release = QDMA_VIVADO_NONE;
			break;
		}
	} else { /* Versal case */
		switch (vivado_release_id) {
		case 0:
			version_info->vivado_release = QDMA_VIVADO_2019_2;
			break;
		default:
			version_info->vivado_release = QDMA_VIVADO_NONE;
			break;
		}
	}

	version_str = qdma_get_vivado_release_id(
			version_info->vivado_release);
	if (version_str != NULL)
		qdma_strncpy(version_info->qdma_vivado_release_id_str,
				version_str,
				QDMA_HW_VERSION_STRING_LEN);
}


/*
 * dump_reg() - Helper function to dump register value into string
 *
 * return len - length of the string copied into buffer
 */
int dump_reg(char *buf, int buf_sz, uint32_t raddr,
		const char *rname, uint32_t rval)
{
	/* length of the line should be minimum 80 chars.
	 * If below print pattern is changed, check for
	 * new buffer size requirement
	 */
	if (buf_sz < DEBGFS_LINE_SZ) {
		qdma_log_error("%s: buf_sz(%d) < expected(%d): err: %d\n",
						__func__,
						buf_sz, DEBGFS_LINE_SZ,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return QDMA_SNPRINTF_S(buf, buf_sz, DEBGFS_LINE_SZ,
			"[%#7x] %-47s %#-10x %u\n",
			raddr, rname, rval, rval);

}

void qdma_memset(void *to, uint8_t val, uint32_t size)
{
	uint32_t i;
	uint8_t *_to = (uint8_t *)to;

	for (i = 0; i < size; i++)
		_to[i] = val;
}

/*****************************************************************************/
/**
 * qdma_queue_cmpt_cidx_read() - function to read the CMPT CIDX register
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @qid:	Queue id relative to the PF/VF calling this API
 * @reg_info:	pointer to array to hold the values read
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_queue_cmpt_cidx_read(void *dev_hndl, uint8_t is_vf,
		uint16_t qid, struct qdma_q_cmpt_cidx_reg_info *reg_info)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX :
			QDMA_OFFSET_DMAP_SEL_CMPT_CIDX;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	if (!reg_info) {
		qdma_log_error("%s: reg_info is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}


	reg_addr += qid * QDMA_CMPT_CIDX_STEP;

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	reg_info->wrb_cidx =
		FIELD_GET(QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK, reg_val);
	reg_info->counter_idx =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK,
			reg_val));
	reg_info->wrb_en =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK,
			reg_val));
	reg_info->irq_en =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK, reg_val));
	reg_info->timer_idx =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK, reg_val));
	reg_info->trig_mode =
		(uint8_t)(FIELD_GET(QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK, reg_val));

	return QDMA_SUCCESS;
}


/*****************************************************************************/
/**
 * qdma_initiate_flr() - function to initiate Function Level Reset
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_initiate_flr(void *dev_hndl, uint8_t is_vf)
{
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_reg_write(dev_hndl, reg_addr, 1);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_is_flr_done() - function to check whether the FLR is done or not
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 * @done:	if FLR process completed ,  done is 1 else 0.
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_is_flr_done(void *dev_hndl, uint8_t is_vf, uint8_t *done)
{
	int rv;
	uint32_t reg_addr = (is_vf) ?  QDMA_OFFSET_VF_REG_FLR_STATUS :
			QDMA_OFFSET_PF_REG_FLR_STATUS;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	if (!done) {
		qdma_log_error("%s: done is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	/* wait for it to become zero */
	rv = hw_monitor_reg(dev_hndl, reg_addr, QDMA_FLR_STATUS_MASK,
			0, 5 * QDMA_REG_POLL_DFLT_INTERVAL_US,
			QDMA_REG_POLL_DFLT_TIMEOUT_US);
	if (rv < 0)
		*done = 0;
	else
		*done = 1;

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_is_config_bar() - function for the config bar verification
 *
 * @dev_hndl:	device handle
 * @is_vf:	Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_is_config_bar(void *dev_hndl, uint8_t is_vf, enum qdma_ip *ip)
{
	uint32_t reg_val = 0;
	uint32_t reg_addr = (is_vf) ? QDMA_OFFSET_VF_VERSION :
			QDMA_OFFSET_CONFIG_BLOCK_ID;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, reg_addr);

	/** TODO: Version register for VFs is 0x5014 for EQDMA and
	 *  0x1014 for QDMA/Versal. First time reading 0x5014 for
	 *  all the device and based on the upper 16 bits value
	 *  (i.e. 0x1fd3), finding out whether its EQDMA or QDMA/Versal
	 *  for EQDMA VFs.
	 *  Need to modify this logic once the hardware team
	 *  comes up with a common register for VFs
	 */
	if (is_vf) {
		if (FIELD_GET(QDMA_GLBL2_VF_UNIQUE_ID_MASK, reg_val)
				!= QDMA_MAGIC_NUMBER) {
			/* Its either QDMA or Versal */
			*ip = EQDMA_IP;
			reg_addr = EQDMA_OFFSET_VF_VERSION;
			reg_val = qdma_reg_read(dev_hndl, reg_addr);
		} else {
			*ip = QDMA_OR_VERSAL_IP;
			return QDMA_SUCCESS;
		}
	}

	if (FIELD_GET(QDMA_CONFIG_BLOCK_ID_MASK, reg_val)
			!= QDMA_MAGIC_NUMBER) {
		qdma_log_error("%s: Invalid config bar, err:%d\n",
					__func__,
					-QDMA_ERR_HWACC_INV_CONFIG_BAR);
		return -QDMA_ERR_HWACC_INV_CONFIG_BAR;
	}

	return QDMA_SUCCESS;
}

int qdma_acc_reg_dump_buf_len(void *dev_hndl,
		enum qdma_ip_type ip_type, int *buflen)
{
	uint32_t len = 0;
	int rv = 0;

	*buflen = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	switch (ip_type) {
	case QDMA_SOFT_IP:
		len = qdma_soft_reg_dump_buf_len();
		break;
	case QDMA_VERSAL_HARD_IP:
		len = qdma_s80_hard_reg_dump_buf_len();
		break;
	case EQDMA_SOFT_IP:
		len = eqdma_reg_dump_buf_len();
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	*buflen = (int)len;
	return rv;
}

int qdma_acc_reg_info_len(void *dev_hndl,
		enum qdma_ip_type ip_type, int *buflen, int *num_regs)
{
	uint32_t len = 0;
	int rv = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!buflen) {
		qdma_log_error("%s: buflen is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	if (!num_regs) {
		qdma_log_error("%s: num_regs is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	*buflen = 0;

	switch (ip_type) {
	case QDMA_SOFT_IP:
		len = 0;
		*num_regs = 0;
		break;
	case QDMA_VERSAL_HARD_IP:
		len = qdma_s80_hard_reg_dump_buf_len();
		*num_regs = (int)((len / REG_DUMP_SIZE_PER_LINE) - 1);
		break;
	case EQDMA_SOFT_IP:
		len = eqdma_reg_dump_buf_len();
		*num_regs = (int)((len / REG_DUMP_SIZE_PER_LINE) - 1);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	*buflen = (int)len;
	return rv;
}

int qdma_acc_context_buf_len(void *dev_hndl,
		enum qdma_ip_type ip_type, uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *buflen)
{
	int rv = 0;

	*buflen = 0;
	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv = qdma_soft_context_buf_len(st, q_type, buflen);
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_context_buf_len(st, q_type, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_context_buf_len(st, q_type, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}

int qdma_acc_get_num_config_regs(void *dev_hndl,
		enum qdma_ip_type ip_type, uint32_t *num_regs)
{
	int rv = 0;

	*num_regs = 0;
	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
			__func__, -QDMA_ERR_INV_PARAM);

		return -QDMA_ERR_INV_PARAM;
	}

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv = qdma_get_config_num_regs();
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_get_config_num_regs();
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_get_config_num_regs();
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	*num_regs = rv;

	return 0;
}

/*****************************************************************************/
/**
 * qdma_acc_get_config_regs() - Function to get qdma config registers.
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @ip_type:	QDMA IP Type
 * @reg_data:   pointer to register data to be filled
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_get_config_regs(void *dev_hndl, uint8_t is_vf,
		enum qdma_ip_type ip_type,
		uint32_t *reg_data)
{
	struct xreg_info *reg_info;
	uint32_t count = 0;
	uint32_t num_regs;
	int rv = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (is_vf) {
		qdma_log_error("%s: Get Config regs not valid for VF, err:%d\n",
			__func__,
			-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (reg_data == NULL) {
		qdma_log_error("%s: reg_data is NULL, err:%d\n",
						__func__,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	switch (ip_type) {
	case QDMA_SOFT_IP:
		num_regs = qdma_get_config_num_regs();
		reg_info = qdma_get_config_regs();
		break;
	case QDMA_VERSAL_HARD_IP:
		num_regs = qdma_s80_hard_get_config_num_regs();
		reg_info = qdma_s80_hard_get_config_regs();
		break;
	case EQDMA_SOFT_IP:
		num_regs = eqdma_get_config_num_regs();
		reg_info = eqdma_get_config_regs();
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	for (count = 0; count < num_regs - 1; count++) {
		reg_data[count] = qdma_reg_read(dev_hndl,
				reg_info[count].addr);
	}

	return rv;
}


/*****************************************************************************/
/**
 * qdma_acc_dump_config_regs() - Function to get qdma config register dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @ip_type:	QDMA IP Type
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		enum qdma_ip_type ip_type,
		char *buf, uint32_t buflen)
{
	int rv = 0;

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv =  qdma_soft_dump_config_regs(dev_hndl, is_vf,
				buf, buflen);
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_dump_config_regs(dev_hndl, is_vf,
				buf, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_dump_config_regs(dev_hndl, is_vf,
				buf, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_acc_dump_reg_info() - Function to dump fileds in
 * a specified register.
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP Type
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_reg_info(void *dev_hndl,
		enum qdma_ip_type ip_type, uint32_t reg_addr,
		uint32_t num_regs, char *buf, uint32_t buflen)
{
	int rv = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	if (!buf || !buflen) {
		qdma_log_error("%s: Invalid input buffer, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	switch (ip_type) {
	case QDMA_SOFT_IP:
		QDMA_SNPRINTF_S(buf, buflen, DEBGFS_LINE_SZ,
		"QDMA reg field info not supported for QDMA_SOFT_IP\n");
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_dump_reg_info(dev_hndl, reg_addr,
				num_regs, buf, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_dump_reg_info(dev_hndl, reg_addr,
				num_regs, buf, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_acc_dump_queue_context() - Function to get qdma queue context dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP Type
 * @st:		Queue Mode (ST or MM)
 * @q_type:	Queue Type
 * @ctxt_data:  Context Data
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_queue_context(void *dev_hndl,
		enum qdma_ip_type ip_type,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen)
{
	int rv = 0;

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv = qdma_soft_dump_queue_context(dev_hndl,
				st, q_type, ctxt_data, buf, buflen);
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_dump_queue_context(dev_hndl,
				st, q_type, ctxt_data, buf, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_dump_queue_context(dev_hndl,
				st, q_type, ctxt_data, buf, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_acc_read_dump_queue_context() - Function to read and dump the queue
 * context in the user-provided buffer. This API is valid only for PF and
 * should not be used for VFs. For VF's use qdma_dump_queue_context() API
 * after reading the context through mailbox.
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP type
 * @hw_qid:     queue id
 * @st:		Queue Mode(ST or MM)
 * @q_type:	Queue type(H2C/C2H/CMPT)*
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_read_dump_queue_context(void *dev_hndl,
				enum qdma_ip_type ip_type,
				uint16_t qid_hw,
				uint8_t st,
				enum qdma_dev_q_type q_type,
				char *buf, uint32_t buflen)
{
	int rv = QDMA_SUCCESS;

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv = qdma_soft_read_dump_queue_context(dev_hndl,
				qid_hw, st, q_type, buf, buflen);
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_read_dump_queue_context(dev_hndl,
				qid_hw, st, q_type, buf, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_read_dump_queue_context(dev_hndl,
				qid_hw, st, q_type, buf, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}

/*****************************************************************************/
/**
 * qdma_acc_dump_config_reg_list() - Dump the registers
 *
 * @dev_hndl:		device handle
 * @ip_type:		QDMA ip type
 * @num_regs :		Max registers to read
 * @reg_list :		array of reg addr and reg values
 * @buf :		pointer to buffer to be filled
 * @buflen :		Length of the buffer
 *
 * Return: returns the platform specific error code
 *****************************************************************************/
int qdma_acc_dump_config_reg_list(void *dev_hndl,
		enum qdma_ip_type ip_type,
		uint32_t num_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen)
{
	int rv = 0;

	switch (ip_type) {
	case QDMA_SOFT_IP:
		rv = qdma_soft_dump_config_reg_list(dev_hndl,
				num_regs,
				reg_list, buf, buflen);
		break;
	case QDMA_VERSAL_HARD_IP:
		rv = qdma_s80_hard_dump_config_reg_list(dev_hndl,
				num_regs,
				reg_list, buf, buflen);
		break;
	case EQDMA_SOFT_IP:
		rv = eqdma_dump_config_reg_list(dev_hndl,
				num_regs,
				reg_list, buf, buflen);
		break;
	default:
		qdma_log_error("%s: Invalid version number, err = %d",
			__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	return rv;
}


/*****************************************************************************/
/**
 * qdma_get_function_number() - Function to get the function number
 *
 * @dev_hndl:	device handle
 * @func_id:	pointer to hold the function id
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_get_function_number(void *dev_hndl, uint8_t *func_id)
{
	if (!dev_hndl || !func_id) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	*func_id = (uint8_t)qdma_reg_read(dev_hndl,
			QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET);

	return QDMA_SUCCESS;
}


/*****************************************************************************/
/**
 * qdma_hw_error_intr_setup() - Function to set up the qdma error
 * interrupt
 *
 * @dev_hndl:	device handle
 * @func_id:	Function id
 * @err_intr_index:	Interrupt vector
 * @rearm:	rearm or not
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_intr_setup(void *dev_hndl, uint16_t func_id,
		uint8_t err_intr_index)
{
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val =
		FIELD_SET(QDMA_GLBL_ERR_FUNC_MASK, func_id) |
		FIELD_SET(QDMA_GLBL_ERR_VEC_MASK, err_intr_index);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_hw_error_intr_rearm() - Function to re-arm the error interrupt
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
static int qdma_hw_error_intr_rearm(void *dev_hndl)
{
	uint32_t reg_val = 0;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	reg_val = qdma_reg_read(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT);
	reg_val |= FIELD_SET(QDMA_GLBL_ERR_ARM_MASK, 1);

	qdma_reg_write(dev_hndl, QDMA_OFFSET_GLBL_ERR_INT, reg_val);

	return QDMA_SUCCESS;
}

/*****************************************************************************/
/**
 * qdma_get_error_code() - function to get the qdma access mapped
 *				error code
 *
 * @acc_err_code: qdma access error code
 *
 * Return:   returns the platform specific error code
 *****************************************************************************/
int qdma_get_error_code(int acc_err_code)
{
	return qdma_get_err_code(acc_err_code);
}

int qdma_hw_access_init(void *dev_hndl, uint8_t is_vf,
				struct qdma_hw_access *hw_access)
{
	int rv = QDMA_SUCCESS;
	enum qdma_ip ip = EQDMA_IP;

	struct qdma_hw_version_info version_info;

	if (!dev_hndl) {
		qdma_log_error("%s: dev_handle is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	if (!hw_access) {
		qdma_log_error("%s: hw_access is NULL, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = qdma_is_config_bar(dev_hndl, is_vf, &ip);
	if (rv != QDMA_SUCCESS) {
		qdma_log_error("%s: config bar passed is INVALID, err:%d\n",
				__func__, -QDMA_ERR_INV_PARAM);
		return rv;
	}

	qdma_memset(hw_access, 0, sizeof(struct qdma_hw_access));

	if (ip == EQDMA_IP)
		hw_access->qdma_get_version = &eqdma_get_version;
	else
		hw_access->qdma_get_version = &qdma_get_version;
	hw_access->qdma_init_ctxt_memory = &qdma_init_ctxt_memory;
	hw_access->qdma_fmap_conf = &qdma_fmap_conf;
	hw_access->qdma_sw_ctx_conf = &qdma_sw_ctx_conf;
	hw_access->qdma_pfetch_ctx_conf = &qdma_pfetch_ctx_conf;
	hw_access->qdma_cmpt_ctx_conf = &qdma_cmpt_ctx_conf;
	hw_access->qdma_hw_ctx_conf = &qdma_hw_ctx_conf;
	hw_access->qdma_credit_ctx_conf = &qdma_credit_ctx_conf;
	hw_access->qdma_indirect_intr_ctx_conf = &qdma_indirect_intr_ctx_conf;
	hw_access->qdma_set_default_global_csr = &qdma_set_default_global_csr;
	hw_access->qdma_global_csr_conf = &qdma_global_csr_conf;
	hw_access->qdma_global_writeback_interval_conf =
					&qdma_global_writeback_interval_conf;
	hw_access->qdma_queue_pidx_update = &qdma_queue_pidx_update;
	hw_access->qdma_queue_cmpt_cidx_read = &qdma_queue_cmpt_cidx_read;
	hw_access->qdma_queue_cmpt_cidx_update = &qdma_queue_cmpt_cidx_update;
	hw_access->qdma_queue_intr_cidx_update = &qdma_queue_intr_cidx_update;
	hw_access->qdma_mm_channel_conf = &qdma_mm_channel_conf;
	hw_access->qdma_get_user_bar = &qdma_get_user_bar;
	hw_access->qdma_get_function_number = &qdma_get_function_number;
	hw_access->qdma_get_device_attributes = &qdma_get_device_attributes;
	hw_access->qdma_hw_error_intr_setup = &qdma_hw_error_intr_setup;
	hw_access->qdma_hw_error_intr_rearm = &qdma_hw_error_intr_rearm;
	hw_access->qdma_hw_error_enable = &qdma_hw_error_enable;
	hw_access->qdma_hw_get_error_name = &qdma_hw_get_error_name;
	hw_access->qdma_hw_error_process = &qdma_hw_error_process;
	hw_access->qdma_dump_config_regs = &qdma_soft_dump_config_regs;
	hw_access->qdma_dump_queue_context = &qdma_soft_dump_queue_context;
	hw_access->qdma_read_dump_queue_context =
					&qdma_soft_read_dump_queue_context;
	hw_access->qdma_dump_intr_context = &qdma_dump_intr_context;
	hw_access->qdma_is_legacy_intr_pend = &qdma_is_legacy_intr_pend;
	hw_access->qdma_clear_pend_legacy_intr = &qdma_clear_pend_legacy_intr;
	hw_access->qdma_legacy_intr_conf = &qdma_legacy_intr_conf;
	hw_access->qdma_initiate_flr = &qdma_initiate_flr;
	hw_access->qdma_is_flr_done = &qdma_is_flr_done;
	hw_access->qdma_get_error_code = &qdma_get_error_code;
	hw_access->qdma_read_reg_list = &qdma_read_reg_list;
	hw_access->qdma_dump_config_reg_list =
			&qdma_soft_dump_config_reg_list;
	hw_access->qdma_dump_reg_info = &qdma_dump_reg_info;
	hw_access->mbox_base_pf = QDMA_OFFSET_MBOX_BASE_PF;
	hw_access->mbox_base_vf = QDMA_OFFSET_MBOX_BASE_VF;
	hw_access->qdma_max_errors = QDMA_ERRS_ALL;

	rv = hw_access->qdma_get_version(dev_hndl, is_vf, &version_info);
	if (rv != QDMA_SUCCESS)
		return rv;

	qdma_log_info("Device Type: %s\n",
			qdma_get_device_type(version_info.device_type));

	qdma_log_info("IP Type: %s\n",
			qdma_get_ip_type(version_info.ip_type));

	qdma_log_info("Vivado Release: %s\n",
		qdma_get_vivado_release_id(version_info.vivado_release));

	if (version_info.ip_type == QDMA_VERSAL_HARD_IP) {
		hw_access->qdma_init_ctxt_memory =
				&qdma_s80_hard_init_ctxt_memory;
		hw_access->qdma_qid2vec_conf = &qdma_s80_hard_qid2vec_conf;
		hw_access->qdma_fmap_conf = &qdma_s80_hard_fmap_conf;
		hw_access->qdma_sw_ctx_conf = &qdma_s80_hard_sw_ctx_conf;
		hw_access->qdma_pfetch_ctx_conf =
				&qdma_s80_hard_pfetch_ctx_conf;
		hw_access->qdma_cmpt_ctx_conf = &qdma_s80_hard_cmpt_ctx_conf;
		hw_access->qdma_hw_ctx_conf = &qdma_s80_hard_hw_ctx_conf;
		hw_access->qdma_credit_ctx_conf =
				&qdma_s80_hard_credit_ctx_conf;
		hw_access->qdma_indirect_intr_ctx_conf =
				&qdma_s80_hard_indirect_intr_ctx_conf;
		hw_access->qdma_set_default_global_csr =
					&qdma_s80_hard_set_default_global_csr;
		hw_access->qdma_queue_pidx_update =
				&qdma_s80_hard_queue_pidx_update;
		hw_access->qdma_queue_cmpt_cidx_update =
				&qdma_s80_hard_queue_cmpt_cidx_update;
		hw_access->qdma_queue_intr_cidx_update =
				&qdma_s80_hard_queue_intr_cidx_update;
		hw_access->qdma_get_user_bar = &qdma_cmp_get_user_bar;
		hw_access->qdma_get_device_attributes =
				&qdma_s80_hard_get_device_attributes;
		hw_access->qdma_dump_config_regs =
				&qdma_s80_hard_dump_config_regs;
		hw_access->qdma_dump_intr_context =
				&qdma_s80_hard_dump_intr_context;
		hw_access->qdma_hw_error_enable =
				&qdma_s80_hard_hw_error_enable;
		hw_access->qdma_hw_error_process =
				&qdma_s80_hard_hw_error_process;
		hw_access->qdma_hw_get_error_name =
				&qdma_s80_hard_hw_get_error_name;
		hw_access->qdma_legacy_intr_conf = NULL;
		hw_access->qdma_read_reg_list = &qdma_s80_hard_read_reg_list;
		hw_access->qdma_dump_config_reg_list =
				&qdma_s80_hard_dump_config_reg_list;
		hw_access->qdma_dump_queue_context =
				&qdma_s80_hard_dump_queue_context;
		hw_access->qdma_read_dump_queue_context =
				&qdma_s80_hard_read_dump_queue_context;
		hw_access->qdma_dump_reg_info = &qdma_s80_hard_dump_reg_info;
		hw_access->qdma_max_errors = QDMA_S80_HARD_ERRS_ALL;
	}

	if (version_info.ip_type == EQDMA_SOFT_IP) {
		hw_access->qdma_init_ctxt_memory = &eqdma_init_ctxt_memory;
		hw_access->qdma_sw_ctx_conf = &eqdma_sw_ctx_conf;
		hw_access->qdma_pfetch_ctx_conf = &eqdma_pfetch_ctx_conf;
		hw_access->qdma_cmpt_ctx_conf = &eqdma_cmpt_ctx_conf;
		hw_access->qdma_indirect_intr_ctx_conf =
				&eqdma_indirect_intr_ctx_conf;
		hw_access->qdma_dump_config_regs = &eqdma_dump_config_regs;
		hw_access->qdma_dump_intr_context = &eqdma_dump_intr_context;
		hw_access->qdma_hw_error_enable = &eqdma_hw_error_enable;
		hw_access->qdma_hw_error_process = &eqdma_hw_error_process;
		hw_access->qdma_hw_get_error_name = &eqdma_hw_get_error_name;
		hw_access->qdma_hw_ctx_conf = &eqdma_hw_ctx_conf;
		hw_access->qdma_credit_ctx_conf = &eqdma_credit_ctx_conf;
		hw_access->qdma_set_default_global_csr =
				&eqdma_set_default_global_csr;
		hw_access->qdma_get_device_attributes =
				&eqdma_get_device_attributes;
		hw_access->qdma_get_user_bar = &eqdma_get_user_bar;
		hw_access->qdma_read_reg_list = &eqdma_read_reg_list;
		hw_access->qdma_dump_config_reg_list =
				&eqdma_dump_config_reg_list;
		hw_access->qdma_dump_queue_context =
				&eqdma_dump_queue_context;
		hw_access->qdma_read_dump_queue_context =
				&eqdma_read_dump_queue_context;
		hw_access->qdma_dump_reg_info = &eqdma_dump_reg_info;
		/* All CSR and Queue space register belongs to Window 0.
		 * Mailbox and MSIX register belongs to Window 1
		 * Therefore, Mailbox offsets are different for EQDMA
		 * Mailbox offset for PF : 128K + original address
		 * Mailbox offset for VF : 16K + original address
		 */
		hw_access->mbox_base_pf = EQDMA_OFFSET_MBOX_BASE_PF;
		hw_access->mbox_base_vf = EQDMA_OFFSET_MBOX_BASE_VF;
		hw_access->qdma_max_errors = EQDMA_ERRS_ALL;
	}

	return QDMA_SUCCESS;
}
