/*-
 * BSD LICENSE
 *
 * Copyright(c) 2020-2021 Xilinx, Inc. All rights reserved.
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

#ifndef __QDMA_DEVOPS_H__
#define __QDMA_DEVOPS_H__


#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup dpdk_devops_func Functions
 */

/**
 * DPDK callback to register an Ethernet PCIe device.
 *
 * The Following actions are performed by this function:
 *  - Parse and validate device arguments
 *  - Identify PCIe BARs present in the device
 *  - Register device operations
 *  - Enable MM C2H and H2C channels
 *  - Register PCIe device with Queue Resource Manager
 *  - Program the QDMA IP global registers (by 1st PF that was probed)
 *  - Enable HW errors and launch QDMA HW error monitoring thread
 *    (by 1st PF that was probed)
 *  - If VF is enabled, then enable Mailbox interrupt and register
 *    Rx message handling function as interrupt handler
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_eth_dev_init(struct rte_eth_dev *dev);

/**
 * DPDK callback for Ethernet device configuration.
 *
 * This API requests the queue base from Queue Resource Manager and programs
 * the queue base and queue count in function map (FMAP)
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_configure(struct rte_eth_dev *dev);

/**
 * DPDK callback to get information about the device
 *
 * @param dev Pointer to Ethernet device structure
 * @param dev_info: Pointer to Device information structure
 *
 * @ingroup dpdk_devops_func
 */
int qdma_dev_infos_get(struct rte_eth_dev *dev,
				struct rte_eth_dev_info *dev_info);

/**
 * DPDK callback to retrieve the physical link information
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param wait_to_complete
 *   wait_to_complete field is ignored
 *
 * @ingroup dpdk_devops_func
 */
int qdma_dev_link_update(struct rte_eth_dev *dev,
				__rte_unused int wait_to_complete);

/**
 * DPDK callback to configure a RX queue.
 *
 * This API validates queue parameters and allocates C2H ring and
 * Streaming CMPT ring from the DPDK reserved hugepage memory zones
 *
 * @param dev Pointer to Ethernet device structure.
 * @param rx_queue_id RX queue index relative to the PCIe function
 *                    pointed by dev
 * @param nb_rx_desc Number of C2H descriptors to configure for this queue
 * @param socket_id NUMA socket on which memory must be allocated
 * @param rx_conf Rx queue configuration parameters
 * @param mb_pool Memory pool to use for buffer allocations on this queue
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
				uint16_t nb_rx_desc, unsigned int socket_id,
				const struct rte_eth_rxconf *rx_conf,
				struct rte_mempool *mb_pool);

/**
 * DPDK callback to configure a TX queue.
 *
 * This API validates queue parameters and allocates H2C ring from the
 * DPDK reserved hugepage memory zone
 *
 * @param dev Pointer to Ethernet device structure
 * @param tx_queue_id TX queue index
 * @param nb_tx_desc Number of descriptors to configure in queue
 * @param socket_id NUMA socket on which memory must be allocated
 * @param tx_conf Tx queue configuration parameters
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
			    uint16_t nb_tx_desc, unsigned int socket_id,
			    const struct rte_eth_txconf *tx_conf);

/**
 * DPDK callback to get Rx queue info of an Ethernet device
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 * @param qinfo
 *   A pointer to a structure of type rte_eth_rxq_info_info to be filled with
 *   the information of given Rx queue
 *
 * @ingroup dpdk_devops_func
 */
void
qdma_dev_rxq_info_get(struct rte_eth_dev *dev, uint16_t rx_queue_id,
		     struct rte_eth_rxq_info *qinfo);

/**
 * DPDK callback to get Tx queue info of an Ethernet device
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param tx_queue_id
 *   The TX queue on the Ethernet device for which information will be
 *   retrieved
 * @param qinfo
 *   A pointer to a structure of type rte_eth_txq_info_info to be filled with
 *   the information of given Tx queue
 *
 * @ingroup dpdk_devops_func
 */
void
qdma_dev_txq_info_get(struct rte_eth_dev *dev, uint16_t tx_queue_id,
		      struct rte_eth_txq_info *qinfo);

/**
 * DPDK callback to start the device.
 *
 * This API starts the Ethernet device by initializing Rx, Tx descriptors
 * and device registers. For the Port queues whose start is not deferred,
 * it calls qdma_dev_tx_queue_start and qdma_dev_rx_queue_start to start
 * the queues for packet processing.
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_start(struct rte_eth_dev *dev);

/**
 * DPDK callback to start a C2H queue which has been deferred start.
 *
 * This API clears and then programs the Software, Prefetch and
 * Completion context of the C2H queue
 *
 * @param dev Pointer to Ethernet device structure
 * @param qid Rx queue index
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid);

/**
 * DPDK callback to start a H2C queue which has been deferred start.
 *
 * This API clears and then programs the Software context of the H2C queue
 *
 * @param dev Pointer to Ethernet device structure
 * @param qid Tx queue index
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid);

/**
 * DPDK callback for receiving packets in burst.
 *
 * This API does following operations:
 *	- Process the Completion ring to determine and store packet information
 *	- Update CMPT CIDX
 *	- Process C2H ring to retrieve rte_mbuf pointers corresponding to
 *    received packets and store in rx_pkts array.
 *	- Populate C2H ring with new pointers for packet buffers
 *	- Update C2H ring PIDX
 *
 * @param rx_queue Generic pointer to Rx queue structure
 * @param rx_pkts The address of an array of pointers to rte_mbuf structures
 *                 that must be large enough to store nb_pkts pointers in it
 * @param nb_pkts Maximum number of packets to retrieve
 *
 * @return Number of packets successfully received (<= nb_pkts)
 * @ingroup dpdk_devops_func
 *
 */
uint16_t qdma_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts);

/**
 * DPDK callback for transmitting packets in burst.
 *
 * This API does following operations:
 *	- Free rte_mbuf pointers to previous transmitted packets,
 *    back to the memory pool
 *	- Retrieve packet buffer pointer from tx_pkts and populate H2C ring
 *    with pointers to new packet buffers.
 *	- Update H2C ring PIDX
 *
 * @param tx_queue Generic pointer to Tx queue structure
 * @param tx_pkts The address of an array of nb_pkts pointers to
 *                rte_mbuf structures which contain the output packets
 * @param nb_pkts The maximum number of packets to transmit
 *
 * @return Number of packets successfully transmitted (<= nb_pkts)
 * @ingroup dpdk_devops_func
 *
 */
uint16_t qdma_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts);

/**
 * DPDK callback for retrieving Port statistics.
 *
 * This API updates Port statistics in rte_eth_stats structure parameters
 *
 * @param dev Pointer to Ethernet device structure
 * @param eth_stats Pointer to structure containing statistics
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_stats_get(struct rte_eth_dev *dev,
			      struct rte_eth_stats *eth_stats);

/**
 * DPDK callback to reset Port statistics.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 *
 * @ingroup dpdk_devops_func
 */
int qdma_dev_stats_reset(struct rte_eth_dev *dev);

/**
 * DPDK callback to set a queue statistics mapping for
 * a tx/rx queue of an Ethernet device.
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param queue_id
 *   Index of the queue for which a queue stats mapping is required
 * @param stat_idx
 *   The per-queue packet statistics functionality number that
 *   the queue_id is to be assigned
 * @param is_rx
 *   Whether queue is a Rx or a Tx queue
 *
 * @return
 *   0 on success, -EINVAL on failure
 * @ingroup dpdk_devops_func
 */
int qdma_dev_queue_stats_mapping(struct rte_eth_dev *dev,
					     uint16_t queue_id,
					     uint8_t stat_idx,
					     uint8_t is_rx);

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
 * DPDK callback to check the status of a Rx descriptor in the queue
 *
 * @param rx_queue
 *   Pointer to Rx queue specific data structure
 * @param offset
 *   The offset of the descriptor starting from tail (0 is the next
 *   packet to be received by the driver)
 *
 * @return
 *  - (RTE_ETH_RX_DESC_AVAIL): Descriptor is available for the hardware to
 *    receive a packet.
 *  - (RTE_ETH_RX_DESC_DONE): Descriptor is done, it is filled by hw, but
 *    not yet processed by the driver (i.e. in the receive queue).
 *  - (RTE_ETH_RX_DESC_UNAVAIL): Descriptor is unavailable, either hold by
 *    the driver and not yet returned to hw, or reserved by the hw.
 *  - (-EINVAL) bad descriptor offset.
 * @ingroup dpdk_devops_func
 */
int
qdma_dev_rx_descriptor_status(void *rx_queue, uint16_t offset);

/**
 * DPDK callback to check the status of a Tx descriptor in the queue
 *
 * @param tx_queue
 *   Pointer to Tx queue specific data structure
 * @param offset
 *   The offset of the descriptor starting from tail (0 is the place where
 *   the next packet will be send)
 *
 * @return
 *  - (RTE_ETH_TX_DESC_FULL) Descriptor is being processed by the hw, i.e.
 *    in the transmit queue.
 *  - (RTE_ETH_TX_DESC_DONE) Hardware is done with this descriptor, it can
 *    be reused by the driver.
 *  - (RTE_ETH_TX_DESC_UNAVAIL): Descriptor is unavailable, reserved by the
 *    driver or the hardware.
 *  - (-EINVAL) bad descriptor offset.
 * @ingroup dpdk_devops_func
 */
int
qdma_dev_tx_descriptor_status(void *tx_queue, uint16_t offset);

/**
 * DPDK callback to request the driver to free mbufs
 * currently cached by the driver
 *
 * @param tx_queue
 *   Pointer to Tx queue specific data structure
 * @param free_cnt
 *   Maximum number of packets to free. Use 0 to indicate all possible packets
 *   should be freed. Note that a packet may be using multiple mbufs.
 *
 * @return
 *   - Failure: < 0
 *   - Success: >= 0
 *     0-n: Number of packets freed. More packets may still remain in ring that
 *     are in use.
 * @ingroup dpdk_devops_func
 */
int
qdma_dev_tx_done_cleanup(void *tx_queue, uint32_t free_cnt);

/**
 * DPDK callback to retrieve device registers and
 * register attributes (number of registers and register size)
 *
 * @param dev
 *   Pointer to Ethernet device structure
 * @param regs
 *   Pointer to rte_dev_reg_info structure to fill in. If regs->data is
 *   NULL the function fills in the width and length fields. If non-NULL
 *   the registers are put into the buffer pointed at by the data field.
 *
 * @return
 *   0 on success, -ENOTSUP on failure
 * @ingroup dpdk_devops_func
 */
int
qdma_dev_get_regs(struct rte_eth_dev *dev,
	      struct rte_dev_reg_info *regs);

/**
 * DPDK callback to stop a C2H queue
 *
 * This API invalidates Hardware, Software, Prefetch and completion contexts
 * of C2H queue. It also free the rte_mbuf pointers assigned to descriptors
 * prepared for packet reception.
 *
 * @param dev Pointer to Ethernet device structure
 * @param qid Rx queue index
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);

/**
 * qdma_dev_tx_queue_stop() - DPDK callback to stop a queue in H2C direction
 *
 * This API invalidates Hardware, Software contexts of H2C queue. It also free
 * the rte_mbuf pointers assigned to descriptors that are pending transmission.
 *
 * @param dev Pointer to Ethernet device structure
 * @param qid TX queue index
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);


/**
 * DPDK callback to stop the device.
 *
 * This API stops the device by invalidating all the contexts of all the queues
 * belonging to the port by calling qdma_dev_tx_queue_stop() and
 * qdma_dev_rx_queue_stop() for all the queues of the port.
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @ingroup dpdk_devops_func
 *
 */
int qdma_dev_stop(struct rte_eth_dev *dev);

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
 * DPDK callback to close the device.
 *
 * This API frees the descriptor rings and objects beonging to all the queues
 * of the given port. It also clears the FMAP.
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @ingroup dpdk_devops_func
 */
int qdma_dev_close(struct rte_eth_dev *dev);

/**
 * DPDK callback to close the VF device.
 *
 * This API frees the descriptor rings and objects beonging to all the queues
 * of the given port. It also clears the FMAP.
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @ingroup dpdk_devops_func
 */
int qdma_vf_dev_close(struct rte_eth_dev *dev);

/**
 * DPDK callback to reset the device.
 *
 * This callback is invoked when applcation calls rte_eth_dev_reset() API
 * to reset a device. This callback uninitialzes PF device after waiting for
 * all its VFs to shutdown. It initialize back PF device and then send
 * Reset done mailbox message to all its VFs to come online again.
 *
 * @param dev
 *   Pointer to Ethernet device structure
 *
 * @return
 *   0 on success, negative errno value on failure
 * @ingroup dpdk_devops_func
 */
int qdma_dev_reset(struct rte_eth_dev *dev);

/**
 * DPDK callback to deregister a PCI device.
 *
 * The Following actions are performed by this function:
 *  - Flushes out pending actions from the Tx Mailbox List
 *  - Terminate Tx Mailbox thread
 *  - Disable Mailbox interrupt and unregister interrupt handler
 *  - Unregister PCIe device from Queue Resource Manager
 *  - Cancel QDMA HW error monitoring thread if created by this device
 *  - Disable MM C2H and H2C channels
 *
 * @param dev Pointer to Ethernet device structure
 *
 * @return 0 on success, < 0 on failure
 * @ingroup dpdk_devops_func
 *
 */
int qdma_eth_dev_uninit(struct rte_eth_dev *dev);

#ifdef __cplusplus
}
#endif
#endif /* ifndef __QDMA_DEVOPS_H__ */
