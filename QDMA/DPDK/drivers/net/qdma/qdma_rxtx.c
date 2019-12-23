/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2019 Xilinx, Inc. All rights reserved.
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
#include "qdma_access.h"

#include <fcntl.h>
#include <unistd.h>
#include "qdma_rxtx.h"

int dma_wb_monitor(void *xq, uint8_t dir, uint16_t expected_count);

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
int dma_wb_monitor(void *xq, uint8_t dir, uint16_t expected_count)
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

static void reclaim_tx_mbuf(struct qdma_tx_queue *txq, uint16_t cidx)
{
	int fl_desc = 0;
	uint16_t count;
	int id;

	id = txq->tx_fl_tail;
	fl_desc = (int)cidx - id;
	if (fl_desc < 0)
		fl_desc += (txq->nb_tx_desc - 1);

	for (count = 0; count < fl_desc; count++) {
		if (txq->sw_ring[id]) {
			rte_pktmbuf_free(txq->sw_ring[id]);
			txq->sw_ring[id] = NULL;
		}
		id++;
		if (unlikely(id >= (txq->nb_tx_desc - 1)))
			id -= (txq->nb_tx_desc - 1);
	}
	txq->tx_fl_tail = id;
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

		sprintf(fln, "q_%d_%s", txq->queue_id,
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
		enum qdma_versal_ip_type *ip_type)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)queue_hndl;
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;

	*device_type = (enum qdma_device_type)qdma_dev->device_type;
	*ip_type = (enum qdma_versal_ip_type)qdma_dev->versal_ip_type;
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
	struct qdma_ul_st_h2c_desc *tx_ring_st = NULL;
	struct qdma_ul_st_h2c_desc *desc;
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)queue_hndl;

	id = txq->q_pidx_info.pidx;
	tx_ring_st = (struct qdma_ul_st_h2c_desc *)txq->tx_ring;
	desc = (struct qdma_ul_st_h2c_desc *)&tx_ring_st[id];

	id++;
	if ((id >= (txq->nb_tx_desc - 1)))
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

uint16_t qdma_recv_pkts_st(struct qdma_rx_queue *rxq, struct rte_mbuf **rx_pkts,
				uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	struct rte_mbuf *first_seg = NULL;
	struct rte_mbuf *last_seg = NULL;
	struct qdma_ul_st_c2h_desc *rx_ring_st = NULL;
	uint32_t count = 0, count_pkts = 0;
	uint16_t id;
	struct c2h_cmpt_info cmpt_desc;
	struct qdma_ul_st_cmpt_ring *user_cmpt_entry;
	struct wb_status *wb_status;
	uint32_t pkt_length;
	uint16_t nb_pkts_avail = 0;
	uint16_t rx_cmpt_tail = 0;
	uint16_t mbuf_index = 0;
	uint16_t pkt_len[QDMA_MAX_BURST_SIZE];
	uint16_t rx_buff_size;
	uint16_t cmpt_pidx, c2h_pidx;
	uint16_t pending_desc;
	struct qdma_pci_dev *qdma_dev = rxq->dev->data->dev_private;
	int ret = 0;
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
	rx_buff_size = rxq->rx_buff_size;

#ifdef TEST_64B_DESC_BYPASS
	if (unlikely(rxq->en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)) {
		PMD_DRV_LOG(DEBUG, "For  RX ST-mode, example"
				" design doesn't support 64byte descriptor\n");
		return 0;
	}
#endif
	rx_ring_st = (struct qdma_ul_st_c2h_desc *)rxq->rx_ring;
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

	while (count < nb_pkts) {
		memset(&cmpt_desc, 0, sizeof(struct c2h_cmpt_info));
		user_cmpt_entry =
		(struct qdma_ul_st_cmpt_ring *)((uint64_t)rxq->cmpt_ring +
		((uint64_t)rx_cmpt_tail *
		rxq->cmpt_desc_len));
		qdma_ul_extract_st_cmpt_info(user_cmpt_entry, &cmpt_desc);

		if (unlikely(cmpt_desc.err || cmpt_desc.data_frmt)) {
			PMD_DRV_LOG(ERR, "Error detected on CMPT ring at index %d, "
					 "len = %d, queue_id = %d\n",
					 rx_cmpt_tail, cmpt_desc.length,
					 rxq->queue_id);
			rxq->err = 1;
			return 0;
		}

		if (unlikely(rxq->dump_immediate_data)) {
			ret = qdma_ul_process_immediate_data_st((void *)rxq,
					user_cmpt_entry, rxq->cmpt_desc_len);
			if (ret < 0)
				return 0;
		}
		pkt_len[count] = cmpt_desc.length;
		rx_cmpt_tail++;
		if (unlikely(rx_cmpt_tail >= (rxq->nb_rx_cmpt_desc - 1)))
			rx_cmpt_tail -= (rxq->nb_rx_cmpt_desc - 1);
		count++;
	}
	// Update the CPMT CIDX
	rxq->cmpt_cidx_info.wrb_cidx = rx_cmpt_tail;
	qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(rxq->dev,
		qdma_dev->is_vf,
		rxq->queue_id, &rxq->cmpt_cidx_info);

	if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
		PMD_DRV_LOG(DEBUG, "%s(): %d: rxq->status = %d\n",
				__func__, __LINE__, rxq->status);
		return 0;
	}

	count = 0;
	mbuf_index = 0;
	id = rxq->rx_tail;
	while (count < nb_pkts) {
		pkt_length = pkt_len[count];

		if (unlikely(!pkt_length)) {
			count++;
			continue;
		}

		do {
			mb = rxq->sw_ring[id];
			rxq->sw_ring[id++] = NULL;

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
				first_seg->pkt_len = pkt_len[count];
				first_seg->packet_type = 0;
				first_seg->ol_flags = 0;
				first_seg->port = rxq->port_id;
				first_seg->vlan_tci = 0;
				first_seg->hash.rss = 0;
			} else {
				first_seg->nb_segs++;
				last_seg->next = mb;
			}

			last_seg = mb;
			mb->next = NULL;
		} while (pkt_length);
		rxq->stats.pkts++;
		rxq->stats.bytes += pkt_len[count++];
		rx_pkts[count_pkts++] = first_seg;
		first_seg = NULL;
	}

	rxq->rx_tail = id;
	c2h_pidx = rxq->q_pidx_info.pidx;

	pending_desc = rxq->rx_tail - c2h_pidx - 1;
	if (rxq->rx_tail < (c2h_pidx + 1))
		pending_desc = rxq->nb_rx_desc - 2 + rxq->rx_tail -
				c2h_pidx;

	/* Batch the PIDX updates, this minimizes overhead on
	 * descriptor engine
	 */
	if (pending_desc >= MIN_RX_PIDX_UPDATE_THRESHOLD) {
		struct rte_mbuf *tmp_sw_ring[pending_desc];
		/* allocate new buffer */
		if (rte_mempool_get_bulk(rxq->mb_pool, (void *)tmp_sw_ring,
						pending_desc) != 0){
			PMD_DRV_LOG(ERR, "%s(): %d: No MBUFS, queue id = %d,"
			"mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d, pending_desc = %d\n",
			__func__, __LINE__, rxq->queue_id,
			rte_mempool_avail_count(rxq->mb_pool),
			rte_mempool_in_use_count(rxq->mb_pool), pending_desc);
			return count_pkts;
		}

		id = c2h_pidx;
		for (mbuf_index = 0; mbuf_index < pending_desc; mbuf_index++) {
			mb = tmp_sw_ring[mbuf_index];

			/* make it so the data pointer starts there too... */
			mb->data_off = RTE_PKTMBUF_HEADROOM;

			/* rearm descriptor */
			qdma_ul_update_st_c2h_desc(mb, &rx_ring_st[id]);
			rxq->sw_ring[id++] = mb;
			if (unlikely(id >= (rxq->nb_rx_desc - 1)))
				id -= (rxq->nb_rx_desc - 1);
		}

		PMD_DRV_LOG(DEBUG, "%s(): %d: PIDX Update: queue id = %d, "
					"pending_desc = %d",
					__func__, __LINE__, rxq->queue_id,
					pending_desc);

		/* Make sure writes to the C2H descriptors are
		 * synchronized before updating PIDX
		 */
		rte_wmb();

		rxq->q_pidx_info.pidx = id;
		qdma_dev->hw_access->qdma_queue_pidx_update(rxq->dev,
			qdma_dev->is_vf,
			rxq->queue_id, 1, &rxq->q_pidx_info);
	}

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

uint16_t qdma_xmit_pkts_st(struct qdma_tx_queue *txq, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct rte_mbuf *mb;
	uint16_t count, id;
	int avail, in_use, ret, nsegs;
	uint16_t pkt_len;
	uint16_t cidx = 0;
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
	reclaim_tx_mbuf(txq, cidx);

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
		pkt_len = rte_pktmbuf_pkt_len(mb);

		ret = qdma_ul_update_st_h2c_desc(txq, mb);
		if (ret < 0)
			break;

		PMD_DRV_LOG(DEBUG, "xmit number of bytes:%d",
				pkt_len);
		txq->stats.pkts++;
		txq->stats.bytes += pkt_len;
	}

	/* Make sure writes to the H2C descriptors are synchronized
	 * before updating PIDX
	 */
	rte_wmb();

	rte_spinlock_lock(&txq->pidx_update_lock);
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
	rte_spinlock_unlock(&txq->pidx_update_lock);

	PMD_DRV_LOG(DEBUG, " xmit completed with count:%d\n", count);

	return count;
}

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
	reclaim_tx_mbuf(txq, cidx);
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
