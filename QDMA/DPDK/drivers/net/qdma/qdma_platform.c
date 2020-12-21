/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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
#include "qdma.h"
#include <rte_malloc.h>
#include <rte_spinlock.h>

static rte_spinlock_t resource_lock = RTE_SPINLOCK_INITIALIZER;
static rte_spinlock_t reg_access_lock = RTE_SPINLOCK_INITIALIZER;

struct err_code_map error_code_map_list[] = {
	{QDMA_SUCCESS,				0},
	{QDMA_ERR_INV_PARAM,			EINVAL},
	{QDMA_ERR_NO_MEM,			ENOMEM},
	{QDMA_ERR_HWACC_BUSY_TIMEOUT,		EBUSY},
	{QDMA_ERR_HWACC_INV_CONFIG_BAR,		EINVAL},
	{QDMA_ERR_HWACC_NO_PEND_LEGCY_INTR,	EINVAL},
	{QDMA_ERR_HWACC_BAR_NOT_FOUND,		EINVAL},
	{QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED,	EINVAL},
	{QDMA_ERR_RM_RES_EXISTS,		EPERM},
	{QDMA_ERR_RM_RES_NOT_EXISTS,		EINVAL},
	{QDMA_ERR_RM_DEV_EXISTS,		EPERM},
	{QDMA_ERR_RM_DEV_NOT_EXISTS,		EINVAL},
	{QDMA_ERR_RM_NO_QUEUES_LEFT,		EPERM},
	{QDMA_ERR_RM_QMAX_CONF_REJECTED,	EPERM},
	{QDMA_ERR_MBOX_FMAP_WR_FAILED,		EIO},
	{QDMA_ERR_MBOX_NUM_QUEUES,		EINVAL},
	{QDMA_ERR_MBOX_INV_QID,			EINVAL},
	{QDMA_ERR_MBOX_INV_RINGSZ,		EINVAL},
	{QDMA_ERR_MBOX_INV_BUFSZ,		EINVAL},
	{QDMA_ERR_MBOX_INV_CNTR_TH,		EINVAL},
	{QDMA_ERR_MBOX_INV_TMR_TH,		EINVAL},
	{QDMA_ERR_MBOX_INV_MSG,			EINVAL},
	{QDMA_ERR_MBOX_SEND_BUSY,		EBUSY},
	{QDMA_ERR_MBOX_NO_MSG_IN,		EINVAL},
	{QDMA_ERR_MBOX_ALL_ZERO_MSG,		EINVAL},
};

/*****************************************************************************/
/**
 * qdma_calloc(): allocate memory and initialize with 0
 *
 * @num_blocks:  number of blocks of contiguous memory of @size
 * @size:    size of each chunk of memory
 *
 * Return: pointer to the memory block created on success and NULL on failure
 *****************************************************************************/
void *qdma_calloc(uint32_t num_blocks, uint32_t size)
{
	return rte_calloc(NULL, num_blocks, size, 0);
}

/*****************************************************************************/
/**
 * qdma_memfree(): free the memory
 *
 * @memptr:  pointer to the memory block
 *
 * Return:	None
 *****************************************************************************/
void qdma_memfree(void *memptr)
{
	return rte_free(memptr);
}

/*****************************************************************************/
/**
 * qdma_resource_lock_take() - take lock to access resource management APIs
 *
 * @return	None
 *****************************************************************************/
void qdma_resource_lock_take(void)
{
	rte_spinlock_lock(&resource_lock);
}

/*****************************************************************************/
/**
 * qdma_resource_lock_give() - release lock after accessing
 *                             resource management APIs
 *
 * @return	None
 *****************************************************************************/
void qdma_resource_lock_give(void)
{
	rte_spinlock_unlock(&resource_lock);
}

/*****************************************************************************/
/**
 * qdma_reg_write() - Register write API.
 *
 * @dev_hndl:   device handle
 * @reg_offst:  QDMA Config bar register offset to write
 * @val:	value to be written
 *
 * Return:	None
 *****************************************************************************/
void qdma_reg_write(void *dev_hndl, uint32_t reg_offst, uint32_t val)
{
	struct qdma_pci_dev *qdma_dev;
	uint64_t bar_addr;

	qdma_dev = ((struct rte_eth_dev *)dev_hndl)->data->dev_private;
	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	*((volatile uint32_t *)(bar_addr + reg_offst)) = val;
}

/*****************************************************************************/
/**
 * qdma_reg_read() - Register read API.
 *
 * @dev_hndl:   device handle
 * @reg_offst:  QDMA Config bar register offset to be read
 *
 * Return: Value read
 *****************************************************************************/
uint32_t qdma_reg_read(void *dev_hndl, uint32_t reg_offst)
{
	struct qdma_pci_dev *qdma_dev;
	uint64_t bar_addr;
	uint32_t val;

	qdma_dev = ((struct rte_eth_dev *)dev_hndl)->data->dev_private;
	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	val = *((volatile uint32_t *)(bar_addr + reg_offst));

	return val;
}

/*****************************************************************************/
/**
 * qdma_reg_access_lock() - Lock function for Register access
 *
 * @dev_hndl:   device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_reg_access_lock(void *dev_hndl)
{
	(void) dev_hndl;
	rte_spinlock_lock(&reg_access_lock);
	return 0;
}

/*****************************************************************************/
/**
 * qdma_reg_access_release() - Release function for Register access
 *
 * @dev_hndl:   device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_reg_access_release(void *dev_hndl)
{
	(void) dev_hndl;
	rte_spinlock_unlock(&reg_access_lock);
	return 0;
}

/*****************************************************************************/
/**
 * qdma_udelay() - delay function to be used in the common library
 *
 * @delay_usec:   delay in microseconds
 *
 * Return:	None
 *****************************************************************************/
void qdma_udelay(uint32_t delay_usec)
{
	rte_delay_us(delay_usec);
}

/*****************************************************************************/
/**
 * qdma_get_hw_access() - function to get the qdma_hw_access
 *
 * @dev_hndl:   device handle
 * @dev_cap: pointer to hold qdma_hw_access structure
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
void qdma_get_hw_access(void *dev_hndl, struct qdma_hw_access **hw)
{
	struct qdma_pci_dev *qdma_dev;
	qdma_dev = ((struct rte_eth_dev *)dev_hndl)->data->dev_private;
	*hw = qdma_dev->hw_access;
}

/*****************************************************************************/
/**
 * qdma_strncpy(): copy n size string from source to destination buffer
 *
 * @memptr:  pointer to the memory block
 *
 * Return:	None
 *****************************************************************************/
void qdma_strncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
}

/*****************************************************************************/
/**
 * qdma_get_err_code() - function to get the qdma access mapped error code
 *
 * @acc_err_code: qdma access error code which is a negative input value
 *
 * Return:   returns the platform specific error code
 *****************************************************************************/
int qdma_get_err_code(int acc_err_code)
{
	/* Multiply acc_err_code with -1 to convert it to a postive number
	 * and use it as an array index for error codes.
	 */
	acc_err_code *= -1;

	return -(error_code_map_list[acc_err_code].err_code);
}
