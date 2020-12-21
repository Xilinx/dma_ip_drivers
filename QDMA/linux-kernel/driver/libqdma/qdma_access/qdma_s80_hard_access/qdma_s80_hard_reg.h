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
 */

#ifndef __QDMA_S80_HARD_REG_H
#define __QDMA_S80_HARD_REG_H


#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_platform.h"

#ifdef CHAR_BIT
#undef CHAR_BIT
#endif
#define CHAR_BIT 8

#ifdef BIT
#undef BIT
#endif
#define BIT(n)                  (1u << (n))

#ifdef BITS_PER_BYTE
#undef BITS_PER_BYTE
#endif
#define BITS_PER_BYTE           CHAR_BIT

#ifdef BITS_PER_LONG
#undef BITS_PER_LONG
#endif
#define BITS_PER_LONG           (sizeof(uint32_t) * BITS_PER_BYTE)

#ifdef BITS_PER_LONG_LONG
#undef BITS_PER_LONG_LONG
#endif
#define BITS_PER_LONG_LONG      (sizeof(uint64_t) * BITS_PER_BYTE)

#ifdef GENMASK
#undef GENMASK
#endif
#define GENMASK(h, l) \
	((0xFFFFFFFF << (l)) & (0xFFFFFFFF >> (BITS_PER_LONG - 1 - (h))))

#ifdef GENMASK_ULL
#undef GENMASK_ULL
#endif
#define GENMASK_ULL(h, l) \
	((0xFFFFFFFFFFFFFFFF << (l)) & \
			(0xFFFFFFFFFFFFFFFF >> (BITS_PER_LONG_LONG - 1 - (h))))

#define DEBGFS_LINE_SZ			(81)

#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif
#define ARRAY_SIZE(arr) (sizeof(arr) / \
							sizeof(arr[0]))


uint32_t qdma_s80_hard_config_num_regs_get(void);
struct xreg_info *qdma_s80_hard_config_regs_get(void);
#define QDMA_S80_HARD_CFG_BLK_IDENTIFIER_ADDR              0x00
#define CFG_BLK_IDENTIFIER_MASK                           GENMASK(31, 20)
#define CFG_BLK_IDENTIFIER_1_MASK                         GENMASK(19, 16)
#define CFG_BLK_IDENTIFIER_RSVD_1_MASK                     GENMASK(15, 8)
#define CFG_BLK_IDENTIFIER_VERSION_MASK                    GENMASK(7, 0)
#define QDMA_S80_HARD_CFG_BLK_BUSDEV_ADDR                  0x04
#define CFG_BLK_BUSDEV_BDF_MASK                            GENMASK(15, 0)
#define QDMA_S80_HARD_CFG_BLK_PCIE_MAX_PLD_SIZE_ADDR       0x08
#define CFG_BLK_PCIE_MAX_PLD_SIZE_MASK                    GENMASK(2, 0)
#define QDMA_S80_HARD_CFG_BLK_PCIE_MAX_READ_REQ_SIZE_ADDR  0x0C
#define CFG_BLK_PCIE_MAX_READ_REQ_SIZE_MASK               GENMASK(2, 0)
#define QDMA_S80_HARD_CFG_BLK_SYSTEM_ID_ADDR               0x10
#define CFG_BLK_SYSTEM_ID_MASK                            GENMASK(15, 0)
#define QDMA_S80_HARD_CFG_BLK_MSI_ENABLE_ADDR              0x014
#define CFG_BLK_MSI_ENABLE_3_MASK                          BIT(17)
#define CFG_BLK_MSI_ENABLE_MSIX3_MASK                      BIT(16)
#define CFG_BLK_MSI_ENABLE_2_MASK                          BIT(13)
#define CFG_BLK_MSI_ENABLE_MSIX2_MASK                      BIT(12)
#define CFG_BLK_MSI_ENABLE_1_MASK                          BIT(9)
#define CFG_BLK_MSI_ENABLE_MSIX1_MASK                      BIT(8)
#define CFG_BLK_MSI_ENABLE_0_MASK                          BIT(1)
#define CFG_BLK_MSI_ENABLE_MSIX0_MASK                      BIT(0)
#define QDMA_S80_HARD_CFG_PCIE_DATA_WIDTH_ADDR             0x18
#define CFG_PCIE_DATA_WIDTH_DATAPATH_MASK                  GENMASK(2, 0)
#define QDMA_S80_HARD_CFG_PCIE_CTL_ADDR                    0x1C
#define CFG_PCIE_CTL_RRQ_DISABLE_MASK                      BIT(1)
#define CFG_PCIE_CTL_RELAXED_ORDERING_MASK                 BIT(0)
#define QDMA_S80_HARD_CFG_AXI_USER_MAX_PLD_SIZE_ADDR       0x40
#define CFG_AXI_USER_MAX_PLD_SIZE_ISSUED_MASK              GENMASK(6, 4)
#define CFG_AXI_USER_MAX_PLD_SIZE_PROG_MASK                GENMASK(2, 0)
#define QDMA_S80_HARD_CFG_AXI_USER_MAX_READ_REQ_SIZE_ADDR  0x44
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_USISSUED_MASK       GENMASK(6, 4)
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_USPROG_MASK         GENMASK(2, 0)
#define QDMA_S80_HARD_CFG_BLK_MISC_CTL_ADDR                0x4C
#define CFG_BLK_MISC_CTL_NUM_TAG_MASK                      GENMASK(19, 8)
#define CFG_BLK_MISC_CTL_RQ_METERING_MULTIPLIER_MASK       GENMASK(4, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_0_ADDR               0x80
#define CFG_BLK_SCRATCH_0_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_1_ADDR               0x84
#define CFG_BLK_SCRATCH_1_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_2_ADDR               0x88
#define CFG_BLK_SCRATCH_2_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_3_ADDR               0x8C
#define CFG_BLK_SCRATCH_3_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_4_ADDR               0x90
#define CFG_BLK_SCRATCH_4_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_5_ADDR               0x94
#define CFG_BLK_SCRATCH_5_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_6_ADDR               0x98
#define CFG_BLK_SCRATCH_6_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_CFG_BLK_SCRATCH_7_ADDR               0x9C
#define CFG_BLK_SCRATCH_7_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_RAM_SBE_MSK_A_ADDR                   0xF0
#define RAM_SBE_MSK_A_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_RAM_SBE_STS_A_ADDR                   0xF4
#define RAM_SBE_STS_A_RSVD_1_MASK                          BIT(31)
#define RAM_SBE_STS_A_PFCH_LL_RAM_MASK                     BIT(30)
#define RAM_SBE_STS_A_WRB_CTXT_RAM_MASK                    BIT(29)
#define RAM_SBE_STS_A_PFCH_CTXT_RAM_MASK                   BIT(28)
#define RAM_SBE_STS_A_DESC_REQ_FIFO_RAM_MASK               BIT(27)
#define RAM_SBE_STS_A_INT_CTXT_RAM_MASK                    BIT(26)
#define RAM_SBE_STS_A_INT_QID2VEC_RAM_MASK                 BIT(25)
#define RAM_SBE_STS_A_WRB_COAL_DATA_RAM_MASK               BIT(24)
#define RAM_SBE_STS_A_TUSER_FIFO_RAM_MASK                  BIT(23)
#define RAM_SBE_STS_A_QID_FIFO_RAM_MASK                    BIT(22)
#define RAM_SBE_STS_A_PLD_FIFO_RAM_MASK                    BIT(21)
#define RAM_SBE_STS_A_TIMER_FIFO_RAM_MASK                  BIT(20)
#define RAM_SBE_STS_A_PASID_CTXT_RAM_MASK                  BIT(19)
#define RAM_SBE_STS_A_DSC_CPLD_MASK                        BIT(18)
#define RAM_SBE_STS_A_DSC_CPLI_MASK                        BIT(17)
#define RAM_SBE_STS_A_DSC_SW_CTXT_MASK                     BIT(16)
#define RAM_SBE_STS_A_DSC_CRD_RCV_MASK                     BIT(15)
#define RAM_SBE_STS_A_DSC_HW_CTXT_MASK                     BIT(14)
#define RAM_SBE_STS_A_FUNC_MAP_MASK                        BIT(13)
#define RAM_SBE_STS_A_C2H_WR_BRG_DAT_MASK                  BIT(12)
#define RAM_SBE_STS_A_C2H_RD_BRG_DAT_MASK                  BIT(11)
#define RAM_SBE_STS_A_H2C_WR_BRG_DAT_MASK                  BIT(10)
#define RAM_SBE_STS_A_H2C_RD_BRG_DAT_MASK                  BIT(9)
#define RAM_SBE_STS_A_RSVD_2_MASK                          GENMASK(8, 5)
#define RAM_SBE_STS_A_MI_C2H0_DAT_MASK                     BIT(4)
#define RAM_SBE_STS_A_RSVD_3_MASK                          GENMASK(3, 1)
#define RAM_SBE_STS_A_MI_H2C0_DAT_MASK                     BIT(0)
#define QDMA_S80_HARD_RAM_DBE_MSK_A_ADDR                   0xF8
#define RAM_DBE_MSK_A_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_RAM_DBE_STS_A_ADDR                   0xFC
#define RAM_DBE_STS_A_RSVD_1_MASK                          BIT(31)
#define RAM_DBE_STS_A_PFCH_LL_RAM_MASK                     BIT(30)
#define RAM_DBE_STS_A_WRB_CTXT_RAM_MASK                    BIT(29)
#define RAM_DBE_STS_A_PFCH_CTXT_RAM_MASK                   BIT(28)
#define RAM_DBE_STS_A_DESC_REQ_FIFO_RAM_MASK               BIT(27)
#define RAM_DBE_STS_A_INT_CTXT_RAM_MASK                    BIT(26)
#define RAM_DBE_STS_A_INT_QID2VEC_RAM_MASK                 BIT(25)
#define RAM_DBE_STS_A_WRB_COAL_DATA_RAM_MASK               BIT(24)
#define RAM_DBE_STS_A_TUSER_FIFO_RAM_MASK                  BIT(23)
#define RAM_DBE_STS_A_QID_FIFO_RAM_MASK                    BIT(22)
#define RAM_DBE_STS_A_PLD_FIFO_RAM_MASK                    BIT(21)
#define RAM_DBE_STS_A_TIMER_FIFO_RAM_MASK                  BIT(20)
#define RAM_DBE_STS_A_PASID_CTXT_RAM_MASK                  BIT(19)
#define RAM_DBE_STS_A_DSC_CPLD_MASK                        BIT(18)
#define RAM_DBE_STS_A_DSC_CPLI_MASK                        BIT(17)
#define RAM_DBE_STS_A_DSC_SW_CTXT_MASK                     BIT(16)
#define RAM_DBE_STS_A_DSC_CRD_RCV_MASK                     BIT(15)
#define RAM_DBE_STS_A_DSC_HW_CTXT_MASK                     BIT(14)
#define RAM_DBE_STS_A_FUNC_MAP_MASK                        BIT(13)
#define RAM_DBE_STS_A_C2H_WR_BRG_DAT_MASK                  BIT(12)
#define RAM_DBE_STS_A_C2H_RD_BRG_DAT_MASK                  BIT(11)
#define RAM_DBE_STS_A_H2C_WR_BRG_DAT_MASK                  BIT(10)
#define RAM_DBE_STS_A_H2C_RD_BRG_DAT_MASK                  BIT(9)
#define RAM_DBE_STS_A_RSVD_2_MASK                          GENMASK(8, 5)
#define RAM_DBE_STS_A_MI_C2H0_DAT_MASK                     BIT(4)
#define RAM_DBE_STS_A_RSVD_3_MASK                          GENMASK(3, 1)
#define RAM_DBE_STS_A_MI_H2C0_DAT_MASK                     BIT(0)
#define QDMA_S80_HARD_GLBL2_IDENTIFIER_ADDR                0x100
#define GLBL2_IDENTIFIER_MASK                             GENMASK(31, 8)
#define GLBL2_IDENTIFIER_VERSION_MASK                      GENMASK(7, 0)
#define QDMA_S80_HARD_GLBL2_PF_BARLITE_INT_ADDR            0x104
#define GLBL2_PF_BARLITE_INT_PF3_BAR_MAP_MASK              GENMASK(23, 18)
#define GLBL2_PF_BARLITE_INT_PF2_BAR_MAP_MASK              GENMASK(17, 12)
#define GLBL2_PF_BARLITE_INT_PF1_BAR_MAP_MASK              GENMASK(11, 6)
#define GLBL2_PF_BARLITE_INT_PF0_BAR_MAP_MASK              GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL2_PF_VF_BARLITE_INT_ADDR         0x108
#define GLBL2_PF_VF_BARLITE_INT_PF3_MAP_MASK               GENMASK(23, 18)
#define GLBL2_PF_VF_BARLITE_INT_PF2_MAP_MASK               GENMASK(17, 12)
#define GLBL2_PF_VF_BARLITE_INT_PF1_MAP_MASK               GENMASK(11, 6)
#define GLBL2_PF_VF_BARLITE_INT_PF0_MAP_MASK               GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL2_PF_BARLITE_EXT_ADDR            0x10C
#define GLBL2_PF_BARLITE_EXT_PF3_BAR_MAP_MASK              GENMASK(23, 18)
#define GLBL2_PF_BARLITE_EXT_PF2_BAR_MAP_MASK              GENMASK(17, 12)
#define GLBL2_PF_BARLITE_EXT_PF1_BAR_MAP_MASK              GENMASK(11, 6)
#define GLBL2_PF_BARLITE_EXT_PF0_BAR_MAP_MASK              GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL2_PF_VF_BARLITE_EXT_ADDR         0x110
#define GLBL2_PF_VF_BARLITE_EXT_PF3_MAP_MASK               GENMASK(23, 18)
#define GLBL2_PF_VF_BARLITE_EXT_PF2_MAP_MASK               GENMASK(17, 12)
#define GLBL2_PF_VF_BARLITE_EXT_PF1_MAP_MASK               GENMASK(11, 6)
#define GLBL2_PF_VF_BARLITE_EXT_PF0_MAP_MASK               GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_INST_ADDR              0x114
#define GLBL2_CHANNEL_INST_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_INST_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_INST_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_INST_RSVD_2_MASK                     GENMASK(15, 9)
#define GLBL2_CHANNEL_INST_C2H_ENG_MASK                    BIT(8)
#define GLBL2_CHANNEL_INST_RSVD_3_MASK                     GENMASK(7, 1)
#define GLBL2_CHANNEL_INST_H2C_ENG_MASK                    BIT(0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_MDMA_ADDR              0x118
#define GLBL2_CHANNEL_MDMA_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_MDMA_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_MDMA_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_MDMA_RSVD_2_MASK                     GENMASK(15, 9)
#define GLBL2_CHANNEL_MDMA_C2H_ENG_MASK                    BIT(8)
#define GLBL2_CHANNEL_MDMA_RSVD_3_MASK                     GENMASK(7, 1)
#define GLBL2_CHANNEL_MDMA_H2C_ENG_MASK                    BIT(0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_STRM_ADDR              0x11C
#define GLBL2_CHANNEL_STRM_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_STRM_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_STRM_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_STRM_RSVD_2_MASK                     GENMASK(15, 9)
#define GLBL2_CHANNEL_STRM_C2H_ENG_MASK                    BIT(8)
#define GLBL2_CHANNEL_STRM_RSVD_3_MASK                     GENMASK(7, 1)
#define GLBL2_CHANNEL_STRM_H2C_ENG_MASK                    BIT(0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_CAP_ADDR               0x120
#define GLBL2_CHANNEL_CAP_RSVD_1_MASK                      GENMASK(31, 12)
#define GLBL2_CHANNEL_CAP_MULTIQ_MAX_MASK                  GENMASK(11, 0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_PASID_CAP_ADDR         0x128
#define GLBL2_CHANNEL_PASID_CAP_RSVD_1_MASK                GENMASK(31, 16)
#define GLBL2_CHANNEL_PASID_CAP_BRIDGEOFFSET_MASK          GENMASK(15, 4)
#define GLBL2_CHANNEL_PASID_CAP_RSVD_2_MASK                GENMASK(3, 2)
#define GLBL2_CHANNEL_PASID_CAP_BRIDGEEN_MASK              BIT(1)
#define GLBL2_CHANNEL_PASID_CAP_DMAEN_MASK                 BIT(0)
#define QDMA_S80_HARD_GLBL2_CHANNEL_FUNC_RET_ADDR          0x12C
#define GLBL2_CHANNEL_FUNC_RET_RSVD_1_MASK                 GENMASK(31, 8)
#define GLBL2_CHANNEL_FUNC_RET_FUNC_MASK                   GENMASK(7, 0)
#define QDMA_S80_HARD_GLBL2_SYSTEM_ID_ADDR                 0x130
#define GLBL2_SYSTEM_ID_RSVD_1_MASK                        GENMASK(31, 16)
#define GLBL2_SYSTEM_ID_MASK                              GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL2_MISC_CAP_ADDR                  0x134
#define GLBL2_MISC_CAP_RSVD_1_MASK                         GENMASK(31, 0)
#define QDMA_S80_HARD_GLBL2_DBG_PCIE_RQ0_ADDR              0x1B8
#define GLBL2_PCIE_RQ0_NPH_AVL_MASK                    GENMASK(31, 20)
#define GLBL2_PCIE_RQ0_RCB_AVL_MASK                    GENMASK(19, 10)
#define GLBL2_PCIE_RQ0_SLV_RD_CREDS_MASK               GENMASK(9, 4)
#define GLBL2_PCIE_RQ0_TAG_EP_MASK                     GENMASK(3, 2)
#define GLBL2_PCIE_RQ0_TAG_FL_MASK                     GENMASK(1, 0)
#define QDMA_S80_HARD_GLBL2_DBG_PCIE_RQ1_ADDR              0x1BC
#define GLBL2_PCIE_RQ1_RSVD_1_MASK                     GENMASK(31, 17)
#define GLBL2_PCIE_RQ1_WTLP_REQ_MASK                   BIT(16)
#define GLBL2_PCIE_RQ1_WTLP_HEADER_FIFO_FL_MASK        BIT(15)
#define GLBL2_PCIE_RQ1_WTLP_HEADER_FIFO_EP_MASK        BIT(14)
#define GLBL2_PCIE_RQ1_RQ_FIFO_EP_MASK                 BIT(13)
#define GLBL2_PCIE_RQ1_RQ_FIFO_FL_MASK                 BIT(12)
#define GLBL2_PCIE_RQ1_TLPSM_MASK                      GENMASK(11, 9)
#define GLBL2_PCIE_RQ1_TLPSM512_MASK                   GENMASK(8, 6)
#define GLBL2_PCIE_RQ1_RREQ0_RCB_OK_MASK               BIT(5)
#define GLBL2_PCIE_RQ1_RREQ0_SLV_MASK                  BIT(4)
#define GLBL2_PCIE_RQ1_RREQ0_VLD_MASK                  BIT(3)
#define GLBL2_PCIE_RQ1_RREQ1_RCB_OK_MASK               BIT(2)
#define GLBL2_PCIE_RQ1_RREQ1_SLV_MASK                  BIT(1)
#define GLBL2_PCIE_RQ1_RREQ1_VLD_MASK                  BIT(0)
#define QDMA_S80_HARD_GLBL2_DBG_AXIMM_WR0_ADDR             0x1C0
#define GLBL2_AXIMM_WR0_RSVD_1_MASK                    GENMASK(31, 27)
#define GLBL2_AXIMM_WR0_WR_REQ_MASK                    BIT(26)
#define GLBL2_AXIMM_WR0_WR_CHN_MASK                    GENMASK(25, 23)
#define GLBL2_AXIMM_WR0_WTLP_DATA_FIFO_EP_MASK         BIT(22)
#define GLBL2_AXIMM_WR0_WPL_FIFO_EP_MASK               BIT(21)
#define GLBL2_AXIMM_WR0_BRSP_CLAIM_CHN_MASK            GENMASK(20, 18)
#define GLBL2_AXIMM_WR0_WRREQ_CNT_MASK                 GENMASK(17, 12)
#define GLBL2_AXIMM_WR0_BID_MASK                       GENMASK(11, 9)
#define GLBL2_AXIMM_WR0_BVALID_MASK                    BIT(8)
#define GLBL2_AXIMM_WR0_BREADY_MASK                    BIT(7)
#define GLBL2_AXIMM_WR0_WVALID_MASK                    BIT(6)
#define GLBL2_AXIMM_WR0_WREADY_MASK                    BIT(5)
#define GLBL2_AXIMM_WR0_AWID_MASK                      GENMASK(4, 2)
#define GLBL2_AXIMM_WR0_AWVALID_MASK                   BIT(1)
#define GLBL2_AXIMM_WR0_AWREADY_MASK                   BIT(0)
#define QDMA_S80_HARD_GLBL2_DBG_AXIMM_WR1_ADDR             0x1C4
#define GLBL2_AXIMM_WR1_RSVD_1_MASK                    GENMASK(31, 30)
#define GLBL2_AXIMM_WR1_BRSP_CNT4_MASK                 GENMASK(29, 24)
#define GLBL2_AXIMM_WR1_BRSP_CNT3_MASK                 GENMASK(23, 18)
#define GLBL2_AXIMM_WR1_BRSP_CNT2_MASK                 GENMASK(17, 12)
#define GLBL2_AXIMM_WR1_BRSP_CNT1_MASK                 GENMASK(11, 6)
#define GLBL2_AXIMM_WR1_BRSP_CNT0_MASK                 GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL2_DBG_AXIMM_RD0_ADDR             0x1C8
#define GLBL2_AXIMM_RD0_RSVD_1_MASK                    GENMASK(31, 23)
#define GLBL2_AXIMM_RD0_PND_CNT_MASK                   GENMASK(22, 17)
#define GLBL2_AXIMM_RD0_RD_CHNL_MASK                   GENMASK(16, 14)
#define GLBL2_AXIMM_RD0_RD_REQ_MASK                    BIT(13)
#define GLBL2_AXIMM_RD0_RRSP_CLAIM_CHNL_MASK           GENMASK(12, 10)
#define GLBL2_AXIMM_RD0_RID_MASK                       GENMASK(9, 7)
#define GLBL2_AXIMM_RD0_RVALID_MASK                    BIT(6)
#define GLBL2_AXIMM_RD0_RREADY_MASK                    BIT(5)
#define GLBL2_AXIMM_RD0_ARID_MASK                      GENMASK(4, 2)
#define GLBL2_AXIMM_RD0_ARVALID_MASK                   BIT(1)
#define GLBL2_AXIMM_RD0_ARREADY_MASK                   BIT(0)
#define QDMA_S80_HARD_GLBL2_DBG_AXIMM_RD1_ADDR             0x1CC
#define GLBL2_AXIMM_RD1_RSVD_1_MASK                    GENMASK(31, 30)
#define GLBL2_AXIMM_RD1_RRSP_CNT4_MASK                 GENMASK(29, 24)
#define GLBL2_AXIMM_RD1_RRSP_CNT3_MASK                 GENMASK(23, 18)
#define GLBL2_AXIMM_RD1_RRSP_CNT2_MASK                 GENMASK(17, 12)
#define GLBL2_AXIMM_RD1_RRSP_CNT1_MASK                 GENMASK(11, 6)
#define GLBL2_AXIMM_RD1_RRSP_CNT0_MASK                 GENMASK(5, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_1_ADDR                   0x204
#define GLBL_RNG_SZ_1_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_1_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_2_ADDR                   0x208
#define GLBL_RNG_SZ_2_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_2_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_3_ADDR                   0x20C
#define GLBL_RNG_SZ_3_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_3_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_4_ADDR                   0x210
#define GLBL_RNG_SZ_4_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_4_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_5_ADDR                   0x214
#define GLBL_RNG_SZ_5_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_5_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_6_ADDR                   0x218
#define GLBL_RNG_SZ_6_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_6_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_7_ADDR                   0x21C
#define GLBL_RNG_SZ_7_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_7_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_8_ADDR                   0x220
#define GLBL_RNG_SZ_8_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_8_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_9_ADDR                   0x224
#define GLBL_RNG_SZ_9_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_9_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_A_ADDR                   0x228
#define GLBL_RNG_SZ_A_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_A_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_B_ADDR                   0x22C
#define GLBL_RNG_SZ_B_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_B_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_C_ADDR                   0x230
#define GLBL_RNG_SZ_C_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_C_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_D_ADDR                   0x234
#define GLBL_RNG_SZ_D_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_D_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_E_ADDR                   0x238
#define GLBL_RNG_SZ_E_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_E_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_F_ADDR                   0x23C
#define GLBL_RNG_SZ_F_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_F_RING_SIZE_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_RNG_SZ_10_ADDR                  0x240
#define GLBL_RNG_SZ_10_RSVD_1_MASK                         GENMASK(31, 16)
#define GLBL_RNG_SZ_10_RING_SIZE_MASK                      GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_ERR_STAT_ADDR                   0x248
#define GLBL_ERR_STAT_RSVD_1_MASK                          GENMASK(31, 12)
#define GLBL_ERR_STAT_ERR_H2C_ST_MASK                      BIT(11)
#define GLBL_ERR_STAT_ERR_BDG_MASK                         BIT(10)
#define GLBL_ERR_STAT_IND_CTXT_CMD_ERR_MASK                BIT(9)
#define GLBL_ERR_STAT_ERR_C2H_ST_MASK                      BIT(8)
#define GLBL_ERR_STAT_ERR_C2H_MM_1_MASK                    BIT(7)
#define GLBL_ERR_STAT_ERR_C2H_MM_0_MASK                    BIT(6)
#define GLBL_ERR_STAT_ERR_H2C_MM_1_MASK                    BIT(5)
#define GLBL_ERR_STAT_ERR_H2C_MM_0_MASK                    BIT(4)
#define GLBL_ERR_STAT_ERR_TRQ_MASK                         BIT(3)
#define GLBL_ERR_STAT_ERR_DSC_MASK                         BIT(2)
#define GLBL_ERR_STAT_ERR_RAM_DBE_MASK                     BIT(1)
#define GLBL_ERR_STAT_ERR_RAM_SBE_MASK                     BIT(0)
#define QDMA_S80_HARD_GLBL_ERR_MASK_ADDR                   0x24C
#define GLBL_ERR_RSVD_1_MASK                          GENMASK(31, 9)
#define GLBL_ERR_MASK                            GENMASK(8, 0)
#define QDMA_S80_HARD_GLBL_DSC_CFG_ADDR                    0x250
#define GLBL_DSC_CFG_RSVD_1_MASK                           GENMASK(31, 10)
#define GLBL_DSC_CFG_UNC_OVR_COR_MASK                      BIT(9)
#define GLBL_DSC_CFG_CTXT_FER_DIS_MASK                     BIT(8)
#define GLBL_DSC_CFG_RSVD_2_MASK                           GENMASK(7, 6)
#define GLBL_DSC_CFG_MAXFETCH_MASK                         GENMASK(5, 3)
#define GLBL_DSC_CFG_WB_ACC_INT_MASK                       GENMASK(2, 0)
#define QDMA_S80_HARD_GLBL_DSC_ERR_STS_ADDR                0x254
#define GLBL_DSC_ERR_STS_RSVD_1_MASK                       GENMASK(31, 25)
#define GLBL_DSC_ERR_STS_SBE_MASK                          BIT(24)
#define GLBL_DSC_ERR_STS_DBE_MASK                          BIT(23)
#define GLBL_DSC_ERR_STS_RQ_CANCEL_MASK                    BIT(22)
#define GLBL_DSC_ERR_STS_DSC_MASK                          BIT(21)
#define GLBL_DSC_ERR_STS_DMA_MASK                          BIT(20)
#define GLBL_DSC_ERR_STS_FLR_CANCEL_MASK                   BIT(19)
#define GLBL_DSC_ERR_STS_RSVD_2_MASK                       GENMASK(18, 17)
#define GLBL_DSC_ERR_STS_DAT_POISON_MASK                   BIT(16)
#define GLBL_DSC_ERR_STS_TIMEOUT_MASK                      BIT(9)
#define GLBL_DSC_ERR_STS_FLR_MASK                          BIT(5)
#define GLBL_DSC_ERR_STS_TAG_MASK                          BIT(4)
#define GLBL_DSC_ERR_STS_ADDR_MASK                         BIT(3)
#define GLBL_DSC_ERR_STS_PARAM_MASK                        BIT(2)
#define GLBL_DSC_ERR_STS_UR_CA_MASK                        BIT(1)
#define GLBL_DSC_ERR_STS_POISON_MASK                       BIT(0)
#define QDMA_S80_HARD_GLBL_DSC_ERR_MSK_ADDR                0x258
#define GLBL_DSC_ERR_MSK_MASK                         GENMASK(8, 0)
#define QDMA_S80_HARD_GLBL_DSC_ERR_LOG0_ADDR               0x25C
#define GLBL_DSC_ERR_LOG0_VALID_MASK                       BIT(31)
#define GLBL_DSC_ERR_LOG0_RSVD_1_MASK                      GENMASK(30, 29)
#define GLBL_DSC_ERR_LOG0_QID_MASK                         GENMASK(28, 17)
#define GLBL_DSC_ERR_LOG0_SEL_MASK                         BIT(16)
#define GLBL_DSC_ERR_LOG0_CIDX_MASK                        GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_DSC_ERR_LOG1_ADDR               0x260
#define GLBL_DSC_ERR_LOG1_RSVD_1_MASK                      GENMASK(31, 9)
#define GLBL_DSC_ERR_LOG1_SUB_TYPE_MASK                    GENMASK(8, 5)
#define GLBL_DSC_ERR_LOG1_ERR_TYPE_MASK                    GENMASK(4, 0)
#define QDMA_S80_HARD_GLBL_TRQ_ERR_STS_ADDR                0x264
#define GLBL_TRQ_ERR_STS_RSVD_1_MASK                       GENMASK(31, 4)
#define GLBL_TRQ_ERR_STS_TCP_TIMEOUT_MASK                  BIT(3)
#define GLBL_TRQ_ERR_STS_VF_ACCESS_ERR_MASK                BIT(2)
#define GLBL_TRQ_ERR_STS_QID_RANGE_MASK                    BIT(1)
#define GLBL_TRQ_ERR_STS_UNMAPPED_MASK                     BIT(0)
#define QDMA_S80_HARD_GLBL_TRQ_ERR_MSK_ADDR                0x268
#define GLBL_TRQ_ERR_MSK_MASK                         GENMASK(31, 0)
#define QDMA_S80_HARD_GLBL_TRQ_ERR_LOG_ADDR                0x26C
#define GLBL_TRQ_ERR_LOG_RSVD_1_MASK                       GENMASK(31, 28)
#define GLBL_TRQ_ERR_LOG_TARGET_MASK                       GENMASK(27, 24)
#define GLBL_TRQ_ERR_LOG_FUNC_MASK                         GENMASK(23, 16)
#define GLBL_TRQ_ERR_LOG_ADDRESS_MASK                      GENMASK(15, 0)
#define QDMA_S80_HARD_GLBL_DSC_DBG_DAT0_ADDR               0x270
#define GLBL_DSC_DAT0_RSVD_1_MASK                      GENMASK(31, 30)
#define GLBL_DSC_DAT0_CTXT_ARB_DIR_MASK                BIT(29)
#define GLBL_DSC_DAT0_CTXT_ARB_QID_MASK                GENMASK(28, 17)
#define GLBL_DSC_DAT0_CTXT_ARB_REQ_MASK                GENMASK(16, 12)
#define GLBL_DSC_DAT0_IRQ_FIFO_FL_MASK                 BIT(11)
#define GLBL_DSC_DAT0_TMSTALL_MASK                     BIT(10)
#define GLBL_DSC_DAT0_RRQ_STALL_MASK                   GENMASK(9, 8)
#define GLBL_DSC_DAT0_RCP_FIFO_SPC_STALL_MASK          GENMASK(7, 6)
#define GLBL_DSC_DAT0_RRQ_FIFO_SPC_STALL_MASK          GENMASK(5, 4)
#define GLBL_DSC_DAT0_FAB_MRKR_RSP_STALL_MASK          GENMASK(3, 2)
#define GLBL_DSC_DAT0_DSC_OUT_STALL_MASK               GENMASK(1, 0)
#define QDMA_S80_HARD_GLBL_DSC_DBG_DAT1_ADDR               0x274
#define GLBL_DSC_DAT1_RSVD_1_MASK                      GENMASK(31, 28)
#define GLBL_DSC_DAT1_EVT_SPC_C2H_MASK                 GENMASK(27, 22)
#define GLBL_DSC_DAT1_EVT_SP_H2C_MASK                  GENMASK(21, 16)
#define GLBL_DSC_DAT1_DSC_SPC_C2H_MASK                 GENMASK(15, 8)
#define GLBL_DSC_DAT1_DSC_SPC_H2C_MASK                 GENMASK(7, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_0_ADDR                  0x400
#define TRQ_SEL_FMAP_0_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_0_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_0_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1_ADDR                  0x404
#define TRQ_SEL_FMAP_1_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_1_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_1_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2_ADDR                  0x408
#define TRQ_SEL_FMAP_2_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_2_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_2_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3_ADDR                  0x40C
#define TRQ_SEL_FMAP_3_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_3_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_3_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4_ADDR                  0x410
#define TRQ_SEL_FMAP_4_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_4_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_4_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5_ADDR                  0x414
#define TRQ_SEL_FMAP_5_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_5_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_5_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6_ADDR                  0x418
#define TRQ_SEL_FMAP_6_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_6_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_6_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7_ADDR                  0x41C
#define TRQ_SEL_FMAP_7_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_7_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_7_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8_ADDR                  0x420
#define TRQ_SEL_FMAP_8_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_8_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_8_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9_ADDR                  0x424
#define TRQ_SEL_FMAP_9_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_9_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_9_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A_ADDR                  0x428
#define TRQ_SEL_FMAP_A_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_A_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_A_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B_ADDR                  0x42C
#define TRQ_SEL_FMAP_B_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_B_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_B_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D_ADDR                  0x430
#define TRQ_SEL_FMAP_D_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_D_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_D_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E_ADDR                  0x434
#define TRQ_SEL_FMAP_E_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_E_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_E_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_F_ADDR                  0x438
#define TRQ_SEL_FMAP_F_RSVD_1_MASK                         GENMASK(31, 23)
#define TRQ_SEL_FMAP_F_QID_MAX_MASK                        GENMASK(22, 11)
#define TRQ_SEL_FMAP_F_QID_BASE_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_10_ADDR                 0x43C
#define TRQ_SEL_FMAP_10_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_10_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_10_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_11_ADDR                 0x440
#define TRQ_SEL_FMAP_11_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_11_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_11_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_12_ADDR                 0x444
#define TRQ_SEL_FMAP_12_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_12_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_12_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_13_ADDR                 0x448
#define TRQ_SEL_FMAP_13_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_13_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_13_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_14_ADDR                 0x44C
#define TRQ_SEL_FMAP_14_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_14_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_14_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_15_ADDR                 0x450
#define TRQ_SEL_FMAP_15_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_15_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_15_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_16_ADDR                 0x454
#define TRQ_SEL_FMAP_16_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_16_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_16_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_17_ADDR                 0x458
#define TRQ_SEL_FMAP_17_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_17_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_17_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_18_ADDR                 0x45C
#define TRQ_SEL_FMAP_18_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_18_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_18_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_19_ADDR                 0x460
#define TRQ_SEL_FMAP_19_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_19_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_19_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1A_ADDR                 0x464
#define TRQ_SEL_FMAP_1A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1B_ADDR                 0x468
#define TRQ_SEL_FMAP_1B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1C_ADDR                 0x46C
#define TRQ_SEL_FMAP_1C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1D_ADDR                 0x470
#define TRQ_SEL_FMAP_1D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1E_ADDR                 0x474
#define TRQ_SEL_FMAP_1E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_1F_ADDR                 0x478
#define TRQ_SEL_FMAP_1F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_1F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_1F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_20_ADDR                 0x47C
#define TRQ_SEL_FMAP_20_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_20_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_20_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_21_ADDR                 0x480
#define TRQ_SEL_FMAP_21_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_21_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_21_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_22_ADDR                 0x484
#define TRQ_SEL_FMAP_22_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_22_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_22_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_23_ADDR                 0x488
#define TRQ_SEL_FMAP_23_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_23_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_23_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_24_ADDR                 0x48C
#define TRQ_SEL_FMAP_24_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_24_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_24_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_25_ADDR                 0x490
#define TRQ_SEL_FMAP_25_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_25_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_25_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_26_ADDR                 0x494
#define TRQ_SEL_FMAP_26_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_26_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_26_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_27_ADDR                 0x498
#define TRQ_SEL_FMAP_27_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_27_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_27_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_28_ADDR                 0x49C
#define TRQ_SEL_FMAP_28_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_28_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_28_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_29_ADDR                 0x4A0
#define TRQ_SEL_FMAP_29_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_29_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_29_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2A_ADDR                 0x4A4
#define TRQ_SEL_FMAP_2A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2B_ADDR                 0x4A8
#define TRQ_SEL_FMAP_2B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2C_ADDR                 0x4AC
#define TRQ_SEL_FMAP_2C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2D_ADDR                 0x4B0
#define TRQ_SEL_FMAP_2D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2E_ADDR                 0x4B4
#define TRQ_SEL_FMAP_2E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_2F_ADDR                 0x4B8
#define TRQ_SEL_FMAP_2F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_2F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_2F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_30_ADDR                 0x4BC
#define TRQ_SEL_FMAP_30_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_30_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_30_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_31_ADDR                 0x4D0
#define TRQ_SEL_FMAP_31_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_31_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_31_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_32_ADDR                 0x4D4
#define TRQ_SEL_FMAP_32_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_32_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_32_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_33_ADDR                 0x4D8
#define TRQ_SEL_FMAP_33_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_33_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_33_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_34_ADDR                 0x4DC
#define TRQ_SEL_FMAP_34_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_34_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_34_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_35_ADDR                 0x4E0
#define TRQ_SEL_FMAP_35_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_35_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_35_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_36_ADDR                 0x4E4
#define TRQ_SEL_FMAP_36_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_36_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_36_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_37_ADDR                 0x4E8
#define TRQ_SEL_FMAP_37_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_37_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_37_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_38_ADDR                 0x4EC
#define TRQ_SEL_FMAP_38_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_38_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_38_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_39_ADDR                 0x4F0
#define TRQ_SEL_FMAP_39_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_39_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_39_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3A_ADDR                 0x4F4
#define TRQ_SEL_FMAP_3A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3B_ADDR                 0x4F8
#define TRQ_SEL_FMAP_3B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3C_ADDR                 0x4FC
#define TRQ_SEL_FMAP_3C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3D_ADDR                 0x500
#define TRQ_SEL_FMAP_3D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3E_ADDR                 0x504
#define TRQ_SEL_FMAP_3E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_3F_ADDR                 0x508
#define TRQ_SEL_FMAP_3F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_3F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_3F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_40_ADDR                 0x50C
#define TRQ_SEL_FMAP_40_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_40_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_40_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_41_ADDR                 0x510
#define TRQ_SEL_FMAP_41_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_41_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_41_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_42_ADDR                 0x514
#define TRQ_SEL_FMAP_42_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_42_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_42_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_43_ADDR                 0x518
#define TRQ_SEL_FMAP_43_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_43_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_43_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_44_ADDR                 0x51C
#define TRQ_SEL_FMAP_44_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_44_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_44_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_45_ADDR                 0x520
#define TRQ_SEL_FMAP_45_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_45_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_45_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_46_ADDR                 0x524
#define TRQ_SEL_FMAP_46_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_46_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_46_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_47_ADDR                 0x528
#define TRQ_SEL_FMAP_47_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_47_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_47_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_48_ADDR                 0x52C
#define TRQ_SEL_FMAP_48_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_48_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_48_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_49_ADDR                 0x530
#define TRQ_SEL_FMAP_49_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_49_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_49_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4A_ADDR                 0x534
#define TRQ_SEL_FMAP_4A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4B_ADDR                 0x538
#define TRQ_SEL_FMAP_4B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4C_ADDR                 0x53C
#define TRQ_SEL_FMAP_4C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4D_ADDR                 0x540
#define TRQ_SEL_FMAP_4D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4E_ADDR                 0x544
#define TRQ_SEL_FMAP_4E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_4F_ADDR                 0x548
#define TRQ_SEL_FMAP_4F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_4F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_4F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_50_ADDR                 0x54C
#define TRQ_SEL_FMAP_50_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_50_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_50_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_51_ADDR                 0x550
#define TRQ_SEL_FMAP_51_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_51_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_51_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_52_ADDR                 0x554
#define TRQ_SEL_FMAP_52_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_52_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_52_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_53_ADDR                 0x558
#define TRQ_SEL_FMAP_53_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_53_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_53_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_54_ADDR                 0x55C
#define TRQ_SEL_FMAP_54_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_54_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_54_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_55_ADDR                 0x560
#define TRQ_SEL_FMAP_55_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_55_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_55_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_56_ADDR                 0x564
#define TRQ_SEL_FMAP_56_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_56_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_56_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_57_ADDR                 0x568
#define TRQ_SEL_FMAP_57_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_57_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_57_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_58_ADDR                 0x56C
#define TRQ_SEL_FMAP_58_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_58_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_58_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_59_ADDR                 0x570
#define TRQ_SEL_FMAP_59_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_59_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_59_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5A_ADDR                 0x574
#define TRQ_SEL_FMAP_5A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5B_ADDR                 0x578
#define TRQ_SEL_FMAP_5B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5C_ADDR                 0x57C
#define TRQ_SEL_FMAP_5C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5D_ADDR                 0x580
#define TRQ_SEL_FMAP_5D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5E_ADDR                 0x584
#define TRQ_SEL_FMAP_5E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_5F_ADDR                 0x588
#define TRQ_SEL_FMAP_5F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_5F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_5F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_60_ADDR                 0x58C
#define TRQ_SEL_FMAP_60_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_60_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_60_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_61_ADDR                 0x590
#define TRQ_SEL_FMAP_61_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_61_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_61_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_62_ADDR                 0x594
#define TRQ_SEL_FMAP_62_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_62_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_62_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_63_ADDR                 0x598
#define TRQ_SEL_FMAP_63_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_63_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_63_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_64_ADDR                 0x59C
#define TRQ_SEL_FMAP_64_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_64_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_64_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_65_ADDR                 0x5A0
#define TRQ_SEL_FMAP_65_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_65_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_65_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_66_ADDR                 0x5A4
#define TRQ_SEL_FMAP_66_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_66_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_66_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_67_ADDR                 0x5A8
#define TRQ_SEL_FMAP_67_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_67_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_67_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_68_ADDR                 0x5AC
#define TRQ_SEL_FMAP_68_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_68_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_68_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_69_ADDR                 0x5B0
#define TRQ_SEL_FMAP_69_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_69_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_69_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6A_ADDR                 0x5B4
#define TRQ_SEL_FMAP_6A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6B_ADDR                 0x5B8
#define TRQ_SEL_FMAP_6B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6C_ADDR                 0x5BC
#define TRQ_SEL_FMAP_6C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6D_ADDR                 0x5C0
#define TRQ_SEL_FMAP_6D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6E_ADDR                 0x5C4
#define TRQ_SEL_FMAP_6E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_6F_ADDR                 0x5C8
#define TRQ_SEL_FMAP_6F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_6F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_6F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_70_ADDR                 0x5CC
#define TRQ_SEL_FMAP_70_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_70_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_70_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_71_ADDR                 0x5D0
#define TRQ_SEL_FMAP_71_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_71_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_71_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_72_ADDR                 0x5D4
#define TRQ_SEL_FMAP_72_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_72_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_72_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_73_ADDR                 0x5D8
#define TRQ_SEL_FMAP_73_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_73_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_73_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_74_ADDR                 0x5DC
#define TRQ_SEL_FMAP_74_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_74_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_74_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_75_ADDR                 0x5E0
#define TRQ_SEL_FMAP_75_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_75_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_75_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_76_ADDR                 0x5E4
#define TRQ_SEL_FMAP_76_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_76_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_76_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_77_ADDR                 0x5E8
#define TRQ_SEL_FMAP_77_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_77_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_77_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_78_ADDR                 0x5EC
#define TRQ_SEL_FMAP_78_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_78_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_78_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_79_ADDR                 0x5F0
#define TRQ_SEL_FMAP_79_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_79_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_79_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7A_ADDR                 0x5F4
#define TRQ_SEL_FMAP_7A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7B_ADDR                 0x5F8
#define TRQ_SEL_FMAP_7B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7C_ADDR                 0x5FC
#define TRQ_SEL_FMAP_7C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7D_ADDR                 0x600
#define TRQ_SEL_FMAP_7D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7E_ADDR                 0x604
#define TRQ_SEL_FMAP_7E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_7F_ADDR                 0x608
#define TRQ_SEL_FMAP_7F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_7F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_7F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_80_ADDR                 0x60C
#define TRQ_SEL_FMAP_80_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_80_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_80_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_81_ADDR                 0x610
#define TRQ_SEL_FMAP_81_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_81_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_81_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_82_ADDR                 0x614
#define TRQ_SEL_FMAP_82_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_82_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_82_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_83_ADDR                 0x618
#define TRQ_SEL_FMAP_83_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_83_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_83_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_84_ADDR                 0x61C
#define TRQ_SEL_FMAP_84_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_84_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_84_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_85_ADDR                 0x620
#define TRQ_SEL_FMAP_85_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_85_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_85_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_86_ADDR                 0x624
#define TRQ_SEL_FMAP_86_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_86_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_86_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_87_ADDR                 0x628
#define TRQ_SEL_FMAP_87_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_87_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_87_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_88_ADDR                 0x62C
#define TRQ_SEL_FMAP_88_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_88_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_88_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_89_ADDR                 0x630
#define TRQ_SEL_FMAP_89_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_89_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_89_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8A_ADDR                 0x634
#define TRQ_SEL_FMAP_8A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8B_ADDR                 0x638
#define TRQ_SEL_FMAP_8B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8C_ADDR                 0x63C
#define TRQ_SEL_FMAP_8C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8D_ADDR                 0x640
#define TRQ_SEL_FMAP_8D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8E_ADDR                 0x644
#define TRQ_SEL_FMAP_8E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_8F_ADDR                 0x648
#define TRQ_SEL_FMAP_8F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_8F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_8F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_90_ADDR                 0x64C
#define TRQ_SEL_FMAP_90_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_90_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_90_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_91_ADDR                 0x650
#define TRQ_SEL_FMAP_91_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_91_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_91_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_92_ADDR                 0x654
#define TRQ_SEL_FMAP_92_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_92_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_92_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_93_ADDR                 0x658
#define TRQ_SEL_FMAP_93_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_93_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_93_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_94_ADDR                 0x65C
#define TRQ_SEL_FMAP_94_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_94_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_94_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_95_ADDR                 0x660
#define TRQ_SEL_FMAP_95_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_95_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_95_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_96_ADDR                 0x664
#define TRQ_SEL_FMAP_96_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_96_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_96_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_97_ADDR                 0x668
#define TRQ_SEL_FMAP_97_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_97_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_97_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_98_ADDR                 0x66C
#define TRQ_SEL_FMAP_98_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_98_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_98_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_99_ADDR                 0x670
#define TRQ_SEL_FMAP_99_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_99_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_99_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9A_ADDR                 0x674
#define TRQ_SEL_FMAP_9A_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9A_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9A_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9B_ADDR                 0x678
#define TRQ_SEL_FMAP_9B_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9B_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9B_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9C_ADDR                 0x67C
#define TRQ_SEL_FMAP_9C_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9C_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9C_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9D_ADDR                 0x680
#define TRQ_SEL_FMAP_9D_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9D_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9D_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9E_ADDR                 0x684
#define TRQ_SEL_FMAP_9E_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9E_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9E_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_9F_ADDR                 0x688
#define TRQ_SEL_FMAP_9F_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_9F_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_9F_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A0_ADDR                 0x68C
#define TRQ_SEL_FMAP_A0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A1_ADDR                 0x690
#define TRQ_SEL_FMAP_A1_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A1_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A1_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A2_ADDR                 0x694
#define TRQ_SEL_FMAP_A2_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A2_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A2_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A3_ADDR                 0x698
#define TRQ_SEL_FMAP_A3_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A3_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A3_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A4_ADDR                 0x69C
#define TRQ_SEL_FMAP_A4_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A4_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A4_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A5_ADDR                 0x6A0
#define TRQ_SEL_FMAP_A5_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A5_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A5_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A6_ADDR                 0x6A4
#define TRQ_SEL_FMAP_A6_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A6_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A6_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A7_ADDR                 0x6A8
#define TRQ_SEL_FMAP_A7_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A7_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A7_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A8_ADDR                 0x6AC
#define TRQ_SEL_FMAP_A8_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A8_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A8_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_A9_ADDR                 0x6B0
#define TRQ_SEL_FMAP_A9_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_A9_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_A9_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AA_ADDR                 0x6B4
#define TRQ_SEL_FMAP_AA_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AA_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AA_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AB_ADDR                 0x6B8
#define TRQ_SEL_FMAP_AB_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AB_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AB_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AC_ADDR                 0x6BC
#define TRQ_SEL_FMAP_AC_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AC_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AC_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AD_ADDR                 0x6D0
#define TRQ_SEL_FMAP_AD_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AD_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AD_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AE_ADDR                 0x6D4
#define TRQ_SEL_FMAP_AE_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AE_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AE_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_AF_ADDR                 0x6D8
#define TRQ_SEL_FMAP_AF_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_AF_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_AF_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B0_ADDR                 0x6DC
#define TRQ_SEL_FMAP_B0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B1_ADDR                 0x6E0
#define TRQ_SEL_FMAP_B1_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B1_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B1_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B2_ADDR                 0x6E4
#define TRQ_SEL_FMAP_B2_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B2_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B2_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B3_ADDR                 0x6E8
#define TRQ_SEL_FMAP_B3_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B3_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B3_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B4_ADDR                 0x6EC
#define TRQ_SEL_FMAP_B4_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B4_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B4_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B5_ADDR                 0x6F0
#define TRQ_SEL_FMAP_B5_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B5_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B5_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B6_ADDR                 0x6F4
#define TRQ_SEL_FMAP_B6_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B6_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B6_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B7_ADDR                 0x6F8
#define TRQ_SEL_FMAP_B7_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B7_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B7_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B8_ADDR                 0x6FC
#define TRQ_SEL_FMAP_B8_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B8_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B8_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_B9_ADDR                 0x700
#define TRQ_SEL_FMAP_B9_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_B9_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_B9_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BA_ADDR                 0x704
#define TRQ_SEL_FMAP_BA_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BA_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BA_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BB_ADDR                 0x708
#define TRQ_SEL_FMAP_BB_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BB_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BB_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BC_ADDR                 0x70C
#define TRQ_SEL_FMAP_BC_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BC_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BC_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BD_ADDR                 0x710
#define TRQ_SEL_FMAP_BD_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BD_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BD_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BE_ADDR                 0x714
#define TRQ_SEL_FMAP_BE_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BE_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BE_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_BF_ADDR                 0x718
#define TRQ_SEL_FMAP_BF_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_BF_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_BF_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C0_ADDR                 0x71C
#define TRQ_SEL_FMAP_C0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C1_ADDR                 0x720
#define TRQ_SEL_FMAP_C1_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C1_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C1_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C2_ADDR                 0x734
#define TRQ_SEL_FMAP_C2_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C2_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C2_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C3_ADDR                 0x748
#define TRQ_SEL_FMAP_C3_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C3_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C3_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C4_ADDR                 0x74C
#define TRQ_SEL_FMAP_C4_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C4_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C4_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C5_ADDR                 0x750
#define TRQ_SEL_FMAP_C5_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C5_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C5_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C6_ADDR                 0x754
#define TRQ_SEL_FMAP_C6_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C6_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C6_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C7_ADDR                 0x758
#define TRQ_SEL_FMAP_C7_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C7_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C7_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C8_ADDR                 0x75C
#define TRQ_SEL_FMAP_C8_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C8_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C8_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_C9_ADDR                 0x760
#define TRQ_SEL_FMAP_C9_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_C9_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_C9_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CA_ADDR                 0x764
#define TRQ_SEL_FMAP_CA_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CA_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CA_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CB_ADDR                 0x768
#define TRQ_SEL_FMAP_CB_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CB_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CB_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CC_ADDR                 0x76C
#define TRQ_SEL_FMAP_CC_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CC_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CC_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CD_ADDR                 0x770
#define TRQ_SEL_FMAP_CD_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CD_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CD_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CE_ADDR                 0x774
#define TRQ_SEL_FMAP_CE_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CE_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CE_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_CF_ADDR                 0x778
#define TRQ_SEL_FMAP_CF_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_CF_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_CF_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D0_ADDR                 0x77C
#define TRQ_SEL_FMAP_D0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D1_ADDR                 0x780
#define TRQ_SEL_FMAP_D1_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D1_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D1_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D2_ADDR                 0x784
#define TRQ_SEL_FMAP_D2_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D2_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D2_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D3_ADDR                 0x788
#define TRQ_SEL_FMAP_D3_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D3_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D3_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D4_ADDR                 0x78C
#define TRQ_SEL_FMAP_D4_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D4_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D4_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D5_ADDR                 0x790
#define TRQ_SEL_FMAP_D5_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D5_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D5_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D6_ADDR                 0x794
#define TRQ_SEL_FMAP_D6_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D6_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D6_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D7_ADDR                 0x798
#define TRQ_SEL_FMAP_D7_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D7_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D7_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D8_ADDR                 0x79C
#define TRQ_SEL_FMAP_D8_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D8_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D8_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_D9_ADDR                 0x7A0
#define TRQ_SEL_FMAP_D9_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_D9_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_D9_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DA_ADDR                 0x7A4
#define TRQ_SEL_FMAP_DA_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DA_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DA_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DB_ADDR                 0x7A8
#define TRQ_SEL_FMAP_DB_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DB_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DB_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DC_ADDR                 0x7AC
#define TRQ_SEL_FMAP_DC_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DC_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DC_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DD_ADDR                 0x7B0
#define TRQ_SEL_FMAP_DD_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DD_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DD_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DE_ADDR                 0x7B4
#define TRQ_SEL_FMAP_DE_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DE_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DE_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_DF_ADDR                 0x7B8
#define TRQ_SEL_FMAP_DF_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_DF_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_DF_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E0_ADDR                 0x7BC
#define TRQ_SEL_FMAP_E0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E1_ADDR                 0x7C0
#define TRQ_SEL_FMAP_E1_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E1_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E1_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E2_ADDR                 0x7C4
#define TRQ_SEL_FMAP_E2_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E2_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E2_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E3_ADDR                 0x7C8
#define TRQ_SEL_FMAP_E3_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E3_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E3_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E4_ADDR                 0x7CC
#define TRQ_SEL_FMAP_E4_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E4_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E4_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E5_ADDR                 0x7D0
#define TRQ_SEL_FMAP_E5_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E5_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E5_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E6_ADDR                 0x7D4
#define TRQ_SEL_FMAP_E6_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E6_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E6_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E7_ADDR                 0x7D8
#define TRQ_SEL_FMAP_E7_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E7_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E7_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E8_ADDR                 0x7DC
#define TRQ_SEL_FMAP_E8_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E8_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E8_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_E9_ADDR                 0x7E0
#define TRQ_SEL_FMAP_E9_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_E9_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_E9_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_EA_ADDR                 0x7E4
#define TRQ_SEL_FMAP_EA_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_EA_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_EA_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_EB_ADDR                 0x7E8
#define TRQ_SEL_FMAP_EB_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_EB_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_EB_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_EC_ADDR                 0x7EC
#define TRQ_SEL_FMAP_EC_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_EC_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_EC_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_ED_ADDR                 0x7F0
#define TRQ_SEL_FMAP_ED_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_ED_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_ED_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_EE_ADDR                 0x7F4
#define TRQ_SEL_FMAP_EE_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_EE_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_EE_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_EF_ADDR                 0x7F8
#define TRQ_SEL_FMAP_EF_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_EF_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_EF_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_TRQ_SEL_FMAP_F0_ADDR                 0x7FC
#define TRQ_SEL_FMAP_F0_RSVD_1_MASK                        GENMASK(31, 23)
#define TRQ_SEL_FMAP_F0_QID_MAX_MASK                       GENMASK(22, 11)
#define TRQ_SEL_FMAP_F0_QID_BASE_MASK                      GENMASK(10, 0)
#define QDMA_S80_HARD_IND_CTXT_DATA_3_ADDR                 0x804
#define IND_CTXT_DATA_3_DATA_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT_DATA_2_ADDR                 0x808
#define IND_CTXT_DATA_2_DATA_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT_DATA_1_ADDR                 0x80C
#define IND_CTXT_DATA_1_DATA_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT_DATA_0_ADDR                 0x810
#define IND_CTXT_DATA_0_DATA_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT3_ADDR                       0x814
#define IND_CTXT3_MASK                                GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT2_ADDR                       0x818
#define IND_CTXT2_MASK                                GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT1_ADDR                       0x81C
#define IND_CTXT1_MASK                                GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT0_ADDR                       0x820
#define IND_CTXT0_MASK                                GENMASK(31, 0)
#define QDMA_S80_HARD_IND_CTXT_CMD_ADDR                    0x824
#define IND_CTXT_CMD_RSVD_1_MASK                           GENMASK(31, 18)
#define IND_CTXT_CMD_QID_MASK                              GENMASK(17, 7)
#define IND_CTXT_CMD_OP_MASK                               GENMASK(6, 5)
#define IND_CTXT_CMD_SET_MASK                              GENMASK(4, 1)
#define IND_CTXT_CMD_BUSY_MASK                             BIT(0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_1_ADDR                 0xA00
#define C2H_TIMER_CNT_1_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_1_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_2_ADDR                 0xA04
#define C2H_TIMER_CNT_2_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_2_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_3_ADDR                 0xA08
#define C2H_TIMER_CNT_3_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_3_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_4_ADDR                 0xA0C
#define C2H_TIMER_CNT_4_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_4_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_5_ADDR                 0xA10
#define C2H_TIMER_CNT_5_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_5_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_6_ADDR                 0xA14
#define C2H_TIMER_CNT_6_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_6_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_7_ADDR                 0xA18
#define C2H_TIMER_CNT_7_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_7_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_8_ADDR                 0xA1C
#define C2H_TIMER_CNT_8_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_8_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_9_ADDR                 0xA20
#define C2H_TIMER_CNT_9_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_9_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_A_ADDR                 0xA24
#define C2H_TIMER_CNT_A_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_A_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_B_ADDR                 0xA28
#define C2H_TIMER_CNT_B_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_B_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_C_ADDR                 0xA2C
#define C2H_TIMER_CNT_C_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_C_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_D_ADDR                 0xA30
#define C2H_TIMER_CNT_D_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_D_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_E_ADDR                 0xA34
#define C2H_TIMER_CNT_E_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_E_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_F_ADDR                 0xA38
#define C2H_TIMER_CNT_F_RSVD_1_MASK                        GENMASK(31, 8)
#define C2H_TIMER_CNT_F_MASK                              GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_TIMER_CNT_10_ADDR                0xA3C
#define C2H_TIMER_CNT_10_RSVD_1_MASK                       GENMASK(31, 8)
#define C2H_TIMER_CNT_10_MASK                             GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_1_ADDR                    0xA40
#define C2H_CNT_TH_1_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_1_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_2_ADDR                    0xA44
#define C2H_CNT_TH_2_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_2_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_3_ADDR                    0xA48
#define C2H_CNT_TH_3_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_3_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_4_ADDR                    0xA4C
#define C2H_CNT_TH_4_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_4_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_5_ADDR                    0xA50
#define C2H_CNT_TH_5_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_5_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_6_ADDR                    0xA54
#define C2H_CNT_TH_6_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_6_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_7_ADDR                    0xA58
#define C2H_CNT_TH_7_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_7_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_8_ADDR                    0xA5C
#define C2H_CNT_TH_8_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_8_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_9_ADDR                    0xA60
#define C2H_CNT_TH_9_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_9_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_A_ADDR                    0xA64
#define C2H_CNT_TH_A_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_A_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_B_ADDR                    0xA68
#define C2H_CNT_TH_B_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_B_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_C_ADDR                    0xA6C
#define C2H_CNT_TH_C_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_C_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_D_ADDR                    0xA70
#define C2H_CNT_TH_D_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_D_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_E_ADDR                    0xA74
#define C2H_CNT_TH_E_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_E_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_F_ADDR                    0xA78
#define C2H_CNT_TH_F_RSVD_1_MASK                           GENMASK(31, 8)
#define C2H_CNT_TH_F_THESHOLD_CNT_MASK                     GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_CNT_TH_10_ADDR                   0xA7C
#define C2H_CNT_TH_10_RSVD_1_MASK                          GENMASK(31, 8)
#define C2H_CNT_TH_10_THESHOLD_CNT_MASK                    GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_QID2VEC_MAP_QID_ADDR             0xA80
#define C2H_QID2VEC_MAP_QID_RSVD_1_MASK                    GENMASK(31, 11)
#define C2H_QID2VEC_MAP_QID_QID_MASK                       GENMASK(10, 0)
#define QDMA_S80_HARD_C2H_QID2VEC_MAP_ADDR                 0xA84
#define C2H_QID2VEC_MAP_RSVD_1_MASK                        GENMASK(31, 19)
#define C2H_QID2VEC_MAP_H2C_EN_COAL_MASK                   BIT(18)
#define C2H_QID2VEC_MAP_H2C_VECTOR_MASK                    GENMASK(17, 9)
#define C2H_QID2VEC_MAP_C2H_EN_COAL_MASK                   BIT(8)
#define C2H_QID2VEC_MAP_C2H_VECTOR_MASK                    GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_STAT_S_AXIS_C2H_ACCEPTED_ADDR    0xA88
#define C2H_STAT_S_AXIS_C2H_ACCEPTED_MASK                 GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_S_AXIS_WRB_ACCEPTED_ADDR    0xA8C
#define C2H_STAT_S_AXIS_WRB_ACCEPTED_MASK                 GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_RSP_PKT_ACCEPTED_ADDR  0xA90
#define C2H_STAT_DESC_RSP_PKT_ACCEPTED_D_MASK              GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_AXIS_PKG_CMP_ADDR           0xA94
#define C2H_STAT_AXIS_PKG_CMP_MASK                        GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_RSP_ACCEPTED_ADDR      0xA98
#define C2H_STAT_DESC_RSP_ACCEPTED_D_MASK                  GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_RSP_CMP_ADDR           0xA9C
#define C2H_STAT_DESC_RSP_CMP_D_MASK                       GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_WRQ_OUT_ADDR                0xAA0
#define C2H_STAT_WRQ_OUT_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_WPL_REN_ACCEPTED_ADDR       0xAA4
#define C2H_STAT_WPL_REN_ACCEPTED_MASK                    GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_TOTAL_WRQ_LEN_ADDR          0xAA8
#define C2H_STAT_TOTAL_WRQ_LEN_MASK                       GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_TOTAL_WPL_LEN_ADDR          0xAAC
#define C2H_STAT_TOTAL_WPL_LEN_MASK                       GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_0_ADDR                    0xAB0
#define C2H_BUF_SZ_0_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_1_ADDR                    0xAB4
#define C2H_BUF_SZ_1_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_2_ADDR                    0xAB8
#define C2H_BUF_SZ_2_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_3_ADDR                    0xABC
#define C2H_BUF_SZ_3_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_4_ADDR                    0xAC0
#define C2H_BUF_SZ_4_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_5_ADDR                    0xAC4
#define C2H_BUF_SZ_5_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_7_ADDR                    0XAC8
#define C2H_BUF_SZ_7_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_8_ADDR                    0XACC
#define C2H_BUF_SZ_8_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_9_ADDR                    0xAD0
#define C2H_BUF_SZ_9_SIZE_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_10_ADDR                   0xAD4
#define C2H_BUF_SZ_10_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_11_ADDR                   0xAD8
#define C2H_BUF_SZ_11_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_12_ADDR                   0xAE0
#define C2H_BUF_SZ_12_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_13_ADDR                   0xAE4
#define C2H_BUF_SZ_13_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_14_ADDR                   0xAE8
#define C2H_BUF_SZ_14_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_BUF_SZ_15_ADDR                   0XAEC
#define C2H_BUF_SZ_15_SIZE_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_ERR_STAT_ADDR                    0xAF0
#define C2H_ERR_STAT_RSVD_1_MASK                           GENMASK(31, 16)
#define C2H_ERR_STAT_WRB_PRTY_ERR_MASK                     BIT(15)
#define C2H_ERR_STAT_WRB_CIDX_ERR_MASK                     BIT(14)
#define C2H_ERR_STAT_WRB_QFULL_ERR_MASK                    BIT(13)
#define C2H_ERR_STAT_WRB_INV_Q_ERR_MASK                    BIT(12)
#define C2H_ERR_STAT_PORT_ID_BYP_IN_MISMATCH_MASK          BIT(11)
#define C2H_ERR_STAT_PORT_ID_CTXT_MISMATCH_MASK            BIT(10)
#define C2H_ERR_STAT_ERR_DESC_CNT_MASK                     BIT(9)
#define C2H_ERR_STAT_RSVD_2_MASK                           BIT(8)
#define C2H_ERR_STAT_MSI_INT_FAIL_MASK                     BIT(7)
#define C2H_ERR_STAT_ENG_WPL_DATA_PAR_ERR_MASK             BIT(6)
#define C2H_ERR_STAT_RSVD_3_MASK                           BIT(5)
#define C2H_ERR_STAT_DESC_RSP_ERR_MASK                     BIT(4)
#define C2H_ERR_STAT_QID_MISMATCH_MASK                     BIT(3)
#define C2H_ERR_STAT_RSVD_4_MASK                           BIT(2)
#define C2H_ERR_STAT_LEN_MISMATCH_MASK                     BIT(1)
#define C2H_ERR_STAT_MTY_MISMATCH_MASK                     BIT(0)
#define QDMA_S80_HARD_C2H_ERR_MASK_ADDR                    0xAF4
#define C2H_ERR_EN_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_FATAL_ERR_STAT_ADDR              0xAF8
#define C2H_FATAL_ERR_STAT_RSVD_1_MASK                     GENMASK(31, 19)
#define C2H_FATAL_ERR_STAT_WPL_DATA_PAR_ERR_MASK           BIT(18)
#define C2H_FATAL_ERR_STAT_PLD_FIFO_RAM_RDBE_MASK          BIT(17)
#define C2H_FATAL_ERR_STAT_QID_FIFO_RAM_RDBE_MASK          BIT(16)
#define C2H_FATAL_ERR_STAT_TUSER_FIFO_RAM_RDBE_MASK        BIT(15)
#define C2H_FATAL_ERR_STAT_WRB_COAL_DATA_RAM_RDBE_MASK     BIT(14)
#define C2H_FATAL_ERR_STAT_INT_QID2VEC_RAM_RDBE_MASK       BIT(13)
#define C2H_FATAL_ERR_STAT_INT_CTXT_RAM_RDBE_MASK          BIT(12)
#define C2H_FATAL_ERR_STAT_DESC_REQ_FIFO_RAM_RDBE_MASK     BIT(11)
#define C2H_FATAL_ERR_STAT_PFCH_CTXT_RAM_RDBE_MASK         BIT(10)
#define C2H_FATAL_ERR_STAT_WRB_CTXT_RAM_RDBE_MASK          BIT(9)
#define C2H_FATAL_ERR_STAT_PFCH_LL_RAM_RDBE_MASK           BIT(8)
#define C2H_FATAL_ERR_STAT_TIMER_FIFO_RAM_RDBE_MASK        GENMASK(7, 4)
#define C2H_FATAL_ERR_STAT_QID_MISMATCH_MASK               BIT(3)
#define C2H_FATAL_ERR_STAT_RSVD_2_MASK                     BIT(2)
#define C2H_FATAL_ERR_STAT_LEN_MISMATCH_MASK               BIT(1)
#define C2H_FATAL_ERR_STAT_MTY_MISMATCH_MASK               BIT(0)
#define QDMA_S80_HARD_C2H_FATAL_ERR_MASK_ADDR              0xAFC
#define C2H_FATAL_ERR_C2HEN_MASK                 GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_FATAL_ERR_ENABLE_ADDR            0xB00
#define C2H_FATAL_ERR_ENABLE_RSVD_1_MASK                   GENMASK(31, 2)
#define C2H_FATAL_ERR_ENABLE_WPL_PAR_INV_MASK             BIT(1)
#define C2H_FATAL_ERR_ENABLE_WRQ_DIS_MASK                 BIT(0)
#define QDMA_S80_HARD_GLBL_ERR_INT_ADDR                    0xB04
#define GLBL_ERR_INT_RSVD_1_MASK                           GENMASK(31, 18)
#define GLBL_ERR_INT_ARM_MASK                             BIT(17)
#define GLBL_ERR_INT_EN_COAL_MASK                          BIT(16)
#define GLBL_ERR_INT_VEC_MASK                              GENMASK(15, 8)
#define GLBL_ERR_INT_FUNC_MASK                             GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_PFCH_CFG_ADDR                    0xB08
#define C2H_PFCH_CFG_EVT_QCNT_TH_MASK                      GENMASK(31, 25)
#define C2H_PFCH_CFG_QCNT_MASK                             GENMASK(24, 16)
#define C2H_PFCH_CFG_NUM_MASK                              GENMASK(15, 8)
#define C2H_PFCH_CFG_FL_TH_MASK                            GENMASK(7, 0)
#define QDMA_S80_HARD_C2H_INT_TIMER_TICK_ADDR              0xB0C
#define C2H_INT_TIMER_TICK_MASK                           GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_RSP_DROP_ACCEPTED_ADDR 0xB10
#define C2H_STAT_DESC_RSP_DROP_ACCEPTED_D_MASK             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_RSP_ERR_ACCEPTED_ADDR  0xB14
#define C2H_STAT_DESC_RSP_ERR_ACCEPTED_D_MASK              GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DESC_REQ_ADDR               0xB18
#define C2H_STAT_DESC_REQ_MASK                            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_0_ADDR          0xB1C
#define C2H_STAT_DMA_ENG_0_RSVD_1_MASK                 BIT(31)
#define C2H_STAT_DMA_ENG_0_WRB_FIFO_OUT_CNT_MASK       GENMASK(30, 28)
#define C2H_STAT_DMA_ENG_0_QID_FIFO_OUT_CNT_MASK       GENMASK(27, 18)
#define C2H_STAT_DMA_ENG_0_PLD_FIFO_OUT_CNT_MASK       GENMASK(17, 8)
#define C2H_STAT_DMA_ENG_0_WRQ_FIFO_OUT_CNT_MASK       GENMASK(7, 5)
#define C2H_STAT_DMA_ENG_0_WRB_SM_CS_MASK              BIT(4)
#define C2H_STAT_DMA_ENG_0_MAIN_SM_CS_MASK             GENMASK(3, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_1_ADDR          0xB20
#define C2H_STAT_DMA_ENG_1_RSVD_1_MASK                 BIT(31)
#define C2H_STAT_DMA_ENG_1_DESC_RSP_LAST_MASK          BIT(30)
#define C2H_STAT_DMA_ENG_1_PLD_FIFO_IN_CNT_MASK        GENMASK(29, 20)
#define C2H_STAT_DMA_ENG_1_PLD_FIFO_OUTPUT_CNT_MASK    GENMASK(19, 10)
#define C2H_STAT_DMA_ENG_1_QID_FIFO_IN_CNT_MASK        GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_2_ADDR          0xB24
#define C2H_STAT_DMA_ENG_2_RSVD_1_MASK                 GENMASK(31, 30)
#define C2H_STAT_DMA_ENG_2_WRB_FIFO_IN_CNT_MASK        GENMASK(29, 20)
#define C2H_STAT_DMA_ENG_2_WRB_FIFO_OUTPUT_CNT_MASK    GENMASK(19, 10)
#define C2H_STAT_DMA_ENG_2_QID_FIFO_OUTPUT_CNT_MASK    GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_3_ADDR          0xB28
#define C2H_STAT_DMA_ENG_3_RSVD_1_MASK                 GENMASK(31, 30)
#define C2H_STAT_DMA_ENG_3_ADDR_4K_SPLIT_CNT_MASK      GENMASK(29, 20)
#define C2H_STAT_DMA_ENG_3_WRQ_FIFO_IN_CNT_MASK        GENMASK(19, 10)
#define C2H_STAT_DMA_ENG_3_WRQ_FIFO_OUTPUT_CNT_MASK    GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_DBG_PFCH_ERR_CTXT_ADDR           0xB2C
#define C2H_PFCH_ERR_CTXT_RSVD_1_MASK                  GENMASK(31, 14)
#define C2H_PFCH_ERR_CTXT_ERR_STAT_MASK                BIT(13)
#define C2H_PFCH_ERR_CTXT_CMD_WR_MASK                  BIT(12)
#define C2H_PFCH_ERR_CTXT_QID_MASK                     GENMASK(11, 1)
#define C2H_PFCH_ERR_CTXT_DONE_MASK                    BIT(0)
#define QDMA_S80_HARD_C2H_FIRST_ERR_QID_ADDR               0xB30
#define C2H_FIRST_ERR_QID_RSVD_1_MASK                      GENMASK(31, 21)
#define C2H_FIRST_ERR_QID_ERR_STAT_MASK                    GENMASK(20, 16)
#define C2H_FIRST_ERR_QID_CMD_WR_MASK                      GENMASK(15, 12)
#define C2H_FIRST_ERR_QID_QID_MASK                         GENMASK(11, 0)
#define QDMA_S80_HARD_STAT_NUM_WRB_IN_ADDR                 0xB34
#define STAT_NUM_WRB_IN_RSVD_1_MASK                        GENMASK(31, 16)
#define STAT_NUM_WRB_IN_WRB_CNT_MASK                       GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_WRB_OUT_ADDR                0xB38
#define STAT_NUM_WRB_OUT_RSVD_1_MASK                       GENMASK(31, 16)
#define STAT_NUM_WRB_OUT_WRB_CNT_MASK                      GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_WRB_DRP_ADDR                0xB3C
#define STAT_NUM_WRB_DRP_RSVD_1_MASK                       GENMASK(31, 16)
#define STAT_NUM_WRB_DRP_WRB_CNT_MASK                      GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_STAT_DESC_OUT_ADDR          0xB40
#define STAT_NUM_STAT_DESC_OUT_RSVD_1_MASK                 GENMASK(31, 16)
#define STAT_NUM_STAT_DESC_OUT_CNT_MASK                    GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_DSC_CRDT_SENT_ADDR          0xB44
#define STAT_NUM_DSC_CRDT_SENT_RSVD_1_MASK                 GENMASK(31, 16)
#define STAT_NUM_DSC_CRDT_SENT_CNT_MASK                    GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_FCH_DSC_RCVD_ADDR           0xB48
#define STAT_NUM_FCH_DSC_RCVD_RSVD_1_MASK                  GENMASK(31, 16)
#define STAT_NUM_FCH_DSC_RCVD_DSC_CNT_MASK                 GENMASK(15, 0)
#define QDMA_S80_HARD_STAT_NUM_BYP_DSC_RCVD_ADDR           0xB4C
#define STAT_NUM_BYP_DSC_RCVD_RSVD_1_MASK                  GENMASK(31, 11)
#define STAT_NUM_BYP_DSC_RCVD_DSC_CNT_MASK                 GENMASK(10, 0)
#define QDMA_S80_HARD_C2H_WRB_COAL_CFG_ADDR                0xB50
#define C2H_WRB_COAL_CFG_MAX_BUF_SZ_MASK                   GENMASK(31, 26)
#define C2H_WRB_COAL_CFG_TICK_VAL_MASK                     GENMASK(25, 14)
#define C2H_WRB_COAL_CFG_TICK_CNT_MASK                     GENMASK(13, 2)
#define C2H_WRB_COAL_CFG_SET_GLB_FLUSH_MASK                BIT(1)
#define C2H_WRB_COAL_CFG_DONE_GLB_FLUSH_MASK               BIT(0)
#define QDMA_S80_HARD_C2H_INTR_H2C_REQ_ADDR                0xB54
#define C2H_INTR_H2C_REQ_RSVD_1_MASK                       GENMASK(31, 18)
#define C2H_INTR_H2C_REQ_CNT_MASK                          GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_MM_REQ_ADDR             0xB58
#define C2H_INTR_C2H_MM_REQ_RSVD_1_MASK                    GENMASK(31, 18)
#define C2H_INTR_C2H_MM_REQ_CNT_MASK                       GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_ERR_INT_REQ_ADDR            0xB5C
#define C2H_INTR_ERR_INT_REQ_RSVD_1_MASK                   GENMASK(31, 18)
#define C2H_INTR_ERR_INT_REQ_CNT_MASK                      GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_ST_REQ_ADDR             0xB60
#define C2H_INTR_C2H_ST_REQ_RSVD_1_MASK                    GENMASK(31, 18)
#define C2H_INTR_C2H_ST_REQ_CNT_MASK                       GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_ADDR 0xB64
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_RSVD_1_MASK       GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_CNT_MASK          GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_ADDR 0xB68
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_RSVD_1_MASK      GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_CNT_MASK         GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_ADDR 0xB6C
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_RSVD_1_MASK   GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_CNT_MASK      GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_ADDR 0xB70
#define C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_RSVD_1_MASK     GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_CNT_MASK        GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_ST_MSIX_ACK_ADDR        0xB74
#define C2H_INTR_C2H_ST_MSIX_ACK_RSVD_1_MASK               GENMASK(31, 18)
#define C2H_INTR_C2H_ST_MSIX_ACK_CNT_MASK                  GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_ST_MSIX_FAIL_ADDR       0xB78
#define C2H_INTR_C2H_ST_MSIX_FAIL_RSVD_1_MASK              GENMASK(31, 18)
#define C2H_INTR_C2H_ST_MSIX_FAIL_CNT_MASK                 GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_ST_NO_MSIX_ADDR         0xB7C
#define C2H_INTR_C2H_ST_NO_MSIX_RSVD_1_MASK                GENMASK(31, 18)
#define C2H_INTR_C2H_ST_NO_MSIX_CNT_MASK                   GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_INTR_C2H_ST_CTXT_INVAL_ADDR      0xB80
#define C2H_INTR_C2H_ST_CTXT_INVAL_RSVD_1_MASK             GENMASK(31, 18)
#define C2H_INTR_C2H_ST_CTXT_INVAL_CNT_MASK                GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_STAT_WR_CMP_ADDR                 0xB84
#define C2H_STAT_WR_CMP_RSVD_1_MASK                        GENMASK(31, 18)
#define C2H_STAT_WR_CMP_CNT_MASK                           GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_4_ADDR          0xB88
#define C2H_STAT_DMA_ENG_4_TUSER_FIFO_OUT_VLD_MASK     BIT(31)
#define C2H_STAT_DMA_ENG_4_WRB_FIFO_IN_RDY_MASK        BIT(30)
#define C2H_STAT_DMA_ENG_4_TUSER_FIFO_IN_CNT_MASK      GENMASK(29, 20)
#define C2H_STAT_DMA_ENG_4_TUSER_FIFO_OUTPUT_CNT_MASK  GENMASK(19, 10)
#define C2H_STAT_DMA_ENG_4_TUSER_FIFO_OUT_CNT_MASK     GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_STAT_DBG_DMA_ENG_5_ADDR          0xB8C
#define C2H_STAT_DMA_ENG_5_RSVD_1_MASK                 GENMASK(31, 25)
#define C2H_STAT_DMA_ENG_5_TUSER_COMB_OUT_VLD_MASK     BIT(24)
#define C2H_STAT_DMA_ENG_5_TUSER_FIFO_IN_RDY_MASK      BIT(23)
#define C2H_STAT_DMA_ENG_5_TUSER_COMB_IN_CNT_MASK      GENMASK(22, 13)
#define C2H_STAT_DMA_ENG_5_TUSE_COMB_OUTPUT_CNT_MASK   GENMASK(12, 3)
#define C2H_STAT_DMA_ENG_5_TUSER_COMB_CNT_MASK         GENMASK(2, 0)
#define QDMA_S80_HARD_C2H_DBG_PFCH_QID_ADDR                0xB90
#define C2H_PFCH_QID_RSVD_1_MASK                       GENMASK(31, 15)
#define C2H_PFCH_QID_ERR_CTXT_MASK                     BIT(14)
#define C2H_PFCH_QID_TARGET_MASK                       GENMASK(13, 11)
#define C2H_PFCH_QID_QID_OR_TAG_MASK                   GENMASK(10, 0)
#define QDMA_S80_HARD_C2H_DBG_PFCH_ADDR                    0xB94
#define C2H_PFCH_DATA_MASK                             GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_INT_DBG_ADDR                     0xB98
#define C2H_INT_RSVD_1_MASK                            GENMASK(31, 8)
#define C2H_INT_INT_COAL_SM_MASK                       GENMASK(7, 4)
#define C2H_INT_INT_SM_MASK                            GENMASK(3, 0)
#define QDMA_S80_HARD_C2H_STAT_IMM_ACCEPTED_ADDR           0xB9C
#define C2H_STAT_IMM_ACCEPTED_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_STAT_IMM_ACCEPTED_CNT_MASK                     GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_STAT_MARKER_ACCEPTED_ADDR        0xBA0
#define C2H_STAT_MARKER_ACCEPTED_RSVD_1_MASK               GENMASK(31, 18)
#define C2H_STAT_MARKER_ACCEPTED_CNT_MASK                  GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_STAT_DISABLE_CMP_ACCEPTED_ADDR   0xBA4
#define C2H_STAT_DISABLE_CMP_ACCEPTED_RSVD_1_MASK          GENMASK(31, 18)
#define C2H_STAT_DISABLE_CMP_ACCEPTED_CNT_MASK             GENMASK(17, 0)
#define QDMA_S80_HARD_C2H_PLD_FIFO_CRDT_CNT_ADDR           0xBA8
#define C2H_PLD_FIFO_CRDT_CNT_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_PLD_FIFO_CRDT_CNT_CNT_MASK                     GENMASK(17, 0)
#define QDMA_S80_HARD_H2C_ERR_STAT_ADDR                    0xE00
#define H2C_ERR_STAT_RSVD_1_MASK                           GENMASK(31, 5)
#define H2C_ERR_STAT_SBE_MASK                              BIT(4)
#define H2C_ERR_STAT_DBE_MASK                              BIT(3)
#define H2C_ERR_STAT_NO_DMA_DS_MASK                        BIT(2)
#define H2C_ERR_STAT_SDI_MRKR_REQ_MOP_ERR_MASK             BIT(1)
#define H2C_ERR_STAT_ZERO_LEN_DS_MASK                      BIT(0)
#define QDMA_S80_HARD_H2C_ERR_MASK_ADDR                    0xE04
#define H2C_ERR_EN_MASK                          GENMASK(31, 0)
#define QDMA_S80_HARD_H2C_FIRST_ERR_QID_ADDR               0xE08
#define H2C_FIRST_ERR_QID_RSVD_1_MASK                      GENMASK(31, 20)
#define H2C_FIRST_ERR_QID_ERR_TYPE_MASK                    GENMASK(19, 16)
#define H2C_FIRST_ERR_QID_RSVD_2_MASK                      GENMASK(15, 12)
#define H2C_FIRST_ERR_QID_QID_MASK                         GENMASK(11, 0)
#define QDMA_S80_HARD_H2C_DBG_REG0_ADDR                    0xE0C
#define H2C_REG0_NUM_DSC_RCVD_MASK                     GENMASK(31, 16)
#define H2C_REG0_NUM_WRB_SENT_MASK                     GENMASK(15, 0)
#define QDMA_S80_HARD_H2C_DBG_REG1_ADDR                    0xE10
#define H2C_REG1_NUM_REQ_SENT_MASK                     GENMASK(31, 16)
#define H2C_REG1_NUM_CMP_SENT_MASK                     GENMASK(15, 0)
#define QDMA_S80_HARD_H2C_DBG_REG2_ADDR                    0xE14
#define H2C_REG2_RSVD_1_MASK                           GENMASK(31, 16)
#define H2C_REG2_NUM_ERR_DSC_RCVD_MASK                 GENMASK(15, 0)
#define QDMA_S80_HARD_H2C_DBG_REG3_ADDR                    0xE18
#define H2C_REG3_MASK                              BIT(31)
#define H2C_REG3_DSCO_FIFO_EMPTY_MASK                  BIT(30)
#define H2C_REG3_DSCO_FIFO_FULL_MASK                   BIT(29)
#define H2C_REG3_CUR_RC_STATE_MASK                     GENMASK(28, 26)
#define H2C_REG3_RDREQ_LINES_MASK                      GENMASK(25, 16)
#define H2C_REG3_RDATA_LINES_AVAIL_MASK                GENMASK(15, 6)
#define H2C_REG3_PEND_FIFO_EMPTY_MASK                  BIT(5)
#define H2C_REG3_PEND_FIFO_FULL_MASK                   BIT(4)
#define H2C_REG3_CUR_RQ_STATE_MASK                     GENMASK(3, 2)
#define H2C_REG3_DSCI_FIFO_FULL_MASK                   BIT(1)
#define H2C_REG3_DSCI_FIFO_EMPTY_MASK                  BIT(0)
#define QDMA_S80_HARD_H2C_DBG_REG4_ADDR                    0xE1C
#define H2C_REG4_RDREQ_ADDR_MASK                       GENMASK(31, 0)
#define QDMA_S80_HARD_H2C_FATAL_ERR_EN_ADDR                0xE20
#define H2C_FATAL_ERR_EN_RSVD_1_MASK                       GENMASK(31, 1)
#define H2C_FATAL_ERR_EN_H2C_MASK                          BIT(0)
#define QDMA_S80_HARD_C2H_CHANNEL_CTL_ADDR                 0x1004
#define C2H_CHANNEL_CTL_RSVD_1_MASK                        GENMASK(31, 1)
#define C2H_CHANNEL_CTL_RUN_MASK                           BIT(0)
#define QDMA_S80_HARD_C2H_CHANNEL_CTL_1_ADDR               0x1008
#define C2H_CHANNEL_CTL_1_RUN_MASK                         GENMASK(31, 1)
#define C2H_CHANNEL_CTL_1_RUN_1_MASK                       BIT(0)
#define QDMA_S80_HARD_C2H_MM_STATUS_ADDR                   0x1040
#define C2H_MM_STATUS_RSVD_1_MASK                          GENMASK(31, 1)
#define C2H_MM_STATUS_RUN_MASK                             BIT(0)
#define QDMA_S80_HARD_C2H_CHANNEL_CMPL_DESC_CNT_ADDR       0x1048
#define C2H_CHANNEL_CMPL_DESC_CNT_C2H_CO_MASK              GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_MM_ERR_CODE_ENABLE_MASK_ADDR     0x1054
#define C2H_MM_ERR_CODE_ENABLE_RSVD_1_MASK            BIT(31)
#define C2H_MM_ERR_CODE_ENABLE_WR_UC_RAM_MASK         BIT(30)
#define C2H_MM_ERR_CODE_ENABLE_WR_UR_MASK             BIT(29)
#define C2H_MM_ERR_CODE_ENABLE_WR_FLR_MASK            BIT(28)
#define C2H_MM_ERR_CODE_ENABLE_RSVD_2_MASK            GENMASK(27, 2)
#define C2H_MM_ERR_CODE_ENABLE_RD_SLV_ERR_MASK        BIT(1)
#define C2H_MM_ERR_CODE_ENABLE_WR_SLV_ERR_MASK        BIT(0)
#define QDMA_S80_HARD_C2H_MM_ERR_CODE_ADDR                 0x1058
#define C2H_MM_ERR_CODE_RSVD_1_MASK                        GENMASK(31, 18)
#define C2H_MM_ERR_CODE_VALID_MASK                         BIT(17)
#define C2H_MM_ERR_CODE_RDWR_MASK                          BIT(16)
#define C2H_MM_ERR_CODE_MASK                              GENMASK(4, 0)
#define QDMA_S80_HARD_C2H_MM_ERR_INFO_ADDR                 0x105C
#define C2H_MM_ERR_INFO_RSVD_1_MASK                        GENMASK(31, 29)
#define C2H_MM_ERR_INFO_QID_MASK                           GENMASK(28, 17)
#define C2H_MM_ERR_INFO_DIR_MASK                           BIT(16)
#define C2H_MM_ERR_INFO_CIDX_MASK                          GENMASK(15, 0)
#define QDMA_S80_HARD_C2H_MM_PERF_MON_CTL_ADDR             0x10C0
#define C2H_MM_PERF_MON_CTL_RSVD_1_MASK                    GENMASK(31, 4)
#define C2H_MM_PERF_MON_CTL_IMM_START_MASK                 BIT(3)
#define C2H_MM_PERF_MON_CTL_RUN_START_MASK                 BIT(2)
#define C2H_MM_PERF_MON_CTL_IMM_CLEAR_MASK                 BIT(1)
#define C2H_MM_PERF_MON_CTL_RUN_CLEAR_MASK                 BIT(0)
#define QDMA_S80_HARD_C2H_MM_PERF_MON_CYCLE_CNT0_ADDR      0x10C4
#define C2H_MM_PERF_MON_CYCLE_CNT0_CYC_CNT_MASK            GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_MM_PERF_MON_CYCLE_CNT1_ADDR      0x10C8
#define C2H_MM_PERF_MON_CYCLE_CNT1_RSVD_1_MASK             GENMASK(31, 10)
#define C2H_MM_PERF_MON_CYCLE_CNT1_CYC_CNT_MASK            GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_MM_PERF_MON_DATA_CNT0_ADDR       0x10CC
#define C2H_MM_PERF_MON_DATA_CNT0_DCNT_MASK                GENMASK(31, 0)
#define QDMA_S80_HARD_C2H_MM_PERF_MON_DATA_CNT1_ADDR       0x10D0
#define C2H_MM_PERF_MON_DATA_CNT1_RSVD_1_MASK              GENMASK(31, 10)
#define C2H_MM_PERF_MON_DATA_CNT1_DCNT_MASK                GENMASK(9, 0)
#define QDMA_S80_HARD_C2H_MM_DBG_ADDR                      0x10E8
#define C2H_MM_RSVD_1_MASK                             GENMASK(31, 24)
#define C2H_MM_RRQ_ENTRIES_MASK                        GENMASK(23, 17)
#define C2H_MM_DAT_FIFO_SPC_MASK                       GENMASK(16, 7)
#define C2H_MM_RD_STALL_MASK                           BIT(6)
#define C2H_MM_RRQ_FIFO_FI_MASK                        BIT(5)
#define C2H_MM_WR_STALL_MASK                           BIT(4)
#define C2H_MM_WRQ_FIFO_FI_MASK                        BIT(3)
#define C2H_MM_WBK_STALL_MASK                          BIT(2)
#define C2H_MM_DSC_FIFO_EP_MASK                        BIT(1)
#define C2H_MM_DSC_FIFO_FL_MASK                        BIT(0)
#define QDMA_S80_HARD_H2C_CHANNEL_CTL_ADDR                 0x1204
#define H2C_CHANNEL_CTL_RSVD_1_MASK                        GENMASK(31, 1)
#define H2C_CHANNEL_CTL_RUN_MASK                           BIT(0)
#define QDMA_S80_HARD_H2C_CHANNEL_CTL_1_ADDR               0x1208
#define H2C_CHANNEL_CTL_1_RUN_MASK                         BIT(0)
#define QDMA_S80_HARD_H2C_CHANNEL_CTL_2_ADDR               0x120C
#define H2C_CHANNEL_CTL_2_RUN_MASK                         BIT(0)
#define QDMA_S80_HARD_H2C_MM_STATUS_ADDR                   0x1240
#define H2C_MM_STATUS_RSVD_1_MASK                          GENMASK(31, 1)
#define H2C_MM_STATUS_RUN_MASK                             BIT(0)
#define QDMA_S80_HARD_H2C_CHANNEL_CMPL_DESC_CNT_ADDR       0x1248
#define H2C_CHANNEL_CMPL_DESC_CNT_H2C_CO_MASK              GENMASK(31, 0)
#define QDMA_S80_HARD_H2C_MM_ERR_CODE_ENABLE_MASK_ADDR     0x1254
#define H2C_MM_ERR_CODE_ENABLE_RSVD_1_MASK            GENMASK(31, 30)
#define H2C_MM_ERR_CODE_ENABLE_WR_SLV_ERR_MASK        BIT(29)
#define H2C_MM_ERR_CODE_ENABLE_WR_DEC_ERR_MASK        BIT(28)
#define H2C_MM_ERR_CODE_ENABLE_RSVD_2_MASK            GENMASK(27, 23)
#define H2C_MM_ERR_CODE_ENABLE_RD_RQ_DIS_ERR_MASK     BIT(22)
#define H2C_MM_ERR_CODE_ENABLE_RSVD_3_MASK            GENMASK(21, 17)
#define H2C_MM_ERR_CODE_ENABLE_RD_DAT_POISON_ERR_MASK BIT(16)
#define H2C_MM_ERR_CODE_ENABLE_RSVD_4_MASK            GENMASK(15, 9)
#define H2C_MM_ERR_CODE_ENABLE_RD_FLR_ERR_MASK        BIT(8)
#define H2C_MM_ERR_CODE_ENABLE_RSVD_5_MASK            GENMASK(7, 6)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_ADR_ERR_MASK    BIT(5)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_PARA_MASK       BIT(4)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_BYTE_ERR_MASK   BIT(3)
#define H2C_MM_ERR_CODE_ENABLE_RD_UR_CA_MASK          BIT(2)
#define H2C_MM_ERR_CODE_ENABLE_RD_HRD_POISON_ERR_MASK BIT(1)
#define H2C_MM_ERR_CODE_ENABLE_RSVD_6_MASK            BIT(0)
#define QDMA_S80_HARD_H2C_MM_ERR_CODE_ADDR                 0x1258
#define H2C_MM_ERR_CODE_RSVD_1_MASK                        GENMASK(31, 18)
#define H2C_MM_ERR_CODE_VALID_MASK                         BIT(17)
#define H2C_MM_ERR_CODE_RDWR_MASK                          BIT(16)
#define H2C_MM_ERR_CODE_MASK                              GENMASK(4, 0)
#define QDMA_S80_HARD_H2C_MM_ERR_INFO_ADDR                 0x125C
#define H2C_MM_ERR_INFO_RSVD_1_MASK                        GENMASK(31, 29)
#define H2C_MM_ERR_INFO_QID_MASK                           GENMASK(28, 17)
#define H2C_MM_ERR_INFO_DIR_MASK                           BIT(16)
#define H2C_MM_ERR_INFO_CIDX_MASK                          GENMASK(15, 0)
#define QDMA_S80_HARD_H2C_MM_PERF_MON_CTL_ADDR             0x12C0
#define H2C_MM_PERF_MON_CTL_RSVD_1_MASK                    GENMASK(31, 4)
#define H2C_MM_PERF_MON_CTL_IMM_START_MASK                 BIT(3)
#define H2C_MM_PERF_MON_CTL_RUN_START_MASK                 BIT(2)
#define H2C_MM_PERF_MON_CTL_IMM_CLEAR_MASK                 BIT(1)
#define H2C_MM_PERF_MON_CTL_RUN_CLEAR_MASK                 BIT(0)
#define QDMA_S80_HARD_H2C_MM_PERF_MON_CYCLE_CNT0_ADDR      0x12C4
#define H2C_MM_PERF_MON_CYCLE_CNT0_CYC_CNT_MASK            GENMASK(31, 0)
#define QDMA_S80_HARD_H2C_MM_PERF_MON_CYCLE_CNT1_ADDR      0x12C8
#define H2C_MM_PERF_MON_CYCLE_CNT1_RSVD_1_MASK             GENMASK(31, 10)
#define H2C_MM_PERF_MON_CYCLE_CNT1_CYC_CNT_MASK            GENMASK(9, 0)
#define QDMA_S80_HARD_H2C_MM_PERF_MON_DATA_CNT0_ADDR       0x12CC
#define H2C_MM_PERF_MON_DATA_CNT0_DCNT_MASK                GENMASK(31, 0)
#define QDMA_S80_HARD_H2C_MM_PERF_MON_DATA_CNT1_ADDR       0x12D0
#define H2C_MM_PERF_MON_DATA_CNT1_RSVD_1_MASK              GENMASK(31, 10)
#define H2C_MM_PERF_MON_DATA_CNT1_DCNT_MASK                GENMASK(9, 0)
#define QDMA_S80_HARD_H2C_MM_DBG_ADDR                      0x12E8
#define H2C_MM_RSVD_1_MASK                             GENMASK(31, 24)
#define H2C_MM_RRQ_ENTRIES_MASK                        GENMASK(23, 17)
#define H2C_MM_DAT_FIFO_SPC_MASK                       GENMASK(16, 7)
#define H2C_MM_RD_STALL_MASK                           BIT(6)
#define H2C_MM_RRQ_FIFO_FI_MASK                        BIT(5)
#define H2C_MM_WR_STALL_MASK                           BIT(4)
#define H2C_MM_WRQ_FIFO_FI_MASK                        BIT(3)
#define H2C_MM_WBK_STALL_MASK                          BIT(2)
#define H2C_MM_DSC_FIFO_EP_MASK                        BIT(1)
#define H2C_MM_DSC_FIFO_FL_MASK                        BIT(0)
#define QDMA_S80_HARD_FUNC_STATUS_REG_ADDR                 0x2400
#define FUNC_STATUS_REG_RSVD_1_MASK                        GENMASK(31, 12)
#define FUNC_STATUS_REG_CUR_SRC_FN_MASK                    GENMASK(11, 4)
#define FUNC_STATUS_REG_ACK_MASK                           BIT(2)
#define FUNC_STATUS_REG_O_MSG_MASK                         BIT(1)
#define FUNC_STATUS_REG_I_MSG_MASK                         BIT(0)
#define QDMA_S80_HARD_FUNC_CMD_REG_ADDR                    0x2404
#define FUNC_CMD_REG_RSVD_1_MASK                           GENMASK(31, 3)
#define FUNC_CMD_REG_RSVD_2_MASK                           BIT(2)
#define FUNC_CMD_REG_MSG_RCV_MASK                          BIT(1)
#define FUNC_CMD_REG_MSG_SENT_MASK                         BIT(0)
#define QDMA_S80_HARD_FUNC_INTERRUPT_VECTOR_REG_ADDR       0x2408
#define FUNC_INTERRUPT_VECTOR_REG_RSVD_1_MASK              GENMASK(31, 5)
#define FUNC_INTERRUPT_VECTOR_REG_IN_MASK                  GENMASK(4, 0)
#define QDMA_S80_HARD_TARGET_FUNC_REG_ADDR                 0x240C
#define TARGET_FUNC_REG_RSVD_1_MASK                        GENMASK(31, 8)
#define TARGET_FUNC_REG_N_ID_MASK                          GENMASK(7, 0)
#define QDMA_S80_HARD_FUNC_INTERRUPT_CTL_REG_ADDR          0x2410
#define FUNC_INTERRUPT_CTL_REG_RSVD_1_MASK                 GENMASK(31, 1)
#define FUNC_INTERRUPT_CTL_REG_INT_EN_MASK                 BIT(0)
#define SW_IND_CTXT_DATA_W3_DSC_BASE_H_MASK               GENMASK(31, 0)
#define SW_IND_CTXT_DATA_W2_DSC_BASE_L_MASK               GENMASK(31, 0)
#define SW_IND_CTXT_DATA_W1_IS_MM_MASK                    BIT(31)
#define SW_IND_CTXT_DATA_W1_MRKR_DIS_MASK                 BIT(30)
#define SW_IND_CTXT_DATA_W1_IRQ_REQ_MASK                  BIT(29)
#define SW_IND_CTXT_DATA_W1_ERR_WB_SENT_MASK              BIT(28)
#define SW_IND_CTXT_DATA_W1_ERR_MASK                      GENMASK(27, 26)
#define SW_IND_CTXT_DATA_W1_IRQ_NO_LAST_MASK              BIT(25)
#define SW_IND_CTXT_DATA_W1_PORT_ID_MASK                  GENMASK(24, 22)
#define SW_IND_CTXT_DATA_W1_IRQ_EN_MASK                   BIT(21)
#define SW_IND_CTXT_DATA_W1_WBK_EN_MASK                   BIT(20)
#define SW_IND_CTXT_DATA_W1_MM_CHN_MASK                   BIT(19)
#define SW_IND_CTXT_DATA_W1_BYPASS_MASK                   BIT(18)
#define SW_IND_CTXT_DATA_W1_DSC_SZ_MASK                   GENMASK(17, 16)
#define SW_IND_CTXT_DATA_W1_RNG_SZ_MASK                   GENMASK(15, 12)
#define SW_IND_CTXT_DATA_W1_FNC_ID_MASK                   GENMASK(11, 4)
#define SW_IND_CTXT_DATA_W1_WBI_INTVL_EN_MASK             BIT(3)
#define SW_IND_CTXT_DATA_W1_WBI_CHK_MASK                  BIT(2)
#define SW_IND_CTXT_DATA_W1_FCRD_EN_MASK                  BIT(1)
#define SW_IND_CTXT_DATA_W1_QEN_MASK                      BIT(0)
#define SW_IND_CTXT_DATA_W0_RSV_MASK                      GENMASK(31, 17)
#define SW_IND_CTXT_DATA_W0_IRQ_ARM_MASK                  BIT(16)
#define SW_IND_CTXT_DATA_W0_PIDX_MASK                     GENMASK(15, 0)
#define HW_IND_CTXT_DATA_W1_RSVD_MASK                     GENMASK(15, 11)
#define HW_IND_CTXT_DATA_W1_FETCH_PND_MASK                BIT(10)
#define HW_IND_CTXT_DATA_W1_IDL_STP_B_MASK                BIT(9)
#define HW_IND_CTXT_DATA_W1_DSC_PND_MASK                  BIT(8)
#define HW_IND_CTXT_DATA_W1_RSVD_1_MASK                   GENMASK(7, 0)
#define HW_IND_CTXT_DATA_W0_CRD_USE_MASK                  GENMASK(31, 16)
#define HW_IND_CTXT_DATA_W0_CIDX_MASK                     GENMASK(15, 0)
#define CRED_CTXT_DATA_W0_RSVD_1_MASK                     GENMASK(31, 16)
#define CRED_CTXT_DATA_W0_CREDT_MASK                      GENMASK(15, 0)
#define PREFETCH_CTXT_DATA_W1_VALID_MASK                  BIT(13)
#define PREFETCH_CTXT_DATA_W1_SW_CRDT_H_MASK              GENMASK(12, 0)
#define PREFETCH_CTXT_DATA_W0_SW_CRDT_L_MASK              GENMASK(31, 29)
#define PREFETCH_CTXT_DATA_W0_PFCH_MASK                   BIT(28)
#define PREFETCH_CTXT_DATA_W0_PFCH_EN_MASK                BIT(27)
#define PREFETCH_CTXT_DATA_W0_ERR_MASK                    BIT(26)
#define PREFETCH_CTXT_DATA_W0_RSVD_MASK                   GENMASK(25, 8)
#define PREFETCH_CTXT_DATA_W0_PORT_ID_MASK                GENMASK(7, 5)
#define PREFETCH_CTXT_DATA_W0_BUF_SIZE_IDX_MASK           GENMASK(4, 1)
#define PREFETCH_CTXT_DATA_W0_BYPASS_MASK                 BIT(0)
#define CMPL_CTXT_DATA_W3_RSVD_MASK                       GENMASK(31, 30)
#define CMPL_CTXT_DATA_W3_FULL_UPD_MASK                   BIT(29)
#define CMPL_CTXT_DATA_W3_TIMER_RUNNING_MASK              BIT(28)
#define CMPL_CTXT_DATA_W3_USER_TRIG_PEND_MASK             BIT(27)
#define CMPL_CTXT_DATA_W3_ERR_MASK                        GENMASK(26, 25)
#define CMPL_CTXT_DATA_W3_VALID_MASK                      BIT(24)
#define CMPL_CTXT_DATA_W3_CIDX_MASK                       GENMASK(23, 8)
#define CMPL_CTXT_DATA_W3_PIDX_H_MASK                     GENMASK(7, 0)
#define CMPL_CTXT_DATA_W2_PIDX_L_MASK                     GENMASK(31, 24)
#define CMPL_CTXT_DATA_W2_DESC_SIZE_MASK                  GENMASK(23, 22)
#define CMPL_CTXT_DATA_W2_BADDR_64_H_MASK                 GENMASK(21, 0)
#define CMPL_CTXT_DATA_W1_BADDR_64_M_MASK                 GENMASK(31, 0)
#define CMPL_CTXT_DATA_W0_BADDR_64_L_MASK                 GENMASK(31, 28)
#define CMPL_CTXT_DATA_W0_QSIZE_IDX_MASK                  GENMASK(27, 24)
#define CMPL_CTXT_DATA_W0_COLOR_MASK                      BIT(23)
#define CMPL_CTXT_DATA_W0_INT_ST_MASK                     GENMASK(22, 21)
#define CMPL_CTXT_DATA_W0_TIMER_IDX_MASK                  GENMASK(20, 17)
#define CMPL_CTXT_DATA_W0_CNTER_IDX_MASK                  GENMASK(16, 13)
#define CMPL_CTXT_DATA_W0_FNC_ID_MASK                     GENMASK(12, 5)
#define CMPL_CTXT_DATA_W0_TRIG_MODE_MASK                  GENMASK(4, 2)
#define CMPL_CTXT_DATA_W0_EN_INT_MASK                     BIT(1)
#define CMPL_CTXT_DATA_W0_EN_STAT_DESC_MASK               BIT(0)
#define INTR_CTXT_DATA_W2_PIDX_MASK                       GENMASK(11, 0)
#define INTR_CTXT_DATA_W1_PAGE_SIZE_MASK                  GENMASK(31, 29)
#define INTR_CTXT_DATA_W1_BADDR_4K_H_MASK                 GENMASK(28, 0)
#define INTR_CTXT_DATA_W0_BADDR_4K_L_MASK                 GENMASK(31, 9)
#define INTR_CTXT_DATA_W0_COLOR_MASK                      BIT(8)
#define INTR_CTXT_DATA_W0_INT_ST_MASK                     BIT(7)
#define INTR_CTXT_DATA_W0_RSVD_MASK                       BIT(6)
#define INTR_CTXT_DATA_W0_VEC_MASK                        GENMASK(5, 1)
#define INTR_CTXT_DATA_W0_VALID_MASK                      BIT(0)

#ifdef __cplusplus
}
#endif

#endif

