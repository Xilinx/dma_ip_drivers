/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
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

#ifndef __XDMA_PCI_ID_H__
#define __XDMA_PCI_ID_H__
/**
 * @file
 * @brief This file contains the list of pcie devices supported for qdma driver
 *
 */

/**
 * list of pcie devices supported for qdma driver
 */
static const struct pci_device_id pci_ids[] = {

#ifdef __QDMA_VF__
	/** Gen 1 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xa011), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa111), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa211), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa311), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xa012), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa112), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa212), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa312), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xa014), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa114), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa214), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa314), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xa018), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa118), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa218), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa318), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xa01f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa11f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa21f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa31f), },	/** VF on PF 3 */

	/** Gen 2 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xa021), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa121), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa221), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa321), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xa022), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa122), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa222), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa322), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xa024), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa124), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa224), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa324), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xa028), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa128), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa228), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa328), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xa02f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa12f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa22f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa32f), },	/** VF on PF 3 */

	/** Gen 3 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xa031), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa131), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa231), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa331), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xa032), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa132), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa232), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa332), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xa034), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa134), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa234), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa334), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xa038), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa138), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa238), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa338), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xa03f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa13f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa23f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa33f), },	/** VF on PF 3 */

	/** Gen 4 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xa041), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa141), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa241), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa341), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xa042), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa142), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa242), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa342), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xa044), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa144), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa244), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa344), },	/** VF on PF 3 */
	{ PCI_DEVICE(0x10ee, 0xa444), },	/** VF on PF 4 */
	{ PCI_DEVICE(0x10ee, 0xa544), },	/** VF on PF 5 */
	{ PCI_DEVICE(0x10ee, 0xa644), },	/** VF on PF 6 */
	{ PCI_DEVICE(0x10ee, 0xa744), },	/** VF on PF 7 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xa048), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xa148), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xa248), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xa348), },	/** VF on PF 3 */

	/** Gen 1 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xc011), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc111), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc211), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc311), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xc012), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc112), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc212), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc312), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xc014), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc114), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc214), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc314), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xc018), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc118), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc218), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc318), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xc01f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc11f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc21f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc31f), },	/** VF on PF 3 */

	/** Gen 2 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xc021), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc121), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc221), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc321), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xc022), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc122), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc222), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc322), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xc024), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc124), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc224), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc324), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xc028), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc128), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc228), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc328), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xc02f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc12f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc22f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc32f), },	/** VF on PF 3 */

	/** Gen 3 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xc031), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc131), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc231), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc331), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xc032), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc132), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc232), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc332), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xc034), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc134), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc234), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc334), },	/** VF on PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xc038), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc138), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc238), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc338), },	/** VF on PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xc03f), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc13f), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc23f), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc33f), },	/** VF on PF 3 */

	/** Gen 4 VF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xc041), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc141), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc241), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc341), },	/** VF on PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xc042), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc142), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc242), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc342), },	/** VF on PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xc044), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc144), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc244), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc344), },	/** VF on PF 3 */
	{ PCI_DEVICE(0x10ee, 0xc444), },	/** VF on PF 4 */
	{ PCI_DEVICE(0x10ee, 0xc544), },	/** VF on PF 5 */
	{ PCI_DEVICE(0x10ee, 0xc644), },	/** VF on PF 6 */
	{ PCI_DEVICE(0x10ee, 0xc744), },	/** VF on PF 7 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xc048), },	/** VF on PF 0 */
	{ PCI_DEVICE(0x10ee, 0xc148), },	/** VF on PF 1 */
	{ PCI_DEVICE(0x10ee, 0xc248), },	/** VF on PF 2 */
	{ PCI_DEVICE(0x10ee, 0xc348), },	/** VF on PF 3 */
#else
	/** Gen 1 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9011), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9111), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9211), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9311), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9012), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9112), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9212), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9312), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9014), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9114), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9214), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9314), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9018), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9118), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9218), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9318), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0x901f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x911f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x921f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x931f), },	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9021), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9121), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9221), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9321), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9022), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9122), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9222), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9322), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9024), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9124), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9224), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9324), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9028), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9128), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9228), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9328), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0x902f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x912f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x922f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x932f), },	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9031), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9131), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9231), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9331), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9032), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9132), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9232), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9332), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9034), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9134), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9234), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9334), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9038), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9138), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9238), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9338), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0x903f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x913f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x923f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x933f), },	/** PF 3 */
	/* { PCI_DEVICE(0x10ee, 0x6a9f), }, */       /** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x6aa0), },	/** PF 1 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0x9041), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9141), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9241), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9341), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0x9042), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9142), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9242), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9342), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0x9044), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9144), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9244), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9344), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0x9048), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0x9148), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0x9248), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0x9348), },	/** PF 3 */

	/** Gen 1 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xb011), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb111), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb211), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb311), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xb012), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb112), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb212), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb312), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xb014), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb114), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb214), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb314), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xb018), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb118), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb218), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb318), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xb01f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb11f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb21f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb31f), },	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xb021), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb121), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb221), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb321), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xb022), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb122), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb222), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb322), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xb024), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb124), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb224), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb324), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xb028), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb128), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb228), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb328), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xb02f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb12f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb22f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb32f), },	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xb031), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb131), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb231), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb331), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xb032), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb132), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb232), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb332), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xb034), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb134), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb234), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb334), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xb038), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb138), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb238), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb338), },	/** PF 3 */
	/** PCIe lane width x16 */
	{ PCI_DEVICE(0x10ee, 0xb03f), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb13f), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb23f), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb33f), },	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	{ PCI_DEVICE(0x10ee, 0xb041), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb141), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb241), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb341), },	/** PF 3 */
	/** PCIe lane width x2 */
	{ PCI_DEVICE(0x10ee, 0xb042), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb142), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb242), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb342), },	/** PF 3 */
	/** PCIe lane width x4 */
	{ PCI_DEVICE(0x10ee, 0xb044), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb144), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb244), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb344), },	/** PF 3 */
	/** PCIe lane width x8 */
	{ PCI_DEVICE(0x10ee, 0xb048), },	/** PF 0 */
	{ PCI_DEVICE(0x10ee, 0xb148), },	/** PF 1 */
	{ PCI_DEVICE(0x10ee, 0xb248), },	/** PF 2 */
	{ PCI_DEVICE(0x10ee, 0xb348), },	/** PF 3 */
#endif

	{0,}
};

/** module device table */
MODULE_DEVICE_TABLE(pci, pci_ids);

#endif /* ifndef __XDMA_PCI_ID_H__ */
