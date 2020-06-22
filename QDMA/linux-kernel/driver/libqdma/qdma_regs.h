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

#ifndef __QDMA_REGS_H__
#define __QDMA_REGS_H__

#include <linux/types.h>
#include "xdev.h"

#define QDMA_REG_SZ_IN_BYTES	4

#define DESC_SZ_64B         3
#define DESC_SZ_8B_BYTES    8
#define DESC_SZ_16B_BYTES   16
#define DESC_SZ_32B_BYTES   32
#define DESC_SZ_64B_BYTES   64

#ifndef __QDMA_VF__

/*
 * monitor
 */
#define QDMA_REG_C2H_STAT_AXIS_PKG_CMP		0xA94

#endif /* ifndef __QDMA_VF__ */

/*
 * descriptor & writeback status
 */
/**
 * @struct - qdma_mm_desc
 * @brief	memory mapped descriptor format
 */
struct qdma_mm_desc {
	/** source address */
	__be64 src_addr;
	/** flags */
	__be32 flag_len;
	/** reserved 32 bits */
	__be32 rsvd0;
	/** destination address */
	__be64 dst_addr;
	/** reserved 64 bits */
	__be64 rsvd1;
};

#define S_DESC_F_DV		    28
#define S_DESC_F_SOP		29
#define S_DESC_F_EOP		30


#define S_H2C_DESC_F_SOP		1
#define S_H2C_DESC_F_EOP		2


#define S_H2C_DESC_NUM_GL		0
#define M_H2C_DESC_NUM_GL		0x7U
#define V_H2C_DESC_NUM_GL(x)	((x) << S_H2C_DESC_NUM_GL)

#define S_H2C_DESC_NUM_CDH		3
#define M_H2C_DESC_NUM_CDH		0xFU
#define V_H2C_DESC_NUM_CDH(x)	((x) << S_H2C_DESC_NUM_CDH)

#define S_H2C_DESC_F_ZERO_CDH		13
#define S_H2C_DESC_F_EOT			14
#define S_H2C_DESC_F_REQ_CMPL_STATUS	15

/* FIXME pld_len and flags members are part of custom descriptor format needed
 * by example design for ST loopback and desc bypass
 */
/**
 * @struct - qdma_h2c_desc
 * @brief	memory mapped descriptor format
 */
struct qdma_h2c_desc {
	__be16 cdh_flags;	/**< cdh flags */
	__be16 pld_len;		/**< current packet length */
	__be16 len;			/**< total packet length */
	__be16 flags;		/**< descriptor flags */
	__be64 src_addr;	/**< source address */
};

/**
 * @struct - qdma_c2h_desc
 * @brief	qdma c2h descriptor
 */
struct qdma_c2h_desc {
	__be64 dst_addr;	/**< destination address */
};

/**
 * @struct - qdma_desc_cmpl_status
 * @brief	qdma writeback descriptor
 */
struct qdma_desc_cmpl_status {
	__be16 pidx;	/**< producer index */
	__be16 cidx;	/**< consumer index */
	__be32 rsvd;	/**< reserved 32 bits */
};

#define S_C2H_CMPT_ENTRY_F_FORMAT		0
#define F_C2H_CMPT_ENTRY_F_FORMAT		(1 << S_C2H_CMPT_ENTRY_F_FORMAT)
#define		DFORMAT0_CMPL_MASK	0xF	/* udd starts at bit 4 */
#define		DFORMAT1_CMPL_MASK	0xFFFFF	/* udd starts at bit 20 */


#define S_C2H_CMPT_ENTRY_F_COLOR		1
#define F_C2H_CMPT_ENTRY_F_COLOR		(1 << S_C2H_CMPT_ENTRY_F_COLOR)

#define S_C2H_CMPT_ENTRY_F_ERR		2
#define F_C2H_CMPT_ENTRY_F_ERR		(1 << S_C2H_CMPT_ENTRY_F_ERR)

#define S_C2H_CMPT_ENTRY_F_DESC_USED	3
#define F_C2H_CMPT_ENTRY_F_DESC_USED	(1 << S_C2H_CMPT_ENTRY_F_DESC_USED)

#define S_C2H_CMPT_ENTRY_LENGTH			4
#define M_C2H_CMPT_ENTRY_LENGTH			0xFFFFU
#define L_C2H_CMPT_ENTRY_LENGTH			16
#define V_C2H_CMPT_ENTRY_LENGTH(x)	\
	(((x) & M_C2H_CMPT_ENTRY_LENGTH) << S_C2H_CMPT_ENTRY_LENGTH)

#define S_C2H_CMPT_ENTRY_F_EOT			20
#define F_C2H_CMPT_ENTRY_F_EOT			(1 << S_C2H_CMPT_ENTRY_F_EOT)

#define S_C2H_CMPT_ENTRY_F_USET_INTR		21

#define S_C2H_CMPT_USER_DEFINED			22
#define V_C2H_CMPT_USER_DEFINED(x)		((x) << S_C2H_CMPT_USER_DEFINED)

#define M_C2H_CMPT_ENTRY_DMA_INFO		0xFFFFFF
#define L_C2H_CMPT_ENTRY_DMA_INFO		3 /* 20 bits */
/**
 * @struct - qdma_c2h_cmpt_cmpl_status
 * @brief	qdma completion data descriptor
 */
struct qdma_c2h_cmpt_cmpl_status {
	__be16 pidx;				/**< producer index */
	__be16 cidx;				/**< consumer index */
	__be32 color_isr_status;	/**< isr color and status */
};
#define S_C2H_CMPT_F_COLOR	0

#define S_C2H_CMPT_INT_STATE	1
#define M_C2H_CMPT_INT_STATE	0x3U

/*
 * HW API
 */

#include "xdev.h"

#define __read_reg(xdev, reg_addr) (readl(xdev->regs + reg_addr))
#ifdef DEBUG__
#define __write_reg(xdev, reg_addr, val) \
	do { \
		pr_debug("%s, reg 0x%x, val 0x%x.\n", \
				xdev->conf.name, reg_addr, (u32)val); \
		writel(val, xdev->regs + reg_addr); \
	} while (0)
#else
#define __write_reg(xdev, reg_addr, val) (writel(val, xdev->regs + reg_addr))
#endif /* #ifdef DEBUG__ */

#ifndef __QDMA_VF__
void qdma_device_attributes_get(struct xlnx_dma_dev *xdev);

#endif /* #ifndef __QDMA_VF__ */

#endif
