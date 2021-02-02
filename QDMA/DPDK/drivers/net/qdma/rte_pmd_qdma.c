/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019-2021 Xilinx, Inc. All rights reserved.
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
#include "qdma_access_common.h"
#include "rte_pmd_qdma.h"
#include "qdma_devops.h"


static int validate_qdma_dev_info(int port_id, uint16_t qid)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed, queue ID = %d\n",
					qid);
		return -EINVAL;
	}

	if (!qdma_dev->dev_configured) {
		PMD_DRV_LOG(ERR,
			"Device for port id %d is not configured yet\n",
			port_id);
		return -EINVAL;
	}

	return 0;
}

static int8_t qdma_get_trigger_mode(enum rte_pmd_qdma_tigger_mode_t mode)
{
	int8_t ret;
	switch (mode) {
	case RTE_PMD_QDMA_TRIG_MODE_DISABLE:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_DIS;
		break;
	case RTE_PMD_QDMA_TRIG_MODE_EVERY:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_EVERY;
		break;
	case RTE_PMD_QDMA_TRIG_MODE_USER_COUNT:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_USR_CNT;
		break;
	case RTE_PMD_QDMA_TRIG_MODE_USER:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_USR;
		break;
	case RTE_PMD_QDMA_TRIG_MODE_USER_TIMER:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_USR_TMR;
		break;
	case RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_TMR_CNTR;
		break;
	default:
		ret = QDMA_CMPT_UPDATE_TRIG_MODE_USR_TMR;
		break;
	}
	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_bar_details
 * Description:		Returns the BAR indices of the QDMA BARs
 *
 * @param	port_id : Port ID
 * @param	config_bar_idx : Config BAR index
 * @param	user_bar_idx   : AXI Master Lite BAR(user bar) index
 * @param	bypass_bar_idx : AXI Bridge Master BAR(bypass bar) index
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note   None.
 ******************************************************************************/
int rte_pmd_qdma_get_bar_details(int port_id, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *dma_priv;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	dma_priv = dev->data->dev_private;
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
 * @param	port_id : Port ID.
 * @param	queue_base : queue base.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Application can call this API only after successful
 *          call to rte_eh_dev_configure() API.
 ******************************************************************************/
int rte_pmd_qdma_get_queue_base(int port_id, uint32_t *queue_base)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *dma_priv;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	dma_priv = dev->data->dev_private;
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
 * @param	port_id : Port ID.
 * @param	func_type : Indicates pci function type.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Returns the PCIe function type i.e. PF or VF of the given port.
 ******************************************************************************/
int rte_pmd_qdma_get_pci_func_type(int port_id,
		enum rte_pmd_qdma_pci_func_type *func_type)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *dma_priv;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	dma_priv = dev->data->dev_private;
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
 * @param	port_id : Port ID.
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
int rte_pmd_qdma_get_immediate_data_state(int port_id, uint32_t qid,
		int *state)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_rx_queue *rxq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qid >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(ERR, "Invalid Q-id passed qid %d max en_qid %d\n",
				qid, dev->data->nb_rx_queues);
		return -EINVAL;
	}

	if (state == NULL) {
		PMD_DRV_LOG(ERR, "Invalid state for qid %d\n", qid);
		return -EINVAL;
	}

	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
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
	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_queue_mode
 * Description:		Sets queue mode for the specified queue
 *
 * @param	port_id : Port ID.
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
int rte_pmd_qdma_set_queue_mode(int port_id, uint32_t qid,
		enum rte_pmd_qdma_queue_mode_t mode)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (mode >= RTE_PMD_QDMA_QUEUE_MODE_MAX) {
		PMD_DRV_LOG(ERR, "Invalid Queue mode passed,Mode = %d\n", mode);
		return -EINVAL;
	}

	qdma_dev->q_info[qid].queue_mode = mode;

	return ret;
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
 * @param	port_id : Port ID.
 * @param	qid : Queue ID.
 * @param	value :	Immediate data state to be set
 *			Set '0' to disable and '1' to enable
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful
 *		call to rte_eth_dev_configure() API. Application can
 *		also call this API after successful call to
 *		rte_eth_rx_queue_setup() only if rx queue is not in
 *		start state. This API is applicable for
 *		streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_set_immediate_data_state(int port_id, uint32_t qid,
		uint8_t state)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_rx_queue *rxq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qid >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for %s,"
				"Queue ID = %d\n", __func__, qid);
		return -EINVAL;
	}

	if (state > 1) {
		PMD_DRV_LOG(ERR, "Invalid value specified for immediate data "
				"state %s, Queue ID = %d\n", __func__, qid);
		return -EINVAL;
	}

	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_STREAMING_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not setup in ST mode\n", qid);
		return -EINVAL;
	}

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	if (rxq == NULL) {
		/* Update the configuration in q_info structure
		 * if rx queue is not setup.
		 */
		qdma_dev->q_info[qid].immediate_data_state = state;
	} else if (dev->data->rx_queue_state[qid] ==
			RTE_ETH_QUEUE_STATE_STOPPED) {
		/* Update the config in both q_info and rxq structures,
		 * only if rx queue is setup but not yet started.
		 */
		qdma_dev->q_info[qid].immediate_data_state = state;
		rxq->dump_immediate_data = state;
	} else {
		PMD_DRV_LOG(ERR,
			"Cannot configure when Qid %d is in start state\n",
			qid);
		return -EINVAL;
	}

	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_overflow_check
 * Description:		Enables or disables the overflow check
 *			(whether PIDX is overflowing the CIDX) performed by
 *			QDMA on the completion descriptor ring of specified
 *			queue.
 *
 * @param	port_id : Port ID.
 * @param	qid	   : Queue ID.
 * @param	enable :  '1' to enable and '0' to disable the overflow check
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() API. Application can also call this
 *		API after successful call to rte_eth_rx_queue_setup()/
 *		rte_pmd_qdma_dev_cmptq_setup() API only if
 *		rx/cmpt queue is not in start state.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_overflow_check(int port_id, uint32_t qid,
		uint8_t enable)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (enable > 1)
		return -EINVAL;

	if (!qdma_dev->dev_cap.cmpt_ovf_chk_dis) {
		PMD_DRV_LOG(ERR, "%s: Completion overflow check disable is "
			"not supported in the current design\n", __func__);
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
		if (rxq == NULL) {
			/* Update the configuration in q_info structure
			 * if rx queue is not setup.
			 */
			qdma_dev->q_info[qid].dis_cmpt_ovf_chk =
					(enable == 1) ? 0 : 1;
		} else if (dev->data->rx_queue_state[qid] ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the config in both q_info and rxq structures,
			 * only if rx queue is setup but not yet started.
			 */
			qdma_dev->q_info[qid].dis_cmpt_ovf_chk =
					(enable == 1) ? 0 : 1;
			rxq->dis_overflow_check =
					qdma_dev->q_info[qid].dis_cmpt_ovf_chk;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	} else {
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq == NULL) {
			/* Update the configuration in q_info structure
			 * if cmpt queue is not setup.
			 */
			qdma_dev->q_info[qid].dis_cmpt_ovf_chk =
					(enable == 1) ? 0 : 1;
		} else if (cmptq->status ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the configuration in both q_info and cmptq
			 * structures if cmpt queue is already setup.
			 */
			qdma_dev->q_info[qid].dis_cmpt_ovf_chk =
					(enable == 1) ? 0 : 1;
			cmptq->dis_overflow_check =
					qdma_dev->q_info[qid].dis_cmpt_ovf_chk;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	}

	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_descriptor_size
 * Description:		Configures the completion ring descriptor size
 *
 * @param	port_id : Port ID.
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
int rte_pmd_qdma_set_cmpt_descriptor_size(int port_id, uint32_t qid,
		enum rte_pmd_qdma_cmpt_desc_len size)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qdma_dev->q_info[qid].queue_mode ==
			RTE_PMD_QDMA_STREAMING_MODE) {
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid Queue id passed for %s,"
					"Queue ID(ST-mode) = %d\n", __func__,
					qid);
			return -EINVAL;
		}
	}

	if (size != RTE_PMD_QDMA_CMPT_DESC_LEN_8B &&
			size != RTE_PMD_QDMA_CMPT_DESC_LEN_16B &&
			size != RTE_PMD_QDMA_CMPT_DESC_LEN_32B &&
			(size != RTE_PMD_QDMA_CMPT_DESC_LEN_64B ||
			!qdma_dev->dev_cap.cmpt_desc_64b)) {

		PMD_DRV_LOG(ERR, "Invalid Size passed for %s, Size = %d\n",
				__func__, size);
		return -EINVAL;
	}

	qdma_dev->q_info[qid].cmpt_desc_sz = size;

	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_trigger_mode
 * Description:		Configures the trigger mode for completion ring CIDX
 *			updates
 *
 * @param	port_id : Port ID.
 * @param	qid : Queue ID.
 * @param	mode : Trigger mode to be configured
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful
 *		call to rte_eth_dev_configure() API. Application can
 *		also call this API after successful call to
 *		rte_eth_rx_queue_setup()/rte_pmd_qdma_dev_cmptq_setup()
 *		API only if rx/cmpt queue is not in start state.
 *		By default, trigger mode is set to Counter + Timer.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_trigger_mode(int port_id, uint32_t qid,
				enum rte_pmd_qdma_tigger_mode_t mode)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (mode >= RTE_PMD_QDMA_TRIG_MODE_MAX) {
		PMD_DRV_LOG(ERR, "Invalid Trigger mode passed\n");
		return -EINVAL;
	}

	if ((mode == RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT) &&
		!qdma_dev->dev_cap.cmpt_trig_count_timer) {
		PMD_DRV_LOG(ERR, "%s: Trigger mode %d is "
			"not supported in the current design\n",
			__func__, mode);
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
		if (rxq == NULL) {
			/* Update the configuration in q_info structure
			 * if rx queue is not setup.
			 */
			qdma_dev->q_info[qid].trigger_mode =
					qdma_get_trigger_mode(mode);
		} else if (dev->data->rx_queue_state[qid] ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the config in both q_info and rxq structures,
			 * only if rx queue is setup but not yet started.
			 */
			qdma_dev->q_info[qid].trigger_mode =
					qdma_get_trigger_mode(mode);
			rxq->triggermode = qdma_dev->q_info[qid].trigger_mode;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	} else if (qdma_dev->dev_cap.mm_cmpt_en) {
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq == NULL) {
			/* Update the configuration in q_info structure
			 * if cmpt queue is not setup.
			 */
			qdma_dev->q_info[qid].trigger_mode =
					qdma_get_trigger_mode(mode);
		} else if (cmptq->status ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the configuration in both q_info and cmptq
			 * structures if cmpt queue is already setup.
			 */
			qdma_dev->q_info[qid].trigger_mode =
					qdma_get_trigger_mode(mode);
			cmptq->triggermode =
					qdma_dev->q_info[qid].trigger_mode;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	} else {
		PMD_DRV_LOG(ERR, "Unable to set trigger mode for %s,"
					"Queue ID = %d, Queue Mode = %d\n",
					__func__,
					qid, qdma_dev->q_info[qid].queue_mode);
	}
	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_timer
 * Description:		Configures the timer interval in microseconds to trigger
 *			the completion ring CIDX updates
 *
 * @param	port_id : Port ID.
 * @param	qid : Queue ID.
 * @param	value : Timer interval for completion trigger to be configured
 *
 * @return	'0' on success and "<0" on failure.
 *
 * @note	Application can call this API after successful
 *		call to rte_eth_dev_configure() API. Application can
 *		also call this API after successful call to
 *		rte_eth_rx_queue_setup()/rte_pmd_qdma_dev_cmptq_setup() API
 *		only if rx/cmpt queue is not in start state.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_timer(int port_id, uint32_t qid, uint32_t value)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_cmpt_queue *cmptq;
	struct qdma_rx_queue *rxq;
	int8_t timer_index;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
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
		if (rxq == NULL) {
			/* Update the configuration in q_info structure
			 * if rx queue is not setup.
			 */
			qdma_dev->q_info[qid].timer_count = value;
		} else if (dev->data->rx_queue_state[qid] ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the config in both q_info and rxq structures,
			 * only if rx queue is setup but not yet started.
			 */
			qdma_dev->q_info[qid].timer_count = value;
			rxq->timeridx = timer_index;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	} else if (qdma_dev->dev_cap.mm_cmpt_en) {
		cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
		if (cmptq == NULL) {
			/* Update the configuration in q_info structure
			 * if cmpt queue is not setup.
			 */
			qdma_dev->q_info[qid].timer_count = value;
		} else if (cmptq->status ==
				RTE_ETH_QUEUE_STATE_STOPPED) {
			/* Update the configuration in both q_info and cmptq
			 * structures if cmpt queue is already setup.
			 */
			qdma_dev->q_info[qid].timer_count = value;
			cmptq->timeridx = timer_index;
		} else {
			PMD_DRV_LOG(ERR,
				"Cannot configure when Qid %d is in start state\n",
				qid);
			return -EINVAL;
		}
	} else {
		PMD_DRV_LOG(ERR, "Unable to set trigger mode for %s,"
					"Queue ID = %d, Queue Mode = %d\n",
					__func__,
					qid, qdma_dev->q_info[qid].queue_mode);
	}
	return ret;
}

/******************************************************************************/
/**
 *Function Name:	rte_pmd_qdma_set_c2h_descriptor_prefetch
 *Description:		Enables or disables prefetch of the descriptors by
 *			prefetch engine
 *
 * @param	port_id : Port ID.
 * @param	qid : Queue ID.
 * @param	enable:'1' to enable and '0' to disable the descriptor prefetch
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	Application can call this API after successful
 *		call to rte_eth_dev_configure() API. Application can
 *		also call this API after successful call to
 *		rte_eth_rx_queue_setup() API, only if rx queue
 *		is not in start state.
 ******************************************************************************/
int rte_pmd_qdma_set_c2h_descriptor_prefetch(int port_id, uint32_t qid,
		uint8_t enable)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_rx_queue *rxq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
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

	if (rxq == NULL) {
		/* Update the configuration in q_info structure
		 * if rx queue is not setup.
		 */
		qdma_dev->q_info[qid].en_prefetch = (enable > 0) ? 1 : 0;
	} else if (dev->data->rx_queue_state[qid] ==
			RTE_ETH_QUEUE_STATE_STOPPED) {
		/* Update the config in both q_info and rxq structures,
		 * only if rx queue is setup but not yet started.
		 */
		qdma_dev->q_info[qid].en_prefetch = (enable > 0) ? 1 : 0;
		rxq->en_prefetch = qdma_dev->q_info[qid].en_prefetch;
	} else {
		PMD_DRV_LOG(ERR,
			"Cannot configure when Qid %d is in start state\n",
			qid);
		return -EINVAL;
	}

	return ret;
}

/*****************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_mm_endpoint_addr
 * Description:		Sets the PCIe endpoint memory offset at which to
 *			perform DMA operation for the specified queue operating
 *			in memory mapped mode.
 *
 * @param	port_id : Port ID.
 * @param	qid : Queue ID.
 * @param	dir : direction i.e. TX or RX.
 * @param	addr : Destination address for TX , Source address for RX
 *
 * @return	'0' on success and '<0' on failure.
 *
 * @note	This API can be called before TX/RX burst API's
 *		(rte_eth_tx_burst() and rte_eth_rx_burst()) are called.
 *****************************************************************************/
int rte_pmd_qdma_set_mm_endpoint_addr(int port_id, uint32_t qid,
			enum rte_pmd_qdma_dir_type dir, uint32_t addr)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
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
	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_tx_bypass
 * Description:		Sets the TX bypass mode and bypass descriptor size
 *			for the specified queue
 *
 * @param	port_id : Port ID.
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
int rte_pmd_qdma_configure_tx_bypass(int port_id, uint32_t qid,
		enum rte_pmd_qdma_tx_bypass_mode bypass_mode,
		enum rte_pmd_qdma_bypass_desc_len size)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qid < dev->data->nb_tx_queues) {
		if (bypass_mode >= RTE_PMD_QDMA_TX_BYPASS_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Tx Bypass mode : %d\n",
					bypass_mode);
			return -EINVAL;
		}
		if (qdma_dev->dev_cap.sw_desc_64b) {
			/*64byte descriptor size supported
			 *in >2018.3 Example Design Only
			 */
			if ((size  != 0) && (size  != 64)) {
				PMD_DRV_LOG(ERR, "%s: Descriptor size %d not supported."
				"64B and internal "
				"mode descriptor sizes (size = 0) "
				"are only supported by the driver in > 2018.3 "
				"example design\n", __func__, size);
				return -EINVAL;
			}
		} else {
			/*In 2018.2 design, internal mode descriptor
			 *sizes are only supported.Hence not allowing
			 *to configure bypass descriptor size.
			 *Size 0 indicates internal mode descriptor size.
			 */
			if (size  != 0) {
				PMD_DRV_LOG(ERR, "%s: Descriptor size %d not supported.Only "
				"Internal mode descriptor sizes (size = 0)"
				"are supported in the current design.\n",
				__func__, size);
				return -EINVAL;
			}
		}
		qdma_dev->q_info[qid].tx_bypass_mode = bypass_mode;

		qdma_dev->q_info[qid].tx_bypass_desc_sz = size;
	} else {
		PMD_DRV_LOG(ERR, "Invalid queue ID specified, Queue ID = %d\n",
				qid);
		return -EINVAL;
	}

	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_rx_bypass
 * Description:		Sets the RX bypass mode and bypass descriptor size for
 *			the specified queue
 *
 * @param	port_id : Port ID.
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
int rte_pmd_qdma_configure_rx_bypass(int port_id, uint32_t qid,
		enum rte_pmd_qdma_rx_bypass_mode bypass_mode,
		enum rte_pmd_qdma_bypass_desc_len size)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qid < dev->data->nb_rx_queues) {
		if (bypass_mode >= RTE_PMD_QDMA_RX_BYPASS_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Rx Bypass mode : %d\n",
					bypass_mode);
			return -EINVAL;
		}

		if (qdma_dev->dev_cap.sw_desc_64b) {
			/*64byte descriptor size supported
			 *in >2018.3 Example Design Only
			 */
			if ((size  != 0) && (size  != 64)) {
				PMD_DRV_LOG(ERR, "%s: Descriptor size %d not supported."
				"64B and internal "
				"mode descriptor sizes (size = 0) "
				"are only supported by the driver in > 2018.3 "
				"example design\n", __func__, size);
				return -EINVAL;
			}
		} else {
			/*In 2018.2 design, internal mode descriptor
			 *sizes are only supported.Hence not allowing
			 *to configure bypass descriptor size.
			 *Size 0 indicates internal mode descriptor size.
			 */
			if (size  != 0) {
				PMD_DRV_LOG(ERR, "%s: Descriptor size %d not supported.Only "
				"Internal mode descriptor sizes (size = 0)"
				"are supported in the current design.\n",
				__func__, size);
				return -EINVAL;
			}
		}

		qdma_dev->q_info[qid].rx_bypass_mode = bypass_mode;

		qdma_dev->q_info[qid].rx_bypass_desc_sz = size;
	} else {
		PMD_DRV_LOG(ERR, "Invalid queue ID specified, Queue ID = %d\n",
				qid);
		return -EINVAL;
	}

	return ret;
}

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_device_capabilities
 * Description:		Retrive the device capabilities
 *
 * @param   port_id : Port ID.
 * @param   dev_attr:Pointer to the device capabilities structure
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_get_device_capabilities(int port_id,
		struct rte_pmd_qdma_dev_attributes *dev_attr)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (!is_qdma_supported(dev)) {
		PMD_DRV_LOG(ERR, "Device is not supported\n");
		return -ENOTSUP;
	}

	if (dev_attr == NULL) {
		PMD_DRV_LOG(ERR, "Caught NULL pointer for dev_attr\n");
		return -EINVAL;
	}

	dev_attr->num_pfs = qdma_dev->dev_cap.num_pfs;
	dev_attr->num_qs = qdma_dev->dev_cap.num_qs;
	dev_attr->flr_present = qdma_dev->dev_cap.flr_present;
	dev_attr->st_en = qdma_dev->dev_cap.st_en;
	dev_attr->mm_en = qdma_dev->dev_cap.mm_en;
	dev_attr->mm_cmpt_en = qdma_dev->dev_cap.mm_cmpt_en;
	dev_attr->mailbox_en = qdma_dev->dev_cap.mailbox_en;
	dev_attr->mm_channel_max = qdma_dev->dev_cap.mm_channel_max;
	dev_attr->debug_mode = qdma_dev->dev_cap.debug_mode;
	dev_attr->desc_eng_mode = qdma_dev->dev_cap.desc_eng_mode;
	dev_attr->cmpt_ovf_chk_dis = qdma_dev->dev_cap.cmpt_ovf_chk_dis;
	dev_attr->sw_desc_64b = qdma_dev->dev_cap.sw_desc_64b;
	dev_attr->cmpt_desc_64b = qdma_dev->dev_cap.cmpt_desc_64b;
	dev_attr->cmpt_trig_count_timer =
				qdma_dev->dev_cap.cmpt_trig_count_timer;

	switch (qdma_dev->device_type) {
	case QDMA_DEVICE_SOFT:
		dev_attr->device_type = RTE_PMD_QDMA_DEVICE_SOFT;
		break;
	case QDMA_DEVICE_VERSAL:
		dev_attr->device_type = RTE_PMD_QDMA_DEVICE_VERSAL;
		break;
	default:
		PMD_DRV_LOG(ERR, "%s: Invalid device type "
			"Id = %d\n",	__func__, qdma_dev->device_type);
		return -EINVAL;
	}

	switch (qdma_dev->ip_type) {
	case QDMA_VERSAL_HARD_IP:
		dev_attr->ip_type =
			RTE_PMD_QDMA_VERSAL_HARD_IP;
		break;
	case QDMA_VERSAL_SOFT_IP:
		dev_attr->ip_type =
			RTE_PMD_QDMA_VERSAL_SOFT_IP;
		break;
	case QDMA_SOFT_IP:
		dev_attr->ip_type =
			RTE_PMD_QDMA_SOFT_IP;
		break;
	case EQDMA_SOFT_IP:
		dev_attr->ip_type =
			RTE_PMD_EQDMA_SOFT_IP;
		break;
	default:
		dev_attr->ip_type = RTE_PMD_QDMA_NONE_IP;
		PMD_DRV_LOG(ERR, "%s: Invalid IP type "
			"ip_type = %d\n", __func__,
			qdma_dev->ip_type);
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
 * @param   port_id : Port ID.
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

int rte_pmd_qdma_dev_cmptq_setup(int port_id, uint32_t cmpt_queue_id,
				 uint16_t nb_cmpt_desc,
				 unsigned int socket_id)
{
	struct rte_eth_dev *dev;
	uint32_t sz;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_cmpt_queue *cmptq = NULL;
	int err;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, cmpt_queue_id);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
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

	if (!qdma_dev->is_vf) {
		err = qdma_dev_increment_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
		if (err != QDMA_SUCCESS)
			return -EINVAL;
	} else {
		err = qdma_dev_notify_qadd(dev, cmpt_queue_id +
				qdma_dev->queue_base, QDMA_DEV_Q_TYPE_CMPT);

		if (err < 0) {
			PMD_DRV_LOG(ERR, "%s: Queue addition failed for CMPT Queue ID "
				"%d\n",	__func__, cmpt_queue_id);
			return -EINVAL;
		}
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
	cmptq->func_id = qdma_dev->func_id;
	cmptq->dev = dev;
	cmptq->st_mode = qdma_dev->q_info[cmpt_queue_id].queue_mode;
	cmptq->triggermode = qdma_dev->q_info[cmpt_queue_id].trigger_mode;
	cmptq->nb_cmpt_desc = nb_cmpt_desc + 1;
	cmptq->cmpt_desc_len = qdma_dev->q_info[cmpt_queue_id].cmpt_desc_sz;
	if ((cmptq->cmpt_desc_len == RTE_PMD_QDMA_CMPT_DESC_LEN_64B) &&
		!qdma_dev->dev_cap.cmpt_desc_64b) {
		PMD_DRV_LOG(ERR, "%s: PF-%d(DEVFN) CMPT of 64B is not supported in the "
			"current design\n",  __func__, qdma_dev->func_id);
		return -ENOTSUP;
	}
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
			qdma_dev->q_info[cmpt_queue_id].timer_count);
	if (cmptq->timeridx < 0) {
		PMD_DRV_LOG(ERR, "Expected timer %d not found, "
				"using the value %d at index 1\n",
				qdma_dev->q_info[cmpt_queue_id].timer_count,
				qdma_dev->g_c2h_timer_cnt[1]);
		cmptq->timeridx = 1;
	}

	cmptq->dis_overflow_check =
			qdma_dev->q_info[cmpt_queue_id].dis_cmpt_ovf_chk;

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
	cmptq->cmpt_ring = (struct qdma_ul_cmpt_ring *)cmptq->cmpt_mz->addr;

	/* Write-back status structure */
	cmptq->wb_status = (struct wb_status *)((uint64_t)cmptq->cmpt_ring +
			(((uint64_t)cmptq->nb_cmpt_desc - 1) *
			 cmptq->cmpt_desc_len));
	memset(cmptq->cmpt_ring, 0, sz);
	qdma_dev->cmpt_queues[cmpt_queue_id] = cmptq;
	return ret;

cmptq_setup_err:
	if (!qdma_dev->is_vf)
		qdma_dev_decrement_active_queue(qdma_dev->dma_device_index,
				qdma_dev->func_id, QDMA_DEV_Q_TYPE_CMPT);
	else
		qdma_dev_notify_qdel(dev, cmpt_queue_id +
				qdma_dev->queue_base, QDMA_DEV_Q_TYPE_CMPT);

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
	descq_conf.cmpt_ring_bs_addr = cmptq->cmpt_mz->iova;
	descq_conf.cmpt_desc_sz = cmpt_desc_fmt;
	descq_conf.triggermode = cmptq->triggermode;

	descq_conf.cmpt_color = CMPT_DEFAULT_COLOR_BIT;
	descq_conf.cmpt_full_upd = 0;
	descq_conf.cnt_thres = qdma_dev->g_c2h_cnt_th[cmptq->threshidx];
	descq_conf.timer_thres = qdma_dev->g_c2h_timer_cnt[cmptq->timeridx];
	descq_conf.cmpt_ringsz = qdma_dev->g_ring_sz[cmptq->ringszidx] - 1;
	descq_conf.cmpt_int_en = 0;
	descq_conf.cmpl_stat_en = 1; /* Enable stats for MM-CMPT */

	if (qdma_dev->dev_cap.cmpt_ovf_chk_dis)
		descq_conf.dis_overflow_check = cmptq->dis_overflow_check;

	descq_conf.func_id = cmptq->func_id;

	qdma_mbox_compose_vf_qctxt_write(cmptq->func_id, qid_hw,
						cmptq->st_mode, 1,
						QDMA_MBOX_CMPT_CTXT_ONLY,
						&descq_conf, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		PMD_DRV_LOG(ERR, "%x, qid_hw 0x%x, mbox failed %d.\n",
				qdma_dev->func_id, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

	cmptq->cmpt_cidx_info.counter_idx = cmptq->threshidx;
	cmptq->cmpt_cidx_info.timer_idx = cmptq->timeridx;
	cmptq->cmpt_cidx_info.trig_mode = cmptq->triggermode;
	cmptq->cmpt_cidx_info.wrb_en = 1;
	cmptq->cmpt_cidx_info.wrb_cidx = 0;
	qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(dev, qdma_dev->is_vf,
			qid, &cmptq->cmpt_cidx_info);
	cmptq->status = RTE_ETH_QUEUE_STATE_STARTED;
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
	qdma_dev->hw_access->qdma_cmpt_ctx_conf(dev, qid,
				&q_cmpt_ctxt, QDMA_HW_ACCESS_CLEAR);

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

	q_cmpt_ctxt.en_stat_desc = 1;
	q_cmpt_ctxt.trig_mode = cmptq->triggermode;
	q_cmpt_ctxt.fnc_id = cmptq->func_id;
	q_cmpt_ctxt.counter_idx = cmptq->threshidx;
	q_cmpt_ctxt.timer_idx = cmptq->timeridx;
	q_cmpt_ctxt.color = CMPT_DEFAULT_COLOR_BIT;
	q_cmpt_ctxt.ringsz_idx = cmptq->ringszidx;
	q_cmpt_ctxt.bs_addr = (uint64_t)cmptq->cmpt_mz->iova;
	q_cmpt_ctxt.desc_sz = cmpt_desc_fmt;
	q_cmpt_ctxt.valid = 1;

	if (qdma_dev->dev_cap.cmpt_ovf_chk_dis)
		q_cmpt_ctxt.ovf_chk_dis = cmptq->dis_overflow_check;

	/* Set Completion Context */
	err = qdma_dev->hw_access->qdma_cmpt_ctx_conf(dev, (qid + queue_base),
				&q_cmpt_ctxt, QDMA_HW_ACCESS_WRITE);
	if (err < 0)
		return qdma_dev->hw_access->qdma_get_error_code(err);

	cmptq->cmpt_cidx_info.counter_idx = cmptq->threshidx;
	cmptq->cmpt_cidx_info.timer_idx = cmptq->timeridx;
	cmptq->cmpt_cidx_info.trig_mode = cmptq->triggermode;
	cmptq->cmpt_cidx_info.wrb_en = 1;
	cmptq->cmpt_cidx_info.wrb_cidx = 0;
	qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(dev, qdma_dev->is_vf,
			qid, &cmptq->cmpt_cidx_info);
	cmptq->status = RTE_ETH_QUEUE_STATE_STARTED;

	return 0;
}

/******************************************************************************/
/*
 * Function Name:   rte_pmd_qdma_dev_cmptq_start
 * Description:     Start the MM completion queue.
 *
 * @param   port_id : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_pmd_qdma_dev_cmptq_setup() API when queue is in
 *		memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_start(int port_id, uint32_t qid)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
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
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;

	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
	qdma_dev->hw_access->qdma_cmpt_ctx_conf(dev,
			(qid + qdma_dev->queue_base),
			&q_cmpt_ctxt, QDMA_HW_ACCESS_INVALIDATE);

	/* Zero the cmpt-ring entries*/
	sz = cmptq->cmpt_desc_len;
	for (i = 0; i < (sz * cmptq->nb_cmpt_desc); i++)
		((volatile char *)cmptq->cmpt_ring)[i] = 0;

	cmptq->status = RTE_ETH_QUEUE_STATE_STOPPED;

	return 0;
}

static int qdma_vf_cmptq_context_invalidate(struct rte_eth_dev *dev,
						uint32_t qid)
{
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_cmpt_queue *cmptq;
	uint32_t qid_hw;
	int rv;

	if (!m)
		return -ENOMEM;

	qid_hw = qdma_dev->queue_base + qid;
	qdma_mbox_compose_vf_qctxt_invalidate(qdma_dev->func_id, qid_hw,
					      0, 0, QDMA_MBOX_CMPT_CTXT_ONLY,
					      m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		if (rv != -ENODEV)
			PMD_DRV_LOG(INFO, "%x, qid_hw 0x%x mbox failed %d.\n",
				    qdma_dev->func_id, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

	cmptq = (struct qdma_cmpt_queue *)qdma_dev->cmpt_queues[qid];
	cmptq->status = RTE_ETH_QUEUE_STATE_STOPPED;

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

/******************************************************************************/
/*
 * Function Name:   rte_pmd_qdma_dev_cmptq_stop
 * Description:     Stop the MM completion queue.
 *
 * @param   port_id : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_pmd_qdma_dev_cmptq_start() API when queue is in
 *		memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_stop(int port_id, uint32_t qid)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not configured in MM-mode\n", qid);
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
 * @param   port_id : Port ID.
 * @param   qid : Queue ID.
 * @param   cmpt_buff : User buffer pointer to store the completion data.
 * @param   nb_entries: Number of compeltion entries to process.
 *
 * @return  'number of entries processed' on success and '< 0' on failure.
 *
 * @note    Application can call this API after successful call to
 *	    rte_pmd_qdma_dev_cmptq_start() API.
 ******************************************************************************/

uint16_t rte_pmd_qdma_mm_cmpt_process(int port_id, uint32_t qid,
					void *cmpt_buff, uint16_t nb_entries)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_cmpt_queue *cmptq;
	uint32_t count = 0;
	struct qdma_ul_cmpt_ring *cmpt_entry;
	struct wb_status *wb_status;
	uint16_t nb_entries_avail = 0;
	uint16_t cmpt_tail = 0;
	uint16_t cmpt_pidx;
	int ret = 0;

	ret = validate_qdma_dev_info(port_id, qid);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR,
			"QDMA device validation failed for port id %d\n",
			port_id);
		return ret;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	if (qdma_dev->q_info[qid].queue_mode !=
			RTE_PMD_QDMA_MEMORY_MAPPED_MODE) {
		PMD_DRV_LOG(ERR, "Qid %d is not configured in MM-mode\n", qid);
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
		cmpt_entry =
		(struct qdma_ul_cmpt_ring *)((uint64_t)cmptq->cmpt_ring +
		((uint64_t)cmpt_tail * cmptq->cmpt_desc_len));

		ret = qdma_ul_process_immediate_data(cmpt_entry,
				cmptq->cmpt_desc_len, cmpt_buff);
		if (ret < 0) {
			PMD_DRV_LOG(ERR, "Error detected on CMPT ring at "
					"index %d, queue_id = %d\n",
					cmpt_tail,
					cmptq->queue_id);
			return 0;
		}
		cmpt_tail++;
		if (unlikely(cmpt_tail >= (cmptq->nb_cmpt_desc - 1)))
			cmpt_tail -= (cmptq->nb_cmpt_desc - 1);
		count++;
	}

	// Update the CPMT CIDX
	cmptq->cmpt_cidx_info.wrb_cidx = cmpt_tail;
	qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(cmptq->dev,
			qdma_dev->is_vf,
			cmptq->queue_id,
			&cmptq->cmpt_cidx_info);
	return count;
}

/*****************************************************************************/
/**
 * Function Name:   rte_pmd_qdma_dev_close
 * Description:     DPDK PMD function to close the device.
 *
 * @param   port_id Port ID
 *
 * @return  '0' on success and '< 0' on failure
 *
 ******************************************************************************/
int rte_pmd_qdma_dev_close(uint16_t port_id)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;

	if (port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR, "Wrong port id %d\n", port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;

	dev->data->dev_started = 0;

	if (qdma_dev->is_vf)
		qdma_vf_dev_close(dev);
	else
		qdma_dev_close(dev);

	dev->data->nb_rx_queues = 0;
	rte_free(dev->data->rx_queues);
	dev->data->rx_queues = NULL;
	dev->data->nb_tx_queues = 0;
	rte_free(dev->data->tx_queues);
	dev->data->tx_queues = NULL;

	return 0;
}
