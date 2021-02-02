/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2021 Xilinx, Inc. All rights reserved.
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
#include <rte_atomic.h>
#include <unistd.h>
#include <string.h>

#include "qdma.h"
#include "qdma_access_common.h"
#include "qdma_mbox_protocol.h"
#include "qdma_mbox.h"
#include "qdma_reg_dump.h"
#include "qdma_platform.h"
#include "qdma_devops.h"

#ifdef QDMA_LATENCY_OPTIMIZED
static void qdma_sort_c2h_cntr_th_values(struct qdma_pci_dev *qdma_dev)
{
	uint8_t i, idx = 0, j = 0;
	uint8_t c2h_cntr_val = qdma_dev->g_c2h_cnt_th[0];
	uint8_t least_max = 0;
	int ref_idx = -1;

get_next_idx:
	for (i = 0; i < QDMA_NUM_C2H_COUNTERS; i++) {
		if ((ref_idx >= 0) && (ref_idx == i))
			continue;
		if (qdma_dev->g_c2h_cnt_th[i] < least_max)
			continue;
		c2h_cntr_val = qdma_dev->g_c2h_cnt_th[i];
		idx = i;
		break;
	}
	for (; i < QDMA_NUM_C2H_COUNTERS; i++) {
		if ((ref_idx >= 0) && (ref_idx == i))
			continue;
		if (qdma_dev->g_c2h_cnt_th[i] < least_max)
			continue;
		if (c2h_cntr_val >= qdma_dev->g_c2h_cnt_th[i]) {
			c2h_cntr_val = qdma_dev->g_c2h_cnt_th[i];
			idx = i;
		}
	}
	qdma_dev->sorted_idx_c2h_cnt_th[j] = idx;
	ref_idx = idx;
	j++;
	idx = j;
	least_max = c2h_cntr_val;
	if (j < QDMA_NUM_C2H_COUNTERS)
		goto get_next_idx;
}
#endif //QDMA_LATENCY_OPTIMIZED

int qdma_vf_csr_read(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	int rv, i;
	struct qdma_csr_info csr_info;

	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_csr_read(qdma_dev->func_id, m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0)
		goto free_msg;

	rv = qdma_mbox_vf_csr_get(m->raw_data, &csr_info);
	if (rv < 0)
		goto free_msg;
	for (i = 0; i < QDMA_NUM_RING_SIZES; i++) {
		qdma_dev->g_ring_sz[i] = (uint32_t)csr_info.ringsz[i];
		qdma_dev->g_c2h_buf_sz[i] = (uint32_t)csr_info.bufsz[i];
		qdma_dev->g_c2h_timer_cnt[i] = (uint32_t)csr_info.timer_cnt[i];
		qdma_dev->g_c2h_cnt_th[i] = (uint32_t)csr_info.cnt_thres[i];
#ifdef QDMA_LATENCY_OPTIMIZED
		qdma_sort_c2h_cntr_th_values(qdma_dev);
#endif //QDMA_LATENCY_OPTIMIZED
	}


free_msg:
	qdma_mbox_msg_free(m);
	return rv;
}

int qdma_pf_csr_read(struct rte_eth_dev *dev)
{
	int ret = 0;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	ret = hw_access->qdma_global_csr_conf(dev, 0,
				QDMA_NUM_RING_SIZES, qdma_dev->g_ring_sz,
		QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ);
	if (ret != QDMA_SUCCESS)
		PMD_DRV_LOG(ERR, "qdma_global_csr_conf for ring size "
				  "returned %d", ret);
	if (qdma_dev->dev_cap.st_en || qdma_dev->dev_cap.mm_cmpt_en) {
		ret = hw_access->qdma_global_csr_conf(dev, 0,
				QDMA_NUM_C2H_TIMERS, qdma_dev->g_c2h_timer_cnt,
		QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
		if (ret != QDMA_SUCCESS)
			PMD_DRV_LOG(ERR, "qdma_global_csr_conf for timer count "
					  "returned %d", ret);

		ret = hw_access->qdma_global_csr_conf(dev, 0,
				QDMA_NUM_C2H_COUNTERS, qdma_dev->g_c2h_cnt_th,
		QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
		if (ret != QDMA_SUCCESS)
			PMD_DRV_LOG(ERR, "qdma_global_csr_conf for counter threshold "
					  "returned %d", ret);
#ifdef QDMA_LATENCY_OPTIMIZED
		qdma_sort_c2h_cntr_th_values(qdma_dev);
#endif //QDMA_LATENCY_OPTIMIZED
	}

	if (qdma_dev->dev_cap.st_en) {
		ret = hw_access->qdma_global_csr_conf(dev, 0,
				QDMA_NUM_C2H_BUFFER_SIZES,
				qdma_dev->g_c2h_buf_sz,
				QDMA_CSR_BUF_SZ,
				QDMA_HW_ACCESS_READ);
		if (ret != QDMA_SUCCESS)
			PMD_DRV_LOG(ERR, "qdma_global_csr_conf for buffer sizes "
					  "returned %d", ret);
	}

	if (ret < 0)
		return qdma_dev->hw_access->qdma_get_error_code(ret);

	return ret;
}

static int qdma_pf_fmap_prog(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_fmap_cfg fmap_cfg;
	int ret = 0;

	memset(&fmap_cfg, 0, sizeof(struct qdma_fmap_cfg));

	/** FMAP configuration **/
	fmap_cfg.qbase = qdma_dev->queue_base;
	fmap_cfg.qmax = qdma_dev->qsets_en;
	ret = qdma_dev->hw_access->qdma_fmap_conf(dev,
			qdma_dev->func_id, &fmap_cfg, QDMA_HW_ACCESS_WRITE);
	if (ret < 0)
		return qdma_dev->hw_access->qdma_get_error_code(ret);

	return ret;
}

int qdma_dev_notify_qadd(struct rte_eth_dev *dev, uint32_t qidx_hw,
						enum qdma_dev_q_type q_type)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m;
	int rv = 0;

	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_notify_qadd(qdma_dev->func_id, qidx_hw,
					q_type, m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);

	qdma_mbox_msg_free(m);
	return rv;
}

int qdma_dev_notify_qdel(struct rte_eth_dev *dev, uint32_t qidx_hw,
						enum qdma_dev_q_type q_type)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m;
	int rv = 0;

	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_notify_qdel(qdma_dev->func_id, qidx_hw,
					q_type, m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);

	qdma_mbox_msg_free(m);
	return rv;
}

uint8_t qmda_get_desc_sz_idx(enum rte_pmd_qdma_bypass_desc_len size)
{
	uint8_t ret;
	switch (size) {
	case RTE_PMD_QDMA_BYPASS_DESC_LEN_8B:
		ret = 0;
		break;
	case RTE_PMD_QDMA_BYPASS_DESC_LEN_16B:
		ret = 1;
		break;
	case RTE_PMD_QDMA_BYPASS_DESC_LEN_32B:
		ret = 2;
		break;
	case RTE_PMD_QDMA_BYPASS_DESC_LEN_64B:
		ret = 3;
		break;
	default:
		/* Suppress compiler warnings*/
		ret = 0;
	}
	return ret;
}

static inline int
qdma_rxq_default_mbuf_init(struct qdma_rx_queue *rxq)
{
	uintptr_t p;
	struct rte_mbuf mb = { .buf_addr = 0 };

	mb.nb_segs = 1;
	mb.data_off = RTE_PKTMBUF_HEADROOM;
	mb.port = rxq->port_id;
	rte_mbuf_refcnt_set(&mb, 1);

	/* prevent compiler reordering */
	rte_compiler_barrier();
	p = (uintptr_t)&mb.rearm_data;
	rxq->mbuf_initializer = *(uint64_t *)p;
	return 0;
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
 *   0 on success,
 *   -ENOMEM when memory allocation fails
 *   -ENOTSUP when HW doesn't support the required configuration
 *   -EINVAL on other failure.
 */
int qdma_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
				uint16_t nb_rx_desc, unsigned int socket_id,
				const struct rte_eth_rxconf *rx_conf,
				struct rte_mempool *mb_pool)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq = NULL;
	struct qdma_ul_mm_desc *rx_ring_mm;
	uint32_t sz;
	uint8_t  *rx_ring_bypass;
	int err = 0;

	PMD_DRV_LOG(INFO, "Configuring Rx queue id:%d\n", rx_queue_id);

	if (nb_rx_desc == 0) {
		PMD_DRV_LOG(ERR, "Invalid descriptor ring size %d\n",
				nb_rx_desc);
		return -EINVAL;
	}

	if (!qdma_dev->dev_configured) {
		PMD_DRV_LOG(ERR,
			"Device for Rx queue id %d is not configured yet\n",
			rx_queue_id);
		return -EINVAL;
	}

	if (!qdma_dev->is_vf) {
		err = qdma_dev_increment_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_C2H);
		if (err != QDMA_SUCCESS)
			return -EINVAL;

		if (qdma_dev->q_info[rx_queue_id].queue_mode ==
				RTE_PMD_QDMA_STREAMING_MODE) {
			err = qdma_dev_increment_active_queue(
						qdma_dev->dma_device_index,
						qdma_dev->func_id,
						QDMA_DEV_Q_TYPE_CMPT);
			if (err != QDMA_SUCCESS) {
				qdma_dev_decrement_active_queue(
						qdma_dev->dma_device_index,
						qdma_dev->func_id,
						QDMA_DEV_Q_TYPE_C2H);
				return -EINVAL;
			}
		}
	} else {
		err = qdma_dev_notify_qadd(dev, rx_queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_C2H);
		if (err < 0)
			return -EINVAL;

		if (qdma_dev->q_info[rx_queue_id].queue_mode ==
				RTE_PMD_QDMA_STREAMING_MODE) {
			err = qdma_dev_notify_qadd(dev, rx_queue_id +
							qdma_dev->queue_base,
							QDMA_DEV_Q_TYPE_CMPT);
			if (err < 0) {
				qdma_dev_notify_qdel(dev, rx_queue_id +
							qdma_dev->queue_base,
							QDMA_DEV_Q_TYPE_C2H);
				return -EINVAL;
			}
		}
	}
	if (!qdma_dev->init_q_range) {
		if (qdma_dev->is_vf) {
			err = qdma_vf_csr_read(dev);
			if (err < 0)
				goto rx_setup_err;
		} else {
			err = qdma_pf_csr_read(dev);
			if (err < 0)
				goto rx_setup_err;
		}
		qdma_dev->init_q_range = 1;
	}

	/* allocate rx queue data structure */
	rxq = rte_zmalloc_socket("QDMA_RxQ", sizeof(struct qdma_rx_queue),
						RTE_CACHE_LINE_SIZE, socket_id);
	if (!rxq) {
		PMD_DRV_LOG(ERR, "Unable to allocate structure rxq of "
				"size %d\n",
				(int)(sizeof(struct qdma_rx_queue)));
		err = -ENOMEM;
		goto rx_setup_err;
	}

	rxq->queue_id = rx_queue_id;
	rxq->port_id = dev->data->port_id;
	rxq->func_id = qdma_dev->func_id;
	rxq->mb_pool = mb_pool;
	rxq->dev = dev;
	rxq->st_mode = qdma_dev->q_info[rx_queue_id].queue_mode;
	rxq->nb_rx_desc = (nb_rx_desc + 1);
	/* <= 2018.2 IP
	 * double the cmpl ring size to avoid run out of cmpl entry while
	 * desc. ring still have free entries
	 */
	rxq->nb_rx_cmpt_desc = ((nb_rx_desc * 2) + 1);
	rxq->en_prefetch = qdma_dev->q_info[rx_queue_id].en_prefetch;
	rxq->cmpt_desc_len = qdma_dev->q_info[rx_queue_id].cmpt_desc_sz;
	if ((rxq->cmpt_desc_len == RTE_PMD_QDMA_CMPT_DESC_LEN_64B) &&
		!qdma_dev->dev_cap.cmpt_desc_64b) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) 64B completion entry size is "
			"not supported in this design\n", qdma_dev->func_id);
		return -ENOTSUP;
	}
	rxq->triggermode = qdma_dev->q_info[rx_queue_id].trigger_mode;
	rxq->rx_deferred_start = rx_conf->rx_deferred_start;
	rxq->dump_immediate_data =
			qdma_dev->q_info[rx_queue_id].immediate_data_state;
	rxq->dis_overflow_check =
			qdma_dev->q_info[rx_queue_id].dis_cmpt_ovf_chk;

	if (qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
				RTE_PMD_QDMA_RX_BYPASS_CACHE ||
			qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
			 RTE_PMD_QDMA_RX_BYPASS_SIMPLE)
		rxq->en_bypass = 1;
	if (qdma_dev->q_info[rx_queue_id].rx_bypass_mode ==
			RTE_PMD_QDMA_RX_BYPASS_SIMPLE)
		rxq->en_bypass_prefetch = 1;

	if (qdma_dev->ip_type == EQDMA_SOFT_IP &&
			qdma_dev->vivado_rel >= QDMA_VIVADO_2020_2) {
		if (qdma_dev->dev_cap.desc_eng_mode ==
				QDMA_DESC_ENG_BYPASS_ONLY) {
			PMD_DRV_LOG(ERR,
				"Bypass only mode design "
				"is not supported\n");
			return -ENOTSUP;
		}

		if (rxq->en_bypass &&
				(qdma_dev->dev_cap.desc_eng_mode ==
				QDMA_DESC_ENG_INTERNAL_ONLY)) {
			PMD_DRV_LOG(ERR,
				"Rx qid %d config in bypass "
				"mode not supported on "
				"internal only mode design\n",
				rx_queue_id);
			return -ENOTSUP;
		}
	}

	if (rxq->en_bypass) {
		rxq->bypass_desc_sz =
				qdma_dev->q_info[rx_queue_id].rx_bypass_desc_sz;
		if ((rxq->bypass_desc_sz == RTE_PMD_QDMA_BYPASS_DESC_LEN_64B)
			&&	!qdma_dev->dev_cap.sw_desc_64b) {
			PMD_DRV_LOG(ERR, "PF-%d(DEVFN) C2H bypass descriptor "
				"size of 64B is not supported in this design:\n",
				qdma_dev->func_id);
			return -ENOTSUP;
		}
	}
	/* Calculate the ring index, completion queue ring size,
	 * buffer index and threshold index.
	 * If index is not found , by default use the index as 0
	 */

	/* Find C2H queue ring size index */
	rxq->ringszidx = index_of_array(qdma_dev->g_ring_sz,
					QDMA_NUM_RING_SIZES, rxq->nb_rx_desc);
	if (rxq->ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected Ring size %d not found\n",
				rxq->nb_rx_desc);
		err = -EINVAL;
		goto rx_setup_err;
	}

	/* Find completion ring size index */
	rxq->cmpt_ringszidx = index_of_array(qdma_dev->g_ring_sz,
						QDMA_NUM_RING_SIZES,
						rxq->nb_rx_cmpt_desc);
	if (rxq->cmpt_ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected completion ring size %d not found\n",
				rxq->nb_rx_cmpt_desc);
		err = -EINVAL;
		goto rx_setup_err;
	}

	/* Find Threshold index */
	rxq->threshidx = index_of_array(qdma_dev->g_c2h_cnt_th,
					QDMA_NUM_C2H_COUNTERS,
					rx_conf->rx_thresh.wthresh);
	if (rxq->threshidx < 0) {
		PMD_DRV_LOG(WARNING, "Expected Threshold %d not found,"
				" using the value %d at index 7\n",
				rx_conf->rx_thresh.wthresh,
				qdma_dev->g_c2h_cnt_th[7]);
		rxq->threshidx = 7;
	}

#ifdef QDMA_LATENCY_OPTIMIZED
	uint8_t next_idx;

	/* Initialize sorted_c2h_cntr_idx */
	rxq->sorted_c2h_cntr_idx = index_of_array(
					qdma_dev->sorted_idx_c2h_cnt_th,
					QDMA_NUM_C2H_COUNTERS,
					qdma_dev->g_c2h_cnt_th[rxq->threshidx]);

	/* Initialize pend_pkt_moving_avg */
	rxq->pend_pkt_moving_avg = qdma_dev->g_c2h_cnt_th[rxq->threshidx];

	/* Initialize pend_pkt_avg_thr_hi */
	if (rxq->sorted_c2h_cntr_idx < (QDMA_NUM_C2H_COUNTERS - 1))
		next_idx = qdma_dev->sorted_idx_c2h_cnt_th[
						rxq->sorted_c2h_cntr_idx + 1];
	else
		next_idx = qdma_dev->sorted_idx_c2h_cnt_th[
				rxq->sorted_c2h_cntr_idx];

	rxq->pend_pkt_avg_thr_hi = qdma_dev->g_c2h_cnt_th[next_idx];

	/* Initialize pend_pkt_avg_thr_lo */
	if (rxq->sorted_c2h_cntr_idx > 0)
		next_idx = qdma_dev->sorted_idx_c2h_cnt_th[
						rxq->sorted_c2h_cntr_idx - 1];
	else
		next_idx = qdma_dev->sorted_idx_c2h_cnt_th[
				rxq->sorted_c2h_cntr_idx];

	rxq->pend_pkt_avg_thr_lo = qdma_dev->g_c2h_cnt_th[next_idx];
#endif //QDMA_LATENCY_OPTIMIZED

	/* Find Timer index */
	rxq->timeridx = index_of_array(qdma_dev->g_c2h_timer_cnt,
				QDMA_NUM_C2H_TIMERS,
				qdma_dev->q_info[rx_queue_id].timer_count);
	if (rxq->timeridx < 0) {
		PMD_DRV_LOG(WARNING, "Expected timer %d not found, "
				"using the value %d at index 1\n",
				qdma_dev->q_info[rx_queue_id].timer_count,
				qdma_dev->g_c2h_timer_cnt[1]);
		rxq->timeridx = 1;
	}

	rxq->rx_buff_size = (uint16_t)
				(rte_pktmbuf_data_room_size(rxq->mb_pool) -
				RTE_PKTMBUF_HEADROOM);
	/* Allocate memory for Rx descriptor ring */
	if (rxq->st_mode) {
		if (!qdma_dev->dev_cap.st_en) {
			PMD_DRV_LOG(ERR, "Streaming mode not enabled "
					"in the hardware\n");
			err = -EINVAL;
			goto rx_setup_err;
		}
		/* Find Buffer size index */
		rxq->buffszidx = index_of_array(qdma_dev->g_c2h_buf_sz,
						QDMA_NUM_C2H_BUFFER_SIZES,
						rxq->rx_buff_size);
		if (rxq->buffszidx < 0) {
			PMD_DRV_LOG(ERR, "Expected buffer size %d not found\n",
					rxq->rx_buff_size);
			err = -EINVAL;
			goto rx_setup_err;
		}

		if (rxq->en_bypass &&
		     (rxq->bypass_desc_sz != 0))
			sz = (rxq->nb_rx_desc) * (rxq->bypass_desc_sz);
		else
			sz = (rxq->nb_rx_desc) *
					sizeof(struct qdma_ul_st_c2h_desc);

		rxq->rx_mz = qdma_zone_reserve(dev, "RxHwRn", rx_queue_id,
						sz, socket_id);
		if (!rxq->rx_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_mz "
					"of size %d\n", sz);
			err = -ENOMEM;
			goto rx_setup_err;
		}
		rxq->rx_ring = rxq->rx_mz->addr;
		memset(rxq->rx_ring, 0, sz);

		/* Allocate memory for Rx completion(CMPT) descriptor ring */
		sz = (rxq->nb_rx_cmpt_desc) * rxq->cmpt_desc_len;
		rxq->rx_cmpt_mz = qdma_zone_reserve(dev, "RxHwCmptRn",
						    rx_queue_id, sz, socket_id);
		if (!rxq->rx_cmpt_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_cmpt_mz "
					"of size %d\n", sz);
			err = -ENOMEM;
			goto rx_setup_err;
		}
		rxq->cmpt_ring =
			(union qdma_ul_st_cmpt_ring *)rxq->rx_cmpt_mz->addr;

		/* Write-back status structure */
		rxq->wb_status = (struct wb_status *)((uint64_t)rxq->cmpt_ring +
				 (((uint64_t)rxq->nb_rx_cmpt_desc - 1) *
				  rxq->cmpt_desc_len));
		memset(rxq->cmpt_ring, 0, sz);
	} else {
		if (!qdma_dev->dev_cap.mm_en) {
			PMD_DRV_LOG(ERR, "Memory mapped mode not enabled "
					"in the hardware\n");
			err = -EINVAL;
			goto rx_setup_err;
		}

		if (rxq->en_bypass &&
			(rxq->bypass_desc_sz != 0))
			sz = (rxq->nb_rx_desc) * (rxq->bypass_desc_sz);
		else
			sz = (rxq->nb_rx_desc) * sizeof(struct qdma_ul_mm_desc);
		rxq->rx_mz = qdma_zone_reserve(dev, "RxHwRn",
						rx_queue_id, sz, socket_id);
		if (!rxq->rx_mz) {
			PMD_DRV_LOG(ERR, "Unable to allocate rxq->rx_mz "
					"of size %d\n", sz);
			err = -ENOMEM;
			goto rx_setup_err;
		}
		rxq->rx_ring = rxq->rx_mz->addr;
		rx_ring_mm = (struct qdma_ul_mm_desc *)rxq->rx_mz->addr;
		memset(rxq->rx_ring, 0, sz);

		rx_ring_bypass = (uint8_t *)rxq->rx_mz->addr;
		if (rxq->en_bypass &&
			(rxq->bypass_desc_sz != 0))
			rxq->wb_status = (struct wb_status *)&
					(rx_ring_bypass[(rxq->nb_rx_desc - 1) *
							(rxq->bypass_desc_sz)]);
		else
			rxq->wb_status = (struct wb_status *)&
					 (rx_ring_mm[rxq->nb_rx_desc - 1]);
	}

	/* allocate memory for RX software ring */
	sz = (rxq->nb_rx_desc) * sizeof(struct rte_mbuf *);
	rxq->sw_ring = rte_zmalloc_socket("RxSwRn", sz,
					RTE_CACHE_LINE_SIZE, socket_id);
	if (!rxq->sw_ring) {
		PMD_DRV_LOG(ERR, "Unable to allocate rxq->sw_ring of size %d\n",
									sz);
		err = -ENOMEM;
		goto rx_setup_err;
	}

	qdma_rxq_default_mbuf_init(rxq);

	dev->data->rx_queues[rx_queue_id] = rxq;

	return 0;

rx_setup_err:
	if (!qdma_dev->is_vf) {
		qdma_dev_decrement_active_queue(qdma_dev->dma_device_index,
						qdma_dev->func_id,
						QDMA_DEV_Q_TYPE_C2H);

		if (qdma_dev->q_info[rx_queue_id].queue_mode ==
				RTE_PMD_QDMA_STREAMING_MODE)
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
	} else {
		qdma_dev_notify_qdel(dev, rx_queue_id +
				qdma_dev->queue_base, QDMA_DEV_Q_TYPE_C2H);

		if (qdma_dev->q_info[rx_queue_id].queue_mode ==
				RTE_PMD_QDMA_STREAMING_MODE)
			qdma_dev_notify_qdel(dev, rx_queue_id +
				qdma_dev->queue_base, QDMA_DEV_Q_TYPE_CMPT);
	}

	if (rxq) {
		if (rxq->rx_mz)
			rte_memzone_free(rxq->rx_mz);
		if (rxq->sw_ring)
			rte_free(rxq->sw_ring);
		rte_free(rxq);
	}
	return err;
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
 *   0 on success
 *   -ENOMEM when memory allocation fails
 *   -EINVAL on other failure.
 */
int qdma_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
			    uint16_t nb_tx_desc, unsigned int socket_id,
			    const struct rte_eth_txconf *tx_conf)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq = NULL;
	struct qdma_ul_mm_desc *tx_ring_mm;
	struct qdma_ul_st_h2c_desc *tx_ring_st;
	uint32_t sz;
	uint8_t  *tx_ring_bypass;
	int err = 0;

	PMD_DRV_LOG(INFO, "Configuring Tx queue id:%d with %d desc\n",
		    tx_queue_id, nb_tx_desc);

	if (!qdma_dev->is_vf) {
		err = qdma_dev_increment_active_queue(
				qdma_dev->dma_device_index,
				qdma_dev->func_id,
				QDMA_DEV_Q_TYPE_H2C);
		if (err != QDMA_SUCCESS)
			return -EINVAL;
	} else {
		err = qdma_dev_notify_qadd(dev, tx_queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_H2C);
		if (err < 0)
			return -EINVAL;
	}
	if (!qdma_dev->init_q_range) {
		if (qdma_dev->is_vf) {
			err = qdma_vf_csr_read(dev);
			if (err < 0) {
				PMD_DRV_LOG(ERR, "CSR read failed\n");
				goto tx_setup_err;
			}
		} else {
			err = qdma_pf_csr_read(dev);
			if (err < 0) {
				PMD_DRV_LOG(ERR, "CSR read failed\n");
				goto tx_setup_err;
			}
		}
		qdma_dev->init_q_range = 1;
	}
	/* allocate rx queue data structure */
	txq = rte_zmalloc_socket("QDMA_TxQ", sizeof(struct qdma_tx_queue),
						RTE_CACHE_LINE_SIZE, socket_id);
	if (txq == NULL) {
		PMD_DRV_LOG(ERR, "Memory allocation failed for "
				"Tx queue SW structure\n");
		err = -ENOMEM;
		goto tx_setup_err;
	}

	txq->st_mode = qdma_dev->q_info[tx_queue_id].queue_mode;
	txq->en_bypass = (qdma_dev->q_info[tx_queue_id].tx_bypass_mode) ? 1 : 0;
	txq->bypass_desc_sz = qdma_dev->q_info[tx_queue_id].tx_bypass_desc_sz;

	txq->nb_tx_desc = (nb_tx_desc + 1);
	txq->queue_id = tx_queue_id;
	txq->dev = dev;
	txq->port_id = dev->data->port_id;
	txq->func_id = qdma_dev->func_id;
	txq->num_queues = dev->data->nb_tx_queues;
	txq->tx_deferred_start = tx_conf->tx_deferred_start;

	txq->ringszidx = index_of_array(qdma_dev->g_ring_sz,
					QDMA_NUM_RING_SIZES, txq->nb_tx_desc);
	if (txq->ringszidx < 0) {
		PMD_DRV_LOG(ERR, "Expected Ring size %d not found\n",
				txq->nb_tx_desc);
		err = -EINVAL;
		goto tx_setup_err;
	}

	if (qdma_dev->ip_type == EQDMA_SOFT_IP &&
			qdma_dev->vivado_rel >= QDMA_VIVADO_2020_2) {
		if (qdma_dev->dev_cap.desc_eng_mode ==
				QDMA_DESC_ENG_BYPASS_ONLY) {
			PMD_DRV_LOG(ERR,
				"Bypass only mode design "
				"is not supported\n");
			return -ENOTSUP;
		}

		if (txq->en_bypass &&
				(qdma_dev->dev_cap.desc_eng_mode ==
				QDMA_DESC_ENG_INTERNAL_ONLY)) {
			PMD_DRV_LOG(ERR,
				"Tx qid %d config in bypass "
				"mode not supported on "
				"internal only mode design\n",
				tx_queue_id);
			return -ENOTSUP;
		}
	}

	/* Allocate memory for TX descriptor ring */
	if (txq->st_mode) {
		if (!qdma_dev->dev_cap.st_en) {
			PMD_DRV_LOG(ERR, "Streaming mode not enabled "
					"in the hardware\n");
			err = -EINVAL;
			goto tx_setup_err;
		}

		if (txq->en_bypass &&
			(txq->bypass_desc_sz != 0))
			sz = (txq->nb_tx_desc) * (txq->bypass_desc_sz);
		else
			sz = (txq->nb_tx_desc) *
					sizeof(struct qdma_ul_st_h2c_desc);
		txq->tx_mz = qdma_zone_reserve(dev, "TxHwRn", tx_queue_id, sz,
						socket_id);
		if (!txq->tx_mz) {
			PMD_DRV_LOG(ERR, "Couldn't reserve memory for "
					"ST H2C ring of size %d\n", sz);
			err = -ENOMEM;
			goto tx_setup_err;
		}

		txq->tx_ring = txq->tx_mz->addr;
		tx_ring_st = (struct qdma_ul_st_h2c_desc *)txq->tx_ring;

		tx_ring_bypass = (uint8_t *)txq->tx_ring;
		/* Write-back status structure */
		if (txq->en_bypass &&
			(txq->bypass_desc_sz != 0))
			txq->wb_status = (struct wb_status *)&
					tx_ring_bypass[(txq->nb_tx_desc - 1) *
					(txq->bypass_desc_sz)];
		else
			txq->wb_status = (struct wb_status *)&
					tx_ring_st[txq->nb_tx_desc - 1];
	} else {
		if (!qdma_dev->dev_cap.mm_en) {
			PMD_DRV_LOG(ERR, "Memory mapped mode not "
					"enabled in the hardware\n");
			err = -EINVAL;
			goto tx_setup_err;
		}

		if (txq->en_bypass &&
			(txq->bypass_desc_sz != 0))
			sz = (txq->nb_tx_desc) * (txq->bypass_desc_sz);
		else
			sz = (txq->nb_tx_desc) * sizeof(struct qdma_ul_mm_desc);
		txq->tx_mz = qdma_zone_reserve(dev, "TxHwRn", tx_queue_id,
						sz, socket_id);
		if (!txq->tx_mz) {
			PMD_DRV_LOG(ERR, "Couldn't reserve memory for "
					"MM H2C ring of size %d\n", sz);
			err = -ENOMEM;
			goto tx_setup_err;
		}

		txq->tx_ring = txq->tx_mz->addr;
		tx_ring_mm = (struct qdma_ul_mm_desc *)txq->tx_ring;

		/* Write-back status structure */

		tx_ring_bypass = (uint8_t *)txq->tx_ring;
		if (txq->en_bypass &&
			(txq->bypass_desc_sz != 0))
			txq->wb_status = (struct wb_status *)&
				tx_ring_bypass[(txq->nb_tx_desc - 1) *
				(txq->bypass_desc_sz)];
		else
			txq->wb_status = (struct wb_status *)&
				tx_ring_mm[txq->nb_tx_desc - 1];
	}

	PMD_DRV_LOG(INFO, "Tx ring phys addr: 0x%lX, Tx Ring virt addr: 0x%lX",
	    (uint64_t)txq->tx_mz->iova, (uint64_t)txq->tx_ring);

	/* Allocate memory for TX software ring */
	sz = txq->nb_tx_desc * sizeof(struct rte_mbuf *);
	txq->sw_ring = rte_zmalloc_socket("TxSwRn", sz,
				RTE_CACHE_LINE_SIZE, socket_id);
	if (txq->sw_ring == NULL) {
		PMD_DRV_LOG(ERR, "Memory allocation failed for "
				 "Tx queue SW ring\n");
		err = -ENOMEM;
		goto tx_setup_err;
	}

	rte_spinlock_init(&txq->pidx_update_lock);
	dev->data->tx_queues[tx_queue_id] = txq;

	return 0;

tx_setup_err:
	PMD_DRV_LOG(ERR, " Tx queue setup failed");
	if (!qdma_dev->is_vf)
		qdma_dev_decrement_active_queue(qdma_dev->dma_device_index,
						qdma_dev->func_id,
						QDMA_DEV_Q_TYPE_H2C);
	else
		qdma_dev_notify_qdel(dev, tx_queue_id +
				qdma_dev->queue_base, QDMA_DEV_Q_TYPE_H2C);
	if (txq) {
		if (txq->tx_mz)
			rte_memzone_free(txq->tx_mz);
		if (txq->sw_ring)
			rte_free(txq->sw_ring);
		rte_free(txq);
	}
	return err;
}

#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
void qdma_txq_pidx_update(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	uint32_t qid;

	for (qid = 0; qid < dev->data->nb_tx_queues; qid++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
		if (txq->tx_desc_pend) {
			rte_spinlock_lock(&txq->pidx_update_lock);
			if (txq->tx_desc_pend) {
				qdma_dev->hw_access->qdma_queue_pidx_update(dev,
					qdma_dev->is_vf,
					qid, 0, &txq->q_pidx_info);

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
	struct qdma_pci_dev *qdma_dev;

	if (txq != NULL) {
		PMD_DRV_LOG(INFO, "Remove H2C queue: %d", txq->queue_id);
		qdma_dev = txq->dev->data->dev_private;

		if (!qdma_dev->is_vf)
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_H2C);
		else
			qdma_dev_notify_qdel(txq->dev, txq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_H2C);
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
	struct qdma_pci_dev *qdma_dev = NULL;

	if (rxq != NULL) {
		PMD_DRV_LOG(INFO, "Remove C2H queue: %d", rxq->queue_id);
		qdma_dev = rxq->dev->data->dev_private;

		if (!qdma_dev->is_vf) {
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
		} else {
			qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
					qdma_dev->queue_base,
					QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_CMPT);
		}

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
int qdma_dev_start(struct rte_eth_dev *dev)
{
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
int qdma_dev_link_update(struct rte_eth_dev *dev,
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
int qdma_dev_infos_get(struct rte_eth_dev *dev,
				struct rte_eth_dev_info *dev_info)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	dev_info->max_rx_queues = qdma_dev->dev_cap.num_qs;
	dev_info->max_tx_queues = qdma_dev->dev_cap.num_qs;

	dev_info->min_rx_bufsize = QDMA_MIN_RXBUFF_SIZE;
	dev_info->max_rx_pktlen = DMA_BRAM_SIZE;
	dev_info->max_mac_addrs = 1;

	return 0;
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
int qdma_dev_stop(struct rte_eth_dev *dev)
{
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
#endif
	uint32_t qid;

	/* reset driver's internal queue structures to default values */
	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) Stop H2C & C2H queues",
			qdma_dev->func_id);
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++)
		qdma_dev_tx_queue_stop(dev, qid);
	for (qid = 0; qid < dev->data->nb_rx_queues; qid++)
		qdma_dev_rx_queue_stop(dev, qid);

#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	/* Cancel pending PIDX updates */
	rte_eal_alarm_cancel(qdma_txq_pidx_update, (void *)dev);
#endif

	return 0;
}

/**
 * DPDK callback to close the device.
 *
 * Destroy all queues and objects, free memory.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 */
int qdma_dev_close(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	struct qdma_cmpt_queue *cmptq;
	uint32_t qid;
	struct qdma_fmap_cfg fmap_cfg;
	int ret = 0;

	PMD_DRV_LOG(INFO, "PF-%d(DEVFN) DEV Close\n", qdma_dev->func_id);

	if (dev->data->dev_started)
		qdma_dev_stop(dev);

	memset(&fmap_cfg, 0, sizeof(struct qdma_fmap_cfg));
	qdma_dev->hw_access->qdma_fmap_conf(dev,
		qdma_dev->func_id, &fmap_cfg, QDMA_HW_ACCESS_CLEAR);

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

			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);

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

			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_H2C);
		}
	}
	if (qdma_dev->dev_cap.mm_cmpt_en) {
		/* iterate over cmpt queues */
		for (qid = 0; qid < qdma_dev->qsets_en; ++qid) {
			cmptq = qdma_dev->cmpt_queues[qid];
			if (cmptq != NULL) {
				PMD_DRV_LOG(INFO, "PF-%d(DEVFN) Remove CMPT queue: %d",
						qdma_dev->func_id, qid);
				if (cmptq->cmpt_mz)
					rte_memzone_free(cmptq->cmpt_mz);
				rte_free(cmptq);
				PMD_DRV_LOG(INFO, "PF-%d(DEVFN) CMPT queue %d removed",
						qdma_dev->func_id, qid);
				qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
			}
		}

		if (qdma_dev->cmpt_queues != NULL) {
			rte_free(qdma_dev->cmpt_queues);
			qdma_dev->cmpt_queues = NULL;
		}
	}
	qdma_dev->qsets_en = 0;
	ret = qdma_dev_update(qdma_dev->dma_device_index, qdma_dev->func_id,
			qdma_dev->qsets_en, (int *)&qdma_dev->queue_base);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) qmax update failed: %d\n",
			qdma_dev->func_id, ret);
		return 0;
	}

	qdma_dev->init_q_range = 0;
	rte_free(qdma_dev->q_info);
	qdma_dev->q_info = NULL;
	qdma_dev->dev_configured = 0;

	/* cancel pending polls*/
	if (qdma_dev->is_master)
		rte_eal_alarm_cancel(qdma_check_errors, (void *)dev);

	return 0;
}

/**
 * DPDK callback to reset the device.
 *
 * Uninitialze PF device after waiting for all its VFs to shutdown.
 * Initialize back PF device and then send Reset done mailbox
 * message to all its VFs to come online again.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
int qdma_dev_reset(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_mbox_msg *m = NULL;
	uint32_t vf_device_count = 0;
	uint32_t i = 0;
	int ret = 0;

	/* Get the number of active VFs for this PF device */
	vf_device_count = qdma_dev->vf_online_count;
	qdma_dev->reset_in_progress = 1;

	/* Uninitialze PCI device */
	ret = qdma_eth_dev_uninit(dev);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) uninitialization failed: %d\n",
			qdma_dev->func_id, ret);
		return -1;
	}

	/* Initialize PCI device */
	ret = qdma_eth_dev_init(dev);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) initialization failed: %d\n",
			qdma_dev->func_id, ret);
		return -1;
	}

	/* Send "PF_RESET_DONE" mailbox message from PF to all its VFs,
	 * so that VFs can come online again
	 */
	for (i = 0; i < pci_dev->max_vfs; i++) {
		if (qdma_dev->vfinfo[i].func_id == QDMA_FUNC_ID_INVALID)
			continue;

		m = qdma_mbox_msg_alloc();
		if (!m)
			return -ENOMEM;

		qdma_mbox_compose_pf_reset_done_message(m->raw_data,
				qdma_dev->func_id,
				qdma_dev->vfinfo[i].func_id);
		ret = qdma_mbox_msg_send(dev, m, 0);
		if (ret < 0)
			PMD_DRV_LOG(ERR, "Sending reset failed from PF:%d to VF:%d\n",
				qdma_dev->func_id,
				qdma_dev->vfinfo[i].func_id);

		/* Mark VFs with invalid function id mapping,
		 * and this gets updated when VF comes online again
		 */
		qdma_dev->vfinfo[i].func_id = QDMA_FUNC_ID_INVALID;
	}

	/* Start waiting for a maximum of 60 secs to get all its VFs
	 * to come online that were active before PF reset
	 */
	i = 0;
	while (i < RESET_TIMEOUT) {
		if (qdma_dev->vf_online_count == vf_device_count) {
			PMD_DRV_LOG(INFO,
				"%s: Reset completed for PF-%d(DEVFN)\n",
				__func__, qdma_dev->func_id);
			break;
		}
		rte_delay_ms(1);
		i++;
	}

	if (i >= RESET_TIMEOUT) {
		PMD_DRV_LOG(ERR, "%s: Failed reset for PF-%d(DEVFN)\n",
			__func__, qdma_dev->func_id);
	}

	return ret;
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
int qdma_dev_configure(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t qid = 0;
	int ret = 0, queue_base = -1;
	uint8_t stat_id;

	PMD_DRV_LOG(INFO, "Configure the qdma engines\n");

	qdma_dev->qsets_en = RTE_MAX(dev->data->nb_rx_queues,
					dev->data->nb_tx_queues);
	if (qdma_dev->qsets_en > qdma_dev->dev_cap.num_qs) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Error: Number of Queues to be"
				" configured are greater than the queues"
				" supported by the hardware\n",
				qdma_dev->func_id);
		qdma_dev->qsets_en = 0;
		return -1;
	}

	/* Request queue base from the resource manager */
	ret = qdma_dev_update(qdma_dev->dma_device_index, qdma_dev->func_id,
			qdma_dev->qsets_en, &queue_base);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) queue allocation failed: %d\n",
			qdma_dev->func_id, ret);
		return -1;
	}

	ret = qdma_dev_qinfo_get(qdma_dev->dma_device_index, qdma_dev->func_id,
				(int *)&qdma_dev->queue_base,
				&qdma_dev->qsets_en);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "%s: Error %d querying qbase\n",
				__func__, ret);
		return -1;
	}
	PMD_DRV_LOG(INFO, "Bus: 0x%x, PF-%d(DEVFN) queue_base: %d\n",
		qdma_dev->dma_device_index,
		qdma_dev->func_id,
		qdma_dev->queue_base);

	qdma_dev->q_info = rte_zmalloc("qinfo", sizeof(struct queue_info) *
					(qdma_dev->qsets_en), 0);
	if (!qdma_dev->q_info) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Cannot allocate "
				"memory for queue info\n", qdma_dev->func_id);
		return (-ENOMEM);
	}

	/* Reserve memory for cmptq ring pointers
	 * Max completion queues can be maximum of rx and tx queues.
	 */
	qdma_dev->cmpt_queues = rte_zmalloc("cmpt_queues",
					    sizeof(qdma_dev->cmpt_queues[0]) *
						qdma_dev->qsets_en,
						RTE_CACHE_LINE_SIZE);
	if (qdma_dev->cmpt_queues == NULL) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) cmpt ring pointers memory "
				"allocation failed:\n", qdma_dev->func_id);
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
		return -(ENOMEM);
	}

	for (qid = 0 ; qid < qdma_dev->qsets_en; qid++) {
		/* Initialize queue_modes to all 1's ( i.e. Streaming) */
		qdma_dev->q_info[qid].queue_mode = RTE_PMD_QDMA_STREAMING_MODE;

		/* Disable the cmpt over flow check by default */
		qdma_dev->q_info[qid].dis_cmpt_ovf_chk = 0;

		qdma_dev->q_info[qid].trigger_mode = qdma_dev->trigger_mode;
		qdma_dev->q_info[qid].timer_count =
					qdma_dev->timer_count;
	}

	for (qid = 0 ; qid < dev->data->nb_rx_queues; qid++) {
		qdma_dev->q_info[qid].cmpt_desc_sz = qdma_dev->cmpt_desc_len;
		qdma_dev->q_info[qid].rx_bypass_mode =
						qdma_dev->c2h_bypass_mode;
		qdma_dev->q_info[qid].en_prefetch = qdma_dev->en_desc_prefetch;
		qdma_dev->q_info[qid].immediate_data_state = 0;
	}

	for (qid = 0 ; qid < dev->data->nb_tx_queues; qid++)
		qdma_dev->q_info[qid].tx_bypass_mode =
						qdma_dev->h2c_bypass_mode;
	for (stat_id = 0, qid = 0;
		stat_id < RTE_ETHDEV_QUEUE_STAT_CNTRS;
		stat_id++, qid++) {
		/* Initialize map with qid same as stat_id */
		qdma_dev->tx_qid_statid_map[stat_id] =
			(qid < dev->data->nb_tx_queues) ? qid : -1;
		qdma_dev->rx_qid_statid_map[stat_id] =
			(qid < dev->data->nb_rx_queues) ? qid : -1;
	}

	ret = qdma_pf_fmap_prog(dev);
	if (ret < 0) {
		PMD_DRV_LOG(ERR, "FMAP programming failed\n");
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
		rte_free(qdma_dev->cmpt_queues);
		qdma_dev->cmpt_queues = NULL;
		return ret;
	}

	qdma_dev->dev_configured = 1;

	return 0;
}

int qdma_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	uint32_t queue_base =  qdma_dev->queue_base;
	int err, bypass_desc_sz_idx;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

	memset(&q_sw_ctxt, 0, sizeof(struct qdma_descq_sw_ctxt));

	bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	qdma_reset_tx_queue(txq);
	qdma_clr_tx_queue_ctxts(dev, (qid + queue_base), txq->st_mode);

	if (txq->st_mode) {
		q_sw_ctxt.desc_sz = SW_DESC_CNTXT_H2C_STREAM_DMA;
	} else {
		q_sw_ctxt.desc_sz = SW_DESC_CNTXT_MEMORY_MAP_DMA;
		q_sw_ctxt.is_mm = 1;
	}
	q_sw_ctxt.wbi_chk = 1;
	q_sw_ctxt.wbi_intvl_en = 1;
	q_sw_ctxt.fnc_id = txq->func_id;
	q_sw_ctxt.qen = 1;
	q_sw_ctxt.rngsz_idx = txq->ringszidx;
	q_sw_ctxt.bypass = txq->en_bypass;
	q_sw_ctxt.wbk_en = 1;
	q_sw_ctxt.ring_bs_addr = (uint64_t)txq->tx_mz->iova;

	if (txq->en_bypass &&
		(txq->bypass_desc_sz != 0))
		q_sw_ctxt.desc_sz = bypass_desc_sz_idx;

	/* Set SW Context */
	err = hw_access->qdma_sw_ctx_conf(dev, 0,
			(qid + queue_base), &q_sw_ctxt,
			QDMA_HW_ACCESS_WRITE);
	if (err < 0)
		return qdma_dev->hw_access->qdma_get_error_code(err);

	txq->q_pidx_info.pidx = 0;
	hw_access->qdma_queue_pidx_update(dev, qdma_dev->is_vf,
		qid, 0, &txq->q_pidx_info);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	txq->status = RTE_ETH_QUEUE_STATE_STARTED;
	return 0;
}


int qdma_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	uint32_t queue_base =  qdma_dev->queue_base;
	uint8_t cmpt_desc_fmt;
	int err, bypass_desc_sz_idx;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;
	struct qdma_descq_prefetch_ctxt q_prefetch_ctxt;
	struct qdma_hw_access *hw_access = qdma_dev->hw_access;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	memset(&q_sw_ctxt, 0, sizeof(struct qdma_descq_sw_ctxt));

	qdma_reset_rx_queue(rxq);
	qdma_clr_rx_queue_ctxts(dev, (qid + queue_base), rxq->st_mode);

	bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);

	switch (rxq->cmpt_desc_len) {
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

	err = qdma_init_rx_queue(rxq);
	if (err != 0)
		return err;

	if (rxq->st_mode) {
		memset(&q_cmpt_ctxt, 0, sizeof(struct qdma_descq_cmpt_ctxt));
		memset(&q_prefetch_ctxt, 0,
				sizeof(struct qdma_descq_prefetch_ctxt));

		q_prefetch_ctxt.bypass = (rxq->en_bypass_prefetch) ? 1 : 0;
		q_prefetch_ctxt.bufsz_idx = rxq->buffszidx;
		q_prefetch_ctxt.pfch_en = (rxq->en_prefetch) ? 1 : 0;
		q_prefetch_ctxt.valid = 1;

#ifdef QDMA_LATENCY_OPTIMIZED
		q_cmpt_ctxt.full_upd = 1;
#endif //QDMA_LATENCY_OPTIMIZED
		q_cmpt_ctxt.en_stat_desc = 1;
		q_cmpt_ctxt.trig_mode = rxq->triggermode;
		q_cmpt_ctxt.fnc_id = rxq->func_id;
		q_cmpt_ctxt.counter_idx = rxq->threshidx;
		q_cmpt_ctxt.timer_idx = rxq->timeridx;
		q_cmpt_ctxt.color = CMPT_DEFAULT_COLOR_BIT;
		q_cmpt_ctxt.ringsz_idx = rxq->cmpt_ringszidx;
		q_cmpt_ctxt.bs_addr = (uint64_t)rxq->rx_cmpt_mz->iova;
		q_cmpt_ctxt.desc_sz = cmpt_desc_fmt;
		q_cmpt_ctxt.valid = 1;
		if (qdma_dev->dev_cap.cmpt_ovf_chk_dis)
			q_cmpt_ctxt.ovf_chk_dis = rxq->dis_overflow_check;


		q_sw_ctxt.desc_sz = SW_DESC_CNTXT_C2H_STREAM_DMA;
		q_sw_ctxt.frcd_en = 1;
	} else {
		q_sw_ctxt.desc_sz = SW_DESC_CNTXT_MEMORY_MAP_DMA;
		q_sw_ctxt.is_mm = 1;
		q_sw_ctxt.wbi_chk = 1;
		q_sw_ctxt.wbi_intvl_en = 1;
	}

	q_sw_ctxt.fnc_id = rxq->func_id;
	q_sw_ctxt.qen = 1;
	q_sw_ctxt.rngsz_idx = rxq->ringszidx;
	q_sw_ctxt.bypass = rxq->en_bypass;
	q_sw_ctxt.wbk_en = 1;
	q_sw_ctxt.ring_bs_addr = (uint64_t)rxq->rx_mz->iova;

	if (rxq->en_bypass &&
		(rxq->bypass_desc_sz != 0))
		q_sw_ctxt.desc_sz = bypass_desc_sz_idx;

	/* Set SW Context */
	err = hw_access->qdma_sw_ctx_conf(dev, 1, (qid + queue_base),
			&q_sw_ctxt, QDMA_HW_ACCESS_WRITE);
	if (err < 0)
		return qdma_dev->hw_access->qdma_get_error_code(err);

	if (rxq->st_mode) {
		/* Set Prefetch Context */
		err = hw_access->qdma_pfetch_ctx_conf(dev, (qid + queue_base),
				&q_prefetch_ctxt, QDMA_HW_ACCESS_WRITE);
		if (err < 0)
			return qdma_dev->hw_access->qdma_get_error_code(err);

		/* Set Completion Context */
		err = hw_access->qdma_cmpt_ctx_conf(dev, (qid + queue_base),
				&q_cmpt_ctxt, QDMA_HW_ACCESS_WRITE);
		if (err < 0)
			return qdma_dev->hw_access->qdma_get_error_code(err);

		rte_wmb();
		/* enable status desc , loading the triggermode,
		 * thresidx and timeridx passed from the user
		 */

		rxq->cmpt_cidx_info.counter_idx = rxq->threshidx;
		rxq->cmpt_cidx_info.timer_idx = rxq->timeridx;
		rxq->cmpt_cidx_info.trig_mode = rxq->triggermode;
		rxq->cmpt_cidx_info.wrb_en = 1;
		rxq->cmpt_cidx_info.wrb_cidx = 0;
		hw_access->qdma_queue_cmpt_cidx_update(dev, qdma_dev->is_vf,
			qid, &rxq->cmpt_cidx_info);

		rxq->q_pidx_info.pidx = (rxq->nb_rx_desc - 2);
		hw_access->qdma_queue_pidx_update(dev, qdma_dev->is_vf, qid,
				1, &rxq->q_pidx_info);
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
		/* For eqdma, c2h marker takes care to drain the pipeline */
		if (!(qdma_dev->ip_type == EQDMA_SOFT_IP)) {
			while (rxq->wb_status->pidx !=
					rxq->cmpt_cidx_info.wrb_cidx) {
				usleep(10);
				if (cnt++ > 10000)
					break;
			}
		}
	} else { /* MM mode */
		while (rxq->wb_status->cidx != rxq->q_pidx_info.pidx) {
			usleep(10);
			if (cnt++ > 10000)
				break;
		}
	}

	qdma_inv_rx_queue_ctxts(dev, (qid + queue_base), rxq->st_mode);

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
	while (txq->wb_status->cidx != txq->q_pidx_info.pidx) {
		usleep(10);
		if (cnt++ > 10000)
			break;
	}

	qdma_inv_tx_queue_ctxts(dev, (qid + queue_base), txq->st_mode);

	/* Relinquish pending mbufs */
	for (count = 0; count < txq->nb_tx_desc - 1; count++) {
		rte_pktmbuf_free(txq->sw_ring[count]);
		txq->sw_ring[count] = NULL;
	}
	qdma_reset_tx_queue(txq);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;

	return 0;
}

/**
 * DPDK callback to retrieve device registers and
 * register attributes (number of registers and register size)
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param regs
 *   Pointer to rte_dev_reg_info structure to fill in. If regs->data is
 *   NULL the function fills in the width and length fields. If non-NULL
 *   the registers are put into the buffer pointed at by the data field.
 *
 * @return
 *   0 on success, -ENOTSUP on failure.
 */
int
qdma_dev_get_regs(struct rte_eth_dev *dev,
	      struct rte_dev_reg_info *regs)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t *data = regs->data;
	uint32_t reg_length = 0;
	int ret = 0;

	ret = qdma_acc_get_num_config_regs(dev,
				(enum qdma_ip_type)qdma_dev->ip_type,
				&reg_length);
	if (ret < 0 || reg_length == 0) {
		PMD_DRV_LOG(ERR, "%s: Failed to get number of config registers\n",
				__func__);
		return ret;
	}

	if (data == NULL) {
		regs->length = reg_length - 1;
		regs->width = sizeof(uint32_t);
		return 0;
	}

	/* Support only full register dump */
	if ((regs->length == 0) ||
	    (regs->length == (reg_length - 1))) {
		regs->version = 1;
		ret = qdma_acc_get_config_regs(dev, qdma_dev->is_vf,
			(enum qdma_ip_type)qdma_dev->ip_type, data);
		if (ret < 0) {
			PMD_DRV_LOG(ERR, "%s: Failed to get config registers\n",
					__func__);
		}
		return ret;
	}

	PMD_DRV_LOG(ERR, "%s: Unsupported length (0x%x) requested\n",
				__func__, regs->length);
	return -ENOTSUP;
}

/**
 * DPDK callback to set a queue statistics mapping for
 * a tx/rx queue of an Ethernet device.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param queue_id
 *   Index of the queue for which a queue stats mapping is required.
 * @param stat_idx
 *   The per-queue packet statistics functionality number that
 *   the queue_id is to be assigned.
 * @param is_rx
 *   Whether queue is a Rx or a Tx queue.
 *
 * @return
 *   0 on success, -EINVAL on failure.
 */
int qdma_dev_queue_stats_mapping(struct rte_eth_dev *dev,
					     uint16_t queue_id,
					     uint8_t stat_idx,
					     uint8_t is_rx)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (is_rx && (queue_id >= dev->data->nb_rx_queues)) {
		PMD_DRV_LOG(ERR, "%s: Invalid Rx qid %d\n",
			__func__, queue_id);
		return -EINVAL;
	}

	if (!is_rx && (queue_id >= dev->data->nb_tx_queues)) {
		PMD_DRV_LOG(ERR, "%s: Invalid Tx qid %d\n",
			__func__, queue_id);
		return -EINVAL;
	}

	if (stat_idx >= RTE_ETHDEV_QUEUE_STAT_CNTRS) {
		PMD_DRV_LOG(ERR, "%s: Invalid stats index %d\n",
			__func__, stat_idx);
		return -EINVAL;
	}

	if (is_rx)
		qdma_dev->rx_qid_statid_map[stat_idx] = queue_id;
	else
		qdma_dev->tx_qid_statid_map[stat_idx] = queue_id;

	return 0;
}

/**
 * DPDK callback for retrieving Port statistics.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param eth_stats
 *   Pointer to structure containing statistics.
 *
 * @return
 *   Returns 0 i.e. success
 */
int qdma_dev_stats_get(struct rte_eth_dev *dev,
			      struct rte_eth_stats *eth_stats)
{
	uint32_t i;
	int qid;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	memset(eth_stats, 0, sizeof(struct rte_eth_stats));
	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[i];
		eth_stats->ipackets += rxq->stats.pkts;
		eth_stats->ibytes += rxq->stats.bytes;
	}

	for (i = 0; i < RTE_ETHDEV_QUEUE_STAT_CNTRS; i++) {
		qid = qdma_dev->rx_qid_statid_map[i];
		if (qid >= 0) {
			rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
			eth_stats->q_ipackets[i] = rxq->stats.pkts;
			eth_stats->q_ibytes[i] = rxq->stats.bytes;
		}
	}

	for (i = 0; i < dev->data->nb_tx_queues; i++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[i];
		eth_stats->opackets += txq->stats.pkts;
		eth_stats->obytes   += txq->stats.bytes;
	}

	for (i = 0; i < RTE_ETHDEV_QUEUE_STAT_CNTRS; i++) {
		qid = qdma_dev->tx_qid_statid_map[i];
		if (qid >= 0) {
			txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
			eth_stats->q_opackets[i] = txq->stats.pkts;
			eth_stats->q_obytes[i] = txq->stats.bytes;
		}
	}
	return 0;
}

/**
 * DPDK callback to reset Port statistics.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 */
int qdma_dev_stats_reset(struct rte_eth_dev *dev)
{
	uint32_t i;

	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		struct qdma_rx_queue *rxq =
			(struct qdma_rx_queue *)dev->data->rx_queues[i];
		rxq->stats.pkts = 0;
		rxq->stats.bytes = 0;
	}

	for (i = 0; i < dev->data->nb_tx_queues; i++) {
		struct qdma_tx_queue *txq =
			(struct qdma_tx_queue *)dev->data->tx_queues[i];
		txq->stats.pkts = 0;
		txq->stats.bytes = 0;
	}
	return 0;
}

/**
 * DPDK callback to get Rx Queue info of an Ethernet device.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 * @param qinfo
 *   A pointer to a structure of type rte_eth_rxq_info_info to be filled with
 *   the information of given Rx queue.
 */
void
qdma_dev_rxq_info_get(struct rte_eth_dev *dev, uint16_t rx_queue_id,
		     struct rte_eth_rxq_info *qinfo)
{
	struct qdma_pci_dev *dma_priv;
	struct qdma_rx_queue *rxq = NULL;

	if (!qinfo)
		return;

	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;

	rxq = dev->data->rx_queues[rx_queue_id];
	memset(qinfo, 0, sizeof(struct rte_eth_rxq_info));
	qinfo->mp = rxq->mb_pool;
	qinfo->conf.rx_deferred_start = rxq->rx_deferred_start;
	qinfo->conf.rx_drop_en = 1;
	qinfo->conf.rx_thresh.wthresh = dma_priv->g_c2h_cnt_th[rxq->threshidx];
	qinfo->scattered_rx = 1;
	qinfo->nb_desc = rxq->nb_rx_desc - 1;
}

/**
 * DPDK callback to get Tx Queue info of an Ethernet device.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param tx_queue_id
 *   The TX queue on the Ethernet device for which information will be
 *   retrieved
 * @param qinfo
 *   A pointer to a structure of type rte_eth_txq_info_info to be filled with
 *   the information of given Tx queue.
 */
void
qdma_dev_txq_info_get(struct rte_eth_dev *dev, uint16_t tx_queue_id,
		      struct rte_eth_txq_info *qinfo)
{
	struct qdma_tx_queue *txq = NULL;

	if (!qinfo)
		return;

	txq = dev->data->tx_queues[tx_queue_id];
	qinfo->conf.offloads = txq->offloads;
	qinfo->conf.tx_deferred_start = txq->tx_deferred_start;
	qinfo->conf.tx_rs_thresh = 0;
	qinfo->nb_desc = txq->nb_tx_desc - 1;

}

static struct eth_dev_ops qdma_eth_dev_ops = {
	.dev_configure            = qdma_dev_configure,
	.dev_infos_get            = qdma_dev_infos_get,
	.dev_start                = qdma_dev_start,
	.dev_stop                 = qdma_dev_stop,
	.dev_close                = qdma_dev_close,
	.dev_reset                = qdma_dev_reset,
	.link_update              = qdma_dev_link_update,
	.rx_queue_setup           = qdma_dev_rx_queue_setup,
	.tx_queue_setup           = qdma_dev_tx_queue_setup,
	.rx_queue_release         = qdma_dev_rx_queue_release,
	.tx_queue_release         = qdma_dev_tx_queue_release,
	.rx_queue_start           = qdma_dev_rx_queue_start,
	.rx_queue_stop            = qdma_dev_rx_queue_stop,
	.tx_queue_start           = qdma_dev_tx_queue_start,
	.tx_queue_stop            = qdma_dev_tx_queue_stop,
	.tx_done_cleanup          = qdma_dev_tx_done_cleanup,
	.queue_stats_mapping_set  = qdma_dev_queue_stats_mapping,
	.get_reg                  = qdma_dev_get_regs,
	.stats_get                = qdma_dev_stats_get,
	.stats_reset              = qdma_dev_stats_reset,
	.rxq_info_get             = qdma_dev_rxq_info_get,
	.txq_info_get             = qdma_dev_txq_info_get,
};

void qdma_dev_ops_init(struct rte_eth_dev *dev)
{
	dev->dev_ops = &qdma_eth_dev_ops;
	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		dev->rx_pkt_burst = &qdma_recv_pkts;
		dev->tx_pkt_burst = &qdma_xmit_pkts;
		dev->rx_queue_count = &qdma_dev_rx_queue_count;
		dev->rx_descriptor_status = &qdma_dev_rx_descriptor_status;
		dev->tx_descriptor_status = &qdma_dev_tx_descriptor_status;
	}
}
