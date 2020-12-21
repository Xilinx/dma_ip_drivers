/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2020 Xilinx, Inc. All rights reserved.
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
#include "qdma_access_common.h"
#include "qdma_reg_dump.h"
#include "qdma_mbox_protocol.h"
#include "qdma_mbox.h"
#define xdebug_info(args...) rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1,\
					## args)
#define xdebug_error(args...) rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1,\
					## args)

struct xdebug_desc_param {
	uint16_t queue;
	int start;
	int end;
	enum rte_pmd_qdma_xdebug_desc_type type;
};

const char *qdma_desc_eng_mode_info[QDMA_DESC_ENG_MODE_MAX] = {
	"Internal and Bypass mode",
	"Bypass only mode",
	"Internal only mode"
};

static void print_header(const char *str)
{
	xdebug_info("\n\n%s\n\n", str);
}

static int qdma_h2c_struct_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_tx_queue *tx_q;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	tx_q = (struct qdma_tx_queue *)dev->data->tx_queues[queue];

	if (queue >= dev->data->nb_tx_queues) {
		xdebug_info("TX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	if (tx_q) {
		print_header("***********TX Queue struct************");
		xdebug_info("\t\t wb_pidx             :%x\n",
				tx_q->wb_status->pidx);
		xdebug_info("\t\t wb_cidx             :%x\n",
				tx_q->wb_status->cidx);
		xdebug_info("\t\t h2c_pidx            :%x\n",
				tx_q->q_pidx_info.pidx);
		xdebug_info("\t\t tx_fl_tail          :%x\n",
				tx_q->tx_fl_tail);
		xdebug_info("\t\t tx_desc_pend        :%x\n",
				tx_q->tx_desc_pend);
		xdebug_info("\t\t nb_tx_desc          :%x\n",
				tx_q->nb_tx_desc);
		xdebug_info("\t\t st_mode             :%x\n",
				tx_q->st_mode);
		xdebug_info("\t\t tx_deferred_start   :%x\n",
				tx_q->tx_deferred_start);
		xdebug_info("\t\t en_bypass           :%x\n",
				tx_q->en_bypass);
		xdebug_info("\t\t bypass_desc_sz      :%x\n",
				tx_q->bypass_desc_sz);
		xdebug_info("\t\t func_id             :%x\n",
				tx_q->func_id);
		xdebug_info("\t\t port_id             :%x\n",
				tx_q->port_id);
		xdebug_info("\t\t ringszidx           :%x\n",
				tx_q->ringszidx);
		xdebug_info("\t\t ep_addr             :%x\n",
				tx_q->ep_addr);
	}

	return 0;
}

static int qdma_c2h_struct_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_rx_queue *rx_q;
	enum qdma_ip_type ip_type;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;
	rx_q = (struct qdma_rx_queue *)dev->data->rx_queues[queue];

	if (queue >= dev->data->nb_rx_queues) {
		xdebug_info("RX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	if (rx_q) {
		print_header(" ***********RX Queue struct********** ");
		xdebug_info("\t\t wb_pidx             :%x\n",
				rx_q->wb_status->pidx);
		xdebug_info("\t\t wb_cidx             :%x\n",
				rx_q->wb_status->cidx);
		xdebug_info("\t\t rx_tail (ST)        :%x\n",
				rx_q->rx_tail);
		xdebug_info("\t\t c2h_pidx            :%x\n",
				rx_q->q_pidx_info.pidx);
		xdebug_info("\t\t rx_cmpt_cidx        :%x\n",
				rx_q->cmpt_cidx_info.wrb_cidx);
		xdebug_info("\t\t cmpt_desc_len       :%x\n",
				rx_q->cmpt_desc_len);
		xdebug_info("\t\t rx_buff_size        :%x\n",
				rx_q->rx_buff_size);
		xdebug_info("\t\t nb_rx_desc          :%x\n",
				rx_q->nb_rx_desc);
		xdebug_info("\t\t nb_rx_cmpt_desc     :%x\n",
				rx_q->nb_rx_cmpt_desc);
		xdebug_info("\t\t ep_addr             :%x\n",
				rx_q->ep_addr);
		xdebug_info("\t\t st_mode             :%x\n",
				rx_q->st_mode);
		xdebug_info("\t\t rx_deferred_start   :%x\n",
				rx_q->rx_deferred_start);
		xdebug_info("\t\t en_prefetch         :%x\n",
				rx_q->en_prefetch);
		xdebug_info("\t\t en_bypass           :%x\n",
				rx_q->en_bypass);
		xdebug_info("\t\t dump_immediate_data :%x\n",
				rx_q->dump_immediate_data);
		xdebug_info("\t\t en_bypass_prefetch  :%x\n",
				rx_q->en_bypass_prefetch);

		if (!(ip_type == QDMA_VERSAL_HARD_IP))
			xdebug_info("\t\t dis_overflow_check  :%x\n",
				rx_q->dis_overflow_check);

		xdebug_info("\t\t bypass_desc_sz      :%x\n",
				rx_q->bypass_desc_sz);
		xdebug_info("\t\t ringszidx           :%x\n",
				rx_q->ringszidx);
		xdebug_info("\t\t cmpt_ringszidx      :%x\n",
				rx_q->cmpt_ringszidx);
		xdebug_info("\t\t buffszidx           :%x\n",
				rx_q->buffszidx);
		xdebug_info("\t\t threshidx           :%x\n",
				rx_q->threshidx);
		xdebug_info("\t\t timeridx            :%x\n",
				rx_q->timeridx);
		xdebug_info("\t\t triggermode         :%x\n",
				rx_q->triggermode);
	}

	return 0;
}

static int qdma_config_read_reg_list(struct rte_eth_dev *dev,
			uint16_t group_num,
			uint16_t *num_regs, struct qdma_reg_data *reg_list)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	int rv;

	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_reg_read(qdma_dev->func_id, group_num, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		if (rv != -ENODEV)
			xdebug_error("reg read mbox failed with error = %d\n",
				rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_reg_list_get(m->raw_data, num_regs, reg_list);
	if (rv < 0) {
		xdebug_error("qdma_mbox_vf_reg_list_get failed with error = %d\n",
			rv);
		goto err_out;
	}

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_config_reg_dump(uint8_t port_id)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	enum qdma_ip_type ip_type;
	char *buf = NULL;
	int buflen;
	int ret;
	struct qdma_reg_data *reg_list;
	uint16_t num_regs = 0, group_num = 0;
	int len = 0, rcv_len = 0, reg_len = 0;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;

	if (qdma_dev->is_vf) {
		reg_len = (QDMA_MAX_REGISTER_DUMP *
						sizeof(struct qdma_reg_data));
		reg_list = (struct qdma_reg_data *)rte_zmalloc("QDMA_DUMP_REG_VF",
				reg_len, RTE_CACHE_LINE_SIZE);
		if (!reg_list) {
			xdebug_error("Unable to allocate memory for VF dump for reglist "
					"size %d\n", reg_len);
			return -ENOMEM;
		}

		ret = qdma_acc_reg_dump_buf_len(dev, ip_type, &buflen);
		if (ret < 0) {
			xdebug_error("Failed to get register dump buffer length\n");
			return ret;
		}
		/*allocate memory for register dump*/
		buf = (char *)rte_zmalloc("QDMA_DUMP_BUF_VF", buflen,
				RTE_CACHE_LINE_SIZE);
		if (!buf) {
			xdebug_error("Unable to allocate memory for reg dump "
					"size %d\n", buflen);
			rte_free(reg_list);
			return -ENOMEM;
		}
		xdebug_info("FPGA Config Registers for port_id: %d\n--------\n",
			port_id);
		xdebug_info(" Offset       Name    "
				"                                    Value(Hex) Value(Dec)\n");

		for (group_num = 0; group_num < QDMA_REG_READ_GROUP_3;
				group_num++) {
			/** Reset the reg_list  with 0's */
			memset(reg_list, 0, (QDMA_MAX_REGISTER_DUMP *
					sizeof(struct qdma_reg_data)));
			ret = qdma_config_read_reg_list(dev,
						group_num, &num_regs, reg_list);
			if (ret < 0) {
				xdebug_error("Failed to read config registers "
					"size %d, err = %d\n", buflen, ret);
				rte_free(reg_list);
				rte_free(buf);
				return ret;
			}

			rcv_len = qdma_acc_dump_config_reg_list(dev,
				ip_type, num_regs,
				reg_list, buf + len, buflen - len);
			if (len < 0) {
				xdebug_error("Failed to dump config regs "
					"size %d, err = %d\n", buflen, ret);
				rte_free(reg_list);
				rte_free(buf);
				return ret;
			}
			len += rcv_len;
		}
		if (ret < 0) {
			xdebug_error("Insufficient space to dump Config Bar register values\n");
			rte_free(reg_list);
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
		xdebug_info("%s\n", buf);
		rte_free(reg_list);
		rte_free(buf);
	} else {
		ret = qdma_acc_reg_dump_buf_len(dev,
			ip_type, &buflen);
		if (ret < 0) {
			xdebug_error("Failed to get register dump buffer length\n");
			return ret;
		}

		/*allocate memory for register dump*/
		buf = (char *)rte_zmalloc("QDMA_REG_DUMP", buflen,
					RTE_CACHE_LINE_SIZE);
		if (!buf) {
			xdebug_error("Unable to allocate memory for reg dump "
					"size %d\n", buflen);
			return -ENOMEM;
		}
		xdebug_info("FPGA Config Registers for port_id: %d\n--------\n",
			port_id);
		xdebug_info(" Offset       Name    "
				"                                    Value(Hex) Value(Dec)\n");

		ret = qdma_acc_dump_config_regs(dev, qdma_dev->is_vf,
			ip_type, buf, buflen);
		if (ret < 0) {
			xdebug_error("Insufficient space to dump Config Bar register values\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
		xdebug_info("%s\n", buf);
		rte_free(buf);
	}

	return 0;
}

static int qdma_device_dump(uint8_t port_id)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;

	xdebug_info("\n*** QDMA Device struct for port_id: %d ***\n\n",
		port_id);

	xdebug_info("\t\t config BAR index         :%x\n",
			qdma_dev->config_bar_idx);
	xdebug_info("\t\t AXI Master Lite BAR index           :%x\n",
			qdma_dev->user_bar_idx);
	xdebug_info("\t\t AXI Bridge Master BAR index         :%x\n",
			qdma_dev->bypass_bar_idx);
	xdebug_info("\t\t qsets enable             :%x\n",
			qdma_dev->qsets_en);
	xdebug_info("\t\t queue base               :%x\n",
			qdma_dev->queue_base);
	xdebug_info("\t\t pf                       :%x\n",
			qdma_dev->func_id);
	xdebug_info("\t\t cmpt desc length         :%x\n",
			qdma_dev->cmpt_desc_len);
	xdebug_info("\t\t c2h bypass mode          :%x\n",
			qdma_dev->c2h_bypass_mode);
	xdebug_info("\t\t h2c bypass mode          :%x\n",
			qdma_dev->h2c_bypass_mode);
	xdebug_info("\t\t trigger mode             :%x\n",
			qdma_dev->trigger_mode);
	xdebug_info("\t\t timer count              :%x\n",
			qdma_dev->timer_count);
	xdebug_info("\t\t is vf                    :%x\n",
			qdma_dev->is_vf);
	xdebug_info("\t\t is master                :%x\n",
			qdma_dev->is_master);
	xdebug_info("\t\t enable desc prefetch     :%x\n",
			qdma_dev->en_desc_prefetch);
	xdebug_info("\t\t ip type                  :%x\n",
			qdma_dev->ip_type);
	xdebug_info("\t\t vivado release           :%x\n",
			qdma_dev->vivado_rel);
	xdebug_info("\t\t rtl version              :%x\n",
			qdma_dev->rtl_version);
	xdebug_info("\t\t is queue conigured       :%x\n",
		qdma_dev->init_q_range);

	xdebug_info("\n\t ***** Device Capabilities *****\n");
	xdebug_info("\t\t number of PFs            :%x\n",
			qdma_dev->dev_cap.num_pfs);
	xdebug_info("\t\t number of Queues         :%x\n",
			qdma_dev->dev_cap.num_qs);
	xdebug_info("\t\t FLR present              :%x\n",
			qdma_dev->dev_cap.flr_present);
	xdebug_info("\t\t ST mode enable           :%x\n",
			qdma_dev->dev_cap.st_en);
	xdebug_info("\t\t MM mode enable           :%x\n",
			qdma_dev->dev_cap.mm_en);
	xdebug_info("\t\t MM with compt enable     :%x\n",
			qdma_dev->dev_cap.mm_cmpt_en);
	xdebug_info("\t\t Mailbox enable           :%x\n",
			qdma_dev->dev_cap.mailbox_en);
	xdebug_info("\t\t Num of MM channels       :%x\n",
			qdma_dev->dev_cap.mm_channel_max);
	xdebug_info("\t\t Descriptor engine mode   :%s\n",
		qdma_desc_eng_mode_info[qdma_dev->dev_cap.desc_eng_mode]);
	xdebug_info("\t\t Debug mode enable        :%x\n",
			qdma_dev->dev_cap.debug_mode);

	return 0;
}

static int qdma_descq_context_read_vf(struct rte_eth_dev *dev,
	unsigned int qid_hw, bool st_mode,
	enum qdma_dev_q_type q_type,
	struct qdma_descq_context *context)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	enum mbox_cmpt_ctxt_type cmpt_ctxt_type = QDMA_MBOX_CMPT_CTXT_NONE;
	int rv;

	if (!m)
		return -ENOMEM;

	if (!st_mode) {
		if (q_type == QDMA_DEV_Q_TYPE_CMPT)
			cmpt_ctxt_type = QDMA_MBOX_CMPT_CTXT_ONLY;
		else
			cmpt_ctxt_type = QDMA_MBOX_CMPT_CTXT_NONE;
	} else {
		if (q_type == QDMA_DEV_Q_TYPE_C2H)
			cmpt_ctxt_type = QDMA_MBOX_CMPT_WITH_ST;
	}

	qdma_mbox_compose_vf_qctxt_read(qdma_dev->func_id,
		qid_hw, st_mode, q_type, cmpt_ctxt_type, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		xdebug_error("%x, qid_hw 0x%x, mbox failed for vf q context %d.\n",
			qdma_dev->func_id, qid_hw, rv);
		goto free_msg;
	}

	rv = qdma_mbox_vf_context_get(m->raw_data, context);
	if (rv < 0) {
		xdebug_error("%x, failed to get vf queue context info %d.\n",
				qdma_dev->func_id, rv);
		goto free_msg;
	}

free_msg:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_c2h_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_descq_context queue_context;
	enum qdma_dev_q_type q_type;
	enum qdma_ip_type ip_type;
	uint16_t qid;
	uint8_t st_mode;
	char *buf = NULL;
	uint32_t buflen = 0;
	int ret = 0;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	qid = qdma_dev->queue_base + queue;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;
	st_mode = qdma_dev->q_info[qid].queue_mode;
	q_type = QDMA_DEV_Q_TYPE_C2H;

	if (queue >= dev->data->nb_rx_queues) {
		xdebug_info("RX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	xdebug_info(
		"\n ***** C2H Queue Contexts on port_id: %d for q_id: %d *****\n",
		port_id, qid);

	ret = qdma_acc_context_buf_len(dev, ip_type, st_mode,
			q_type, &buflen);
	if (ret < 0) {
		xdebug_error("Failed to get context buffer length,\n");
		return ret;
	}

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_C2H_CONTEXT_DUMP",
				buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error("Unable to allocate memory for c2h context dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	if (qdma_dev->is_vf) {
		ret = qdma_descq_context_read_vf(dev, qid,
				st_mode, q_type, &queue_context);
		if (ret < 0) {
			xdebug_error("Failed to read c2h queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}

		ret = qdma_acc_dump_queue_context(dev, ip_type,
			st_mode, q_type, &queue_context, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to dump c2h queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	} else {
		ret = qdma_acc_read_dump_queue_context(dev, ip_type,
			qid, st_mode, q_type, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to read and dump c2h queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	}

	xdebug_info("%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_h2c_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_descq_context queue_context;
	enum qdma_dev_q_type q_type;
	enum qdma_ip_type ip_type;
	uint32_t buflen = 0;
	uint16_t qid;
	uint8_t st_mode;
	char *buf = NULL;
	int ret = 0;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	qid = qdma_dev->queue_base + queue;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;
	st_mode = qdma_dev->q_info[qid].queue_mode;
	q_type = QDMA_DEV_Q_TYPE_H2C;

	if (queue >= dev->data->nb_tx_queues) {
		xdebug_info("TX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	xdebug_info(
		"\n ***** H2C Queue Contexts on port_id: %d for q_id: %d *****\n",
		port_id, qid);

	ret = qdma_acc_context_buf_len(dev, ip_type, st_mode,
			q_type, &buflen);
	if (ret < 0) {
		xdebug_error("Failed to get context buffer length,\n");
		return ret;
	}

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_H2C_CONTEXT_DUMP",
			buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error("Unable to allocate memory for h2c context dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	if (qdma_dev->is_vf) {
		ret = qdma_descq_context_read_vf(dev, qid,
				st_mode, q_type, &queue_context);
		if (ret < 0) {
			xdebug_error("Failed to read h2c queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}

		ret = qdma_acc_dump_queue_context(dev, ip_type,
			st_mode, q_type, &queue_context, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to dump h2c queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	} else {
		ret = qdma_acc_read_dump_queue_context(dev, ip_type,
				qid, st_mode, q_type, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to read and dump h2c queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	}

	xdebug_info("%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_cmpt_context_dump(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	struct qdma_descq_context queue_context;
	enum qdma_dev_q_type q_type;
	enum qdma_ip_type ip_type;
	uint32_t buflen;
	uint16_t qid;
	uint8_t st_mode;
	char *buf = NULL;
	int ret = 0;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	qid = qdma_dev->queue_base + queue;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;
	st_mode = qdma_dev->q_info[qid].queue_mode;
	q_type = QDMA_DEV_Q_TYPE_CMPT;

	if (queue >= dev->data->nb_rx_queues) {
		xdebug_info("RX queue_id=%d not configured\n", queue);
		return -EINVAL;
	}

	xdebug_info(
		"\n ***** CMPT Queue Contexts on port_id: %d for q_id: %d *****\n",
		port_id, qid);

	ret = qdma_acc_context_buf_len(dev, ip_type,
			st_mode, q_type, &buflen);
	if (ret < 0) {
		xdebug_error("Failed to get context buffer length\n");
		return ret;
	}

	/*allocate memory for csr dump*/
	buf = (char *)rte_zmalloc("QDMA_CMPT_CONTEXT_DUMP",
			buflen, RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error("Unable to allocate memory for cmpt context dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	if (qdma_dev->is_vf) {
		ret = qdma_descq_context_read_vf(dev, qid,
			st_mode, q_type, &queue_context);
		if (ret < 0) {
			xdebug_error("Failed to read cmpt queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}

		ret = qdma_acc_dump_queue_context(dev, ip_type,
			st_mode, q_type,
			&queue_context, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to dump cmpt queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	} else {
		ret = qdma_acc_read_dump_queue_context(dev,
			ip_type, qid, st_mode,
			q_type, buf, buflen);
		if (ret < 0) {
			xdebug_error("Failed to read and dump cmpt queue context\n");
			rte_free(buf);
			return qdma_get_error_code(ret);
		}
	}

	xdebug_info("%s\n", buf);
	rte_free(buf);

	return 0;
}

static int qdma_queue_desc_dump(uint8_t port_id,
		struct xdebug_desc_param *param)
{
	struct rte_eth_dev *dev;
	struct qdma_rx_queue *rxq;
	struct qdma_tx_queue *txq;
	uint8_t *rx_ring_bypass = NULL;
	uint8_t *tx_ring_bypass = NULL;
	char str[50];
	int x;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];

	switch (param->type) {
	case RTE_PMD_QDMA_XDEBUG_DESC_C2H:

		if (param->queue >= dev->data->nb_rx_queues) {
			xdebug_info("queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq == NULL) {
			xdebug_info("Caught NULL pointer for queue_id: %d\n",
				param->queue);
			return -1;
		}

		if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
			xdebug_info("Queue_id %d is not yet started\n",
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

			xdebug_info("\n===== C2H bypass descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint8_t *rx_bypass =
						&rx_ring_bypass[x];
				snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
					(const void *)rx_bypass,
					rxq->bypass_desc_sz);
			}
		} else {
			if (rxq->st_mode) {
				struct qdma_ul_st_c2h_desc *rx_ring_st =
				(struct qdma_ul_st_c2h_desc *)rxq->rx_ring;

				xdebug_info("\n===== C2H ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					struct qdma_ul_st_c2h_desc *rx_st =
						&rx_ring_st[x];
					snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
					(const void *)rx_st,
					sizeof(struct qdma_ul_st_c2h_desc));
				}
			} else {
				struct qdma_ul_mm_desc *rx_ring_mm =
					(struct qdma_ul_mm_desc *)rxq->rx_ring;
				xdebug_info("\n====== C2H ring descriptors======\n");
				for (x = param->start; x < param->end; x++) {
					snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&rx_ring_mm[x],
						sizeof(struct qdma_ul_mm_desc));
				}
			}
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_DESC_CMPT:

		if (param->queue >= dev->data->nb_rx_queues) {
			xdebug_info("queue_id=%d not configured",
					param->queue);
			return -1;
		}

		rxq = (struct qdma_rx_queue *)
			dev->data->rx_queues[param->queue];

		if (rxq) {
			if (rxq->status != RTE_ETH_QUEUE_STATE_STARTED) {
				xdebug_info("Queue_id %d is not yet started\n",
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
				xdebug_info("Queue_id %d is not initialized "
					"in Stream mode\n", param->queue);
				return -1;
			}

			xdebug_info("\n===== CMPT ring descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint32_t *cmpt_ring = (uint32_t *)
					((uint64_t)(rxq->cmpt_ring) +
					((uint64_t)x * rxq->cmpt_desc_len));
				snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
						(const void *)cmpt_ring,
						rxq->cmpt_desc_len);
			}
		}
		break;
	case RTE_PMD_QDMA_XDEBUG_DESC_H2C:

		if (param->queue >= dev->data->nb_tx_queues) {
			xdebug_info("queue_id=%d not configured",
				param->queue);
			return -1;
		}

		txq = (struct qdma_tx_queue *)
			dev->data->tx_queues[param->queue];

		if (txq == NULL) {
			xdebug_info("Caught NULL pointer for queue_id: %d\n",
				param->queue);
			return -1;
		}

		if (txq->status != RTE_ETH_QUEUE_STATE_STARTED) {
			xdebug_info("Queue_id %d is not yet started\n",
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

			xdebug_info("\n====== H2C bypass descriptors=====\n");
			for (x = param->start; x < param->end; x++) {
				uint8_t *tx_bypass =
					&tx_ring_bypass[x];
				snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
				rte_hexdump(stdout, str,
					(const void *)tx_bypass,
					txq->bypass_desc_sz);
			}
		} else {
			if (txq->st_mode) {
				struct qdma_ul_st_h2c_desc *qdma_h2c_ring =
				(struct qdma_ul_st_h2c_desc *)txq->tx_ring;
				xdebug_info("\n====== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
					(const void *)&qdma_h2c_ring[x],
					sizeof(struct qdma_ul_st_h2c_desc));
				}
			} else {
				struct qdma_ul_mm_desc *tx_ring_mm =
					(struct qdma_ul_mm_desc *)txq->tx_ring;
				xdebug_info("\n===== H2C ring descriptors=====\n");
				for (x = param->start; x < param->end; x++) {
					snprintf(str, sizeof(str),
						"\nDescriptor ID %d\t", x);
					rte_hexdump(stdout, str,
						(const void *)&tx_ring_mm[x],
						sizeof(struct qdma_ul_mm_desc));
				}
			}
		}
		break;
	default:
		xdebug_info("Invalid ring selected\n");
		break;
	}
	return 0;
}

int rte_pmd_qdma_dbg_regdump(uint8_t port_id)
{
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	err = qdma_config_reg_dump(port_id);
	if (err) {
		xdebug_error("Error dumping Global registers\n");
		return err;
	}
	return 0;
}

int rte_pmd_qdma_dbg_reg_info_dump(uint8_t port_id,
	uint32_t num_regs, uint32_t reg_addr)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	enum qdma_ip_type ip_type;
	char *buf = NULL;
	int buflen = QDMA_MAX_BUFLEN;
	int ret;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	ip_type = (enum qdma_ip_type)qdma_dev->ip_type;

	/*allocate memory for register dump*/
	buf = (char *)rte_zmalloc("QDMA_DUMP_BUF_REG_INFO", buflen,
			RTE_CACHE_LINE_SIZE);
	if (!buf) {
		xdebug_error("Unable to allocate memory for reg info dump "
				"size %d\n", buflen);
		return -ENOMEM;
	}

	ret = qdma_acc_dump_reg_info(dev, ip_type,
		reg_addr, num_regs, buf, buflen);
	if (ret < 0) {
		xdebug_error("Failed to dump reg field values\n");
		rte_free(buf);
		return qdma_get_error_code(ret);
	}
	xdebug_info("%s\n", buf);
	rte_free(buf);

	return 0;
}

int rte_pmd_qdma_dbg_qdevice(uint8_t port_id)
{
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	err = qdma_device_dump(port_id);
	if (err) {
		xdebug_error("Error dumping QDMA device\n");
		return err;
	}
	return 0;
}

int rte_pmd_qdma_dbg_qinfo(uint8_t port_id, uint16_t queue)
{
	struct rte_eth_dev *dev;
	struct qdma_pci_dev *qdma_dev;
	uint16_t qid;
	uint8_t st_mode;
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	dev = &rte_eth_devices[port_id];
	qdma_dev = dev->data->dev_private;
	qid = qdma_dev->queue_base + queue;
	st_mode = qdma_dev->q_info[qid].queue_mode;

	err = qdma_h2c_context_dump(port_id, queue);
	if (err) {
		xdebug_error("Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	err = qdma_c2h_context_dump(port_id, queue);
	if (err) {
		xdebug_error("Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	if (!st_mode && qdma_dev->dev_cap.mm_cmpt_en) {
		err = qdma_cmpt_context_dump(port_id, queue);
		if (err) {
			xdebug_error("Error dumping %d: %d\n",
					queue, err);
			return err;
		}
	}

	err = qdma_h2c_struct_dump(port_id, queue);
	if (err) {
		xdebug_error("Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	err = qdma_c2h_struct_dump(port_id, queue);
	if (err) {
		xdebug_error("Error dumping %d: %d\n",
				queue, err);
		return err;
	}

	return 0;
}

int rte_pmd_qdma_dbg_qdesc(uint8_t port_id, uint16_t queue, int start,
		int end, enum rte_pmd_qdma_xdebug_desc_type type)
{
	struct xdebug_desc_param param;
	int err;

	if (port_id >= rte_eth_dev_count_avail()) {
		xdebug_error("Wrong port id %d\n", port_id);
		return -EINVAL;
	}

	param.queue = queue;
	param.start = start;
	param.end = end;
	param.type = type;

	err = qdma_queue_desc_dump(port_id, &param);
	if (err) {
		xdebug_error("Error dumping %d: %d\n",
			queue, err);
		return err;
	}
	return 0;
}
