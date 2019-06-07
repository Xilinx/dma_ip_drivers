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
#include <rte_alarm.h>
#include <rte_cycles.h>
#include <unistd.h>
#include <string.h>

#include "qdma.h"
#include "version.h"
#include "qdma_access.h"
#include "qdma_access_export.h"
#include "qdma_mbox.h"

/* Poll for QDMA errors every 1 second */
#define QDMA_ERROR_POLL_FRQ (1000000)

static void qdma_device_attributes_get(struct rte_eth_dev *dev);

/* Poll for any QDMA errors */
static void qdma_check_errors(void *arg)
{
	qdma_error_process(arg);
	rte_eal_alarm_set(QDMA_ERROR_POLL_FRQ, qdma_check_errors, arg);
}

/*
 * The set of PCI devices this driver supports
 */
static struct rte_pci_id qdma_pci_id_tbl[] = {
#define RTE_PCI_DEV_ID_DECL_XNIC(vend, dev) {RTE_PCI_DEVICE(vend, dev)},
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif

	/** Gen 1 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9011)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9111)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9211)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9311)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9014)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9114)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9214)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9314)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9018)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9118)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9218)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9318)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x901f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x911f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x921f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x931f)	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9021)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9121)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9221)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9321)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9024)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9124)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9224)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9324)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9028)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9128)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9228)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9328)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x902f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x912f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x922f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x932f)	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9031)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9131)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9231)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9331)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9034)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9134)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9234)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9334)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9038)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9138)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9238)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9338)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x903f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x913f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x923f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x933f)	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9041)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9141)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9241)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9341)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9044)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9144)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9244)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9344)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9048)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9148)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9248)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL_XNIC(PCI_VENDOR_ID_XILINX, 0x9348)	/** PF 3 */

	{ .vendor_id = 0, /* sentinel */ },
};

static void qdma_device_attributes_get(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev;
	struct qdma_dev_attributes qdma_attrib;

	qdma_dev = (struct qdma_pci_dev *)dev->data->dev_private;
	qdma_get_device_attributes(dev, &qdma_attrib);

	qdma_dev->dev_cap.num_qs = qdma_attrib.num_qs;
	/* Check DPDK configured queues per port */
	if (qdma_dev->dev_cap.num_qs > RTE_MAX_QUEUES_PER_PORT)
		qdma_dev->dev_cap.num_qs = RTE_MAX_QUEUES_PER_PORT;

	qdma_dev->dev_cap.mm_en = qdma_attrib.mm_en ? 1 : 0;
	qdma_dev->dev_cap.st_en = qdma_attrib.st_en ? 1 : 0;
	qdma_dev->dev_cap.mm_cmpt_en = qdma_attrib.mm_cmpt_en;
	qdma_dev->dev_cap.mailbox_en = qdma_attrib.mailbox_en;

	PMD_DRV_LOG(INFO, "qmax = %d, mm %d, st %d.\n",
	qdma_dev->dev_cap.num_qs, qdma_dev->dev_cap.mm_en,
	qdma_dev->dev_cap.st_en);
}

static inline uint8_t pcie_find_cap(const struct rte_pci_device *pci_dev,
					uint8_t cap)
{
	uint8_t pcie_cap_pos = 0;
	uint8_t pcie_cap_id = 0;

	if (rte_pci_read_config(pci_dev, &pcie_cap_pos, sizeof(uint8_t),
				PCI_CAPABILITY_LIST) < 0) {
		PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
		return 0;
	}

	if (pcie_cap_pos < 0x40)
		return 0;

	while (pcie_cap_pos >= 0x40) {
		pcie_cap_pos &= ~3;

		if (rte_pci_read_config(pci_dev, &pcie_cap_id, sizeof(uint8_t),
					pcie_cap_pos + PCI_CAP_LIST_ID) < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
			goto ret;
		}

		if (pcie_cap_id == 0xff)
			break;

		if (pcie_cap_id == cap)
			return pcie_cap_pos;

		if (rte_pci_read_config(pci_dev, &pcie_cap_pos, sizeof(uint8_t),
					pcie_cap_pos + PCI_CAP_LIST_NEXT) < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
			goto ret;
		}
	}

ret:
	return 0;
}

static void pcie_perf_enable(const struct rte_pci_device *pci_dev)
{
	uint16_t value;
	uint8_t pcie_cap_pos = pcie_find_cap(pci_dev, PCI_CAP_ID_EXP);

	if (!pcie_cap_pos)
		return;

	if (pcie_cap_pos > 0) {
		if (rte_pci_read_config(pci_dev, &value, sizeof(uint16_t),
					 pcie_cap_pos + PCI_EXP_DEVCTL) < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
			return;
		}

		value |= (PCI_EXP_DEVCTL_EXT_TAG | PCI_EXP_DEVCTL_RELAX_EN);

		if (rte_pci_write_config(pci_dev, &value, sizeof(uint16_t),
					   pcie_cap_pos + PCI_EXP_DEVCTL) < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space write failed..\n");
			return;
		}
	}
}


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
static int eth_qdma_dev_init(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *dma_priv;
	uint8_t *baseaddr;
	int i, idx, ret;
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
		qdma_dev_ops_init(dev);
		return 0;
	}


	/* allocate space for a single Ethernet MAC address */
	dev->data->mac_addrs = rte_zmalloc("qdma", ETHER_ADDR_LEN * 1, 0);
	if (dev->data->mac_addrs == NULL)
		return -ENOMEM;

	/* Copy some dummy Ethernet MAC address for QDMA device
	 * This will change in real NIC device...
	 */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		dev->data->mac_addrs[0].addr_bytes[i] = 0x15 + i;

	/* Init system & device */
	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	dma_priv->pf = PCI_DEVFN(pci_dev->addr.devid, pci_dev->addr.function);
	dma_priv->is_vf = 0;
	dma_priv->is_master = 0;
	dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_TIMER;

	/* Setting default Mode to RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT */
	dma_priv->trigger_mode = RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT;
	if (dma_priv->trigger_mode == RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT)
		dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER;

	dma_priv->en_desc_prefetch = 0; //Keep prefetch default to 0
	dma_priv->cmpt_desc_len = DEFAULT_QDMA_CMPT_DESC_LEN;
	dma_priv->c2h_bypass_mode = RTE_PMD_QDMA_RX_BYPASS_NONE;
	dma_priv->h2c_bypass_mode = 0;

	dma_priv->config_bar_idx = DEFAULT_PF_CONFIG_BAR;
	dma_priv->bypass_bar_idx = BAR_ID_INVALID;
	dma_priv->user_bar_idx = BAR_ID_INVALID;

	/* Check and handle device devargs*/
	if (qdma_check_kvargs(dev->device->devargs, dma_priv)) {
		PMD_DRV_LOG(INFO, "devargs failed\n");
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	/* Store BAR address and length of Config BAR */
	baseaddr = (uint8_t *)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr;
	dma_priv->bar_addr[dma_priv->config_bar_idx] = baseaddr;

	idx = qdma_identify_bars(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	/* Store BAR address and length of User BAR */
	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			    pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		dma_priv->bar_addr[dma_priv->user_bar_idx] = baseaddr;
	}

	PMD_DRV_LOG(INFO, "QDMA device driver probe:");

	idx = qdma_get_hw_version(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	qdma_dev_ops_init(dev);

	/* Getting the device attributes from the Hardware */
	qdma_device_attributes_get(dev);
	if (dma_priv->dev_cap.mm_en) {
		/* Enable MM C2H Channel */
		qdma_mm_channel_enable(dev, 0, 1);
		/* Enable MM H2C Channel */
		qdma_mm_channel_enable(dev, 0, 0);
	} else {
		/* Disable MM C2H Channel */
		qdma_mm_channel_disable(dev, 0, 1);
		/* Disable MM H2C Channel */
		qdma_mm_channel_disable(dev, 0, 0);
	}

	/* Create master resource node for queue management on the given
	 * bus number. Node will be created only once per bus number.
	 */
	ret = qdma_master_resource_create(pci_dev->addr.bus, DEFAULT_QUEUE_BASE,
				    QDMA_QUEUES_NUM_MAX);
	if (ret == -QDMA_RESOURCE_MGMT_MEMALLOC_FAIL) {
		rte_free(dev->data->mac_addrs);
		return -ENOMEM;
	}

	/* CSR programming is done once per given board or bus number,
	 * done by the master PF
	 */
	if (ret == QDMA_RESOURCE_MGMT_SUCCESS) {
		RTE_LOG(INFO, PMD, "QDMA PMD VERSION: %s\n", QDMA_PMD_VERSION);
		qdma_set_default_global_csr(dev);

		qdma_error_enable(dev, QDMA_ERRS_ALL);

		rte_eal_alarm_set(QDMA_ERROR_POLL_FRQ, qdma_check_errors,
							(void *)dev);
		dma_priv->is_master = 1;
	}

	/*
	 * Create an entry for the device in board list if not already
	 * created
	 */
	ret = qdma_dev_entry_create(pci_dev->addr.bus, dma_priv->pf);
	if ((ret != QDMA_RESOURCE_MGMT_SUCCESS) &&
		(ret != -QDMA_DEV_ALREADY_EXISTS)) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) qdma_dev_entry_create failed: %d\n",
			    dma_priv->pf, ret);
		return -ENOMEM;
	}


	pcie_perf_enable(pci_dev);
	if (dma_priv->dev_cap.mailbox_en && pci_dev->max_vfs)
		qdma_mbox_init(dev);


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
static int eth_qdma_dev_uninit(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	if (qdma_dev->dev_cap.mailbox_en && pci_dev->max_vfs)
		qdma_mbox_uninit(dev);
	/* Remove the device node from the board list */
	qdma_dev_entry_destroy(pci_dev->addr.bus, qdma_dev->pf);

	/* cancel pending polls*/
	if (qdma_dev->is_master)
		rte_eal_alarm_cancel(qdma_check_errors, (void *)dev);

	/* Disable MM C2H Channel */
	qdma_mm_channel_disable(dev, 0, 1);
	/* Disable MM H2C Channel */
	qdma_mm_channel_disable(dev, 0, 0);

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

static int eth_qdma_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
				struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev,
						sizeof(struct qdma_pci_dev),
						eth_qdma_dev_init);
}

/* Detach a ethdev interface */
static int eth_qdma_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, eth_qdma_dev_uninit);
}

static struct rte_pci_driver rte_qdma_pmd = {
	.id_table = qdma_pci_id_tbl,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
	.probe = eth_qdma_pci_probe,
	.remove = eth_qdma_pci_remove,
};

bool
is_pf_device_supported(struct rte_eth_dev *dev)
{
	if (strcmp(dev->device->driver->name, rte_qdma_pmd.driver.name))
		return false;

	return true;
}

bool is_qdma_supported(struct rte_eth_dev *dev)
{
	bool is_pf, is_vf;

	is_pf = is_pf_device_supported(dev);
	is_vf = is_vf_device_supported(dev);

	if (!is_pf && !is_vf)
		return false;

	return true;
}

RTE_PMD_REGISTER_PCI(net_qdma, rte_qdma_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_qdma, qdma_pci_id_tbl);
