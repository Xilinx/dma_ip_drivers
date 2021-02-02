/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2020 Xilinx, Inc. All rights reserved.
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
#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_ethdev_pci.h>
#include <rte_cycles.h>
#include <rte_kvargs.h>
#include "qdma.h"
#include "qdma_access_common.h"

#include <fcntl.h>
#include <unistd.h>

/* Read register */
uint32_t qdma_read_reg(uint64_t reg_addr)
{
	uint32_t val;

	val = *((volatile uint32_t *)(reg_addr));
	return val;
}

/* Write register */
void qdma_write_reg(uint64_t reg_addr, uint32_t val)
{
	*((volatile uint32_t *)(reg_addr)) = val;
}

uint32_t qdma_pci_read_reg(struct rte_eth_dev *dev, uint32_t bar, uint32_t reg)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint64_t baseaddr;
	uint32_t val;

	if (bar >= (QDMA_NUM_BARS - 1)) {
		printf("Error: PCI BAR number:%u not supported\n"
			"Please enter valid BAR number\n", bar);
		return -1;
	}

	baseaddr = (uint64_t)qdma_dev->bar_addr[bar];
	if (!baseaddr) {
		printf("Error: PCI BAR number:%u not mapped\n", bar);
		return -1;
	}
	val = *((volatile uint32_t *)(baseaddr + reg));

	return val;
}

void qdma_pci_write_reg(struct rte_eth_dev *dev, uint32_t bar,
			uint32_t reg, uint32_t val)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint64_t baseaddr;

	if (bar >= (QDMA_NUM_BARS - 1)) {
		printf("Error: PCI BAR index:%u not supported\n"
			"Please enter valid BAR index\n", bar);
		return;
	}

	baseaddr = (uint64_t)qdma_dev->bar_addr[bar];
	if (!baseaddr) {
		printf("Error: PCI BAR number:%u not mapped\n", bar);
		return;
	}
	*((volatile uint32_t *)(baseaddr + reg)) = val;
}

void qdma_reset_rx_queue(struct qdma_rx_queue *rxq)
{
	uint32_t i;
	uint32_t sz;

	rxq->rx_tail = 0;
	rxq->q_pidx_info.pidx = 0;

	/* Zero out HW ring memory, For MM Descriptor */
	if (rxq->st_mode) {  /** if ST-mode **/
		sz = rxq->cmpt_desc_len;
		for (i = 0; i < (sz * rxq->nb_rx_cmpt_desc); i++)
			((volatile char *)rxq->cmpt_ring)[i] = 0;

		sz = sizeof(struct qdma_ul_st_c2h_desc);
		for (i = 0; i < (sz * rxq->nb_rx_desc); i++)
			((volatile char *)rxq->rx_ring)[i] = 0;

	} else {
		sz = sizeof(struct qdma_ul_mm_desc);
		for (i = 0; i < (sz * rxq->nb_rx_desc); i++)
			((volatile char *)rxq->rx_ring)[i] = 0;
	}

	/* Initialize SW ring entries */
	for (i = 0; i < rxq->nb_rx_desc; i++)
		rxq->sw_ring[i] = NULL;
}

void qdma_inv_rx_queue_ctxts(struct rte_eth_dev *dev,
			     uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_descq_prefetch_ctxt q_prefetch_ctxt;
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;
	struct qdma_descq_hw_ctxt q_hw_ctxt;
	struct qdma_descq_credit_ctxt q_credit_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	hw_access->qdma_sw_ctx_conf(dev, 1, qid, &q_sw_ctxt,
			QDMA_HW_ACCESS_INVALIDATE);
	hw_access->qdma_hw_ctx_conf(dev, 1, qid, &q_hw_ctxt,
			QDMA_HW_ACCESS_INVALIDATE);
	if (mode) {  /** ST-mode **/
		hw_access->qdma_pfetch_ctx_conf(dev, qid,
			&q_prefetch_ctxt, QDMA_HW_ACCESS_INVALIDATE);
		hw_access->qdma_cmpt_ctx_conf(dev, qid,
			&q_cmpt_ctxt, QDMA_HW_ACCESS_INVALIDATE);
		hw_access->qdma_credit_ctx_conf(dev, 1, qid,
			&q_credit_ctxt, QDMA_HW_ACCESS_INVALIDATE);
	}
}

/**
 * Clears the Rx queue contexts.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   Nothing.
 */
void qdma_clr_rx_queue_ctxts(struct rte_eth_dev *dev,
			     uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_descq_prefetch_ctxt q_prefetch_ctxt;
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;
	struct qdma_descq_hw_ctxt q_hw_ctxt;
	struct qdma_descq_credit_ctxt q_credit_ctxt;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	hw_access->qdma_sw_ctx_conf(dev, 1, qid, &q_sw_ctxt,
			QDMA_HW_ACCESS_CLEAR);
	hw_access->qdma_hw_ctx_conf(dev, 1, qid, &q_hw_ctxt,
			QDMA_HW_ACCESS_CLEAR);
	if (mode) {  /** ST-mode **/
		hw_access->qdma_pfetch_ctx_conf(dev, qid,
			&q_prefetch_ctxt, QDMA_HW_ACCESS_CLEAR);
		hw_access->qdma_cmpt_ctx_conf(dev, qid,
			&q_cmpt_ctxt, QDMA_HW_ACCESS_CLEAR);
		hw_access->qdma_credit_ctx_conf(dev, 1, qid,
			&q_credit_ctxt, QDMA_HW_ACCESS_CLEAR);
	}
}

int qdma_init_rx_queue(struct qdma_rx_queue *rxq)
{
	struct rte_mbuf *mb;
	void *obj = NULL;
	uint64_t phys_addr;
	uint16_t i;
	struct qdma_ul_st_c2h_desc *rx_ring_st = NULL;

	/* allocate new buffers for the Rx descriptor ring */
	if (rxq->st_mode) {  /** ST-mode **/
		rx_ring_st = (struct qdma_ul_st_c2h_desc *)rxq->rx_ring;
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id %d, mbuf_avail_count =%d,"
				"mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
		for (i = 0; i < (rxq->nb_rx_desc - 2); i++) {
			if (rte_mempool_get(rxq->mb_pool, &obj) != 0) {
				PMD_DRV_LOG(ERR, "qdma-start-rx-queue(): "
						"rte_mempool_get: failed");
				goto fail;
			}

			if (obj != NULL)
				mb = obj;
			else {
				PMD_DRV_LOG(ERR, "%s(): %d: qid %d, rte_mempool_get failed",
				__func__, __LINE__, rxq->queue_id);
				goto fail;
			}

			phys_addr = (uint64_t)mb->buf_iova +
				     RTE_PKTMBUF_HEADROOM;

			mb->data_off = RTE_PKTMBUF_HEADROOM;
			rxq->sw_ring[i] = mb;
			rx_ring_st[i].dst_addr = phys_addr;
		}
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: qid %d, mbuf_avail_count = %d,"
				"mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
	}

	/* initialize tail */
	rxq->rx_tail = 0;

	return 0;
fail:
	return -ENOMEM;
}

/*
 * Tx queue reset
 */
void qdma_reset_tx_queue(struct qdma_tx_queue *txq)
{
	uint32_t i;
	uint32_t sz;

	txq->tx_fl_tail = 0;
	if (txq->st_mode) {  /** ST-mode **/
		sz = sizeof(struct qdma_ul_st_h2c_desc);
		/* Zero out HW ring memory */
		for (i = 0; i < (sz * (txq->nb_tx_desc)); i++)
			((volatile char *)txq->tx_ring)[i] = 0;
	} else {
		sz = sizeof(struct qdma_ul_mm_desc);
		/* Zero out HW ring memory */
		for (i = 0; i < (sz * (txq->nb_tx_desc)); i++)
			((volatile char *)txq->tx_ring)[i] = 0;
	}

	/* Initialize SW ring entries */
	for (i = 0; i < txq->nb_tx_desc; i++)
		txq->sw_ring[i] = NULL;
}

void qdma_inv_tx_queue_ctxts(struct rte_eth_dev *dev,
			     uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_descq_hw_ctxt q_hw_ctxt;
	struct qdma_descq_credit_ctxt q_credit_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	hw_access->qdma_sw_ctx_conf(dev, 0, qid, &q_sw_ctxt,
			QDMA_HW_ACCESS_INVALIDATE);
	hw_access->qdma_hw_ctx_conf(dev, 0, qid, &q_hw_ctxt,
			QDMA_HW_ACCESS_INVALIDATE);

	if (mode) {  /** ST-mode **/
		hw_access->qdma_credit_ctx_conf(dev, 0, qid,
			&q_credit_ctxt, QDMA_HW_ACCESS_INVALIDATE);
	}
}

/**
 * Clear Tx queue contexts
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   Nothing.
 */
void qdma_clr_tx_queue_ctxts(struct rte_eth_dev *dev,
			     uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_descq_credit_ctxt q_credit_ctxt;
	struct qdma_descq_hw_ctxt q_hw_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	hw_access->qdma_sw_ctx_conf(dev, 0, qid, &q_sw_ctxt,
			QDMA_HW_ACCESS_CLEAR);
	hw_access->qdma_hw_ctx_conf(dev, 0, qid, &q_hw_ctxt,
			QDMA_HW_ACCESS_CLEAR);
	if (mode) {  /** ST-mode **/
		hw_access->qdma_credit_ctx_conf(dev, 0, qid,
			&q_credit_ctxt, QDMA_HW_ACCESS_CLEAR);
	}
}

/* Utility function to find index of an element in an array */
int index_of_array(uint32_t *arr, uint32_t n, uint32_t element)
{
	int index = 0;

	for (index = 0; (uint32_t)index < n; index++) {
		if (*(arr + index) == element)
			return index;
	}
	return -1;
}

static int pfetch_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;
	uint8_t desc_prefetch;

	PMD_DRV_LOG(INFO, "QDMA devargs desc_prefetch is: %s\n", value);
	desc_prefetch = (uint8_t)strtoul(value, &end, 10);
	if (desc_prefetch > 1) {
		PMD_DRV_LOG(INFO, "QDMA devargs prefetch should be 1 or 0,"
						  " setting to 1.\n");
	}
	qdma_dev->en_desc_prefetch = desc_prefetch ? 1 : 0;
	return 0;
}

static int cmpt_desc_len_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs cmpt_desc_len is: %s\n", value);
	qdma_dev->cmpt_desc_len =  (uint8_t)strtoul(value, &end, 10);
	if (qdma_dev->cmpt_desc_len != RTE_PMD_QDMA_CMPT_DESC_LEN_8B &&
		qdma_dev->cmpt_desc_len != RTE_PMD_QDMA_CMPT_DESC_LEN_16B &&
		qdma_dev->cmpt_desc_len != RTE_PMD_QDMA_CMPT_DESC_LEN_32B &&
		(qdma_dev->cmpt_desc_len != RTE_PMD_QDMA_CMPT_DESC_LEN_64B ||
		!qdma_dev->dev_cap.cmpt_desc_64b)) {
		PMD_DRV_LOG(INFO, "QDMA devargs incorrect cmpt_desc_len = %d "
						  "specified\n",
						  qdma_dev->cmpt_desc_len);
		return -1;
	}

	return 0;
}

static int trigger_mode_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs trigger mode: %s\n", value);
	qdma_dev->trigger_mode =  (uint8_t)strtoul(value, &end, 10);

	if (qdma_dev->trigger_mode >= RTE_PMD_QDMA_TRIG_MODE_MAX) {
		qdma_dev->trigger_mode = RTE_PMD_QDMA_TRIG_MODE_MAX;
		PMD_DRV_LOG(INFO, "QDMA devargs trigger mode invalid,"
						  "reset to default: %d\n",
						  qdma_dev->trigger_mode);
	}
	return 0;
}

static int config_bar_idx_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs trigger mode: %s\n", value);
	qdma_dev->config_bar_idx =  (int)strtoul(value, &end, 10);

	if (qdma_dev->config_bar_idx >= QDMA_NUM_BARS ||
			qdma_dev->config_bar_idx < 0) {
		PMD_DRV_LOG(INFO, "QDMA devargs config bar idx invalid: %d\n",
				qdma_dev->config_bar_idx);
		return -1;
	}
	return 0;
}

static int c2h_byp_mode_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs c2h_byp_mode is: %s\n", value);
	qdma_dev->c2h_bypass_mode =  (uint8_t)strtoul(value, &end, 10);

	if (qdma_dev->c2h_bypass_mode >= RTE_PMD_QDMA_RX_BYPASS_MAX) {
		PMD_DRV_LOG(INFO, "QDMA devargs incorrect "
				"c2h_byp_mode= %d specified\n",
						qdma_dev->c2h_bypass_mode);
		return -1;
	}

	return 0;
}

static int h2c_byp_mode_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs h2c_byp_mode is: %s\n", value);
	qdma_dev->h2c_bypass_mode =  (uint8_t)strtoul(value, &end, 10);

	if (qdma_dev->h2c_bypass_mode > 1) {
		PMD_DRV_LOG(INFO, "QDMA devargs incorrect"
				" h2c_byp_mode =%d specified\n",
					qdma_dev->h2c_bypass_mode);
		return -1;
	}

	return 0;
}

/* Process the all devargs */
int qdma_check_kvargs(struct rte_devargs *devargs,
						struct qdma_pci_dev *qdma_dev)
{
	struct rte_kvargs *kvlist;
	const char *pfetch_key = "desc_prefetch";
	const char *cmpt_desc_len_key = "cmpt_desc_len";
	const char *trigger_mode_key = "trigger_mode";
	const char *config_bar_key = "config_bar";
	const char *c2h_byp_mode_key = "c2h_byp_mode";
	const char *h2c_byp_mode_key = "h2c_byp_mode";
	int ret = 0;

	if (!devargs)
		return 0;

	kvlist = rte_kvargs_parse(devargs->args, NULL);
	if (!kvlist)
		return 0;

	/* process the desc_prefetch*/
	if (rte_kvargs_count(kvlist, pfetch_key)) {
		ret = rte_kvargs_process(kvlist, pfetch_key,
						pfetch_check_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	/* process the cmpt_desc_len*/
	if (rte_kvargs_count(kvlist, cmpt_desc_len_key)) {
		ret = rte_kvargs_process(kvlist, cmpt_desc_len_key,
					 cmpt_desc_len_check_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	/* process the trigger_mode*/
	if (rte_kvargs_count(kvlist, trigger_mode_key)) {
		ret = rte_kvargs_process(kvlist, trigger_mode_key,
						trigger_mode_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	/* process the config bar*/
	if (rte_kvargs_count(kvlist, config_bar_key)) {
		ret = rte_kvargs_process(kvlist, config_bar_key,
					   config_bar_idx_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	/* process c2h_byp_mode*/
	if (rte_kvargs_count(kvlist, c2h_byp_mode_key)) {
		ret = rte_kvargs_process(kvlist, c2h_byp_mode_key,
					  c2h_byp_mode_check_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	/* process h2c_byp_mode*/
	if (rte_kvargs_count(kvlist, h2c_byp_mode_key)) {
		ret = rte_kvargs_process(kvlist, h2c_byp_mode_key,
					  h2c_byp_mode_check_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	rte_kvargs_free(kvlist);
	return ret;
}

int qdma_identify_bars(struct rte_eth_dev *dev)
{
	int      bar_len, i, ret;
	uint8_t  usr_bar;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_pci_dev *dma_priv;

	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;

	/* Config bar */
	bar_len = pci_dev->mem_resource[dma_priv->config_bar_idx].len;
	if (!bar_len) {
		PMD_DRV_LOG(INFO, "QDMA config BAR index :%d is not enabled",
					dma_priv->config_bar_idx);
		return -1;
	}

	/* Find AXI Master Lite(user bar) */
	ret = dma_priv->hw_access->qdma_get_user_bar(dev,
			dma_priv->is_vf, dma_priv->func_id, &usr_bar);
	if ((ret != QDMA_SUCCESS) ||
			(pci_dev->mem_resource[usr_bar].len == 0)) {
		if (dma_priv->ip_type == QDMA_VERSAL_HARD_IP) {
			if (pci_dev->mem_resource[1].len == 0)
				dma_priv->user_bar_idx = 2;
			else
				dma_priv->user_bar_idx = 1;
		} else {
			dma_priv->user_bar_idx = -1;
			PMD_DRV_LOG(INFO, "Cannot find AXI Master Lite BAR");
		}
	} else
		dma_priv->user_bar_idx = usr_bar;

	/* Find AXI Bridge Master bar(bypass bar) */
	for (i = 0; i < QDMA_NUM_BARS; i++) {
		bar_len = pci_dev->mem_resource[i].len;
		if (!bar_len) /* Bar not enabled ? */
			continue;
		if (dma_priv->user_bar_idx != i &&
				dma_priv->config_bar_idx != i) {
			dma_priv->bypass_bar_idx = i;
			break;
		}
	}

	PMD_DRV_LOG(INFO, "QDMA config bar idx :%d\n",
			dma_priv->config_bar_idx);
	PMD_DRV_LOG(INFO, "QDMA AXI Master Lite bar idx :%d\n",
			dma_priv->user_bar_idx);
	PMD_DRV_LOG(INFO, "QDMA AXI Bridge Master bar idx :%d\n",
			dma_priv->bypass_bar_idx);

	return 0;
}
int qdma_get_hw_version(struct rte_eth_dev *dev)
{
	int ret;
	struct qdma_pci_dev *dma_priv;
	struct qdma_hw_version_info version_info;

	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	ret = dma_priv->hw_access->qdma_get_version(dev,
			dma_priv->is_vf, &version_info);
	if (ret < 0)
		return dma_priv->hw_access->qdma_get_error_code(ret);

	dma_priv->rtl_version = version_info.rtl_version;
	dma_priv->vivado_rel = version_info.vivado_release;
	dma_priv->device_type = version_info.device_type;
	dma_priv->ip_type = version_info.ip_type;

	PMD_DRV_LOG(INFO, "QDMA RTL VERSION : %s\n",
		version_info.qdma_rtl_version_str);
	PMD_DRV_LOG(INFO, "QDMA DEVICE TYPE : %s\n",
		version_info.qdma_device_type_str);
	PMD_DRV_LOG(INFO, "QDMA VIVADO RELEASE ID : %s\n",
		version_info.qdma_vivado_release_id_str);
	if (version_info.ip_type == QDMA_VERSAL_HARD_IP) {
		PMD_DRV_LOG(INFO, "QDMA VERSAL IP TYPE : %s\n",
			version_info.qdma_ip_type_str);
	}

	return 0;
}
