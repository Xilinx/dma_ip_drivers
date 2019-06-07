/**
 * DOC: QDMA DPDK driver Callbacks
 *
 * Callback APIs implemented by PF driver are described below.
 * The implementation details for VF are similar in nature.
 * There will be extra mailbox communication to PF from VF APIs
 * for queue configuration.
 *
 */


/**
 * eth_qdma_dev_init() - DPDK callback to register an Ethernet PCIe device.
 *
 * The Following actions are performed by this function:
 *  - Parse and validate device arguments
 *  - Identify PCIe BARs present in the device
 *  - Register device operations
 *  - Enable MM C2H and H2C channels
 *  - Register PCIe device with Queue Resource Manager
 *  - Program the QDMA IP global registers (by 1st PF that was probed)
 *  - Enable HW errors and launch QDMA HW error monitoring thread (by 1st PF that was probed)
 *  - If VF is enabled, then enable Mailbox interrupt and register Rx message handling function as interrupt handler
 *
 * @dev: Pointer to Ethernet device structure.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int eth_qdma_dev_init(struct rte_eth_dev *dev);

/**
 * qdma_dev_configure() - DPDK callback for Ethernet device configuration.
 *
 * This API requests the queue base from Queue Resource Manager and programs the queue base and queue count in function map (FMAP).
 *
 * @dev: Pointer to Ethernet device structure.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_configure(struct rte_eth_dev *dev);

/**
 * qdma_dev_infos_get() - DPDK callback to get information about the device.
 *
 * @dev: Pointer to Ethernet device structure.
 * @dev_info: Pointer to Device information structure.
 */
void qdma_dev_infos_get(__rte_unused struct rte_eth_dev *dev,
				struct rte_eth_dev_info *dev_info);


/**
 * qdma_dev_rx_queue_setup() - DPDK callback to configure a RX queue.
 *
 * This API validates queue parameters and allocates C2H ring and Streaming CMPT ring from the DPDK reserved hugepage Memory zones
 *
 * @dev: Pointer to Ethernet device structure.
 * @rx_queue_id: RX queue index relative to the PCIe function pointed by ``dev``.
 * @nb_rx_desc: Number of C2H descriptors to configure for this queue.
 * @socket_id: NUMA socket on which memory must be allocated.
 * @rx_conf: Rx queue configuration parameters.
 * @mb_pool: Memory pool to use for buffer allocations on this queue.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
				uint16_t nb_rx_desc, unsigned int socket_id,
				const struct rte_eth_rxconf *rx_conf,
				struct rte_mempool *mb_pool);

/**
 *qdma_dev_tx_queue_setup() - DPDK callback to configure a TX queue.
 *
 * This API validates queue parameters and allocates H2C ring from the DPDK reserved hugepage Memory zone
 *
 * @dev: Pointer to Ethernet device structure.
 * @tx_queue_id: TX queue index.
 * @nb_tx_desc: Number of descriptors to configure in queue.
 * @socket_id: NUMA socket on which memory must be allocated.
 * @tx_conf: Tx queue configuration parameters.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
			    uint16_t nb_tx_desc, unsigned int socket_id,
			    const struct rte_eth_txconf *tx_conf);

/**
 * qdma_dev_start() - DPDK callback to start the device.
 *
 * This API starts the Ethernet device by initializing Rx, Tx descriptors and device registers.
 * For the Port queues whose start is not deferred, it calls qdma_dev_tx_queue_start and qdma_dev_rx_queue_start to start the queues for packet processing.
 *
 * @dev: Pointer to Ethernet device structure.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_start(struct rte_eth_dev *dev);

/**
 * qdma_dev_rx_queue_start() - DPDK callback to start a C2H queue which has been deferred start.
 *
 * This API clears and then programs the Software, Prefetch and Completion context of the C2H queue
 *
 * @dev: Pointer to Ethernet device structure.
 * @qid: Rx queue index.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid);

/**
 * qdma_dev_tx_queue_start() - DPDK callback to start a H2C queue which has been deferred start.
 *
 * This API clears and then programs the Software context of the H2C queue
 *
 * @dev: Pointer to Ethernet device structure.
 * @qid: Tx queue index.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid);

/**
 * qdma_recv_pkts() - DPDK callback for receiving packets in burst.
 *
 * This API does following operations:
 *	- Process the Completion ring to determine and store packet information
 *	- Update CMPT CIDX
 *	- Process C2H ring to retrieve rte_mbuf pointers corresponding to received packets and store in ``rx_pkts`` array.
 *	- Populate C2H ring with new pointers for packet buffers
 *	- Update C2H ring PIDX
 *
 * @rx_queue: Generic pointer to Rx queue structure.
 * @rx_pkts: The address of an array of pointers to rte_mbuf structures that must be large enough to store ``nb_pkts`` pointers in it.
 * @nb_pkts: Maximum number of packets to retrieve.
 *
 * Return: Number of packets successfully received (<= nb_pkts).
 *
 */
uint16_t qdma_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts);

/**
 * qdma_xmit_pkts() - DPDK callback for transmitting packets in burst.
 *
 * This API does following operations:
 *	- Free rte_mbuf pointers to previous transmitted packets, back to the memory pool
 *	- Retrieve packet buffer pointer from ``tx_pkts`` and populate H2C ring with pointers to new packet buffers.
 *	- Update H2C ring PIDX
 *
 * @tx_queue: Generic pointer to Tx queue structure.
 * @tx_pkts: The address of an array of ``nb_pkts`` pointers to rte_mbuf structures which contain the output packets.
 * @nb_pkts: The maximum number of packets to transmit.
 *
 * Return: Number of packets successfully transmitted (<= nb_pkts).
 *
 */
uint16_t qdma_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts);

/**
 * qdma_dev_stats_get() - DPDK callback for retrieving Port statistics.
 *
 * This API updates Port statistics in ``rte_eth_stats`` structure parameters.
 *
 * @dev: Pointer to Ethernet device structure.
 * @eth_stats: Pointer to structure containing statistics.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_stats_get(struct rte_eth_dev *dev,
			      struct rte_eth_stats *eth_stats);

/**
 * qdma_dev_rx_queue_stop() - DPDK callback to stop a C2H queue
 *
 * This API invalidates Hardware, Software, Prefetch and completion contexts of C2H queue.
 * It also free the rte_mbuf pointers assigned to descriptors prepared for packet reception.
 *
 * @dev: Pointer to Ethernet device structure.
 * @qid: Rx queue index.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);

/**
 * qdma_dev_tx_queue_stop() - DPDK callback to stop a queue in H2C direction
 *
 * This API invalidates Hardware, Software contexts of H2C queue.
 * It also free the rte_mbuf pointers assigned to descriptors that are pending transmission.
 *
 * @dev: Pointer to Ethernet device structure.
 * @qid: TX queue index.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int qdma_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);


/**
 * qdma_dev_stop() - DPDK callback to stop the device.
 *
 * This API stops the device by invalidating all the contexts of all the queues
 * belonging to the port by calling qdma_dev_tx_queue_stop and qdma_dev_rx_queue_stop for all the queues of the port.
 *
 * @dev: Pointer to Ethernet device structure.
 *
 */
void qdma_dev_stop(struct rte_eth_dev *dev);

/**
 * qdma_dev_rx_queue_release() - DPDK callback to release a Rx queue.
 *
 * This API releases the descriptor rings and any additional memory allocated for given C2H queue
 *
 * @rqueue: Generic Rx queue pointer.
 *
 */
void qdma_dev_rx_queue_release(void *rqueue);

/**
 * qdma_dev_tx_queue_release() - DPDK callback to release a Tx queue.
 *
 * This API releases the descriptor rings and any additional memory allocated for given H2C queue
 *
 * @tqueue: Generic Tx queue pointer.
 *
 */
void qdma_dev_tx_queue_release(void *tqueue);

/**
 * qdma_dev_close() - DPDK callback to close the device.
 *
 * This API frees the descriptor rings and objects beonging to all the queues of the given port.
 * Clears the FMAP (Queue range assigned to the port).
 *
 * @dev: Pointer to Ethernet device structure.
 *
 */
void qdma_dev_close(struct rte_eth_dev *dev);

/**
 * eth_qdma_dev_uninit() - DPDK callback to deregister a PCI device.
 *
 * The Following actions are performed by this function:
 *  - Flushes out pending actions from the Tx Mailbox List
 *  - Terminate Tx Mailbox thread
 *  - Disable Mailbox interrupt and unregister interrupt handler
 *  - Unregister PCIe device from Queue Resource Manager
 *  - Cancel QDMA HW error monitoring thread if created by this device
 *  - Disable MM C2H and H2C channels
 *
 * @dev: Pointer to Ethernet device structure.
 *
 * Return: 0 on success, < 0 on failure.
 *
 */
int eth_qdma_dev_uninit(struct rte_eth_dev *dev);
