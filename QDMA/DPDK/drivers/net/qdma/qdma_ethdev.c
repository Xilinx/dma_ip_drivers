/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2021 Xilinx, Inc. All rights reserved.
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
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include "qdma.h"
#include "version.h"
#include "qdma_access_common.h"
#include "qdma_access_export.h"
#include "qdma_mbox.h"
#include "qdma_devops.h"

/* Poll for QDMA errors every 1 second */
#define QDMA_ERROR_POLL_FRQ (1000000)

#define PCI_CONFIG_BRIDGE_DEVICE              (6)
#define PCI_CONFIG_CLASS_CODE_SHIFT        (16)

#define MAX_PCIE_CAPABILITY    (48)

static void qdma_device_attributes_get(struct rte_eth_dev *dev);

/* Poll for any QDMA errors */
void qdma_check_errors(void *arg)
{
	struct qdma_pci_dev *qdma_dev;
	qdma_dev = ((struct rte_eth_dev *)arg)->data->dev_private;
	qdma_dev->hw_access->qdma_hw_error_process(arg);
	rte_eal_alarm_set(QDMA_ERROR_POLL_FRQ, qdma_check_errors, arg);
}

/*
 * The set of PCI devices this driver supports
 */
static struct rte_pci_id qdma_pci_id_tbl[] = {
#define RTE_PCI_DEV_ID_DECL(vend, dev) {RTE_PCI_DEVICE(vend, dev)},
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif

	/** Gen 1 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9011)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9111)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9211)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9311)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9014)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9114)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9214)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9314)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9018)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9118)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9218)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9318)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x901f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x911f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x921f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x931f)	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9021)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9121)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9221)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9321)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9024)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9124)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9224)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9324)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9028)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9128)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9228)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9328)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x902f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x912f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x922f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x932f)	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9031)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9131)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9231)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9331)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9034)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9134)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9234)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9334)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9038)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9138)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9238)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9338)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x903f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x913f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x923f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x933f)	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9041)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9141)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9241)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9341)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9044)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9144)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9244)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9344)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9048)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9148)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9248)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9348)	/** PF 3 */

	/** Versal */
	/** Gen 1 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb011)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb111)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb211)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb311)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb014)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb114)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb214)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb314)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb018)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb118)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb218)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb318)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb01f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb11f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb21f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb31f)	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb021)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb121)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb221)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb321)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb024)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb124)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb224)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb324)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb028)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb128)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb228)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb328)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb02f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb12f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb22f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb32f)	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb031)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb131)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb231)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb331)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb034)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb134)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb234)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb334)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb038)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb138)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb238)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb338)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb03f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb13f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb23f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb33f)	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb041)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb141)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb241)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb341)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb044)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb144)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb244)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb344)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb048)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb148)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb248)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb348)	/** PF 3 */

	{ .vendor_id = 0, /* sentinel */ },
};

static void qdma_device_attributes_get(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev;

	qdma_dev = (struct qdma_pci_dev *)dev->data->dev_private;
	qdma_dev->hw_access->qdma_get_device_attributes(dev,
			&qdma_dev->dev_cap);

	/* Check DPDK configured queues per port */
	if (qdma_dev->dev_cap.num_qs > RTE_MAX_QUEUES_PER_PORT)
		qdma_dev->dev_cap.num_qs = RTE_MAX_QUEUES_PER_PORT;

	PMD_DRV_LOG(INFO, "qmax = %d, mm %d, st %d.\n",
	qdma_dev->dev_cap.num_qs, qdma_dev->dev_cap.mm_en,
	qdma_dev->dev_cap.st_en);
}

static inline uint8_t pcie_find_cap(const struct rte_pci_device *pci_dev,
					uint8_t cap)
{
	uint8_t pcie_cap_pos = 0;
	uint8_t pcie_cap_id = 0;
	int ttl = MAX_PCIE_CAPABILITY;
	int ret;

	ret = rte_pci_read_config(pci_dev, &pcie_cap_pos, sizeof(uint8_t),
		PCI_CAPABILITY_LIST);
	if (ret < 0) {
		PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
		return 0;
	}

	while (ttl-- && pcie_cap_pos >= PCI_STD_HEADER_SIZEOF) {
		pcie_cap_pos &= ~3;

		ret = rte_pci_read_config(pci_dev,
			&pcie_cap_id, sizeof(uint8_t),
			(pcie_cap_pos + PCI_CAP_LIST_ID));
		if (ret < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
			return 0;
		}

		if (pcie_cap_id == 0xff)
			break;

		if (pcie_cap_id == cap)
			return pcie_cap_pos;

		ret = rte_pci_read_config(pci_dev,
			&pcie_cap_pos, sizeof(uint8_t),
			(pcie_cap_pos + PCI_CAP_LIST_NEXT));
		if (ret < 0) {
			PMD_DRV_LOG(ERR, "PCIe config space read failed..\n");
			return 0;
		}
	}

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

/* parse a sysfs file containing one integer value */
static int parse_sysfs_value(const char *filename, uint32_t *val)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	f = fopen(filename, "r");
	if (f == NULL) {
		PMD_DRV_LOG(ERR, "%s(): Failed to open sysfs file %s\n",
				__func__, filename);
		return -1;
	}

	if (fgets(buf, sizeof(buf), f) == NULL) {
		PMD_DRV_LOG(ERR, "%s(): Failed to read sysfs value %s\n",
			__func__, filename);
		fclose(f);
		return -1;
	}
	*val = (uint32_t)strtoul(buf, &end, 0);
	if ((buf[0] == '\0') || (end == NULL) || (*end != '\n')) {
		PMD_DRV_LOG(ERR, "%s(): Failed to parse sysfs value %s\n",
				__func__, filename);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

/* Split up a pci address into its constituent parts. */
static int parse_pci_addr_format(const char *buf,
		int bufsize, struct rte_pci_addr *addr)
{
	/* first split on ':' */
	union splitaddr {
		struct {
			char *domain;
			char *bus;
			char *devid;
			char *function;
		};
		/* last element-separator is "." not ":" */
		char *str[PCI_FMT_NVAL];
	} splitaddr;

	char *buf_copy = strndup(buf, bufsize);
	if (buf_copy == NULL) {
		PMD_DRV_LOG(ERR, "Failed to get pci address duplicate copy\n");
		return -1;
	}

	if (rte_strsplit(buf_copy, bufsize, splitaddr.str, PCI_FMT_NVAL, ':')
			!= PCI_FMT_NVAL - 1) {
		PMD_DRV_LOG(ERR, "Failed to split pci address string\n");
		goto error;
	}

	/* final split is on '.' between devid and function */
	splitaddr.function = strchr(splitaddr.devid, '.');
	if (splitaddr.function == NULL) {
		PMD_DRV_LOG(ERR, "Failed to split pci devid and function\n");
		goto error;
	}
	*splitaddr.function++ = '\0';

	/* now convert to int values */
	addr->domain = strtoul(splitaddr.domain, NULL, 16);
	addr->bus = strtoul(splitaddr.bus, NULL, 16);
	addr->devid = strtoul(splitaddr.devid, NULL, 16);
	addr->function = strtoul(splitaddr.function, NULL, 10);

	free(buf_copy); /* free the copy made with strdup */
	return 0;

error:
	free(buf_copy);
	return -1;
}

/* Get max pci bus number from the corresponding pci bridge device */
static int get_max_pci_bus_num(uint8_t start_bus, uint8_t *end_bus)
{
	char dirname[PATH_MAX];
	char filename[PATH_MAX];
	char cfgname[PATH_MAX];
	struct rte_pci_addr addr;
	struct dirent *dp;
	uint32_t pci_class_code;
	uint8_t sec_bus_num, sub_bus_num;
	DIR *dir;
	int ret, fd;

	/* Initialize end bus number to zero */
	*end_bus = 0;

	/* Open pci devices directory */
	dir = opendir(rte_pci_get_sysfs_path());
	if (dir == NULL) {
		PMD_DRV_LOG(ERR, "%s(): opendir failed\n",
			__func__);
		return -1;
	}

	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		/* Split pci address to get bus, devid and function numbers */
		if (parse_pci_addr_format(dp->d_name,
				sizeof(dp->d_name), &addr) != 0)
			continue;

		snprintf(dirname, sizeof(dirname), "%s/%s",
				rte_pci_get_sysfs_path(), dp->d_name);

		/* get class code */
		snprintf(filename, sizeof(filename), "%s/class", dirname);
		if (parse_sysfs_value(filename, &pci_class_code) < 0) {
			PMD_DRV_LOG(ERR, "Failed to get pci class code\n");
			goto error;
		}

		/* Get max pci number from pci bridge device */
		if ((((pci_class_code >> PCI_CONFIG_CLASS_CODE_SHIFT) & 0xFF) ==
				PCI_CONFIG_BRIDGE_DEVICE)) {
			snprintf(cfgname, sizeof(cfgname),
					"%s/config", dirname);
			fd = open(cfgname, O_RDWR);
			if (fd < 0) {
				PMD_DRV_LOG(ERR, "Failed to open %s\n",
					cfgname);
				goto error;
			}

			/* get secondary bus number */
			ret = pread(fd, &sec_bus_num, sizeof(uint8_t),
						PCI_SECONDARY_BUS);
			if (ret == -1) {
				PMD_DRV_LOG(ERR, "Failed to read secondary bus number\n");
				close(fd);
				goto error;
			}

			/* get subordinate bus number */
			ret = pread(fd, &sub_bus_num, sizeof(uint8_t),
						PCI_SUBORDINATE_BUS);
			if (ret == -1) {
				PMD_DRV_LOG(ERR, "Failed to read subordinate bus number\n");
				close(fd);
				goto error;
			}

			/* Get max bus number by checking if given bus number
			 * falls in between secondary and subordinate bus
			 * numbers of this pci bridge device.
			 */
			if ((start_bus >= sec_bus_num) &&
					(start_bus <= sub_bus_num)) {
				*end_bus = sub_bus_num;
				close(fd);
				closedir(dir);
				return 0;
			}

			close(fd);
		}
	}

error:
	closedir(dir);
	return -1;
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
int qdma_eth_dev_init(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *dma_priv;
	uint8_t *baseaddr;
	int i, idx, ret, qbase;
	struct rte_pci_device *pci_dev;
	uint16_t num_vfs;
	uint8_t max_pci_bus = 0;

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
	dev->data->mac_addrs = rte_zmalloc("qdma", RTE_ETHER_ADDR_LEN * 1, 0);
	if (dev->data->mac_addrs == NULL)
		return -ENOMEM;

	/* Copy some dummy Ethernet MAC address for QDMA device
	 * This will change in real NIC device...
	 */
	for (i = 0; i < RTE_ETHER_ADDR_LEN; ++i)
		dev->data->mac_addrs[0].addr_bytes[i] = 0x15 + i;

	/* Init system & device */
	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	dma_priv->is_vf = 0;
	dma_priv->is_master = 0;
	dma_priv->vf_online_count = 0;
	dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_TIMER;

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

	/*Assigning QDMA access layer function pointers based on the HW design*/
	dma_priv->hw_access = rte_zmalloc("hwaccess",
					sizeof(struct qdma_hw_access), 0);
	if (dma_priv->hw_access == NULL) {
		rte_free(dev->data->mac_addrs);
		return -ENOMEM;
	}
	idx = qdma_hw_access_init(dev, dma_priv->is_vf, dma_priv->hw_access);
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	idx = qdma_get_hw_version(dev);
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	idx = qdma_identify_bars(dev);
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	/* Store BAR address and length of AXI Master Lite BAR(user bar) */
	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			    pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		dma_priv->bar_addr[dma_priv->user_bar_idx] = baseaddr;
	}

	PMD_DRV_LOG(INFO, "QDMA device driver probe:");

	qdma_dev_ops_init(dev);

	/* Getting the device attributes from the Hardware */
	qdma_device_attributes_get(dev);

	if (dma_priv->dev_cap.cmpt_trig_count_timer) {
		/* Setting default Mode to
		 * RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT
		 */
		dma_priv->trigger_mode =
					RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT;
	} else{
		/* Setting default Mode to RTE_PMD_QDMA_TRIG_MODE_USER_TIMER */
		dma_priv->trigger_mode = RTE_PMD_QDMA_TRIG_MODE_USER_TIMER;
	}
	if (dma_priv->trigger_mode == RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT)
		dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER;

	/* Create master resource node for queue management on the given
	 * bus number. Node will be created only once per bus number.
	 */
	qbase = DEFAULT_QUEUE_BASE;

	ret = get_max_pci_bus_num(pci_dev->addr.bus, &max_pci_bus);
	if ((ret != QDMA_SUCCESS) && !max_pci_bus) {
		PMD_DRV_LOG(ERR, "Failed to get max pci bus number\n");
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}
	PMD_DRV_LOG(INFO, "PCI max bus number : 0x%x", max_pci_bus);

	ret = qdma_master_resource_create(pci_dev->addr.bus, max_pci_bus,
				qbase, dma_priv->dev_cap.num_qs,
				&dma_priv->dma_device_index);
	if (ret == -QDMA_ERR_NO_MEM) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -ENOMEM;
	}

	dma_priv->hw_access->qdma_get_function_number(dev,
			&dma_priv->func_id);
	PMD_DRV_LOG(INFO, "PF function ID: %d", dma_priv->func_id);

	/* CSR programming is done once per given board or bus number,
	 * done by the master PF
	 */
	if (ret == QDMA_SUCCESS) {
		RTE_LOG(INFO, PMD, "QDMA PMD VERSION: %s\n", QDMA_PMD_VERSION);
		dma_priv->hw_access->qdma_set_default_global_csr(dev);
		for (i = 0; i < dma_priv->dev_cap.mm_channel_max; i++) {
			if (dma_priv->dev_cap.mm_en) {
				/* Enable MM C2H Channel */
				dma_priv->hw_access->qdma_mm_channel_conf(dev,
							i, 1, 1);
				/* Enable MM H2C Channel */
				dma_priv->hw_access->qdma_mm_channel_conf(dev,
							i, 0, 1);
			} else {
				/* Disable MM C2H Channel */
				dma_priv->hw_access->qdma_mm_channel_conf(dev,
							i, 1, 0);
				/* Disable MM H2C Channel */
				dma_priv->hw_access->qdma_mm_channel_conf(dev,
							i, 0, 0);
			}
		}

		ret = dma_priv->hw_access->qdma_init_ctxt_memory(dev);
		if (ret < 0) {
			PMD_DRV_LOG(ERR,
				"%s: Failed to initialize ctxt memory, err = %d\n",
				__func__, ret);
			return -EINVAL;
		}

		dma_priv->hw_access->qdma_hw_error_enable(dev,
				dma_priv->hw_access->qdma_max_errors);
		if (ret < 0) {
			PMD_DRV_LOG(ERR,
				"%s: Failed to enable hw errors, err = %d\n",
				__func__, ret);
			return -EINVAL;
		}

		rte_eal_alarm_set(QDMA_ERROR_POLL_FRQ, qdma_check_errors,
							(void *)dev);
		dma_priv->is_master = 1;
	}

	/*
	 * Create an entry for the device in board list if not already
	 * created
	 */
	ret = qdma_dev_entry_create(dma_priv->dma_device_index,
				dma_priv->func_id);
	if ((ret != QDMA_SUCCESS) &&
		(ret != -QDMA_ERR_RM_DEV_EXISTS)) {
		PMD_DRV_LOG(ERR, "PF-%d(DEVFN) qdma_dev_entry_create failed: %d\n",
			    dma_priv->func_id, ret);
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -ENOMEM;
	}

	pcie_perf_enable(pci_dev);
	if (dma_priv->dev_cap.mailbox_en && pci_dev->max_vfs)
		qdma_mbox_init(dev);

	if (!dma_priv->reset_in_progress) {
		num_vfs = pci_dev->max_vfs;
		if (num_vfs) {
			dma_priv->vfinfo = rte_zmalloc("vfinfo",
				sizeof(struct qdma_vf_info) * num_vfs, 0);
			if (dma_priv->vfinfo == NULL)
				rte_panic("Cannot allocate memory for private VF info\n");

			/* Mark all VFs with invalid function id mapping*/
			for (i = 0; i < num_vfs; i++)
				dma_priv->vfinfo[i].func_id =
					QDMA_FUNC_ID_INVALID;
		}
	}

	dma_priv->reset_in_progress = 0;

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
int qdma_eth_dev_uninit(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct qdma_mbox_msg *m = NULL;
	int i, rv;

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	if (qdma_dev->vf_online_count) {
		for (i = 0; i < pci_dev->max_vfs; i++) {
			if (qdma_dev->vfinfo[i].func_id == QDMA_FUNC_ID_INVALID)
				continue;

			m = qdma_mbox_msg_alloc();
			if (!m)
				return -ENOMEM;

			if (!qdma_dev->reset_in_progress)
				qdma_mbox_compose_pf_offline(m->raw_data,
						qdma_dev->func_id,
						qdma_dev->vfinfo[i].func_id);
			else
				qdma_mbox_compose_vf_reset_message(m->raw_data,
						qdma_dev->func_id,
						qdma_dev->vfinfo[i].func_id);
			rv = qdma_mbox_msg_send(dev, m, 0);
			if (rv < 0)
				PMD_DRV_LOG(ERR, "Send bye failed from PF:%d to VF:%d\n",
					qdma_dev->func_id,
					qdma_dev->vfinfo[i].func_id);
		}

		PMD_DRV_LOG(INFO,
			"%s: Wait till all VFs shutdown for PF-%d(DEVFN)\n",
			__func__, qdma_dev->func_id);

		i = 0;
		while (i < SHUTDOWN_TIMEOUT) {
			if (!qdma_dev->vf_online_count) {
				PMD_DRV_LOG(INFO,
					"%s: VFs shutdown completed for PF-%d(DEVFN)\n",
					__func__, qdma_dev->func_id);
				break;
			}
			rte_delay_ms(1);
			i++;
		}

		if (i >= SHUTDOWN_TIMEOUT) {
			PMD_DRV_LOG(ERR, "%s: Failed VFs shutdown for PF-%d(DEVFN)\n",
				__func__, qdma_dev->func_id);
		}
	}

	if (qdma_dev->dev_configured)
		qdma_dev_close(dev);

	if (qdma_dev->dev_cap.mailbox_en && pci_dev->max_vfs)
		qdma_mbox_uninit(dev);

	/* cancel pending polls*/
	if (qdma_dev->is_master)
		rte_eal_alarm_cancel(qdma_check_errors, (void *)dev);

	/* Remove the device node from the board list */
	qdma_dev_entry_destroy(qdma_dev->dma_device_index,
			qdma_dev->func_id);
	qdma_master_resource_destroy(qdma_dev->dma_device_index);

	dev->dev_ops = NULL;
	dev->rx_pkt_burst = NULL;
	dev->tx_pkt_burst = NULL;
	dev->data->nb_rx_queues = 0;
	dev->data->nb_tx_queues = 0;

	if (!qdma_dev->reset_in_progress &&
			qdma_dev->vfinfo != NULL) {
		rte_free(qdma_dev->vfinfo);
		qdma_dev->vfinfo = NULL;
	}

	if (dev->data->mac_addrs != NULL) {
		rte_free(dev->data->mac_addrs);
		dev->data->mac_addrs = NULL;
	}

	if (qdma_dev->q_info != NULL) {
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
	}

	if (qdma_dev->hw_access != NULL) {
		rte_free(qdma_dev->hw_access);
		qdma_dev->hw_access = NULL;
	}
	return 0;
}

static int eth_qdma_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
				struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev,
						sizeof(struct qdma_pci_dev),
						qdma_eth_dev_init);
}

/* Detach a ethdev interface */
static int eth_qdma_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, qdma_eth_dev_uninit);
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
