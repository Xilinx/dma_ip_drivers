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
#include <rte_cycles.h>
#include <rte_alarm.h>
#include <unistd.h>
#include <string.h>
#include <linux/pci.h>

#include "qdma.h"
#include "version.h"
#include "qdma_access_common.h"
#include "qdma_mbox_protocol.h"
#include "qdma_mbox.h"
#include "qdma_devops.h"

static int eth_qdma_vf_dev_init(struct rte_eth_dev *dev);
static int eth_qdma_vf_dev_uninit(struct rte_eth_dev *dev);

/*
 * The set of PCI devices this driver supports
 */
static struct rte_pci_id qdma_vf_pci_id_tbl[] = {
#define RTE_PCI_DEV_ID_DECL(vend, dev) {RTE_PCI_DEVICE(vend, dev)},
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif

	/** Gen 1 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa011)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa111)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa211)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa311)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa014)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa114)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa214)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa314)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa018)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa118)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa218)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa318)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa01f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa11f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa21f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa31f)	/* VF on PF 3 */

	/** Gen 2 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa021)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa121)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa221)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa321)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa024)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa124)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa224)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa324)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa028)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa128)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa228)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa328)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa02f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa12f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa22f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa32f)	/* VF on PF 3 */

	/** Gen 3 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa031)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa131)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa231)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa331)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa034)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa134)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa234)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa334)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa038)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa138)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa238)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa338)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa03f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa13f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa23f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa33f)	/* VF on PF 3 */

	/** Gen 4 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa041)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa141)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa241)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa341)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa044)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa144)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa244)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa344)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa048)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa148)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa248)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xa348)	/* VF on PF 3 */

	/** Versal */
	/** Gen 1 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc011)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc111)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc211)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc311)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc014)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc114)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc214)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc314)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc018)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc118)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc218)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc318)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc01f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc11f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc21f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc31f)	/* VF on PF 3 */

	/** Gen 2 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc021)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc121)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc221)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc321)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc024)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc124)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc224)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc324)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc028)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc128)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc228)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc328)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc02f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc12f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc22f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc32f)	/* VF on PF 3 */

	/** Gen 3 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc031)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc131)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc231)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc331)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc034)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc134)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc234)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc334)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc038)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc138)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc238)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc338)	/* VF on PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc03f)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc13f)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc23f)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc33f)	/* VF on PF 3 */

	/** Gen 4 VF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc041)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc141)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc241)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc341)	/* VF on PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc044)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc144)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc244)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc344)	/* VF on PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc048)	/* VF on PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc148)	/* VF on PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc248)	/* VF on PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xc348)	/* VF on PF 3 */

	{ .vendor_id = 0, /* sentinel */ },
};

static int qdma_ethdev_online(struct rte_eth_dev *dev)
{
	int rv = 0;
	int qbase = -1;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();

	if (!m)
		return -ENOMEM;

	qmda_mbox_compose_vf_online(qdma_dev->func_id, 0, &qbase, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0)
		PMD_DRV_LOG(ERR, "%x, send hello failed %d.\n",
			    qdma_dev->func_id, rv);

	rv = qdma_mbox_vf_dev_info_get(m->raw_data,
				&qdma_dev->dev_cap,
				&qdma_dev->dma_device_index);

	if (rv < 0)
		PMD_DRV_LOG(ERR, "%x, failed to get dev info %d.\n",
				qdma_dev->func_id, rv);
	else {
		qdma_mbox_msg_free(m);
	}
	return rv;
}

static int qdma_ethdev_offline(struct rte_eth_dev *dev)
{
	int rv;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();

	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_offline(qdma_dev->func_id, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, 0);
	if (rv < 0)
		PMD_DRV_LOG(ERR, "%x, send bye failed %d.\n",
			    qdma_dev->func_id, rv);

	return rv;
}

static int qdma_vf_set_qrange(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m;
	int rv = 0;


	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_fmap_prog(qdma_dev->func_id,
					(uint16_t)qdma_dev->qsets_en,
					(int)qdma_dev->queue_base,
					m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		if (rv != -ENODEV)
			PMD_DRV_LOG(ERR, "%x set q range (fmap) failed %d.\n",
				    qdma_dev->func_id, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_set_qmax(struct rte_eth_dev *dev, int *qmax, int *qbase)
{
	struct qdma_mbox_msg *m;
	int rv = 0;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	m = qdma_mbox_msg_alloc();
	if (!m)
		return -ENOMEM;

	qdma_mbox_compose_vf_qreq(qdma_dev->func_id, (uint16_t)*qmax & 0xFFFF,
				  *qbase, m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		PMD_DRV_LOG(ERR, "%x set q max failed %d.\n",
			qdma_dev->func_id, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_qinfo_get(m->raw_data, qbase, (uint16_t *)qmax);
err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_rxq_context_setup(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t qid_hw;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct mbox_descq_conf descq_conf;
	int rv, bypass_desc_sz_idx;
	struct qdma_rx_queue *rxq;
	uint8_t cmpt_desc_fmt;
	enum mbox_cmpt_ctxt_type cmpt_ctxt_type = QDMA_MBOX_CMPT_CTXT_NONE;

	if (!m)
		return -ENOMEM;
	memset(&descq_conf, 0, sizeof(struct mbox_descq_conf));
	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	qid_hw =  qdma_dev->queue_base + rxq->queue_id;

	switch (rxq->cmpt_desc_len) {
	case RTE_PMD_QDMA_CMPT_DESC_LEN_8B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_16B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_16B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_32B:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_32B;
		break;
	case RTE_PMD_QDMA_CMPT_DESC_LEN_64B:
		if (!qdma_dev->dev_cap.cmpt_desc_64b) {
			PMD_DRV_LOG(ERR, "PF-%d(DEVFN) 64B is not supported in this "
				"mode:\n", qdma_dev->func_id);
			return -1;
		}
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_64B;
		break;
	default:
		cmpt_desc_fmt = CMPT_CNTXT_DESC_SIZE_8B;
		break;
	}
	descq_conf.ring_bs_addr = rxq->rx_mz->iova;
	descq_conf.en_bypass = rxq->en_bypass;
	descq_conf.irq_arm = 0;
	descq_conf.at = 0;
	descq_conf.wbk_en = 1;
	descq_conf.irq_en = 0;

	bypass_desc_sz_idx = qmda_get_desc_sz_idx(rxq->bypass_desc_sz);

	if (!rxq->st_mode) {/* mm c2h */
		descq_conf.desc_sz = SW_DESC_CNTXT_MEMORY_MAP_DMA;
		descq_conf.wbi_intvl_en = 1;
		descq_conf.wbi_chk = 1;
	} else {/* st c2h*/
		descq_conf.desc_sz = SW_DESC_CNTXT_C2H_STREAM_DMA;
		descq_conf.forced_en = 1;
		descq_conf.cmpt_ring_bs_addr = rxq->rx_cmpt_mz->iova;
		descq_conf.cmpt_desc_sz = cmpt_desc_fmt;
		descq_conf.triggermode = rxq->triggermode;

		descq_conf.cmpt_color = CMPT_DEFAULT_COLOR_BIT;
		descq_conf.cmpt_full_upd = 0;
		descq_conf.cnt_thres =
				qdma_dev->g_c2h_cnt_th[rxq->threshidx];
		descq_conf.timer_thres =
				qdma_dev->g_c2h_timer_cnt[rxq->timeridx];
		descq_conf.cmpt_ringsz =
				qdma_dev->g_ring_sz[rxq->cmpt_ringszidx] - 1;
		descq_conf.bufsz = qdma_dev->g_c2h_buf_sz[rxq->buffszidx];
		descq_conf.cmpt_int_en = 0;
		descq_conf.cmpl_stat_en = rxq->st_mode;
		descq_conf.pfch_en = rxq->en_prefetch;
		descq_conf.en_bypass_prefetch = rxq->en_bypass_prefetch;
		if (qdma_dev->dev_cap.cmpt_ovf_chk_dis)
			descq_conf.dis_overflow_check = rxq->dis_overflow_check;

		cmpt_ctxt_type = QDMA_MBOX_CMPT_WITH_ST;
	}

	if (rxq->en_bypass &&
			(rxq->bypass_desc_sz != 0))
		descq_conf.desc_sz = bypass_desc_sz_idx;

	descq_conf.func_id = rxq->func_id;
	descq_conf.ringsz = qdma_dev->g_ring_sz[rxq->ringszidx] - 1;

	qdma_mbox_compose_vf_qctxt_write(rxq->func_id, qid_hw, rxq->st_mode, 1,
					 cmpt_ctxt_type,
					 &descq_conf, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		PMD_DRV_LOG(ERR, "%x, qid_hw 0x%x, mbox failed %d.\n",
			qdma_dev->func_id, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_txq_context_setup(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct mbox_descq_conf descq_conf;
	int rv, bypass_desc_sz_idx;
	struct qdma_tx_queue *txq;
	uint32_t qid_hw;

	if (!m)
		return -ENOMEM;
	memset(&descq_conf, 0, sizeof(struct mbox_descq_conf));
	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
	qid_hw =  qdma_dev->queue_base + txq->queue_id;
	descq_conf.ring_bs_addr = txq->tx_mz->iova;
	descq_conf.en_bypass = txq->en_bypass;
	descq_conf.wbi_intvl_en = 1;
	descq_conf.wbi_chk = 1;
	descq_conf.wbk_en = 1;

	bypass_desc_sz_idx = qmda_get_desc_sz_idx(txq->bypass_desc_sz);

	if (!txq->st_mode) /* mm h2c */
		descq_conf.desc_sz = SW_DESC_CNTXT_MEMORY_MAP_DMA;
	else /* st h2c */
		descq_conf.desc_sz = SW_DESC_CNTXT_H2C_STREAM_DMA;
	descq_conf.func_id = txq->func_id;
	descq_conf.ringsz = qdma_dev->g_ring_sz[txq->ringszidx] - 1;

	if (txq->en_bypass &&
		(txq->bypass_desc_sz != 0))
		descq_conf.desc_sz = bypass_desc_sz_idx;

	qdma_mbox_compose_vf_qctxt_write(txq->func_id, qid_hw, txq->st_mode, 0,
					 QDMA_MBOX_CMPT_CTXT_NONE,
					 &descq_conf, m->raw_data);

	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		PMD_DRV_LOG(ERR, "%x, qid_hw 0x%x, mbox failed %d.\n",
			qdma_dev->func_id, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_queue_context_invalidate(struct rte_eth_dev *dev, uint32_t qid,
				  bool st, bool c2h)
{
	struct qdma_mbox_msg *m = qdma_mbox_msg_alloc();
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t qid_hw;
	int rv;
	enum mbox_cmpt_ctxt_type cmpt_ctxt_type = QDMA_MBOX_CMPT_CTXT_NONE;

	if (!m)
		return -ENOMEM;

	if (st && c2h)
		cmpt_ctxt_type = QDMA_MBOX_CMPT_WITH_ST;
	qid_hw = qdma_dev->queue_base + qid;
	qdma_mbox_compose_vf_qctxt_invalidate(qdma_dev->func_id, qid_hw,
					      st, c2h, cmpt_ctxt_type,
					      m->raw_data);
	rv = qdma_mbox_msg_send(dev, m, MBOX_OP_RSP_TIMEOUT);
	if (rv < 0) {
		if (rv != -ENODEV)
			PMD_DRV_LOG(INFO, "%x, qid_hw 0x%x mbox failed %d.\n",
				    qdma_dev->func_id, qid_hw, rv);
		goto err_out;
	}

	rv = qdma_mbox_vf_response_status(m->raw_data);

err_out:
	qdma_mbox_msg_free(m);
	return rv;
}

static int qdma_vf_dev_start(struct rte_eth_dev *dev)
{
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	uint32_t qid;
	int err;

	PMD_DRV_LOG(INFO, "qdma_dev_start: Starting\n");
	/* prepare descriptor rings for operation */
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++) {
		txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!txq->tx_deferred_start) {
			err = qdma_vf_dev_tx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}

	for (qid = 0; qid < dev->data->nb_rx_queues; qid++) {
		rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

		/*Deferred Queues should not start with dev_start*/
		if (!rxq->rx_deferred_start) {
			err = qdma_vf_dev_rx_queue_start(dev, qid);
			if (err != 0)
				return err;
		}
	}
	return 0;
}

static int qdma_vf_dev_link_update(struct rte_eth_dev *dev,
					__rte_unused int wait_to_complete)
{
	dev->data->dev_link.link_status = ETH_LINK_UP;
	dev->data->dev_link.link_duplex = ETH_LINK_FULL_DUPLEX;
	dev->data->dev_link.link_speed = ETH_SPEED_NUM_100G;

	PMD_DRV_LOG(INFO, "Link update done\n");

	return 0;
}

static int qdma_vf_dev_infos_get(__rte_unused struct rte_eth_dev *dev,
					struct rte_eth_dev_info *dev_info)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	dev_info->max_rx_queues = qdma_dev->dev_cap.num_qs;
	dev_info->max_tx_queues = qdma_dev->dev_cap.num_qs;

	dev_info->min_rx_bufsize = QDMA_MIN_RXBUFF_SIZE;
	dev_info->max_rx_pktlen = DMA_BRAM_SIZE;
	dev_info->max_mac_addrs = 1;

	return 0;
}

static int qdma_vf_dev_stop(struct rte_eth_dev *dev)
{
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
#endif
	uint32_t qid;

	/* reset driver's internal queue structures to default values */
	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Stop H2C & C2H queues",
			qdma_dev->func_id);
	for (qid = 0; qid < dev->data->nb_tx_queues; qid++)
		qdma_vf_dev_tx_queue_stop(dev, qid);
	for (qid = 0; qid < dev->data->nb_rx_queues; qid++)
		qdma_vf_dev_rx_queue_stop(dev, qid);

	return 0;
}

int qdma_vf_dev_close(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;
	struct qdma_rx_queue *rxq;
	struct qdma_cmpt_queue *cmptq;
	uint32_t qid;

	PMD_DRV_LOG(INFO, "Closing all queues\n");

	if (dev->data->dev_started)
		qdma_vf_dev_stop(dev);

	/* iterate over rx queues */
	for (qid = 0; qid < dev->data->nb_rx_queues; ++qid) {
		rxq = dev->data->rx_queues[qid];
		if (rxq != NULL) {
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Remove C2H queue: %d",
							qdma_dev->func_id, qid);

			qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_C2H);

			if (rxq->st_mode)
				qdma_dev_notify_qdel(rxq->dev, rxq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_CMPT);

			if (rxq->sw_ring)
				rte_free(rxq->sw_ring);

			if (rxq->st_mode) { /** if ST-mode **/
				if (rxq->rx_cmpt_mz)
					rte_memzone_free(rxq->rx_cmpt_mz);
			}

			if (rxq->rx_mz)
				rte_memzone_free(rxq->rx_mz);
			rte_free(rxq);
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) C2H queue %d removed",
							qdma_dev->func_id, qid);
		}
	}

	/* iterate over tx queues */
	for (qid = 0; qid < dev->data->nb_tx_queues; ++qid) {
		txq = dev->data->tx_queues[qid];
		if (txq != NULL) {
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Remove H2C queue: %d",
							qdma_dev->func_id, qid);

			qdma_dev_notify_qdel(txq->dev, txq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_H2C);
			if (txq->sw_ring)
				rte_free(txq->sw_ring);
			if (txq->tx_mz)
				rte_memzone_free(txq->tx_mz);
			rte_free(txq);
			PMD_DRV_LOG(INFO, "VF-%d(DEVFN) H2C queue %d removed",
							qdma_dev->func_id, qid);
		}
	}
	if (qdma_dev->dev_cap.mm_cmpt_en) {
		/* iterate over cmpt queues */
		for (qid = 0; qid < qdma_dev->qsets_en; ++qid) {
			cmptq = qdma_dev->cmpt_queues[qid];
			if (cmptq != NULL) {
				PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Remove CMPT queue: %d",
						qdma_dev->func_id, qid);
				qdma_dev_notify_qdel(cmptq->dev,
						cmptq->queue_id +
						qdma_dev->queue_base,
						QDMA_DEV_Q_TYPE_CMPT);
				if (cmptq->cmpt_mz)
					rte_memzone_free(cmptq->cmpt_mz);
				rte_free(cmptq);
				PMD_DRV_LOG(INFO, "VF-%d(DEVFN) CMPT queue %d removed",
						qdma_dev->func_id, qid);
			}
		}

		if (qdma_dev->cmpt_queues != NULL) {
			rte_free(qdma_dev->cmpt_queues);
			qdma_dev->cmpt_queues = NULL;
		}
	}

	qdma_dev->qsets_en = 0;
	qdma_set_qmax(dev, (int *)&qdma_dev->qsets_en,
		      (int *)&qdma_dev->queue_base);
	qdma_dev->init_q_range = 0;
	rte_free(qdma_dev->q_info);
	qdma_dev->q_info = NULL;
	qdma_dev->dev_configured = 0;

	return 0;
}

static int qdma_vf_dev_reset(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t i = 0;
	int ret;

	PMD_DRV_LOG(INFO, "%s: Reset VF-%d(DEVFN)",
			__func__, qdma_dev->func_id);

	ret = eth_qdma_vf_dev_uninit(dev);
	if (ret)
		return ret;

	if (qdma_dev->reset_state == RESET_STATE_IDLE) {
		ret = eth_qdma_vf_dev_init(dev);
	} else {

		/* VFs do not stop mbox and start waiting for a
		 * "PF_RESET_DONE" mailbox message from PF
		 * for a maximum of 60 secs
		 */
		PMD_DRV_LOG(INFO,
			"%s: Waiting for reset done message from PF",
			__func__);
		while (i < RESET_TIMEOUT) {
			if (qdma_dev->reset_state ==
					RESET_STATE_RECV_PF_RESET_DONE) {
				qdma_mbox_uninit(dev);

				ret = eth_qdma_vf_dev_init(dev);
				return ret;
			}

			rte_delay_ms(1);
			i++;
		}
	}

	if (i >= RESET_TIMEOUT) {
		PMD_DRV_LOG(ERR, "%s: Reset failed for VF-%d(DEVFN)\n",
			__func__, qdma_dev->func_id);
		return -ETIMEDOUT;
	}

	return ret;
}

static int qdma_vf_dev_configure(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	int32_t ret = 0, queue_base = -1;
	uint32_t qid = 0;

	/** FMAP configuration **/
	qdma_dev->qsets_en = RTE_MAX(dev->data->nb_rx_queues,
					dev->data->nb_tx_queues);

	if (qdma_dev->qsets_en > qdma_dev->dev_cap.num_qs) {
		PMD_DRV_LOG(INFO, "VF-%d(DEVFN) Error: Number of Queues to be "
				"configured are greater than the queues "
				"supported by the hardware\n",
				qdma_dev->func_id);
		qdma_dev->qsets_en = 0;
		return -1;
	}

	/* Request queue base from the resource manager */
	ret = qdma_set_qmax(dev, (int *)&qdma_dev->qsets_en,
			    (int *)&queue_base);
	if (ret != QDMA_SUCCESS) {
		PMD_DRV_LOG(ERR, "VF-%d(DEVFN) queue allocation failed: %d\n",
			qdma_dev->func_id, ret);
		return -1;
	}
	qdma_dev->queue_base = queue_base;

	qdma_dev->q_info = rte_zmalloc("qinfo", sizeof(struct queue_info) *
						qdma_dev->qsets_en, 0);
	if (qdma_dev->q_info == NULL) {
		PMD_DRV_LOG(INFO, "VF-%d fail to allocate queue info memory\n",
						qdma_dev->func_id);
		return (-ENOMEM);
	}

	/* Reserve memory for cmptq ring pointers
	 * Max completion queues can be maximum of rx and tx queues.
	 */
	qdma_dev->cmpt_queues = rte_zmalloc("cmpt_queues",
					    sizeof(qdma_dev->cmpt_queues[0]) *
						qdma_dev->qsets_en,
						RTE_CACHE_LINE_SIZE);
	if (qdma_dev->cmpt_queues == NULL) {
		PMD_DRV_LOG(ERR, "VF-%d(DEVFN) cmpt ring pointers memory "
				"allocation failed:\n", qdma_dev->func_id);
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
		return -(ENOMEM);
	}

	/* Initialize queue_modes to all 1's ( i.e. Streaming) */
	for (qid = 0 ; qid < qdma_dev->qsets_en; qid++)
		qdma_dev->q_info[qid].queue_mode = RTE_PMD_QDMA_STREAMING_MODE;

	for (qid = 0 ; qid < dev->data->nb_rx_queues; qid++) {
		qdma_dev->q_info[qid].cmpt_desc_sz = qdma_dev->cmpt_desc_len;
		qdma_dev->q_info[qid].rx_bypass_mode =
						qdma_dev->c2h_bypass_mode;
		qdma_dev->q_info[qid].trigger_mode = qdma_dev->trigger_mode;
		qdma_dev->q_info[qid].timer_count =
					qdma_dev->timer_count;
	}

	for (qid = 0 ; qid < dev->data->nb_tx_queues; qid++)
		qdma_dev->q_info[qid].tx_bypass_mode =
						qdma_dev->h2c_bypass_mode;

	ret = qdma_vf_set_qrange(dev);
	if (ret < 0) {
		PMD_DRV_LOG(ERR, "FMAP programming failed\n");
		rte_free(qdma_dev->q_info);
		qdma_dev->q_info = NULL;
		rte_free(qdma_dev->cmpt_queues);
		qdma_dev->cmpt_queues = NULL;
		return ret;
	}

	qdma_dev->dev_configured = 1;

	return ret;
}

int qdma_vf_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_tx_queue *txq;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];
	qdma_reset_tx_queue(txq);

	if (qdma_txq_context_setup(dev, qid) < 0)
		return -1;

	txq->q_pidx_info.pidx = 0;
	qdma_dev->hw_access->qdma_queue_pidx_update(dev, qdma_dev->is_vf,
			qid, 0, &txq->q_pidx_info);

	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	txq->status = RTE_ETH_QUEUE_STATE_STARTED;

	return 0;
}

int qdma_vf_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qdma_rx_queue *rxq;
	int err;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];
	qdma_reset_rx_queue(rxq);

	err = qdma_init_rx_queue(rxq);
	if (err != 0)
		return err;
	if (qdma_rxq_context_setup(dev, qid) < 0) {
		PMD_DRV_LOG(ERR, "context_setup for qid - %u failed", qid);

		return -1;
	}

	if (rxq->st_mode) {
		rxq->cmpt_cidx_info.counter_idx = rxq->threshidx;
		rxq->cmpt_cidx_info.timer_idx = rxq->timeridx;
		rxq->cmpt_cidx_info.trig_mode = rxq->triggermode;
		rxq->cmpt_cidx_info.wrb_en = 1;
		qdma_dev->hw_access->qdma_queue_cmpt_cidx_update(dev, 1,
				qid, &rxq->cmpt_cidx_info);

		rxq->q_pidx_info.pidx = (rxq->nb_rx_desc - 2);
		qdma_dev->hw_access->qdma_queue_pidx_update(dev, 1,
				qid, 1, &rxq->q_pidx_info);
	}

	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STARTED;
	rxq->status = RTE_ETH_QUEUE_STATE_STARTED;
	return 0;
}

int qdma_vf_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_rx_queue *rxq;
	int i = 0, cnt = 0;

	rxq = (struct qdma_rx_queue *)dev->data->rx_queues[qid];

	rxq->status = RTE_ETH_QUEUE_STATE_STOPPED;

	/* Wait for queue to recv all packets. */
	if (rxq->st_mode) {  /** ST-mode **/
		while (rxq->wb_status->pidx != rxq->cmpt_cidx_info.wrb_cidx) {
			usleep(10);
			if (cnt++ > 10000)
				break;
		}
	} else { /* MM mode */
		while (rxq->wb_status->cidx != rxq->q_pidx_info.pidx) {
			usleep(10);
			if (cnt++ > 10000)
				break;
		}
	}

	qdma_queue_context_invalidate(dev, qid, rxq->st_mode, 1);

	if (rxq->st_mode) {  /** ST-mode **/
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id = %d, mbuf_avail_count = "
				"%d, mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS

		for (i = 0; i < rxq->nb_rx_desc - 1; i++) {
			rte_pktmbuf_free(rxq->sw_ring[i]);
			rxq->sw_ring[i] = NULL;
		}
#ifdef DUMP_MEMPOOL_USAGE_STATS
		PMD_DRV_LOG(INFO, "%s(): %d: queue id = %d, mbuf_avail_count = "
				"%d, mbuf_in_use_count = %d",
				__func__, __LINE__, rxq->queue_id,
				rte_mempool_avail_count(rxq->mb_pool),
				rte_mempool_in_use_count(rxq->mb_pool));
#endif //DUMP_MEMPOOL_USAGE_STATS
	}

	qdma_reset_rx_queue(rxq);
	dev->data->rx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;
	return 0;
}


int qdma_vf_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid)
{
	struct qdma_tx_queue *txq;
	int i = 0, cnt = 0;

	txq = (struct qdma_tx_queue *)dev->data->tx_queues[qid];

	txq->status = RTE_ETH_QUEUE_STATE_STOPPED;
	/* Wait for TXQ to send out all packets. */
	while (txq->wb_status->cidx != txq->q_pidx_info.pidx) {
		usleep(10);
		if (cnt++ > 10000)
			break;
	}

	qdma_queue_context_invalidate(dev, qid, txq->st_mode, 0);

	/* Free mbufs if any pending in the ring */
	for (i = 0; i < txq->nb_tx_desc; i++) {
		rte_pktmbuf_free(txq->sw_ring[i]);
		txq->sw_ring[i] = NULL;
	}
	qdma_reset_tx_queue(txq);
	dev->data->tx_queue_state[qid] = RTE_ETH_QUEUE_STATE_STOPPED;
	return 0;
}

static struct eth_dev_ops qdma_vf_eth_dev_ops = {
	.dev_configure        = qdma_vf_dev_configure,
	.dev_infos_get        = qdma_vf_dev_infos_get,
	.dev_start            = qdma_vf_dev_start,
	.dev_stop             = qdma_vf_dev_stop,
	.dev_close            = qdma_vf_dev_close,
	.dev_reset            = qdma_vf_dev_reset,
	.link_update          = qdma_vf_dev_link_update,
	.rx_queue_setup       = qdma_dev_rx_queue_setup,
	.tx_queue_setup       = qdma_dev_tx_queue_setup,
	.rx_queue_release     = qdma_dev_rx_queue_release,
	.tx_queue_release     = qdma_dev_tx_queue_release,
	.rx_queue_start       = qdma_vf_dev_rx_queue_start,
	.rx_queue_stop        = qdma_vf_dev_rx_queue_stop,
	.tx_queue_start       = qdma_vf_dev_tx_queue_start,
	.tx_queue_stop        = qdma_vf_dev_tx_queue_stop,
	.stats_get            = qdma_dev_stats_get,
};

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
static int eth_qdma_vf_dev_init(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *dma_priv;
	uint8_t *baseaddr;
	int i, idx;
	static bool once = true;
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
		dev->dev_ops = &qdma_vf_eth_dev_ops;
		return 0;
	}

	if (once) {
		RTE_LOG(INFO, PMD, "QDMA PMD VERSION: %s\n", QDMA_PMD_VERSION);
		once = false;
	}

	/* allocate space for a single Ethernet MAC address */
	dev->data->mac_addrs = rte_zmalloc("qdma_vf",
			RTE_ETHER_ADDR_LEN * 1, 0);
	if (dev->data->mac_addrs == NULL)
		return -ENOMEM;

	/* Copy some dummy Ethernet MAC address for XDMA device
	 * This will change in real NIC device...
	 */
	for (i = 0; i < RTE_ETHER_ADDR_LEN; ++i)
		dev->data->mac_addrs[0].addr_bytes[i] = 0x15 + i;

	/* Init system & device */
	dma_priv = (struct qdma_pci_dev *)dev->data->dev_private;
	dma_priv->func_id = 0;
	dma_priv->is_vf = 1;
	dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_TIMER;

	dma_priv->en_desc_prefetch = 0;
	dma_priv->cmpt_desc_len = DEFAULT_QDMA_CMPT_DESC_LEN;
	dma_priv->c2h_bypass_mode = RTE_PMD_QDMA_RX_BYPASS_NONE;
	dma_priv->h2c_bypass_mode = 0;

	dev->dev_ops = &qdma_vf_eth_dev_ops;
	dev->rx_pkt_burst = &qdma_recv_pkts;
	dev->tx_pkt_burst = &qdma_xmit_pkts;

	dma_priv->config_bar_idx = DEFAULT_VF_CONFIG_BAR;
	dma_priv->bypass_bar_idx = BAR_ID_INVALID;
	dma_priv->user_bar_idx = BAR_ID_INVALID;

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
	dma_priv->hw_access = rte_zmalloc("vf_hwaccess",
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

	/* Store BAR address and length of AXI Master Lite BAR(user bar)*/
	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			     pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		dma_priv->bar_addr[dma_priv->user_bar_idx] = baseaddr;
	}

	if (dma_priv->ip_type == QDMA_VERSAL_HARD_IP)
		dma_priv->dev_cap.mailbox_intr = 0;
	else
		dma_priv->dev_cap.mailbox_intr = 1;

	qdma_mbox_init(dev);
	idx = qdma_ethdev_online(dev);
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	if (dma_priv->dev_cap.cmpt_trig_count_timer) {
		/* Setting default Mode to
		 * RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT
		 */
		dma_priv->trigger_mode =
				RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT;
	} else {
		/* Setting default Mode to RTE_PMD_QDMA_TRIG_MODE_USER_TIMER */
		dma_priv->trigger_mode = RTE_PMD_QDMA_TRIG_MODE_USER_TIMER;
	}
	if (dma_priv->trigger_mode == RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT)
		dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER;

	dma_priv->reset_state = RESET_STATE_IDLE;

	PMD_DRV_LOG(INFO, "VF-%d(DEVFN) QDMA device driver probe:",
				dma_priv->func_id);

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
static int eth_qdma_vf_dev_uninit(struct rte_eth_dev *dev)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	if (qdma_dev->dev_configured)
		qdma_vf_dev_close(dev);

	qdma_ethdev_offline(dev);

	if (qdma_dev->reset_state != RESET_STATE_RECV_PF_RESET_REQ)
		qdma_mbox_uninit(dev);

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

static int eth_qdma_vf_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
					struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev,
						sizeof(struct qdma_pci_dev),
						eth_qdma_vf_dev_init);
}

/* Detach a ethdev interface */
static int eth_qdma_vf_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, eth_qdma_vf_dev_uninit);
}

static struct rte_pci_driver rte_qdma_vf_pmd = {
	.id_table = qdma_vf_pci_id_tbl,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
	.probe = eth_qdma_vf_pci_probe,
	.remove = eth_qdma_vf_pci_remove,
};

bool
is_vf_device_supported(struct rte_eth_dev *dev)
{
	if (strcmp(dev->device->driver->name, rte_qdma_vf_pmd.driver.name))
		return false;

	return true;
}

RTE_PMD_REGISTER_PCI(net_qdma_vf, rte_qdma_vf_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_qdma_vf, qdma_vf_pci_id_tbl);
