/*-
 * BSD LICENSE
 *
 * Copyright (c) 2017-2022 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __QDMA_DPDK_COMPAT_H__
#define __QDMA_DPDK_COMPAT_H__

#if (defined(QDMA_DPDK_21_11) || defined(QDMA_DPDK_22_11) || \
     defined(QDMA_DPDK_23_11))
#include <ethdev_driver.h>
#include <ethdev_pci.h>

#define ETH_LINK_UP RTE_ETH_LINK_UP
#define ETH_LINK_FULL_DUPLEX RTE_ETH_LINK_FULL_DUPLEX
#define ETH_SPEED_NUM_200G RTE_ETH_SPEED_NUM_200G
#define pci_dev_intr_handle pci_dev->intr_handle
#define qdma_dev_rx_queue_count qdma_dev_rx_queue_count_v2122
#define qdma_dev_rx_queue_release qdma_dev_rx_queue_release_v2122
#define qdma_dev_tx_queue_release qdma_dev_tx_queue_release_v2122

#ifdef QDMA_DPDK_21_11
#include <rte_bus_pci.h>
#else                            //QDMA_DPDK_22_11
#include <bus_pci_driver.h>
#endif

/**
 * DPDK callback to get the number of rx_queue
 *
 * @param rx_queue
 *   Generic recieve queue pointer
 * @return
 *   The number of receive queues
 * @ingroup dpdk_devops_func
 */
uint32_t
qdma_dev_rx_queue_count(void *rx_queue);

/**
 * DPDK callback to release a Rx queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 *
 * @ingroup dpdk_devops_func
 */
void qdma_dev_rx_queue_release(struct rte_eth_dev *dev, uint16_t queue_id);

/**
 * DPDK callback to release a Tx queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param queue_id
 *   The Tx queue on the Ethernet device for which information will be
 *   retrieved
 *
 * @ingroup dpdk_devops_func
 */

void qdma_dev_tx_queue_release(struct rte_eth_dev *dev, uint16_t queue_id);
#endif

#ifdef QDMA_DPDK_20_11

#include <rte_ethdev_pci.h>
#include <rte_ethdev_driver.h>
#define pci_dev_intr_handle (&pci_dev->intr_handle)
#define	qdma_dev_rx_queue_count qdma_dev_rx_queue_count_v2011
#define qdma_dev_rx_queue_release qdma_dev_rx_queue_release_v2011
#define qdma_dev_tx_queue_release qdma_dev_tx_queue_release_v2011

/**
 * DPDK callback to get the number of used descriptors of a rx queue
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 *
 * @return
 *   The number of used descriptors in the specific queue
 * @ingroup dpdk_devops_func
 */
uint32_t
qdma_dev_rx_queue_count(struct rte_eth_dev *dev, uint16_t rx_queue_id);

/**
 * DPDK callback to release a Tx queue.
 *
 * This API releases the descriptor rings and any additional memory allocated
 * for given H2C queue
 *
 * @param tqueue: Generic Tx queue pointer
 *
 * @ingroup dpdk_devops_func
 */
void qdma_dev_tx_queue_release(void *tqueue);

/**
 * DPDK callback to release a Rx queue.
 *
 * This API releases the descriptor rings and any additional memory allocated
 * for given C2H queue
 *
 * @param rqueue: Generic Rx queue pointer
 *
 * @ingroup dpdk_devops_func
 */
void qdma_dev_rx_queue_release(void *rqueue);

#endif


#endif /* ifndef __QDMA_DPDK_COMPAT_H__ */




