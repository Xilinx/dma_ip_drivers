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
#include "qdma_cpm_access.h"
#include "qdma_reg_dump.h"
#define xdebug_info(x, args...) rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1,\
					## args)
#define xdebug_error(x, args...) rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1,\
					## args)

static void print_header(const char *str)
{
	xdebug_info(adap, "\n\n%s\n\n", str);
}

static int qdma_h2c_struct_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_tx_queue *tx_q =
		(struct qdma_tx_queue *)dev->data->tx_queues[queue];

	if (queue >= dev->data->nb_tx_queues) {
		xdebug_info(dev, "TX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

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

	return 0;
}

static int qdma_c2h_struct_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rx_q =
		(struct qdma_rx_queue *)dev->data->rx_queues[queue];

	if (queue >= dev->data->nb_rx_queues) {
		xdebug_info(dev, "RX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

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

		if (!((qdma_dev->device_type == QDMA_DEVICE_VERSAL) &&
		(qdma_dev->versal_ip_type == QDMA_VERSAL_HARD_IP)))
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

	return 0;
}

static int qdma_csr_dump(uint8_t port_id)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	char *buf = NULL;
	uint32_t buflen;
	int ret;

	if (rte_eth_dev_count_avail() < 1)
		return -1;
	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	buflen = qdma_reg_dump_buf_len();

	if (qdma_dev->is_vf) {
		xdebug_info(qdma_dev, "QDMA CSR dump not applicable for VF device\n");
		return 0;
	}

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_CSR_DUMP", buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error(qdma_dev, "Unable to allocate memory for csr dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}
	xdebug_info(qdma_dev, "FPGA Config Registers for port_id: %d\n--------\n",
		port_id);
	xdebug_info(qdma_dev, " Offset       Name    "
			"                                    Value(Hex) Value(Dec)\n");

	if ((qdma_dev->device_type == QDMA_DEVICE_VERSAL) &&
			(qdma_dev->versal_ip_type == QDMA_VERSAL_HARD_IP))
		ret = qdma_cpm_dump_config_regs(dev,
				qdma_dev->is_vf, buf, buflen);
	else
		ret = qdma_dump_config_regs(dev,
				qdma_dev->is_vf, buf, buflen);
	if (ret < 0) {
		xdebug_error(qdma_dev, "Insufficient space to dump Config Bar register values\n");
		rte_free(buf);
		return qdma_get_error_code(ret);
	}
	xdebug_info(qdma_dev, "%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_device_dump(uint8_t port_id)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error(qdma_dev, "Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	xdebug_info(qdma_dev, "\n*** QDMA Device struct for port_id: %d ***\n\n",
		port_id);

	xdebug_info(qdma_dev, "\t\t config BAR index         :%x\n",
			qdma_dev->config_bar_idx);
	xdebug_info(qdma_dev, "\t\t user BAR index           :%x\n",
			qdma_dev->user_bar_idx);
	xdebug_info(qdma_dev, "\t\t bypass BAR index         :%x\n",
			qdma_dev->bypass_bar_idx);
	xdebug_info(qdma_dev, "\t\t qsets enable             :%x\n",
			qdma_dev->qsets_en);
	xdebug_info(qdma_dev, "\t\t queue base               :%x\n",
			qdma_dev->queue_base);
	xdebug_info(qdma_dev, "\t\t pf                       :%x\n",
			qdma_dev->func_id);
	xdebug_info(qdma_dev, "\t\t cmpt desc length         :%x\n",
			qdma_dev->cmpt_desc_len);
	xdebug_info(qdma_dev, "\t\t c2h bypass mode          :%x\n",
			qdma_dev->c2h_bypass_mode);
	xdebug_info(qdma_dev, "\t\t h2c bypass mode          :%x\n",
			qdma_dev->h2c_bypass_mode);
	xdebug_info(qdma_dev, "\t\t trigger mode             :%x\n",
			qdma_dev->trigger_mode);
	xdebug_info(qdma_dev, "\t\t timer count              :%x\n",
			qdma_dev->timer_count);
	xdebug_info(qdma_dev, "\t\t is vf                    :%x\n",
			qdma_dev->is_vf);
	xdebug_info(qdma_dev, "\t\t is master                :%x\n",
			qdma_dev->is_master);
	xdebug_info(qdma_dev, "\t\t enable desc prefetch     :%x\n",
			qdma_dev->en_desc_prefetch);
	xdebug_info(qdma_dev, "\t\t versal ip type               :%x\n",
			qdma_dev->versal_ip_type);
	xdebug_info(qdma_dev, "\t\t vivado release           :%x\n",
			qdma_dev->vivado_rel);
	xdebug_info(qdma_dev, "\t\t rtl version              :%x\n",
			qdma_dev->rtl_version);
	xdebug_info(qdma_dev, "\t\t is queue conigured       :%x\n",
		qdma_dev->init_q_range);

	xdebug_info(qdma_dev, "\n\t ***** Device Capabilities *****\n");
	xdebug_info(qdma_dev, "\t\t number of PFs            :%x\n",
			qdma_dev->dev_cap.num_pfs);
	xdebug_info(qdma_dev, "\t\t number of Queues         :%x\n",
			qdma_dev->dev_cap.num_qs);
	xdebug_info(qdma_dev, "\t\t FLR present              :%x\n",
			qdma_dev->dev_cap.flr_present);
	xdebug_info(qdma_dev, "\t\t ST mode enable           :%x\n",
			qdma_dev->dev_cap.st_en);
	xdebug_info(qdma_dev, "\t\t MM mode enable           :%x\n",
			qdma_dev->dev_cap.mm_en);
	xdebug_info(qdma_dev, "\t\t MM with compt enable     :%x\n",
			qdma_dev->dev_cap.mm_cmpt_en);
	xdebug_info(qdma_dev, "\t\t Mailbox enable           :%x\n",
			qdma_dev->dev_cap.mailbox_en);
	xdebug_info(qdma_dev, "\t\t Num of MM channels       :%x\n",
			qdma_dev->dev_cap.mm_channel_max);

	return 0;
}

static int qdma_c2h_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t qid = qdma_dev->queue_base + queue;
	uint8_t st_mode = 0;
	char *buf = NULL;
	uint32_t buflen;
	int ret = 0;
	int dir = 1;// assigning direction to c2h
	char pfetch_valid = 0;
	char cmpt_valid = 0;

	if (queue >= dev->data->nb_rx_queues) {
		xdebug_info(qdma_dev, "RX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	xdebug_info(qdma_dev,
		"\n ***** C2H Queue Contexts on port_id: %d for queue_id: %d *****\n",
		port_id, qid);

	st_mode = qdma_dev->q_info[qid].queue_mode;
	if (st_mode && dir)
		pfetch_valid = 1;
	if ((st_mode && dir) ||
		(!st_mode && qdma_dev->dev_cap.mm_cmpt_en))
		cmpt_valid = 1;

	buflen = qdma_context_buf_len(pfetch_valid, cmpt_valid);

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_C2H_CONTEXT_DUMP",
				buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error(qdma_dev, "Unable to allocate memory for c2h context dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	if ((qdma_dev->device_type == QDMA_DEVICE_VERSAL) &&
			(qdma_dev->versal_ip_type == QDMA_VERSAL_HARD_IP))
		ret = qdma_cpm_dump_queue_context(dev, qid,
					st_mode, dir, buf, buflen);
	else
		ret = qdma_dump_queue_context(dev, qid,
					st_mode, dir, buf, buflen);
	if (ret < 0) {
		xdebug_error(qdma_dev,
			"Insufficient space to dump c2h context values\n");
		rte_free(buf);
		return qdma_get_error_code(ret);
	}
	xdebug_info(qdma_dev, "%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_h2c_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t qid = qdma_dev->queue_base + queue;
	uint8_t st_mode = 0;
	char *buf = NULL;
	uint32_t buflen;
	int ret = 0;
	int dir = 0;// assigning direction to h2c
	char pfetch_valid = 0;
	char cmpt_valid = 0;

	if (queue >= dev->data->nb_tx_queues) {
		xdebug_info(qdma_dev, "TX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	xdebug_info(qdma_dev,
		"\n ***** H2C Queue Contexts on port_id: %d for queue_id: %d *****\n",
		port_id, qid);

	st_mode = qdma_dev->q_info[qid].queue_mode;
	if (!st_mode && qdma_dev->dev_cap.mm_cmpt_en)
		cmpt_valid = 1;

	buflen = qdma_context_buf_len(pfetch_valid, cmpt_valid);

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_H2C_CONTEXT_DUMP",
			buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error(qdma_dev, "Unable to allocate memory for h2c context dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	if ((qdma_dev->device_type == QDMA_DEVICE_VERSAL) &&
			(qdma_dev->versal_ip_type == QDMA_VERSAL_HARD_IP))
		ret = qdma_cpm_dump_queue_context(dev, qid,
					st_mode, dir, buf, buflen);
	else
		ret = qdma_dump_queue_context(dev, qid,
					st_mode, dir, buf, buflen);
	if (ret < 0) {
		xdebug_error(qdma_dev,
				"Insufficient space to dump h2c context values\n");
		rte_free(buf);
		return qdma_get_error_code(ret);
	}
	xdebug_info(qdma_dev, "%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_queue_desc_dump(uint8_t port_id,
		struct rte_pmd_qdma_xdebug_desc_param *param)
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	int x;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	uint8_t *rx_ring_bypass = NULL;
	uint8_t *tx_ring_bypass = NULL;
	char str[50];

	switch (param->type) {
	case RTE_PMD_QDMA_XDEBUG_DESC_C2H:

		if (param->queue >= dev->data->nb_rx_queues) {
			xdebug_info(dev, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq == NULL) {
			xdebug_info(dev,
				"Caught NULL pointer for queue_id: %d\n",
				param->queue);
			return -1;
		}

		if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
			xdebug_info(dev, "Queue_id %d is not yet started\n",
				param->queue);
			return -1;
		}

		if (param->start < 0 || param->start > rxq->nb_rx_desc)
			param->start = 0;
		if (param->end <= param->start ||
				param->end > rxq->nb_rx_desc)
			param->end = rxq->nb_rx_desc;

		if ((rxq->en_bypass) && (rxq->bypass_desc_sz != 0)) {
			rx_ring_bypass = (uint8_t *)rxq->rx_ring;

			xdebug_info(dev, "\n===== C2H bypass descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint8_t *rx_bypass =
						&rx_ring_bypass[x];
				sprintf(str, "\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
					(const void *)rx_bypass,
					rxq->bypass_desc_sz);
			}
		} else {
			if (rxq->st_mode) {
				struct qdma_ul_st_c2h_desc *rx_ring_st =
				(struct qdma_ul_st_c2h_desc *)rxq->rx_ring;

				xdebug_info(dev, "\n===== C2H ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					struct qdma_ul_st_c2h_desc *rx_st =
						&rx_ring_st[x];
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
					(const void *)rx_st,
					sizeof(struct qdma_ul_st_c2h_desc));
				}
			} else {
				struct qdma_ul_mm_desc *rx_ring_mm =
					(struct qdma_ul_mm_desc *)rxq->rx_ring;
				xdebug_info(dev, "\n====== C2H ring descriptors======\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&rx_ring_mm[x],
						sizeof(struct qdma_ul_mm_desc));
				}
			}
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_DESC_CMPT:

		if (param->queue >= dev->data->nb_rx_queues) {
			xdebug_info(dev, "queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq) {
			if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
				xdebug_info(dev, "Queue_id %d is not yet started\n",
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
				xdebug_info(dev, "Queue_id %d is not initialized "
					"in Stream mode\n", param->queue);
				return -1;
			}

			xdebug_info(dev, "\n===== CMPT ring descriptors=====\n");
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
			xdebug_info(dev, "queue_id=%d not configured",
				param->queue);
			return -1;
		}

		txq = (struct qdma_tx_queue *)
			dev->data->tx_queues[param->queue];

		if (txq == NULL) {
			xdebug_info(dev,
				"Caught NULL pointer for queue_id: %d\n",
				param->queue);
			return -1;
		}

		if (txq->status != RTE_ETH_QUEUE_STATE_STARTED) {
			xdebug_info(dev, "Queue_id %d is not yet started\n",
				param->queue);
			return -1;
		}

		if (param->start < 0 || param->start > txq->nb_tx_desc)
			param->start = 0;
		if (param->end <= param->start ||
				param->end > txq->nb_tx_desc)
			param->end = txq->nb_tx_desc;

		if ((txq->en_bypass) && (txq->bypass_desc_sz != 0)) {
			tx_ring_bypass = (uint8_t *)txq->tx_ring;

			xdebug_info(dev, "\n====== H2C bypass descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint8_t *tx_bypass =
					&tx_ring_bypass[x];
				sprintf(str, "\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
					(const void *)tx_bypass,
					txq->bypass_desc_sz);
			}
		} else {
			if (txq->st_mode) {
				struct qdma_ul_st_h2c_desc *qdma_h2c_ring =
				(struct qdma_ul_st_h2c_desc *)txq->tx_ring;
				xdebug_info(dev, "\n====== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
					(const void *)&qdma_h2c_ring[x],
					sizeof(struct qdma_ul_st_h2c_desc));
				}
			} else {
				struct qdma_ul_mm_desc *tx_ring_mm =
					(struct qdma_ul_mm_desc *)txq->tx_ring;
				xdebug_info(dev, "\n===== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					sprintf(str, "\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&tx_ring_mm[x],
						sizeof(struct qdma_ul_mm_desc));
				}
			}
		}
		break;
	default:
		xdebug_info(dev, "Invalid ring selected\n");
		break;
	}
	return 0;
}

int rte_pmd_qdma_dbg_regdump(uint8_t port_id)
{
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error(adap, "Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	err = qdma_csr_dump(port_id);
	if (err) {
		xdebug_error(adap, "Error dumping Global registers\n");
		return err;
	}
	return 0;
}

int rte_pmd_qdma_dbg_qdevice(uint8_t port_id)
{
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error(adap, "Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	err = qdma_device_dump(port_id);
	if (err) {
		xdebug_error(adap, "Error dumping QDMA device\n");
		return err;
	}
	return 0;
}

int rte_pmd_qdma_dbg_qinfo(uint8_t port_id, uint16_t queue)
{
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error(adap, "Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	err = qdma_c2h_context_dump(port_id, queue);
	if (err) {
		xdebug_error(adap, "Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	err = qdma_c2h_struct_dump(port_id, queue);
	if (err) {
		xdebug_error(adap, "Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	err = qdma_h2c_context_dump(port_id, queue);
	if (err) {
		xdebug_error(adap, "Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	err = qdma_h2c_struct_dump(port_id, queue);
	if (err) {
		xdebug_error(adap, "Error dumping %d: %d\n",
				queue, err);
		return err;
	}
	return 0;
}

int rte_pmd_qdma_dbg_qdesc(uint8_t port_id, uint16_t queue, int start,
		int end, enum rte_pmd_qdma_xdebug_desc_type type)
{
	struct rte_pmd_qdma_xdebug_desc_param param;
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error(adap, "Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	param.queue = queue;
	param.start = start;
	param.end = end;
	param.type = type;

	err = qdma_queue_desc_dump(port_id, &param);
	if (err) {
		xdebug_error(adap, "Error dumping %d: %d\n",
			queue, err);
		return err;
	}
	return 0;
}
