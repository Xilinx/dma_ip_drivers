/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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


#include <stdint.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <rte_memzone.h>
#include <rte_string_fns.h>
#include <rte_ethdev_pci.h>
#include <rte_malloc.h>
#include <rte_dev.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_alarm.h>
#include <rte_cycles.h>
#include <unistd.h>
#include <string.h>

#include "qdma.h"
#include "qdma_access.h"
#include "rte_pmd_qdma.h"

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_bar_details
 * Description:		Returns the BAR indices of the QDMA BARs
 *
 * @param	portid : Port ID
 * @param	config_bar_idx : Config BAR index
 * @param	user_bar_idx   : User BAR index
 * @param	bypass_bar_idx : Bypass BAR index
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note   None.
 ******************************************************************************/
int rte_pmd_qdma_get_bar_details(int portid, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *dma_priv = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (config_bar_idx != NULL)
		*(config_bar_idx) = dma_priv->config_bar_idx;

	if (user_bar_idx != NULL)
		*(user_bar_idx) = dma_priv->user_bar_idx;

	if (bypass_bar_idx != NULL)
		*(bypass_bar_idx) = dma_priv->bypass_bar_idx;

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_queue_base
 * Description:		Returns queue base for given port
 *
 * @param	portid : Port ID.
 * @param	queue_base : queue base.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Application can call this API only after successful
 *          call to rte_eh_dev_configure() API.
 ******************************************************************************/
int rte_pmd_qdma_get_queue_base(int portid, uint32_t *queue_base)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *dma_priv = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (queue_base == NULL) {
		PMD_DRV_LOG(ERR, "Caught NULL pointer for queue base\n");
		return -EINVAL;
	}

	*(queue_base) = dma_priv->queue_base;

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_pci_func_type
 * Description:		Retrieves pci function type i.e. PF or VF
 *
 * @param	portid : Port ID.
 * @param	func_type : Indicates pci function type.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Returns the PCIe function type i.e. PF or VF of the given port.
 ******************************************************************************/
int rte_pmd_qdma_get_pci_func_type(int portid,
		enum rte_pmd_qdma_pci_func_type *func_type)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *dma_priv = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (func_type == NULL) {
		PMD_DRV_LOG(ERR, "Caught NULL pointer for function type\n");
		return -EINVAL;
	}


	*((enum rte_pmd_qdma_pci_func_type *)func_type) = (dma_priv->is_vf) ?
			RTE_PMD_QDMA_PCI_FUNC_VF : RTE_PMD_QDMA_PCI_FUNC_PF;

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_immediate_data_state
 * Description:		Returns immediate data state
 *			i.e. whether enabled or disabled, for the specified
 *			queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	state : Pointer to the state specifying whether
 *			immediate data is enabled or not
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this function after
 *		rte_eth_tx_queue_setup() or
 *		rte_eth_rx_queue_setup() is called.
 *		API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_get_immediate_data_state(int portid, uint32_t qid,
		int *state)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(ERR, "Invalid Q-id passed qid %d max en_qid %d\n",
				qid, dev->data->nb_rx_queues);
		return -EINVAL;
	}

	if (state == NULL) {
		PMD_DRV_LOG(ERR, "Invalid state for qid %d\n", qid);
		return -EINVAL;
	}

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (rxq != NULL) {
			*((int *)state) = rxq->dump_immediate_data;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}

	} else {
		PMD_DRV_LOG(ERR, "Qid %d is not setup in Streaming mode\n",
				qid);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_queue_mode
 * Description:		Sets queue mode for the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	mode : Queue mode to be set
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before
 *		rte_eth_tx_queue_setup/rte_eth_rx_queue_setup() API.
 *		By default, all queues are setup in streaming mode.
 ******************************************************************************/
int rte_pmd_qdma_set_queue_mode(int portid, uint32_t qid,
		enum rte_pmd_qdma_queue_mode_t mode)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed, queue ID = %d\n",
					qid);
		return -EINVAL;
	}

	if (mode >= RTE_PMD_QDMA_QUEUE_MODE_MAX) {
		PMD_DRV_LOG(ERR, "Invalid Queue mode passed,Mode = %d\n", mode);
		return -EINVAL;
	}

	if (qdma_dev->q_info != NULL)
		qdma_dev->q_info[qid].queue_mode = mode;
	else {
		PMD_DRV_LOG(ERR, "Queue Info is not valid for queue ID %d\n",
				qid);
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************/
/**
 *Function Name:	rte_pmd_qdma_set_immediate_data_state
 *Description:		Sets immediate data state
 *			i.e. enable or disable, for the specified queue.
 *			If enabled, the user defined data in the completion
 *			ring are dumped in to a queue specific file
 *			"q_<qid>_immmediate_data.txt" in the local directory.
 *
 *@param	portid : Port ID.
 *@param	qid : Queue ID.
 *@param	value :	Immediate data state to be set
 *			Set '0' to disable and '1' to enable
 *
 *@return	'0' on success and '< 0' on failure.
 *
 *@note		Application can call this API after successful
 *		call to rte_eth_rx_queue_setup() API.
 *		This API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_set_immediate_data_state(int portid, uint32_t qid,
		uint8_t state)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for %s,"
				"Queue ID = %d\n", __func__, qid);
		return -EINVAL;
	}

	if (state > 1)
		return -EINVAL;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (rxq != NULL) {
			rxq->dump_immediate_data = state;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	} else {
		PMD_DRV_LOG(ERR, "Qid %d is not setup in ST mode\n", qid);
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_overflow_check
 * Description:		Enables or disables the overflow check
 *			(whether PIDX is overflowing the CIDX) performed by
 *			QDMA on the completion descriptor ring of specified
 *			queue.
 *
 * @param	portid : Port ID.
 * @param	qid	   : Queue ID.
 * @param	enable :  '1' to enable and '0' to disable the overflow check
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_overflow_check(int portid, uint32_t qid,
		uint8_t enable)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (enable > 1)
		return -EINVAL;

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(ST-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		if (rxq != NULL) {
			rxq->dis_overflow_check = (enable == 1) ? 0 : 1;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	} else {
		if (qid >= qdma_dev->qsets_en) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(MM-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq != NULL) {
			cmptq->dis_overflow_check = (enable == 1) ? 0 : 1;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	}

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_descriptor_size
 * Description:		Configures the completion ring descriptor size
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	size : Descriptor size to be configured
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API
 *		when queue is in streaming mode, and before
 *		rte_pmd_qdma_dev_cmptq_setup when queue is in memory mapped
 *		mode.
 *		By default, the completion desciptor size is set to 8 bytes.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_descriptor_size(int portid, uint32_t qid,
		enum rte_pmd_qdma_cmpt_desc_len size)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(ST-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
	} else {
		if (qid >= qdma_dev->qsets_en) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(MM-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
	}

	if (size != RTE_PMD_QDMA_CMPT_DESC_LEN_8B &&
			size != RTE_PMD_QDMA_CMPT_DESC_LEN_16B &&
			size != RTE_PMD_QDMA_CMPT_DESC_LEN_32B &&
			size != RTE_PMD_QDMA_CMPT_DESC_LEN_64B) {
		PMD_DRV_LOG(ERR, "Invalid Size passed for %s, Size = %d\n",
				__func__, size);
		return -EINVAL;
	}

	if (qdma_dev->q_info != NULL)
		qdma_dev->q_info[qid].cmpt_desc_sz = size;
	else {
		PMD_DRV_LOG(ERR, "Queue Info is not valid for queue ID %d\n",
				qid);
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_trigger_mode
 * Description:		Configures the trigger mode for completion ring CIDX
 *			updates
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	mode : Trigger mode to be configured
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 *		By default, trigger mode is set to Counter + Timer.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_trigger_mode(int portid, uint32_t qid,
				enum rte_pmd_qdma_tigger_mode_t mode)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (mode >= RTE_PMD_QDMA_TRIG_MODE_MAX) {
		PMD_DRV_LOG(ERR, "Invalid Trigger mode passed\n");
		return -EINVAL;
	}

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(ST-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		if (rxq != NULL) {
			rxq->triggermode = mode;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}

	} else {
		if (qid >= qdma_dev->qsets_en) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(MM-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq != NULL) {
			cmptq->triggermode = mode;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	}

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_timer
 * Description:		Configures the timer interval in microseconds to trigger
 *			the completion ring CIDX updates
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	value : Timer interval for completion trigger to be configured
 *
 * @return	'0' on success and "<0" on failure.
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_timer(int portid, uint32_t qid, uint32_t value)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;
	int8_t timer_index;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	timer_index = index_of_array(qdma_dev->g_c2h_timer_cnt,
			QDMA_NUM_C2H_TIMERS,
			value);

	if (timer_index < 0) {
		PMD_DRV_LOG(ERR, "Expected timer %d not found\n", value);
		return -ENOTSUP;
	}

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(ST-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		if (rxq != NULL) {
			rxq->timeridx = timer_index;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}

	} else {
		if (qid >= qdma_dev->qsets_en) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(MM-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq != NULL) {
			cmptq->timeridx = timer_index;
		} else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	}

	return 0;
}

/******************************************************************************/
/**
 *Function Name:	rte_pmd_qdma_set_c2h_descriptor_prefetch
 *Description:		Enables or disables prefetch of the descriptors by
 *			prefetch engine
 *
 *@param	portid : Port ID.
 *@param	qid : Queue ID.
 *@param	enable:'1' to enable and '0' to disable the descriptor prefetch
 *
 *@return	'0' on success and '<0' on failure.
 *
 *@note		Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_c2h_descriptor_prefetch(int portid, uint32_t qid,
		uint8_t enable)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s, "
				"Queue ID = %d\n", __func__, qid);
		return -EINVAL;
	}
	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "%s() not supported for qid %d in MM mode",
				__func__, qid);
		return -ENOTSUP;
	}

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	if (rxq != NULL)
		rxq->en_prefetch = (enable > 0) ? 1 : 0;
	else {
		PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
		return -EINVAL;
	}

	return 0;
}

/*****************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_mm_endpoint_addr
 * Description:		Sets the PCIe endpoint memory offset at which to
 *			perform DMA operation for the specified queue operating
 *			in memory mapped mode.
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	dir : direction i.e. TX or RX.
 * @param	addr : Destination address for TX , Source address for RX
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	This API can be called before TX/RX burst API's
 *		(rte_eth_tx_burst() and rte_eth_rx_burst()) are called.
 *****************************************************************************/
int rte_pmd_qdma_set_mm_endpoint_addr(int portid, uint32_t qid,
			enum rte_pmd_qdma_dir_type dir, uint32_t addr)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
				"Queue ID = %d\n", __func__, qid);
		return -EINVAL;
	}

	if (qdma_dev->q_info == NULL) {
		PMD_DRV_LOG(ERR, "Queue Info is not valid for queue ID %d\n",
				qid);
		return -EINVAL;
	}

	if (qdma_dev->q_info[qid].queue_mode !=
		RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Invalid Queue mode for %s, Queue ID = %d,"
				"mode = %d\n", __func__, qid,
				qdma_dev->q_info[qid].queue_mode);
		return -EINVAL;
	}

	if (dir == RTE_PMD_QDMA_TX) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
		if (txq != NULL)
			txq->ep_addr = addr;
		else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	} else if (dir == RTE_PMD_QDMA_RX) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		if (rxq != NULL)
			rxq->ep_addr = addr;
		else {
			PMD_DRV_LOG(ERR, "Qid %d is not setup\n", qid);
			return -EINVAL;
		}
	} else {
		PMD_DRV_LOG(ERR, "Invalid direction specified,"
			"Direction is %d\n", dir);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_tx_bypass
 * Description:		Sets the TX bypass mode and bypass descriptor size
 *			for the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	bypass_mode : Bypass mode to be set
 * @param	size : Bypass descriptor size to be set
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before tx_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_tx_bypass(int portid, uint32_t qid,
		enum rte_pmd_qdma_tx_bypass_mode bypass_mode,
		enum rte_pmd_qdma_bypass_desc_len size)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info == NULL) {
		PMD_DRV_LOG(ERR, "Queue Info is not valid for queue ID %d\n",
				qid);
		return -EINVAL;
	}

	if (qid < dev->data->nb_tx_queues) {
		if (bypass_mode >= RTE_PMD_QDMA_TX_BYPASS_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Tx Bypass mode : %d\n",
					bypass_mode);
			return -EINVAL;
		}

		/*Only 64byte descriptor size supported in 2018.3 Example Design
		 **/
		if ((size  != 0) && (size  != 64))
			return -EINVAL;

		qdma_dev->q_info[qid].tx_bypass_mode = bypass_mode;

		qdma_dev->q_info[qid].tx_bypass_desc_sz = size;
	} else {
		PMD_DRV_LOG(ERR, "Invalid queue ID specified, Queue ID = %d\n",
				qid);
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_rx_bypass
 * Description:		Sets the RX bypass mode and bypass descriptor size for
 *			the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	bypass_mode : Bypass mode to be set
 * @param	size : Bypass descriptor size to be set
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_rx_bypass(int portid, uint32_t qid,
		enum rte_pmd_qdma_rx_bypass_mode bypass_mode,
		enum rte_pmd_qdma_bypass_desc_len size)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info == NULL) {
		PMD_DRV_LOG(ERR, "Queue Info is not valid for queue ID %d\n",
				qid);
		return -EINVAL;
	}

	if (qid < dev->data->nb_rx_queues) {
		if (bypass_mode >= RTE_PMD_QDMA_RX_BYPASS_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Rx Bypass mode : %d\n",
					bypass_mode);
			return -EINVAL;
		}

		/* Only 64byte descriptor size supported in 2018.3 Example
		 * Design
		 */
		if ((size  != 0) && (size  != 64))
			return -EINVAL;

		qdma_dev->q_info[qid].rx_bypass_mode = bypass_mode;

		qdma_dev->q_info[qid].rx_bypass_desc_sz = size;
	} else {
		PMD_DRV_LOG(ERR, "Invalid queue ID specified, Queue ID = %d\n",
				qid);
		return -EINVAL;
	}

	return 0;
}
/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dev_cmptq_setup
 * Description:		Allocate and set up a completion queue for
 *			memory mapped mode.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 * @param   nb_cmpt_desc:	Completion queue ring size.
 * @param   socket_id :	The socket_id argument is the socket identifier
 *			in case of NUMA. Its value can be SOCKET_ID_ANY
 *			if there is no NUMA constraint for the DMA memory
 *			allocated for the transmit descriptors of the ring.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() and rte_pmd_qdma_set_queue_mode()
 *		for queues in memory mapped mode.
 ******************************************************************************/

int rte_pmd_qdma_dev_cmptq_setup(int portid, uint32_t cmpt_queue_id,
				 uint16_t nb_cmpt_desc,
				 unsigned int socket_id)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	uint32_t sz;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_cmpt_queue *cmptq = NULL;
	int err;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (cmpt_queue_id >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
				"Queue ID(MM-mode) = %d\n", __func__,
				cmpt_queue_id);
		return -EINVAL;
	}

	if (nb_cmpt_desc == 0) {
		PMD_DRV_LOG(ERR, "Invalid descriptor ring size %d\n",
				nb_cmpt_desc);
		return -EINVAL;
	}

	if (!qdma_dev->dev_cap.mm_cmpt_en) {
		PMD_DRV_LOG(ERR, "Completion Queue support for MM-mode "
					"not enabled");
		return -EINVAL;
	}

	if (!qdma_dev->is_vf)
		qdma_dev_increment_active_queue(pci_dev->addr.bus,
				qdma_dev->pf);
	else {
		err = qdma_dev_notify_qadd(dev, cmpt_queue_id +
				qdma_dev->queue_base);

		if (err < 0)
			return err;
	}

	if (!qdma_dev->init_q_range) {
		if (qdma_dev->is_vf) {
			err = qdma_vf_csr_read(dev);
			if (err < 0)
				goto cmptq_setup_err;
		} else {
			err = qdma_pf_csr_read(dev);
			if (err < 0)
				goto cmptq_setup_err;
		}
		qdma_dev->init_q_range = 1;
	}

	/* Allocate cmpt queue data structure */
	cmptq = rte_zmalloc("QDMA_CmptQ", sizeof(struct qdma_cmpt_queue),
						RTE_CACHE_LINE_SIZE);

	if (!cmptq) {
		PMD_DRV_LOG(ERR, "Unable to allocate structure cmptq of "
				"size %d\n",
				(int)(sizeof(struct qdma_cmpt_queue)));
		err = -ENOMEM;
		goto cmptq_setup_err;
	}

	cmptq->queue_id = cmpt_queue_id;
	cmptq->port_id = dev->data->port_id;
	cmptq->func_id = qdma_dev->pf;
	cmptq->dev = dev;
	cmptq->st_mode = qdma_dev->q_info[cmpt_queue_id].queue_mode;
	cmptq->triggermode = qdma_dev->trigger_mode;
	cmptq->nb_cmpt_desc = nb_cmpt_desc + 1;
	cmptq->cmpt_desc_len = qdma_dev->q_info[cmpt_queue_id].cmpt_desc_sz;

	/* Find completion ring size index */
	cmptq->ringszidx = index_of_array(qdma_dev->g_ring_sz,
			QDMA_NUM_RING_SIZES,
			cmptq->nb_cmpt_desc);
	if (cmptq->ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected completion ring size %d not found\n",
				cmptq->nb_cmpt_desc);
		err = -EINVAL;
		goto cmptq_setup_err;
	}

	/* Find Threshold index */
	cmptq->threshidx = index_of_array(qdma_dev->g_c2h_cnt_th,
						QDMA_NUM_C2H_COUNTERS,
						DEFAULT_MM_CMPT_CNT_THRESHOLD);
	if (cmptq->threshidx < 0) {
		PMD_DRV_LOG(ERR, "Expected Threshold %d not found,"
				" using the value %d at index 0\n",
				DEFAULT_MM_CMPT_CNT_THRESHOLD,
				qdma_dev->g_c2h_cnt_th[0]);
		cmptq->threshidx = 0;
	}

	/* Find Timer index */
	cmptq->timeridx = index_of_array(qdma_dev->g_c2h_timer_cnt,
			QDMA_NUM_C2H_TIMERS,
			qdma_dev->timer_count);
	if (cmptq->timeridx < 0) {
		PMD_DRV_LOG(ERR, "Expected timer %d not found, "
				"using the value %d at index 1\n",
				qdma_dev->timer_count,
				qdma_dev->g_c2h_timer_cnt[1]);
		cmptq->timeridx = 1;
	}

	/* Disable the cmpt over flow check by default
	 * Applcation can test enable/disable via update param before
	 * queue_start is issued
	 */
	cmptq->dis_overflow_check = 0;


	/* Allocate memory for completion(CMPT) descriptor ring */
	sz = (cmptq->nb_cmpt_desc) * cmptq->cmpt_desc_len;
	cmptq->cmpt_mz = qdma_zone_reserve(dev, "RxHwCmptRn",
			cmpt_queue_id, sz, socket_id);
	if (!cmptq->cmpt_mz) {
		PMD_DRV_LOG(ERR, "Unable to allocate cmptq->cmpt_mz "
				"of size %d\n", sz);
		err = -ENOMEM;
		goto cmptq_setup_err;
	}
	cmptq->cmpt_ring = (struct mm_cmpt_ring *)cmptq->cmpt_mz->addr;

	/* Write-back status structure */
	cmptq->wb_status = (struct wb_status *)((uint64_t)cmptq->cmpt_ring +
			(((uint64_t)cmptq->nb_cmpt_desc - 1) *
			 cmptq->cmpt_desc_len));
	memset(cmptq->cmpt_ring, 0, sz);
	qdma_dev->cmpt_queues[cmpt_queue_id] = cmptq;
	return 0;

cmptq_setup_err:
	if (!qdma_dev->is_vf)
		qdma_dev_decrement_active_queue(pci_dev->addr.bus,
				qdma_dev->pf);
	else
		qdma_dev_notify_qdel(dev, cmpt_queue_id +
				qdma_dev->queue_base);
	if (cmptq) {
		if (cmptq->cmpt_mz)
			rte_memzone_free(cmptq->cmpt_mz);
		rte_free(cmptq);
	}
	return err;
}

static int qdma_vf_cmptq_context_write(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t qid_hw;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct mbox_descq_conf descq_conf;
	int rv;
	struct qdma_cmpt_queue *cmptq;
	uint8_t cmpt_desc_fmt;

	if (!m)
		return -ENOMEM;
	memset(&descq_conf, 0, sizeof(struct mbox_descq_conf));
	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
	qid_hw =  qdma_dev->queue_base + cmptq->queue_id;

	switch (cmptq->cmpt_desc_len) {
	case RTE_PMD_QDMA_CMPT_DESC_LEN_8B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_16B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_16B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_32B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_32B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_64B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_64B;
		break;
	default:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	}

	descq_conf.irq_arm = 0;
	descq_conf.at = 0;
	descq_conf.wbk_en = 1;
	descq_conf.irq_en = 0;
	descq_conf.desc_sz = SW_DESC_CNTXT_MEMORY_MAP_DMA;
	descq_conf.forced_en = 1;
	descq_conf.cmpt_ring_bs_addr = cmptq->cmpt_mz->phys_addr;
	descq_conf.cmpt_desc_sz = cmpt_desc_fmt;
	descq_conf.triggermode = cmptq->triggermode;
	descq_conf.cmpt_at = 0;
	descq_conf.cmpt_color = CMPT_DEFAULT_COLOR_BIT;
	descq_conf.cmpt_full_upd = 0;
	descq_conf.cnt_thres = qdma_dev->g_c2h_cnt_th[cmptq->threshidx];
	descq_conf.timer_thres = qdma_dev->g_c2h_timer_cnt[cmptq->timeridx];
	descq_conf.cmpt_ringsz = qdma_dev->g_ring_sz[cmptq->ringszidx] - 1;
	descq_conf.cmpt_int_en = 0;
	descq_conf.cmpl_stat_en = 1; /* Enable stats for MM-CMPT */
	descq_conf.dis_overflow_check = cmptq->dis_overflow_check;
	descq_conf.func_id = cmptq->func_id;

	qdma_mbox_compose_vf_qctxt_write(cmptq->func_id, qid_hw,
						cmptq->st_mode, 1,
						QDMA_MBOX_CMPT_CTXT_ONLY,
						&descq_conf, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		PMD_DRV_LOG(ERR, "%x, qid_hw 0x%x, mbox failed %d.\n",
				qdma_dev->pf, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

	cmptq->cmpt_cidx_info.counter_idx = cmptq->threshidx;
	cmptq->cmpt_cidx_info.timer_idx = cmptq->timeridx;
	cmptq->cmpt_cidx_info.trig_mode = cmptq->triggermode;
	cmptq->cmpt_cidx_info.wrb_en = 1;
	cmptq->cmpt_cidx_info.wrb_cidx = 0;
	qdma_queue_cmpt_cidx_update(dev, qdma_dev->is_vf,
			qid, &cmptq->cmpt_cidx_info);
err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_pf_cmptq_context_write(struct rte_eth_dev *dev, uint32_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	uint32_t queue_base =  qdma_dev->queue_base;
	uint8_t cmpt_desc_fmt;
	int err = 0;
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;

	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
	memset(&q_cmpt_ctxt, 0, sizeof(struct qdma_descq_cmpt_ctxt));
	/* Clear Completion Context */
	qdma_cmpt_context_clear(dev, qid);

	switch (cmptq->cmpt_desc_len) {
	case RTE_PMD_QDMA_CMPT_DESC_LEN_8B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_16B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_16B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_32B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_32B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_64B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_64B;
		break;
	default:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	}

	if (err != 0)
		return err;

	q_cmpt_ctxt.en_stat_desc = 1;
	q_cmpt_ctxt.trig_mode = cmptq->triggermode;
	q_cmpt_ctxt.fnc_id = cmptq->func_id;
	q_cmpt_ctxt.counter_idx = cmptq->threshidx;
	q_cmpt_ctxt.timer_idx = cmptq->timeridx;
	q_cmpt_ctxt.color = CMPT_DEFAULT_COLOR_BIT;
	q_cmpt_ctxt.ringsz_idx = cmptq->ringszidx;
	q_cmpt_ctxt.bs_addr = (uint64_t)cmptq->cmpt_mz->phys_addr;
	q_cmpt_ctxt.desc_sz = cmpt_desc_fmt;
	q_cmpt_ctxt.valid = 1;
	q_cmpt_ctxt.ovf_chk_dis = cmptq->dis_overflow_check;

	/* Set Completion Context */
	err = qdma_cmpt_context_write(dev, (qid + queue_base), &q_cmpt_ctxt);
	if (err != QDMA_SUCCESS)
		return -1;

	cmptq->cmpt_cidx_info.counter_idx = cmptq->threshidx;
	cmptq->cmpt_cidx_info.timer_idx = cmptq->timeridx;
	cmptq->cmpt_cidx_info.trig_mode = cmptq->triggermode;
	cmptq->cmpt_cidx_info.wrb_en = 1;
	cmptq->cmpt_cidx_info.wrb_cidx = 0;
	qdma_queue_cmpt_cidx_update(dev, qdma_dev->is_vf,
			qid, &cmptq->cmpt_cidx_info);

	return 0;
}

/******************************************************************************/
/*
 * Function Name:   rte_pmd_qdma_dev_cmptq_start
 * Description:     Start the MM completion queue.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_pmd_qdma_dev_cmptq_setup() API when queue is in
 *		memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_start(int portid, uint32_t qid)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not configured in MM-mode\n", qid);
		return -EINVAL;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
				"Queue ID(MM-mode) = %d\n", __func__,
				qid);
		return -EINVAL;
	}

	if (qdma_dev->is_vf)
		return qdma_vf_cmptq_context_write(dev, qid);
	else
		return qdma_pf_cmptq_context_write(dev, qid);
}

static int qdma_pf_cmptq_context_invalidate(struct rte_eth_dev *dev,
		uint32_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	uint32_t sz, i = 0;

	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
	qdma_cmpt_context_invalidate(dev, (qid + qdma_dev->queue_base));

	/* Zero the cmpt-ring entries*/
	sz = cmptq->cmpt_desc_len;
	for (i = 0; i < (sz * cmptq->nb_cmpt_desc); i++)
		((volatile char *)cmptq->cmpt_ring)[i] = 0;
	return 0;
}

static int qdma_vf_cmptq_context_invalidate(struct rte_eth_dev *dev,
						uint32_t qid)
{
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t qid_hw;
	int rv;

	if (!m)
		return -ENOMEM;

	qid_hw = qdma_dev->queue_base + qid;
	qdma_mbox_compose_vf_qctxt_invalidate(qdma_dev->pf, qid_hw,
					      0, 0, QDMA_MBOX_CMPT_CTXT_ONLY,
					      m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		if (rv != -ENODEV)
			PMD_DRV_LOG(INFO, "%x, qid_hw 0x%x mbox failed %d.\n",
				    qdma_dev->pf, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

/******************************************************************************/
/*
 * Function Name:   rte_pmd_qdma_dev_cmptq_stop
 * Description:     Stop the MM completion queue.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_pmd_qdma_dev_cmptq_start() API when queue is in
 *		memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_stop(int portid, uint32_t qid)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not configured in MM-mode\n", qid);
		return -EINVAL;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
				"Queue ID(MM-mode) = %d\n", __func__,
				qid);
		return -EINVAL;
	}

	if (qdma_dev->is_vf)
		return qdma_vf_cmptq_context_invalidate(dev, qid);
	else
		return qdma_pf_cmptq_context_invalidate(dev, qid);
}

/*****************************************************************************/
/**
 * Function Name:   rte_pmd_qdma_mm_cmpt_process
 * Description:     Process the MM Completion queue entries.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 * @param   cmpt_buff : User buffer pointer to store the completion data.
 * @param   nb_entries: Number of compeltion entries to process.
 *
 * @return  'number of entries processed' on success and '< 0' on failure.
 *
 * @note    Application can call this API after successful call to
 *	    rte_pmd_qdma_dev_cmptq_start() API.
 ******************************************************************************/

uint16_t rte_pmd_qdma_mm_cmpt_process(int portid, uint32_t qid,
					void *cmpt_buff, uint16_t nb_entries)
{
	struct rte_eth_dev *dev = &rte_eth_devices[portid];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	uint32_t count = 0;
	struct mm_cmpt_ring *cmpt_ring;
	struct wb_status *wb_status;
	uint16_t nb_entries_avail = 0;
	uint16_t cmpt_tail = 0;
	uint16_t cmpt_pidx;
	char *cmpt_buff_ptr;

	if (portid < 0 || portid >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", portid);
		return -ENOTSUP;
	}

	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not configured in MM-mode\n", qid);
		return -EINVAL;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
				"Queue ID(MM-mode) = %d\n", __func__,
				qid);
		return -EINVAL;
	}

	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];

	if (cmpt_buff == NULL) {
		PMD_DRV_LOG(ERR, "Invalid user buffer pointer from user");
		return 0;
	}

	wb_status = cmptq->wb_status;
	cmpt_tail = cmptq->cmpt_cidx_info.wrb_cidx;
	cmpt_pidx = wb_status->pidx;

	if (cmpt_tail < cmpt_pidx)
		nb_entries_avail = cmpt_pidx - cmpt_tail;
	else if (cmpt_tail > cmpt_pidx)
		nb_entries_avail = cmptq->nb_cmpt_desc - 1 - cmpt_tail +
			cmpt_pidx;

	if (nb_entries_avail == 0) {
		PMD_DRV_LOG(DEBUG, "%s(): %d: nb_entries_avail = 0\n",
				__func__, __LINE__);
		return 0;
	}

	if (nb_entries > nb_entries_avail)
		nb_entries = nb_entries_avail;

	while (count < nb_entries) {
		cmpt_ring = (struct mm_cmpt_ring *)((uint64_t)cmptq->cmpt_ring +
				((uint64_t)cmpt_tail * cmptq->cmpt_desc_len));

		if (unlikely(cmpt_ring->err || cmpt_ring->data_frmt)) {
			PMD_DRV_LOG(ERR, "Error detected on CMPT ring at "
					"index %d, queue_id = %d\n",
					cmpt_tail,
					cmptq->queue_id);
			return 0;
		}

		uint16_t i = 0;
		cmpt_buff_ptr = (char *)cmpt_buff;
		*(cmpt_buff_ptr) = (*((uint8_t *)cmpt_ring) & 0xF0);
		for (i = 1; i < (cmptq->cmpt_desc_len); i++)
			*(cmpt_buff_ptr + i) = (*((uint8_t *)cmpt_ring + i));
		cmpt_tail++;
		if (unlikely(cmpt_tail >= (cmptq->nb_cmpt_desc - 1)))
			cmpt_tail -= (cmptq->nb_cmpt_desc - 1);
		count++;
	}

	// Update the CPMT CIDX
	cmptq->cmpt_cidx_info.wrb_cidx = cmpt_tail;
	qdma_queue_cmpt_cidx_update(cmptq->dev, qdma_dev->is_vf,
			cmptq->queue_id,
			&cmptq->cmpt_cidx_info);
	return count;
}
