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
#include <stdbool.h>
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
#include <rte_alarm.h>
#include <unistd.h>
#include <string.h>
#include <linux/pci.h>

#include "qdma.h"
#include "qdma_regs.h"
#include "version.h"

/*
 * The set of PCI devices this driver supports
 */
static struct rte_pci_id qdma_vf_pci_id_tbl[] = {
#define RTE_PCI_DEV_ID_DECL_XNIC(vend, dev) {RTE_PCI_DEVICE(vend, dev)},
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif

	/** Gen 1 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa011)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa111)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa211)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa311)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa014)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa114)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa214)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa314)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa018)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa118)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa218)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa318)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa01f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa11f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa21f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa31f)	/* VF on PF 3 */

	/** Gen 2 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa021)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa121)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa221)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa321)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa024)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa124)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa224)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa324)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa028)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa128)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa228)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa328)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa02f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa12f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa22f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa32f)	/* VF on PF 3 */

	/** Gen 3 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa031)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa131)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa231)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa331)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa034)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa134)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa234)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa334)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa038)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa138)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa238)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa338)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa03f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa13f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa23f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa33f)	/* VF on PF 3 */

	/** Gen 4 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa041)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa141)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa241)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa341)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa044)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa144)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa244)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa344)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa048)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa148)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa248)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0xa348)	/* VF on PF 3 */

	{ .vendor_id = 0, /* sentinel */ },
};

static int qdma_vf_write_mbx(struct rte_eth_dev *dev,
				struct qdma_mbox_data *send_msg)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct mbox_omsg *out_msg;
	struct mbox_fsr *mbx_stat;
	uint64_t bar_addr, reg_addr;
	uint16_t i;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	reg_addr = bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_FSR;

	mbx_stat = (struct mbox_fsr *)(reg_addr);

	i = MAILBOX_PROG_POLL_COUNT;
	while (mbx_stat->omsg_stat && i) {
		rte_delay_ms(MAILBOX_VF_MSG_DELAY);
		i--;
	}
	if (!i)
		return -1;
	out_msg = (struct mbox_omsg *)(bar_addr + QDMA_TRQ_EXT_VF +
							QDMA_MBOX_OMSG);

	qdma_write_reg((uint64_t)&out_msg->omsg[0],
			rte_cpu_to_le_32(send_msg->err << 24 |
					  send_msg->filler << 20 |
					  send_msg->debug_funid << 8 |
					  send_msg->opcode));
	for (i = 1; i < 32; i++)
		qdma_write_reg((uint64_t)&out_msg->omsg[i],
				rte_cpu_to_le_32(send_msg->data[i - 1]));

	qdma_write_reg((uint64_t)(bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_FCMD),
						QDMA_MBOX_CMD_SEND);

	return 0;
}

static int check_outmsg_status(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct mbox_fsr *mbx_stat;
	uint64_t bar_addr, reg_addr;
	uint16_t i;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	reg_addr = bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_FSR;

	mbx_stat = (struct mbox_fsr *)(reg_addr);

	i = MAILBOX_PROG_POLL_COUNT;
	while (mbx_stat->omsg_stat && i) {
		rte_delay_ms(MAILBOX_VF_MSG_DELAY);
		i--;
	}
	if (!i) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) did not receive ack from PF",
				qdma_dev->pf);
		return -1;
	}
	return 0;
}

static int qdma_vf_dev_start(struct rte_eth_dev *dev)
{
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	uint32_t qid;
	int err;

	PMD_DRV_LOG(INFO, "qdma_dev_start: Starting\n");

	/* prepare descriptor rings for operation */
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!txq->tx_deferred_start) {
			err = qdma_vf_dev_tx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}

	for (qid = 0; qid < dev->data->nb_rx_queues; qid++) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!rxq->rx_deferred_start) {
			err = qdma_vf_dev_rx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}
	return 0;
}

static int qdma_pf_bye(struct rte_eth_dev *dev)
{
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) received BYE message from PF",
							qdma_dev->pf);
#endif
	_rte_eth_dev_callback_process(dev, RTE_ETH_EVENT_VF_MBOX, NULL, NULL);
	return 0;
}

static void qdma_read_pf_msg(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint64_t bar_addr, reg_addr;
	struct qdma_mbox_data recv_msg;
	struct mbox_imsg *in_msg;
	int32_t retval = -1;
	uint32_t reg_val;
	int i;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	reg_addr = bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_IMSG;

	in_msg = (struct mbox_imsg *)(reg_addr);

	reg_val = qdma_read_reg((uint64_t)&in_msg->imsg[0]);

	recv_msg.opcode = reg_val & 0xff;
	recv_msg.debug_funid  = (reg_val >> 8) & 0xfff;
	recv_msg.filler = (reg_val >> 20) & 0xf;
	recv_msg.err    = (reg_val >> 24) & 0xff;

	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) received mbox message from PF %d",
					qdma_dev->pf, recv_msg.debug_funid);
	for (i = 1; i < 32; i++)
		recv_msg.data[i - 1] = qdma_read_reg((uint64_t)&
							in_msg->imsg[i]);

	switch (recv_msg.opcode) {
	case QDMA_MBOX_OPCD_BYE:
		retval = qdma_pf_bye(dev);
		break;
	}
	/* once the msg is handled ACK it */
	if (!retval)
		qdma_write_reg((bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_FCMD),
							QDMA_MBOX_CMD_RECV);
}

static void qdma_check_pf_msg(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint64_t bar_addr, reg_addr;
	struct mbox_fsr *mbx_stat;

	if (!qdma_dev)
		return;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	reg_addr = bar_addr + QDMA_TRQ_EXT_VF + QDMA_MBOX_FSR;
	mbx_stat = (struct mbox_fsr *)(reg_addr);

	if (mbx_stat->imsg_stat == 1)
		qdma_read_pf_msg(dev);
	rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_check_pf_msg, arg);
}

static int qdma_vf_dev_link_update(struct rte_eth_dev *dev,
					__rte_unused int wait_to_complete)
{
	dev->data->dev_link.link_status = ETH_LINK_UP;
	dev->data->dev_link.link_duplex = ETH_LINK_FULL_DUPLEX;
	dev->data->dev_link.link_speed = ETH_SPEED_NUM_10G;

	PMD_DRV_LOG(INFO, "Link update done\n");

	return 0;
}

static void qdma_vf_dev_infos_get(__rte_unused struct rte_eth_dev *dev,
					struct rte_eth_dev_info *dev_info)
{
	dev_info->max_rx_queues = QDMA_QUEUES_NUM_MAX;
	dev_info->max_tx_queues = QDMA_QUEUES_NUM_MAX;
	dev_info->min_rx_bufsize = QDMA_MIN_RXBUFF_SIZE;
	dev_info->max_rx_pktlen = DMA_BRAM_SIZE;
	dev_info->max_mac_addrs = 1;
}

static void qdma_vf_dev_stop(struct rte_eth_dev *dev)
{
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
#endif
	uint32_t qid;

	/* reset driver's internal queue structures to default values */
	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Stop H2C & C2H queues", qdma_dev->pf);
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++)
		qdma_vf_dev_tx_queue_stop(dev, qid);
	for (qid = 0; qid < dev->data->nb_rx_queues; qid++)
		qdma_vf_dev_rx_queue_stop(dev, qid);
}

static void qdma_vf_dev_close(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	struct qdma_mbox_data send_msg;
	int ret = 0;
	uint32_t qid;

	PMD_DRV_LOG(INFO, "Closing all queues\n");

	/* send MBOX Bye MSG for both TX and RX*/

	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.opcode = QDMA_MBOX_OPCD_BYE;
	send_msg.debug_funid = qdma_dev->pf;

	ret = qdma_vf_write_mbx(dev, &send_msg);
	if (ret != 0)
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) Failed sending QBYE MSG",
								qdma_dev->pf);
	ret = check_outmsg_status(dev);
	if (ret != 0)
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) did not receive ack from PF",
								qdma_dev->pf);

	/* iterate over rx queues */
	for (qid = 0; qid < dev->data->nb_rx_queues; ++qid) {
		rxq = dev->data->rx_queues[qid];
		if (rxq != NULL) {
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Remove C2H queue: %d",
							qdma_dev->pf, qid);

			if (rxq->sw_ring)
				rte_free(rxq->sw_ring);
//#ifdef QDMA_STREAM
			if (rxq->st_mode) { /** if ST-mode **/
				if (rxq->rx_cmpt_mz)
					rte_memzone_free(rxq->rx_cmpt_mz);
			}
//#endif
			if (rxq->rx_mz)
				rte_memzone_free(rxq->rx_mz);
			rte_free(rxq);
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) C2H queue %d removed",
							qdma_dev->pf, qid);
		}
	}
	/* iterate over tx queues */
	for (qid = 0; qid < dev->data->nb_tx_queues; ++qid) {
		txq = dev->data->tx_queues[qid];
		if (txq != NULL) {
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Remove H2C queue: %d",
							qdma_dev->pf, qid);

			if (txq->sw_ring)
				rte_free(txq->sw_ring);
			if (txq->tx_mz)
				rte_memzone_free(txq->tx_mz);
			rte_free(txq);
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) H2C queue %d removed",
							qdma_dev->pf, qid);
		}
	}
}

static int qdma_vf_dev_configure(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_data send_msg;
	int32_t ret = 0;
	uint32_t qid = 0;

	/** FMAP configuration **/
	qdma_dev->qsets_en = RTE_MAX(dev->data->nb_rx_queues,
					dev->data->nb_tx_queues);

	if (qdma_dev->queue_base + qdma_dev->qsets_en > qdma_dev->qsets_max) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Error: Number of Queues to be "
				"configured are greater than the queues "
				"supported by the hardware\n", qdma_dev->pf);
		qdma_dev->qsets_en = 0;
		return -1;
	}

	qdma_dev->q_info = rte_zmalloc("qinfo", sizeof(struct queue_info) *
						qdma_dev->qsets_en, 0);
	if (qdma_dev->q_info == NULL) {
		PMD_DRV_LOG(INFO, "VF-%d fail to allocate queue info memory\n",
								qdma_dev->pf);
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

	memset(&send_msg, 0, sizeof(send_msg));

	/* send hi msg to PF with queue-base and queue-max*/
	send_msg.opcode = QDMA_MBOX_OPCD_HI;
	send_msg.debug_funid = qdma_dev->pf;

	send_msg.data[0] = qdma_dev->queue_base;
	send_msg.data[1] = qdma_dev->qsets_en;

	ret = qdma_vf_write_mbx(dev, &send_msg);
	if (ret < 0) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Failed sending HI MSG",
								qdma_dev->pf);
		return ret;
	}
	ret = check_outmsg_status(dev);

	return ret;
}

int qdma_vf_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_mbox_data send_msg;
	uint32_t queue_base =  qdma_dev->queue_base;
	uint64_t phys_addr;
	int err;

	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.opcode = QDMA_MBOX_OPCD_QADD;
	send_msg.debug_funid = qdma_dev->pf;
	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
	qdma_reset_tx_queue(txq);

	phys_addr = (uint64_t)txq->tx_mz->phys_addr;
	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Phys-addr for H2C queue-id:%d: "
			"is Lo:0x%lx, Hi:0x%lx\n",
			qdma_dev->pf, qid,
			rte_cpu_to_le_32(phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(phys_addr >> 32));

	/* send MBOX msg QADD for H2C*/
	send_msg.data[0] = (qid + queue_base);
	send_msg.data[1] = txq->st_mode;
	send_msg.data[2] = 0;
	send_msg.data[3] = txq->ringszidx;
	send_msg.data[4] = 0;
	send_msg.data[5] = 0;
	send_msg.data[6] = 0;
	send_msg.data[7] = 0;
	send_msg.data[8] = rte_cpu_to_le_32(phys_addr & MASK_32BIT);
	send_msg.data[9] = rte_cpu_to_le_32(phys_addr >> 32);
	send_msg.data[15] = txq->en_bypass;
	send_msg.data[16] = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	err = qdma_vf_write_mbx(dev, &send_msg);
	if (err != 0) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Failed sending H2C QADD MSG "
				"for QID:%d", qdma_dev->pf, qid);
		return err;
	}
	err = check_outmsg_status(dev);
	if (err != 0)
		return err;

	qdma_start_tx_queue(txq);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	txq->status = RTE_ETH_QUEUE_STATE_STARTED;

	return 0;
}

int qdma_vf_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	struct qdma_mbox_data send_msg;
	uint32_t queue_base =  qdma_dev->queue_base;
	uint64_t phys_addr, cmpt_phys_addr;
	uint8_t en_pfetch, cmpt_desc_fmt;
	int err;

	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.opcode = QDMA_MBOX_OPCD_QADD;
	send_msg.debug_funid = qdma_dev->pf;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	qdma_reset_rx_queue(rxq);

	en_pfetch = (rxq->en_prefetch) ? 1 : 0;
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

	phys_addr = (uint64_t)rxq->rx_mz->phys_addr;
	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Phys-addr for C2H queue-id:%d: "
			"is Lo:0x%lx, Hi:0x%lx\n",
			qdma_dev->pf, qid,
			rte_cpu_to_le_32(phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(phys_addr >> 32));

	if (rxq->st_mode)
		cmpt_phys_addr = (uint64_t)rxq->rx_cmpt_mz->phys_addr;
	else
		cmpt_phys_addr = 0;

	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) C2H Completion queue Phys-addr "
			"for queue-id:%d: is Lo:0x%lx, Hi:0x%lx\n",
			qdma_dev->pf, qid,
			rte_cpu_to_le_32(cmpt_phys_addr & MASK_32BIT),
			rte_cpu_to_le_32(cmpt_phys_addr >> 32));
	err = qdma_start_rx_queue(rxq);
	if (err != 0)
		return err;

	/* Send Mbox Msg QADD for C2H */
	send_msg.data[0] = (qid + queue_base);
	send_msg.data[1] = rxq->st_mode;
	send_msg.data[2] = 1;
	send_msg.data[3] = rxq->ringszidx;
	send_msg.data[4] = rxq->buffszidx;
	send_msg.data[5] = rxq->threshidx;
	send_msg.data[6] = rxq->timeridx;
	send_msg.data[7] = rxq->triggermode;
	send_msg.data[8] = rte_cpu_to_le_32(phys_addr & MASK_32BIT);
	send_msg.data[9] = rte_cpu_to_le_32(phys_addr >> 32);
	send_msg.data[10] = rte_cpu_to_le_32(cmpt_phys_addr & MASK_32BIT);
	send_msg.data[11] = rte_cpu_to_le_32(cmpt_phys_addr >> 32);
	send_msg.data[12] = rxq->cmpt_ringszidx;
	send_msg.data[13] = en_pfetch;
	send_msg.data[14] = cmpt_desc_fmt;
	send_msg.data[15] = rxq->en_bypass;
	send_msg.data[16] = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);
	send_msg.data[17] = rxq->en_bypass_prefetch;
	send_msg.data[19] = rxq->dis_overflow_check;

	err = qdma_vf_write_mbx(dev, &send_msg);
	if (err != 0) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Failed sending C2H QADD MSG "
				"for QID:%d", qdma_dev->pf, qid);
		return err;
	}
	err = check_outmsg_status(dev);
	if (err != 0)
		return err;

	if (rxq->st_mode) {
		*rxq->rx_cidx = (0x09000000) | 0x0;
		*rxq->rx_pidx = (rxq->nb_rx_desc - 2);
	}

	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	rxq->status = RTE_ETH_QUEUE_STATE_STARTED;
	return 0;
}

int qdma_vf_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	struct qdma_mbox_data send_msg;
	uint32_t queue_base =  qdma_dev->queue_base;
	int i = 0, err = 0, cnt = 0;

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


	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.opcode = QDMA_MBOX_OPCD_QDEL;
	send_msg.debug_funid = qdma_dev->pf;
	send_msg.data[0] = (qid + queue_base);
	send_msg.data[1] = DMA_FROM_DEVICE;

	err = qdma_vf_write_mbx(dev, &send_msg);
	if (err != 0) {
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) Failed sending RX QDEL MSG "
				"for QID:%d", qdma_dev->pf, qid);
		return err;
	}

	err = check_outmsg_status(dev);
	if (err != 0) {
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) did not receive ack from PF "
			"for RX QDEL MSG for QID:%d", qdma_dev->pf, qid);
		return err;
	}

	if (rxq->st_mode) {  /** ST-mode **/
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id = %d, mbuf_avail_count = "
				"%d, mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS

		for (i = 0; i < rxq->nb_rx_desc - 1; i++) {
			rte_pktmbuf_free(rxq->sw_ring[i]);
			rxq->sw_ring[i] = NULL;
		}
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id = %d, mbuf_avail_count = "
				"%d, mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
	}

	qdma_reset_rx_queue(rxq);
	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;
	return 0;
}


int qdma_vf_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_mbox_data send_msg;
	uint32_t queue_base =  qdma_dev->queue_base;
	int err = 0;
	int i = 0, cnt = 0;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

	txq->status = RTE_ETH_QUEUE_STATE_STOPPED;
	/* Wait for TXQ to send out all packets. */
	while (txq->wb_status->cidx != txq->tx_tail) {
		usleep(10);
		if (cnt++ > 10000)
			break;
	}

	memset(&send_msg, 0, sizeof(send_msg));
	send_msg.opcode = QDMA_MBOX_OPCD_QDEL;
	send_msg.debug_funid = qdma_dev->pf;
	send_msg.data[0] = (qid + queue_base);
	send_msg.data[1] = DMA_TO_DEVICE;

	err = qdma_vf_write_mbx(dev, &send_msg);
	if (err != 0) {
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) Failed sending TX QDEL MSG "
				"for QID:%d", qdma_dev->pf, qid);
		return err;
	}

	err = check_outmsg_status(dev);
	if (err != 0) {
		RTE_LOG(ERR, PMD, "VF-%d(DEVFN) did not receive ack from PF "
			"for TX QDEL MSG for QID:%d", qdma_dev->pf, qid);
		return err;
	}

	qdma_reset_tx_queue(txq);

	/* Free mbufs if any pending in the ring */
	for (i = 0; i < txq->nb_tx_desc - 1; i++) {
		rte_pktmbuf_free(txq->sw_ring[i]);
		txq->sw_ring[i] = NULL;
	}

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;
	return 0;
}

static struct eth_dev_ops qdma_vf_eth_dev_ops = {
	.dev_configure        = qdma_vf_dev_configure,
	.dev_infos_get        = qdma_vf_dev_infos_get,
	.dev_start            = qdma_vf_dev_start,
	.dev_stop             = qdma_vf_dev_stop,
	.dev_close            = qdma_vf_dev_close,
	.link_update          = qdma_vf_dev_link_update,
	.rx_queue_setup       = qdma_dev_rx_queue_setup,
	.tx_queue_setup       = qdma_dev_tx_queue_setup,
	.rx_queue_release       = qdma_dev_rx_queue_release,
	.tx_queue_release       = qdma_dev_tx_queue_release,
	.rx_queue_start       = qdma_vf_dev_rx_queue_start,
	.rx_queue_stop        = qdma_vf_dev_rx_queue_stop,
	.tx_queue_start       = qdma_vf_dev_tx_queue_start,
	.tx_queue_stop        = qdma_vf_dev_tx_queue_stop,
};

/**
 * DPDK callback to register a PCI device.
 *
 * This function creates an Ethernet device for each port of a given
 * PCI device.
 *
 * @param[in] dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
static int eth_qdma_vf_dev_init(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *dma_priv;
	uint8_t *baseaddr;
	uint64_t bar_len;
	int i, idx;
	static bool once = true;
	struct rte_pci_device *pci_dev;

	/* sanity checks */
	if (dev == NULL)
		return -EINVAL;
	if (dev->data == NULL)
		return -EINVAL;
	if (dev->data->dev_private == NULL)
		return -EINVAL;

	pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	if (pci_dev == NULL)
		return -EINVAL;

	/* for secondary processes, we don't initialise any further as primary
	 * has already done this work.
	 */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
		dev->dev_ops = &qdma_vf_eth_dev_ops;
		return 0;
	}

	if (once) {
		RTE_LOG(INFO, PMD, "QDMA PMD VERSION: %s\n", QDMA_PMD_VERSION);
		once = false;
	}

	/* allocate space for a single Ethernet MAC address */
	dev->data->mac_addrs = rte_zmalloc("qdma_vf", ETHER_ADDR_LEN * 1, 0);
	if (dev->data->mac_addrs == NULL)
		return -ENOMEM;

	/* Copy some dummy Ethernet MAC address for XDMA device
	 * This will change in real NIC device...
	 */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		dev->data->mac_addrs[0].addr_bytes[i] = 0x15 + i;

	/* Init system & device */
	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	dma_priv->pf = PCI_DEVFN(pci_dev->addr.devid, pci_dev->addr.function);
	dma_priv->is_vf = 1;

	dma_priv->qsets_max = QDMA_QUEUES_NUM_MAX;
	dma_priv->queue_base = DEFAULT_QUEUE_BASE;
	dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_TIMER;

	/* Setting default Mode to TRIG_MODE_USER_TIMER_COUNT */
	dma_priv->trigger_mode = TRIG_MODE_USER_TIMER_COUNT;
	if (dma_priv->trigger_mode == TRIG_MODE_USER_TIMER_COUNT)
		dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER;

	/* !! FIXME default to enabled for everything.
	 *
	 * Currently the registers are not available for VFs, so
	 * setting them as enabled.
	 */
	dma_priv->st_mode_en = 1;
	dma_priv->mm_mode_en = 1;

	dma_priv->en_desc_prefetch = 0; //Keep prefetch default to 0
	dma_priv->cmpt_desc_len = DEFAULT_QDMA_CMPT_DESC_LEN;
	dma_priv->c2h_bypass_mode = C2H_BYPASS_MODE_NONE;
	dma_priv->h2c_bypass_mode = 0;

	dev->dev_ops = &qdma_vf_eth_dev_ops;
	dev->rx_pkt_burst = &qdma_recv_pkts;
	dev->tx_pkt_burst = &qdma_xmit_pkts;

	dma_priv->config_bar_idx = DEFAULT_VF_CONFIG_BAR;
	dma_priv->bypass_bar_idx = BAR_ID_INVALID;
	dma_priv->user_bar_idx = BAR_ID_INVALID;

	if (qdma_check_kvargs(dev->device->devargs, dma_priv)) {
		PMD_DRV_LOG(INFO, "devargs failed\n");
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	idx = qdma_identify_bars(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) QDMA device driver probe:",
								dma_priv->pf);

	idx = qdma_get_hw_version(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	baseaddr = (uint8_t *)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr;
	bar_len = pci_dev->mem_resource[dma_priv->config_bar_idx].len;

	dma_priv->bar_addr[dma_priv->config_bar_idx] = baseaddr;
	dma_priv->bar_len[dma_priv->config_bar_idx] = bar_len;

	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			     pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		bar_len = pci_dev->mem_resource[dma_priv->user_bar_idx].len;
		dma_priv->bar_addr[dma_priv->user_bar_idx] = baseaddr;
		dma_priv->bar_len[dma_priv->user_bar_idx] = bar_len;
	}
	rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_check_pf_msg, (void *)dev);
	return 0;
}

/**
 * DPDK callback to deregister PCI device.
 *
 * @param[in] dev
 *   Pointer to Ethernet device structure.
 *
 * @return
 *   0 on success, negative errno value on failure.
 */
static int eth_qdma_vf_dev_uninit(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;
	/*Cancel pending polls*/
	rte_eal_alarm_cancel(qdma_check_pf_msg, (void *)dev);
	dev->dev_ops = NULL;
	dev->rx_pkt_burst = NULL;
	dev->tx_pkt_burst = NULL;
	dev->data->nb_rx_queues = 0;
	dev->data->nb_tx_queues = 0;

	if (dev->data->mac_addrs != NULL) {
		rte_free(dev->data->mac_addrs);
		dev->data->mac_addrs = NULL;
	}

	if (qdma_dev->q_info != NULL) {
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
	}

	return 0;
}

static int eth_qdma_vf_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
					struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev,
						sizeof(struct qdma_pci_dev),
						eth_qdma_vf_dev_init);
}

/* Detach a ethdev interface */
static int eth_qdma_vf_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, eth_qdma_vf_dev_uninit);
}

static struct rte_pci_driver rte_qdma_vf_pmd = {
	.id_table = qdma_vf_pci_id_tbl,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
	.probe = eth_qdma_vf_pci_probe,
	.remove = eth_qdma_vf_pci_remove,
};

RTE_PMD_REGISTER_PCI(net_qdma_vf, rte_qdma_vf_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_qdma_vf, qdma_vf_pci_id_tbl);
