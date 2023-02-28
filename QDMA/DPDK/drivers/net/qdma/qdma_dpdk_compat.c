#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <rte_memzone.h>
#include <rte_string_fns.h>
#include <rte_malloc.h>
#include <rte_dev.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_alarm.h>
#include <rte_cycles.h>
#include <rte_atomic.h>
#include <unistd.h>
#include <string.h>
#include "qdma.h"
#include "qdma_access_common.h"
#include "qdma_mbox_protocol.h"
#include "qdma_mbox.h"
#include "qdma_reg_dump.h"
#include "qdma_platform.h"
#include "qdma_devops.h"


#if defined(QDMA_DPDK_21_11) || defined(QDMA_DPDK_22_11)

void qdma_dev_tx_queue_release(struct rte_eth_dev *dev,
			       uint16_t queue_id)
{
	struct qdma_tx_queue *txq =
	       (struct qdma_tx_queue *)dev->data->tx_queues[queue_id];
	struct qdma_pci_dev *qdma_dev;

	if (txq != NULL) {
		PMD_DRV_LOG(INFO, "Remove H2C queue: %d", txq->queue_id);
		qdma_dev = txq->dev->data->dev_private;

		if (!qdma_dev->is_vf)
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_H2C);
		else
			qdma_dev_notify_qdel(txq->dev, txq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_H2C);
		if (txq->sw_ring)
			rte_free(txq->sw_ring);
		if (txq->tx_mz)
			rte_memzone_free(txq->tx_mz);
		rte_free(txq);
		PMD_DRV_LOG(INFO, "H2C queue %d removed", txq->queue_id);
	}
}

void qdma_dev_rx_queue_release(struct rte_eth_dev *dev,
			       uint16_t queue_id)
{
	struct qdma_rx_queue *rxq =
	       (struct qdma_rx_queue *)dev->data->rx_queues[queue_id];
	struct qdma_pci_dev *qdma_dev = NULL;

	if (rxq != NULL) {
		PMD_DRV_LOG(INFO, "Remove C2H queue: %d", rxq->queue_id);
		qdma_dev = rxq->dev->data->dev_private;

		if (!qdma_dev->is_vf) {
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
		} else {
			qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
					qdma_dev->queue_base,
					QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_CMPT);
		}

		if (rxq->sw_ring)
			rte_free(rxq->sw_ring);
		if (rxq->st_mode) { /** if ST-mode **/
			if (rxq->rx_cmpt_mz)
				rte_memzone_free(rxq->rx_cmpt_mz);
		}
		if (rxq->rx_mz)
			rte_memzone_free(rxq->rx_mz);
		rte_free(rxq);
		PMD_DRV_LOG(INFO, "C2H queue %d removed", rxq->queue_id);
	}
}

/**
 * DPDK callback to get the number of receive queue.
 *
 * @param dev
 *   Generic pointer to receive queue
 *
 * @return
 *   The number of receieve queue count.
 */
uint32_t
qdma_dev_rx_queue_count(void *rx_queue)
{
	struct qdma_rx_queue *rxq = rx_queue;
	return rx_queue_count(rxq);
}

#endif

#ifdef QDMA_DPDK_20_11
void qdma_dev_tx_queue_release(void *tqueue)
{
	struct qdma_tx_queue *txq = (struct qdma_tx_queue *)tqueue;
	struct qdma_pci_dev *qdma_dev;
	if (txq != NULL) {
		PMD_DRV_LOG(INFO, "Remove H2C queue: %d", txq->queue_id);
		qdma_dev = txq->dev->data->dev_private;
		if (!qdma_dev->is_vf)
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_H2C);
		else
			qdma_dev_notify_qdel(txq->dev, txq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_H2C);
		if (txq->sw_ring)
			rte_free(txq->sw_ring);
		if (txq->tx_mz)
			rte_memzone_free(txq->tx_mz);
		rte_free(txq);
		PMD_DRV_LOG(INFO, "H2C queue %d removed", txq->queue_id);
	}
}

void qdma_dev_rx_queue_release(void *rqueue)
{
	struct qdma_rx_queue *rxq = (struct qdma_rx_queue *)rqueue;
	struct qdma_pci_dev *qdma_dev = NULL;
	if (rxq != NULL) {
		PMD_DRV_LOG(INFO, "Remove C2H queue: %d", rxq->queue_id);
		qdma_dev = rxq->dev->data->dev_private;
		if (!qdma_dev->is_vf) {
			qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_C2H);
			if (rxq->st_mode)
				qdma_dev_decrement_active_queue(
					qdma_dev->dma_device_index,
					qdma_dev->func_id,
					QDMA_DEV_Q_TYPE_CMPT);
		} else {
			qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
					qdma_dev->queue_base,
					QDMA_DEV_Q_TYPE_C2H);
			if (rxq->st_mode)
				qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_CMPT);
		}
		if (rxq->sw_ring)
			rte_free(rxq->sw_ring);
		if (rxq->st_mode) { /** if ST-mode **/
			if (rxq->rx_cmpt_mz)
				rte_memzone_free(rxq->rx_cmpt_mz);
		}
		if (rxq->rx_mz)
			rte_memzone_free(rxq->rx_mz);
		rte_free(rxq);
		PMD_DRV_LOG(INFO, "C2H queue %d removed", rxq->queue_id);
	}
}

/**
 * DPDK callback to get the number of used descriptors of a rx queue.
 *
 * @param dev
 *   Pointer to Ethernet device structure.
 * @param rx_queue_id
 *   The RX queue on the Ethernet device for which information will be
 *   retrieved
 *
 * @return
 *   The number of used descriptors in the specific queue.
 */
uint32_t
qdma_dev_rx_queue_count(struct rte_eth_dev *dev, uint16_t rx_queue_id)
{
	return rx_queue_count(dev->data->rx_queues[rx_queue_id]);
}

#endif

void rte_pmd_qdma_compat_memzone_reserve_aligned(void)
{
	const struct rte_memzone *mz = 0;

	mz = rte_memzone_reserve_aligned("eth_devices", RTE_MAX_ETHPORTS *
					  sizeof(*rte_eth_devices), 0, 0, 4096);

	if (mz == NULL)
		rte_exit(EXIT_FAILURE, "Failed to allocate aligned memzone\n");

	memcpy(mz->addr, &rte_eth_devices[0], RTE_MAX_ETHPORTS *
					sizeof(*rte_eth_devices));
}

void rte_pmd_qdma_get_bdf(uint32_t m_id, uint32_t *bus,
		uint32_t *dev, uint32_t *fn)
{
	struct rte_pci_device *pci_dev;
	pci_dev = RTE_ETH_DEV_TO_PCI(&rte_eth_devices[m_id]);
	*bus = pci_dev->addr.bus;
	*dev = pci_dev->addr.devid;
	*fn = pci_dev->addr.function;
}

int rte_pmd_qdma_dev_remove(int port_id)
{
	struct rte_device *dev;
	dev = rte_eth_devices[port_id].device;
	return rte_dev_remove(dev);
}

struct rte_device *rte_pmd_qdma_get_device(int port_id)
{
	struct rte_device *dev;
	dev = rte_eth_devices[port_id].device;
	return dev;
}

bool rte_pmd_qdma_validate_dev(int port_id)
{
	struct rte_device *device = rte_pmd_qdma_get_device(port_id);

	if (device && ((!strcmp(device->driver->name, "net_qdma")) ||
	     (!strcmp(device->driver->name, "net_qdma_vf"))))
		return true;
	else
		return false;
}

uint16_t rte_pmd_qdma_get_dev_id(int port_id)
{
	struct rte_pci_device *pci_dev;
	pci_dev = RTE_ETH_DEV_TO_PCI(&rte_eth_devices[port_id]);
	return pci_dev->id.device_id;
}

struct rte_pci_device *rte_pmd_qdma_eth_dev_to_pci(int port_id)
{
	return RTE_ETH_DEV_TO_PCI(&rte_eth_devices[port_id]);
}

unsigned int rte_pmd_qdma_compat_pci_read_reg(int port_id,
		unsigned int bar, unsigned int offset)
{
	return qdma_pci_read_reg(&rte_eth_devices[port_id], bar, offset);
}

void rte_pmd_qdma_compat_pci_write_reg(int port_id, uint32_t bar,
		uint32_t offset, uint32_t reg_val)
{
	qdma_pci_write_reg(&rte_eth_devices[port_id], bar, offset, reg_val);
}

void rte_pmd_qdma_dev_started(int port_id, bool status)
{
	struct rte_eth_dev *dev;
	dev = &rte_eth_devices[port_id];
	dev->data->dev_started = status;
}

int rte_pmd_qdma_dev_fp_ops_config(int port_id)
{
#if (defined(QDMA_DPDK_21_11) || defined(QDMA_DPDK_22_11))
	struct rte_eth_dev *dev;
	struct rte_eth_fp_ops *fpo = rte_eth_fp_ops;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR,
			"%s:%d Wrong port id %d\n",
			__func__, __LINE__, port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];

	fpo[port_id].rx_pkt_burst = dev->rx_pkt_burst;
	fpo[port_id].tx_pkt_burst = dev->tx_pkt_burst;
	fpo[port_id].rx_queue_count = dev->rx_queue_count;
	fpo[port_id].rx_descriptor_status = dev->rx_descriptor_status;
	fpo[port_id].tx_descriptor_status = dev->tx_descriptor_status;
	fpo[port_id].rxq.data = dev->data->rx_queues;
	fpo[port_id].txq.data = dev->data->tx_queues;

	return 0;
#endif

#ifdef QDMA_DPDK_20_11
	RTE_SET_USED(port_id);
	return 0;
#endif
}

