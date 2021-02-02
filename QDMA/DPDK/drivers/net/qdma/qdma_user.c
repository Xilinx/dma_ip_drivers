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

#include <rte_mbuf.h>
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include "qdma_user.h"
#include "qdma_access_common.h"
#include "qdma_log.h"

#include <fcntl.h>
#include <unistd.h>

/**
 * Extract the fields of given completion entry in the completion ring.
 *
 * @param ul_cmpt_entry
 *   Pointer to completion entry to be extracted.
 * @param cmpt_info
 *   Pointer to variable to which completion entry details to be extracted.
 *
 * @return
 *   0 on success and -1 on failure.
 */
int qdma_ul_extract_st_cmpt_info(void *ul_cmpt_entry, void *cmpt_info)
{
	union qdma_ul_st_cmpt_ring *cmpt_data, *cmpt_desc;

	cmpt_desc = (union qdma_ul_st_cmpt_ring *)(ul_cmpt_entry);
	cmpt_data = (union qdma_ul_st_cmpt_ring *)(cmpt_info);

	if (unlikely(cmpt_desc->err || cmpt_desc->data_frmt))
		return -1;

	cmpt_data->data = cmpt_desc->data;
	if (unlikely(!cmpt_desc->desc_used))
		cmpt_data->length = 0;

	return 0;
}

/**
 * Extract the packet length from the given completion entry.
 *
 * @param ul_cmpt_entry
 *   Pointer to completion entry to be extracted.
 *
 * @return
 *   Packet length
 */
uint16_t qdma_ul_get_cmpt_pkt_len(void *ul_cmpt_entry)
{
	return ((union qdma_ul_st_cmpt_ring *)ul_cmpt_entry)->length;
}

/**
 * Processes the immediate data for the given completion ring entry
 * and stores in a file.
 *
 * @param qhndl
 *   Pointer to RX queue handle.
 * @param cmpt_desc_len
 *   Completion descriptor length.
 * @param cmpt_entry
 *   Pointer to completion entry to be processed.
 *
 * @return
 *   None.
 */
int qdma_ul_process_immediate_data_st(void *qhndl, void *cmpt_entry,
			uint16_t cmpt_desc_len)
{
	int ofd;
	char fln[50];
#ifndef TEST_64B_DESC_BYPASS
	uint16_t i = 0;
	enum qdma_device_type dev_type;
	enum qdma_ip_type ip_type;
#else
	int ret = 0;
#endif
	uint16_t queue_id = 0;

	queue_id = qdma_get_rx_queue_id(qhndl);
	snprintf(fln, sizeof(fln), "q_%d_%s", queue_id,
			"immmediate_data.txt");
	ofd = open(fln, O_RDWR | O_CREAT | O_APPEND |
			O_SYNC, 0666);
	if (ofd < 0) {
		PMD_DRV_LOG(INFO, "recv on qhndl[%d] CMPT, "
				"unable to create outfile "
				" to dump immediate data",
				queue_id);
		return ofd;
	}
#ifdef TEST_64B_DESC_BYPASS
	ret = write(ofd, cmpt_entry, cmpt_desc_len);
	if (ret < cmpt_desc_len)
		PMD_DRV_LOG(DEBUG, "recv on rxq[%d] CMPT, "
			"immediate data len: %d, "
			"written to outfile :%d bytes",
			 queue_id, cmpt_desc_len,
			 ret);
#else
	qdma_get_device_info(qhndl, &dev_type, &ip_type);

	if (ip_type == QDMA_VERSAL_HARD_IP) {
		//Ignoring first 20 bits of length feild
		dprintf(ofd, "%02x",
			(*((uint8_t *)cmpt_entry + 2) & 0xF0));
		for (i = 3; i < (cmpt_desc_len) ; i++)
			dprintf(ofd, "%02x",
				*((uint8_t *)cmpt_entry + i));
	} else {
		dprintf(ofd, "%02x",
			(*((uint8_t *)cmpt_entry) & 0xF0));
		for (i = 1; i < (cmpt_desc_len) ; i++)
			dprintf(ofd, "%02x",
				*((uint8_t *)cmpt_entry + i));
	}
#endif

	close(ofd);
	return 0;
}

/**
 * Updates the ST H2C descriptor.
 *
 * @param qhndl
 *   Pointer to TX queue handle.
 * @param q_offloads
 *   Offloads supported for the queue.
 * @param mb
 *   Pointer to memory buffer.
 *
 * @return
 *   None.
 */
int qdma_ul_update_st_h2c_desc(void *qhndl, uint64_t q_offloads,
				struct rte_mbuf *mb)
{
	(void)q_offloads;
	struct qdma_ul_st_h2c_desc *desc_info;
	int nsegs = mb->nb_segs;
	int pkt_segs = nsegs;

	if (nsegs == 1) {
		desc_info = get_st_h2c_desc(qhndl);
		desc_info->len = rte_pktmbuf_data_len(mb);
		desc_info->pld_len = desc_info->len;
		desc_info->src_addr = mb->buf_iova + mb->data_off;
		desc_info->flags = (S_H2C_DESC_F_SOP | S_H2C_DESC_F_EOP);
		desc_info->cdh_flags = 0;
	} else {
		while (nsegs && mb) {
			desc_info = get_st_h2c_desc(qhndl);

			desc_info->len = rte_pktmbuf_data_len(mb);
			desc_info->pld_len = desc_info->len;
			desc_info->src_addr = mb->buf_iova + mb->data_off;
			desc_info->flags = 0;
			if (nsegs == pkt_segs)
				desc_info->flags |= S_H2C_DESC_F_SOP;
			if (nsegs == 1)
				desc_info->flags |= S_H2C_DESC_F_EOP;
			desc_info->cdh_flags = 0;

			nsegs--;
			mb = mb->next;
		}
	}
	return 0;
}

/**
 * updates the MM c2h descriptor.
 *
 * @param qhndl
 *   Pointer to RX queue handle.
 * @param mb
 *   Pointer to memory buffer.
 * @param desc
 *   Pointer to descriptor entry.
 *
 * @return
 *   None.
 */
int qdma_ul_update_mm_c2h_desc(void *qhndl, struct rte_mbuf *mb, void *desc)
{
	struct qdma_ul_mm_desc *desc_info = (struct qdma_ul_mm_desc *)desc;

	desc_info->src_addr = get_mm_c2h_ep_addr(qhndl);
	/* make it so the data pointer starts there too... */
	mb->data_off = RTE_PKTMBUF_HEADROOM;
	/* low 32-bits of phys addr must be 4KB aligned... */
	desc_info->dst_addr = (uint64_t)mb->buf_iova + RTE_PKTMBUF_HEADROOM;
	desc_info->dv = 1;
	desc_info->eop = 1;
	desc_info->sop = 1;
	desc_info->len = (int)get_mm_buff_size(qhndl);

	return 0;
}

/**
 * updates the MM h2c descriptor.
 *
 * @param qhndl
 *   Pointer to TX queue handle.
 * @param mb
 *   Pointer to memory buffer.
 *
 * @return
 *   None.
 */
int qdma_ul_update_mm_h2c_desc(void *qhndl, struct rte_mbuf *mb)
{
	struct qdma_ul_mm_desc *desc_info;

	desc_info = (struct qdma_ul_mm_desc *)get_mm_h2c_desc(qhndl);
	desc_info->src_addr = mb->buf_iova + mb->data_off;
	desc_info->dst_addr = get_mm_h2c_ep_addr(qhndl);
	desc_info->dv = 1;
	desc_info->eop = 1;
	desc_info->sop = 1;
	desc_info->len = rte_pktmbuf_data_len(mb);

	return 0;
}

/**
 * Processes the completion data from the given completion entry.
 *
 * @param cmpt_entry
 *   Pointer to completion entry to be processed.
 * @param cmpt_desc_len
 *   Completion descriptor length.
 * @param cmpt_buff
 *   Pointer to the data buffer to which the data will be extracted.
 *
 * @return
 *   None.
 */
int qdma_ul_process_immediate_data(void *cmpt_entry, uint16_t cmpt_desc_len,
				char *cmpt_buff)
{
	uint16_t i = 0;
	char *cmpt_buff_ptr;
	struct qdma_ul_cmpt_ring *cmpt_desc =
			(struct qdma_ul_cmpt_ring *)(cmpt_entry);

	if (unlikely(cmpt_desc->err || cmpt_desc->data_frmt))
		return -1;

	cmpt_buff_ptr = (char *)cmpt_buff;
	*(cmpt_buff_ptr) = (*((uint8_t *)cmpt_desc) & 0xF0);
	for (i = 1; i < (cmpt_desc_len); i++)
		*(cmpt_buff_ptr + i) = (*((uint8_t *)cmpt_desc + i));

	return 0;
}
