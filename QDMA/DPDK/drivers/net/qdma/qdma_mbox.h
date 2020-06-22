/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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
#ifndef QDMA_DPDK_MBOX_H_
#define QDMA_DPDK_MBOX_H_

#include "qdma_list.h"
#include "qdma_mbox_protocol.h"
#include <rte_ethdev.h>

#define MBOX_POLL_FRQ 1000
#define MBOX_OP_RSP_TIMEOUT (10000 * MBOX_POLL_FRQ) /* 10 sec */
#define MBOX_SEND_RETRY_COUNT (MBOX_OP_RSP_TIMEOUT/MBOX_POLL_FRQ)

enum qdma_mbox_rsp_state {
	QDMA_MBOX_RSP_NO_WAIT,
	QDMA_MBOX_RSP_WAIT
};

struct qdma_dev_mbox {
	struct qdma_list_head tx_todo_list;
	struct qdma_list_head rx_pend_list;
	rte_spinlock_t list_lock;
	uint32_t rx_data[MBOX_MSG_REG_MAX];
};

struct qdma_mbox_msg {
	uint8_t rsp_rcvd;
	uint32_t retry_cnt;
	enum qdma_mbox_rsp_state rsp_wait;
	uint32_t raw_data[MBOX_MSG_REG_MAX];
	struct qdma_list_head node;
};

int qdma_mbox_init(struct rte_eth_dev *dev);
void qdma_mbox_uninit(struct rte_eth_dev *dev);
void *qdma_mbox_msg_alloc(void);
void qdma_mbox_msg_free(void *buffer);
int qdma_mbox_msg_send(struct rte_eth_dev *dev, struct qdma_mbox_msg *msg,
		       unsigned int timeout_ms);
int qdma_dev_notify_qadd(struct rte_eth_dev *dev, uint32_t qidx_hw,
						enum qdma_dev_q_type q_type);
int qdma_dev_notify_qdel(struct rte_eth_dev *dev, uint32_t qidx_hw,
						enum qdma_dev_q_type q_type);

#endif /* QDMA_DPDK_MBOX_H_ */
