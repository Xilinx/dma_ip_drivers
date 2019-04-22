/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2018 Xilinx, Inc. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "qdma_regs.h"

uint32_t g_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
uint32_t g_c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ];
uint32_t g_c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
uint32_t g_c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ];

int get_param(struct rte_eth_dev *dev, enum get_param_type param,
		void *ret_param)
{
	struct qdma_pci_dev *dma_priv = dev->data->dev_private;

	if (!ret_param)
		return -1;

	switch (param) {
	case CONFIG_BAR:
		*((int *)ret_param) = dma_priv->config_bar_idx;
		break;
	case USER_BAR:
		*((int *)ret_param) = dma_priv->user_bar_idx;
		break;
	case BYPASS_BAR:
		*((int *)ret_param) = dma_priv->bypass_bar_idx;
		break;
	}
	return 0;
}


int get_queue_param(struct rte_eth_dev *dev, uint32_t qid,
			enum get_queue_param_type param, void *ret_param)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Q-id passed qid %d max en_qid %d\n",
				qid, qdma_dev->qsets_en);
		return -1;
	}

	if (!ret_param) {
		PMD_DRV_LOG(ERR, "Invalid ret_param for qid %d\n", qid);
		return -1;
	}
	switch (param) {
	/* Use this param after queue setup is done, and before queue start*/
	case CHECK_DUMP_IMMEDIATE_DATA:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"DUMP_IMMEDIATE_DATA\n");
			return -1;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		*((int *)ret_param) = rxq->dump_immediate_data;
		break;
	}
	return 0;
}

int update_param(struct rte_eth_dev *dev, enum dev_param_type param,
							uint16_t value)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	switch (param) {
	case QUEUE_BASE:
		if (value > qdma_dev->qsets_max) {
			PMD_DRV_LOG(ERR, "Invalid Queue base passed\n");
			return -1;
		}
		qdma_dev->queue_base = value;
		break;
	case TIMER_COUNT:
		qdma_dev->timer_count = value;
		break;
	case TRIGGER_MODE:
		if (value > TRIG_MODE_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Trigger mode passed\n");
			return -1;
		}
		qdma_dev->trigger_mode = value;
		break;
	default:
		PMD_DRV_LOG(ERR, "Invalid param %x specified\n", param);
		break;
	}
	return 0;
}

int update_queue_param(struct rte_eth_dev *dev, uint32_t qid,
				enum queue_param_type param, uint32_t value)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;

	if (qid >= qdma_dev->qsets_en) {
		PMD_DRV_LOG(ERR, "Invalid Queue id passed\n");
		return -1;
	}

	switch (param) {
	case QUEUE_MODE:
		if (value > STREAMING_MODE) {
			PMD_DRV_LOG(ERR, "Invalid Queue mode passed\n");
			return -1;
		}
		qdma_dev->q_info[qid].queue_mode = value;
		break;

	case DIS_OVERFLOW_CHECK: /* Update this param after queue-setup issued*/
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"DIS_OVERFLOW_CHECK\n");
			return -1;
		}
		if (value > 1)
			return -1;
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		rxq->dis_overflow_check = value;
		break;

	case DUMP_IMMEDIATE_DATA: /*Update this param after queue-setup issued*/
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"DUMP_IMMEDIATE_DATA\n");
			return -1;
		}
		if (value > 1)
			return -1;
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		rxq->dump_immediate_data = value;
		break;

	case DESC_PREFETCH:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"DESC_PREFETCH\n");
				return -1;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		rxq->en_prefetch = (value > 0) ? 1 : 0;
		break;

	case RX_BYPASS_MODE:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"RX_BYPASS_MODE\n");
			return -1;
		}
		if (value > C2H_BYPASS_MODE_MAX) {
			PMD_DRV_LOG(ERR, "Invalid Rx Bypass mode : %d\n",
					value);
			return -1;
		}
		qdma_dev->q_info[qid].rx_bypass_mode = value;
		break;

	case TX_BYPASS_MODE:
		if (qid >= dev->data->nb_tx_queues) {
			PMD_DRV_LOG(ERR, "Invalid TX Queue id passed for "
					"TX_BYPASS_MODE\n");
			return -1;
		}
		if (value > 1) {
			PMD_DRV_LOG(ERR, "Invalid Tx Bypass mode : %d\n",
					value);
			return -1;
		}
		qdma_dev->q_info[qid].tx_bypass_mode = value;
		break;

	case RX_BYPASS_DESC_SIZE:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed for "
					"RX_BYPASS_DESC_SIZE\n");
			return -1;
		}
	/* Only 64byte descriptor size supported in 2018.3 Example Design*/
		if (value  != 64)
			return -1;
		qdma_dev->q_info[qid].rx_bypass_desc_sz =
			(enum bypass_desc_len)value;
		break;

	case TX_BYPASS_DESC_SIZE:
		if (qid >= dev->data->nb_tx_queues) {
			PMD_DRV_LOG(ERR, "Invalid TX Queue id passed "
					"for TX_BYPASS_DESC_SIZE\n");
			return -1;
		}
	/* Only 64byte descriptor size supported in 2018.3 Example Design*/
		if (value  != 64)
			return -1;
		qdma_dev->q_info[qid].tx_bypass_desc_sz =
			(enum bypass_desc_len)value;
		break;

	case TX_DST_ADDRESS:
		if (qid >= dev->data->nb_tx_queues) {
			PMD_DRV_LOG(ERR, "Invalid TX Queue id passed for  "
					"TX_DST_ADDRESS\n");
			return -1;
		}
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
		txq->ep_addr = value;
		break;

	case RX_SRC_ADDRESS:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed "
					"for RX_SRC_ADDRESS\n");
			return -1;
		}
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
		rxq->ep_addr = value;
		break;

	case CMPT_DESC_SIZE:
		if (qid >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(ERR, "Invalid RX Queue id passed "
					"for CMPT_DESC_SIZE\n");
			return -1;
		}
		if (value != CMPT_DESC_LEN_8B &&
			value != CMPT_DESC_LEN_16B &&
			value != CMPT_DESC_LEN_32B &&
			value != CMPT_DESC_LEN_64B)
			return -1;
		qdma_dev->q_info[qid].cmpt_desc_sz = value;
		break;

	default:
		PMD_DRV_LOG(ERR, "Invalid param %x specified\n", param);
		break;
	}

	return 0;
}

int qdma_set_fmap(struct qdma_pci_dev *qdma_dev, uint16_t devfn,
			uint32_t q_base, uint32_t q_count)
{
	uint64_t bar_addr;
#if FMAP_CNTXT
	struct queue_ind_prg *q_regs;
	uint32_t ctxt_sel;
	uint16_t flag, i;
#else
	uint64_t fmap;
#endif
#if defined(RTE_LIBRTE_QDMA_DEBUG_DRIVER) || FMAP_CNTXT
	uint64_t reg_val;
#endif

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
#if FMAP_CNTXT
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
				QDMA_IND_Q_PRG_OFF);

	qdma_write_reg((uint64_t)&q_regs->ctxt_data[0], q_base);
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[1], q_count);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], MASK_32BIT);
	ctxt_sel = (QDMA_CTXT_SEL_FMAP << CTXT_SEL_SHIFT_B) |
				(devfn << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_WR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, " Read cmd for FMAP CTXT for device:%d: "
				"reg_val:%lx\n", devfn, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (flag) {
		PMD_DRV_LOG(ERR, "Error: Busy on device:%d: FMAP "
				"initailization with cmd reg_val:%lx\n",
				devfn, reg_val);
		goto fail;
	}
#else
	fmap = (uint64_t)(bar_addr + QDMA_TRQ_SEL_FMAP + devfn * 4);
	qdma_write_reg((uint64_t)fmap, (q_count  << QID_MAX_SHIFT_B) |
					(q_base & QID_BASE_MSK));
#endif
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
#if FMAP_CNTXT
	ctxt_sel = (QDMA_CTXT_SEL_FMAP << CTXT_SEL_SHIFT_B) |
			(devfn << QID_SHIFT_B) |
			(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	for (i = 0; i < 2; i++) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_data[i]);
		PMD_DRV_LOG(INFO, " FMAP CTXT for device :%d, idx:%d:"
				"reg_val:%lx\n", devfn, i, reg_val);
		/* TO-DO: Need to verify the Queue ctxt-data and
		 * if Queue has any errors, then return -1. **/
	}
#else
	reg_val = qdma_read_reg((uint64_t)fmap);
	PMD_DRV_LOG(INFO, "Fmap for function id %d reg_val:0x%lx,"
			" qid_max:%lu\n", devfn, reg_val,
			((reg_val & QID_MAX_MSK) >> QID_MAX_SHIFT_B));
#endif
#endif
	return 0;

#if FMAP_CNTXT
fail:
	return -1;
#endif
}
uint8_t qmda_get_desc_sz_idx(enum bypass_desc_len size)
{
	uint8_t ret;
	switch (size) {
	case BYPASS_DESC_LEN_8B:
		ret = 0;
		break;
	case BYPASS_DESC_LEN_16B:
		ret = 1;
		break;
	case BYPASS_DESC_LEN_32B:
		ret = 2;
		break;
	case BYPASS_DESC_LEN_64B:
		ret = 3;
		break;
	default:
		/* Suppress compiler warnings*/
		ret = 0;
	}
	return ret;
}

/**
 * DPDK callback to configure a RX queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param rx_queue_id
 *   RX queue index.
 * @param nb_rx_desc
 *   Number of descriptors to configure in queue.
 * @param socket_id
 *   NUMA socket on which memory must be allocated.
 * @param[in] rx_conf
 *   Thresholds parameters.
 * @param mp_pool
 *   Memory pool for buffer allocations.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
int qdma_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
				uint16_t nb_rx_desc, unsigned int socket_id,
				const struct rte_eth_rxconf *rx_conf,
				struct rte_mempool *mb_pool)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	struct qdma_mm_desc *rx_ring_mm;
	uint64_t dma_bar;
	uint32_t sz;
	uint8_t  *rx_ring_bypass;
	int bypass_desc_sz_idx;

	PMD_DRV_LOG(INFO, "Configuring Rx queue id:%d\n", rx_queue_id);

	if (nb_rx_desc == 0) {
		PMD_DRV_LOG(ERR, "Invalid descriptor ring size %d\n",
				nb_rx_desc);
		return -EINVAL;
	}

	/* allocate rx queue data structure */
	rxq = rte_zmalloc("QDMA_RxQ", sizeof(struct qdma_rx_queue),
						RTE_CACHE_LINE_SIZE);
	if (!rxq) {
		PMD_DRV_LOG(ERR, "Unable to allocate structure rxq of "
				"size %d\n",
				(int)(sizeof(struct qdma_rx_queue)));
		return (-ENOMEM);
	}

	dma_bar = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	rxq->st_mode = qdma_dev->q_info[rx_queue_id].queue_mode;
	rxq->nb_rx_desc = nb_rx_desc;
	rxq->bypass_desc_sz = qdma_dev->q_info[rx_queue_id].rx_bypass_desc_sz;
	bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);

	if (qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
				C2H_BYPASS_MODE_CACHE ||
			qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
			 C2H_BYPASS_MODE_SIMPLE)
		rxq->en_bypass = 1;
	if (qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
			C2H_BYPASS_MODE_SIMPLE)
		rxq->en_bypass_prefetch = 1;

	/* <= 2018.2 IP
	 * double the cmpl ring size to avoid run out of cmpl entry while
	 * desc. ring still have free entries
	 */
	rxq->nb_rx_cmpt_desc = (nb_rx_desc * 2);
	rxq->en_prefetch = qdma_dev->en_desc_prefetch;
	rxq->cmpt_desc_len = qdma_dev->q_info[rx_queue_id].cmpt_desc_sz;

	/* Disable the cmpt over flow check by default
	 * Applcation can test enable/disable via update param before
	 * queue_start is issued
	 */
	rxq->dis_overflow_check = 0;

	/* allocate memory for Rx descriptor ring */
	if (rxq->st_mode) {
		if (!qdma_dev->st_mode_en) {
			PMD_DRV_LOG(ERR, "Streaming mode not enabled "
					"in the hardware\n");
			rte_free(rxq);
			return -EINVAL;
		}

		if (rxq->en_bypass &&
		     bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			sz = (rxq->nb_rx_desc) * (rxq->bypass_desc_sz);
		else
			sz = (rxq->nb_rx_desc) * sizeof(struct qdma_c2h_desc);

		rxq->rx_mz = qdma_zone_reserve(dev, "RxHwRn", rx_queue_id,
						sz, socket_id);
		if (!rxq->rx_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_mz "
					"of size %d\n", sz);
			rte_free(rxq);
			return -ENOMEM;
		}
		rxq->rx_ring = rxq->rx_mz->addr;
	} else {
		if (!qdma_dev->mm_mode_en) {
			PMD_DRV_LOG(ERR, "Memory mapped mode not enabled "
					"in the hardware\n");
			rte_free(rxq);
			return -EINVAL;
		}
		if (rxq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			sz = (rxq->nb_rx_desc) * (rxq->bypass_desc_sz);
		else
			sz = (rxq->nb_rx_desc) * sizeof(struct qdma_mm_desc);
		rxq->rx_mz = qdma_zone_reserve(dev, "RxHwRn",
						rx_queue_id, sz, socket_id);
		if (!rxq->rx_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_mz "
					"of size %d\n", sz);
			rte_free(rxq);
			return -ENOMEM;
		}
		rxq->rx_ring = rxq->rx_mz->addr;
		rx_ring_mm = (struct qdma_mm_desc *)rxq->rx_mz->addr;
		rx_ring_bypass = (uint8_t *)rxq->rx_mz->addr;
		if (rxq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			rxq->wb_status = (struct wb_status *)&
					(rx_ring_bypass[(rxq->nb_rx_desc - 1) *
							(rxq->bypass_desc_sz)]);
		else
			rxq->wb_status = (struct wb_status *)&
					 (rx_ring_mm[rxq->nb_rx_desc - 1]);
	}

	if (rxq->st_mode) {
		/* allocate memory for Rx completion(cpmt) descriptor ring */
		sz = (rxq->nb_rx_cmpt_desc) * rxq->cmpt_desc_len;
		rxq->rx_cmpt_mz = qdma_zone_reserve(dev, "RxHwCmptRn",
						    rx_queue_id, sz, socket_id);
		if (!rxq->rx_cmpt_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_cmpt_mz "
					"of size %d\n", sz);
			rte_memzone_free(rxq->rx_mz);
			rte_free(rxq);
			return -ENOMEM;
		}
		rxq->cmpt_ring = (struct c2h_cmpt_ring *)rxq->rx_cmpt_mz->addr;

		/* Write-back status structure */
		rxq->wb_status = (struct wb_status *)((uint64_t)rxq->cmpt_ring +
				 (((uint64_t)rxq->nb_rx_cmpt_desc - 1) *
				  rxq->cmpt_desc_len));

		if (!qdma_dev->is_vf) {
			rxq->rx_cidx = (volatile uint32_t *)(dma_bar +
					QDMA_TRQ_SEL_QUEUE_PF +
					QDMA_SEL_CMPT_CIDX_Q_OFF +
					(rx_queue_id * QDMA_CIDX_STEP));
		} else {
			rxq->rx_cidx = (volatile uint32_t *)(dma_bar +
					QDMA_TRQ_SEL_QUEUE_VF +
					QDMA_SEL_CMPT_CIDX_Q_OFF +
					(rx_queue_id * QDMA_CIDX_STEP));
		}
	}

	/* allocate memory for RX software ring */
	sz = rxq->nb_rx_desc * sizeof(struct rte_mbuf *);
	rxq->sw_ring = rte_zmalloc("RxSwRn", sz, RTE_CACHE_LINE_SIZE);
	if (!rxq->sw_ring) {
		PMD_DRV_LOG(ERR, "Unable to allocate rxq->sw_ring of size %d\n",
									sz);
		rte_memzone_free(rxq->rx_cmpt_mz);
		rte_memzone_free(rxq->rx_mz);
		rte_free(rxq);
		return -ENOMEM;
	}

	rxq->queue_id = rx_queue_id;

	rxq->port_id = dev->data->port_id;
	rxq->func_id = qdma_dev->pf;
	rxq->mb_pool = mb_pool;

	/* Calculate the ring index, completion queue ring size,
	 * buffer index and threshold index.
	 * If index is not found , by default use the index as 0
	 */

	/* Find C2H queue ring size index */
	rxq->ringszidx = index_of_array(g_ring_sz, QDMA_GLOBAL_CSR_ARRAY_SZ,
					rxq->nb_rx_desc);
	if (rxq->ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected Ring size %d not found\n",
				rxq->nb_rx_desc);
		rte_memzone_free(rxq->rx_cmpt_mz);
		rte_memzone_free(rxq->rx_mz);
		rte_free(rxq->sw_ring);
		rte_free(rxq);
		return -EINVAL;
	}

	/* Find completion ring size index */
	rxq->cmpt_ringszidx = index_of_array(g_ring_sz,
						QDMA_GLOBAL_CSR_ARRAY_SZ,
						rxq->nb_rx_cmpt_desc);
	if (rxq->cmpt_ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected completion ring size %d not found\n",
				rxq->nb_rx_cmpt_desc);
		rte_memzone_free(rxq->rx_cmpt_mz);
		rte_memzone_free(rxq->rx_mz);
		rte_free(rxq->sw_ring);
		rte_free(rxq);
		return -EINVAL;
	}

	/* Find Threshold index */
	rxq->threshidx = index_of_array(g_c2h_cnt_th, QDMA_GLOBAL_CSR_ARRAY_SZ,
					rx_conf->rx_thresh.wthresh);
	if (rxq->threshidx < 0) {
		PMD_DRV_LOG(WARNING, "Expected Threshold %d not found,"
				" using the value %d at index 0\n",
				rx_conf->rx_thresh.wthresh, g_c2h_cnt_th[0]);
		rxq->threshidx = 0;
	}

	/* Find Timer index */
	rxq->timeridx = index_of_array(g_c2h_timer_cnt,
					QDMA_GLOBAL_CSR_ARRAY_SZ,
					qdma_dev->timer_count);
	if (rxq->timeridx < 0) {
		PMD_DRV_LOG(WARNING, "Expected timer %d not found, "
				"using the value %d at index 1\n",
				qdma_dev->timer_count, g_c2h_timer_cnt[1]);
		rxq->timeridx = 1;
	}

	/* Find Buffer size index */
	rxq->rx_buff_size = (uint16_t)
				(rte_pktmbuf_data_room_size(rxq->mb_pool) -
				 RTE_PKTMBUF_HEADROOM);
	rxq->buffszidx = index_of_array(g_c2h_buf_sz, QDMA_GLOBAL_CSR_ARRAY_SZ,
					rxq->rx_buff_size);
	if (rxq->buffszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected buffer size %d not found\n",
				rxq->rx_buff_size);
		rte_memzone_free(rxq->rx_cmpt_mz);
		rte_memzone_free(rxq->rx_mz);
		rte_free(rxq->sw_ring);
		rte_free(rxq);
		return -EINVAL;
	}

	rxq->triggermode = qdma_dev->trigger_mode;
	rxq->rx_deferred_start = rx_conf->rx_deferred_start;

	if (!qdma_dev->is_vf) {
		rxq->rx_pidx = (volatile uint32_t *)(dma_bar +
							QDMA_TRQ_SEL_QUEUE_PF +
							QDMA_C2H_PIDX_Q_OFF +
							(rx_queue_id *
							 QDMA_PIDX_STEP));
	} else {
		rxq->rx_pidx = (volatile uint32_t *)(dma_bar +
							QDMA_TRQ_SEL_QUEUE_VF +
							QDMA_C2H_PIDX_Q_OFF +
							(rx_queue_id *
							 QDMA_PIDX_STEP));
	}

	/* store rx_pkt_burst function pointer */
	dev->rx_pkt_burst = qdma_recv_pkts;

	dev->data->rx_queues[rx_queue_id] = rxq;

	return 0;
}

/**
 * DPDK callback to configure a TX queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param tx_queue_id
 *   TX queue index.
 * @param nb_tx_desc
 *   Number of descriptors to configure in queue.
 * @param socket_id
 *   NUMA socket on which memory must be allocated.
 * @param[in] tx_conf
 *   Thresholds parameters.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
int qdma_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
			    uint16_t nb_tx_desc, unsigned int socket_id,
			    const struct rte_eth_txconf *tx_conf)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_mm_desc *tx_ring_mm;
	struct qdma_h2c_desc *tx_ring_st;
	uint8_t  *tx_ring_bypass;
	uint64_t dma_bar;
	uint32_t sz;
	int	bypass_desc_sz_idx;

	PMD_DRV_LOG(INFO, "Configuring Tx queue id:%d\n", tx_queue_id);

	/* allocate rx queue data structure */
	txq = rte_zmalloc("QDMA_TxQ", sizeof(struct qdma_tx_queue),
						RTE_CACHE_LINE_SIZE);
	if (!txq) {
		PMD_DRV_LOG(ERR, "Memory allocation failed for "
				"Tx queue SW structure\n");
		return (-ENOMEM);
	}

	dma_bar = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	txq->st_mode = qdma_dev->q_info[tx_queue_id].queue_mode;
	txq->en_bypass = (qdma_dev->q_info[tx_queue_id].tx_bypass_mode) ? 1 : 0;
	txq->bypass_desc_sz = qdma_dev->q_info[tx_queue_id].tx_bypass_desc_sz;
	bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	/* allocate memory for TX descriptor ring */
	if (txq->st_mode) {
		if (!qdma_dev->st_mode_en) {
			PMD_DRV_LOG(ERR, "Streaming mode not enabled "
					"in the hardware\n");
			rte_free(txq);
			return -EINVAL;
		}
		if (txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			sz = (nb_tx_desc) * (txq->bypass_desc_sz);
		else
			sz = (nb_tx_desc) * sizeof(struct qdma_h2c_desc);
		txq->tx_mz = qdma_zone_reserve(dev, "TxHwRn", tx_queue_id, sz,
						socket_id);
		if (!txq->tx_mz) {
			PMD_DRV_LOG(ERR, "Couldn't reserve memory for "
					"ST H2C ring of size %d\n", sz);
			rte_free(txq);
			return -ENOMEM;
		}

		txq->tx_ring = txq->tx_mz->addr;
		tx_ring_st = (struct qdma_h2c_desc *)txq->tx_ring;
		tx_ring_bypass = (uint8_t *)txq->tx_ring;
		/* Write-back status structure */
		if (txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			txq->wb_status = (struct wb_status *)&
					tx_ring_bypass[(nb_tx_desc - 1) *
					(txq->bypass_desc_sz)];
		else
			txq->wb_status = (struct wb_status *)&
					tx_ring_st[nb_tx_desc - 1];
	} else {
		if (!qdma_dev->mm_mode_en) {
			PMD_DRV_LOG(ERR, "Memory mapped mode not "
					"enabled in the hardware\n");
			rte_free(txq);
			return -EINVAL;
		}

		if (txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			sz = (nb_tx_desc) * (txq->bypass_desc_sz);
		else
			sz = (nb_tx_desc) * sizeof(struct qdma_mm_desc);
		txq->tx_mz = qdma_zone_reserve(dev, "TxHwRn", tx_queue_id,
						sz, socket_id);
		if (!txq->tx_mz) {
			PMD_DRV_LOG(ERR, "Couldn't reserve memory for "
					"MM H2C ring of size %d\n", sz);
			rte_free(txq);
			return -ENOMEM;
		}

		txq->tx_ring = txq->tx_mz->addr;
		tx_ring_mm = (struct qdma_mm_desc *)txq->tx_ring;
		tx_ring_bypass = (uint8_t *)txq->tx_ring;

		/* Write-back status structure */
		if (txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			txq->wb_status = (struct wb_status *)&
				tx_ring_bypass[(nb_tx_desc - 1) *
				(txq->bypass_desc_sz)];
		else
			txq->wb_status = (struct wb_status *)&
				tx_ring_mm[nb_tx_desc - 1];
	}

	PMD_DRV_LOG(INFO, "Tx ring phys addr: 0x%lX, Tx Ring virt addr: 0x%lX",
	    (uint64_t)txq->tx_mz->phys_addr, (uint64_t)txq->tx_ring);

	/* allocate memory for TX software ring */
	sz = nb_tx_desc * sizeof(struct rte_mbuf *);
	txq->sw_ring = rte_zmalloc("TxSwRn", sz, RTE_CACHE_LINE_SIZE);
	if (!txq->sw_ring) {
		PMD_DRV_LOG(ERR, "Memory allocation failed for Tx queue SW ring\n");
		rte_memzone_free(txq->tx_mz);
		rte_free(txq);
		return -ENOMEM;
	}

	txq->nb_tx_desc = nb_tx_desc;
	txq->queue_id = tx_queue_id;

	txq->port_id = dev->data->port_id;
	txq->func_id = qdma_dev->pf;
	txq->num_queues = dev->data->nb_tx_queues;

	txq->ringszidx = index_of_array(g_ring_sz, QDMA_GLOBAL_CSR_ARRAY_SZ,
					txq->nb_tx_desc);

	if (txq->ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected Ring size %d not found\n",
				txq->nb_tx_desc);
		rte_memzone_free(txq->tx_mz);
		rte_free(txq->sw_ring);
		rte_free(txq);
		return -EINVAL;
	}

	txq->tx_deferred_start = tx_conf->tx_deferred_start;

	if (!qdma_dev->is_vf) {
		txq->tx_pidx = (volatile uint32_t *)(dma_bar +
							QDMA_TRQ_SEL_QUEUE_PF +
							QDMA_H2C_PIDX_Q_OFF +
							(tx_queue_id * 0x10));
	} else {
		txq->tx_pidx = (volatile uint32_t *)(dma_bar +
							QDMA_TRQ_SEL_QUEUE_VF +
							QDMA_H2C_PIDX_Q_OFF +
							(tx_queue_id * 0x10));
	}
	rte_spinlock_init(&txq->pidx_update_lock);
	/* store tx_pkt_burst function pointer */
	dev->tx_pkt_burst = qdma_xmit_pkts;

	dev->data->tx_queues[tx_queue_id] = txq;

	return 0;
}

#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
void qdma_txq_pidx_update(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_tx_queue *txq;
	uint32_t qid;

	for (qid = 0; qid < dev->data->nb_tx_queues; qid++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
		if (txq->tx_desc_pend) {
			rte_spinlock_lock(&txq->pidx_update_lock);
			if (txq->tx_desc_pend) {
				*txq->tx_pidx = txq->tx_tail;
				txq->tx_desc_pend = 0;
			}
			rte_spinlock_unlock(&txq->pidx_update_lock);
		}
	}
	rte_eal_alarm_set(QDMA_TXQ_PIDX_UPDATE_INTERVAL,
			qdma_txq_pidx_update, (void *)arg);
}
#endif



void qdma_dev_tx_queue_release(void *tqueue)
{
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)tqueue;

	if (txq) {
		PMD_DRV_LOG(INFO, "Remove H2C queue: %d", txq->queue_id);

		if (txq->sw_ring)
			rte_free(txq->sw_ring);
		if (txq->tx_mz)
			rte_memzone_free(txq->tx_mz);
		rte_free(txq);
		PMD_DRV_LOG(INFO, "H2C queue %d removed", txq->queue_id);
	}
}

void qdma_dev_rx_queue_release(void *rqueue)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)rqueue;

	if (rxq) {
		PMD_DRV_LOG(INFO, "Remove C2H queue: %d", rxq->queue_id);

		if (rxq->sw_ring)
			rte_free(rxq->sw_ring);
		if (rxq->st_mode) { /** if ST-mode **/
			if (rxq->rx_cmpt_mz)
				rte_memzone_free(rxq->rx_cmpt_mz);
		}
		if (rxq->rx_mz)
			rte_memzone_free(rxq->rx_mz);
		rte_free(rxq);
		PMD_DRV_LOG(INFO, "C2H queue %d removed", rxq->queue_id);
	}
}

/**
 * DPDK callback to start the device.
 *
 * Start the device by configuring the Rx/Tx descriptor and device registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
static int qdma_dev_start(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	uint32_t qid;
	int err;

	PMD_DRV_LOG(INFO, "qdma-dev-start: Starting\n");

	/* prepare descriptor rings for operation */
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!txq->tx_deferred_start) {
			err = qdma_dev_tx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}

	for (qid = 0; qid < dev->data->nb_rx_queues; qid++) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!rxq->rx_deferred_start) {
			err = qdma_dev_rx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}

	/* Start Tx h2c engine */
	*qdma_dev->h2c_mm_control = QDMA_MM_CTRL_START;

	/* Start Rx c2h engine */
	*qdma_dev->c2h_mm_control = QDMA_MM_CTRL_START;
#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	rte_eal_alarm_set(QDMA_TXQ_PIDX_UPDATE_INTERVAL,
			qdma_txq_pidx_update, (void *)dev);
#endif
	return 0;
}

/**
 * DPDK callback to retrieve the physical link information.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param wait_to_complete
 *   wait_to_complete field is ignored.
 */
static int qdma_dev_link_update(struct rte_eth_dev *dev,
				__rte_unused int wait_to_complete)
{
	dev->data->dev_link.link_status = ETH_LINK_UP;
	dev->data->dev_link.link_duplex = ETH_LINK_FULL_DUPLEX;
	dev->data->dev_link.link_speed = ETH_SPEED_NUM_100G;
	PMD_DRV_LOG(INFO, "Link update done\n");
	return 0;
}


/**
 * DPDK callback to get information about the device.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param[out] dev_info
 *   Device information structure output buffer.
 */
static void qdma_dev_infos_get(__rte_unused struct rte_eth_dev *dev,
				struct rte_eth_dev_info *dev_info)
{
	//struct qdma_pci_dev *xdev = dev->data->dev_private;

//	xdev->magic		= MAGIC_DEVICE;
	dev_info->max_rx_queues = QDMA_QUEUES_NUM_MAX;
	dev_info->max_tx_queues = QDMA_QUEUES_NUM_MAX;
	dev_info->min_rx_bufsize = QDMA_MIN_RXBUFF_SIZE;
	dev_info->max_rx_pktlen = DMA_BRAM_SIZE;
	dev_info->max_mac_addrs = 1;
}
/**
 * DPDK callback to stop the device.
 *
 * Stop the device by clearing all configured Rx/Tx queue
 * descriptors and registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 */
static void qdma_dev_stop(struct rte_eth_dev *dev)
{
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
#endif
	uint32_t qid;

	/* reset driver's internal queue structures to default values */
	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) Stop H2C & C2H queues", qdma_dev->pf);
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++)
		qdma_dev_tx_queue_stop(dev, qid);
	for (qid = 0; qid < dev->data->nb_rx_queues; qid++)
		qdma_dev_rx_queue_stop(dev, qid);
}

/**
 * DPDK callback to close the device.
 *
 * Destroy all queues and objects, free memory.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 */
static void qdma_dev_close(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	uint64_t bar_addr;
#if FMAP_CNTXT
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;
	struct queue_ind_prg *q_regs;
#else
	uint64_t reg_addr;
#endif
	uint32_t qid;

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) DEV Close\n", qdma_dev->pf);

#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	/* Cancel pending PIDX updates */
	rte_eal_alarm_cancel(qdma_txq_pidx_update, (void *)dev);
#endif

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
#if FMAP_CNTXT
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
						QDMA_IND_Q_PRG_OFF);
	ctxt_sel = (QDMA_CTXT_SEL_FMAP << CTXT_SEL_SHIFT_B) | (qdma_dev->pf <<
								QID_SHIFT_B) |
		    (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, " Read cmd for device:%d: reg_val:%x\n",
				qdma_dev->pf, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
#else
	reg_addr = (uint64_t)(bar_addr + QDMA_TRQ_SEL_FMAP + qdma_dev->pf * 4);
	qdma_write_reg((uint64_t)reg_addr, 0x0);
#endif
		/* iterate over rx queues */
	for (qid = 0; qid < dev->data->nb_rx_queues; ++qid) {
		rxq = dev->data->rx_queues[qid];
		if (rxq) {
			PMD_DRV_LOG(INFO, "Remove C2H queue: %d", qid);

			if (rxq->sw_ring)
				rte_free(rxq->sw_ring);
			if (rxq->st_mode) { /** if ST-mode **/
				if (rxq->rx_cmpt_mz)
					rte_memzone_free(rxq->rx_cmpt_mz);
			}
			if (rxq->rx_mz)
				rte_memzone_free(rxq->rx_mz);
			rte_free(rxq);
			PMD_DRV_LOG(INFO, "C2H queue %d removed", qid);
		}
	}
	/* iterate over tx queues */
	for (qid = 0; qid < dev->data->nb_tx_queues; ++qid) {
		txq = dev->data->tx_queues[qid];
		if (txq) {
			PMD_DRV_LOG(INFO, "Remove H2C queue: %d", qid);

			if (txq->sw_ring)
				rte_free(txq->sw_ring);
			if (txq->tx_mz)
				rte_memzone_free(txq->tx_mz);
			rte_free(txq);
			PMD_DRV_LOG(INFO, "H2C queue %d removed", qid);
		}
	}
}


/**
 * DPDK callback for Ethernet device configuration.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
static int qdma_dev_configure(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t qid = 0;
	int ret = 0;

	PMD_DRV_LOG(INFO, "Configure the qdma engines\n");

	qdma_dev->qsets_en = RTE_MAX(dev->data->nb_rx_queues,
					dev->data->nb_tx_queues);
	if ((qdma_dev->queue_base + qdma_dev->qsets_en) > qdma_dev->qsets_max) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Error: Number of Queues to be"
				" configured are greater than the queues"
				" supported by the hardware\n", qdma_dev->pf);
		qdma_dev->qsets_en = 0;
		return -1;
	}

	qdma_dev->q_info = rte_zmalloc("qinfo", sizeof(struct queue_info) *
					(qdma_dev->qsets_en), 0);
	if (!qdma_dev->q_info) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Cannot allocate "
				"memory for queue info\n", qdma_dev->pf);
		return (-ENOMEM);
	}

	/* Initialize queue_modes to all 1's ( i.e. Streaming) */
	for (qid = 0 ; qid < qdma_dev->qsets_en; qid++)
		qdma_dev->q_info[qid].queue_mode = STREAMING_MODE;

	for (qid = 0 ; qid < dev->data->nb_rx_queues; qid++) {
		qdma_dev->q_info[qid].cmpt_desc_sz = qdma_dev->cmpt_desc_len;
		qdma_dev->q_info[qid].rx_bypass_mode =
						qdma_dev->c2h_bypass_mode;
	}

	for (qid = 0 ; qid < dev->data->nb_tx_queues; qid++)
		qdma_dev->q_info[qid].tx_bypass_mode =
						qdma_dev->h2c_bypass_mode;

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) qdma-dev-configure:\n", qdma_dev->pf);

	/** FMAP configuration **/
	ret = qdma_set_fmap(qdma_dev, qdma_dev->pf, qdma_dev->queue_base,
			    qdma_dev->qsets_en);
	if (ret < 0) {
		rte_free(qdma_dev->q_info);
		return -1;
	}

	/* Start Tx h2c engine */
	*qdma_dev->h2c_mm_control = QDMA_MM_CTRL_START;
	/* Start Rx c2h engine */
	*qdma_dev->c2h_mm_control = QDMA_MM_CTRL_START;

	return 0;
}

int qdma_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	uint64_t phys_addr;
	uint32_t queue_base =  qdma_dev->queue_base;
	int err, bypass_desc_sz_idx;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
	bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	qdma_reset_tx_queue(txq);
	qdma_reset_tx_queues(dev, (qid + queue_base), txq->st_mode);

	rte_delay_ms(PF_START_DELAY);

	phys_addr = (uint64_t)txq->tx_mz->phys_addr;

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) Phys-addr for H2C queue-id:%d: "
			"is Lo:0x%lx, Hi:0x%lx\n", qdma_dev->pf, qid,
			rte_cpu_to_le_32(phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(phys_addr >> 32));

	err = qdma_queue_ctxt_tx_prog(dev, (qid + queue_base), txq->st_mode,
					txq->ringszidx, txq->func_id, phys_addr,
					txq->en_bypass, bypass_desc_sz_idx);
	if (err != 0)
		return err;
	qdma_start_tx_queue(txq);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	txq->status = RTE_ETH_QUEUE_STATE_STARTED;
	return 0;
}


int qdma_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	uint64_t phys_addr, cmpt_phys_addr;
	uint32_t queue_base =  qdma_dev->queue_base;
	uint8_t en_pfetch;
	uint8_t cmpt_desc_fmt;
	int err, bypass_desc_sz_idx;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	qdma_reset_rx_queue(rxq);
	qdma_reset_rx_queues(dev, (qid + queue_base), rxq->st_mode);
	bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);

	rte_delay_ms(1);
	en_pfetch = (rxq->en_prefetch) ? 1 : 0;
	phys_addr = (uint64_t)rxq->rx_mz->phys_addr;

	switch (rxq->cmpt_desc_len) {
	case CMPT_DESC_LEN_8B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	case CMPT_DESC_LEN_16B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_16B;
		break;
	case CMPT_DESC_LEN_32B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_32B;
		break;
	case CMPT_DESC_LEN_64B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_64B;
		break;
	default:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	}

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) Phys-addr for C2H queue-id:%d: "
			"is Lo:0x%lx, Hi:0x%lx\n",
			qdma_dev->pf, qid,
			rte_cpu_to_le_32(phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(phys_addr >> 32));

	if (rxq->st_mode)
		cmpt_phys_addr = (uint64_t)rxq->rx_cmpt_mz->phys_addr;
	else
		cmpt_phys_addr = 0;

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) C2H Completion Phys-addr for "
			"queue-id:%d: is Lo:0x%lx, Hi:0x%lx\n",
			qdma_dev->pf, qid,
			rte_cpu_to_le_32(cmpt_phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(cmpt_phys_addr >> 32));

	err = qdma_start_rx_queue(rxq);
	if (err != 0)
		return err;
	err = qdma_queue_ctxt_rx_prog(dev, (qid + queue_base), rxq->st_mode,
					en_pfetch, rxq->ringszidx,
					rxq->cmpt_ringszidx, rxq->buffszidx,
					rxq->threshidx,	rxq->timeridx,
					rxq->triggermode, rxq->func_id,
					phys_addr, cmpt_phys_addr,
					cmpt_desc_fmt, rxq->en_bypass,
					rxq->en_bypass_prefetch,
					bypass_desc_sz_idx,
					rxq->dis_overflow_check);
	if (err != 0)
		return err;

	if (rxq->st_mode) {
		rte_wmb();
		/* enable status desc , loading the triggermode,
		 * thresidx and timeridx passed from the user
		 */
		*rxq->rx_cidx = (CMPT_STATUS_DESC_EN <<
					CMPT_STATUS_DESC_EN_SHIFT) |
				(rxq->triggermode << CMPT_TRIGGER_MODE_SHIFT) |
				(rxq->threshidx << CMPT_THREHOLD_CNT_SHIFT) |
				(rxq->timeridx << CMPT_TIMER_CNT_SHIFT);
		*rxq->rx_pidx = (rxq->nb_rx_desc - 2);
	}

	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	rxq->status = RTE_ETH_QUEUE_STATE_STARTED;
	return 0;
}

int qdma_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	uint32_t queue_base =  qdma_dev->queue_base;
	int i = 0;
	int cnt = 0;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	rxq->status = RTE_ETH_QUEUE_STATE_STOPPED;

	/* Wait for queue to recv all packets. */
	if (rxq->st_mode) {  /** ST-mode **/
		while (rxq->wb_status->pidx != rxq->rx_cmpt_tail) {
			usleep(10);
			if (cnt++ > 10000)
				break;
		}
	} else { /* MM mode */
		while (rxq->wb_status->cidx != rxq->rx_tail) {
			usleep(10);
			if (cnt++ > 10000)
				break;
		}
	}

	qdma_inv_rx_queues(dev, (qid + queue_base), rxq->st_mode);
	qdma_reset_rx_queues(dev, (qid + queue_base), rxq->st_mode);

	if (rxq->st_mode) {  /** ST-mode **/
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id %d,"
				"mbuf_avail_count = %d, mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
		for (i = 0; i < rxq->nb_rx_desc - 1; i++) {
			rte_pktmbuf_free(rxq->sw_ring[i]);
			rxq->sw_ring[i] = NULL;
		}
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id %d,"
				"mbuf_avail_count = %d, mbuf_in_use_count = %d",
			__func__, __LINE__, rxq->queue_id,
			rte_mempool_avail_count(rxq->mb_pool),
			rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
	}

	qdma_reset_rx_queue(rxq);

	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;

	return 0;
}

int qdma_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t queue_base =  qdma_dev->queue_base;
	struct qdma_tx_queue *txq;
	int cnt = 0;
	uint16_t count;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

	txq->status = RTE_ETH_QUEUE_STATE_STOPPED;
	/* Wait for TXQ to send out all packets. */
	while (txq->wb_status->cidx != txq->tx_tail) {
		usleep(10);
		if (cnt++ > 10000)
			break;
	}

	qdma_inv_tx_queues(dev, (qid + queue_base), txq->st_mode);
	qdma_reset_tx_queues(dev, (qid + queue_base), txq->st_mode);

	/* Relinquish pending mbufs */
	for (count = 0; count < txq->nb_tx_desc - 1; count++) {
		rte_pktmbuf_free(txq->sw_ring[count]);
		txq->sw_ring[count] = NULL;
	}
	qdma_reset_tx_queue(txq);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;

	return 0;
}

static int qdma_dev_stats_get(struct rte_eth_dev *dev,
			      struct rte_eth_stats *eth_stats)
{
	unsigned int i;

	eth_stats->opackets = 0;
	eth_stats->obytes = 0;
	eth_stats->ipackets = 0;
	eth_stats->ibytes = 0;

	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		struct qdma_rx_queue *rxq =
			(struct qdma_rx_queue *)dev->data->rx_queues[i];

		eth_stats->q_ipackets[i] = rxq->stats.pkts;
		eth_stats->q_ibytes[i] = rxq->stats.bytes;
		eth_stats->ipackets += eth_stats->q_ipackets[i];
		eth_stats->ibytes += eth_stats->q_ibytes[i];
	}

	for (i = 0; i < dev->data->nb_tx_queues; i++) {
		struct qdma_tx_queue *txq =
			(struct qdma_tx_queue *)dev->data->tx_queues[i];

		eth_stats->q_opackets[i] = txq->stats.pkts;
		eth_stats->q_obytes[i] = txq->stats.bytes;
		eth_stats->opackets += eth_stats->q_opackets[i];
		eth_stats->obytes   += eth_stats->q_obytes[i];
	}
	return 0;
}

static struct eth_dev_ops qdma_eth_dev_ops = {
	.dev_configure        = qdma_dev_configure,
	.dev_infos_get        = qdma_dev_infos_get,
	.dev_start            = qdma_dev_start,
	.dev_stop             = qdma_dev_stop,
	.dev_close            = qdma_dev_close,
	.link_update          = qdma_dev_link_update,
	.rx_queue_setup       = qdma_dev_rx_queue_setup,
	.tx_queue_setup       = qdma_dev_tx_queue_setup,
	.rx_queue_release	  = qdma_dev_rx_queue_release,
	.tx_queue_release	  = qdma_dev_tx_queue_release,
	.rx_queue_start	  = qdma_dev_rx_queue_start,
	.rx_queue_stop	  = qdma_dev_rx_queue_stop,
	.tx_queue_start	  = qdma_dev_tx_queue_start,
	.tx_queue_stop	  = qdma_dev_tx_queue_stop,
	.stats_get		  = qdma_dev_stats_get,
};

void qdma_dev_ops_init(struct rte_eth_dev *dev)
{
	dev->dev_ops = &qdma_eth_dev_ops;
	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		dev->rx_pkt_burst = &qdma_recv_pkts;
		dev->tx_pkt_burst = &qdma_xmit_pkts;
	}
}
