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

#include <rte_mbuf.h>
#include <rte_cycles.h>
#include "qdma.h"
#include "qdma_access_common.h"

#include <fcntl.h>
#include <unistd.h>
#include "qdma_rxtx.h"
#include "qdma_devops.h"

#if defined RTE_ARCH_X86_64
#include <immintrin.h>
#include <emmintrin.h>
#define RTE_QDMA_DESCS_PER_LOOP (2)
#endif //RTE_ARCH_X86_64

/******** User logic dependent functions start **********/
static int qdma_ul_extract_st_cmpt_info_v(void *ul_cmpt_entry, void *cmpt_info)
{
	union qdma_ul_st_cmpt_ring *cmpt_data, *cmpt_desc;

	cmpt_desc = (union qdma_ul_st_cmpt_ring *)(ul_cmpt_entry);
	cmpt_data = (union qdma_ul_st_cmpt_ring *)(cmpt_info);

	cmpt_data->data = cmpt_desc->data;
	if (unlikely(!cmpt_desc->desc_used))
		cmpt_data->length = 0;

	return 0;
}

#ifdef QDMA_RX_VEC_X86_64
/* Vector implementation to get packet length from two completion entries */
static void qdma_ul_get_cmpt_pkt_len_v(void *ul_cmpt_entry, __m128i *data)
{
	union qdma_ul_st_cmpt_ring *cmpt_entry1, *cmpt_entry2;
	__m128i pkt_len_shift = _mm_set_epi64x(0, 4);

	cmpt_entry1 = (union qdma_ul_st_cmpt_ring *)(ul_cmpt_entry);
	cmpt_entry2 = cmpt_entry1 + 1;

	/* Read desc statuses backwards to avoid race condition */
	/* Load a pkt desc */
	data[1] = _mm_set_epi64x(0, cmpt_entry2->data);
	/* Find packet length, currently driver needs
	 * only packet length from completion info
	 */
	data[1] = _mm_srl_epi32(data[1], pkt_len_shift);

	/* Load a pkt desc */
	data[0] = _mm_set_epi64x(0, cmpt_entry1->data);
	/* Find packet length, currently driver needs
	 * only packet length from completion info
	 */
	data[0] = _mm_srl_epi32(data[0], pkt_len_shift);
}
#endif //QDMA_RX_VEC_X86_64

#ifdef QDMA_TX_VEC_X86_64
/* Vector implementation to update H2C descriptor */
static int qdma_ul_update_st_h2c_desc_v(void *qhndl, uint64_t q_offloads,
				struct rte_mbuf *mb)
{
	(void)q_offloads;
	int nsegs = mb->nb_segs;
	uint16_t flags = S_H2C_DESC_F_SOP | S_H2C_DESC_F_EOP;
	uint16_t id;
	struct qdma_ul_st_h2c_desc *tx_ring_st;
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)qhndl;

	tx_ring_st = (struct qdma_ul_st_h2c_desc *)txq->tx_ring;
	id = txq->q_pidx_info.pidx;

	if (nsegs == 1) {
		__m128i descriptor;
		uint16_t datalen = mb->data_len;

		descriptor = _mm_set_epi64x(mb->buf_iova + mb->data_off,
				(uint64_t)datalen << 16 |
				(uint64_t)datalen << 32 |
				(uint64_t)flags << 48);
		_mm_store_si128((__m128i *)&tx_ring_st[id], descriptor);

		id++;
		if (unlikely(id >= (txq->nb_tx_desc - 1)))
			id -= (txq->nb_tx_desc - 1);
	} else {
		int pkt_segs = nsegs;
		while (nsegs && mb) {
			__m128i descriptor;
			uint16_t datalen = mb->data_len;

			flags = 0;
			if (nsegs == pkt_segs)
				flags |= S_H2C_DESC_F_SOP;
			if (nsegs == 1)
				flags |= S_H2C_DESC_F_EOP;

			descriptor = _mm_set_epi64x(mb->buf_iova + mb->data_off,
					(uint64_t)datalen << 16 |
					(uint64_t)datalen << 32 |
					(uint64_t)flags << 48);
			_mm_store_si128((__m128i *)&tx_ring_st[id], descriptor);

			nsegs--;
			mb = mb->next;
			id++;
			if (unlikely(id >= (txq->nb_tx_desc - 1)))
				id -= (txq->nb_tx_desc - 1);
		}
	}

	txq->q_pidx_info.pidx = id;

	return 0;
}
#endif //QDMA_TX_VEC_X86_64

/******** User logic dependent functions end **********/

/**
 * Poll the QDMA engine for transfer completion.
 *
 * @param txq
 *   Generic pointer to either Rx/Tx queue structure based on the DMA direction.
 * @param expected_count
 *   expected transfer count.
 *
 * @return
 *   Number of packets transferred successfully by the engine.
 */
static int dma_wb_monitor(void *xq, uint8_t dir, uint16_t expected_count)
{
	struct wb_status *wb_status;
	uint16_t mode, wb_tail;
	uint32_t i = 0;

	if (dir == DMA_TO_DEVICE) {
		struct qdma_tx_queue *txq = (struct qdma_tx_queue *)xq;
		wb_status = txq->wb_status;

		while (i < WB_TIMEOUT) {
			if (expected_count == wb_status->cidx) {
				PMD_DRV_LOG(DEBUG, "Poll writeback count "
					    "matches to the expected count :%d",
					    expected_count);
				return 0;
			}
			PMD_DRV_LOG(DEBUG, "poll wait on wb-count:%d and "
					"expected-count:%d\n",
					wb_status->cidx, expected_count);
			rte_delay_us(2);
			i++;
		}
		PMD_DRV_LOG(DEBUG, "DMA Engine write-back monitor "
				"timeout error occurred\n");
		return -1;
	}
	/* dir == DMA_FROM_DEVICE */
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)xq;
	wb_status = rxq->wb_status;
	mode = rxq->st_mode;

	/* Poll the writeback location until timeout Or, the expected
	 * count matches to the writeback count
	 */
	while (i < WB_TIMEOUT) {
		if (mode) {
			wb_tail =
			(rxq->cmpt_cidx_info.wrb_cidx + expected_count) %
			(rxq->nb_rx_cmpt_desc - 1);
			if (wb_tail == wb_status->pidx) {
				PMD_DRV_LOG(DEBUG, "ST: Poll cmpt count matches"
						" to the expected count :%d",
						expected_count);
				return 0;
			}
			PMD_DRV_LOG(DEBUG, "ST: poll wait on wb-count:%d and"
					" expected-count:%d\n",
					wb_status->pidx, expected_count);
		} else {
			if (expected_count == wb_status->cidx) {
				PMD_DRV_LOG(DEBUG, "MM: Poll writeback count "
						"matches to the expected count"
						" :%d", expected_count);
				return 0;
			}
			PMD_DRV_LOG(DEBUG, "MM: poll wait on wb-count:%d "
					"and expected-count:%d\n",
					wb_status->cidx, expected_count);
		}
		rte_delay_us(2);
		i++;
	}
	return -1;
}

static int reclaim_tx_mbuf(struct qdma_tx_queue *txq,
			uint16_t cidx, uint16_t free_cnt)
{
	int fl_desc = 0;
	uint16_t count;
	int id;

	id = txq->tx_fl_tail;
	fl_desc = (int)cidx - id;
	if (fl_desc < 0)
		fl_desc += (txq->nb_tx_desc - 1);

	if (free_cnt && (fl_desc > free_cnt))
		fl_desc = free_cnt;

	if ((id + fl_desc) < (txq->nb_tx_desc - 1)) {
		for (count = 0; count < ((uint16_t)fl_desc & 0xFFFF);
				count++) {
			rte_pktmbuf_free(txq->sw_ring[id]);
			txq->sw_ring[id++] = NULL;
		}
	} else {
		fl_desc -= (txq->nb_tx_desc - 1 - id);
		for (; id < (txq->nb_tx_desc - 1); id++) {
			rte_pktmbuf_free(txq->sw_ring[id]);
			txq->sw_ring[id] = NULL;
		}

		id -= (txq->nb_tx_desc - 1);
		for (count = 0; count < ((uint16_t)fl_desc & 0xFFFF);
				count++) {
			rte_pktmbuf_free(txq->sw_ring[id]);
			txq->sw_ring[id++] = NULL;
		}
	}
	txq->tx_fl_tail = id;

	return fl_desc;
}

#ifdef TEST_64B_DESC_BYPASS
static uint16_t qdma_xmit_64B_desc_bypass(struct qdma_tx_queue *txq,
			struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
	uint16_t count, id;
	uint8_t *tx_ring_st_bypass = NULL;
	int ofd = -1, ret = 0;
	char fln[50];
	struct qdma_pci_dev *qdma_dev = txq->dev->data->dev_private;

	id = txq->q_pidx_info.pidx;

	for (count = 0; count < nb_pkts; count++) {
		tx_ring_st_bypass = (uint8_t *)txq->tx_ring;
		memset(&tx_ring_st_bypass[id * (txq->bypass_desc_sz)],
				((id  % 255) + 1), txq->bypass_desc_sz);

		snprintf(fln, sizeof(fln), "q_%u_%s", txq->queue_id,
				"h2c_desc_data.txt");
		ofd = open(fln, O_RDWR | O_CREAT | O_APPEND | O_SYNC,
				0666);
		if (ofd < 0) {
			PMD_DRV_LOG(INFO, " txq[%d] unable to create "
					"outfile to dump descriptor"
					" data", txq->queue_id);
			return 0;
		}
		ret = write(ofd, &(tx_ring_st_bypass[id *
					(txq->bypass_desc_sz)]),
					txq->bypass_desc_sz);
		if (ret < txq->bypass_desc_sz)
			PMD_DRV_LOG(DEBUG, "Txq[%d] descriptor data "
					"len: %d, written to inputfile"
					" :%d bytes", txq->queue_id,
					txq->bypass_desc_sz, ret);
		close(ofd);

		rte_pktmbuf_free(tx_pkts[count]);

		id++;
		if (unlikely(id >= (txq->nb_tx_desc - 1)))
			id -= (txq->nb_tx_desc - 1);
	}

	/* Make sure writes to the H2C descriptors are synchronized
	 * before updating PIDX
	 */
	rte_wmb();

	txq->q_pidx_info.pidx = id;
	qdma_dev->hw_access->qdma_queue_pidx_update(txq->dev, qdma_dev->is_vf,
		txq->queue_id, 0, &txq->q_pidx_info);

	PMD_DRV_LOG(DEBUG, " xmit completed with count:%d\n", count);

	return count;
}
#endif

uint16_t qdma_get_rx_queue_id(void *queue_hndl)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)queue_hndl;

	return rxq->queue_id;
}

void qdma_get_device_info(void *queue_hndl,
		enum qdma_device_type *device_type,
		enum qdma_ip_type *ip_type)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)queue_hndl;
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;

	*device_type = (enum qdma_device_type)qdma_dev->device_type;
	*ip_type = (enum qdma_ip_type)qdma_dev->ip_type;
}

uint32_t get_mm_c2h_ep_addr(void *queue_hndl)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)queue_hndl;

	return rxq->ep_addr;
}

uint32_t get_mm_buff_size(void *queue_hndl)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)queue_hndl;

	return rxq->rx_buff_size;
}

struct qdma_ul_st_h2c_desc *get_st_h2c_desc(void *queue_hndl)
{
	volatile uint16_t id;
	struct qdma_ul_st_h2c_desc *tx_ring_st;
	struct qdma_ul_st_h2c_desc *desc;
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)queue_hndl;

	id = txq->q_pidx_info.pidx;
	tx_ring_st = (struct qdma_ul_st_h2c_desc *)txq->tx_ring;
	desc = (struct qdma_ul_st_h2c_desc *)&tx_ring_st[id];

	id++;
	if (unlikely(id >= (txq->nb_tx_desc - 1)))
		id -= (txq->nb_tx_desc - 1);

	txq->q_pidx_info.pidx = id;

	return desc;
}

struct qdma_ul_mm_desc *get_mm_h2c_desc(void *queue_hndl)
{
	struct qdma_ul_mm_desc *desc;
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)queue_hndl;
	struct qdma_ul_mm_desc *tx_ring =
					(struct qdma_ul_mm_desc *)txq->tx_ring;
	uint32_t id;

	id = txq->q_pidx_info.pidx;
	desc =  (struct qdma_ul_mm_desc *)&tx_ring[id];

	id = (id + 1) % (txq->nb_tx_desc - 1);
	txq->q_pidx_info.pidx = id;

	return desc;
}

uint32_t get_mm_h2c_ep_addr(void *queue_hndl)
{
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)queue_hndl;

	return txq->ep_addr;
}

#ifdef QDMA_LATENCY_OPTIMIZED
static void adjust_c2h_cntr_avgs(struct qdma_rx_queue *rxq)
{
	int i;
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;

	rxq->pend_pkt_moving_avg =
		qdma_dev->g_c2h_cnt_th[rxq->cmpt_cidx_info.counter_idx];

	if (rxq->sorted_c2h_cntr_idx == (QDMA_GLOBAL_CSR_ARRAY_SZ - 1))
		i = qdma_dev->sorted_idx_c2h_cnt_th[rxq->sorted_c2h_cntr_idx];
	else
		i = qdma_dev->sorted_idx_c2h_cnt_th[
					rxq->sorted_c2h_cntr_idx + 1];

	rxq->pend_pkt_avg_thr_hi = qdma_dev->g_c2h_cnt_th[i];

	if (rxq->sorted_c2h_cntr_idx > 0)
		i = qdma_dev->sorted_idx_c2h_cnt_th[
					rxq->sorted_c2h_cntr_idx - 1];
	else
		i = qdma_dev->sorted_idx_c2h_cnt_th[rxq->sorted_c2h_cntr_idx];

	rxq->pend_pkt_avg_thr_lo = qdma_dev->g_c2h_cnt_th[i];

	PMD_DRV_LOG(DEBUG, "q%u: c2h_cntr_idx =  %u %u %u",
		rxq->queue_id,
		rxq->cmpt_cidx_info.counter_idx,
		rxq->pend_pkt_avg_thr_lo,
		rxq->pend_pkt_avg_thr_hi);
}

static void incr_c2h_cntr_th(struct qdma_rx_queue *rxq)
{
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
	unsigned char i, c2h_cntr_idx;
	unsigned char c2h_cntr_val_new;
	unsigned char c2h_cntr_val_curr;

	if (rxq->sorted_c2h_cntr_idx ==
			(QDMA_NUM_C2H_COUNTERS - 1))
		return;

	rxq->c2h_cntr_monitor_cnt = 0;
	i = rxq->sorted_c2h_cntr_idx;
	c2h_cntr_idx = qdma_dev->sorted_idx_c2h_cnt_th[i];
	c2h_cntr_val_curr = qdma_dev->g_c2h_cnt_th[c2h_cntr_idx];
	i++;
	c2h_cntr_idx = qdma_dev->sorted_idx_c2h_cnt_th[i];
	c2h_cntr_val_new = qdma_dev->g_c2h_cnt_th[c2h_cntr_idx];

	/* Choose the closest counter value */
	if ((c2h_cntr_val_new >= rxq->pend_pkt_moving_avg) &&
		(c2h_cntr_val_new - rxq->pend_pkt_moving_avg) >=
		(rxq->pend_pkt_moving_avg - c2h_cntr_val_curr))
		return;

	/* Do not allow c2h counter value go beyond half of C2H ring sz*/
	if (c2h_cntr_val_new < (qdma_dev->g_ring_sz[rxq->ringszidx] >> 1)) {
		rxq->cmpt_cidx_info.counter_idx = c2h_cntr_idx;
		rxq->sorted_c2h_cntr_idx = i;
		adjust_c2h_cntr_avgs(rxq);
	}
}

static void decr_c2h_cntr_th(struct qdma_rx_queue *rxq)
{
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
	unsigned char i, c2h_cntr_idx;
	unsigned char c2h_cntr_val_new;
	unsigned char c2h_cntr_val_curr;

	if (!rxq->sorted_c2h_cntr_idx)
		return;
	rxq->c2h_cntr_monitor_cnt = 0;
	i = rxq->sorted_c2h_cntr_idx;
	c2h_cntr_idx = qdma_dev->sorted_idx_c2h_cnt_th[i];
	c2h_cntr_val_curr = qdma_dev->g_c2h_cnt_th[c2h_cntr_idx];
	i--;
	c2h_cntr_idx = qdma_dev->sorted_idx_c2h_cnt_th[i];

	c2h_cntr_val_new = qdma_dev->g_c2h_cnt_th[c2h_cntr_idx];

	/* Choose the closest counter value */
	if ((c2h_cntr_val_new <= rxq->pend_pkt_moving_avg) &&
		(rxq->pend_pkt_moving_avg - c2h_cntr_val_new) >=
		(c2h_cntr_val_curr - rxq->pend_pkt_moving_avg))
		return;

	rxq->cmpt_cidx_info.counter_idx = c2h_cntr_idx;

	rxq->sorted_c2h_cntr_idx = i;
	adjust_c2h_cntr_avgs(rxq);
}

#define MAX_C2H_CNTR_STAGNANT_CNT 16
static void adapt_update_counter(struct qdma_rx_queue *rxq,
		uint16_t nb_pkts_avail)
{
	/* Add available pkt count and average */
	rxq->pend_pkt_moving_avg += nb_pkts_avail;
	rxq->pend_pkt_moving_avg >>= 1;

	/* if avg > hi_th, increase the counter
	 * if avg < lo_th, decrease the counter
	 */
	if (rxq->pend_pkt_avg_thr_hi <= rxq->pend_pkt_moving_avg)
		incr_c2h_cntr_th(rxq);
	else if (rxq->pend_pkt_avg_thr_lo >=
				rxq->pend_pkt_moving_avg)
		decr_c2h_cntr_th(rxq);
	else {
		rxq->c2h_cntr_monitor_cnt++;
		if (rxq->c2h_cntr_monitor_cnt == MAX_C2H_CNTR_STAGNANT_CNT) {
			/* go down on counter value to see if we actually are
			 * increasing latency by setting
			 * higher counter threshold
			 */
			decr_c2h_cntr_th(rxq);
			rxq->c2h_cntr_monitor_cnt = 0;
		} else
			return;
	}
}
#endif //QDMA_LATENCY_OPTIMIZED

/* Process completion ring */
static int process_cmpt_ring(struct qdma_rx_queue *rxq,
		uint16_t num_cmpt_entries)
{
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
	union qdma_ul_st_cmpt_ring *user_cmpt_entry;
	uint32_t count = 0;
	int ret = 0;
	uint16_t rx_cmpt_tail = rxq->cmpt_cidx_info.wrb_cidx;

	if (likely(!rxq->dump_immediate_data)) {
		if ((rx_cmpt_tail + num_cmpt_entries) <
			(rxq->nb_rx_cmpt_desc - 1)) {
			for (count = 0; count < num_cmpt_entries; count++) {
				user_cmpt_entry =
				(union qdma_ul_st_cmpt_ring *)
				((uint64_t)rxq->cmpt_ring +
				((uint64_t)rx_cmpt_tail * rxq->cmpt_desc_len));

				ret = qdma_ul_extract_st_cmpt_info_v(
						user_cmpt_entry,
						&rxq->cmpt_data[count]);
				if (ret != 0) {
					PMD_DRV_LOG(ERR, "Error detected on CMPT ring "
						"at index %d, queue_id = %d\n",
						rx_cmpt_tail, rxq->queue_id);
					rxq->err = 1;
					return -1;
				}
				rx_cmpt_tail++;
			}
		} else {
			while (count < num_cmpt_entries) {
				user_cmpt_entry =
				(union qdma_ul_st_cmpt_ring *)
				((uint64_t)rxq->cmpt_ring +
				((uint64_t)rx_cmpt_tail * rxq->cmpt_desc_len));

				ret = qdma_ul_extract_st_cmpt_info_v(
						user_cmpt_entry,
						&rxq->cmpt_data[count]);
				if (ret != 0) {
					PMD_DRV_LOG(ERR, "Error detected on CMPT ring "
						"at index %d, queue_id = %d\n",
						rx_cmpt_tail, rxq->queue_id);
					rxq->err = 1;
					return -1;
				}

				rx_cmpt_tail++;
				if (unlikely(rx_cmpt_tail >=
					(rxq->nb_rx_cmpt_desc - 1)))
					rx_cmpt_tail -=
						(rxq->nb_rx_cmpt_desc - 1);
				count++;
			}
		}
	} else {
		while (count < num_cmpt_entries) {
			user_cmpt_entry =
			(union qdma_ul_st_cmpt_ring *)
			((uint64_t)rxq->cmpt_ring +
			((uint64_t)rx_cmpt_tail * rxq->cmpt_desc_len));

			ret = qdma_ul_extract_st_cmpt_info(
					user_cmpt_entry,
					&rxq->cmpt_data[count]);
			if (ret != 0) {
				PMD_DRV_LOG(ERR, "Error detected on CMPT ring "
					"at CMPT index %d, queue_id = %d\n",
					rx_cmpt_tail, rxq->queue_id);
				rxq->err = 1;
				return -1;
			}

			ret = qdma_ul_process_immediate_data_st((void *)rxq,
					user_cmpt_entry, rxq->cmpt_desc_len);
			if (ret < 0) {
				PMD_DRV_LOG(ERR, "Error processing immediate data "
					"at CMPT index = %d, queue_id = %d\n",
					rx_cmpt_tail, rxq->queue_id);
				return -1;
			}

			rx_cmpt_tail++;
			if (unlikely(rx_cmpt_tail >=
				(rxq->nb_rx_cmpt_desc - 1)))
				rx_cmpt_tail -= (rxq->nb_rx_cmpt_desc - 1);
			count++;
		}
	}

	// Update the CPMT CIDX
	rxq->cmpt_cidx_info.wrb_cidx = rx_cmpt_tail;
	qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(rxq->dev,
		qdma_dev->is_vf,
		rxq->queue_id, &rxq->cmpt_cidx_info);

	return 0;
}

static uint32_t rx_queue_count(void *rx_queue)
{
	struct qdma_rx_queue *rxq = rx_queue;
	struct wb_status *wb_status;
	uint16_t pkt_length;
	uint16_t nb_pkts_avail = 0;
	uint16_t rx_cmpt_tail = 0;
	uint16_t cmpt_pidx;
	uint32_t nb_desc_used = 0, count = 0;
	union qdma_ul_st_cmpt_ring *user_cmpt_entry;
	union qdma_ul_st_cmpt_ring cmpt_data;

	wb_status = rxq->wb_status;
	rx_cmpt_tail = rxq->cmpt_cidx_info.wrb_cidx;
	cmpt_pidx = wb_status->pidx;

	if (rx_cmpt_tail < cmpt_pidx)
		nb_pkts_avail = cmpt_pidx - rx_cmpt_tail;
	else if (rx_cmpt_tail > cmpt_pidx)
		nb_pkts_avail = rxq->nb_rx_cmpt_desc - 1 - rx_cmpt_tail +
				cmpt_pidx;

	if (nb_pkts_avail == 0)
		return 0;

	while (count < nb_pkts_avail) {
		user_cmpt_entry =
		(union qdma_ul_st_cmpt_ring *)((uint64_t)rxq->cmpt_ring +
		((uint64_t)rx_cmpt_tail * rxq->cmpt_desc_len));

		if (qdma_ul_extract_st_cmpt_info(user_cmpt_entry,
				&cmpt_data)) {
			break;
		}

		pkt_length = qdma_ul_get_cmpt_pkt_len(&cmpt_data);
		if (unlikely(!pkt_length)) {
			count++;
			continue;
		}

		nb_desc_used += ((pkt_length/rxq->rx_buff_size) + 1);
		rx_cmpt_tail++;
		if (unlikely(rx_cmpt_tail >= (rxq->nb_rx_cmpt_desc - 1)))
			rx_cmpt_tail -= (rxq->nb_rx_cmpt_desc - 1);
		count++;
	}
	PMD_DRV_LOG(DEBUG, "%s: nb_desc_used = %d",
			__func__, nb_desc_used);
	return nb_desc_used;
}

/**
 * DPDK callback to get the number of used descriptors of a rx queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 *
 * @return
 *   The number of used descriptors in the specific queue.
 */
uint32_t
qdma_dev_rx_queue_count(struct rte_eth_dev *dev, uint16_t rx_queue_id)
{
	return rx_queue_count(dev->data->rx_queues[rx_queue_id]);
}

/**
 * DPDK callback to check the status of a Rx descriptor in the queue.
 *
 * @param rx_queue
 *   Pointer to Rx queue specific data structure.
 * @param offset
 *   The offset of the descriptor starting from tail (0 is the next
 *   packet to be received by the driver).
 *
 * @return
 *  - (RTE_ETH_RX_DESC_AVAIL): Descriptor is available for the hardware to
 *    receive a packet.
 *  - (RTE_ETH_RX_DESC_DONE): Descriptor is done, it is filled by hw, but
 *    not yet processed by the driver (i.e. in the receive queue).
 *  - (RTE_ETH_RX_DESC_UNAVAIL): Descriptor is unavailable, either hold by
 *    the driver and not yet returned to hw, or reserved by the hw.
 *  - (-EINVAL) bad descriptor offset.
 */
int
qdma_dev_rx_descriptor_status(void *rx_queue, uint16_t offset)
{
	struct qdma_rx_queue *rxq = rx_queue;
	uint32_t desc_used_count;
	uint16_t rx_tail, c2h_pidx, pending_desc;

	if (unlikely(offset >= (rxq->nb_rx_desc - 1)))
		return -EINVAL;

	/* One descriptor is reserved so that pidx is not same as tail */
	if (offset == (rxq->nb_rx_desc - 2))
		return RTE_ETH_RX_DESC_UNAVAIL;

	desc_used_count = rx_queue_count(rxq);
	if (offset < desc_used_count)
		return RTE_ETH_RX_DESC_DONE;

	/* If Tail is not same as PIDX, descriptors are held by the driver */
	rx_tail = rxq->rx_tail;
	c2h_pidx = rxq->q_pidx_info.pidx;

	pending_desc = rx_tail - c2h_pidx - 1;
	if (rx_tail < (c2h_pidx + 1))
		pending_desc = rxq->nb_rx_desc - 2 + rx_tail -
				c2h_pidx;

	if (offset < (desc_used_count + pending_desc))
		return RTE_ETH_RX_DESC_UNAVAIL;

	return RTE_ETH_RX_DESC_AVAIL;
}

/* Update mbuf for a segmented packet */
static struct rte_mbuf *prepare_segmented_packet(struct qdma_rx_queue *rxq,
		uint16_t pkt_length, uint16_t *tail)
{
	struct rte_mbuf *mb;
	struct rte_mbuf *first_seg = NULL;
	struct rte_mbuf *last_seg = NULL;
	uint16_t id = *tail;
	uint16_t length;
	uint16_t rx_buff_size = rxq->rx_buff_size;

	do {
		mb = rxq->sw_ring[id];
		rxq->sw_ring[id++] = NULL;
		length = pkt_length;

		if (unlikely(id >= (rxq->nb_rx_desc - 1)))
			id -= (rxq->nb_rx_desc - 1);
		if (pkt_length > rx_buff_size) {
			rte_pktmbuf_data_len(mb) = rx_buff_size;
			pkt_length -= rx_buff_size;
		} else {
			rte_pktmbuf_data_len(mb) = pkt_length;
			pkt_length = 0;
		}
		rte_mbuf_refcnt_set(mb, 1);

		if (first_seg == NULL) {
			first_seg = mb;
			first_seg->nb_segs = 1;
			first_seg->pkt_len = length;
			first_seg->packet_type = 0;
			first_seg->ol_flags = 0;
			first_seg->port = rxq->port_id;
			first_seg->vlan_tci = 0;
			first_seg->hash.rss = 0;
		} else {
			first_seg->nb_segs++;
			if (last_seg != NULL)
				last_seg->next = mb;
		}

		last_seg = mb;
		mb->next = NULL;
	} while (pkt_length);

	*tail = id;
	return first_seg;
}

/* Prepare mbuf for one packet */
static inline
struct rte_mbuf *prepare_single_packet(struct qdma_rx_queue *rxq,
		uint16_t cmpt_idx)
{
	struct rte_mbuf *mb = NULL;
	uint16_t id = rxq->rx_tail;
	uint16_t pkt_length;

	pkt_length = qdma_ul_get_cmpt_pkt_len(&rxq->cmpt_data[cmpt_idx]);

	if (pkt_length) {
		rxq->stats.pkts++;
		rxq->stats.bytes += pkt_length;

		if (likely(pkt_length <= rxq->rx_buff_size)) {
			mb = rxq->sw_ring[id];
			rxq->sw_ring[id++] = NULL;

			if (unlikely(id >= (rxq->nb_rx_desc - 1)))
				id -= (rxq->nb_rx_desc - 1);

			rte_mbuf_refcnt_set(mb, 1);
			mb->nb_segs = 1;
			mb->port = rxq->port_id;
			mb->ol_flags = 0;
			mb->packet_type = 0;
			mb->pkt_len = pkt_length;
			mb->data_len = pkt_length;
		} else {
			mb = prepare_segmented_packet(rxq, pkt_length, &id);
		}

		rxq->rx_tail = id;
	}
	return mb;
}

#ifdef QDMA_RX_VEC_X86_64
/* Vector implementation to prepare mbufs for packets.
 * Update this API if HW provides more information to be populated in mbuf.
 */
static uint16_t prepare_packets_v(struct qdma_rx_queue *rxq,
			struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	uint16_t count = 0, count_pkts = 0;
	uint16_t n_pkts = nb_pkts & -2;
	uint16_t id = rxq->rx_tail;
	struct rte_mbuf **sw_ring = rxq->sw_ring;
	uint16_t rx_buff_size = rxq->rx_buff_size;
	/* mask to shuffle from desc. to mbuf */
	__m128i shuf_msk = _mm_set_epi8(
			0xFF, 0xFF, 0xFF, 0xFF,  /* skip 32bits rss */
			0xFF, 0xFF,      /* skip low 16 bits vlan_macip */
			1, 0,      /* octet 0~1, 16 bits data_len */
			0xFF, 0xFF,  /* skip high 16 bits pkt_len, zero out */
			1, 0,      /* octet 0~1, low 16 bits pkt_len */
			0xFF, 0xFF,  /* skip 32 bit pkt_type */
			0xFF, 0xFF
			);
	__m128i mbuf_init, pktlen, zero_data;

	mbuf_init = _mm_set_epi64x(0, rxq->mbuf_initializer);
	pktlen = _mm_setzero_si128();
	zero_data = _mm_setzero_si128();

	/* compile-time check */
	RTE_BUILD_BUG_ON(offsetof(struct rte_mbuf, pkt_len) !=
			offsetof(struct rte_mbuf, rx_descriptor_fields1) + 4);
	RTE_BUILD_BUG_ON(offsetof(struct rte_mbuf, data_len) !=
			offsetof(struct rte_mbuf, rx_descriptor_fields1) + 8);
	RTE_BUILD_BUG_ON(offsetof(struct rte_mbuf, rearm_data) !=
			RTE_ALIGN(offsetof(struct rte_mbuf, rearm_data), 16));

	for (count = 0; count < n_pkts;
		count += RTE_QDMA_DESCS_PER_LOOP) {
		__m128i pkt_len[RTE_QDMA_DESCS_PER_LOOP];
		__m128i pkt_mb1, pkt_mb2;
		__m128i mbp1;
		uint16_t pktlen1, pktlen2;

		qdma_ul_get_cmpt_pkt_len_v(
			&rxq->cmpt_data[count], pkt_len);

		pktlen1 = _mm_extract_epi16(pkt_len[0], 0);
		pktlen2 = _mm_extract_epi16(pkt_len[1], 0);

		/* Check if packets are segmented across descriptors */
		if ((pktlen1 && (pktlen1 <= rx_buff_size)) &&
			(pktlen2 && (pktlen2 <= rx_buff_size)) &&
			((id + RTE_QDMA_DESCS_PER_LOOP) <
				(rxq->nb_rx_desc - 1))) {
			/* Load 2 (64 bit) mbuf pointers */
			mbp1 = _mm_loadu_si128((__m128i *)&sw_ring[id]);

			/* Copy 2 64 bit mbuf point into rx_pkts */
			_mm_storeu_si128((__m128i *)&rx_pkts[count_pkts], mbp1);
			_mm_storeu_si128((__m128i *)&sw_ring[id], zero_data);

			/* Pkt 1,2 convert format from desc to pktmbuf */
			/* We only have packet length to copy */
			pkt_mb2 = _mm_shuffle_epi8(pkt_len[1], shuf_msk);
			pkt_mb1 = _mm_shuffle_epi8(pkt_len[0], shuf_msk);

			/* Write the rearm data and the olflags in one write */
			_mm_store_si128(
			(__m128i *)&rx_pkts[count_pkts]->rearm_data, mbuf_init);
			_mm_store_si128(
			(__m128i *)&rx_pkts[count_pkts + 1]->rearm_data,
			mbuf_init);

			/* Write packet length */
			_mm_storeu_si128(
			(void *)&rx_pkts[count_pkts]->rx_descriptor_fields1,
			pkt_mb1);
			_mm_storeu_si128(
			(void *)&rx_pkts[count_pkts + 1]->rx_descriptor_fields1,
			pkt_mb2);

			/* Accumulate packet length counter */
			pktlen = _mm_add_epi32(pktlen, pkt_len[0]);
			pktlen = _mm_add_epi32(pktlen, pkt_len[1]);

			count_pkts += RTE_QDMA_DESCS_PER_LOOP;
			id += RTE_QDMA_DESCS_PER_LOOP;
		} else {
			/* Handle packets segmented
			 * across multiple descriptors
			 * or ring wrap
			 */
			if (pktlen1) {
				mb = prepare_segmented_packet(rxq,
					pktlen1, &id);
				rx_pkts[count_pkts++] = mb;
				pktlen = _mm_add_epi32(pktlen, pkt_len[0]);
			}

			if (pktlen2) {
				mb = prepare_segmented_packet(rxq,
					pktlen2, &id);
				rx_pkts[count_pkts++] = mb;
				pktlen = _mm_add_epi32(pktlen, pkt_len[1]);
			}
		}
	}

	rxq->stats.pkts += count_pkts;
	rxq->stats.bytes += _mm_extract_epi64(pktlen, 0);
	rxq->rx_tail = id;

	/* Handle single packet, if any pending */
	if (nb_pkts & 1) {
		mb = prepare_single_packet(rxq, count);
		if (mb)
			rx_pkts[count_pkts++] = mb;
	}

	return count_pkts;
}
#endif //QDMA_RX_VEC_X86_64

/* Prepare mbufs with packet information */
static uint16_t prepare_packets(struct qdma_rx_queue *rxq,
			struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
	uint16_t count_pkts = 0;

#ifdef QDMA_RX_VEC_X86_64
	count_pkts = prepare_packets_v(rxq, rx_pkts, nb_pkts);
#else //QDMA_RX_VEC_X86_64
	struct rte_mbuf *mb;
	uint16_t pkt_length;
	uint16_t count = 0;
	while (count < nb_pkts) {
		pkt_length = qdma_ul_get_cmpt_pkt_len(
					&rxq->cmpt_data[count]);
		if (pkt_length) {
			rxq->stats.pkts++;
			rxq->stats.bytes += pkt_length;
			mb = prepare_segmented_packet(rxq,
					pkt_length, &rxq->rx_tail);
			rx_pkts[count_pkts++] = mb;
		}
		count++;
	}
#endif //QDMA_RX_VEC_X86_64

	return count_pkts;
}

/* Populate C2H ring with new buffers */
static int rearm_c2h_ring(struct qdma_rx_queue *rxq, uint16_t num_desc)
{
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
	struct rte_mbuf *mb;
	struct qdma_ul_st_c2h_desc *rx_ring_st =
			(struct qdma_ul_st_c2h_desc *)rxq->rx_ring;
	uint16_t mbuf_index = 0;
	uint16_t id;
	int rearm_descs;

	id = rxq->q_pidx_info.pidx;

	/* Split the C2H ring updation in two parts.
	 * First handle till end of ring and then
	 * handle from beginning of ring, if ring wraps
	 */
	if ((id + num_desc) < (rxq->nb_rx_desc - 1))
		rearm_descs = num_desc;
	else
		rearm_descs = (rxq->nb_rx_desc - 1) - id;

	/* allocate new buffer */
	if (rte_mempool_get_bulk(rxq->mb_pool, (void *)&rxq->sw_ring[id],
					rearm_descs) != 0){
		PMD_DRV_LOG(ERR, "%s(): %d: No MBUFS, queue id = %d,"
		"mbuf_avail_count = %d,"
		" mbuf_in_use_count = %d, num_desc_req = %d\n",
		__func__, __LINE__, rxq->queue_id,
		rte_mempool_avail_count(rxq->mb_pool),
		rte_mempool_in_use_count(rxq->mb_pool), rearm_descs);
		return -1;
	}

#ifdef QDMA_RX_VEC_X86_64
	int rearm_cnt = rearm_descs & -2;
	__m128i head_room = _mm_set_epi64x(RTE_PKTMBUF_HEADROOM,
			RTE_PKTMBUF_HEADROOM);

	for (mbuf_index = 0; mbuf_index < ((uint16_t)rearm_cnt  & 0xFFFF);
			mbuf_index += RTE_QDMA_DESCS_PER_LOOP,
			id += RTE_QDMA_DESCS_PER_LOOP) {
		__m128i vaddr0, vaddr1;
		__m128i dma_addr;

		/* load buf_addr(lo 64bit) and buf_iova(hi 64bit) */
		RTE_BUILD_BUG_ON(offsetof(struct rte_mbuf, buf_iova) !=
				offsetof(struct rte_mbuf, buf_addr) + 8);

		/* Load two mbufs data addresses */
		vaddr0 = _mm_loadu_si128(
				(__m128i *)&(rxq->sw_ring[id]->buf_addr));
		vaddr1 = _mm_loadu_si128(
				(__m128i *)&(rxq->sw_ring[id+1]->buf_addr));

		/* Extract physical addresses of two mbufs */
		dma_addr = _mm_unpackhi_epi64(vaddr0, vaddr1);

		/* Add headroom to dma_addr */
		dma_addr = _mm_add_epi64(dma_addr, head_room);

		/* Write C2H desc with physical dma_addr */
		_mm_storeu_si128((__m128i *)&rx_ring_st[id], dma_addr);
	}

	if (rearm_descs & 1) {
		mb = rxq->sw_ring[id];

		/* rearm descriptor */
		rx_ring_st[id].dst_addr =
				(uint64_t)mb->buf_iova +
					RTE_PKTMBUF_HEADROOM;
		id++;
	}
#else //QDMA_RX_VEC_X86_64
	for (mbuf_index = 0; mbuf_index < rearm_descs;
			mbuf_index++, id++) {
		mb = rxq->sw_ring[id];
		mb->data_off = RTE_PKTMBUF_HEADROOM;

		/* rearm descriptor */
		rx_ring_st[id].dst_addr =
				(uint64_t)mb->buf_iova +
					RTE_PKTMBUF_HEADROOM;
	}
#endif //QDMA_RX_VEC_X86_64

	if (unlikely(id >= (rxq->nb_rx_desc - 1)))
		id -= (rxq->nb_rx_desc - 1);

	/* Handle from beginning of ring, if ring wrapped */
	rearm_descs = num_desc - rearm_descs;
	if (unlikely(rearm_descs)) {
		/* allocate new buffer */
		if (rte_mempool_get_bulk(rxq->mb_pool,
			(void *)&rxq->sw_ring[id], rearm_descs) != 0) {
			PMD_DRV_LOG(ERR, "%s(): %d: No MBUFS, queue id = %d,"
			"mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d, num_desc_req = %d\n",
			__func__, __LINE__, rxq->queue_id,
			rte_mempool_avail_count(rxq->mb_pool),
			rte_mempool_in_use_count(rxq->mb_pool), rearm_descs);

			rxq->q_pidx_info.pidx = id;
			qdma_dev->hw_access->qdma_queue_pidx_update(rxq->dev,
				qdma_dev->is_vf,
				rxq->queue_id, 1, &rxq->q_pidx_info);

			return -1;
		}

		for (mbuf_index = 0;
				mbuf_index < ((uint16_t)rearm_descs & 0xFFFF);
				mbuf_index++, id++) {
			mb = rxq->sw_ring[id];
			mb->data_off = RTE_PKTMBUF_HEADROOM;

			/* rearm descriptor */
			rx_ring_st[id].dst_addr =
					(uint64_t)mb->buf_iova +
						RTE_PKTMBUF_HEADROOM;
		}
	}

	PMD_DRV_LOG(DEBUG, "%s(): %d: PIDX Update: queue id = %d, "
				"num_desc = %d",
				__func__, __LINE__, rxq->queue_id,
				num_desc);

	/* Make sure writes to the C2H descriptors are
	 * synchronized before updating PIDX
	 */
	rte_wmb();

	rxq->q_pidx_info.pidx = id;
	qdma_dev->hw_access->qdma_queue_pidx_update(rxq->dev,
		qdma_dev->is_vf,
		rxq->queue_id, 1, &rxq->q_pidx_info);

	return 0;
}

/* Receive API for Streaming mode */
uint16_t qdma_recv_pkts_st(struct qdma_rx_queue *rxq, struct rte_mbuf **rx_pkts,
				uint16_t nb_pkts)
{
	uint16_t count_pkts;
	struct wb_status *wb_status;
	uint16_t nb_pkts_avail = 0;
	uint16_t rx_cmpt_tail = 0;
	uint16_t cmpt_pidx, c2h_pidx;
	uint16_t pending_desc;
#ifdef TEST_64B_DESC_BYPASS
	int bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);
#endif

	if (unlikely(rxq->err))
		return 0;

	PMD_DRV_LOG(DEBUG, "recv start on rx queue-id :%d, on "
			"tail index:%d number of pkts %d",
			rxq->queue_id, rxq->rx_tail, nb_pkts);
	wb_status = rxq->wb_status;
	rx_cmpt_tail = rxq->cmpt_cidx_info.wrb_cidx;

#ifdef TEST_64B_DESC_BYPASS
	if (unlikely(rxq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)) {
		PMD_DRV_LOG(DEBUG, "For  RX ST-mode, example"
				" design doesn't support 64byte descriptor\n");
		return 0;
	}
#endif
	cmpt_pidx = wb_status->pidx;

	if (rx_cmpt_tail < cmpt_pidx)
		nb_pkts_avail = cmpt_pidx - rx_cmpt_tail;
	else if (rx_cmpt_tail > cmpt_pidx)
		nb_pkts_avail = rxq->nb_rx_cmpt_desc - 1 - rx_cmpt_tail +
				cmpt_pidx;

	if (nb_pkts_avail == 0) {
		PMD_DRV_LOG(DEBUG, "%s(): %d: nb_pkts_avail = 0\n",
				__func__, __LINE__);
		return 0;
	}

	if (nb_pkts > QDMA_MAX_BURST_SIZE)
		nb_pkts = QDMA_MAX_BURST_SIZE;

	if (nb_pkts > nb_pkts_avail)
		nb_pkts = nb_pkts_avail;

#ifdef DUMP_MEMPOOL_USAGE_STATS
	PMD_DRV_LOG(DEBUG, "%s(): %d: queue id = %d, mbuf_avail_count = %d, "
			"mbuf_in_use_count = %d",
		__func__, __LINE__, rxq->queue_id,
		rte_mempool_avail_count(rxq->mb_pool),
		rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
	/* Make sure reads to CMPT ring are synchronized before
	 * accessing the ring
	 */
	rte_rmb();
#ifdef QDMA_LATENCY_OPTIMIZED
	adapt_update_counter(rxq, nb_pkts_avail);
#endif //QDMA_LATENCY_OPTIMIZED
	if (process_cmpt_ring(rxq, nb_pkts) != 0)
		return 0;

	if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
		PMD_DRV_LOG(DEBUG, "%s(): %d: rxq->status = %d\n",
				__func__, __LINE__, rxq->status);
		return 0;
	}

	count_pkts = prepare_packets(rxq, rx_pkts, nb_pkts);

	c2h_pidx = rxq->q_pidx_info.pidx;
	pending_desc = rxq->rx_tail - c2h_pidx - 1;
	if (rxq->rx_tail < (c2h_pidx + 1))
		pending_desc = rxq->nb_rx_desc - 2 + rxq->rx_tail -
				c2h_pidx;

	/* Batch the PIDX updates, this minimizes overhead on
	 * descriptor engine
	 */
	if (pending_desc >= MIN_RX_PIDX_UPDATE_THRESHOLD)
		rearm_c2h_ring(rxq, pending_desc);

#ifdef DUMP_MEMPOOL_USAGE_STATS
	PMD_DRV_LOG(DEBUG, "%s(): %d: queue id = %d, mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d, count_pkts = %d",
		__func__, __LINE__, rxq->queue_id,
		rte_mempool_avail_count(rxq->mb_pool),
		rte_mempool_in_use_count(rxq->mb_pool), count_pkts);
#endif //DUMP_MEMPOOL_USAGE_STATS

	PMD_DRV_LOG(DEBUG, " Recv complete with hw cidx :%d",
				rxq->wb_status->cidx);
	PMD_DRV_LOG(DEBUG, " Recv complete with hw pidx :%d\n",
				rxq->wb_status->pidx);

	return count_pkts;
}

/* Receive API for Memory mapped mode */
uint16_t qdma_recv_pkts_mm(struct qdma_rx_queue *rxq, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	uint32_t count, id;
	struct qdma_ul_mm_desc *desc;
	uint32_t len;
	int ret;
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
#ifdef TEST_64B_DESC_BYPASS
	int bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);
#endif

	if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED)
		return 0;

	id = rxq->q_pidx_info.pidx; /* Descriptor index */

	PMD_DRV_LOG(DEBUG, "recv start on rx queue-id :%d, on tail index:%d\n",
			rxq->queue_id, id);

#ifdef TEST_64B_DESC_BYPASS
	if (unlikely(rxq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)) {
		PMD_DRV_LOG(DEBUG, "For MM mode, example design doesn't "
				"support 64byte descriptor\n");
		return 0;
	}
#endif
	/* Make 1 less available, otherwise if we allow all descriptors
	 * to be filled,when nb_pkts = nb_tx_desc - 1, pidx will be same
	 * as old pidx and HW will treat this as no new descriptors were added.
	 * Hence, DMA won't happen with new descriptors.
	 */
	if (nb_pkts > rxq->nb_rx_desc - 2)
		nb_pkts = rxq->nb_rx_desc - 2;

	for (count = 0; count < nb_pkts; count++) {
		/* allocate new buffer */
		if (rte_mempool_get(rxq->mb_pool, (void *)&mb) != 0) {
			PMD_DRV_LOG(ERR, "%s(): %d: No MBUFS, queue id = %d,"
			"mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d\n",
			__func__, __LINE__, rxq->queue_id,
			rte_mempool_avail_count(rxq->mb_pool),
			rte_mempool_in_use_count(rxq->mb_pool));
			return 0;
		}

		desc = (struct qdma_ul_mm_desc *)rxq->rx_ring;
		desc += id;
		qdma_ul_update_mm_c2h_desc(rxq, mb, desc);

		len = (int)rxq->rx_buff_size;
		rte_pktmbuf_pkt_len(mb) = len;

		rte_mbuf_refcnt_set(mb, 1);
		mb->packet_type = 0;
		mb->ol_flags = 0;
		mb->next = 0;
		mb->nb_segs = 1;
		mb->port = rxq->port_id;
		mb->vlan_tci = 0;
		mb->hash.rss = 0;

		rx_pkts[count] = mb;

		rxq->ep_addr = (rxq->ep_addr + len) % DMA_BRAM_SIZE;
		id = (id + 1) % (rxq->nb_rx_desc - 1);
	}

	/* Make sure writes to the C2H descriptors are synchronized
	 * before updating PIDX
	 */
	rte_wmb();

	/* update pidx pointer for MM-mode*/
	if (count > 0) {
		rxq->q_pidx_info.pidx = id;
		qdma_dev->hw_access->qdma_queue_pidx_update(rxq->dev,
			qdma_dev->is_vf,
			rxq->queue_id, 1, &rxq->q_pidx_info);
	}

	ret = dma_wb_monitor(rxq, DMA_FROM_DEVICE, id);
	if (ret) {//Error
		PMD_DRV_LOG(ERR, "DMA Engine write-back monitor "
				"timeout error occurred, wb-count:%d "
				"and expected-count:%d\n",
				rxq->wb_status->cidx, id);

		return 0;
	}
	return count;
}
/**
 * DPDK callback for receiving packets in burst.
 *
 * @param rx_queue
 *   Generic pointer to Rx queue structure.
 * @param[out] rx_pkts
 *   Array to store received packets.
 * @param nb_pkts
 *   Maximum number of packets in array.
 *
 * @return
 *   Number of packets successfully received (<= nb_pkts).
 */
uint16_t qdma_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts)
{
	struct qdma_rx_queue *rxq = rx_queue;
	uint32_t count;

	if (rxq->st_mode)
		count = qdma_recv_pkts_st(rxq, rx_pkts, nb_pkts);
	else
		count = qdma_recv_pkts_mm(rxq, rx_pkts, nb_pkts);

	return count;
}

/**
 * DPDK callback to request the driver to free mbufs
 * currently cached by the driver.
 *
 * @param tx_queue
 *   Pointer to Tx queue specific data structure.
 * @param free_cnt
 *   Maximum number of packets to free. Use 0 to indicate all possible packets
 *   should be freed. Note that a packet may be using multiple mbufs.
 *
 * @return
 *   Failure: < 0
 *   Success: >= 0
 *     0-n: Number of packets freed. More packets may still remain in ring that
 *     are in use.
 */
int
qdma_dev_tx_done_cleanup(void *tx_queue, uint32_t free_cnt)
{
	struct qdma_tx_queue *txq = tx_queue;

	if ((uint16_t)free_cnt >= (txq->nb_tx_desc - 1))
		return -EINVAL;

	/* Free transmitted mbufs back to pool */
	return reclaim_tx_mbuf(txq, txq->wb_status->cidx, free_cnt);
}

/**
 * DPDK callback to check the status of a Tx descriptor in the queue.
 *
 * @param tx_queue
 *   Pointer to Tx queue specific data structure.
 * @param offset
 *   The offset of the descriptor starting from tail (0 is the place where
 *   the next packet will be send).
 *
 * @return
 *  - (RTE_ETH_TX_DESC_FULL) Descriptor is being processed by the hw, i.e.
 *    in the transmit queue.
 *  - (RTE_ETH_TX_DESC_DONE) Hardware is done with this descriptor, it can
 *    be reused by the driver.
 *  - (RTE_ETH_TX_DESC_UNAVAIL): Descriptor is unavailable, reserved by the
 *    driver or the hardware.
 *  - (-EINVAL) bad descriptor offset.
 */
int
qdma_dev_tx_descriptor_status(void *tx_queue, uint16_t offset)
{
	struct qdma_tx_queue *txq = tx_queue;
	uint16_t id;
	int avail, in_use;
	uint16_t cidx = 0;

	if (unlikely(offset >= (txq->nb_tx_desc - 1)))
		return -EINVAL;

	/* One descriptor is reserved so that pidx is not same as old pidx */
	if (offset == (txq->nb_tx_desc - 2))
		return RTE_ETH_TX_DESC_UNAVAIL;

	id = txq->q_pidx_info.pidx;
	cidx = txq->wb_status->cidx;

	in_use = (int)id - cidx;
	if (in_use < 0)
		in_use += (txq->nb_tx_desc - 1);
	avail = txq->nb_tx_desc - 2 - in_use;

	if (offset < avail)
		return RTE_ETH_TX_DESC_DONE;

	return RTE_ETH_TX_DESC_FULL;
}

/* Transmit API for Streaming mode */
uint16_t qdma_xmit_pkts_st(struct qdma_tx_queue *txq, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	uint64_t pkt_len = 0;
	int avail, in_use, ret, nsegs;
	uint16_t cidx = 0;
	uint16_t count = 0, id;
	struct qdma_pci_dev *qdma_dev = txq->dev->data->dev_private;
#ifdef TEST_64B_DESC_BYPASS
	int bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	if (unlikely(txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)) {
		return qdma_xmit_64B_desc_bypass(txq, tx_pkts, nb_pkts);
	}
#endif

	id = txq->q_pidx_info.pidx;
	cidx = txq->wb_status->cidx;
	PMD_DRV_LOG(DEBUG, "Xmit start on tx queue-id:%d, tail index:%d\n",
			txq->queue_id, id);

	/* Free transmitted mbufs back to pool */
	reclaim_tx_mbuf(txq, cidx, 0);

	in_use = (int)id - cidx;
	if (in_use < 0)
		in_use += (txq->nb_tx_desc - 1);

	/* Make 1 less available, otherwise if we allow all descriptors
	 * to be filled, when nb_pkts = nb_tx_desc - 1, pidx will be same
	 * as old pidx and HW will treat this as no new descriptors were added.
	 * Hence, DMA won't happen with new descriptors.
	 */
	avail = txq->nb_tx_desc - 2 - in_use;
	if (!avail) {
		PMD_DRV_LOG(DEBUG, "Tx queue full, in_use = %d", in_use);
		return 0;
	}

	for (count = 0; count < nb_pkts; count++) {
		mb = tx_pkts[count];
		nsegs = mb->nb_segs;
		if (nsegs > avail) {
			/* Number of segments in current mbuf are greater
			 * than number of descriptors available,
			 * hence update PIDX and return
			 */
			break;
		}
		avail -= nsegs;
		id = txq->q_pidx_info.pidx;
		txq->sw_ring[id] = mb;
		pkt_len += rte_pktmbuf_pkt_len(mb);

#ifdef QDMA_TX_VEC_X86_64
		ret = qdma_ul_update_st_h2c_desc_v(txq, txq->offloads, mb);
#else
		ret = qdma_ul_update_st_h2c_desc(txq, txq->offloads, mb);
#endif //RTE_ARCH_X86_64
		if (ret < 0)
			break;
	}

	txq->stats.pkts += count;
	txq->stats.bytes += pkt_len;

	/* Make sure writes to the H2C descriptors are synchronized
	 * before updating PIDX
	 */
	rte_wmb();

#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	rte_spinlock_lock(&txq->pidx_update_lock);
#endif
	txq->tx_desc_pend += count;

	/* Send PIDX update only if pending desc is more than threshold
	 * Saves frequent Hardware transactions
	 */
	if (txq->tx_desc_pend >= MIN_TX_PIDX_UPDATE_THRESHOLD) {
		qdma_dev->hw_access->qdma_queue_pidx_update(txq->dev,
			qdma_dev->is_vf,
			txq->queue_id, 0, &txq->q_pidx_info);

		txq->tx_desc_pend = 0;
	}
#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	rte_spinlock_unlock(&txq->pidx_update_lock);
#endif
	PMD_DRV_LOG(DEBUG, " xmit completed with count:%d\n", count);

	return count;
}

/* Transmit API for Memory mapped mode */
uint16_t qdma_xmit_pkts_mm(struct qdma_tx_queue *txq, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	uint32_t count, id;
	uint64_t	len = 0;
	int avail, in_use;
	int ret;
	struct qdma_pci_dev *qdma_dev = txq->dev->data->dev_private;
	uint16_t cidx = 0;

#ifdef TEST_64B_DESC_BYPASS
	int bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);
#endif

	id = txq->q_pidx_info.pidx;
	PMD_DRV_LOG(DEBUG, "Xmit start on tx queue-id:%d, tail index:%d\n",
			txq->queue_id, id);

#ifdef TEST_64B_DESC_BYPASS
	if (unlikely(txq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)) {
		PMD_DRV_LOG(DEBUG, "For MM mode, example design doesn't "
				"support 64B bypass testing\n");
		return 0;
	}
#endif
	cidx = txq->wb_status->cidx;
	/* Free transmitted mbufs back to pool */
	reclaim_tx_mbuf(txq, cidx, 0);
	in_use = (int)id - cidx;
	if (in_use < 0)
		in_use += (txq->nb_tx_desc - 1);

	/* Make 1 less available, otherwise if we allow all descriptors to be
	 * filled, when nb_pkts = nb_tx_desc - 1, pidx will be same as old pidx
	 * and HW will treat this as no new descriptors were added.
	 * Hence, DMA won't happen with new descriptors.
	 */
	avail = txq->nb_tx_desc - 2 - in_use;
	if (!avail) {
		PMD_DRV_LOG(ERR, "Tx queue full, in_use = %d", in_use);
		return 0;
	}

	if (nb_pkts > avail)
		nb_pkts = avail;

	// Set the xmit descriptors and control bits
	for (count = 0; count < nb_pkts; count++) {

		mb = tx_pkts[count];
		txq->sw_ring[id] = mb;
		/*Update the descriptor control feilds*/
		qdma_ul_update_mm_h2c_desc(txq, mb);

		len = rte_pktmbuf_data_len(mb);
		PMD_DRV_LOG(DEBUG, "xmit number of bytes:%ld, count:%d ",
				len, count);

		txq->ep_addr = (txq->ep_addr + len) % DMA_BRAM_SIZE;
		id = txq->q_pidx_info.pidx;
	}

	/* Make sure writes to the H2C descriptors are synchronized before
	 * updating PIDX
	 */
	rte_wmb();

	/* update pidx pointer */
	if (count > 0) {
		PMD_DRV_LOG(INFO, "tx PIDX=%d", txq->q_pidx_info.pidx);
		qdma_dev->hw_access->qdma_queue_pidx_update(txq->dev,
			qdma_dev->is_vf,
			txq->queue_id, 0, &txq->q_pidx_info);
	}

	ret = dma_wb_monitor(txq, DMA_TO_DEVICE, id);
	if (ret) {
		PMD_DRV_LOG(ERR, "DMA Engine write-back monitor "
				"timeout error occurred, wb-count:%d "
				"and expected-count:%d\n",
				txq->wb_status->cidx, id);
		return 0;
	}

	PMD_DRV_LOG(DEBUG, " xmit completed with count:%d", count);
	return count;
}
/**
 * DPDK callback for transmitting packets in burst.
 *
 * @param tx_queue
 G*   Generic pointer to TX queue structure.
 * @param[in] tx_pkts
 *   Packets to transmit.
 * @param nb_pkts
 *   Number of packets in array.
 *
 * @return
 *   Number of packets successfully transmitted (<= nb_pkts).
 */
uint16_t qdma_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct qdma_tx_queue *txq = tx_queue;
	uint16_t count;

	if (txq->status != RTE_ETH_QUEUE_STATE_STARTED)
		return 0;

	if (txq->st_mode)
		count =	qdma_xmit_pkts_st(txq, tx_pkts, nb_pkts);
	else
		count =	qdma_xmit_pkts_mm(txq, tx_pkts, nb_pkts);

	return count;
}
