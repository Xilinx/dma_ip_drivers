/*
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
#include <rte_cycles.h>
#include <unistd.h>
#include <string.h>
#include <rte_hexdump.h>

#include "qdma.h"
#include "qdma_export.h"
#include "qdma_regs.h"

union __attribute__ ((packed)) h2c_c2h_ctxt
{
	struct  __attribute__ ((packed))  ctxt_data
	{
		uint32_t data[5];
	} c_data;
	struct  __attribute__ ((packed)) ctxt_fields
	{
		uint16_t pidx;

		uint16_t irq_arm:1;
		uint16_t fnc_id:8;
		uint16_t rsv0:7;

		uint16_t qen:1;
		uint16_t fcrd_en:1;
		uint16_t wbi_chk:1;
		uint16_t wbi_intvl_en:1;
		uint16_t at:1;
		uint16_t fetch_max:3;
		uint16_t rsv1:4;
		uint16_t rng_sz:4;

		uint16_t dsc_sz:2;
		uint16_t byp:1;
		uint16_t mm_chn:1;
		uint16_t wbk_en:1;
		uint16_t irq_en:1;
		uint16_t port_id:3;
		uint16_t irq_no_last:1;
		uint16_t err:2;
		uint16_t err_wb_sent:1;
		uint16_t irq_req:1;
		uint16_t mrkr_dis:1;
		uint16_t is_mm:1;

		uint64_t dsc_base;

		uint16_t vec:10;
		uint16_t int_aggr:1;
	} c_fields;
};

union __attribute__ ((packed)) c2h_cmpt_ctxt
{
	struct  __attribute__ ((packed))  c2h_cmpt_data
	{
		uint32_t data[5];
	} c_data;
	struct  __attribute__ ((packed)) c2h_cmpt_fields
	{
		uint32_t en_stat_desc:1;
		uint32_t en_int:1;
		uint32_t trig_mode:3;
		uint32_t fnc_id:8;
		uint32_t rsv0:4;
		uint32_t count_idx:4;
		uint32_t timer_idx:4;
		uint32_t int_st:2;
		uint32_t color:1;
		uint32_t qsize_idx:4;

		uint64_t rsv1:6;
		uint64_t baddr:52;
		uint64_t desc_sz:2;
		uint64_t pidx_l:4;

		uint32_t pidx_h:12;
		uint32_t cidx:16;
		uint32_t valid:1;
		uint32_t err:2;
		uint32_t user_trig_pend:1;

		uint32_t timer_running:1;
		uint32_t full_upd:1;
		uint32_t ovf_chk_dis:1;
		uint32_t at:1;
		uint32_t vec:11;
		uint32_t int_aggr:1;
		uint32_t rsv2:17;
	} c_fields;
};

union __attribute__ ((packed)) h2c_c2h_hw_ctxt
{
	struct  __attribute__ ((packed))  hw_ctxt_data
	{
		uint32_t data[2];
	} c_data;
	struct  __attribute__ ((packed)) hw_ctxt_fields
	{
		uint16_t cidx;
		uint16_t crd_use;

		uint16_t rsv0:8;
		uint16_t dsc_pnd:1;
		uint16_t idl_stp_b:1;
		uint16_t evt_pnd:1;
		uint16_t fetch_pnd:4;
		uint16_t rsv1:1;

	} c_fields;
};

union __attribute__ ((packed)) prefetch_ctxt
{
	struct __attribute__ ((packed)) pref_ctxt_data
	{
		uint32_t data[2];
	} c_data;

	struct __attribute__ ((packed)) pref_ctxt_fields
	{
		uint64_t bypass:1;
		uint64_t buf_sz_idx:4;
		uint64_t port_id:3;
		uint64_t rsvd:18;
		uint64_t err:1;
		uint64_t pfch_en:1;
		uint64_t pfch:1;
		uint64_t sw_crdt:16;
		uint64_t valid:1;
	} c_fields;
};

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

		if(rx_q) {
			print_header(" ***********RX Queue struct********** ");
			xdebug_info(dev, "\t\t wb_pidx             :%x\n",
					rx_q->wb_status->pidx);
			xdebug_info(dev, "\t\t wb_cidx             :%x\n",
					rx_q->wb_status->cidx);
			xdebug_info(dev, "\t\t rx_pidx             :%x\n",
					*rx_q->rx_pidx);
			xdebug_info(dev, "\t\t rx_cidx             :%x\n",
					*rx_q->rx_cidx);
			xdebug_info(dev, "\t\t rx_tail             :%x\n",
					rx_q->rx_tail);
			xdebug_info(dev, "\t\t c2h_pidx            :%x\n",
					rx_q->c2h_pidx);
			xdebug_info(dev, "\t\t rx_cmpt_tail        :%x\n",
					rx_q->rx_cmpt_tail);
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

		if(tx_q) {
			print_header("***********TX Queue struct************");
			xdebug_info(dev, "\t\t wb_pidx             :%x\n",
					tx_q->wb_status->pidx);
			xdebug_info(dev, "\t\t wb_cidx             :%x\n",
					tx_q->wb_status->cidx);
			xdebug_info(dev, "\t\t tx_pidx             :%x\n",
					*tx_q->tx_pidx);
			xdebug_info(dev, "\t\t tx_tail             :%x\n",
					tx_q->tx_tail);
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
	uint64_t bar_addr;
	uint32_t reg_val;

	if (rte_eth_dev_count() < 1)
		return -1;

	dev = &rte_eth_devices[0];
	qdma_dev = dev->data->dev_private;
	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];

	xdebug_info(dev, "FPGA Registers\n--------------\n");
	xdebug_info(dev, "NAME                               offset       "
			"description               register-Value\n");
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x0));
	xdebug_info(dev, "FPGA_VER                           0x00000000   "
			"FPGA Version              %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x204));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[0]              0x00000204   "
			"Ring size index 0         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x208));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[1]              0x00000208   "
			"Ring size index 1         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x20c));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[2]              0x0000020c   "
			"Ring size index 2         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x210));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[3]              0x00000210   "
			"Ring size index 3         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x214));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[4]              0x00000214   "
			"Ring size index 4         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x218));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[5]              0x00000218   "
			"Ring size index 5         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x21c));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[6]              0x0000021c   "
			"Ring size index 6         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x220));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[7]              0x00000220   "
			"Ring size index 7         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x224));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[8]              0x00000224   "
			"Ring size index 8         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x228));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[9]              0x00000228   "
			"Ring size index 9         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x22c));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[10]             0x0000022c   "
			"Ring size index 10        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x230));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[11]             0x00000230   "
			"Ring size index 11        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x234));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[12]             0x00000234   "
			"Ring size index 12        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x238));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[13]             0x00000238   "
			"Ring size index 13        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x23c));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[14]             0x0000023c   "
			"Ring size index 14        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x240));
	xdebug_info(dev, "QDMA_GLBL_RNG_SZ_A[15]             0x00000240   "
			"Ring size index 15        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x248));
	xdebug_info(dev, "QDMA_GLBL_ERR_STAT                 0x00000248   "
			"Global error status       %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x24c));
	xdebug_info(dev, "QDMA_GLBL_ERR_MASK                 0x0000024c   "
			"Global error mask         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x250));
	xdebug_info(dev, "QDMA_GLBL_DSC_CFG                  0x00000250   "
			"Global Desc configuration %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x254));
	xdebug_info(dev, "QDMA_GLBL_DSC_ERR_STS              0x00000254   "
			"Global Desc error status  %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x258));
	xdebug_info(dev, "QDMA_GLBL_DSC_ERR_MSK              0x00000258   "
			"Global Desc error Mask    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x25c));
	xdebug_info(dev, "QDMA_GLBL_DSC_ERR_LOG0             0x0000025c   "
			"Global Desc error log0    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x260));
	xdebug_info(dev, "QDMA_GLBL_DSC_ERR_LOG1             0x00000260   "
			"Global Desc error log1    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x264));
	xdebug_info(dev, "QDMA_GLBL_TRQ_ERR_STS              0x00000264   "
			"Glbl target error status  %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x268));
	xdebug_info(dev, "QDMA_GLBL_TRQ_ERR_MSK              0x00000268   "
			"Glbl target error Mask    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x26c));
	xdebug_info(dev, "QDMA_GLBL_TRQ_ERR_LOG              0x0000026c   "
			"Register access space     %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x400));
	xdebug_info(dev, "QDMA_TRQ_SEL_FMAP[0]               0x00000400   "
			"FMAP target index-0       %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0x404));
	xdebug_info(dev, "QDMA_TRQ_SEL_FMAP[1]               0x00000404   "
			"FMAP target index-1       %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa88));
	xdebug_info(dev, "QDMA_C2H_STAT_AXIS_C2H_ACPTD       0x00000a88   "
			"C2H pkts accepted         %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa8c));
	xdebug_info(dev, "QDMA_C2H_STAT_AXIS_CMPT_ACPTD      0x00000a8c   "
			"C2H CMPT pkts accepted    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa90));
	xdebug_info(dev, "QDMA_C2H_STAT_DESC_RSP_PKT_ACPTD   0x00000a90   "
			"desc rsp pkts accepted    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa94));
	xdebug_info(dev, "QDMA_C2H_STAT_AXIS_PKG_CMP         0x00000a94   "
			"C2H pkts completed        %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa98));
	xdebug_info(dev, "QDMA_C2H_STAT_DESC_RSP_ACPTD       0x00000a98   "
			"dsc rsp pkts accepted     %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xa9c));
	xdebug_info(dev, "QDMA_C2H_STAT_DESC_RSP_CMP D       0x00000a9c   "
			"dsc rsp pkts completed    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xaa0));
	xdebug_info(dev, "QDMA_C2H_STAT_WRQ_OUT              0x00000aa0   "
			"Number of WRQ             %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xaa4));
	xdebug_info(dev, "QDMA_C2H_STAT_WPL_REN_ACPTD        0x00000aa4   "
			"WPL REN accepted          %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xaf0));
	xdebug_info(dev, "QDMA_GLBL_ERR_STAT                 0x00000af0   "
			"C2H error status          %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xaf4));
	xdebug_info(dev, "QDMA_GLBL_ERR_MASK                 0x00000af4   "
			"C2H error Mask            %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xaf8));
	xdebug_info(dev, "QDMA_C2H_FATAL_ERR_STAT            0x00000af8   "
			"C2H fatal error status    %x\n", reg_val);
	reg_val = qdma_read_reg((uint64_t)(bar_addr + 0xafc));
	xdebug_info(dev, "QDMA_C2H_FATAL_ERR_MSK             0x00000afc   "
			"C2H fatal error Mask      %x\n", reg_val);

	return 0;
}

static int qdma_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct queue_ind_prg *q_regs;
	uint32_t reg_val, ctxt_sel;
	uint64_t bar_addr;
	uint16_t flag, i;
	uint16_t qid = qdma_dev->queue_base + queue;

	union h2c_c2h_ctxt q_ctxt;
	union c2h_cmpt_ctxt c2h_cmpt;
	union h2c_c2h_hw_ctxt hw_ctxt;
	union prefetch_ctxt pref_ctxt;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
			QDMA_IND_Q_PRG_OFF);

	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[0], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[1], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[2], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[3], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[4], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[5], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[6], 0xffffffff);
	qdma_write_reg((uint64_t)&q_regs->ctxt_mask[7], 0xffffffff);

	if (queue >= dev->data->nb_rx_queues) {
		PMD_DRV_LOG(INFO, " RX queue_id=%d not configured\n", queue);
	} else {
		print_header(" *************C2H Queue Contexts************** ");
		/* SW context */
		ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_C2H << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 5; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			q_ctxt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t SW                  "
				":0x%x  0x%x  0x%x  0x%x 0x%x\n",
				qid, q_ctxt.c_data.data[4],
				q_ctxt.c_data.data[3], q_ctxt.c_data.data[2],
				q_ctxt.c_data.data[1], q_ctxt.c_data.data[0]);
		xdebug_info(qdma_dev, "\t\t int_aggr            :%x\n",
				q_ctxt.c_fields.int_aggr);
		xdebug_info(qdma_dev, "\t\t vec                 :%x\n",
				q_ctxt.c_fields.vec);
		xdebug_info(qdma_dev, "\t\t Base-addr           :%lx\n",
				q_ctxt.c_fields.dsc_base);
		xdebug_info(qdma_dev, "\t\t is_mm               :%x\n",
				q_ctxt.c_fields.is_mm);
		xdebug_info(qdma_dev, "\t\t mark_dis            :%x\n",
				q_ctxt.c_fields.mrkr_dis);
		xdebug_info(qdma_dev, "\t\t irq_req             :%x\n",
				q_ctxt.c_fields.irq_req);
		xdebug_info(qdma_dev, "\t\t err-cmpt-sent       :%x\n",
				q_ctxt.c_fields.err_wb_sent);
		xdebug_info(qdma_dev, "\t\t Error status        :%x\n",
				q_ctxt.c_fields.err);
		xdebug_info(qdma_dev, "\t\t irq_no_last         :%x\n",
				q_ctxt.c_fields.irq_no_last);
		xdebug_info(qdma_dev, "\t\t port-id             :%x\n",
				q_ctxt.c_fields.port_id);
		xdebug_info(qdma_dev, "\t\t irq-enable          :%x\n",
				q_ctxt.c_fields.irq_en);
		xdebug_info(qdma_dev, "\t\t write-back enable   :%x\n",
				q_ctxt.c_fields.wbk_en);
		xdebug_info(qdma_dev, "\t\t mm-channel-id       :%x\n",
				q_ctxt.c_fields.mm_chn);
		xdebug_info(qdma_dev, "\t\t bypass              :%x\n",
				q_ctxt.c_fields.byp);
		xdebug_info(qdma_dev, "\t\t desc-size index     :%x\n",
				q_ctxt.c_fields.dsc_sz);
		xdebug_info(qdma_dev, "\t\t ring-size index     :%x\n",
				q_ctxt.c_fields.rng_sz);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_ctxt.c_fields.rsv1);
		xdebug_info(qdma_dev, "\t\t fetch_max           :%x\n",
				q_ctxt.c_fields.fetch_max);
		xdebug_info(qdma_dev, "\t\t at                  :%x\n",
				q_ctxt.c_fields.at);
		xdebug_info(qdma_dev, "\t\t wbi_intvl_en        :%x\n",
				q_ctxt.c_fields.wbi_intvl_en);
		xdebug_info(qdma_dev, "\t\t wbi_chk             :%x\n",
				q_ctxt.c_fields.wbi_chk);
		xdebug_info(qdma_dev, "\t\t fetch credits       :%x\n",
				q_ctxt.c_fields.fcrd_en);
		xdebug_info(qdma_dev, "\t\t queue-enable        :%x\n",
				q_ctxt.c_fields.qen);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_ctxt.c_fields.rsv0);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				q_ctxt.c_fields.fnc_id);
		xdebug_info(qdma_dev, "\t\t irq_arm             :%x\n",
				q_ctxt.c_fields.irq_arm);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n\n",
				q_ctxt.c_fields.pidx);

		/* c2h cmpt context */
		ctxt_sel = (QDMA_CTXT_SEL_DESC_CMPT << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 5; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			c2h_cmpt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t C2H-CMPT             "
				":0x%x  0x%x  0x%x  0x%x 0x%x\n",
				qid, c2h_cmpt.c_data.data[4],
				c2h_cmpt.c_data.data[3],
				c2h_cmpt.c_data.data[2],
				c2h_cmpt.c_data.data[1],
				c2h_cmpt.c_data.data[0]);

		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				c2h_cmpt.c_fields.rsv2);
		xdebug_info(qdma_dev, "\t\t intr aggr           :%x\n",
				c2h_cmpt.c_fields.int_aggr);
		xdebug_info(qdma_dev, "\t\t atc                 :%x\n",
				c2h_cmpt.c_fields.at);
		xdebug_info(qdma_dev, "\t\t Overflow Chk Disable:%x\n",
				c2h_cmpt.c_fields.ovf_chk_dis);
		xdebug_info(qdma_dev, "\t\t Full Update         :%x\n",
				c2h_cmpt.c_fields.full_upd);
		xdebug_info(qdma_dev, "\t\t timer_running       :%x\n",
				c2h_cmpt.c_fields.timer_running);
		xdebug_info(qdma_dev, "\t\t user_trig_pend      :%x\n",
				c2h_cmpt.c_fields.user_trig_pend);
		xdebug_info(qdma_dev, "\t\t error status d      :%x\n",
				c2h_cmpt.c_fields.err);
		xdebug_info(qdma_dev, "\t\t valid               :%x\n",
				c2h_cmpt.c_fields.valid);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n",
				c2h_cmpt.c_fields.cidx);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n",
				(c2h_cmpt.c_fields.pidx_h << 4 |
				 c2h_cmpt.c_fields.pidx_l));
		xdebug_info(qdma_dev, "\t\t desc-size           :%x\n",
				c2h_cmpt.c_fields.desc_sz);
		xdebug_info(qdma_dev, "\t\t baddr(4K/22 bits)   :%lx\n",
				(unsigned long int)c2h_cmpt.c_fields.baddr);
		xdebug_info(qdma_dev, "\t\t size-index          :%x\n",
				c2h_cmpt.c_fields.qsize_idx);
		xdebug_info(qdma_dev, "\t\t color               :%x\n",
				c2h_cmpt.c_fields.color);
		xdebug_info(qdma_dev, "\t\t interrupt-state     :%x\n",
				c2h_cmpt.c_fields.int_st);
		xdebug_info(qdma_dev, "\t\t timer-index         :%x\n",
				c2h_cmpt.c_fields.timer_idx);
		xdebug_info(qdma_dev, "\t\t counter-index       :%x\n",
				c2h_cmpt.c_fields.count_idx);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				c2h_cmpt.c_fields.fnc_id);
		xdebug_info(qdma_dev, "\t\t trigger-mode        :%x\n",
				c2h_cmpt.c_fields.trig_mode);
		xdebug_info(qdma_dev, "\t\t cause interrupt     :%x\n",
				c2h_cmpt.c_fields.en_int);
		xdebug_info(qdma_dev, "\t\t cause status descwr :%x\n\n",
				c2h_cmpt.c_fields.en_stat_desc);

		/* pfch context */
		ctxt_sel = (QDMA_CTXT_SEL_PFTCH << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 2; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			pref_ctxt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t PFCH                "
				":0x%x  0x%x\n",
				qid, c2h_cmpt.c_data.data[1],
				c2h_cmpt.c_data.data[0]);
		xdebug_info(qdma_dev, "\t\t valid               :%x\n",
				pref_ctxt.c_fields.valid);
		xdebug_info(qdma_dev, "\t\t software credit     :%x\n",
				pref_ctxt.c_fields.sw_crdt);
		xdebug_info(qdma_dev, "\t\t queue is in prefetch:%x\n",
				pref_ctxt.c_fields.pfch);
		xdebug_info(qdma_dev, "\t\t enable prefetch     :%x\n",
				pref_ctxt.c_fields.pfch_en);
		xdebug_info(qdma_dev, "\t\t error               :%x\n",
				pref_ctxt.c_fields.err);
		xdebug_info(qdma_dev, "\t\t port ID             :%x\n",
				pref_ctxt.c_fields.port_id);
		xdebug_info(qdma_dev, "\t\t buffer size index   :%x\n",
				pref_ctxt.c_fields.buf_sz_idx);
		xdebug_info(qdma_dev, "\t\t C2H in bypass mode  :%x\n\n",
				pref_ctxt.c_fields.bypass);

		/* hwc c2h context */
		ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_C2H << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 2; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			hw_ctxt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t HW                  "
				":0x%x  0x%x\n",
				qid, hw_ctxt.c_data.data[1],
				hw_ctxt.c_data.data[0]);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				hw_ctxt.c_fields.rsv1);
		xdebug_info(qdma_dev, "\t\t fetch pending       :%x\n",
				hw_ctxt.c_fields.fetch_pnd);
		xdebug_info(qdma_dev, "\t\t event pending       :%x\n",
				hw_ctxt.c_fields.evt_pnd);
		xdebug_info(qdma_dev, "\t\t Q-inval/no desc pend:%x\n",
				hw_ctxt.c_fields.idl_stp_b);
		xdebug_info(qdma_dev, "\t\t descriptor pending  :%x\n",
				hw_ctxt.c_fields.dsc_pnd);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				hw_ctxt.c_fields.rsv0);
		xdebug_info(qdma_dev, "\t\t credit-use          :%x\n",
				hw_ctxt.c_fields.crd_use);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n\n",
				hw_ctxt.c_fields.cidx);
	}

	if (queue >= dev->data->nb_tx_queues) {
		PMD_DRV_LOG(INFO, " TX queue_id=%d not configured\n", queue);
	} else {
		print_header(" *************H2C Queue Contexts************** ");
		/* SW context */
		ctxt_sel = (QDMA_CTXT_SEL_DESC_SW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 5; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			q_ctxt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t SW                  "
				":0x%x  0x%x  0x%x  0x%x 0x%x\n",
				qid, q_ctxt.c_data.data[4],
				q_ctxt.c_data.data[3], q_ctxt.c_data.data[2],
				q_ctxt.c_data.data[1], q_ctxt.c_data.data[0]);
		xdebug_info(qdma_dev, "\t\t int_aggr            :%x\n",
				q_ctxt.c_fields.int_aggr);
		xdebug_info(qdma_dev, "\t\t vec                 :%x\n",
				q_ctxt.c_fields.vec);
		xdebug_info(qdma_dev, "\t\t Base-addr           :%lx\n",
				q_ctxt.c_fields.dsc_base);
		xdebug_info(qdma_dev, "\t\t is_mm               :%x\n",
				q_ctxt.c_fields.is_mm);
		xdebug_info(qdma_dev, "\t\t mark_dis            :%x\n",
				q_ctxt.c_fields.mrkr_dis);
		xdebug_info(qdma_dev, "\t\t irq_req             :%x\n",
				q_ctxt.c_fields.irq_req);
		xdebug_info(qdma_dev, "\t\t err-cmpt-sent       :%x\n",
				q_ctxt.c_fields.err_wb_sent);
		xdebug_info(qdma_dev, "\t\t Error status        :%x\n",
				q_ctxt.c_fields.err);
		xdebug_info(qdma_dev, "\t\t irq_no_last         :%x\n",
				q_ctxt.c_fields.irq_no_last);
		xdebug_info(qdma_dev, "\t\t port-id             :%x\n",
				q_ctxt.c_fields.port_id);
		xdebug_info(qdma_dev, "\t\t irq-enable          :%x\n",
				q_ctxt.c_fields.irq_en);
		xdebug_info(qdma_dev, "\t\t write-back enable   :%x\n",
				q_ctxt.c_fields.wbk_en);
		xdebug_info(qdma_dev, "\t\t mm-channel-id       :%x\n",
				q_ctxt.c_fields.mm_chn);
		xdebug_info(qdma_dev, "\t\t bypass              :%x\n",
				q_ctxt.c_fields.byp);
		xdebug_info(qdma_dev, "\t\t desc-size index     :%x\n",
				q_ctxt.c_fields.dsc_sz);
		xdebug_info(qdma_dev, "\t\t ring-size index     :%x\n",
				q_ctxt.c_fields.rng_sz);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_ctxt.c_fields.rsv1);
		xdebug_info(qdma_dev, "\t\t fetch_max           :%x\n",
				q_ctxt.c_fields.fetch_max);
		xdebug_info(qdma_dev, "\t\t at                  :%x\n",
				q_ctxt.c_fields.at);
		xdebug_info(qdma_dev, "\t\t wbi_intvl_en        :%x\n",
				q_ctxt.c_fields.wbi_intvl_en);
		xdebug_info(qdma_dev, "\t\t wbi_chk             :%x\n",
				q_ctxt.c_fields.wbi_chk);
		xdebug_info(qdma_dev, "\t\t fetch credits       :%x\n",
				q_ctxt.c_fields.fcrd_en);
		xdebug_info(qdma_dev, "\t\t queue-enable        :%x\n",
				q_ctxt.c_fields.qen);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				q_ctxt.c_fields.rsv0);
		xdebug_info(qdma_dev, "\t\t function-id         :%x\n",
				q_ctxt.c_fields.fnc_id);
		xdebug_info(qdma_dev, "\t\t irq_arm             :%x\n",
				q_ctxt.c_fields.irq_arm);
		xdebug_info(qdma_dev, "\t\t producer-index      :%x\n\n",
				q_ctxt.c_fields.pidx);

		/* hwc h2c context */
		ctxt_sel = (QDMA_CTXT_SEL_DESC_HW_H2C << CTXT_SEL_SHIFT_B) |
				(qid << QID_SHIFT_B) |
				(QDMA_CTXT_CMD_RD << OP_CODE_SHIFT_B);
		qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

		flag = 1;
		i = CONTEXT_PROG_POLL_COUNT;
		while (flag && i) {
			reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
			PMD_DRV_LOG(DEBUG, " Read cmd for queue-id:%d: "
					"reg_val:%x\n", qid, reg_val);
			flag = reg_val & BUSY_BIT_MSK;
			rte_delay_ms(CONTEXT_PROG_DELAY);
			i--;
		}
		if (flag) {
			PMD_DRV_LOG(ERR, "Error: Busy on queue-id:%d: "
					"initailization with cmd reg_val:%x\n",
					qid, reg_val);
			return -1;
		}

		for (i = 0; i < 2; i++) {
			reg_val = qdma_read_reg((uint64_t)&
					q_regs->ctxt_data[i]);
			hw_ctxt.c_data.data[i] = reg_val;
		}

		xdebug_info(qdma_dev, "Q-id: %d, \t HW                  "
				":0x%x  0x%x\n",
				qid, hw_ctxt.c_data.data[1],
				hw_ctxt.c_data.data[0]);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				hw_ctxt.c_fields.rsv1);
		xdebug_info(qdma_dev, "\t\t fetch pending       :%x\n",
				hw_ctxt.c_fields.fetch_pnd);
		xdebug_info(qdma_dev, "\t\t event pending       :%x\n",
				hw_ctxt.c_fields.evt_pnd);
		xdebug_info(qdma_dev, "\t\t Q-inval/no desc pend:%x\n",
				hw_ctxt.c_fields.idl_stp_b);
		xdebug_info(qdma_dev, "\t\t descriptor pending  :%x\n",
				hw_ctxt.c_fields.dsc_pnd);
		xdebug_info(qdma_dev, "\t\t reserved            :%x\n",
				hw_ctxt.c_fields.rsv0);
		xdebug_info(qdma_dev, "\t\t credit-use          :%x\n",
				hw_ctxt.c_fields.crd_use);
		xdebug_info(qdma_dev, "\t\t consumer-index      :%x\n\n",
				hw_ctxt.c_fields.cidx);
	}

	return 0;
}
static int qdma_queue_desc_dump(uint8_t port_id,
				struct rte_xdebug_desc_param *param)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	int x;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	char str[50];

	switch (param->type) {
	case ETH_XDEBUG_DESC_C2H:

		if (param->queue >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if(rxq) {
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
	case ETH_XDEBUG_DESC_CMPT:

		if (param->queue >= dev->data->nb_rx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if(rxq) {
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
	case ETH_XDEBUG_DESC_H2C:

		if (param->queue >= dev->data->nb_tx_queues) {
			PMD_DRV_LOG(INFO, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		txq = (struct qdma_tx_queue *)
			dev->data->tx_queues[param->queue];

		if(txq) {
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

int qdma_xdebug(uint8_t port_id, enum rte_xdebug_type type,
		void *params)
{
	int err = -ENOTSUP;

	switch (type) {
	case ETH_XDEBUG_QDMA_GLOBAL_CSR:
		err = qdma_csr_dump();
		if (err) {
			xdebug_info(adap, "Error dumping Global registers\n");
			return err;
		}
		break;
	case ETH_XDEBUG_QUEUE_CONTEXT:

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

	case ETH_XDEBUG_QUEUE_STRUCT:

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

	case ETH_XDEBUG_QUEUE_DESC_DUMP:

		if (!params) {
			printf("Params required for Queue descriptor dump\n");
			return -1;
		}

		err = qdma_queue_desc_dump(port_id,
				(struct rte_xdebug_desc_param *)params);
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
