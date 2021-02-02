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

#include <rte_ethdev_pci.h>
#include "qdma.h"
#include "qdma_mbox.h"
#include <rte_malloc.h>
#include <rte_spinlock.h>
#include <rte_alarm.h>

/*
 * Get index from VF info array of PF device for a given VF funcion id.
 */
static int qdma_get_internal_vf_index(struct rte_eth_dev *dev, uint8_t devfn)
{
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint16_t  i;

	for (i = 0; i < pci_dev->max_vfs; i++) {
		if (qdma_dev->vfinfo[i].func_id == devfn)
			return i;
	}

	return QDMA_FUNC_ID_INVALID;
}

static void qdma_mbox_process_msg_from_vf(void *arg)
{
	struct qdma_mbox_msg *mbox_msg_rsp = qdma_mbox_msg_alloc();
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	uint16_t vf_func_id;
	uint16_t vf_index;
	int i, rv;

	if (mbox_msg_rsp == NULL)
		return;

	if (!qdma_dev)
		return;

	rv = qdma_mbox_pf_rcv_msg_handler(dev,
					  qdma_dev->dma_device_index,
					  qdma_dev->func_id,
					  qdma_dev->mbox.rx_data,
					  mbox_msg_rsp->raw_data);
	if (rv != QDMA_MBOX_VF_OFFLINE &&
			rv != QDMA_MBOX_VF_RESET &&
			rv != QDMA_MBOX_PF_RESET_DONE &&
			rv != QDMA_MBOX_PF_BYE)
		qdma_mbox_msg_send(dev, mbox_msg_rsp, 0);
	else
		qdma_mbox_msg_free(mbox_msg_rsp);

	if (rv == QDMA_MBOX_VF_ONLINE) {
		vf_func_id = qdma_mbox_vf_func_id_get(qdma_dev->mbox.rx_data,
			qdma_dev->is_vf);
		/* Mapping internal VF function id to a valid VF function id */
		for (i = 0; i < pci_dev->max_vfs; i++) {
			if (qdma_dev->vfinfo[i].func_id ==
					QDMA_FUNC_ID_INVALID) {
				qdma_dev->vfinfo[i].func_id =
					vf_func_id;
				break;
			}
		}

		if (i == pci_dev->max_vfs) {
			PMD_DRV_LOG(INFO, "PF-%d  failed to create function id mapping VF func_id%d",
					qdma_dev->func_id, vf_func_id);
			return;
		}

		qdma_dev->vf_online_count++;
	} else if (rv == QDMA_MBOX_VF_OFFLINE) {
		if (!qdma_dev->reset_in_progress) {
			vf_func_id =
				qdma_mbox_vf_func_id_get(qdma_dev->mbox.rx_data,
					qdma_dev->is_vf);
			vf_index = qdma_get_internal_vf_index(dev, vf_func_id);
			if (vf_index != QDMA_FUNC_ID_INVALID)
				qdma_dev->vfinfo[vf_index].func_id =
					QDMA_FUNC_ID_INVALID;
		}
		qdma_dev->vf_online_count--;
	}
}

static void *qdma_reset_task(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (!qdma_dev)
		return NULL;

	rte_eth_dev_callback_process(dev, RTE_ETH_EVENT_INTR_RESET,
					      NULL);

	return NULL;
}

static void *qdma_remove_task(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (!qdma_dev)
		return NULL;

	rte_eth_dev_callback_process(dev, RTE_ETH_EVENT_INTR_RMV,
					      NULL);

	return NULL;
}

static void qdma_mbox_process_msg_from_pf(void *arg)
{
	struct qdma_mbox_msg *mbox_msg_rsp = NULL;
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	pthread_t thread;
	pthread_attr_t tattr;
	int rv;

	if (!qdma_dev)
		return;

	mbox_msg_rsp = qdma_mbox_msg_alloc();
	if (!mbox_msg_rsp)
		return;

	rv = qdma_mbox_vf_rcv_msg_handler(qdma_dev->mbox.rx_data,
					  mbox_msg_rsp->raw_data);
	if (rv)
		qdma_mbox_msg_send(dev, mbox_msg_rsp, 0);
	else {
		qdma_mbox_msg_free(mbox_msg_rsp);
		return;
	}

	if (rv == QDMA_MBOX_VF_RESET) {
		qdma_dev->reset_state = RESET_STATE_RECV_PF_RESET_REQ;

		rv = pthread_attr_init(&tattr);
		if (rv)
			PMD_DRV_LOG(ERR,
				"Failed pthread_attr_init for PF reset\n");

		rv = pthread_attr_setdetachstate(&tattr,
					PTHREAD_CREATE_DETACHED);
		if (rv)
			PMD_DRV_LOG(ERR,
				"Failed pthread_attr_setdetachstate for PF reset\n");

		if (pthread_create(&thread, NULL,
				qdma_reset_task, (void *)dev)) {
			PMD_DRV_LOG(ERR, "Could not create qdma reset"
					" starter thread\n");
		}
	} else if (rv == QDMA_MBOX_PF_RESET_DONE) {
		qdma_dev->reset_state = RESET_STATE_RECV_PF_RESET_DONE;
	} else if (rv == QDMA_MBOX_PF_BYE) {
		rv = pthread_attr_init(&tattr);
		if (rv)
			PMD_DRV_LOG(ERR,
				"Failed pthread_attr_init for PF shutdown\n");

		rv = pthread_attr_setdetachstate(&tattr,
					PTHREAD_CREATE_DETACHED);
		if (rv)
			PMD_DRV_LOG(ERR,
				"Failed pthread_attr_setdetachstate for PF shutdown\n");

		if (pthread_create(&thread, NULL,
				qdma_remove_task, (void *)dev)) {
			PMD_DRV_LOG(ERR,
				"Could not create qdma remove"
				" starter thread\n");
		}
	}
}

static void qdma_mbox_process_rsp_from_pf(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_list_head *entry = NULL;
	struct qdma_list_head *tmp = NULL;

	if (!qdma_dev)
		return;

	if ((qdma_dev->is_vf) && (qdma_dev->func_id == 0)) {
		qdma_dev->func_id =
			qdma_mbox_vf_func_id_get(qdma_dev->mbox.rx_data,
					qdma_dev->is_vf);
		PMD_DRV_LOG(INFO, "VF function ID: %d", qdma_dev->func_id);
	}

	rte_spinlock_lock(&qdma_dev->mbox.list_lock);
	qdma_list_for_each_safe(entry, tmp,
				&qdma_dev->mbox.rx_pend_list) {
		struct qdma_mbox_msg *msg = QDMA_LIST_GET_DATA(entry);

		if (qdma_mbox_is_msg_response(msg->raw_data,
					      qdma_dev->mbox.rx_data)) {
			/* copy response message back to tx buffer */
			memcpy(msg->raw_data, qdma_dev->mbox.rx_data,
			       MBOX_MSG_REG_MAX * sizeof(uint32_t));
			msg->rsp_rcvd = 1;
			qdma_list_del(entry);
			break;
		}
	}
	rte_spinlock_unlock(&qdma_dev->mbox.list_lock);
}

static void qdma_mbox_rcv_task(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	int rv;

	if (!qdma_dev)
		return;

	do {
		memset(qdma_dev->mbox.rx_data, 0,
		       MBOX_MSG_REG_MAX * sizeof(uint32_t));
		rv = qdma_mbox_rcv(dev, qdma_dev->is_vf,
				   qdma_dev->mbox.rx_data);
		if (rv < 0)
			break;
		if (qdma_dev->is_vf) {
			qdma_mbox_process_msg_from_pf(arg);
			qdma_mbox_process_rsp_from_pf(arg);
		} else
			qdma_mbox_process_msg_from_vf(arg);

	} while (1);

	if (!qdma_dev->dev_cap.mailbox_intr)
		rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_mbox_rcv_task, arg);
}

static void qdma_mbox_send_task(void *arg)
{
	struct rte_eth_dev *dev = (struct rte_eth_dev *)arg;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_list_head *entry = NULL;
	struct qdma_list_head *tmp = NULL;
	int rv;

	rte_spinlock_lock(&qdma_dev->mbox.list_lock);
	qdma_list_for_each_safe(entry, tmp, &qdma_dev->mbox.tx_todo_list) {
		struct qdma_mbox_msg *msg = QDMA_LIST_GET_DATA(entry);

		rv = qdma_mbox_send(dev, qdma_dev->is_vf, msg->raw_data);
		if (rv < 0) {
			msg->retry_cnt--;
			if (!msg->retry_cnt) {
				qdma_list_del(entry);
				if (msg->rsp_wait == QDMA_MBOX_RSP_NO_WAIT)
					qdma_mbox_msg_free(msg);
			}
		} else {
			qdma_list_del(entry);
			if (msg->rsp_wait == QDMA_MBOX_RSP_WAIT)
				qdma_list_add_tail(entry,
					   &qdma_dev->mbox.rx_pend_list);
			else
				qdma_mbox_msg_free(msg);
		}
	}
	if (!qdma_list_is_empty(&qdma_dev->mbox.tx_todo_list))
		rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_mbox_send_task, arg);
	rte_spinlock_unlock(&qdma_dev->mbox.list_lock);
}

int qdma_mbox_msg_send(struct rte_eth_dev *dev, struct qdma_mbox_msg *msg,
		       unsigned int timeout_ms)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	if (!msg)
		return -EINVAL;

	msg->retry_cnt = timeout_ms ? ((timeout_ms / MBOX_POLL_FRQ) + 1) :
			MBOX_SEND_RETRY_COUNT;
	QDMA_LIST_SET_DATA(&msg->node, msg);

	rte_spinlock_lock(&qdma_dev->mbox.list_lock);
	qdma_list_add_tail(&msg->node, &qdma_dev->mbox.tx_todo_list);
	rte_spinlock_unlock(&qdma_dev->mbox.list_lock);

	msg->rsp_wait = (!timeout_ms) ? QDMA_MBOX_RSP_NO_WAIT :
			QDMA_MBOX_RSP_WAIT;
	rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_mbox_send_task, dev);

	if (!timeout_ms)
		return 0;

	/* if code reached here, caller should free the buffer */
	while (msg->retry_cnt && !msg->rsp_rcvd)
		rte_delay_ms(1);

	if (!msg->rsp_rcvd)
		return  -EPIPE;

	return 0;
}

void *qdma_mbox_msg_alloc(void)
{
	return rte_zmalloc(NULL, sizeof(struct qdma_mbox_msg), 0);
}

void qdma_mbox_msg_free(void *buffer)
{
	rte_free(buffer);
}

int qdma_mbox_init(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	uint32_t raw_data[MBOX_MSG_REG_MAX] = {0};
	struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;

	if (!qdma_dev->is_vf) {
		int i;

		for (i = 0; i < pci_dev->max_vfs; i++)
			qdma_mbox_rcv(dev, 0, raw_data);
	} else
		qdma_mbox_rcv(dev, 1, raw_data);

	qdma_mbox_hw_init(dev, qdma_dev->is_vf);
	qdma_list_init_head(&qdma_dev->mbox.tx_todo_list);
	qdma_list_init_head(&qdma_dev->mbox.rx_pend_list);
	rte_spinlock_init(&qdma_dev->mbox.list_lock);

	if (qdma_dev->dev_cap.mailbox_intr) {
		/* Register interrupt call back handler */
		rte_intr_callback_register(intr_handle,
					qdma_mbox_rcv_task, dev);

		/* enable uio/vfio intr/eventfd mapping */
		rte_intr_enable(intr_handle);

		/* enable qdma mailbox interrupt */
		qdma_mbox_enable_interrupts((void *)dev, qdma_dev->is_vf);
	} else {
		rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_mbox_rcv_task,
				  (void *)dev);
	}

	return 0;
}

void qdma_mbox_uninit(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;

	do {
		rte_spinlock_lock(&qdma_dev->mbox.list_lock);
		if (!qdma_list_is_empty(&qdma_dev->mbox.tx_todo_list)) {
			rte_spinlock_unlock(&qdma_dev->mbox.list_lock);
			rte_delay_ms(100);
			continue;
		}
		rte_spinlock_unlock(&qdma_dev->mbox.list_lock);
		break;
	} while (1);

	rte_eal_alarm_cancel(qdma_mbox_send_task, (void *)dev);
	if (qdma_dev->dev_cap.mailbox_intr) {
		/* Disable the mailbox interrupt */
		qdma_mbox_disable_interrupts((void *)dev, qdma_dev->is_vf);

		/* Disable uio intr before callback unregister */
		rte_intr_disable(intr_handle);

		rte_intr_callback_unregister(intr_handle,
				qdma_mbox_rcv_task, dev);
	} else {
		rte_eal_alarm_cancel(qdma_mbox_rcv_task, (void *)dev);
	}
}

