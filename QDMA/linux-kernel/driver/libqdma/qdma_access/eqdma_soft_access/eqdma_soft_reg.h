/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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

#ifndef EQDMA_SOFT_REG_H_
#define EQDMA_SOFT_REG_H_

#include "qdma_soft_reg.h"
#ifdef __cplusplus
extern "C" {
#endif

/* H2C Throttle settings */
#define EQDMA_H2C_THROT_DATA_THRESH       0x5000
#define EQDMA_THROT_EN_DATA               1
#define EQDMA_THROT_EN_REQ                0
#define EQDMA_H2C_THROT_REQ_THRESH        0xC0

/** Software Context */
#define EQDMA_SW_CTXT_VIRTIO_DSC_BASE_GET_H_MASK    GENMASK_ULL(63, 53)
#define EQDMA_SW_CTXT_VIRTIO_DSC_BASE_GET_M_MASK    GENMASK_ULL(52, 21)
#define EQDMA_SW_CTXT_VIRTIO_DSC_BASE_GET_L_MASK    GENMASK_ULL(20, 0)

#define EQDMA_SW_CTXT_PASID_GET_H_MASK              GENMASK(21, 12)
#define EQDMA_SW_CTXT_PASID_GET_L_MASK              GENMASK(11, 0)

#define EQDMA_SW_CTXT_W7_VIRTIO_DSC_BASE_H_MASK     GENMASK(10, 0)
#define EQDMA_SW_CTXT_W6_VIRTIO_DSC_BASE_M_MASK     GENMASK(31, 0)
#define EQDMA_SW_CTXT_W5_VIRTIO_DSC_BASE_L_MASK     GENMASK(31, 11)
#define EQDMA_SW_CTXT_W5_PASID_EN_MASK              BIT(10)
#define EQDMA_SW_CTXT_W5_PASID_H_MASK               GENMASK(9, 0)

#define EQDMA_SW_CTXT_W4_PASID_L_MASK               GENMASK(31, 20)
#define EQDMA_SW_CTXT_W4_HOST_ID_MASK               GENMASK(19, 16)
#define EQDMA_SW_CTXT_W4_IRQ_BYP_MASK               BIT(15)
#define EQDMA_SW_CTXT_W4_PACK_BYP_OUT_MASK          BIT(14)
#define EQDMA_SW_CTXT_W4_VIRTIO_EN_MASK             BIT(13)
#define EQDMA_SW_CTXT_W4_DIS_INTR_VF_MASK           BIT(12)


/** Completion Context */
#define EQDMA_CMPL_CTXT_PASID_GET_H_MASK            GENMASK(21, 9)
#define EQDMA_CMPL_CTXT_PASID_GET_L_MASK            GENMASK(8, 0)

#define EQDMA_COMPL_CTXT_W5_SH_CMPT_MASK            BIT(19)
#define EQDMA_COMPL_CTXT_W5_VIO_EOP_MASK            BIT(18)
#define EQDMA_COMPL_CTXT_W5_BADDR4_LOW_MASK         GENMASK(17, 14)
#define EQDMA_COMPL_CTXT_W5_PASID_EN_MASK           BIT(13)
#define EQDMA_COMPL_CTXT_W5_PASID_H_MASK            GENMASK(12, 0)
#define EQDMA_COMPL_CTXT_W4_PASID_L_MASK            GENMASK(31, 23)
#define EQDMA_COMPL_CTXT_W4_HOST_ID_MASK            GENMASK(22, 19)
#define EQDMA_COMPL_CTXT_W4_DIR_C2H_MASK            BIT(18)
#define EQDMA_COMPL_CTXT_W4_VIO_MASK                BIT(17)
#define EQDMA_COMPL_CTXT_W4_DIS_INTR_VF_MASK        BIT(16)


/** Interrupt Context */
#define EQDMA_INTR_CTXT_PASID_GET_H_MASK            GENMASK(21, 9)
#define EQDMA_INTR_CTXT_PASID_GET_L_MASK            GENMASK(8, 0)

#define EQDMA_INTR_CTXT_W3_FUNC_ID_MASK             GENMASK(29, 18)
#define EQDMA_INTR_CTXT_W3_PASID_EN_MASK            BIT(13)
#define EQDMA_INTR_CTXT_W3_PASID_H_MASK             GENMASK(12, 0)
#define EQDMA_INTR_CTXT_W2_PASID_L_MASK             GENMASK(31, 23)
#define EQDMA_INTR_CTXT_W2_HOST_ID_MASK             GENMASK(22, 19)

/** Prefetch Context */
#define EQDMA_PFTCH_CTXT_W0_NUM_PFTCH_MASK          GENMASK(18, 9)
#define EQDMA_PFTCH_CTXT_W0_VAR_DESC_MASK           BIT(8)

/* ------------------------- Hardware Errors ------------------------------ */
#define EQDMA_TOTAL_LEAF_ERROR_AGGREGATORS                        9

#define EQDMA_OFFSET_GLBL_ERR_INT                       0XB04
#define     EQDMA_GLBL_ERR_FUNC_MASK                    GENMASK(11, 0)
#define     EQDMA_GLBL_ERR_VEC_MASK                     GENMASK(22, 12)
#define     EQDMA_GLBL_ERR_ARM_MASK                     BIT(24)
#define     EQDMA_GLBL_ERR_COAL_MASK                    BIT(23)
#define     EQDMA_GLBL_ERR_DIS_INTR_ON_VF_MASK          BIT(25)
#define     EQDMA_GLBL_ERR_HOST_ID_MASK                 BIT(25)


#define EQDMA_OFFSET_GLBL_ERR_STAT                      0X248
#define EQDMA_OFFSET_GLBL_ERR_MASK                      0X24C
#define     EQDMA_GLBL_ERR_RAM_SBE_MASK                 BIT(0)
#define     EQDMA_GLBL_ERR_RAM_DBE_MASK                 BIT(1)
#define     EQDMA_GLBL_ERR_DSC_MASK                     BIT(2)
#define     EQDMA_GLBL_ERR_TRQ_MASK                     BIT(3)
#define     EQDMA_GLBL_ERR_H2C_MM_0_MASK                BIT(4)
#define     EQDMA_GLBL_ERR_H2C_MM_1_MASK                BIT(5)
#define     EQDMA_GLBL_ERR_C2H_MM_0_MASK                BIT(6)
#define     EQDMA_GLBL_ERR_C2H_MM_1_MASK                BIT(7)
#define     EQDMA_GLBL_ERR_ST_C2H_MASK                  BIT(8)
#define     EQDMA_GLBL_ERR_BDG_MASK                     BIT(15)
#define     EQDMA_GLBL_ERR_IND_CTXT_CMD_MASK            GENMASK(14, 9)
#define     EQDMA_GLBL_ERR_ST_H2C_MASK                  BIT(16)

#define EQDMA_OFFSET_C2H_ERR_STAT                       0XAF0
#define EQDMA_OFFSET_C2H_ERR_MASK                       0XAF4
#define     EQDMA_C2H_ERR_MTY_MISMATCH_MASK             BIT(0)
#define     EQDMA_C2H_ERR_LEN_MISMATCH_MASK             BIT(1)
#define     EQDMA_C2H_ERR_SH_CMPT_DSC_MASK              BIT(2)
#define     EQDMA_C2H_ERR_QID_MISMATCH_MASK             BIT(3)
#define     EQDMA_C2H_ERR_DESC_RSP_ERR_MASK             BIT(4)
#define     EQDMA_C2H_ERR_ENG_WPL_DATA_PAR_ERR_MASK     BIT(6)
#define     EQDMA_C2H_ERR_MSI_INT_FAIL_MASK             BIT(7)
#define     EQDMA_C2H_ERR_ERR_DESC_CNT_MASK             BIT(9)
#define     EQDMA_C2H_ERR_PORTID_CTXT_MISMATCH_MASK     BIT(10)
#define     EQDMA_C2H_ERR_CMPT_INV_Q_ERR_MASK           BIT(12)
#define     EQDMA_C2H_ERR_CMPT_QFULL_ERR_MASK           BIT(13)
#define     EQDMA_C2H_ERR_CMPT_CIDX_ERR_MASK            BIT(14)
#define     EQDMA_C2H_ERR_CMPT_PRTY_ERR_MASK            BIT(15)
#define     EQDMA_C2H_ERR_AVL_RING_DSC_MASK             BIT(16)
#define     EQDMA_C2H_ERR_HDR_ECC_UNC_MASK              BIT(17)
#define     EQDMA_C2H_ERR_HDR_ECC_COR_MASK              BIT(18)
#define     EQDMA_C2H_ERR_ALL_MASK                      0X3F6DF

#define EQDMA_OFFSET_C2H_FATAL_ERR_STAT                       0XAF8
#define EQDMA_OFFSET_C2H_FATAL_ERR_MASK                       0XAFC
#define     EQDMA_C2H_FATAL_ERR_MTY_MISMATCH_MASK             BIT(0)
#define     EQDMA_C2H_FATAL_ERR_LEN_MISMATCH_MASK             BIT(1)
#define     EQDMA_C2H_FATAL_ERR_QID_MISMATCH_MASK             BIT(3)
#define     EQDMA_C2H_FATAL_ERR_TIMER_FIFO_RAM_RDBE_MASK      BIT(4)
#define     EQDMA_C2H_FATAL_ERR_PFCH_II_RAM_RDBE_MASK         BIT(8)
#define     EQDMA_C2H_FATAL_ERR_CMPT_CTXT_RAM_RDBE_MASK       BIT(9)
#define     EQDMA_C2H_FATAL_ERR_PFCH_CTXT_RAM_RDBE_MASK       BIT(10)
#define     EQDMA_C2H_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE_MASK   BIT(11)
#define     EQDMA_C2H_FATAL_ERR_INT_CTXT_RAM_RDBE_MASK        BIT(12)
#define     EQDMA_C2H_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE_MASK  BIT(14)
#define     EQDMA_C2H_FATAL_ERR_CMPT_FIFO_RAM_RDBE_MASK       BIT(15)
#define     EQDMA_C2H_FATAL_ERR_QID_FIFO_RAM_RDBE_MASK        BIT(16)
#define     EQDMA_C2H_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE_MASK    BIT(17)
#define     EQDMA_C2H_FATAL_ERR_WPL_DATA_PAR_MASK             BIT(18)
#define     EQDMA_C2H_FATAL_ERR_AVL_RING_FIFO_RAM_RDBE_MASK   BIT(19)
#define     EQDMA_C2H_FATAL_ERR_HDR_ECC_UNC_MASK              BIT(20)
#define     EQDMA_C2H_FATAL_ERR_ALL_MASK                      0X1FDF1B

#define EQDMA_OFFSET_H2C_ERR_STAT                             0XE00
#define EQDMA_OFFSET_H2C_ERR_MASK                             0XE04
#define     EQDMA_H2C_ERR_ZERO_LEN_DESC_MASK                  BIT(0)
#define     EQDMA_H2C_ERR_SDI_MRKR_REQ_MOP_MASK               BIT(1)
#define     EQDMA_H2C_ERR_NO_DMA_DSC_MASK                     BIT(2)
#define     EQDMA_H2C_ERR_SBE_MASK                            BIT(3)
#define     EQDMA_H2C_ERR_DBE_MASK                            BIT(4)
#define     EQDMA_H2C_ERR_PAR_ERR_MASK                        BIT(5)
#define     EQDMA_H2C_ERR_ALL_MASK                            0X3F

#define EQDMA_OFFSET_GLBL_DSC_ERR_STAT                       0X254
#define EQDMA_OFFSET_GLBL_DSC_ERR_MASK                       0X258
#define     EQDMA_GLBL_DSC_ERR_POISON_MASK                   BIT(1)
#define     EQDMA_GLBL_DSC_ERR_UR_CA_MASK                    BIT(2)
#define     EQDMA_GLBL_DSC_ERR_BCNT_MASK                     BIT(3)
#define     EQDMA_GLBL_DSC_ERR_PARAM_MASK                    BIT(4)
#define     EQDMA_GLBL_DSC_ERR_ADDR_MASK                     BIT(5)
#define     EQDMA_GLBL_DSC_ERR_TAG_MASK                      BIT(6)
#define     EQDMA_GLBL_DSC_ERR_FLR_MASK                      BIT(8)
#define     EQDMA_GLBL_DSC_ERR_TIMEOUT_MASK                  BIT(9)
#define     EQDMA_GLBL_DSC_ERR_DAT_POISON_MASK               BIT(16)
#define     EQDMA_GLBL_DSC_ERR_FLR_CANCEL_MASK               BIT(19)
#define     EQDMA_GLBL_DSC_ERR_DMA_MASK                      BIT(20)
#define     EQDMA_GLBL_DSC_ERR_DSC_MASK                      BIT(21)
#define     EQDMA_GLBL_DSC_ERR_RQ_CANCEL_MASK                BIT(22)
#define     EQDMA_GLBL_DSC_ERR_DBE_MASK                      BIT(23)
#define     EQDMA_GLBL_DSC_ERR_SBE_MASK                      BIT(24)
#define     EQDMA_GLBL_DSC_ERR_ALL_MASK                      0X1F9037E

#define EQDMA_OFFSET_GLBL_TRQ_ERR_STAT                       0X264
#define EQDMA_OFFSET_GLBL_TRQ_ERR_MASK                       0X268
#define     EQDMA_GLBL_TRQ_ERR_CSR_UNMAPPED_MASK             BIT(0)
#define     EQDMA_GLBL_TRQ_ERR_VF_ACCESS_MASK                BIT(1)
#define     EQDMA_GLBL_TRQ_ERR_TCP_CSR_MASK                  BIT(3)
#define     EQDMA_GLBL_TRQ_ERR_QSPC_UNMAPPED_MASK            BIT(4)
#define     EQDMA_GLBL_TRQ_ERR_QID_RANGE_MASK                BIT(5)
#define     EQDMA_GLBL_TRQ_ERR_TCP_QSPC_TIMEOUT_MASK         BIT(7)
#define     EQDMA_GLBL_TRQ_ERR_ALL_MASK                      0XB3

#define EQDMA_OFFSET_RAM_SBE_1_STAT                          0XE4
#define EQDMA_OFFSET_RAM_SBE_1_MASK                          0XE0
#define     EQDMA_SBE_1_ERR_RC_RRQ_EVEN_RAM_MASK             BIT(0)
#define     EQDMA_SBE_1_ERR_TAG_ODD_RAM_MASK                 BIT(1)
#define     EQDMA_SBE_1_ERR_TAG_EVEN_RAM_MASK                BIT(2)
#define     EQDMA_SBE_1_ERR_PFCH_CTXT_CAM_RAM_0_MASK         BIT(3)
#define     EQDMA_SBE_1_ERR_PFCH_CTXT_CAM_RAM_1_MASK         BIT(4)
#define     EQDMA_SBE_1_ERR_ALL_MASK                         0X1F

#define EQDMA_OFFSET_RAM_DBE_1_STAT                          0XEC
#define EQDMA_OFFSET_RAM_DBE_1_MASK                          0XE8
#define     EQDMA_DBE_1_ERR_RC_RRQ_EVEN_RAM_MASK             BIT(0)
#define     EQDMA_DBE_1_ERR_TAG_ODD_RAM_MASK                 BIT(1)
#define     EQDMA_DBE_1_ERR_TAG_EVEN_RAM_MASK                BIT(2)
#define     EQDMA_DBE_1_ERR_PFCH_CTXT_CAM_RAM_0_MASK         BIT(3)
#define     EQDMA_DBE_1_ERR_PFCH_CTXT_CAM_RAM_1_MASK         BIT(4)
#define     EQDMA_DBE_1_ERR_ALL_MASK                         0X1F

#define EQDMA_OFFSET_RAM_SBE_STAT                            0XF4
#define EQDMA_OFFSET_RAM_SBE_MASK                            0XF0
#define     EQDMA_SBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     EQDMA_SBE_ERR_MI_H2C1_DAT_MASK                   BIT(1)
#define     EQDMA_SBE_ERR_MI_H2C2_DAT_MASK                   BIT(2)
#define     EQDMA_SBE_ERR_MI_H2C3_DAT_MASK                   BIT(3)
#define     EQDMA_SBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     EQDMA_SBE_ERR_MI_C2H1_DAT_MASK                   BIT(5)
#define     EQDMA_SBE_ERR_MI_C2H2_DAT_MASK                   BIT(6)
#define     EQDMA_SBE_ERR_MI_C2H3_DAT_MASK                   BIT(7)
#define     EQDMA_SBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(8)
#define     EQDMA_SBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(9)
#define     EQDMA_SBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(10)
#define     EQDMA_SBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(11)
#define     EQDMA_SBE_ERR_FUNC_MAP_MASK                      BIT(12)
#define     EQDMA_SBE_ERR_DSC_HW_CTXT_MASK                   BIT(13)
#define     EQDMA_SBE_ERR_DSC_CRD_RCV_MASK                   BIT(14)
#define     EQDMA_SBE_ERR_DSC_SW_CTXT_MASK                   BIT(15)
#define     EQDMA_SBE_ERR_DSC_CPLI_MASK                      BIT(16)
#define     EQDMA_SBE_ERR_DSC_CPLD_MASK                      BIT(17)
#define     EQDMA_SBE_ERR_MI_TL_SLV_FIFO_RAM_MASK            BIT(18)
#define     EQDMA_SBE_ERR_TIMER_FIFO_RAM_MASK                GENMASK(22, 19)
#define     EQDMA_SBE_ERR_QID_FIFO_RAM_MASK                  BIT(23)
#define     EQDMA_SBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT(24)
#define     EQDMA_SBE_ERR_INT_CTXT_RAM_MASK                  BIT(25)
#define     EQDMA_SBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(26)
#define     EQDMA_SBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(27)
#define     EQDMA_SBE_ERR_WRB_CTXT_RAM_MASK                  BIT(28)
#define     EQDMA_SBE_ERR_PFCH_LL_RAM_MASK                   BIT(29)
#define     EQDMA_SBE_ERR_PEND_FIFO_RAM_MASK                 BIT(30)
#define     EQDMA_SBE_ERR_RC_RRQ_ODD_RAM_MASK                BIT(31)
#define     EQDMA_SBE_ERR_ALL_MASK                           0XFFFFFFFF

#define EQDMA_OFFSET_RAM_DBE_STAT                            0XFC
#define EQDMA_OFFSET_RAM_DBE_MASK                            0XF8
#define     EQDMA_DBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     EQDMA_DBE_ERR_MI_H2C1_DAT_MASK                   BIT(1)
#define     EQDMA_DBE_ERR_MI_H2C2_DAT_MASK                   BIT(2)
#define     EQDMA_DBE_ERR_MI_H2C3_DAT_MASK                   BIT(3)
#define     EQDMA_DBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     EQDMA_DBE_ERR_MI_C2H1_DAT_MASK                   BIT(5)
#define     EQDMA_DBE_ERR_MI_C2H2_DAT_MASK                   BIT(6)
#define     EQDMA_DBE_ERR_MI_C2H3_DAT_MASK                   BIT(7)
#define     EQDMA_DBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(8)
#define     EQDMA_DBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(9)
#define     EQDMA_DBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(10)
#define     EQDMA_DBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(11)
#define     EQDMA_DBE_ERR_FUNC_MAP_MASK                      BIT(12)
#define     EQDMA_DBE_ERR_DSC_HW_CTXT_MASK                   BIT(13)
#define     EQDMA_DBE_ERR_DSC_CRD_RCV_MASK                   BIT(14)
#define     EQDMA_DBE_ERR_DSC_SW_CTXT_MASK                   BIT(15)
#define     EQDMA_DBE_ERR_DSC_CPLI_MASK                      BIT(16)
#define     EQDMA_DBE_ERR_DSC_CPLD_MASK                      BIT(17)
#define     EQDMA_DBE_ERR_MI_TL_SLV_FIFO_RAM_MASK            BIT(18)
#define     EQDMA_DBE_ERR_TIMER_FIFO_RAM_MASK                GENMASK(22, 19)
#define     EQDMA_DBE_ERR_QID_FIFO_RAM_MASK                  BIT(23)
#define     EQDMA_DBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT(24)
#define     EQDMA_DBE_ERR_INT_CTXT_RAM_MASK                  BIT(25)
#define     EQDMA_DBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(26)
#define     EQDMA_DBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(27)
#define     EQDMA_DBE_ERR_WRB_CTXT_RAM_MASK                  BIT(28)
#define     EQDMA_DBE_ERR_PFCH_LL_RAM_MASK                   BIT(29)
#define     EQDMA_DBE_ERR_PEND_FIFO_RAM_MASK                 BIT(30)
#define     EQDMA_DBE_ERR_RC_RRQ_ODD_RAM_MASK                BIT(31)
#define     EQDMA_DBE_ERR_ALL_MASK                           0XFFFFFFFF

#define EQDMA_OFFSET_VF_VERSION                              0x5014
#define EQDMA_OFFSET_VF_USER_BAR                             0x5018

#define EQDMA_OFFSET_MBOX_BASE_VF                            0x5000
#define EQDMA_OFFSET_MBOX_BASE_PF                            0x22400

#ifdef __cplusplus
}
#endif

#endif /* EQDMA_SOFT_REG_H_ */
