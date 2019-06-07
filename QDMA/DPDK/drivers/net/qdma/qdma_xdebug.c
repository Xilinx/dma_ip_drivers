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
#include <rte_cycles.h>
#include <unistd.h>
#include <string.h>
#include <rte_hexdump.h>

#include "qdma.h"
#include "rte_pmd_qdma.h"
#include "qdma_access.h"
#include "qdma_reg_dump.h"
#define xdebug_info(x, args...) rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1,\
					## args)

static void print_header(const char *str)
{
	xdebug_info(adap, "\n\n%s\n\n", str);
}

static int qdma_struct_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];

	if (queue >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(INFO, " RX queue_id=%d not configured\n", queue);
	} else {
		struct qdma_rx_queue *rx_q =
			(struct qdma_rx_queue *)dev->data->rx_queues[queue];

		if (rx_q) {
			print_header(" ***********RX Queue struct********** ");
			xdebug_info(dev, "\t\t wb_pidx             :%x\n",
					rx_q->wb_status->pidx);
			xdebug_info(dev, "\t\t wb_cidx             :%x\n",
					rx_q->wb_status->cidx);
			xdebug_info(dev, "\t\t rx_tail (ST)        :%x\n",
					rx_q->rx_tail);
			xdebug_info(dev, "\t\t c2h_pidx            :%x\n",
					rx_q->q_pidx_info.pidx);
			xdebug_info(dev, "\t\t rx_cmpt_cidx        :%x\n",
					rx_q->cmpt_cidx_info.wrb_cidx);
			xdebug_info(dev, "\t\t cmpt_desc_len       :%x\n",
					rx_q->cmpt_desc_len);
			xdebug_info(dev, "\t\t rx_buff_size        :%x\n",
					rx_q->rx_buff_size);
			xdebug_info(dev, "\t\t nb_rx_desc          :%x\n",
					rx_q->nb_rx_desc);
			xdebug_info(dev, "\t\t nb_rx_cmpt_desc     :%x\n",
					rx_q->nb_rx_cmpt_desc);
			xdebug_info(dev, "\t\t ep_addr             :%x\n",
					rx_q->ep_addr);
			xdebug_info(dev, "\t\t st_mode             :%x\n",
					rx_q->st_mode);
			xdebug_info(dev, "\t\t rx_deferred_start   :%x\n",
					rx_q->rx_deferred_start);
			xdebug_info(dev, "\t\t en_prefetch         :%x\n",
					rx_q->en_prefetch);
			xdebug_info(dev, "\t\t en_bypass           :%x\n",
					rx_q->en_bypass);
			xdebug_info(dev, "\t\t dump_immediate_data :%x\n",
					rx_q->dump_immediate_data);
			xdebug_info(dev, "\t\t en_bypass_prefetch  :%x\n",
					rx_q->en_bypass_prefetch);
			xdebug_info(dev, "\t\t dis_overflow_check  :%x\n",
					rx_q->dis_overflow_check);
			xdebug_info(dev, "\t\t bypass_desc_sz      :%x\n",
					rx_q->bypass_desc_sz);
			xdebug_info(dev, "\t\t ringszidx           :%x\n",
					rx_q->ringszidx);
			xdebug_info(dev, "\t\t cmpt_ringszidx      :%x\n",
					rx_q->cmpt_ringszidx);
			xdebug_info(dev, "\t\t buffszidx           :%x\n",
					rx_q->buffszidx);
			xdebug_info(dev, "\t\t threshidx           :%x\n",
					rx_q->threshidx);
			xdebug_info(dev, "\t\t timeridx            :%x\n",
					rx_q->timeridx);
			xdebug_info(dev, "\t\t triggermode         :%x\n",
					rx_q->triggermode);
		}
	}

	if (queue >= dev->data->nb_tx_queues) {
		PMD_DRV_LOG(INFO, " TX queue_id=%d not configured\n", queue);
	} else {
		struct qdma_tx_queue *tx_q =
			(struct qdma_tx_queue *)dev->data->tx_queues[queue];

		if (tx_q) {
			print_header("***********TX Queue struct************");
			xdebug_info(dev, "\t\t wb_pidx             :%x\n",
					tx_q->wb_status->pidx);
			xdebug_info(dev, "\t\t wb_cidx             :%x\n",
					tx_q->wb_status->cidx);
			xdebug_info(dev, "\t\t h2c_pidx            :%x\n",
					tx_q->q_pidx_info.pidx);
			xdebug_info(dev, "\t\t tx_fl_tail          :%x\n",
					tx_q->tx_fl_tail);
			xdebug_info(dev, "\t\t tx_desc_pend        :%x\n",
					tx_q->tx_desc_pend);
			xdebug_info(dev, "\t\t nb_tx_desc          :%x\n",
					tx_q->nb_tx_desc);
			xdebug_info(dev, "\t\t st_mode             :%x\n",
					tx_q->st_mode);
			xdebug_info(dev, "\t\t tx_deferred_start   :%x\n",
					tx_q->tx_deferred_start);
			xdebug_info(dev, "\t\t en_bypass           :%x\n",
					tx_q->en_bypass);
			xdebug_info(dev, "\t\t bypass_desc_sz      :%x\n",
					tx_q->bypass_desc_sz);
			xdebug_info(dev, "\t\t func_id             :%x\n",
					tx_q->func_id);
			xdebug_info(dev, "\t\t port_id             :%x\n",
					tx_q->port_id);
			xdebug_info(dev, "\t\t ringszidx           :%x\n",
					tx_q->ringszidx);
			xdebug_info(dev, "\t\t ep_addr             :%x\n",
					tx_q->ep_addr);
		}
	}
	return 0;
}

static int qdma_csr_dump(void)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	char *buf = NULL;
	uint32_t buflen;
	int ret;

	if (rte_eth_dev_count_avail() < 1)
		return -1;
	dev = &rte_eth_devices[0];
	qdma_dev = dev->data->dev_private;
	buflen = qdma_reg_dump_buf_len();
	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_CSR_DUMP", buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		PMD_DRV_LOG(ERR, "Unable to allocate memory for csr dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}
	xdebug_info(dev, "FPGA Config Registers\n--------------\n");
	xdebug_info(dev, " Offset       Name    "
			"                                    Value(Hex) Value(Dec)\n");
	ret = qdma_dump_config_regs(dev, qdma_dev->is_vf, buf, buflen);
	if (ret < 0) {
		PMD_DRV_LOG(ERR, "Insufficient space to dump Config Bar register values\n");
		rte_free(buf);
		return -EINVAL;
	}
	xdebug_info(dev, "%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t qid = qdma_dev->queue_base + queue;
	struct qdma_descq_sw_ctxt q_sw_ctxt;
	struct qdma_descq_cmpt_ctxt q_cmpt_ctxt;
	struct qdma_descq_prefetch_ctxt q_prefetch_ctxt;
	struct qdma_descq_hw_ctxt q_hw_ctxt;

	if (queue >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(INFO, " RX queue_id=%d not configured\n", queue);
	} else {
		print_header(" *************C2H Queue Contexts************** ");

		memset(&q_sw_ctxt, 0, sizeof(struct qdma_descq_sw_ctxt));
		memset(&q_cmpt_ctxt, 0, sizeof(struct qdma_descq_cmpt_ctxt));
		memset(&q_prefetch_ctxt, 0,
				sizeof(struct qdma_descq_prefetch_ctxt));
		memset(&q_hw_ctxt, 0, sizeof(struct qdma_descq_hw_ctxt));

		qdma_sw_context_read(dev, 1, qid, &q_sw_ctxt);
		qdma_pfetch_context_read(dev, qid, &q_prefetch_ctxt);
		qdma_cmpt_context_read(dev, qid, &q_cmpt_ctxt);
		qdma_hw_context_read(dev, 1, qid, &q_hw_ctxt);

		/* C2H SW context */
		xdebug_info(qdma_dev, "\t\t intr_aggr           :%x\n",
				q_sw_ctxt.intr_aggr);
		xdebug_info(qdma_dev, "\t\t vec                 :%x\n",
				q_sw_ctxt.vec);
		xdebug_info(qdma_dev, "\t\t Ring Base-addr      :%lx\n",
				q_sw_ctxt.ring_bs_addr);
		xdebug_info(qdma_dev, "\t\t is_mm               :%x\n",
				q_sw_ctxt.is_mm);
		xdebug_info(qdma_dev, "\t\t mrkr_dis            :%x\n",
				q_sw_ctxt.mrkr_dis);
		xdebug_info(qdma_dev, "\t\t irq_req             :%x\n",
				q_sw_ctxt.irq_req);
		xdebug_info(qdma_dev, "\t\t err_wb_sent         :%x\n",
				q_sw_ctxt.err_wb_sent);
		xdebug_info(qdma_dev, "\t\t Error status        :%x\n",
				q_sw_ctxt.err);
		xdebug_info(qdma_dev, "\t\t irq_no_last         :%x\n",
				q_sw_ctxt.irq_no_last);
		xdebug_info(qdma_dev, "\t\t port-id             :%x\n",
				q_sw_ctxt.port_id);
		xdebug_info(qdma_dev, "\t\t irq-enable          :%x\n",
				q_sw_ctxt.irq_en);
		xdebug_info(qdma_dev, "\t\t write-back enable   :%x\n",
				q_sw_ctxt.wbk_en);
		xdebug_info(qdma_dev, "\t\t mm-channel-id       :%x\n",
				q_sw_ctxt.mm_chn);
		xdebug_info(qdma_dev, "\t\t bypass              :%x\n",
				q_sw_ctxt.bypass);
		xdebug_info(qdma_dev, "\t\t desc-size index     :%x\n",
				q_sw_ctxt.desc_sz);
		xdebug_info(qdma_dev, "\t\t ring-size index     :%x\n",
				q_sw_ctxt.rngsz_idx);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_sw_ctxt.rsvd1);
		xdebug_info(qdma_dev, "\t\t fetch_max           :%x\n",
				q_sw_ctxt.fetch_max);
		xdebug_info(qdma_dev, "\t\t at                  :%x\n",
				q_sw_ctxt.at);
		xdebug_info(qdma_dev, "\t\t wbi_intvl_en        :%x\n",
				q_sw_ctxt.wbi_intvl_en);
		xdebug_info(qdma_dev, "\t\t wbi_chk             :%x\n",
				q_sw_ctxt.wbi_chk);
		xdebug_info(qdma_dev, "\t\t fetch credits       :%x\n",
				q_sw_ctxt.frcd_en);
		xdebug_info(qdma_dev, "\t\t queue-enable        :%x\n",
				q_sw_ctxt.qen);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_sw_ctxt.rsvd0);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				q_sw_ctxt.fnc_id);
		xdebug_info(qdma_dev, "\t\t irq_arm             :%x\n",
				q_sw_ctxt.irq_arm);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n\n",
				q_sw_ctxt.pidx);

		/* C2H Completion context */
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_cmpt_ctxt.rsvd2);
		xdebug_info(qdma_dev, "\t\t intr aggr           :%x\n",
				q_cmpt_ctxt.int_aggr);
		xdebug_info(qdma_dev, "\t\t Interrupt vector    :%x\n",
				q_cmpt_ctxt.vec);
		xdebug_info(qdma_dev, "\t\t atc                 :%x\n",
				q_cmpt_ctxt.at);
		xdebug_info(qdma_dev, "\t\t Overflow Chk Disable:%x\n",
				q_cmpt_ctxt.ovf_chk_dis);
		xdebug_info(qdma_dev, "\t\t Full Update         :%x\n",
				q_cmpt_ctxt.full_upd);
		xdebug_info(qdma_dev, "\t\t timer_running       :%x\n",
				q_cmpt_ctxt.timer_running);
		xdebug_info(qdma_dev, "\t\t user_trig_pend      :%x\n",
				q_cmpt_ctxt.user_trig_pend);
		xdebug_info(qdma_dev, "\t\t error status        :%x\n",
				q_cmpt_ctxt.err);
		xdebug_info(qdma_dev, "\t\t valid               :%x\n",
				q_cmpt_ctxt.valid);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n",
				q_cmpt_ctxt.cidx);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n",
				q_cmpt_ctxt.pidx);
		xdebug_info(qdma_dev, "\t\t desc-size           :%x\n",
				q_cmpt_ctxt.desc_sz);
		xdebug_info(qdma_dev, "\t\t bs_addr(4K/22 bits) :%lx\n",
				(unsigned long int)q_cmpt_ctxt.bs_addr);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_cmpt_ctxt.rsvd1);
		xdebug_info(qdma_dev, "\t\t ring-size-index     :%x\n",
				q_cmpt_ctxt.ringsz_idx);
		xdebug_info(qdma_dev, "\t\t color               :%x\n",
				q_cmpt_ctxt.color);
		xdebug_info(qdma_dev, "\t\t interrupt-state     :%x\n",
				q_cmpt_ctxt.in_st);
		xdebug_info(qdma_dev, "\t\t timer-index         :%x\n",
				q_cmpt_ctxt.timer_idx);
		xdebug_info(qdma_dev, "\t\t counter-index       :%x\n",
				q_cmpt_ctxt.counter_idx);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_cmpt_ctxt.rsvd);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				q_cmpt_ctxt.fnc_id);
		xdebug_info(qdma_dev, "\t\t trigger-mode        :%x\n",
				q_cmpt_ctxt.trig_mode);
		xdebug_info(qdma_dev, "\t\t cause interrupt     :%x\n",
				q_cmpt_ctxt.en_int);
		xdebug_info(qdma_dev, "\t\t cause status descwr :%x\n\n",
				q_cmpt_ctxt.en_stat_desc);

		/* Prefetch context */
		xdebug_info(qdma_dev, "\t\t valid               :%x\n",
				q_prefetch_ctxt.valid);
		xdebug_info(qdma_dev, "\t\t software credit     :%x\n",
				q_prefetch_ctxt.sw_crdt);
		xdebug_info(qdma_dev, "\t\t queue is in prefetch:%x\n",
				q_prefetch_ctxt.pfch);
		xdebug_info(qdma_dev, "\t\t enable prefetch     :%x\n",
				q_prefetch_ctxt.pfch_en);
		xdebug_info(qdma_dev, "\t\t error               :%x\n",
				q_prefetch_ctxt.err);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_prefetch_ctxt.rsvd0);
		xdebug_info(qdma_dev, "\t\t port ID             :%x\n",
				q_prefetch_ctxt.port_id);
		xdebug_info(qdma_dev, "\t\t buffer size index   :%x\n",
				q_prefetch_ctxt.bufsz_idx);
		xdebug_info(qdma_dev, "\t\t C2H in bypass mode  :%x\n\n",
				q_prefetch_ctxt.bypass);

		/* C2H HW context */
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_hw_ctxt.rsvd1);
		xdebug_info(qdma_dev, "\t\t fetch pending       :%x\n",
				q_hw_ctxt.fetch_pnd);
		xdebug_info(qdma_dev, "\t\t event pending       :%x\n",
				q_hw_ctxt.evt_pnd);
		xdebug_info(qdma_dev, "\t\t Q-inval/no desc pend:%x\n",
				q_hw_ctxt.idl_stp_b);
		xdebug_info(qdma_dev, "\t\t descriptor pending  :%x\n",
				q_hw_ctxt.dsc_pend);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_hw_ctxt.rsvd);
		xdebug_info(qdma_dev, "\t\t credit-use          :%x\n",
				q_hw_ctxt.crd_use);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n\n",
				q_hw_ctxt.cidx);
	}

	if (queue >= dev->data->nb_tx_queues) {
		PMD_DRV_LOG(INFO, " TX queue_id=%d not configured\n", queue);
	} else {
		print_header(" *************H2C Queue Contexts************** ");

		memset(&q_sw_ctxt, 0, sizeof(struct qdma_descq_sw_ctxt));
		memset(&q_hw_ctxt, 0, sizeof(struct qdma_descq_hw_ctxt));

		qdma_sw_context_read(dev, 0, qid, &q_sw_ctxt);
		qdma_hw_context_read(dev, 0, qid, &q_hw_ctxt);

		/* H2C SW context */
		xdebug_info(qdma_dev, "\t\t intr_aggr           :%x\n",
				q_sw_ctxt.intr_aggr);
		xdebug_info(qdma_dev, "\t\t vec                 :%x\n",
				q_sw_ctxt.vec);
		xdebug_info(qdma_dev, "\t\t Ring Base-addr      :%lx\n",
				q_sw_ctxt.ring_bs_addr);
		xdebug_info(qdma_dev, "\t\t is_mm               :%x\n",
				q_sw_ctxt.is_mm);
		xdebug_info(qdma_dev, "\t\t mrkr_dis            :%x\n",
				q_sw_ctxt.mrkr_dis);
		xdebug_info(qdma_dev, "\t\t irq_req             :%x\n",
				q_sw_ctxt.irq_req);
		xdebug_info(qdma_dev, "\t\t err_wb_sent         :%x\n",
				q_sw_ctxt.err_wb_sent);
		xdebug_info(qdma_dev, "\t\t Error status        :%x\n",
				q_sw_ctxt.err);
		xdebug_info(qdma_dev, "\t\t irq_no_last         :%x\n",
				q_sw_ctxt.irq_no_last);
		xdebug_info(qdma_dev, "\t\t port-id             :%x\n",
				q_sw_ctxt.port_id);
		xdebug_info(qdma_dev, "\t\t irq-enable          :%x\n",
				q_sw_ctxt.irq_en);
		xdebug_info(qdma_dev, "\t\t write-back enable   :%x\n",
				q_sw_ctxt.wbk_en);
		xdebug_info(qdma_dev, "\t\t mm-channel-id       :%x\n",
				q_sw_ctxt.mm_chn);
		xdebug_info(qdma_dev, "\t\t bypass              :%x\n",
				q_sw_ctxt.bypass);
		xdebug_info(qdma_dev, "\t\t desc-size index     :%x\n",
				q_sw_ctxt.desc_sz);
		xdebug_info(qdma_dev, "\t\t ring-size index     :%x\n",
				q_sw_ctxt.rngsz_idx);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_sw_ctxt.rsvd1);
		xdebug_info(qdma_dev, "\t\t fetch_max           :%x\n",
				q_sw_ctxt.fetch_max);
		xdebug_info(qdma_dev, "\t\t at                  :%x\n",
				q_sw_ctxt.at);
		xdebug_info(qdma_dev, "\t\t wbi_intvl_en        :%x\n",
				q_sw_ctxt.wbi_intvl_en);
		xdebug_info(qdma_dev, "\t\t wbi_chk             :%x\n",
				q_sw_ctxt.wbi_chk);
		xdebug_info(qdma_dev, "\t\t fetch credits       :%x\n",
				q_sw_ctxt.frcd_en);
		xdebug_info(qdma_dev, "\t\t queue-enable        :%x\n",
				q_sw_ctxt.qen);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_sw_ctxt.rsvd0);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				q_sw_ctxt.fnc_id);
		xdebug_info(qdma_dev, "\t\t irq_arm             :%x\n",
				q_sw_ctxt.irq_arm);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n\n",
				q_sw_ctxt.pidx);

		/* H2C HW context */
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_hw_ctxt.rsvd1);
		xdebug_info(qdma_dev, "\t\t fetch pending       :%x\n",
				q_hw_ctxt.fetch_pnd);
		xdebug_info(qdma_dev, "\t\t event pending       :%x\n",
				q_hw_ctxt.evt_pnd);
		xdebug_info(qdma_dev, "\t\t Q-inval/no desc pend:%x\n",
				q_hw_ctxt.idl_stp_b);
		xdebug_info(qdma_dev, "\t\t descriptor pending  :%x\n",
				q_hw_ctxt.dsc_pend);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_hw_ctxt.rsvd);
		xdebug_info(qdma_dev, "\t\t credit-use          :%x\n",
				q_hw_ctxt.crd_use);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n\n",
				q_hw_ctxt.cidx);
	}

	return 0;
}
static int qdma_queue_desc_dump(uint8_t port_id,
				struct rte_pmd_qdma_xdebug_desc_param *param)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	int x;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	char str[50];

	switch (param->type) {
	case RTE_PMD_QDMA_XDEBUG_DESC_C2H:

		if (param->queue >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq) {
			if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
				printf("Queue_id %d is not yet started\n",
						param->queue);
				return -1;
			}

			if (param->start < 0 || param->start > rxq->nb_rx_desc)
				param->start = 0;
			if (param->end <= param->start ||
					param->end > rxq->nb_rx_desc)
				param->end = rxq->nb_rx_desc;

			if (rxq->st_mode) {
				struct qdma_c2h_desc *rx_ring_st =
					(struct qdma_c2h_desc *)rxq->rx_ring;

				printf("\n===== C2H ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					struct qdma_c2h_desc *rx_st =
						&rx_ring_st[x];
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)rx_st,
						sizeof(struct qdma_c2h_desc));
				}
			} else {
				struct qdma_mm_desc *rx_ring_mm =
					(struct qdma_mm_desc *)rxq->rx_ring;
				printf("\n====== C2H ring descriptors======\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&rx_ring_mm[x],
						sizeof(struct qdma_mm_desc));
				}
			}
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_DESC_CMPT:

		if (param->queue >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq) {
			if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
				printf("Queue_id %d is not yet started\n",
						param->queue);
				return -1;
			}

			if (param->start < 0 ||
					param->start > rxq->nb_rx_cmpt_desc)
				param->start = 0;
			if (param->end <= param->start ||
					param->end > rxq->nb_rx_cmpt_desc)
				param->end = rxq->nb_rx_cmpt_desc;

			if (!rxq->st_mode) {
				printf("Queue_id %d is not initialized "
					"in Stream mode\n", param->queue);
				return -1;
			}

			printf("\n===== CMPT ring descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint32_t *cmpt_ring = (uint32_t *)
					((uint64_t)(rxq->cmpt_ring) +
					((uint64_t)x * rxq->cmpt_desc_len));
				sprintf(str, "\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
						(const void *)cmpt_ring,
						rxq->cmpt_desc_len);
			}
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_DESC_H2C:

		if (param->queue >= dev->data->nb_tx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		txq = (struct qdma_tx_queue *)
			dev->data->tx_queues[param->queue];

		if (txq) {
			if (txq->status != RTE_ETH_QUEUE_STATE_STARTED) {
				printf("Queue_id %d is not yet started\n",
						param->queue);
				return -1;
			}

			if (param->start < 0 || param->start > txq->nb_tx_desc)
				param->start = 0;
			if (param->end <= param->start ||
					param->end > txq->nb_tx_desc)
				param->end = txq->nb_tx_desc;

			if (txq->st_mode) {
				struct qdma_h2c_desc *qdma_h2c_ring =
					(struct qdma_h2c_desc *)txq->tx_ring;
				printf("\n====== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&qdma_h2c_ring[x],
						sizeof(struct qdma_h2c_desc));
				}
			} else {
				struct qdma_mm_desc *tx_ring_mm =
					(struct qdma_mm_desc *)txq->tx_ring;
				printf("\n===== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&tx_ring_mm[x],
						sizeof(struct qdma_mm_desc));
				}
			}
		}
		break;
	default:
		printf("Invalid ring selected\n");
		break;
	}
	return 0;
}

int rte_pmd_qdma_xdebug(uint8_t port_id, enum rte_pmd_qdma_xdebug_type type,
		void *params)
{
	int err = -ENOTSUP;

	switch (type) {
	case RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR:
		err = qdma_csr_dump();
		if (err) {
			xdebug_info(adap, "Error dumping Global registers\n");
			return err;
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_QUEUE_CONTEXT:

		if (!params) {
			printf("QID required for Queue context dump\n");
			return -1;
		}

		err = qdma_context_dump(port_id, *(uint16_t *)params);
		if (err) {
			xdebug_info(adap, "Error dumping %d: %d\n",
					*(int *)params, err);
			return err;
		}
		break;

	case RTE_PMD_QDMA_XDEBUG_QUEUE_STRUCT:

		if (!params) {
			printf("QID required for Queue Structure dump\n");
			return -1;
		}

		err = qdma_struct_dump(port_id, *(uint16_t *)params);
		if (err) {
			xdebug_info(adap, "Error dumping %d: %d\n",
					*(int *)params, err);
			return err;
		}
		break;

	case RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP:

		if (!params) {
			printf("Params required for Queue descriptor dump\n");
			return -1;
		}

		err = qdma_queue_desc_dump(port_id,
			(struct rte_pmd_qdma_xdebug_desc_param *)params);
			if (err) {
				xdebug_info(adap, "Error dumping %d: %d\n",
					*(int *)params, err);
			return err;
		}
		break;

	default:
		xdebug_info(adap, "Unsupported type\n");
		return err;
	}

	return 0;
}
