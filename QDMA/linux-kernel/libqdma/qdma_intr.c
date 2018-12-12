/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_intr.h"

#include <linux/kernel.h>
#include "qdma_descq.h"
#include "qdma_device.h"
#include "qdma_regs.h"
#include "thread.h"
#include "version.h"
#include "qdma_regs.h"

struct qdma_err_info {
	u32 intr_mask;
	char **stat;
};

struct qdma_err_stat_info {
	char *err_name;
	u32 stat_reg_addr;
	u32 mask_reg_addr;
	struct qdma_err_info err_info;
};

/** List shall be from Bit 0 - Bit31 */
char *glbl_err_info[] = {
	"err_ram_sbe",
	"err_ram_dbe",
	"err_dsc",
	"err_trq",
	"err_h2c_mm_0",
	"err_h2c_mm_1",
	"err_c2h_mm_0",
	"err_c2h_mm_1",
	"err_c2h_st",
	"ind_ctxt_cmd_err",
	"err_bdg",
	"err_h2c_st"
};

char *dsc_err_info[] = {
	"poison",
	"ur_ca",
	"param",
	"addr",
	"tag",
	"flr",
	"timeout",
	"dat_poison",
	"flr_cancel",
	"dma",
	"dsc",
	"rq_cancel",
	"dbe",
	"sbe"
};

char *trq_err_info[] = {
	"unmapped",
	"qid_range",
	"vf_access_err",
	"tcp_timeout"
};

char *c2h_err_info[] = {
	"mty_mismatch",
	"len_mismatch",
	"qid_mismatch",
	"desc_rsp_err",
	"eng_wpl_data_par_err",
	"msi_int_fail",
	"err_desc_cnt",
	"portid_ctxt_mismatch",
	"portid_byp_in_mismatch",
	"cmpt_inv_q_err",
	"cmpt_qfull_err",
	"cmpt_cidx_err",
	"cmpt_prty_err"
};

char *c2h_fatal_err_info[] = {
	"mty_mismatch",
	"len_mismatch",
	"qid_mismatch",
	"timer_fifo_ram_rdbe",
	"eng_wpl_data_par_err",
	"pfch_II_ram_rdbe",
	"cmpt_ctxt_ram_rdbe",
	"pfch_ctxt_ram_rdbe",
	"desc_req_fifo_ram_rdbe",
	"int_ctxt_ram_rdbe",
	"cmpt_coal_data_ram_rdbe",
	"tuser_fifo_ram_rdbe",
	"qid_fifo_ram_rdbe",
	"payload_fifo_ram_rdbe",
	"wpl_data_par_err"
};

char *h2c_err_info[] = {
	"zero_len_desc_err",
	"sdi_mrkr_req_mop_err",
	"no_dma_dsc_err",
};

/** ECC Single bit errors from Bit 0 -Bit 31 */
char *ecc_sb_err_info[] = {
	"mi_h2c0_dat",
	"mi_c2h0_dat",
	"h2c_rd_brg_dat",
	"h2c_wr_brg_dat",
	"c2h_rd_brg_dat",
	"c2h_wr_brg_dat",
	"func_map",
	"dsc_hw_ctxt",
	"dsc_crd_rcv",
	"dsc_sw_ctxt",
	"dsc_cpli",
	"dsc_cpld",
	"pasid_ctxt_ram",
	"timer_fifo_ram",
	"payload_fifo_ram",
	"qid_fifo_ram",
	"tuser_fifo_ram",
	"wrb_coal_data_ram",
	"int_qid2vec_ram",
	"int_ctxt_ram",
	"desc_req_fifo_ram",
	"pfch_ctxt_ram",
	"wrb_ctxt_ram",
	"pfch_ll_ram",
	"h2c_pend_fifo"
};

/** ECC Double bit errors from Bit 0 -Bit 31 */
char *ecc_db_err_info[] = {
	"mi_h2c0_dat",
	"mi_c2h0_dat",
	"h2c_rd_brg_dat",
	"h2c_wr_brg_dat",
	"c2h_rd_brg_dat",
	"c2h_wr_brg_dat",
	"func_map",
	"dsc_hw_ctxt",
	"dsc_crd_rcv",
	"dsc_sw_ctxt",
	"dsc_cpli",
	"dsc_cpld",
	"pasid_ctxt_ram",
	"timer_fifo_ram",
	"payload_fifo_ram",
	"qid_fifo_ram",
	"tuser_fifo_ram",
	"wrb_coal_data_ram",
	"int_qid2vec_ram",
	"int_ctxt_ram",
	"desc_req_fifo_ram",
	"pfch_ctxt_ram",
	"wrb_ctxt_ram",
	"pfch_ll_ram",
	"h2c_pend_fifo",
};

struct qdma_err_stat_info err_stat_info[HW_ERRS] = {
	{ "glbl_err", QDMA_REG_GLBL_ERR_STAT, QDMA_REG_GLBL_ERR_MASK,
			{ QDMA_REG_GLBL_ERR_MASK_VALUE, glbl_err_info } },
	{ "dsc_err", QDMA_GLBL_DSC_ERR_STS, QDMA_GLBL_DSC_ERR_MSK,
			{ QDMA_GLBL_DSC_ERR_MSK_VALUE, dsc_err_info } },
	{ "trq_err", QDMA_GLBL_TRQ_ERR_STS, QDMA_GLBL_TRQ_ERR_MSK,
			{ QDMA_GLBL_TRQ_ERR_MSK_VALUE, trq_err_info } },
	{ "c2h_err", QDMA_REG_C2H_ERR_STAT, QDMA_REG_C2H_ERR_MASK,
			{ QDMA_REG_C2H_ERR_MASK_VALUE, c2h_err_info } },
	{ "c2h_fatal_err", QDMA_C2H_FATAL_ERR_STAT, QDMA_C2H_FATAL_ERR_MASK,
			{ QDMA_C2H_FATAL_ERR_MASK_VALUE, c2h_fatal_err_info } },
	{ "h2c_err", QDMA_H2C_ERR_STAT, QDMA_H2C_ERR_MASK,
			{ QDMA_H2C_ERR_MASK_VALUE, h2c_err_info } },
	{ "ecc_sb_err", QDMA_RAM_SBE_STAT_A, QDMA_RAM_SBE_MASK_A,
			{ QDMA_RAM_SBE_MASK_VALUE, ecc_sb_err_info } },
	{ "ecc_sb_err", QDMA_RAM_DBE_STAT_A, QDMA_RAM_DBE_MASK_A,
			{ QDMA_RAM_DBE_MASK_VALUE, ecc_db_err_info } },
};

#ifndef __QDMA_VF__
static LIST_HEAD(legacy_intr_q_list);
static spinlock_t legacy_intr_lock;
static spinlock_t legacy_q_add_lock;
static unsigned long legacy_intr_flags = IRQF_SHARED;
#endif

void err_stat_handler(struct xlnx_dma_dev *xdev)
{
	u32 i;
	u32 err_stat;
	u32 glb_err_stat = 0;

	for (i = 0; i < HW_ERRS; i++) {
		err_stat = __read_reg(xdev, err_stat_info[i].stat_reg_addr);
		if ((i == 0) && err_stat_info[i].err_info.intr_mask)
			glb_err_stat = err_stat;
		if (err_stat & err_stat_info[i].err_info.intr_mask) {
			uint8_t bit = 0;
			uint32_t intr_mask =
					err_stat_info[i].err_info.intr_mask;
			uint32_t chk_mask = 0x01;

			pr_info("%s[0x%x] : 0x%x", err_stat_info[i].err_name,
					err_stat_info[i].stat_reg_addr,
					err_stat);
			while (intr_mask) {
				if (((intr_mask & 0x01)) &&
					(err_stat & chk_mask))
					pr_err("\t%s detected",
					err_stat_info[i].err_info.stat[bit]);

				if (intr_mask & 0x01)
					bit++;
				intr_mask >>= 1;
				chk_mask <<= 1;
			}
			__write_reg(xdev, err_stat_info[i].stat_reg_addr,
				err_stat);
		}
	}
	if (glb_err_stat) {
		__write_reg(xdev, err_stat_info[0].stat_reg_addr,
			    glb_err_stat);
		qdma_err_intr_setup(xdev, 1);
	}
}

static irqreturn_t user_intr_handler(int irq_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	pr_info("User IRQ fired on PF#%d: index=%d, vector=%d\n",
		xdev->func_id, irq_index, irq);

	if (xdev->conf.fp_user_isr_handler)
		xdev->conf.fp_user_isr_handler((unsigned long)xdev,
						xdev->conf.uld);

	return IRQ_HANDLED;
}

#ifndef __QDMA_VF__
static irqreturn_t error_intr_handler(int irq_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;
	unsigned long flags;

	pr_info("Error IRQ fired on PF#%d: index=%d, vector=%d\n",
			xdev->func_id, irq_index, irq);

	spin_lock_irqsave(&xdev->lock, flags);

	err_stat_handler(xdev);

	spin_unlock_irqrestore(&xdev->lock, flags);

	return IRQ_HANDLED;
}
#endif

static void data_intr_aggregate(struct xlnx_dma_dev *xdev, int vidx, int irq)
{
	struct qdma_descq *descq = NULL;
	u32 counter = 0;
	int ring_index = 0;
	struct intr_coal_conf *coal_entry =
			(xdev->intr_coal_list + vidx - xdev->dvec_start_idx);
	struct qdma_intr_ring *ring_entry;

	if (!coal_entry) {
		pr_err("Failed to locate the coalescing entry for vector = %d\n",
			vidx);
		return;
	}
	pr_debug("INTR_COAL: msix[%d].vector=%d, msix[%d].entry=%d, rngsize=%d, cidx = %d\n",
		vidx, xdev->msix[vidx].vector,
		vidx,
		xdev->msix[vidx].entry,
		coal_entry->intr_rng_num_entries,
		coal_entry->cidx);

	pr_debug("vidx = %d, dvec_start_idx = %d\n", vidx,
		 xdev->dvec_start_idx);

	if ((xdev->msix[vidx].entry) !=  coal_entry->vec_id) {
		pr_err("msix[%d].entry[%d] != vec_id[%d]\n",
			vidx, xdev->msix[vidx].entry,
			coal_entry->vec_id);

		return;
	}

	counter = coal_entry->cidx;
	ring_entry = (coal_entry->intr_ring_base + counter);
	if (!ring_entry) {
		pr_err("Failed to locate the ring entry for vector = %d\n",
			vidx);
		return;
	}
	while (ring_entry->coal_color == coal_entry->color) {
		pr_debug("IRQ[%d]: IVE[%d], Qid = %d, e_color = %d, c_color = %d, intr_type = %d\n",
				irq, vidx, ring_entry->qid, coal_entry->color,
				ring_entry->coal_color, ring_entry->intr_type);

		descq = qdma_device_get_descq_by_hw_qid(xdev, ring_entry->qid,
				ring_entry->intr_type);
		if (!descq) {
			pr_err("IRQ[%d]: IVE[%d], Qid = %d: desc not found\n",
					irq, vidx, ring_entry->qid);
			return;
		}

		if (descq->conf.fp_descq_isr_top) {
			struct qdma_dev *qdev = xdev_2_qdev(xdev);

			descq->conf.fp_descq_isr_top(descq->conf.qidx +
					(descq->conf.c2h ? qdev->qmax : 0),
					descq->conf.quld);
		} else {
			if (descq->cpu_assigned)
				schedule_work_on(descq->intr_work_cpu,
						&descq->work);
			else
				schedule_work(&descq->work);
		}

		if (++coal_entry->cidx == coal_entry->intr_rng_num_entries) {
			counter = 0;
			xdev->intr_coal_list->color =
				(xdev->intr_coal_list->color) ? 0 : 1;
			coal_entry->cidx = 0;
		} else
			counter++;

		ring_entry = (coal_entry->intr_ring_base + counter);
	}

	if (descq) {
		ring_index = get_intr_ring_index(descq->xdev,
						 coal_entry->vec_id);
		intr_cidx_update(descq, coal_entry->cidx, ring_index);
	}
}

static void data_intr_direct(struct xlnx_dma_dev *xdev, int vidx, int irq)
{
	struct qdma_descq *descq;

	list_for_each_entry(descq, &xdev->dev_intr_info_list[vidx].intr_list,
			    intr_list)
		if (descq->conf.fp_descq_isr_top) {
			struct qdma_dev *qdev = xdev_2_qdev(xdev);

			descq->conf.fp_descq_isr_top(descq->conf.qidx +
					(descq->conf.c2h ? qdev->qmax : 0),
					descq->conf.quld);
		} else {
			if (descq->cpu_assigned)
				schedule_work_on(descq->intr_work_cpu,
						&descq->work);
			else
				schedule_work(&descq->work);
		}
}

static irqreturn_t data_intr_handler(int vector_index, int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;
	unsigned long flags;

	pr_debug("Data IRQ fired on PF#%d: index=%d, vector=%d\n",
		xdev->func_id, vector_index, irq);

	spin_lock_irqsave(&xdev->lock, flags);
	if ((xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(xdev->conf.qdma_drv_mode == AUTO_MODE))
		data_intr_aggregate(xdev, vector_index, irq);
	else
		data_intr_direct(xdev, vector_index, irq);
	spin_unlock_irqrestore(&xdev->lock, flags);

	return IRQ_HANDLED;
}

static inline void intr_ring_free(struct xlnx_dma_dev *xdev, int ring_sz,
			int intr_desc_sz, u8 *intr_desc, dma_addr_t desc_bus)
{
	unsigned int len = ring_sz * intr_desc_sz;

	pr_debug("free %u(0x%x)=%d*%u, 0x%p, bus 0x%llx.\n",
		len, len, intr_desc_sz, ring_sz, intr_desc, desc_bus);

	dma_free_coherent(&xdev->conf.pdev->dev, (size_t)ring_sz * intr_desc_sz,
			intr_desc, desc_bus);
}

static void *intr_ring_alloc(struct xlnx_dma_dev *xdev, int ring_sz,
				int intr_desc_sz, dma_addr_t *bus)
{
	unsigned int len = ring_sz * intr_desc_sz;
	u8 *p = dma_alloc_coherent(&xdev->conf.pdev->dev, len, bus, GFP_KERNEL);

	if (!p) {
		pr_err("%s, OOM, sz ring %d, intr_desc %d.\n",
			xdev->conf.name, ring_sz, intr_desc_sz);
		return NULL;
	}

	memset(p, 0, len);

	pr_debug("alloc %u(0x%x)=%d*%u, bus 0x%llx .\n",
		len, len, intr_desc_sz, ring_sz, *bus);

	return p;
}

void intr_ring_teardown(struct xlnx_dma_dev *xdev)
{
	int i = 0;
	struct intr_coal_conf  *ring_entry;


#ifndef __QDMA_VF__
	int rv = 0;
	unsigned int ring_index = 0;
#endif

	while (i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT) {
#ifndef __QDMA_VF__
		ring_index = get_intr_ring_index(xdev,
				(i + xdev->dvec_start_idx));
#endif
		ring_entry = (xdev->intr_coal_list + i);
		if (ring_entry) {
			intr_ring_free(xdev,
				ring_entry->intr_rng_num_entries,
				sizeof(struct qdma_intr_ring),
				(u8 *)ring_entry->intr_ring_base,
				ring_entry->intr_ring_bus);
#ifndef __QDMA_VF__
			pr_debug("Clearing intr_ctxt for ring_index =%d\n",
				ring_index);
			/* clear interrupt context (0x8) */
			rv = hw_indirect_ctext_prog(xdev,
				ring_index, QDMA_CTXT_CMD_CLR,
				QDMA_CTXT_SEL_COAL, NULL, 0, 0);
			if (rv < 0) {
				pr_err("Failed to clear interrupt context, rv = %d\n",
					rv);
			}
#endif
		}
		i++;
	}

	kfree(xdev->intr_coal_list);
	pr_debug("dev %s interrupt coalescing ring teardown successful\n",
				dev_name(&xdev->conf.pdev->dev));
}

static void data_vector_handler(int irq, struct xlnx_dma_dev *xdev)
{
	int i;

	for (i = 0; i < xdev->num_vecs; i++) {
		if (xdev->msix[i].vector == irq) {
			xdev->dev_intr_info_list[i].intr_vec_map.intr_handler(i,
					irq, (void *)xdev);
			break;
		}
	}
}

static irqreturn_t irq_bottom(int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	data_vector_handler(irq, xdev);

	return IRQ_HANDLED;
}

static irqreturn_t irq_top(int irq, void *dev_id)
{
	struct xlnx_dma_dev *xdev = dev_id;

	if (xdev->conf.fp_q_isr_top_dev) {
		xdev->conf.fp_q_isr_top_dev((unsigned long)xdev,
					xdev->conf.uld);
	}

	return IRQ_WAKE_THREAD;
}


void intr_teardown(struct xlnx_dma_dev *xdev)
{
	int i = xdev->num_vecs;

	while (--i >= 0)
		free_irq(xdev->msix[i].vector, xdev);

	if (xdev->num_vecs)
		pci_disable_msix(xdev->conf.pdev);

	kfree(xdev->msix);
	kfree(xdev->dev_intr_info_list);
}


static int intr_vector_setup(struct xlnx_dma_dev *xdev, int idx,
			enum intr_type_list type, f_intr_handler handler)
{
	int rv;

	if (type == INTR_TYPE_ERROR)
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-error",
			 xdev->conf.name);
	else if (type == INTR_TYPE_USER)
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-user", xdev->conf.name);
	else if (type == INTR_TYPE_DATA)
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s-data", xdev->conf.name);
	else
		snprintf(xdev->dev_intr_info_list[idx].msix_name,
			 QDMA_DEV_NAME_MAXLEN + 16, "%s", xdev->conf.name);

	xdev->dev_intr_info_list[idx].intr_vec_map.intr_type = type;
	xdev->dev_intr_info_list[idx].intr_vec_map.intr_vec_index = idx;
	xdev->dev_intr_info_list[idx].intr_vec_map.intr_handler = handler;

	if (type == INTR_TYPE_DATA)
		rv = request_irq(xdev->msix[idx].vector, irq_bottom, 0,
				 xdev->dev_intr_info_list[idx].msix_name, xdev);
	else
		rv = request_threaded_irq(xdev->msix[idx].vector, irq_top,
					  irq_bottom, 0,
				  xdev->dev_intr_info_list[idx].msix_name,
				  xdev);

	pr_info("%s requesting IRQ vector #%d: vec %d, type %d, %s.\n",
			xdev->conf.name, idx, xdev->msix[idx].vector,
			type, xdev->dev_intr_info_list[idx].msix_name);

	if (rv) {
		pr_err("%s requesting IRQ vector #%d: vec %d failed %d.\n",
			xdev->conf.name, idx, xdev->msix[idx].vector, rv);
		return rv;
	}

	return 0;
}
#ifdef __PCI_MSI_VEC_COUNT__

#define msix_table_size(flags)	((flags & PCI_MSIX_FLAGS_QSIZE) + 1)

static int pci_msix_vec_count(struct pci_dev *dev)
{
	u16 control;

	if (!dev->msix_cap)
		return 0;

	pci_read_config_word(dev, dev->msix_cap + PCI_MSIX_FLAGS, &control);
	return msix_table_size(control);
}
#endif

int intr_setup(struct xlnx_dma_dev *xdev)
{
	int rv = 0;
	int i;

	if ((xdev->conf.qdma_drv_mode == POLL_MODE) ||
			(xdev->conf.qdma_drv_mode == LEGACY_INTR_MODE)) {
		goto exit;
	}
	xdev->num_vecs = pci_msix_vec_count(xdev->conf.pdev);
	pr_debug("dev %s, xdev->num_vecs = %d\n",
			dev_name(&xdev->conf.pdev->dev), xdev->num_vecs);

	if (!xdev->num_vecs) {
		pr_warn("MSI-X not supported, running in polled mode\n");
		return 0;
	}

	xdev->msix = kzalloc((sizeof(struct msix_entry) * xdev->num_vecs),
						GFP_KERNEL);
	if (!xdev->msix) {
		pr_err("dev %s xdev->msix OOM.\n",
			dev_name(&xdev->conf.pdev->dev));
		rv = -ENOMEM;
		goto exit;
	}

	xdev->dev_intr_info_list =
			kzalloc((sizeof(struct intr_info_t) * xdev->num_vecs),
					GFP_KERNEL);
	if (!xdev->dev_intr_info_list) {
		pr_err("dev %s xdev->dev_intr_info_list OOM.\n",
			dev_name(&xdev->conf.pdev->dev));
		rv = -ENOMEM;
		goto free_msix;
	}

	for (i = 0; i < xdev->num_vecs; i++) {
		xdev->msix[i].entry = i;
		INIT_LIST_HEAD(&xdev->dev_intr_info_list[i].intr_list);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	rv = pci_enable_msix_exact(xdev->conf.pdev, xdev->msix, xdev->num_vecs);
#else
	rv = pci_enable_msix(xdev->conf.pdev, xdev->msix, xdev->num_vecs);
#endif
	if (rv < 0) {
		pr_err("Error enabling MSI-X (%d)\n", rv);
		goto free_intr_info;
	}

	/** On master PF0, vector#0 is dedicated for Error interrupts and
	 * vector #1 is dedicated for User interrupts
	 * For all other PFs and VFs, vector#0 is dedicated for User interrupts
	 * The remaining vectors are for Data interrupts
	 */
	i = 0;
#ifndef __QDMA_VF__
	/* global error interrupt */
	if (xdev->conf.master_pf) {
		rv = intr_vector_setup(xdev, 0, INTR_TYPE_ERROR,
					error_intr_handler);
		if (rv)
			goto cleanup_irq;
		i = 1;
	}
#endif

	/* user interrupt */
	rv = intr_vector_setup(xdev, i, INTR_TYPE_USER, user_intr_handler);
	if (rv)
		goto cleanup_irq;

	/* data interrupt */
	xdev->dvec_start_idx = ++i;
	for (; i < xdev->num_vecs; i++) {
		rv = intr_vector_setup(xdev, i, INTR_TYPE_DATA,
					data_intr_handler);
		if (rv)
			goto cleanup_irq;
	}

	xdev->flags |= XDEV_FLAG_IRQ;
	return rv;

cleanup_irq:
	while (--i >= 0)
		free_irq(xdev->msix[i].vector, xdev);

	pci_disable_msix(xdev->conf.pdev);
	xdev->num_vecs = 0;
free_intr_info:
	kfree(xdev->dev_intr_info_list);
free_msix:
	kfree(xdev->msix);
exit:
	return rv;
}

#ifndef __QDMA_VF__
static irqreturn_t irq_legacy(int irq, void *param)
{
	struct list_head *entry, *tmp;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)param;
	irqreturn_t ret = IRQ_NONE;

	if (!xdev) {
		pr_err("Invalid Xdev");
		goto irq_return;
	}

	spin_lock_irqsave(&legacy_intr_lock, legacy_intr_flags);
	if (__read_reg(xdev, QDMA_GLBL_INTERRUPT_CFG) &
			QDMA_GLBL_INTERRUPT_LGCY_INTR_PEND) {

		list_for_each_safe(entry, tmp, &legacy_intr_q_list) {
			struct qdma_descq *descq =
					container_of(entry,
						     struct qdma_descq,
						     legacy_intr_q_list);

			qdma_descq_service_cmpl_update(descq, 0, 1);
		}
		__write_reg(xdev, QDMA_GLBL_INTERRUPT_CFG,
			    QDMA_GLBL_INTERRUPT_CFG_EN_LGCY_INTR |
			    QDMA_GLBL_INTERRUPT_LGCY_INTR_PEND);
		ret = IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&legacy_intr_lock, legacy_intr_flags);

irq_return:
	return ret;
}

void intr_legacy_clear(struct qdma_descq *descq)
{

	if (!descq) {
		pr_err("Invalid descq received");
		return;
	}
	list_del(&descq->legacy_intr_q_list);

	if (list_empty(&legacy_intr_q_list)) {

		pr_info("un-registering legacy interrupt from qdma%05x\n",
			descq->xdev->conf.bdf);
		__write_reg(descq->xdev, QDMA_GLBL_INTERRUPT_CFG,
			    QDMA_GLBL_INTERRUPT_LGCY_INTR_PEND);
		free_irq(descq->xdev->conf.pdev->irq, descq->xdev);
	}
}

int intr_legacy_setup(struct qdma_descq *descq)
{
	int req_irq = 0;
	int rv = 0;

	if (!descq) {
		pr_err("Invalid descq received");
		return -EINVAL;
	}

	spin_lock(&legacy_q_add_lock);
	req_irq = list_empty(&legacy_intr_q_list);
	rv = req_irq ? 0 : 1;

	if (req_irq != 0) {
		spin_lock_init(&legacy_intr_lock);
		pr_debug("registering legacy interrupt for irq-%d from qdma%05x\n",
			descq->xdev->conf.pdev->irq, descq->xdev->conf.bdf);
		__write_reg(descq->xdev, QDMA_GLBL_INTERRUPT_CFG,
			    QDMA_GLBL_INTERRUPT_LGCY_INTR_PEND);
		rv = request_threaded_irq(descq->xdev->conf.pdev->irq, irq_top,
					  irq_legacy, legacy_intr_flags,
					  "qdma legacy intr",
					  descq->xdev);

		if (rv < 0)
			goto exit_intr_setup;
		else {
			list_add_tail(&descq->legacy_intr_q_list,
				      &legacy_intr_q_list);
			rv = 0;
		}
		__write_reg(descq->xdev, QDMA_GLBL_INTERRUPT_CFG,
			    QDMA_GLBL_INTERRUPT_CFG_EN_LGCY_INTR);
	} else
		list_add_tail(&descq->legacy_intr_q_list,
			      &legacy_intr_q_list);

exit_intr_setup:
	spin_unlock(&legacy_q_add_lock);
	return rv;
}
#endif

int intr_ring_setup(struct xlnx_dma_dev *xdev)
{
	int num_entries = 0;
	int counter = 0;
	struct intr_coal_conf  *intr_coal_list;
	struct intr_coal_conf  *intr_coal_list_entry;

	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		pr_debug("skipping interrupt aggregation: driver is loaded in %s mode\n",
			mode_name_list[xdev->conf.qdma_drv_mode].name);
		xdev->intr_coal_list = NULL;
		return 0;
	}

	/** For master_pf, vec0 and vec1 is used for
	 *  error and user interrupts
	 *  for other pfs, vec0 is used for user interrupts
	 */
	if ((xdev->num_vecs != 0) &&
	    ((xdev->num_vecs - xdev->dvec_start_idx) < xdev->conf.qsets_max)) {
		pr_debug("dev %s num_vectors[%d] < num_queues [%d]\n",
					dev_name(&xdev->conf.pdev->dev),
					xdev->num_vecs,
					xdev->conf.qsets_max);
		pr_debug("Enabling Interrupt aggregation\n");

		/** obtain the number of queue entries
		 * in each inr_ring based on ring size
		 */
		num_entries = ((xdev->conf.intr_rngsz + 1) * 512);

		pr_debug("%s interrupt coalescing ring with %d entries\n",
			dev_name(&xdev->conf.pdev->dev), num_entries);
		/**
		 * Initially assuming that each vector has the same size of the
		 * ring, In practical it is possible to have different ring
		 * size of different vectors (?)
		 */
		intr_coal_list = kzalloc(
				sizeof(struct intr_coal_conf) *
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT,
				GFP_KERNEL);
		if (!intr_coal_list) {
			pr_err("dev %s num_vecs %d OOM.\n",
				dev_name(&xdev->conf.pdev->dev),
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT);
			return -ENOMEM;
		}

		for (counter = 0;
			counter < QDMA_NUM_DATA_VEC_FOR_INTR_CXT;
			counter++) {
			intr_coal_list_entry = (intr_coal_list + counter);
			intr_coal_list_entry->intr_rng_num_entries =
							num_entries;
			intr_coal_list_entry->intr_ring_base = intr_ring_alloc(
					xdev, num_entries,
					sizeof(struct qdma_intr_ring),
					&intr_coal_list_entry->intr_ring_bus);
			if (!intr_coal_list_entry->intr_ring_base) {
				pr_err("dev %s, sz %u, intr_desc ring OOM.\n",
				xdev->conf.name,
				intr_coal_list_entry->intr_rng_num_entries);
				goto err_out;
			}

			intr_coal_list_entry->vec_id =
			xdev->msix[counter + xdev->dvec_start_idx].entry;
			intr_coal_list_entry->cidx = 0;
			intr_coal_list_entry->color = 1;
			pr_debug("ring_number = %d, vector_index = %d, ring_size = %d, ring_base = 0x%08x",
			    counter, intr_coal_list_entry->vec_id,
			    intr_coal_list_entry->intr_rng_num_entries,
			    (unsigned int)intr_coal_list_entry->intr_ring_bus);
		}

		pr_debug("dev %s interrupt coalescing ring setup successful\n",
					dev_name(&xdev->conf.pdev->dev));

		xdev->intr_coal_list = intr_coal_list;
	} else {
		pr_info("dev %s intr vec[%d] >= queues[%d], No aggregation\n",
			dev_name(&xdev->conf.pdev->dev),
			(xdev->num_vecs - xdev->dvec_start_idx),
			xdev->conf.qsets_max);

		xdev->intr_coal_list = NULL;
		/* Fallback from indirect interrupt mode */
		if (xdev->num_vecs != 0)
			xdev->conf.qdma_drv_mode = DIRECT_INTR_MODE;
	        else
	        	xdev->conf.qdma_drv_mode = POLL_MODE;
	}
	return 0;

err_out:
	while (--counter >= 0) {
		intr_coal_list_entry = (intr_coal_list + counter);
		intr_ring_free(xdev, intr_coal_list_entry->intr_rng_num_entries,
				sizeof(struct qdma_intr_ring),
				(u8 *)intr_coal_list_entry->intr_ring_base,
				intr_coal_list_entry->intr_ring_bus);
	}
	kfree(intr_coal_list);
	return -ENOMEM;
}

void intr_work(struct work_struct *work)
{
	struct qdma_descq *descq;

	descq = container_of(work, struct qdma_descq, work);
	qdma_descq_service_cmpl_update(descq, 0, 1);
}

/**
 * qdma_queue_service - service the queue
 * in the case of irq handler is registered by the user, the user should
 * call qdma_queue_service() in its interrupt handler to service the queue
 * @dev_hndl: hndl retured from qdma_device_open()
 * @qhndl: hndl retured from qdma_queue_add()
 */
void qdma_queue_service(unsigned long dev_hndl, unsigned long id, int budget,
			bool c2h_upd_cmpl)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq = qdma_device_get_descq_by_id(xdev, id,
							NULL, 0, 1);

	if (descq)
		qdma_descq_service_cmpl_update(descq, budget, c2h_upd_cmpl);
}

static u8 get_intr_vec_index(struct xlnx_dma_dev *xdev, u8 intr_type)
{
	u8 i = 0;

	for (i = 0; i < xdev->num_vecs; i++) {
		if (xdev->dev_intr_info_list[i].intr_vec_map.intr_type ==
		    intr_type)
			return xdev->dev_intr_info_list[i].intr_vec_map.intr_vec_index;
	}
	return 0;
}

void qdma_err_intr_setup(struct xlnx_dma_dev *xdev, u8 rearm)
{
	u32 val = 0;
	u8  err_intr_index = 0;
	u8 i;

	val = xdev->func_id;
	err_intr_index = get_intr_vec_index(xdev, INTR_TYPE_ERROR);
	val |= V_QDMA_C2H_ERR_INT_VEC(err_intr_index);

	val |= (1 << S_QDMA_C2H_ERR_INT_F_ERR_INT_ARM);

	__write_reg(xdev, QDMA_C2H_ERR_INT, val);

	if (rearm)
		return;

	pr_debug("Error interrupt setup: val = 0x%08x,  readback =  0x%08x err_intr_index = %d func_id = %d\n",
			val, __read_reg(xdev, QDMA_C2H_ERR_INT),
			err_intr_index, xdev->func_id);

	for (i = 0; i < HW_ERRS; i++)
		qdma_enable_hw_err(xdev, i);
}


void qdma_enable_hw_err(struct xlnx_dma_dev *xdev, u8 hw_err_type)
{

	switch (hw_err_type) {
	case GLBL_ERR:
	case GLBL_DSC_ERR:
	case GLBL_TRQ_ERR:
	case C2H_ERR:
	case C2H_FATAL_ERR:
	case H2C_ERR:
	case ECC_SB_ERR:
	case ECC_DB_ERR:
		break;
	default:
		hw_err_type = 0;
		break;
	}

	__write_reg(xdev,
	    err_stat_info[hw_err_type].mask_reg_addr,
		err_stat_info[hw_err_type].err_info.intr_mask);
	pr_info("%s interrupts enabled: reg -> 0x%08x,  value =  0x%08x\n",
		 err_stat_info[hw_err_type].err_name,
		 err_stat_info[hw_err_type].mask_reg_addr,
		 __read_reg(xdev, err_stat_info[hw_err_type].mask_reg_addr));
}


int get_intr_ring_index(struct xlnx_dma_dev *xdev, u32 vector_index)
{
	int ring_index = 0;

	ring_index = (vector_index - xdev->dvec_start_idx) +
			(xdev->func_id * QDMA_NUM_DATA_VEC_FOR_INTR_CXT);
	pr_debug("func_id = %d, vector_index = %d, ring_index = %d\n",
			xdev->func_id, vector_index, ring_index);

	return ring_index;
}

void intr_legacy_init(void)
{
#ifndef __QDMA_VF__
	spin_lock_init(&legacy_q_add_lock);
#endif
}
