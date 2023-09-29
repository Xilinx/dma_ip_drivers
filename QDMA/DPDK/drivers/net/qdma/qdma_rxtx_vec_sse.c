/*-
 * BSD LICENSE
 *
 * Copyright (c) 2017-2022 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
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
#endif

/* Vector implementation to get packet length from two completion entries */
static void qdma_ul_get_cmpt_pkt_len_vec(void *ul_cmpt_entry, __m128i *data)
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

/* Vector implementation to update H2C descriptor */
static int qdma_ul_update_st_h2c_desc_vec(void *qhndl, uint64_t q_offloads,
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

/* Process completion ring */
static int process_cmpt_ring_vec(struct qdma_rx_queue *rxq,
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

				ret = qdma_ul_extract_st_cmpt_info(
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

				ret = qdma_ul_extract_st_cmpt_info(
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

/* Vector implementation to prepare mbufs for packets.
 * Update this API if HW provides more information to be populated in mbuf.
 */
static uint16_t prepare_packets_vec(struct qdma_rx_queue *rxq,
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

		qdma_ul_get_cmpt_pkt_len_vec(
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
			pktlen = _mm_add_epi64(pktlen,
				_mm_set_epi16(0, 0, 0, 0,
					0, 0, 0, pktlen1));
			pktlen = _mm_add_epi64(pktlen,
				_mm_set_epi16(0, 0, 0, 0,
					0, 0, 0, pktlen2));

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
				pktlen = _mm_add_epi64(pktlen,
					_mm_set_epi16(0, 0, 0, 0,
						0, 0, 0, pktlen1));
			}

			if (pktlen2) {
				mb = prepare_segmented_packet(rxq,
					pktlen2, &id);
				rx_pkts[count_pkts++] = mb;
				pktlen = _mm_add_epi64(pktlen,
					_mm_set_epi16(0, 0, 0, 0,
						0, 0, 0, pktlen2));
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

/* Populate C2H ring with new buffers */
static int rearm_c2h_ring_vec(struct qdma_rx_queue *rxq, uint16_t num_desc)
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
	else {
		rearm_descs = (rxq->nb_rx_desc - 1) - id;
		rxq->qstats.ring_wrap_cnt++;
	}

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
#ifdef LATENCY_MEASUREMENT
			/* start the timer */
			rxq->qstats.pkt_lat.prev = rte_get_timer_cycles();
#endif
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

#ifdef LATENCY_MEASUREMENT
	/* start the timer */
	rxq->qstats.pkt_lat.prev = rte_get_timer_cycles();
#endif
	return 0;
}

/* Receive API for Streaming mode */
uint16_t qdma_recv_pkts_st_vec(struct qdma_rx_queue *rxq,
		struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
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

#ifdef LATENCY_MEASUREMENT
	if (cmpt_pidx != rxq->qstats.wrb_pidx) {
		/* stop the timer */
		rxq->qstats.pkt_lat.curr = rte_get_timer_cycles();
		c2h_pidx_to_cmpt_pidx_lat[rxq->queue_id][rxq->qstats.lat_cnt] =
			rxq->qstats.pkt_lat.curr - rxq->qstats.pkt_lat.prev;
		rxq->qstats.lat_cnt = ((rxq->qstats.lat_cnt + 1) % LATENCY_CNT);
	}
#endif

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

	nb_pkts = RTE_MIN(nb_pkts, RTE_MIN(nb_pkts_avail, QDMA_MAX_BURST_SIZE));

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

	int ret = process_cmpt_ring_vec(rxq, nb_pkts);
	if (unlikely(ret))
		return 0;

	if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
		PMD_DRV_LOG(DEBUG, "%s(): %d: rxq->status = %d\n",
				__func__, __LINE__, rxq->status);
		return 0;
	}

	count_pkts = prepare_packets_vec(rxq, rx_pkts, nb_pkts);

	c2h_pidx = rxq->q_pidx_info.pidx;
	pending_desc = rxq->rx_tail - c2h_pidx - 1;
	if (rxq->rx_tail < (c2h_pidx + 1))
		pending_desc = rxq->nb_rx_desc - 2 + rxq->rx_tail -
				c2h_pidx;

	rxq->qstats.pidx = rxq->q_pidx_info.pidx;
	rxq->qstats.wrb_pidx = rxq->wb_status->pidx;
	rxq->qstats.wrb_cidx = rxq->wb_status->cidx;
	rxq->qstats.rxq_cmpt_tail = rx_cmpt_tail;
	rxq->qstats.pending_desc = pending_desc;
	rxq->qstats.mbuf_avail_cnt = rte_mempool_avail_count(rxq->mb_pool);
	rxq->qstats.mbuf_in_use_cnt = rte_mempool_in_use_count(rxq->mb_pool);

	/* Batch the PIDX updates, this minimizes overhead on
	 * descriptor engine
	 */
	if (pending_desc >= MIN_RX_PIDX_UPDATE_THRESHOLD)
		rearm_c2h_ring_vec(rxq, pending_desc);

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
uint16_t qdma_recv_pkts_vec(void *rx_queue, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts)
{
	struct qdma_rx_queue *rxq = rx_queue;
	uint32_t count;

	if (rxq->st_mode)
		count = qdma_recv_pkts_st_vec(rxq, rx_pkts, nb_pkts);
	else
		count = qdma_recv_pkts_mm(rxq, rx_pkts, nb_pkts);

	return count;
}

/* Transmit API for Streaming mode */
uint16_t qdma_xmit_pkts_st_vec(struct qdma_tx_queue *txq,
		struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
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

	/* Make sure reads to Tx ring are synchronized before
	 * accessing the status descriptor.
	 */
	rte_rmb();

	cidx = txq->wb_status->cidx;

#ifdef LATENCY_MEASUREMENT
	uint32_t cidx_cnt = 0;
	if (cidx != txq->qstats.wrb_cidx) {
		if ((cidx - txq->qstats.wrb_cidx) > 0) {
			cidx_cnt = cidx - txq->qstats.wrb_cidx;

			if (cidx_cnt <= 8)
				txq->qstats.wrb_cidx_cnt_lt_8++;
			else if (cidx_cnt > 8 && cidx_cnt <= 32)
				txq->qstats.wrb_cidx_cnt_8_to_32++;
			else if (cidx_cnt > 32 && cidx_cnt <= 64)
				txq->qstats.wrb_cidx_cnt_32_to_64++;
			else
				txq->qstats.wrb_cidx_cnt_gt_64++;
		}

		/* stop the timer */
		txq->qstats.pkt_lat.curr = rte_get_timer_cycles();
		h2c_pidx_to_hw_cidx_lat[txq->queue_id][txq->qstats.lat_cnt] =
			txq->qstats.pkt_lat.curr - txq->qstats.pkt_lat.prev;
		txq->qstats.lat_cnt = ((txq->qstats.lat_cnt + 1) % LATENCY_CNT);
	} else {
		txq->qstats.wrb_cidx_cnt_no_change++;
	}
#endif

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

	if (unlikely(!avail)) {
		txq->qstats.txq_full_cnt++;
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

		ret = qdma_ul_update_st_h2c_desc_vec(txq, txq->offloads, mb);

		if (unlikely(ret < 0))
			break;
	}

	txq->stats.pkts += count;
	txq->stats.bytes += pkt_len;

	txq->qstats.pidx = id;
	txq->qstats.wrb_cidx = cidx;
	txq->qstats.txq_tail = txq->tx_fl_tail;
	txq->qstats.in_use_desc = in_use;
	txq->qstats.nb_pkts = nb_pkts;

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

#ifdef LATENCY_MEASUREMENT
		/* start the timer */
		txq->qstats.pkt_lat.prev = rte_get_timer_cycles();
#endif
	}
#if (MIN_TX_PIDX_UPDATE_THRESHOLD > 1)
	rte_spinlock_unlock(&txq->pidx_update_lock);
#endif
	PMD_DRV_LOG(DEBUG, " xmit completed with count:%d\n", count);

	return count;
}

/**
 * DPDK callback for transmitting packets in burst.
 *
 * @param tx_queue
 *   Generic pointer to TX queue structure.
 * @param[in] tx_pkts
 *   Packets to transmit.
 * @param nb_pkts
 *   Number of packets in array.
 *
 * @return
 *   Number of packets successfully transmitted (<= nb_pkts).
 */
uint16_t qdma_xmit_pkts_vec(void *tx_queue, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct qdma_tx_queue *txq = tx_queue;
	uint16_t count;

	if (txq->status != RTE_ETH_QUEUE_STATE_STARTED)
		return 0;

	if (txq->st_mode)
		count =	qdma_xmit_pkts_st_vec(txq, tx_pkts, nb_pkts);
	else
		count =	qdma_xmit_pkts_mm(txq, tx_pkts, nb_pkts);

	return count;
}
