/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2018 Xilinx, Inc. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "qdma_regs.h"
#include "version.h"

static void qdma_device_attributes_get(struct qdma_pci_dev *qdma_dev);

uint32_t g_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {
1024, 256, 512, 768, 1280, 1536, 2048, 2560,
3072, 4096, 5120, 6144, 8192, 1024, 1024, 1024
};

uint32_t g_c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {
64, 2, 4, 8, 12, 16, 24, 32, 40, 48, 96, 128, 192, 256, 384, 512
};

uint32_t g_c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {
4096, 256, 512, 1024, 2048, 8192, 9018, 16384, 3968, 4096, 4096,
4096, 4096, 4096, 4096, 4096
};

uint32_t g_c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ] = {
0, 1, 2, 3, 4, 5, 7, 8, 10, 12, 15, 20, 30, 50, 100, 200
};

static int qdma_vf_hi(struct rte_eth_dev *dev, uint16_t vf_funid,
			struct qdma_mbox_data *recv_msg)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint32_t qbase, qmax, i;
	struct rte_pci_device *pci_dev;
	int ret = 0;
	uint64_t sz;

	PMD_DRV_LOG(INFO, "PF-%d received HI msg from VF global function-id %d",
			PCI_FUNC(qdma_dev->pf), vf_funid);
	pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	/* Mapping internal function id index to external function id */
	for (i = 0; i < pci_dev->max_vfs; i++) {
		if (qdma_dev->vfinfo[i].func_id == QDMA_FUNC_ID_INVALID) {
			qdma_dev->vfinfo[i].func_id = vf_funid;
			break;
		}
	}

	if (i == pci_dev->max_vfs) {
		PMD_DRV_LOG(INFO, "PF-%d  failed to create function "
				"id mapping VF- func_id%d",
				PCI_FUNC(qdma_dev->pf), vf_funid);
		return -1;
	}

	PMD_DRV_LOG(INFO, "PF-%d received HI msg from VF internal "
			"id %d global %d", PCI_FUNC(qdma_dev->pf), i, vf_funid);

	qbase = recv_msg->data[0];
	qdma_dev->vfinfo[i].qbase = recv_msg->data[0];
	qmax = recv_msg->data[1];
	qdma_dev->vfinfo[i].qmax = recv_msg->data[1];
	qdma_dev->vfinfo[i].func_id = vf_funid;

	sz = sizeof(struct qdma_vf_queue_info) * qmax;
	qdma_dev->vfinfo[i].vfqinfo = rte_zmalloc("vfqinfo", sz, 0);
	if (qdma_dev->vfinfo[i].vfqinfo == NULL) {
		PMD_DRV_LOG(INFO, "cannot allocate memory for VF queue info");
		return -1;
	}

	ret = qdma_set_fmap(qdma_dev, vf_funid, qbase, qmax);
	if (ret < 0) {
		rte_free(qdma_dev->vfinfo[i].vfqinfo);
		return -1;
	}
	return 0;
}

static int qdma_vf_bye(struct rte_eth_dev *dev, uint16_t vf, uint16_t vf_funid)
{
	struct qdma_pci_dev *qdma_dev  = dev->data->dev_private;
	uint64_t bar_addr;
#if FMAP_CNTXT
	uint32_t ctxt_sel, reg_val;
	uint32_t i, flag;
	struct queue_ind_prg *q_regs;
#else
	uint64_t fmap;
#endif

	PMD_DRV_LOG(INFO, "PF-%d received BYE msg from VF internal id %d, "
			"global id %d", PCI_FUNC(qdma_dev->pf), vf, vf_funid);

	if (qdma_dev->vfinfo[vf].vfqinfo != NULL)
		rte_free(qdma_dev->vfinfo[vf].vfqinfo);

	qdma_dev->vfinfo[vf].qbase = 0;
	qdma_dev->vfinfo[vf].qmax = 0;
	qdma_dev->vfinfo[vf].func_id = QDMA_FUNC_ID_INVALID;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
#if FMAP_CNTXT
	q_regs = (struct queue_ind_prg *)(bar_addr + QDMA_TRQ_SEL_IND +
						QDMA_IND_Q_PRG_OFF);

	ctxt_sel = (QDMA_CTXT_SEL_FMAP << CTXT_SEL_SHIFT_B) |
			(vf_funid << QID_SHIFT_B) |
			(QDMA_CTXT_CMD_CLR << OP_CODE_SHIFT_B);
	qdma_write_reg((uint64_t)&q_regs->ctxt_cmd, ctxt_sel);

	flag = 1;
	i = CONTEXT_PROG_POLL_COUNT;
	while (flag && i) {
		reg_val = qdma_read_reg((uint64_t)&q_regs->ctxt_cmd);
		PMD_DRV_LOG(INFO, " Read cmd for device:%d: reg_val:%x\n",
				vf_funid, reg_val);
		flag = reg_val & BUSY_BIT_MSK;
		rte_delay_ms(CONTEXT_PROG_DELAY);
		i--;
	}
#else

	fmap = (uint64_t)(bar_addr + QDMA_TRQ_SEL_FMAP + (vf_funid * 4));
	qdma_write_reg((uint64_t)fmap, 0);
#endif

	return 0;
}

/* Perform queue context programming */

static int qdma_vf_qadd(struct rte_eth_dev *dev, uint16_t vf, uint8_t vf_funid,
			struct qdma_mbox_data *recv_msg)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	struct qadd_msg msg;
	int err, qbase;
	uint64_t addr;

	PMD_DRV_LOG(INFO, "PF-%d received QADD msg from VF internal id %d, "
			"global id %d", PCI_FUNC(qdma_dev->pf), vf, vf_funid);

	msg.qid			= recv_msg->data[0];
	msg.st			= recv_msg->data[1];
	msg.c2h			= recv_msg->data[2];
	msg.ringsz		= recv_msg->data[3];
	msg.bufsz		= recv_msg->data[4];
	msg.thresidx		= recv_msg->data[5];
	msg.timeridx		= recv_msg->data[6];
	msg.triggermode		= recv_msg->data[7];
	msg.cmpt_ringszidx	= recv_msg->data[12];
	msg.prefetch		= recv_msg->data[13];
	msg.cmpt_desc_fmt	= recv_msg->data[14];
	msg.en_bypass		= recv_msg->data[15];
	msg.bypass_desc_sz_idx	= recv_msg->data[16];
	msg.en_bypass_prefetch	= recv_msg->data[17];
	msg.dis_overflow_check	= recv_msg->data[18];

	addr = rte_le_to_cpu_64(((uint64_t)recv_msg->data[9] << 32) |
					recv_msg->data[8]);
	msg.ring_bs_addr = addr;
	addr = rte_le_to_cpu_64(((uint64_t)recv_msg->data[11] << 32) |
					recv_msg->data[10]);
	msg.cmpt_ring_bs_addr = addr;

	qbase = qdma_dev->vfinfo[vf].qbase;

	qdma_dev->vfinfo[vf].vfqinfo[msg.qid - qbase].mode = msg.st;

	if (msg.c2h) {
		qdma_reset_rx_queues(dev, msg.qid, msg.st);
		err = qdma_queue_ctxt_rx_prog(dev, msg.qid, msg.st,
						msg.prefetch, msg.ringsz,
						msg.cmpt_ringszidx, msg.bufsz,
						msg.thresidx, msg.timeridx,
						msg.triggermode, vf_funid,
						msg.ring_bs_addr,
						msg.cmpt_ring_bs_addr,
						msg.cmpt_desc_fmt,
						msg.en_bypass,
						msg.en_bypass_prefetch,
						msg.bypass_desc_sz_idx,
						msg.dis_overflow_check);
		if (err != 0)
			return err;
	} else {
		qdma_reset_tx_queues(dev, msg.qid, msg.st);
		err = qdma_queue_ctxt_tx_prog(dev, msg.qid, msg.st, msg.ringsz,
						vf_funid, msg.ring_bs_addr,
						msg.en_bypass,
						msg.bypass_desc_sz_idx);
		if (err != 0)
			return err;
	}

	*qdma_dev->h2c_mm_control = QDMA_MM_CTRL_START; /* Start Tx h2c engine*/
	*qdma_dev->c2h_mm_control = QDMA_MM_CTRL_START; /* Start Rx c2h engine*/

	return 0;
}

static int qdma_vf_qdel(struct rte_eth_dev *dev, uint8_t vf,
				struct qdma_mbox_data *recv_msg)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	int qbase = 0;
	uint32_t qid, dir;

	PMD_DRV_LOG(INFO, "PF-%d received QDEL msg from VF-%d",
			PCI_FUNC(qdma_dev->pf), vf);

	qid = recv_msg->data[0];
	dir = recv_msg->data[1];
	qbase = qdma_dev->vfinfo[vf].qbase;

	if (dir == DMA_FROM_DEVICE)
		qdma_reset_rx_queues(dev, qid,
				qdma_dev->vfinfo[vf].vfqinfo[qid - qbase].mode);
	else
		qdma_reset_tx_queues(dev, qid,
				qdma_dev->vfinfo[vf].vfqinfo[qid - qbase].mode);

	return 0;
}

/*
 * Calculate vf internal funciton of PF from global function id
 */
static int qdma_get_dev_internal_vfid(struct rte_eth_dev *dev, uint8_t devfn)
{
	uint16_t  i;
	struct rte_pci_device *pci_dev;
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;

	pci_dev = RTE_ETH_DEV_TO_PCI(dev);

	for (i = 0; i < pci_dev->max_vfs; i++) {
		if (qdma_dev->vfinfo[i].func_id == devfn)
			return i;
	}

	return QDMA_FUNC_ID_INVALID;
}

static void qdma_read_vf_msg(struct rte_eth_dev *dev, uint8_t devfn)
{
	struct qdma_pci_dev *qdma_dev = dev->data->dev_private;
	uint64_t bar_addr;
	struct qdma_mbox_data recv_msg;
	struct mbox_imsg *in_msg;
	int32_t retval = -1;
	uint32_t reg_val;
	uint16_t vf;
	int i;
	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];

	/* write function ID into target function register to get the
	 * msg in incoming message register
	 */

	qdma_write_reg((uint64_t)(bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_TRGT_FN),
									devfn);
	in_msg = (struct mbox_imsg *)(bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_IMSG);
	reg_val = qdma_read_reg((uint64_t)&in_msg->imsg[0]);

	recv_msg.opcode = reg_val & 0xff;
	recv_msg.debug_funid  = (reg_val >> 8) & 0xfff;
	recv_msg.filler = (reg_val >> 20) & 0xf;
	recv_msg.err    = (reg_val >> 24) & 0xff;

	for (i = 1; i < 32; i++)
		recv_msg.data[i - 1] = qdma_read_reg((uint64_t)&
							in_msg->imsg[i]);

	if (recv_msg.opcode == QDMA_MBOX_OPCD_HI) {
		retval = qdma_vf_hi(dev, devfn, &recv_msg);
		goto ack_ok;
	}

	vf = qdma_get_dev_internal_vfid(dev, devfn);

	if (vf == QDMA_FUNC_ID_INVALID)
		return;

	switch (recv_msg.opcode) {
	case QDMA_MBOX_OPCD_BYE:
		retval = qdma_vf_bye(dev, vf, devfn);
		break;
	case QDMA_MBOX_OPCD_QADD:
		retval = qdma_vf_qadd(dev, vf, devfn, &recv_msg);
		break;
	case QDMA_MBOX_OPCD_QDEL:
		retval = qdma_vf_qdel(dev, vf, &recv_msg);
		break;
	}

ack_ok:
	/* once the msg is handled ACK it */
	if (!retval)
		qdma_write_reg((bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_FCMD),
					QDMA_MBOX_CMD_RECV);
}

static void qdma_check_vf_msg(void *arg)
{
	struct qdma_pci_dev *qdma_dev = NULL;
	uint64_t bar_addr, reg_addr;
	struct mbox_fsr *mbx_stat;

	qdma_dev = ((struct rte_eth_dev *)arg)->data->dev_private;
	if (!qdma_dev)
		return;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	reg_addr = bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_FSR;
	mbx_stat = (struct mbox_fsr *)(reg_addr);


	if (mbx_stat->imsg_stat == 1)
		qdma_read_vf_msg((struct rte_eth_dev *)arg,
					mbx_stat->curr_src_fn);

	rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_check_vf_msg, arg);
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

static void qdma_device_attributes_get(struct qdma_pci_dev *qdma_dev)
{
	int mm_c2h_flag = 0;
	int mm_h2c_flag = 0;
	int st_c2h_flag = 0;
	int st_h2c_flag = 0;
	uint64_t bar_addr;
	uint32_t v1;

	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	v1 = qdma_read_reg((uint64_t)(bar_addr + QDMA_REG_GLBL_QMAX));

	/* DPDK limitation */
	if (v1 > RTE_MAX_QUEUES_PER_PORT)
		v1 = RTE_MAX_QUEUES_PER_PORT;
	qdma_dev->qsets_max = v1;

	v1 = qdma_read_reg((uint64_t)(bar_addr + QDMA_REG_GLBL_MDMA_CHANNEL));
	mm_c2h_flag = (v1 & MDMA_CHANNEL_MM_C2H_ENABLED_MASK) ? 1 : 0;
	mm_h2c_flag = (v1 & MDMA_CHANNEL_MM_H2C_ENABLED_MASK) ? 1 : 0;
	st_c2h_flag = (v1 & MDMA_CHANNEL_ST_C2H_ENABLED_MASK) ? 1 : 0;
	st_h2c_flag = (v1 & MDMA_CHANNEL_ST_H2C_ENABLED_MASK) ? 1 : 0;

	qdma_dev->mm_mode_en = (mm_c2h_flag && mm_h2c_flag) ? 1 : 0;
	qdma_dev->st_mode_en = (st_c2h_flag && st_h2c_flag) ? 1 : 0;

	PMD_DRV_LOG(INFO, "qmax = %d, mm %d, st %d.\n", qdma_dev->qsets_max,
			 qdma_dev->mm_mode_en, qdma_dev->st_mode_en);
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
	uint64_t bar_len;
	int i, idx;
	static bool once = true;
	int vf_num;
	uint64_t reg_addr, sz;
	uint32_t reg_val;
	uint32_t pfch_val;
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

	if (once)
		RTE_LOG(INFO, PMD, "QDMA PMD VERSION: %s\n", QDMA_PMD_VERSION);

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
	dma_priv->queue_base = DEFAULT_QUEUE_BASE;
	dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_TIMER;

	/* Setting default Mode to TRIG_MODE_USER_TIMER_COUNT */
	dma_priv->trigger_mode = TRIG_MODE_USER_TIMER_COUNT;
	if (dma_priv->trigger_mode == TRIG_MODE_USER_TIMER_COUNT)
		dma_priv->timer_count = DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER;

	dma_priv->en_desc_prefetch = 0; //Keep prefetch default to 0
	dma_priv->cmpt_desc_len = DEFAULT_QDMA_CMPT_DESC_LEN;
	dma_priv->c2h_bypass_mode = C2H_BYPASS_MODE_NONE;
	dma_priv->h2c_bypass_mode = 0;

	/*Default write-back accumulation interval */
	dma_priv->wb_acc_int = DEFAULT_WB_ACC_INT;

	dma_priv->config_bar_idx = DEFAULT_PF_CONFIG_BAR;
	dma_priv->bypass_bar_idx = BAR_ID_INVALID;
	dma_priv->user_bar_idx = BAR_ID_INVALID;

	/* Check and handle device devargs*/
	if (qdma_check_kvargs(dev->device->devargs, dma_priv)) {
		PMD_DRV_LOG(INFO, "devargs failed\n");
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	idx = qdma_identify_bars(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	PMD_DRV_LOG(INFO, "QDMA device driver probe:");

	idx = qdma_get_hw_version(dev);
	if (idx < 0) {
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	baseaddr = (uint8_t *)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr;
	bar_len = pci_dev->mem_resource[dma_priv->config_bar_idx].len;

	dma_priv->bar_addr[dma_priv->config_bar_idx] = baseaddr;
	dma_priv->bar_len[dma_priv->config_bar_idx] = bar_len;

	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			    pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		bar_len = pci_dev->mem_resource[dma_priv->user_bar_idx].len;

		dma_priv->bar_addr[dma_priv->user_bar_idx] = baseaddr;
		dma_priv->bar_len[dma_priv->user_bar_idx] = bar_len;
	}

	qdma_dev_ops_init(dev);
	baseaddr = (uint8_t *)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr;

	/* Getting the device attributes from the Hardware */
	qdma_device_attributes_get(dma_priv);

	dma_priv->c2h_mm_control = (volatile uint32_t *)(baseaddr +
							  QDMA_TRQ_SEL_C2H_MM0 +
							  QDMA_C2H_MM0_CONTROL);
	dma_priv->h2c_mm_control = (volatile uint32_t *)(baseaddr +
							 QDMA_TRQ_SEL_H2C_MM0 +
							 QDMA_H2C_MM0_CONTROL);
	if (once) {
		/** Write-back accumulation configuration **/
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_GLBL +
					QDMA_GLBL_DSC_CFG);
		reg_val = ((dma_priv->wb_acc_int <<
					QDMA_GLBL_DSC_CFG_WB_ACC_SHFT) |
				(DEFAULT_MAX_DESC_FETCH <<
				 QDMA_GLBL_DSC_CFG_MAX_DSC_FTCH_SHFT));
		qdma_write_reg((uint64_t)reg_addr, reg_val);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		reg_val = qdma_read_reg((uint64_t)reg_addr);
		PMD_DRV_LOG(INFO, " GLBL_DSC_CFG reg_val:0x%x, wb_acc: %d,"
				" max_desc_fetch: %d\n",
			reg_val, (reg_val & QDMA_GLBL_DSC_CFG_WB_ACC_MSK),
			(reg_val & QDMA_GLBL_DSC_CFG_MAX_DSC_FTCH_MSK) >>
				QDMA_GLBL_DSC_CFG_MAX_DSC_FTCH_SHFT);
#endif
		/* Ring-size configuration, need to set the same index
		 * in the H2C/C2H context structure
		 */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_GLBL +
					QDMA_GLBL_RING_SZ);
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++, reg_addr += 4) {
			qdma_write_reg((uint64_t)reg_addr, g_ring_sz[i]);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
			reg_val = qdma_read_reg((uint64_t)reg_addr);
			PMD_DRV_LOG(INFO, " Ring-size reg_val:0x%x written to "
					"index %d", reg_val, i);
#endif
		}

		/* C2H threshold count configuration, need to set the same index
		 * in the C2H completion context structure
		 */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
					QDMA_C2H_CNT_TH_BASE);
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++, reg_addr += 4) {
			qdma_write_reg((uint64_t)reg_addr, g_c2h_cnt_th[i]);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
			reg_val = qdma_read_reg((uint64_t)reg_addr);
			PMD_DRV_LOG(INFO, " threshold-count reg_val:0x%x "
					"written to index %d\n", reg_val, i);
#endif
		}

		/* C2H Buf size, need to allocate the same buffer size
		 * in the C2H descriptor source addr
		 */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
						QDMA_C2H_BUF_SZ_BASE);
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++, reg_addr += 4) {
			qdma_write_reg((uint64_t)reg_addr, g_c2h_buf_sz[i]);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
			reg_val = qdma_read_reg((uint64_t)reg_addr);
			PMD_DRV_LOG(INFO, " Buffer size for C2H, reg_val:0x%x "
					"written to index %d\n", reg_val, i);
#endif
		}

		/** C2H Prefetch configuration */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
					QDMA_C2H_PFCH_CACHE_DEPTH_OFFSET);
		pfch_val = qdma_read_reg(reg_addr);

		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
						QDMA_C2H_PFCH_CFG_OFFSET);
		reg_val = ((DEFAULT_PFCH_STOP_THRESH <<
					QDMA_C2H_PFCH_CFG_STOP_THRESH_SHIFT) |
				(DEFAULT_PFCH_NUM_ENTRIES_PER_Q <<
				 QDMA_C2H_PFCH_CFG_NUM_ENTRIES_PER_Q_SHIFT) |
				((pfch_val / 2) <<
				 QDMA_C2H_PFCH_CFG_MAX_Q_CNT_SHIFT) |
				(((pfch_val / 2) - 2) <<
				 QDMA_C2H_PFCH_CFG_EVICTION_Q_CNT_SHIFT));
		qdma_write_reg((uint64_t)reg_addr, reg_val);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		reg_val = qdma_read_reg((uint64_t)reg_addr);
		PMD_DRV_LOG(INFO, "C2H prefetch config, reg_val:0x%x", reg_val);
#endif

		/** C2H timer tick configuration */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
					QDMA_C2H_INT_TIMER_TICK_OFFSET);
		qdma_write_reg((uint64_t)reg_addr, DEFAULT_C2H_INTR_TIMER_TICK);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		reg_val = qdma_read_reg((uint64_t)reg_addr);
		PMD_DRV_LOG(INFO, " timer tick for C2H, reg_val:0x%x", reg_val);
#endif

		/** C2H Writeback Coalesce configuration */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
					QDMA_C2H_CMPT_COAL_BUF_DEPTH_OFFSET);
		pfch_val = qdma_read_reg(reg_addr);

		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
					QDMA_C2H_CMPT_COAL_CFG_OFFSET);
		reg_val = ((DEFAULT_CMPT_COAL_TIMER_CNT <<
				QDMA_C2H_CMPT_COAL_TIMER_CNT_VAL_SHIFT) |
				(DEFAULT_CMPT_COAL_TIMER_TICK <<
				 QDMA_C2H_CMPT_COAL_TIMER_TICK_VAL_SHIFT) |
				(pfch_val <<
				 QDMA_C2H_CMPT_COAL_MAX_BUF_SZ_SHIFT));

		qdma_write_reg((uint64_t)reg_addr, reg_val);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
		reg_val = qdma_read_reg((uint64_t)reg_addr);
		PMD_DRV_LOG(INFO, "C2H writeback coalesce config, reg_val:0x%x",
				reg_val);
#endif
		/** C2H timer configuration*/
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_C2H +
						QDMA_C2H_TIMER_BASE);
		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++, reg_addr += 4) {
			qdma_write_reg((uint64_t)reg_addr, g_c2h_timer_cnt[i]);
#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
			reg_val = qdma_read_reg((uint64_t)reg_addr);
			PMD_DRV_LOG(INFO, " timer count for C2H, reg_val:0x%x "
					"written to index %d\n", reg_val, i);
#endif
		}

		/* Data Threshold (Throttle) register setting when
		 * Prefetch is disabled
		 */
		reg_addr = (uint64_t)(baseaddr + QDMA_TRQ_SEL_H2C +
					QDMA_H2C_DATA_THRESHOLD_OFFSET);
		qdma_write_reg((uint64_t)reg_addr, DEFAULT_H2C_THROTTLE);
		once = false;
	}

	vf_num = pci_dev->max_vfs;
	if (vf_num) {
		sz = sizeof(struct qdma_vf_info) * vf_num;
		dma_priv->vfinfo = rte_zmalloc("vfinfo", sz, 0);
		if (dma_priv->vfinfo == NULL)
			rte_panic("Cannot allocate memory for VF info\n");
		/* Mark all VFs with invalid function id mapping*/
		for (i = 0; i < vf_num; i++)
			dma_priv->vfinfo[i].func_id = QDMA_FUNC_ID_INVALID;

		rte_eal_alarm_set(MBOX_POLL_FRQ, qdma_check_vf_msg,
							(void *)dev);
	}

	pcie_perf_enable(pci_dev);

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
	uint32_t numvf, i, count, v;
	struct mbox_fsr *mbx_stat;
	struct qdma_mbox_data send_msg;
	uint64_t bar_addr, addr;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;
	bar_addr = (uint64_t)qdma_dev->bar_addr[qdma_dev->config_bar_idx];
	mbx_stat = (struct mbox_fsr *)(bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_FSR);

	numvf = pci_dev->max_vfs;

	for (i = 0; i < numvf; i++) {
		count = MAILBOX_PROG_POLL_COUNT;
		memset(&send_msg, 0, sizeof(send_msg));
		if (qdma_dev->vfinfo[i].qmax) {
			qdma_write_reg((uint64_t)(bar_addr + QDMA_TRQ_EXT +
							QDMA_MBOX_TRGT_FN),
					qdma_dev->vfinfo[i].func_id);
			while (mbx_stat->omsg_stat && count) {
				rte_delay_ms(MAILBOX_PF_MSG_DELAY);
				count--;
			}
			if (count) {
				PMD_DRV_LOG(INFO, "PF-%d(DEVFN) send BYE "
						"message to VF-%d(DEVFN)",
						qdma_dev->pf,
						qdma_dev->vfinfo[i].func_id);
				send_msg.opcode = QDMA_MBOX_OPCD_BYE;
				send_msg.debug_funid = qdma_dev->pf;
				v = rte_cpu_to_le_32(send_msg.debug_funid << 8 |
							send_msg.opcode);
				addr = bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_OMSG;
				qdma_write_reg(addr, v);

				addr = bar_addr + QDMA_TRQ_EXT + QDMA_MBOX_FCMD;
				qdma_write_reg(addr, QDMA_MBOX_CMD_SEND);

				count = MAILBOX_PROG_POLL_COUNT;
				while (mbx_stat->omsg_stat && count) {
					rte_delay_ms(MAILBOX_PF_MSG_DELAY);
					count--;
				}
				if (!count)
					PMD_DRV_LOG(INFO, "Pf-%d(DEVFN) did "
							"not receive ACK from "
							"VF-%d(DEVFN)",
							qdma_dev->pf,
						   qdma_dev->vfinfo[i].func_id);
			} else {
				PMD_DRV_LOG(INFO, "Pf-%d(DEVFN) could not send "
						"BYE message to VF-%d(DEVFN)",
						qdma_dev->pf,
						qdma_dev->vfinfo[i].func_id);
				break;
			}
		}
	}

	/* cancel pending polls*/
	rte_eal_alarm_cancel(qdma_check_vf_msg, (void *)dev);
	*qdma_dev->h2c_mm_control = 0; /* Stop Tx h2c engine */
	*qdma_dev->c2h_mm_control = 0;

	if (qdma_dev->vfinfo != NULL) {
		rte_free(qdma_dev->vfinfo);
		qdma_dev->vfinfo = NULL;
	}

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

RTE_PMD_REGISTER_PCI(net_qdma, rte_qdma_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_qdma, qdma_pci_id_tbl);
