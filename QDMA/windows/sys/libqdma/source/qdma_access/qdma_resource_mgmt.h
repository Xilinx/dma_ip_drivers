/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef __QDMA_RESOURCE_MGMT_H_
#define __QDMA_RESOURCE_MGMT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA resource management interface definitions
 *
 * Header file *qdma_resource_mgmt.h* defines data structures and function
 * signatures exported for QDMA queue management.
 */

#include "qdma_platform_env.h"
#include "qdma_access_export.h"

/**
 * enum qdma_dev_q_range: Q ranage check
 */
enum qdma_dev_q_range {
	/** @QDMA_DEV_Q_IN_RANGE: Q belongs to dev */
	QDMA_DEV_Q_IN_RANGE,
	/** @QDMA_DEV_Q_OUT_OF_RANGE: Q does not belong to dev */
	QDMA_DEV_Q_OUT_OF_RANGE,
	/** @QDMA_DEV_Q_RANGE_MAX: total Q validity states */
	QDMA_DEV_Q_RANGE_MAX
};

/*****************************************************************************/
/**
 * qdma_master_resource_create(): create the master q resource
 *
 * @bus_start:  Bus number of the device i.e. pdev->bus->number
 * @bus_end:    Ending bus number i.e. the subordinate bus number of the
 *              parent bridge
 * @q_base:     base from which this master resource needs to be created
 * @total_q:     total queues in this master resource
 * @dma_device_index: DMA device identifier assigned by resource manager to
 *                    track the number of devices
 *
 * A master resource per driver per board is created to manage the queues
 * allocated to this driver.
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_master_resource_create(uint32_t bus_start, uint32_t bus_end,
		int q_base, uint32_t total_q, uint32_t *dma_device_index);

/*****************************************************************************/
/**
 * qdma_master_resource_destroy(): destroy the master q resource
 *
 * @dma_device_index:  DMA device identifier this master resource belongs to
 *
 * Return:	None
 *****************************************************************************/
void qdma_master_resource_destroy(uint32_t dma_device_index);

/*****************************************************************************/
/**
 * qdma_dev_entry_create(): create a device entry for @func_id
 *
 * @dma_device_index:  DMA device identifier that this device belongs to
 * @func_id:     device identification id
 *
 * A device entry is to be created on every function probe.
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_dev_entry_create(uint32_t dma_device_index, uint16_t func_id);

/*****************************************************************************/
/**
 * qdma_dev_entry_destroy(): destroy device entry for @func_id
 *
 * @dma_device_index:  DMA device identifier that this device belongs to
 * @func_id:     device identification id
 *
 * Return:	None
 *****************************************************************************/
void qdma_dev_entry_destroy(uint32_t dma_device_index, uint16_t func_id);

/*****************************************************************************/
/**
 * qdma_dev_update(): update qmax for the device
 *
 * @dma_device_index: DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @dev_type:    device type
 * @qmax:        qmax for this device
 * @qbase:       output qbase for this device
 *
 * This API is to be called for update request of qmax of any function.
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_dev_update(uint32_t dma_device_index, uint16_t func_id,
		    uint32_t qmax, int *qbase);

/*****************************************************************************/
/**
 * qdma_dev_qinfo_get(): get device info
 *
 * @dma_device_index: DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @dev_type:    device type
 * @qmax:        output qmax for this device
 * @qbase:       output qbase for this device
 *
 * This API can be used get the qbase and qmax for any function
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_dev_qinfo_get(uint32_t dma_device_index, uint16_t func_id,
		       int *qbase, uint32_t *qmax);

/*****************************************************************************/
/**
 * qdma_dev_is_queue_in_range(): check if queue belongs to this device
 *
 * @dma_device_index:  DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @qid_hw:      hardware queue id
 *
 * This API checks if the queue ID is in valid range for function specified
 *
 * Return:	@QDMA_DEV_Q_IN_RANGE  : valid and
 * @QDMA_DEV_Q_OUT_OF_RANGE: invalid
 *****************************************************************************/
enum qdma_dev_q_range qdma_dev_is_queue_in_range(uint32_t dma_device_index,
						 uint16_t func_id,
						 uint32_t qid_hw);

/*****************************************************************************/
/**
 * qdma_dev_increment_active_queue(): increment active queue count
 *
 * @dma_device_index: DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @q_type:      Queue type i.e. C2H or H2C or CMPT
 *
 * This API is used to increment the active queue count of this function
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_dev_increment_active_queue(uint32_t dma_device_index, uint16_t func_id,
				    enum qdma_dev_q_type q_type);

/*****************************************************************************/
/**
 * qdma_dev_decrement_active_queue(): increment active queue count
 *
 * @dma_device_index: DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @q_type:      Queue type i.e. C2H or H2C or CMPT
 *
 * This API is used to increment the active queue count of this function
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_dev_decrement_active_queue(uint32_t dma_device_index, uint16_t func_id,
				    enum qdma_dev_q_type q_type);

/*****************************************************************************/
/**
 * qdma_is_active_queue(): check if any queue is active
 *
 * @dma_device_index:  DMA device identifier that this resource belongs to
 *
 * This API is used to check if any active queue is present.
 *
 * Return:	active queue count
 *****************************************************************************/
uint32_t qdma_get_active_queue_count(uint32_t dma_device_index);

/*****************************************************************************/
/**
 * qdma_get_device_active_queue_count(): get device active queue count
 *
 * @dma_device_index: DMA device identifier that this device belongs to
 * @func_id:     device identification id
 * @q_type:      Queue type i.e. C2H or H2C or CMPT
 *
 * This API is used to get the active queue count of this function
 *
 * Return:	0  : success and < 0: failure
 *****************************************************************************/
int qdma_get_device_active_queue_count(uint32_t dma_device_index,
					uint16_t func_id,
					enum qdma_dev_q_type q_type);

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_RESOURCE_MGMT_H_ */
