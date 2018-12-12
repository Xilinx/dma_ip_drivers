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
#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_ethdev_pci.h>
#include <rte_cycles.h>
#include <rte_kvargs.h>
#include "qdma.h"
#include "qdma_regs.h"

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

	if (bar >= (NUM_BARS - 1)) {
		printf("Error: PCI BAR number:%d not supported\n"
			"Please enter valid BAR number\n", bar);
		return -1;
	}

	baseaddr = (uint64_t)qdma_dev->bar_addr[bar];
	if (!baseaddr) {
		printf("Error: PCI BAR number:%d not mapped\n", bar);
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

	if (bar >= (NUM_BARS - 1)) {
		printf("Error: PCI BAR index:%d not supported\n"
			"Please enter valid BAR index\n", bar);
		return;
	}

	baseaddr = (uint64_t)qdma_dev->bar_addr[bar];
	if (!baseaddr) {
		printf("Error: PCI BAR number:%d not mapped\n", bar);
		return;
	}
	*((volatile uint32_t *)(baseaddr + reg)) = val;
}

void qdma_desc_dump(struct rte_eth_dev *dev, uint32_t qid)
{
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	uint32_t i, k, bypass_desc_sz_idx;
	struct qdma_c2h_desc *rx_ring_st;
	struct qdma_mm_desc *ring_mm;
	struct qdma_h2c_desc *tx_ring_st;
	uint8_t *rx_ring_bypass = NULL;
	uint8_t *tx_ring_bypass = NULL;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
	bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);

	if (rxq->en_bypass &&
	    bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA) {
		rx_ring_bypass = (uint8_t *)rxq->rx_ring;
		for (i = 0; i < rxq->nb_rx_desc; ++i) {
			for (k = 0; k < rxq->bypass_desc_sz; k++)
				printf("\trxq descriptor-data[%d]: %x\n", i,
				       rx_ring_bypass[i * (rxq->bypass_desc_sz)
				       + k]);
		}
	} else if (rxq->st_mode) {
		rx_ring_st = (struct qdma_c2h_desc *)rxq->rx_ring;
		printf("\nST-mode Rx descriptor dump on Queue-id:%d:\n", qid);
		for (i = 0; i < rxq->nb_rx_desc; ++i) {
			printf("\t desc-index:%d: dest-addr:%lx\n", i,
					rx_ring_st[i].dst_addr);
		}
	} else {
		ring_mm = (struct qdma_mm_desc *)rxq->rx_ring;
		printf("\nMM-mode Rx descriptor dump on Queue-id:%d:\n", qid);
		for (i = 0; i < rxq->nb_rx_desc; ++i) {
			printf("\t desc-index:%d: dest-addr:%lx, src-addr:%lx,"
					" valid:%d, eop:%d, sop:%d,"
					"length:%d\n", i,
					ring_mm[i].dst_addr,
					ring_mm[i].src_addr,
					ring_mm[i].dv,
					ring_mm[i].eop,
					ring_mm[i].sop,
					ring_mm[i].len);
		}
	}

	bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	if (txq->en_bypass &&
	    bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA) {
		tx_ring_bypass = (uint8_t *)txq->tx_ring;
		for (i = 0; i < txq->nb_tx_desc; ++i) {
			for (k = 0; k < txq->bypass_desc_sz; k++)
				printf("\t txq descriptor-data[%d]: %x\n", i,
						tx_ring_bypass[i *
						(txq->bypass_desc_sz) + k]);
		}
	} else if (txq->st_mode) {
		tx_ring_st = (struct qdma_h2c_desc *)txq->tx_ring;
		printf("\nST-mode Tx descriptor dump on Queue-id:%d:\n", qid);
		for (i = 0; i < txq->nb_tx_desc; ++i) {
			printf("\t desc-index:%d: src-addr:%lx,length:%d\n", i,
					tx_ring_st[i].src_addr,
					tx_ring_st[i].len);
		}

	} else {
		ring_mm = (struct qdma_mm_desc *)txq->tx_ring;
		printf("\nMM-mode Tx descriptor dump on Queue-id:%d:\n", qid);
		for (i = 0; i < txq->nb_tx_desc; ++i) {
			printf("\t desc-index:%d: dest-addr:%lx, src-addr:%lx,"
					" valid:%d, eop:%d,sop:%d, length:%d\n",
					i, ring_mm[i].dst_addr,
					ring_mm[i].src_addr,
					ring_mm[i].dv,
					ring_mm[i].eop,
					ring_mm[i].sop,
					ring_mm[i].len);
		}
	}
}

void qdma_reset_rx_queue(struct qdma_rx_queue *rxq)
{
	uint32_t i;
	uint32_t sz;

	rxq->rx_tail = 0;
	rxq->c2h_pidx = 0;
	rxq->rx_cmpt_tail = 0;

	/* Zero out HW ring memory, For MM Descriptor */
	if (rxq->st_mode) {  /** if ST-mode **/
		sz = rxq->cmpt_desc_len;
		for (i = 0; i < (sz * rxq->nb_rx_cmpt_desc); i++)
			((volatile char *)rxq->cmpt_ring)[i] = 0;

		sz = sizeof(struct qdma_c2h_desc);
		for (i = 0; i < (sz * rxq->nb_rx_desc); i++)
			((volatile char *)rxq->rx_ring)[i] = 0;

	} else {
		sz = sizeof(struct qdma_mm_desc);
		for (i = 0; i < (sz * rxq->nb_rx_desc); i++)
			((volatile char *)rxq->rx_ring)[i] = 0;
	}

	/* Initialize SW ring entries */
	for (i = 0; i < rxq->nb_rx_desc; i++)
		rxq->sw_ring[i] = NULL;
}

void qdma_inv_rx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;
	uint64_t bar_addr;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
					  QDMA_IND_Q_PRG_OFF);

	/** To clear the Queues **/
	/** To clear the SW C2H Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_C2H << CTXT_SEL_SHIFT_B) |
		   (qid << QID_SHIFT_B) |
		   (QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "SW C2H Clear cmd for queue-id:%d:"
			    "reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	/** To clear the HW C2H Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_C2H << CTXT_SEL_SHIFT_B) |
		   (qid << QID_SHIFT_B) |
		   (QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
				"reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (mode) {  /** ST-mode **/
		/** To clear the C2H prefetch context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_PFTCH << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}

		/** To clear the C2H Completion Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_DESC_CMPT << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}

		/** To clear the C2H credit context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_CR_C2H << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "C2H  creditsClear cmd for qid:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
	}
}
/**
 * reset the Rx device queues.
 *
 * reset the device by clearing the Rx descriptors for all
 * queues and device registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   Nothing.
 */
void qdma_reset_rx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;
	uint64_t bar_addr;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
					  QDMA_IND_Q_PRG_OFF);

	/** To clear the Queues **/
	/** To clear the SW C2H Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_C2H << CTXT_SEL_SHIFT_B) |
		   (qid << QID_SHIFT_B) |
		   (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "SW C2H Clear cmd for queue-id:%d:"
			    "reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	/** To clear the HW C2H Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_C2H << CTXT_SEL_SHIFT_B) |
		   (qid << QID_SHIFT_B) |
		   (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
				"reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (mode) {  /** ST-mode **/
		/** To clear the C2H prefetch context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_PFTCH << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}

		/** To clear the C2H Completion Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_DESC_CMPT << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "HW C2H Clear cmd for queue-id:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}

		/** To clear the C2H credit context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_CR_C2H << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "C2H  creditsClear cmd for qid:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
	}
}

int qdma_start_rx_queue(struct qdma_rx_queue *rxq)
{
	struct rte_mbuf *mb;
	void *obj = NULL;
	uint64_t phys_addr;
	uint16_t i;
	struct qdma_c2h_desc *rx_ring_st = NULL;

	/* allocate new buffers for the Rx descriptor ring */
	if (rxq->st_mode) {  /** ST-mode **/
		rx_ring_st = (struct qdma_c2h_desc *)rxq->rx_ring;
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id %d, mbuf_avail_count =%d,"
				"mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
		for (i = 0; i < (rxq->nb_rx_desc - 1); i++) {
			if (rte_mempool_get(rxq->mb_pool, &obj) != 0) {
				PMD_DRV_LOG(ERR, "qdma-start-rx-queue(): "
						"rte_mempool_get: failed");
				goto fail;
			}

			mb = obj;
			phys_addr = (uint64_t)mb->buf_physaddr +
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
	rxq->c2h_pidx = 0;
	rxq->rx_cmpt_tail = 0;

	return 0;
fail:
	return -ENOMEM;
}

/**
 * Start the Rx device queues.
 *
 * Start the device by configuring the Rx descriptors for all
 * the queues and device registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */

int qdma_queue_ctxt_rx_prog(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode, uint8_t en_prefetch,
				uint32_t ringszidx, uint32_t cmpt_ringszidx,
				uint32_t bufszidx, uint32_t threshidx,
				uint32_t timeridx, uint32_t triggermode,
				uint16_t func_id, uint64_t phys_addr,
				uint64_t cmpt_phys_addr, uint8_t cmpt_desc_fmt,
				uint8_t en_bypass, uint8_t en_bypass_prefetch,
				uint8_t bypass_desc_sz_idx,
				uint8_t dis_overflow_check)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint64_t bar_addr;
	uint32_t reg_val, ctxt_sel;
	uint16_t flag, i;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
					  QDMA_IND_Q_PRG_OFF);

	/* C2H Queue initialization */
	reg_val = (func_id << SW_DESC_CNTXT_FUNC_ID_SHIFT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[0], reg_val);
	reg_val = (SW_DESC_CNTXT_WB_EN << SW_DESC_CNTXT_WB_EN_SHIFT_B) |
			(ringszidx << SW_DESC_CNTXT_RING_SIZE_ID_SHIFT) |
			(SW_DESC_CNTXT_QUEUE_ENABLE <<
			 SW_DESC_CNTXT_QUEUE_EN_SHIFT);

	if (mode) {  /** ST-mode **/
		if (en_bypass &&
		    bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			reg_val =  reg_val | (bypass_desc_sz_idx <<
						SW_DESC_CNTXT_DESC_SZ_SHIFT);
		else
			reg_val = reg_val | (SW_DESC_CNTXT_C2H_STREAM_DMA <<
						SW_DESC_CNTXT_DESC_SZ_SHIFT);

		reg_val = reg_val | (en_bypass << SW_DESC_CNTXT_BYPASS_SHIFT) |
			  (SW_DESC_CNTXT_FETCH_CREDIT_EN <<
			   SW_DESC_CNTXT_FETCH_CREDIT_EN_SHIFT);
	} else {
		if (en_bypass &&
			bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
			reg_val = reg_val | (bypass_desc_sz_idx <<
						  SW_DESC_CNTXT_DESC_SZ_SHIFT) |
					(SW_DESC_CNTXT_IS_MM <<
					 SW_DESC_CNTXT_IS_MM_SHIFT);
		else
			reg_val = reg_val | (SW_DESC_CNTXT_MEMORY_MAP_DMA <<
						  SW_DESC_CNTXT_DESC_SZ_SHIFT) |
					(SW_DESC_CNTXT_IS_MM <<
						 SW_DESC_CNTXT_IS_MM_SHIFT);

		reg_val = reg_val |  (en_bypass << SW_DESC_CNTXT_BYPASS_SHIFT) |
			 (SW_DESC_CNTXT_WBI_INTERVAL <<
			  SW_DESC_CNTXT_WBI_INTERVAL_SHIFT) |
			 (SW_DESC_CNTXT_WBI_CHK <<
			  SW_DESC_CNTXT_WBI_CHK_SHIFT);
	}

	qdma_write_reg((uint64_t)&q_regs->ctxt_data[1], reg_val);
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[2],
				rte_cpu_to_le_32(phys_addr & MASK_32BIT));
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[3],
				rte_cpu_to_le_32(phys_addr >> 32));
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[4], 0);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[2], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[3], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[4], MASK_32BIT);
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_C2H << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_WR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, " Read cmd for queue-id:%d: reg_val:%x\n",
					qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (flag) {
		PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
				"initailization with cmd reg_val:%x\n",
						 qid, reg_val);
		goto fail;
	}

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	/** To read the Queue **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_C2H << CTXT_SEL_SHIFT_B) |
			(qid << QID_SHIFT_B) |
			(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	for (i = 0; i < 5; i++) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_data[i]);
		PMD_DRV_LOG(INFO, " Read context-data on queue-id:%d,"
				" idx:%d:reg_val:%x\n", qid, i, reg_val);
	/* TO-DO: Need to verify the Queue ctxt-data and if Queue has
	 * any errors, then return -1. **/
	}
#endif //RTE_LIBRTE_QDMA_DEBUG_DRIVER

	/* C2H Prefetch & Completion context initialization */
	if (mode) {  /** ST-mode **/
		/** C2H Prefetch Context **/
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[0],
				(en_prefetch << PREFETCH_EN_SHIFT) |
				(bufszidx << BUFF_SIZE_INDEX_SHIFT) |
				(en_bypass_prefetch << PREFETCH_BYPASS_SHIFT));
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[1],
				(VALID_CNTXT << VALID_CNTXT_SHIFT));
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[2], 0);
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[3], 0);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[2], MASK_0BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[3], MASK_0BIT);
		ctxt_sel = (QDMA_CTXT_SEL_PFTCH << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_WR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		i = CONTEXT_PROG_POLL_COUNT;
		flag = 1;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, " Read cmd for queue-id:%d:"
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d:"
					" C2H Prefetch initailization with cmd"
					" reg_val:%x\n", qid, reg_val);
			goto fail;
		}

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		/** To read the Queue **/
		ctxt_sel = (QDMA_CTXT_SEL_PFTCH << CTXT_SEL_SHIFT_B) |
			   (qid << QID_SHIFT_B) |
			   (QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		for (i = 0; i < NUM_CONTEXT_REGS; i++) {
			reg_val =
				qdma_read_reg((uint64_t)&q_regs->ctxt_data[i]);
			PMD_DRV_LOG(INFO, " Read context-data C2H Prefetch on "
					"queue-id:%d, idx:%d:reg_val:%x\n",
							qid, i, reg_val);
		}
#endif //RTE_LIBRTE_QDMA_DEBUG_DRIVER

		/** C2H Completion Context **/
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[0],
					(CMPT_CNTXT_EN_STAT_DESC <<
					 CMPT_CNTXT_EN_STAT_DESC_SHIFT) |
					(CMPT_CNTXT_COLOR_BIT <<
					 CMPT_CNTXT_COLOR_BIT_SHIFT) |
					((uint32_t)func_id <<
					 CMPT_CNTXT_FUNC_ID_SHIFT) |
					((uint64_t)threshidx <<
					 CMPT_CNTXT_THRESHOLD_MODE_SHIFT) |
					((uint64_t)timeridx <<
					 CMPT_CNTXT_TIMER_INDEX_SHIFT) |
					(triggermode <<
					 CMPT_CNTXT_TRIGGER_MODE_SHIFT) |
					((uint64_t)cmpt_ringszidx <<
					 CMPT_CNTXT_RING_SIZE_INDEX_SHIFT));

		qdma_write_reg((uint64_t)&q_regs->ctxt_data[1],
				(rte_cpu_to_le_32(cmpt_phys_addr >> 6) &
					 MASK_32BIT));

		qdma_write_reg((uint64_t)&q_regs->ctxt_data[2],
				(rte_cpu_to_le_32(cmpt_phys_addr >> 32) >> 6) |
					((uint32_t)cmpt_desc_fmt <<
					 CMPT_CNTXT_DESC_SIZE_SHIFT));

		qdma_write_reg((uint64_t)&q_regs->ctxt_data[3],
					(CMPT_CNTXT_CTXT_VALID <<
					 CMPT_CNTXT_CTXT_VALID_SHIFT));
		qdma_write_reg((uint64_t)&q_regs->ctxt_data[4],
					(dis_overflow_check <<
					 CMPT_CNTXT_OVF_CHK_DIS_SHIFT));
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[2], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[3], MASK_32BIT);
		qdma_write_reg((uint64_t)&q_regs->ctxt_mask[4], MASK_32BIT);
		ctxt_sel = (QDMA_CTXT_SEL_DESC_CMPT << CTXT_SEL_SHIFT_B) |
					(qid << QID_SHIFT_B) |
					(QDMA_CTXT_CMD_WR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		i = CONTEXT_PROG_POLL_COUNT;
		flag = 1;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, " Read cmd for queue-id:%d:"
						" reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: C2H CMPT"
					" initailization with cmd reg_val:%x\n",
					qid, reg_val);
			goto fail;
		}

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		/** To read the Queue **/
		ctxt_sel = (QDMA_CTXT_SEL_DESC_CMPT << CTXT_SEL_SHIFT_B) |
					(qid << QID_SHIFT_B) |
					(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		for (i = 0; i < NUM_CONTEXT_REGS; i++) {
			reg_val = qdma_read_reg((uint64_t)&
							q_regs->ctxt_data[i]);
			PMD_DRV_LOG(INFO, " Read context-data C2H-CMPT on "
					"queue-id:%d, idx:%d:reg_val:%x\n",
						qid, i, reg_val);
		}
#endif //RTE_LIBRTE_QDMA_DEBUG_DRIVER
	}

	return 0;
fail:
	return -1;
}

/*
 * Tx queue reset
 */
void qdma_reset_tx_queue(struct qdma_tx_queue *txq)
{
	uint32_t i;
	uint32_t sz;

	txq->tx_tail = 0;
	txq->tx_fl_tail = 0;
	if (txq->st_mode) {  /** ST-mode **/
		sz = sizeof(struct qdma_h2c_desc);
		/* Zero out HW ring memory */
		for (i = 0; i < (sz * (txq->nb_tx_desc)); i++)
			((volatile char *)txq->tx_ring)[i] = 0;
	} else {
		sz = sizeof(struct qdma_mm_desc);
		/* Zero out HW ring memory */
		for (i = 0; i < (sz * (txq->nb_tx_desc)); i++)
			((volatile char *)txq->tx_ring)[i] = 0;
	}

	/* Initialize SW ring entries */
	for (i = 0; i < txq->nb_tx_desc; i++)
		txq->sw_ring[i] = NULL;
}

void qdma_inv_tx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint64_t bar_addr;
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
					  QDMA_IND_Q_PRG_OFF);

	/** To clear the SW H2C Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "SW H2C Clear cmd for queue-id:%d:"
				" reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	/** To clear the HW H2C Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "HW H2C Clear cmd for queue-id:%d:"
				" reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (mode) {  /** ST-mode **/
		/** To clear the C2H credit context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_CR_H2C << CTXT_SEL_SHIFT_B) |
					(qid << QID_SHIFT_B) |
					(QDMA_CTXT_CMD_INV << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "H2C Credits Clear cmd for queue-id:"
					  "%d: reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
	}
}

/**
 * reset the Tx device queues.
 *
 * reset the device by clearing the Tx descriptors for
 * all queues and device registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   Nothing.
 */
void qdma_reset_tx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint64_t bar_addr;
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
					  QDMA_IND_Q_PRG_OFF);

	/** To clear the SW H2C Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "SW H2C Clear cmd for queue-id:%d:"
				" reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	/** To clear the HW H2C Queues **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, "HW H2C Clear cmd for queue-id:%d:"
				" reg_val:%x\n", qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (mode) {  /** ST-mode **/
		/** To clear the C2H credit context Queues **/
		ctxt_sel = (QDMA_CTXT_SEL_CR_H2C << CTXT_SEL_SHIFT_B) |
					(qid << QID_SHIFT_B) |
					(QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);
		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(INFO, "H2C Credits Clear cmd for queue-id:"
					  "%d: reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
	}
}

void qdma_start_tx_queue(struct qdma_tx_queue *txq)
{
	/* initialize tail */
	txq->tx_tail = 0;
	*txq->tx_pidx = 0;
}

/**
 * Start the tx device queues.
 *
 * Start the device by configuring the Tx descriptors for
 * all the queues and device registers.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   Nothing.
 */

int qdma_queue_ctxt_tx_prog(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode, uint32_t ringszidx,
				uint16_t func_id, uint64_t phys_addr,
				uint8_t en_bypass, uint8_t bypass_desc_sz_idx)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint32_t ctxt_sel, reg_val;
	uint64_t bar_addr;
	uint16_t flag, i;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
			  QDMA_IND_Q_PRG_OFF);

	reg_val = (func_id << SW_DESC_CNTXT_FUNC_ID_SHIFT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[0], reg_val);

	reg_val = (en_bypass << SW_DESC_CNTXT_BYPASS_SHIFT) |
				(SW_DESC_CNTXT_WB_EN <<
				 SW_DESC_CNTXT_WB_EN_SHIFT_B) |
				(ringszidx <<
				 SW_DESC_CNTXT_RING_SIZE_ID_SHIFT) |
				(SW_DESC_CNTXT_WBI_INTERVAL <<
				 SW_DESC_CNTXT_WBI_INTERVAL_SHIFT) |
				(SW_DESC_CNTXT_WBI_CHK <<
				 SW_DESC_CNTXT_WBI_CHK_SHIFT) |
				(SW_DESC_CNTXT_QUEUE_ENABLE <<
				 SW_DESC_CNTXT_QUEUE_EN_SHIFT);

	if (en_bypass && bypass_desc_sz_idx == SW_DESC_CNTXT_64B_BYPASS_DMA)
		reg_val = reg_val | (bypass_desc_sz_idx <<
						SW_DESC_CNTXT_DESC_SZ_SHIFT);
	else if (mode)   /** ST-mode **/
		reg_val = reg_val | (SW_DESC_CNTXT_H2C_STREAM_DMA <<
						SW_DESC_CNTXT_DESC_SZ_SHIFT);
	else		 /* MM- mode*/
		reg_val = reg_val | (SW_DESC_CNTXT_MEMORY_MAP_DMA <<
						SW_DESC_CNTXT_DESC_SZ_SHIFT) |
					(SW_DESC_CNTXT_IS_MM <<
					 SW_DESC_CNTXT_IS_MM_SHIFT);

	qdma_write_reg((uint64_t)&q_regs->ctxt_data[1], reg_val);
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[2],
					rte_cpu_to_le_32(phys_addr &
								MASK_32BIT));
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[3],
					rte_cpu_to_le_32(phys_addr >> 32));
	qdma_write_reg((uint64_t)&q_regs->ctxt_data[4], 0);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[2], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[3], MASK_32BIT);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[4], MASK_32BIT);
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_WR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, " Read cmd for queue-id:%d: reg_val:%x\n",
					qid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
	if (flag) {
		PMD_DRV_LOG(ERR, "H2C Error: Busy on queue-id:%d: "
				"initailization with cmd reg_val:%x\n",
				qid, reg_val);
		goto fail;
	}

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	/** To read the Queue **/
	ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	for (i = 0; i < NUM_CONTEXT_REGS; i++) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_data[i]);
		PMD_DRV_LOG(INFO, " Read context-data on queue-id:%d,"
				"idx:%d:reg_val:%x\n", qid, i, reg_val);
	}
#endif //RTE_LIBRTE_QDMA_DEBUG_DRIVER
	return 0;
fail:
	return -1;
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

static int qbase_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs queue base is: %s\n", value);
	qdma_dev->queue_base =  (unsigned int)strtoul(value, &end, 10);
	if (qdma_dev->queue_base >= QDMA_QUEUES_NUM_MAX) {
		PMD_DRV_LOG(INFO, "QDMA devargs queue-base > max allowed\n");
		return -1;
	}

	return 0;
}

static int pfetch_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs desc_prefetch is: %s\n", value);
	qdma_dev->en_desc_prefetch = (uint8_t)strtoul(value, &end, 10);
	if (qdma_dev->en_desc_prefetch > 1) {
		PMD_DRV_LOG(INFO, "QDMA devargs prefetch should be 1 or 0,"
						  " setting to 1.\n");
		qdma_dev->en_desc_prefetch = 1;
	}
	return 0;
}

static int cmpt_desc_len_check_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs cmpt_desc_len is: %s\n", value);
	qdma_dev->cmpt_desc_len =  (uint8_t)strtoul(value, &end, 10);
	if (qdma_dev->cmpt_desc_len != CMPT_DESC_LEN_8B &&
			qdma_dev->cmpt_desc_len != CMPT_DESC_LEN_16B &&
			qdma_dev->cmpt_desc_len != CMPT_DESC_LEN_32B &&
			qdma_dev->cmpt_desc_len != CMPT_DESC_LEN_64B) {
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

	if (qdma_dev->trigger_mode > TRIG_MODE_MAX) {
		qdma_dev->trigger_mode = TRIG_MODE_MAX;
		PMD_DRV_LOG(INFO, "QDMA devargs trigger mode invalid,"
						  "reset to default: %d\n",
						  qdma_dev->trigger_mode);
	}
	return 0;
}

static int wb_accumulation_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct qdma_pci_dev *qdma_dev = (struct qdma_pci_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs trigger mode: %s\n", value);
	qdma_dev->wb_acc_int =  (uint8_t)strtoul(value, &end, 10);

	if (qdma_dev->wb_acc_int > MAX_WB_ACC_INT) {
		qdma_dev->wb_acc_int = DEFAULT_WB_ACC_INT;
		PMD_DRV_LOG(INFO, "QDMA devargs write-back-accumulation "
				"invalid, reset to default: %d\n",
						  qdma_dev->wb_acc_int);
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

	if (qdma_dev->config_bar_idx >= QDMA_BAR_NUM ||
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

	if (qdma_dev->c2h_bypass_mode > C2H_BYPASS_MODE_MAX) {
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
	const char *qbase_key = "queue_base";
	const char *pfetch_key = "desc_prefetch";
	const char *cmpt_desc_len_key = "cmpt_desc_len";
	const char *trigger_mode_key = "trigger_mode";
	const char *wb_acc_int_key = "wb_acc_int";
	const char *config_bar_key = "config_bar";
	const char *c2h_byp_mode_key = "c2h_byp_mode";
	const char *h2c_byp_mode_key = "h2c_byp_mode";
	int ret = 0;

	if (!devargs)
		return 0;

	kvlist = rte_kvargs_parse(devargs->args, NULL);
	if (!kvlist)
		return 0;

	/* process the queue_base*/
	if (rte_kvargs_count(kvlist, qbase_key)) {
		ret = rte_kvargs_process(kvlist, qbase_key,
						qbase_check_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

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

	/* process the writeback accumalation*/
	if (rte_kvargs_count(kvlist, wb_acc_int_key)) {
		ret = rte_kvargs_process(kvlist, wb_acc_int_key,
					  wb_accumulation_handler, qdma_dev);
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
	uint64_t baseaddr;
	int      bar_len, i, j;
	uint32_t xlnx_id_mask = 0xffff0000;
	uint64_t reg_val;
	uint32_t mask = 1;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_pci_dev *dma_priv;

	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;

	/* Config bar */
	bar_len = pci_dev->mem_resource[dma_priv->config_bar_idx].len;
	if (bar_len) {
		baseaddr = (uint64_t)
			   pci_dev->mem_resource[dma_priv->config_bar_idx].addr;

		if (dma_priv->is_vf)
			reg_val = qdma_read_reg((uint64_t)baseaddr +
							QDMA_VF_RTL_VER);
		else
			reg_val = qdma_read_reg((uint64_t)baseaddr);

		if ((reg_val & xlnx_id_mask) != QDMA_CONFIG_BLOCK_ID) {
			PMD_DRV_LOG(INFO, "QDMA config BAR index :%d invalid",
						dma_priv->config_bar_idx);
			return -1;
		}
	} else {
		PMD_DRV_LOG(INFO, "QDMA config BAR index :%d is not enabled",
					dma_priv->config_bar_idx);
		return -1;
	}

	/* Find user bar*/
	if (dma_priv->is_vf) {
		reg_val = qdma_read_reg((uint64_t)baseaddr +
						QDMA_GLBL2_VF_BARLITE_EXT);
		reg_val = reg_val & 0x3F;

	} else {
		reg_val = qdma_read_reg((uint64_t)baseaddr +
						QDMA_GLBL2_PF_BARLITE_EXT);
		reg_val = (reg_val >> (6 * PCI_FUNC(dma_priv->pf))) & 0x3F;
	}

	for (j = 0 ; j < QDMA_BAR_NUM; j++) {
		if (reg_val & mask) {
			if (pci_dev->mem_resource[j].len) {
				dma_priv->user_bar_idx = j;
				break;
			}
		}
		mask <<= 1;
	}

	/* Find bypass bar*/
	for (i = 0; i < QDMA_BAR_NUM; i++) {
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
	PMD_DRV_LOG(INFO, "QDMA user bar idx :%d\n", dma_priv->user_bar_idx);
	PMD_DRV_LOG(INFO, "QDMA bypass bar idx :%d\n",
			dma_priv->bypass_bar_idx);

	return 0;
}
int qdma_get_hw_version(struct rte_eth_dev *dev)
{
	uint64_t baseaddr, reg_val;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_pci_dev *dma_priv;

	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	baseaddr = ((uint64_t)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr);

	if (dma_priv->is_vf) {
		reg_val = qdma_read_reg((uint64_t)baseaddr + QDMA_VF_RTL_VER);
		dma_priv->rtl_version = (reg_val & VF_RTL_VERSION_MASK) >>
						VF_RTL_VERSION_SHIFT;
		dma_priv->vivado_rel = (reg_val & VF_VIVADO_RELEASE_ID_MASK) >>
						VF_VIVADO_RELEASE_ID_SHIFT;
		dma_priv->everest_ip = (reg_val & VF_EVEREST_IP_MASK) >>
						VF_EVEREST_IP_SHIFT;

	} else {
		reg_val = qdma_read_reg((uint64_t)baseaddr + QDMA_PF_RTL_VER);
		dma_priv->rtl_version = (reg_val & PF_RTL_VERSION_MASK) >>
						 PF_RTL_VERSION_SHIFT;
		dma_priv->vivado_rel = (reg_val & PF_VIVADO_RELEASE_ID_MASK) >>
						PF_VIVADO_RELEASE_ID_SHIFT;
		dma_priv->everest_ip = (reg_val & PF_EVEREST_IP_MASK) >>
						PF_EVEREST_IP_SHIFT;
	}

	if (dma_priv->rtl_version == 0)
		PMD_DRV_LOG(INFO, "QDMA RTL VERSION : RTL-1\n");
	else if (dma_priv->rtl_version == 1)
		PMD_DRV_LOG(INFO, "QDMA RTL VERSION : RTL-2\n");

	if (dma_priv->vivado_rel == 0)
		PMD_DRV_LOG(INFO, "QDMA VIVADO RELEASE ID : 2018.3\n");

	if (dma_priv->everest_ip == 0)
		PMD_DRV_LOG(INFO, "QDMA IP : SOFT-IP\n");
	else
		PMD_DRV_LOG(INFO, "QDMA IP : HARD-IP\n");
	return 0;
}
