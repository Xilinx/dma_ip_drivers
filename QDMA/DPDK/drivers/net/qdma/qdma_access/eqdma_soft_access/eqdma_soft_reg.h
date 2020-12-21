/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
 *
 * BSD LICENSE
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

#ifndef __EQDMA_SOFT_REG_H
#define __EQDMA_SOFT_REG_H


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


uint32_t eqdma_config_num_regs_get(void);
struct xreg_info *eqdma_config_regs_get(void);
#define EQDMA_CFG_BLK_IDENTIFIER_ADDR                      0x00
#define CFG_BLK_IDENTIFIER_MASK                           GENMASK(31, 20)
#define CFG_BLK_IDENTIFIER_1_MASK                         GENMASK(19, 16)
#define CFG_BLK_IDENTIFIER_RSVD_1_MASK                     GENMASK(15, 8)
#define CFG_BLK_IDENTIFIER_VERSION_MASK                    GENMASK(7, 0)
#define EQDMA_CFG_BLK_PCIE_MAX_PLD_SIZE_ADDR               0x08
#define CFG_BLK_PCIE_MAX_PLD_SIZE_RSVD_1_MASK              GENMASK(31, 7)
#define CFG_BLK_PCIE_MAX_PLD_SIZE_PROG_MASK                GENMASK(6, 4)
#define CFG_BLK_PCIE_MAX_PLD_SIZE_RSVD_2_MASK              BIT(3)
#define CFG_BLK_PCIE_MAX_PLD_SIZE_ISSUED_MASK              GENMASK(2, 0)
#define EQDMA_CFG_BLK_PCIE_MAX_READ_REQ_SIZE_ADDR          0x0C
#define CFG_BLK_PCIE_MAX_READ_REQ_SIZE_RSVD_1_MASK         GENMASK(31, 7)
#define CFG_BLK_PCIE_MAX_READ_REQ_SIZE_PROG_MASK           GENMASK(6, 4)
#define CFG_BLK_PCIE_MAX_READ_REQ_SIZE_RSVD_2_MASK         BIT(3)
#define CFG_BLK_PCIE_MAX_READ_REQ_SIZE_ISSUED_MASK         GENMASK(2, 0)
#define EQDMA_CFG_BLK_SYSTEM_ID_ADDR                       0x10
#define CFG_BLK_SYSTEM_ID_RSVD_1_MASK                      GENMASK(31, 17)
#define CFG_BLK_SYSTEM_ID_INST_TYPE_MASK                   BIT(16)
#define CFG_BLK_SYSTEM_ID_MASK                            GENMASK(15, 0)
#define EQDMA_CFG_BLK_MSIX_ENABLE_ADDR                     0x014
#define CFG_BLK_MSIX_ENABLE_MASK                          GENMASK(31, 0)
#define EQDMA_CFG_PCIE_DATA_WIDTH_ADDR                     0x18
#define CFG_PCIE_DATA_WIDTH_RSVD_1_MASK                    GENMASK(31, 3)
#define CFG_PCIE_DATA_WIDTH_DATAPATH_MASK                  GENMASK(2, 0)
#define EQDMA_CFG_PCIE_CTL_ADDR                            0x1C
#define CFG_PCIE_CTL_RSVD_1_MASK                           GENMASK(31, 18)
#define CFG_PCIE_CTL_MGMT_AXIL_CTRL_MASK                   GENMASK(17, 16)
#define CFG_PCIE_CTL_RSVD_2_MASK                           GENMASK(15, 2)
#define CFG_PCIE_CTL_RRQ_DISABLE_MASK                      BIT(1)
#define CFG_PCIE_CTL_RELAXED_ORDERING_MASK                 BIT(0)
#define EQDMA_CFG_BLK_MSI_ENABLE_ADDR                      0x20
#define CFG_BLK_MSI_ENABLE_MASK                           GENMASK(31, 0)
#define EQDMA_CFG_AXI_USER_MAX_PLD_SIZE_ADDR               0x40
#define CFG_AXI_USER_MAX_PLD_SIZE_RSVD_1_MASK              GENMASK(31, 7)
#define CFG_AXI_USER_MAX_PLD_SIZE_ISSUED_MASK              GENMASK(6, 4)
#define CFG_AXI_USER_MAX_PLD_SIZE_RSVD_2_MASK              BIT(3)
#define CFG_AXI_USER_MAX_PLD_SIZE_PROG_MASK                GENMASK(2, 0)
#define EQDMA_CFG_AXI_USER_MAX_READ_REQ_SIZE_ADDR          0x44
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_RSVD_1_MASK         GENMASK(31, 7)
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_USISSUED_MASK       GENMASK(6, 4)
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_RSVD_2_MASK         BIT(3)
#define CFG_AXI_USER_MAX_READ_REQ_SIZE_USPROG_MASK         GENMASK(2, 0)
#define EQDMA_CFG_BLK_MISC_CTL_ADDR                        0x4C
#define CFG_BLK_MISC_CTL_RSVD_1_MASK                       GENMASK(31, 24)
#define CFG_BLK_MISC_CTL_10B_TAG_EN_MASK                   BIT(23)
#define CFG_BLK_MISC_CTL_RSVD_2_MASK                       BIT(22)
#define CFG_BLK_MISC_CTL_AXI_WBK_MASK                      BIT(21)
#define CFG_BLK_MISC_CTL_AXI_DSC_MASK                      BIT(20)
#define CFG_BLK_MISC_CTL_NUM_TAG_MASK                      GENMASK(19, 8)
#define CFG_BLK_MISC_CTL_RSVD_3_MASK                       GENMASK(7, 5)
#define CFG_BLK_MISC_CTL_RQ_METERING_MULTIPLIER_MASK       GENMASK(4, 0)
#define EQDMA_CFG_PL_CRED_CTL_ADDR                         0x68
#define CFG_PL_CRED_CTL_RSVD_1_MASK                        GENMASK(31, 5)
#define CFG_PL_CRED_CTL_SLAVE_CRD_RLS_MASK                 BIT(4)
#define CFG_PL_CRED_CTL_RSVD_2_MASK                        GENMASK(3, 1)
#define CFG_PL_CRED_CTL_MASTER_CRD_RST_MASK                BIT(0)
#define EQDMA_CFG_BLK_SCRATCH_ADDR                         0x80
#define CFG_BLK_SCRATCH_MASK                              GENMASK(31, 0)
#define EQDMA_CFG_GIC_ADDR                                 0xA0
#define CFG_GIC_RSVD_1_MASK                                GENMASK(31, 1)
#define CFG_GIC_GIC_IRQ_MASK                               BIT(0)
#define EQDMA_RAM_SBE_MSK_1_A_ADDR                         0xE0
#define RAM_SBE_MSK_1_A_MASK                          GENMASK(31, 0)
#define EQDMA_RAM_SBE_STS_1_A_ADDR                         0xE4
#define RAM_SBE_STS_1_A_RSVD_MASK                          GENMASK(31, 5)
#define RAM_SBE_STS_1_A_PFCH_CTXT_CAM_RAM_1_MASK           BIT(4)
#define RAM_SBE_STS_1_A_PFCH_CTXT_CAM_RAM_0_MASK           BIT(3)
#define RAM_SBE_STS_1_A_TAG_EVEN_RAM_MASK                  BIT(2)
#define RAM_SBE_STS_1_A_TAG_ODD_RAM_MASK                   BIT(1)
#define RAM_SBE_STS_1_A_RC_RRQ_EVEN_RAM_MASK               BIT(0)
#define EQDMA_RAM_DBE_MSK_1_A_ADDR                         0xE8
#define RAM_DBE_MSK_1_A_MASK                          GENMASK(31, 0)
#define EQDMA_RAM_DBE_STS_1_A_ADDR                         0xEC
#define RAM_DBE_STS_1_A_RSVD_MASK                          GENMASK(31, 5)
#define RAM_DBE_STS_1_A_PFCH_CTXT_CAM_RAM_1_MASK           BIT(4)
#define RAM_DBE_STS_1_A_PFCH_CTXT_CAM_RAM_0_MASK           BIT(3)
#define RAM_DBE_STS_1_A_TAG_EVEN_RAM_MASK                  BIT(2)
#define RAM_DBE_STS_1_A_TAG_ODD_RAM_MASK                   BIT(1)
#define RAM_DBE_STS_1_A_RC_RRQ_EVEN_RAM_MASK               BIT(0)
#define EQDMA_RAM_SBE_MSK_A_ADDR                           0xF0
#define RAM_SBE_MSK_A_MASK                            GENMASK(31, 0)
#define EQDMA_RAM_SBE_STS_A_ADDR                           0xF4
#define RAM_SBE_STS_A_RC_RRQ_ODD_RAM_MASK                  BIT(31)
#define RAM_SBE_STS_A_PEND_FIFO_RAM_MASK                   BIT(30)
#define RAM_SBE_STS_A_PFCH_LL_RAM_MASK                     BIT(29)
#define RAM_SBE_STS_A_WRB_CTXT_RAM_MASK                    BIT(28)
#define RAM_SBE_STS_A_PFCH_CTXT_RAM_MASK                   BIT(27)
#define RAM_SBE_STS_A_DESC_REQ_FIFO_RAM_MASK               BIT(26)
#define RAM_SBE_STS_A_INT_CTXT_RAM_MASK                    BIT(25)
#define RAM_SBE_STS_A_WRB_COAL_DATA_RAM_MASK               BIT(24)
#define RAM_SBE_STS_A_QID_FIFO_RAM_MASK                    BIT(23)
#define RAM_SBE_STS_A_TIMER_FIFO_RAM_MASK                  GENMASK(22, 19)
#define RAM_SBE_STS_A_MI_TL_SLV_FIFO_RAM_MASK              BIT(18)
#define RAM_SBE_STS_A_DSC_CPLD_MASK                        BIT(17)
#define RAM_SBE_STS_A_DSC_CPLI_MASK                        BIT(16)
#define RAM_SBE_STS_A_DSC_SW_CTXT_MASK                     BIT(15)
#define RAM_SBE_STS_A_DSC_CRD_RCV_MASK                     BIT(14)
#define RAM_SBE_STS_A_DSC_HW_CTXT_MASK                     BIT(13)
#define RAM_SBE_STS_A_FUNC_MAP_MASK                        BIT(12)
#define RAM_SBE_STS_A_C2H_WR_BRG_DAT_MASK                  BIT(11)
#define RAM_SBE_STS_A_C2H_RD_BRG_DAT_MASK                  BIT(10)
#define RAM_SBE_STS_A_H2C_WR_BRG_DAT_MASK                  BIT(9)
#define RAM_SBE_STS_A_H2C_RD_BRG_DAT_MASK                  BIT(8)
#define RAM_SBE_STS_A_MI_C2H3_DAT_MASK                     BIT(7)
#define RAM_SBE_STS_A_MI_C2H2_DAT_MASK                     BIT(6)
#define RAM_SBE_STS_A_MI_C2H1_DAT_MASK                     BIT(5)
#define RAM_SBE_STS_A_MI_C2H0_DAT_MASK                     BIT(4)
#define RAM_SBE_STS_A_MI_H2C3_DAT_MASK                     BIT(3)
#define RAM_SBE_STS_A_MI_H2C2_DAT_MASK                     BIT(2)
#define RAM_SBE_STS_A_MI_H2C1_DAT_MASK                     BIT(1)
#define RAM_SBE_STS_A_MI_H2C0_DAT_MASK                     BIT(0)
#define EQDMA_RAM_DBE_MSK_A_ADDR                           0xF8
#define RAM_DBE_MSK_A_MASK                            GENMASK(31, 0)
#define EQDMA_RAM_DBE_STS_A_ADDR                           0xFC
#define RAM_DBE_STS_A_RC_RRQ_ODD_RAM_MASK                  BIT(31)
#define RAM_DBE_STS_A_PEND_FIFO_RAM_MASK                   BIT(30)
#define RAM_DBE_STS_A_PFCH_LL_RAM_MASK                     BIT(29)
#define RAM_DBE_STS_A_WRB_CTXT_RAM_MASK                    BIT(28)
#define RAM_DBE_STS_A_PFCH_CTXT_RAM_MASK                   BIT(27)
#define RAM_DBE_STS_A_DESC_REQ_FIFO_RAM_MASK               BIT(26)
#define RAM_DBE_STS_A_INT_CTXT_RAM_MASK                    BIT(25)
#define RAM_DBE_STS_A_WRB_COAL_DATA_RAM_MASK               BIT(24)
#define RAM_DBE_STS_A_QID_FIFO_RAM_MASK                    BIT(23)
#define RAM_DBE_STS_A_TIMER_FIFO_RAM_MASK                  GENMASK(22, 19)
#define RAM_DBE_STS_A_MI_TL_SLV_FIFO_RAM_MASK              BIT(18)
#define RAM_DBE_STS_A_DSC_CPLD_MASK                        BIT(17)
#define RAM_DBE_STS_A_DSC_CPLI_MASK                        BIT(16)
#define RAM_DBE_STS_A_DSC_SW_CTXT_MASK                     BIT(15)
#define RAM_DBE_STS_A_DSC_CRD_RCV_MASK                     BIT(14)
#define RAM_DBE_STS_A_DSC_HW_CTXT_MASK                     BIT(13)
#define RAM_DBE_STS_A_FUNC_MAP_MASK                        BIT(12)
#define RAM_DBE_STS_A_C2H_WR_BRG_DAT_MASK                  BIT(11)
#define RAM_DBE_STS_A_C2H_RD_BRG_DAT_MASK                  BIT(10)
#define RAM_DBE_STS_A_H2C_WR_BRG_DAT_MASK                  BIT(9)
#define RAM_DBE_STS_A_H2C_RD_BRG_DAT_MASK                  BIT(8)
#define RAM_DBE_STS_A_MI_C2H3_DAT_MASK                     BIT(7)
#define RAM_DBE_STS_A_MI_C2H2_DAT_MASK                     BIT(6)
#define RAM_DBE_STS_A_MI_C2H1_DAT_MASK                     BIT(5)
#define RAM_DBE_STS_A_MI_C2H0_DAT_MASK                     BIT(4)
#define RAM_DBE_STS_A_MI_H2C3_DAT_MASK                     BIT(3)
#define RAM_DBE_STS_A_MI_H2C2_DAT_MASK                     BIT(2)
#define RAM_DBE_STS_A_MI_H2C1_DAT_MASK                     BIT(1)
#define RAM_DBE_STS_A_MI_H2C0_DAT_MASK                     BIT(0)
#define EQDMA_GLBL2_IDENTIFIER_ADDR                        0x100
#define GLBL2_IDENTIFIER_MASK                             GENMASK(31, 8)
#define GLBL2_IDENTIFIER_VERSION_MASK                      GENMASK(7, 0)
#define EQDMA_GLBL2_CHANNEL_INST_ADDR                      0x114
#define GLBL2_CHANNEL_INST_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_INST_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_INST_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_INST_RSVD_2_MASK                     GENMASK(15, 12)
#define GLBL2_CHANNEL_INST_C2H_ENG_MASK                    GENMASK(11, 8)
#define GLBL2_CHANNEL_INST_RSVD_3_MASK                     GENMASK(7, 4)
#define GLBL2_CHANNEL_INST_H2C_ENG_MASK                    GENMASK(3, 0)
#define EQDMA_GLBL2_CHANNEL_MDMA_ADDR                      0x118
#define GLBL2_CHANNEL_MDMA_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_MDMA_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_MDMA_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_MDMA_RSVD_2_MASK                     GENMASK(15, 12)
#define GLBL2_CHANNEL_MDMA_C2H_ENG_MASK                    GENMASK(11, 8)
#define GLBL2_CHANNEL_MDMA_RSVD_3_MASK                     GENMASK(7, 4)
#define GLBL2_CHANNEL_MDMA_H2C_ENG_MASK                    GENMASK(3, 0)
#define EQDMA_GLBL2_CHANNEL_STRM_ADDR                      0x11C
#define GLBL2_CHANNEL_STRM_RSVD_1_MASK                     GENMASK(31, 18)
#define GLBL2_CHANNEL_STRM_C2H_ST_MASK                     BIT(17)
#define GLBL2_CHANNEL_STRM_H2C_ST_MASK                     BIT(16)
#define GLBL2_CHANNEL_STRM_RSVD_2_MASK                     GENMASK(15, 12)
#define GLBL2_CHANNEL_STRM_C2H_ENG_MASK                    GENMASK(11, 8)
#define GLBL2_CHANNEL_STRM_RSVD_3_MASK                     GENMASK(7, 4)
#define GLBL2_CHANNEL_STRM_H2C_ENG_MASK                    GENMASK(3, 0)
#define EQDMA_GLBL2_CHANNEL_CAP_ADDR                       0x120
#define GLBL2_CHANNEL_CAP_RSVD_1_MASK                      GENMASK(31, 12)
#define GLBL2_CHANNEL_CAP_MULTIQ_MAX_MASK                  GENMASK(11, 0)
#define EQDMA_GLBL2_CHANNEL_PASID_CAP_ADDR                 0x128
#define GLBL2_CHANNEL_PASID_CAP_RSVD_1_MASK                GENMASK(31, 2)
#define GLBL2_CHANNEL_PASID_CAP_BRIDGEEN_MASK              BIT(1)
#define GLBL2_CHANNEL_PASID_CAP_DMAEN_MASK                 BIT(0)
#define EQDMA_GLBL2_SYSTEM_ID_ADDR                         0x130
#define GLBL2_SYSTEM_ID_RSVD_1_MASK                        GENMASK(31, 16)
#define GLBL2_SYSTEM_ID_MASK                              GENMASK(15, 0)
#define EQDMA_GLBL2_MISC_CAP_ADDR                          0x134
#define GLBL2_MISC_CAP_MASK                               GENMASK(31, 0)
#define EQDMA_GLBL2_DBG_PCIE_RQ0_ADDR                      0x1B8
#define GLBL2_PCIE_RQ0_NPH_AVL_MASK                    GENMASK(31, 20)
#define GLBL2_PCIE_RQ0_RCB_AVL_MASK                    GENMASK(19, 9)
#define GLBL2_PCIE_RQ0_SLV_RD_CREDS_MASK               GENMASK(8, 2)
#define GLBL2_PCIE_RQ0_TAG_EP_MASK                     GENMASK(1, 0)
#define EQDMA_GLBL2_DBG_PCIE_RQ1_ADDR                      0x1BC
#define GLBL2_PCIE_RQ1_RSVD_1_MASK                     GENMASK(31, 21)
#define GLBL2_PCIE_RQ1_TAG_FL_MASK                     GENMASK(20, 19)
#define GLBL2_PCIE_RQ1_WTLP_HEADER_FIFO_FL_MASK        BIT(18)
#define GLBL2_PCIE_RQ1_WTLP_HEADER_FIFO_EP_MASK        BIT(17)
#define GLBL2_PCIE_RQ1_RQ_FIFO_EP_MASK                 BIT(16)
#define GLBL2_PCIE_RQ1_RQ_FIFO_FL_MASK                 BIT(15)
#define GLBL2_PCIE_RQ1_TLPSM_MASK                      GENMASK(14, 12)
#define GLBL2_PCIE_RQ1_TLPSM512_MASK                   GENMASK(11, 9)
#define GLBL2_PCIE_RQ1_RREQ_RCB_OK_MASK                BIT(8)
#define GLBL2_PCIE_RQ1_RREQ0_SLV_MASK                  BIT(7)
#define GLBL2_PCIE_RQ1_RREQ0_VLD_MASK                  BIT(6)
#define GLBL2_PCIE_RQ1_RREQ0_RDY_MASK                  BIT(5)
#define GLBL2_PCIE_RQ1_RREQ1_SLV_MASK                  BIT(4)
#define GLBL2_PCIE_RQ1_RREQ1_VLD_MASK                  BIT(3)
#define GLBL2_PCIE_RQ1_RREQ1_RDY_MASK                  BIT(2)
#define GLBL2_PCIE_RQ1_WTLP_REQ_MASK                   BIT(1)
#define GLBL2_PCIE_RQ1_WTLP_STRADDLE_MASK              BIT(0)
#define EQDMA_GLBL2_DBG_AXIMM_WR0_ADDR                     0x1C0
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
#define EQDMA_GLBL2_DBG_AXIMM_WR1_ADDR                     0x1C4
#define GLBL2_AXIMM_WR1_RSVD_1_MASK                    GENMASK(31, 30)
#define GLBL2_AXIMM_WR1_BRSP_CNT4_MASK                 GENMASK(29, 24)
#define GLBL2_AXIMM_WR1_BRSP_CNT3_MASK                 GENMASK(23, 18)
#define GLBL2_AXIMM_WR1_BRSP_CNT2_MASK                 GENMASK(17, 12)
#define GLBL2_AXIMM_WR1_BRSP_CNT1_MASK                 GENMASK(11, 6)
#define GLBL2_AXIMM_WR1_BRSP_CNT0_MASK                 GENMASK(5, 0)
#define EQDMA_GLBL2_DBG_AXIMM_RD0_ADDR                     0x1C8
#define GLBL2_AXIMM_RD0_RSVD_1_MASK                    GENMASK(31, 23)
#define GLBL2_AXIMM_RD0_PND_CNT_MASK                   GENMASK(22, 17)
#define GLBL2_AXIMM_RD0_RD_REQ_MASK                    BIT(16)
#define GLBL2_AXIMM_RD0_RD_CHNL_MASK                   GENMASK(15, 13)
#define GLBL2_AXIMM_RD0_RRSP_CLAIM_CHNL_MASK           GENMASK(12, 10)
#define GLBL2_AXIMM_RD0_RID_MASK                       GENMASK(9, 7)
#define GLBL2_AXIMM_RD0_RVALID_MASK                    BIT(6)
#define GLBL2_AXIMM_RD0_RREADY_MASK                    BIT(5)
#define GLBL2_AXIMM_RD0_ARID_MASK                      GENMASK(4, 2)
#define GLBL2_AXIMM_RD0_ARVALID_MASK                   BIT(1)
#define GLBL2_AXIMM_RD0_ARREADY_MASK                   BIT(0)
#define EQDMA_GLBL2_DBG_AXIMM_RD1_ADDR                     0x1CC
#define GLBL2_AXIMM_RD1_RSVD_1_MASK                    GENMASK(31, 30)
#define GLBL2_AXIMM_RD1_RRSP_CNT4_MASK                 GENMASK(29, 24)
#define GLBL2_AXIMM_RD1_RRSP_CNT3_MASK                 GENMASK(23, 18)
#define GLBL2_AXIMM_RD1_RRSP_CNT2_MASK                 GENMASK(17, 12)
#define GLBL2_AXIMM_RD1_RRSP_CNT1_MASK                 GENMASK(11, 6)
#define GLBL2_AXIMM_RD1_RRSP_CNT0_MASK                 GENMASK(5, 0)
#define EQDMA_GLBL2_DBG_FAB0_ADDR                          0x1D0
#define GLBL2_FAB0_H2C_INB_CONV_IN_VLD_MASK            BIT(31)
#define GLBL2_FAB0_H2C_INB_CONV_IN_RDY_MASK            BIT(30)
#define GLBL2_FAB0_H2C_SEG_IN_VLD_MASK                 BIT(29)
#define GLBL2_FAB0_H2C_SEG_IN_RDY_MASK                 BIT(28)
#define GLBL2_FAB0_H2C_SEG_OUT_VLD_MASK                GENMASK(27, 24)
#define GLBL2_FAB0_H2C_SEG_OUT_RDY_MASK                BIT(23)
#define GLBL2_FAB0_H2C_MST_CRDT_STAT_MASK              GENMASK(22, 16)
#define GLBL2_FAB0_C2H_SLV_AFIFO_FULL_MASK             BIT(15)
#define GLBL2_FAB0_C2H_SLV_AFIFO_EMPTY_MASK            BIT(14)
#define GLBL2_FAB0_C2H_DESEG_SEG_VLD_MASK              GENMASK(13, 10)
#define GLBL2_FAB0_C2H_DESEG_SEG_RDY_MASK              BIT(9)
#define GLBL2_FAB0_C2H_DESEG_OUT_VLD_MASK              BIT(8)
#define GLBL2_FAB0_C2H_DESEG_OUT_RDY_MASK              BIT(7)
#define GLBL2_FAB0_C2H_INB_DECONV_OUT_VLD_MASK         BIT(6)
#define GLBL2_FAB0_C2H_INB_DECONV_OUT_RDY_MASK         BIT(5)
#define GLBL2_FAB0_C2H_DSC_CRDT_AFIFO_FULL_MASK        BIT(4)
#define GLBL2_FAB0_C2H_DSC_CRDT_AFIFO_EMPTY_MASK       BIT(3)
#define GLBL2_FAB0_IRQ_IN_AFIFO_FULL_MASK              BIT(2)
#define GLBL2_FAB0_IRQ_IN_AFIFO_EMPTY_MASK             BIT(1)
#define GLBL2_FAB0_IMM_CRD_AFIFO_EMPTY_MASK            BIT(0)
#define EQDMA_GLBL2_DBG_FAB1_ADDR                          0x1D4
#define GLBL2_FAB1_BYP_OUT_CRDT_STAT_MASK              GENMASK(31, 25)
#define GLBL2_FAB1_TM_DSC_STS_CRDT_STAT_MASK           GENMASK(24, 18)
#define GLBL2_FAB1_C2H_CMN_AFIFO_FULL_MASK             BIT(17)
#define GLBL2_FAB1_C2H_CMN_AFIFO_EMPTY_MASK            BIT(16)
#define GLBL2_FAB1_RSVD_1_MASK                         GENMASK(15, 13)
#define GLBL2_FAB1_C2H_BYP_IN_AFIFO_FULL_MASK          BIT(12)
#define GLBL2_FAB1_RSVD_2_MASK                         GENMASK(11, 9)
#define GLBL2_FAB1_C2H_BYP_IN_AFIFO_EMPTY_MASK         BIT(8)
#define GLBL2_FAB1_RSVD_3_MASK                         GENMASK(7, 5)
#define GLBL2_FAB1_H2C_BYP_IN_AFIFO_FULL_MASK          BIT(4)
#define GLBL2_FAB1_RSVD_4_MASK                         GENMASK(3, 1)
#define GLBL2_FAB1_H2C_BYP_IN_AFIFO_EMPTY_MASK         BIT(0)
#define EQDMA_GLBL2_DBG_MATCH_SEL_ADDR                     0x1F4
#define GLBL2_MATCH_SEL_RSV_MASK                       GENMASK(31, 18)
#define GLBL2_MATCH_SEL_CSR_SEL_MASK                   GENMASK(17, 13)
#define GLBL2_MATCH_SEL_CSR_EN_MASK                    BIT(12)
#define GLBL2_MATCH_SEL_ROTATE1_MASK                   GENMASK(11, 10)
#define GLBL2_MATCH_SEL_ROTATE0_MASK                   GENMASK(9, 8)
#define GLBL2_MATCH_SEL_SEL_MASK                       GENMASK(7, 0)
#define EQDMA_GLBL2_DBG_MATCH_MSK_ADDR                     0x1F8
#define GLBL2_MATCH_MSK_MASK                      GENMASK(31, 0)
#define EQDMA_GLBL2_DBG_MATCH_PAT_ADDR                     0x1FC
#define GLBL2_MATCH_PAT_PATTERN_MASK                   GENMASK(31, 0)
#define EQDMA_GLBL_RNG_SZ_1_ADDR                           0x204
#define GLBL_RNG_SZ_1_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_1_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_2_ADDR                           0x208
#define GLBL_RNG_SZ_2_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_2_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_3_ADDR                           0x20C
#define GLBL_RNG_SZ_3_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_3_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_4_ADDR                           0x210
#define GLBL_RNG_SZ_4_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_4_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_5_ADDR                           0x214
#define GLBL_RNG_SZ_5_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_5_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_6_ADDR                           0x218
#define GLBL_RNG_SZ_6_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_6_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_7_ADDR                           0x21C
#define GLBL_RNG_SZ_7_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_7_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_8_ADDR                           0x220
#define GLBL_RNG_SZ_8_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_8_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_9_ADDR                           0x224
#define GLBL_RNG_SZ_9_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_9_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_A_ADDR                           0x228
#define GLBL_RNG_SZ_A_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_A_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_B_ADDR                           0x22C
#define GLBL_RNG_SZ_B_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_B_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_C_ADDR                           0x230
#define GLBL_RNG_SZ_C_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_C_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_D_ADDR                           0x234
#define GLBL_RNG_SZ_D_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_D_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_E_ADDR                           0x238
#define GLBL_RNG_SZ_E_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_E_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_F_ADDR                           0x23C
#define GLBL_RNG_SZ_F_RSVD_1_MASK                          GENMASK(31, 16)
#define GLBL_RNG_SZ_F_RING_SIZE_MASK                       GENMASK(15, 0)
#define EQDMA_GLBL_RNG_SZ_10_ADDR                          0x240
#define GLBL_RNG_SZ_10_RSVD_1_MASK                         GENMASK(31, 16)
#define GLBL_RNG_SZ_10_RING_SIZE_MASK                      GENMASK(15, 0)
#define EQDMA_GLBL_ERR_STAT_ADDR                           0x248
#define GLBL_ERR_STAT_RSVD_1_MASK                          GENMASK(31, 18)
#define GLBL_ERR_STAT_ERR_FAB_MASK                         BIT(17)
#define GLBL_ERR_STAT_ERR_H2C_ST_MASK                      BIT(16)
#define GLBL_ERR_STAT_ERR_BDG_MASK                         BIT(15)
#define GLBL_ERR_STAT_IND_CTXT_CMD_ERR_MASK                GENMASK(14, 9)
#define GLBL_ERR_STAT_ERR_C2H_ST_MASK                      BIT(8)
#define GLBL_ERR_STAT_ERR_C2H_MM_1_MASK                    BIT(7)
#define GLBL_ERR_STAT_ERR_C2H_MM_0_MASK                    BIT(6)
#define GLBL_ERR_STAT_ERR_H2C_MM_1_MASK                    BIT(5)
#define GLBL_ERR_STAT_ERR_H2C_MM_0_MASK                    BIT(4)
#define GLBL_ERR_STAT_ERR_TRQ_MASK                         BIT(3)
#define GLBL_ERR_STAT_ERR_DSC_MASK                         BIT(2)
#define GLBL_ERR_STAT_ERR_RAM_DBE_MASK                     BIT(1)
#define GLBL_ERR_STAT_ERR_RAM_SBE_MASK                     BIT(0)
#define EQDMA_GLBL_ERR_MASK_ADDR                           0x24C
#define GLBL_ERR_MASK                            GENMASK(31, 0)
#define EQDMA_GLBL_DSC_CFG_ADDR                            0x250
#define GLBL_DSC_CFG_RSVD_1_MASK                           GENMASK(31, 10)
#define GLBL_DSC_CFG_UNC_OVR_COR_MASK                      BIT(9)
#define GLBL_DSC_CFG_CTXT_FER_DIS_MASK                     BIT(8)
#define GLBL_DSC_CFG_RSVD_2_MASK                           GENMASK(7, 6)
#define GLBL_DSC_CFG_MAXFETCH_MASK                         GENMASK(5, 3)
#define GLBL_DSC_CFG_WB_ACC_INT_MASK                       GENMASK(2, 0)
#define EQDMA_GLBL_DSC_ERR_STS_ADDR                        0x254
#define GLBL_DSC_ERR_STS_RSVD_1_MASK                       GENMASK(31, 26)
#define GLBL_DSC_ERR_STS_PORT_ID_MASK                      BIT(25)
#define GLBL_DSC_ERR_STS_SBE_MASK                          BIT(24)
#define GLBL_DSC_ERR_STS_DBE_MASK                          BIT(23)
#define GLBL_DSC_ERR_STS_RQ_CANCEL_MASK                    BIT(22)
#define GLBL_DSC_ERR_STS_DSC_MASK                          BIT(21)
#define GLBL_DSC_ERR_STS_DMA_MASK                          BIT(20)
#define GLBL_DSC_ERR_STS_FLR_CANCEL_MASK                   BIT(19)
#define GLBL_DSC_ERR_STS_RSVD_2_MASK                       GENMASK(18, 17)
#define GLBL_DSC_ERR_STS_DAT_POISON_MASK                   BIT(16)
#define GLBL_DSC_ERR_STS_TIMEOUT_MASK                      BIT(9)
#define GLBL_DSC_ERR_STS_FLR_MASK                          BIT(8)
#define GLBL_DSC_ERR_STS_TAG_MASK                          BIT(6)
#define GLBL_DSC_ERR_STS_ADDR_MASK                         BIT(5)
#define GLBL_DSC_ERR_STS_PARAM_MASK                        BIT(4)
#define GLBL_DSC_ERR_STS_BCNT_MASK                         BIT(3)
#define GLBL_DSC_ERR_STS_UR_CA_MASK                        BIT(2)
#define GLBL_DSC_ERR_STS_POISON_MASK                       BIT(1)
#define EQDMA_GLBL_DSC_ERR_MSK_ADDR                        0x258
#define GLBL_DSC_ERR_MSK_MASK                         GENMASK(31, 0)
#define EQDMA_GLBL_DSC_ERR_LOG0_ADDR                       0x25C
#define GLBL_DSC_ERR_LOG0_VALID_MASK                       BIT(31)
#define GLBL_DSC_ERR_LOG0_SEL_MASK                         BIT(30)
#define GLBL_DSC_ERR_LOG0_RSVD_1_MASK                      GENMASK(29, 13)
#define GLBL_DSC_ERR_LOG0_QID_MASK                         GENMASK(12, 0)
#define EQDMA_GLBL_DSC_ERR_LOG1_ADDR                       0x260
#define GLBL_DSC_ERR_LOG1_RSVD_1_MASK                      GENMASK(31, 28)
#define GLBL_DSC_ERR_LOG1_CIDX_MASK                        GENMASK(27, 12)
#define GLBL_DSC_ERR_LOG1_RSVD_2_MASK                      GENMASK(11, 9)
#define GLBL_DSC_ERR_LOG1_SUB_TYPE_MASK                    GENMASK(8, 5)
#define GLBL_DSC_ERR_LOG1_ERR_TYPE_MASK                    GENMASK(4, 0)
#define EQDMA_GLBL_TRQ_ERR_STS_ADDR                        0x264
#define GLBL_TRQ_ERR_STS_RSVD_1_MASK                       GENMASK(31, 8)
#define GLBL_TRQ_ERR_STS_TCP_QSPC_TIMEOUT_MASK             BIT(7)
#define GLBL_TRQ_ERR_STS_RSVD_2_MASK                       BIT(6)
#define GLBL_TRQ_ERR_STS_QID_RANGE_MASK                    BIT(5)
#define GLBL_TRQ_ERR_STS_QSPC_UNMAPPED_MASK                BIT(4)
#define GLBL_TRQ_ERR_STS_TCP_CSR_TIMEOUT_MASK              BIT(3)
#define GLBL_TRQ_ERR_STS_RSVD_3_MASK                       BIT(2)
#define GLBL_TRQ_ERR_STS_VF_ACCESS_ERR_MASK                BIT(1)
#define GLBL_TRQ_ERR_STS_CSR_UNMAPPED_MASK                 BIT(0)
#define EQDMA_GLBL_TRQ_ERR_MSK_ADDR                        0x268
#define GLBL_TRQ_ERR_MSK_MASK                         GENMASK(31, 0)
#define EQDMA_GLBL_TRQ_ERR_LOG_ADDR                        0x26C
#define GLBL_TRQ_ERR_LOG_SRC_MASK                          BIT(31)
#define GLBL_TRQ_ERR_LOG_TARGET_MASK                       GENMASK(30, 27)
#define GLBL_TRQ_ERR_LOG_FUNC_MASK                         GENMASK(26, 17)
#define GLBL_TRQ_ERR_LOG_ADDRESS_MASK                      GENMASK(16, 0)
#define EQDMA_GLBL_DSC_DBG_DAT0_ADDR                       0x270
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
#define EQDMA_GLBL_DSC_DBG_DAT1_ADDR                       0x274
#define GLBL_DSC_DAT1_RSVD_1_MASK                      GENMASK(31, 28)
#define GLBL_DSC_DAT1_EVT_SPC_C2H_MASK                 GENMASK(27, 22)
#define GLBL_DSC_DAT1_EVT_SP_H2C_MASK                  GENMASK(21, 16)
#define GLBL_DSC_DAT1_DSC_SPC_C2H_MASK                 GENMASK(15, 8)
#define GLBL_DSC_DAT1_DSC_SPC_H2C_MASK                 GENMASK(7, 0)
#define EQDMA_GLBL_DSC_DBG_CTL_ADDR                        0x278
#define GLBL_DSC_CTL_RSVD_1_MASK                       GENMASK(31, 3)
#define GLBL_DSC_CTL_SELECT_MASK                       GENMASK(2, 0)
#define EQDMA_GLBL_DSC_ERR_LOG2_ADDR                       0x27c
#define GLBL_DSC_ERR_LOG2_OLD_PIDX_MASK                    GENMASK(31, 16)
#define GLBL_DSC_ERR_LOG2_NEW_PIDX_MASK                    GENMASK(15, 0)
#define EQDMA_GLBL_GLBL_INTERRUPT_CFG_ADDR                 0x2c4
#define GLBL_GLBL_INTERRUPT_CFG_RSVD_1_MASK                GENMASK(31, 2)
#define GLBL_GLBL_INTERRUPT_CFG_LGCY_INTR_PENDING_MASK     BIT(1)
#define GLBL_GLBL_INTERRUPT_CFG_EN_LGCY_INTR_MASK          BIT(0)
#define EQDMA_GLBL_VCH_HOST_PROFILE_ADDR                   0x2c8
#define GLBL_VCH_HOST_PROFILE_RSVD_1_MASK                  GENMASK(31, 28)
#define GLBL_VCH_HOST_PROFILE_2C_MM_MASK                   GENMASK(27, 24)
#define GLBL_VCH_HOST_PROFILE_2C_ST_MASK                   GENMASK(23, 20)
#define GLBL_VCH_HOST_PROFILE_VCH_DSC_MASK                 GENMASK(19, 16)
#define GLBL_VCH_HOST_PROFILE_VCH_INT_MSG_MASK             GENMASK(15, 12)
#define GLBL_VCH_HOST_PROFILE_VCH_INT_AGGR_MASK            GENMASK(11, 8)
#define GLBL_VCH_HOST_PROFILE_VCH_CMPT_MASK                GENMASK(7, 4)
#define GLBL_VCH_HOST_PROFILE_VCH_C2H_PLD_MASK             GENMASK(3, 0)
#define EQDMA_GLBL_BRIDGE_HOST_PROFILE_ADDR                0x308
#define GLBL_BRIDGE_HOST_PROFILE_RSVD_1_MASK               GENMASK(31, 4)
#define GLBL_BRIDGE_HOST_PROFILE_BDGID_MASK                GENMASK(3, 0)
#define EQDMA_AXIMM_IRQ_DEST_ADDR_ADDR                     0x30c
#define AXIMM_IRQ_DEST_ADDR_ADDR_MASK                      GENMASK(31, 0)
#define EQDMA_FAB_ERR_LOG_ADDR                             0x314
#define FAB_ERR_LOG_RSVD_1_MASK                            GENMASK(31, 7)
#define FAB_ERR_LOG_SRC_MASK                               GENMASK(6, 0)
#define EQDMA_GLBL_REQ_ERR_STS_ADDR                        0x318
#define GLBL_REQ_ERR_STS_RSVD_1_MASK                       GENMASK(31, 11)
#define GLBL_REQ_ERR_STS_RC_DISCONTINUE_MASK               BIT(10)
#define GLBL_REQ_ERR_STS_RC_PRTY_MASK                      BIT(9)
#define GLBL_REQ_ERR_STS_RC_FLR_MASK                       BIT(8)
#define GLBL_REQ_ERR_STS_RC_TIMEOUT_MASK                   BIT(7)
#define GLBL_REQ_ERR_STS_RC_INV_BCNT_MASK                  BIT(6)
#define GLBL_REQ_ERR_STS_RC_INV_TAG_MASK                   BIT(5)
#define GLBL_REQ_ERR_STS_RC_START_ADDR_MISMCH_MASK         BIT(4)
#define GLBL_REQ_ERR_STS_RC_RID_TC_ATTR_MISMCH_MASK        BIT(3)
#define GLBL_REQ_ERR_STS_RC_NO_DATA_MASK                   BIT(2)
#define GLBL_REQ_ERR_STS_RC_UR_CA_CRS_MASK                 BIT(1)
#define GLBL_REQ_ERR_STS_RC_POISONED_MASK                  BIT(0)
#define EQDMA_GLBL_REQ_ERR_MSK_ADDR                        0x31C
#define GLBL_REQ_ERR_MSK_MASK                         GENMASK(31, 0)
#define EQDMA_IND_CTXT_DATA_ADDR                           0x804
#define IND_CTXT_DATA_DATA_MASK                            GENMASK(31, 0)
#define EQDMA_IND_CTXT_MASK_ADDR                           0x824
#define IND_CTXT_MASK                            GENMASK(31, 0)
#define EQDMA_IND_CTXT_CMD_ADDR                            0x844
#define IND_CTXT_CMD_RSVD_1_MASK                           GENMASK(31, 20)
#define IND_CTXT_CMD_QID_MASK                              GENMASK(19, 7)
#define IND_CTXT_CMD_OP_MASK                               GENMASK(6, 5)
#define IND_CTXT_CMD_SEL_MASK                              GENMASK(4, 1)
#define IND_CTXT_CMD_BUSY_MASK                             BIT(0)
#define EQDMA_C2H_TIMER_CNT_ADDR                           0xA00
#define C2H_TIMER_CNT_RSVD_1_MASK                          GENMASK(31, 16)
#define C2H_TIMER_CNT_MASK                                GENMASK(15, 0)
#define EQDMA_C2H_CNT_TH_ADDR                              0xA40
#define C2H_CNT_TH_RSVD_1_MASK                             GENMASK(31, 16)
#define C2H_CNT_TH_THESHOLD_CNT_MASK                       GENMASK(15, 0)
#define EQDMA_C2H_STAT_S_AXIS_C2H_ACCEPTED_ADDR            0xA88
#define C2H_STAT_S_AXIS_C2H_ACCEPTED_MASK                 GENMASK(31, 0)
#define EQDMA_C2H_STAT_S_AXIS_WRB_ACCEPTED_ADDR            0xA8C
#define C2H_STAT_S_AXIS_WRB_ACCEPTED_MASK                 GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_RSP_PKT_ACCEPTED_ADDR          0xA90
#define C2H_STAT_DESC_RSP_PKT_ACCEPTED_D_MASK              GENMASK(31, 0)
#define EQDMA_C2H_STAT_AXIS_PKG_CMP_ADDR                   0xA94
#define C2H_STAT_AXIS_PKG_CMP_MASK                        GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_RSP_ACCEPTED_ADDR              0xA98
#define C2H_STAT_DESC_RSP_ACCEPTED_D_MASK                  GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_RSP_CMP_ADDR                   0xA9C
#define C2H_STAT_DESC_RSP_CMP_D_MASK                       GENMASK(31, 0)
#define EQDMA_C2H_STAT_WRQ_OUT_ADDR                        0xAA0
#define C2H_STAT_WRQ_OUT_MASK                             GENMASK(31, 0)
#define EQDMA_C2H_STAT_WPL_REN_ACCEPTED_ADDR               0xAA4
#define C2H_STAT_WPL_REN_ACCEPTED_MASK                    GENMASK(31, 0)
#define EQDMA_C2H_STAT_TOTAL_WRQ_LEN_ADDR                  0xAA8
#define C2H_STAT_TOTAL_WRQ_LEN_MASK                       GENMASK(31, 0)
#define EQDMA_C2H_STAT_TOTAL_WPL_LEN_ADDR                  0xAAC
#define C2H_STAT_TOTAL_WPL_LEN_MASK                       GENMASK(31, 0)
#define EQDMA_C2H_BUF_SZ_ADDR                              0xAB0
#define C2H_BUF_SZ_IZE_MASK                                GENMASK(31, 0)
#define EQDMA_C2H_ERR_STAT_ADDR                            0xAF0
#define C2H_ERR_STAT_RSVD_1_MASK                           GENMASK(31, 21)
#define C2H_ERR_STAT_WRB_PORT_ID_ERR_MASK                  BIT(20)
#define C2H_ERR_STAT_HDR_PAR_ERR_MASK                      BIT(19)
#define C2H_ERR_STAT_HDR_ECC_COR_ERR_MASK                  BIT(18)
#define C2H_ERR_STAT_HDR_ECC_UNC_ERR_MASK                  BIT(17)
#define C2H_ERR_STAT_AVL_RING_DSC_ERR_MASK                 BIT(16)
#define C2H_ERR_STAT_WRB_PRTY_ERR_MASK                     BIT(15)
#define C2H_ERR_STAT_WRB_CIDX_ERR_MASK                     BIT(14)
#define C2H_ERR_STAT_WRB_QFULL_ERR_MASK                    BIT(13)
#define C2H_ERR_STAT_WRB_INV_Q_ERR_MASK                    BIT(12)
#define C2H_ERR_STAT_RSVD_2_MASK                           BIT(11)
#define C2H_ERR_STAT_PORT_ID_CTXT_MISMATCH_MASK            BIT(10)
#define C2H_ERR_STAT_ERR_DESC_CNT_MASK                     BIT(9)
#define C2H_ERR_STAT_RSVD_3_MASK                           BIT(8)
#define C2H_ERR_STAT_MSI_INT_FAIL_MASK                     BIT(7)
#define C2H_ERR_STAT_ENG_WPL_DATA_PAR_ERR_MASK             BIT(6)
#define C2H_ERR_STAT_RSVD_4_MASK                           BIT(5)
#define C2H_ERR_STAT_DESC_RSP_ERR_MASK                     BIT(4)
#define C2H_ERR_STAT_QID_MISMATCH_MASK                     BIT(3)
#define C2H_ERR_STAT_SH_CMPT_DSC_ERR_MASK                  BIT(2)
#define C2H_ERR_STAT_LEN_MISMATCH_MASK                     BIT(1)
#define C2H_ERR_STAT_MTY_MISMATCH_MASK                     BIT(0)
#define EQDMA_C2H_ERR_MASK_ADDR                            0xAF4
#define C2H_ERR_EN_MASK                          GENMASK(31, 0)
#define EQDMA_C2H_FATAL_ERR_STAT_ADDR                      0xAF8
#define C2H_FATAL_ERR_STAT_RSVD_1_MASK                     GENMASK(31, 21)
#define C2H_FATAL_ERR_STAT_HDR_ECC_UNC_ERR_MASK            BIT(20)
#define C2H_FATAL_ERR_STAT_AVL_RING_FIFO_RAM_RDBE_MASK     BIT(19)
#define C2H_FATAL_ERR_STAT_WPL_DATA_PAR_ERR_MASK           BIT(18)
#define C2H_FATAL_ERR_STAT_PLD_FIFO_RAM_RDBE_MASK          BIT(17)
#define C2H_FATAL_ERR_STAT_QID_FIFO_RAM_RDBE_MASK          BIT(16)
#define C2H_FATAL_ERR_STAT_CMPT_FIFO_RAM_RDBE_MASK         BIT(15)
#define C2H_FATAL_ERR_STAT_WRB_COAL_DATA_RAM_RDBE_MASK     BIT(14)
#define C2H_FATAL_ERR_STAT_RESERVED2_MASK                  BIT(13)
#define C2H_FATAL_ERR_STAT_INT_CTXT_RAM_RDBE_MASK          BIT(12)
#define C2H_FATAL_ERR_STAT_DESC_REQ_FIFO_RAM_RDBE_MASK     BIT(11)
#define C2H_FATAL_ERR_STAT_PFCH_CTXT_RAM_RDBE_MASK         BIT(10)
#define C2H_FATAL_ERR_STAT_WRB_CTXT_RAM_RDBE_MASK          BIT(9)
#define C2H_FATAL_ERR_STAT_PFCH_LL_RAM_RDBE_MASK           BIT(8)
#define C2H_FATAL_ERR_STAT_TIMER_FIFO_RAM_RDBE_MASK        GENMASK(7, 4)
#define C2H_FATAL_ERR_STAT_QID_MISMATCH_MASK               BIT(3)
#define C2H_FATAL_ERR_STAT_RESERVED1_MASK                  BIT(2)
#define C2H_FATAL_ERR_STAT_LEN_MISMATCH_MASK               BIT(1)
#define C2H_FATAL_ERR_STAT_MTY_MISMATCH_MASK               BIT(0)
#define EQDMA_C2H_FATAL_ERR_MASK_ADDR                      0xAFC
#define C2H_FATAL_ERR_C2HEN_MASK                 GENMASK(31, 0)
#define EQDMA_C2H_FATAL_ERR_ENABLE_ADDR                    0xB00
#define C2H_FATAL_ERR_ENABLE_RSVD_1_MASK                   GENMASK(31, 2)
#define C2H_FATAL_ERR_ENABLE_WPL_PAR_INV_MASK             BIT(1)
#define C2H_FATAL_ERR_ENABLE_WRQ_DIS_MASK                 BIT(0)
#define EQDMA_GLBL_ERR_INT_ADDR                            0xB04
#define GLBL_ERR_INT_RSVD_1_MASK                           GENMASK(31, 30)
#define GLBL_ERR_INT_HOST_ID_MASK                          GENMASK(29, 26)
#define GLBL_ERR_INT_DIS_INTR_ON_VF_MASK                   BIT(25)
#define GLBL_ERR_INT_ARM_MASK                             BIT(24)
#define GLBL_ERR_INT_EN_COAL_MASK                          BIT(23)
#define GLBL_ERR_INT_VEC_MASK                              GENMASK(22, 12)
#define GLBL_ERR_INT_FUNC_MASK                             GENMASK(11, 0)
#define EQDMA_C2H_PFCH_CFG_ADDR                            0xB08
#define C2H_PFCH_CFG_EVTFL_TH_MASK                         GENMASK(31, 16)
#define C2H_PFCH_CFG_FL_TH_MASK                            GENMASK(15, 0)
#define EQDMA_C2H_PFCH_CFG_1_ADDR                          0xA80
#define C2H_PFCH_CFG_1_EVT_QCNT_TH_MASK                    GENMASK(31, 16)
#define C2H_PFCH_CFG_1_QCNT_MASK                           GENMASK(15, 0)
#define EQDMA_C2H_PFCH_CFG_2_ADDR                          0xA84
#define C2H_PFCH_CFG_2_FENCE_MASK                          BIT(31)
#define C2H_PFCH_CFG_2_RSVD_MASK                           GENMASK(30, 29)
#define C2H_PFCH_CFG_2_VAR_DESC_NO_DROP_MASK               BIT(28)
#define C2H_PFCH_CFG_2_LL_SZ_TH_MASK                       GENMASK(27, 12)
#define C2H_PFCH_CFG_2_VAR_DESC_NUM_MASK                   GENMASK(11, 6)
#define C2H_PFCH_CFG_2_NUM_MASK                            GENMASK(5, 0)
#define EQDMA_C2H_INT_TIMER_TICK_ADDR                      0xB0C
#define C2H_INT_TIMER_TICK_MASK                           GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_RSP_DROP_ACCEPTED_ADDR         0xB10
#define C2H_STAT_DESC_RSP_DROP_ACCEPTED_D_MASK             GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_RSP_ERR_ACCEPTED_ADDR          0xB14
#define C2H_STAT_DESC_RSP_ERR_ACCEPTED_D_MASK              GENMASK(31, 0)
#define EQDMA_C2H_STAT_DESC_REQ_ADDR                       0xB18
#define C2H_STAT_DESC_REQ_MASK                            GENMASK(31, 0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_0_ADDR                  0xB1C
#define C2H_STAT_DMA_ENG_0_S_AXIS_C2H_TVALID_MASK      BIT(31)
#define C2H_STAT_DMA_ENG_0_S_AXIS_C2H_TREADY_MASK      BIT(30)
#define C2H_STAT_DMA_ENG_0_S_AXIS_WRB_TVALID_MASK      GENMASK(29, 27)
#define C2H_STAT_DMA_ENG_0_S_AXIS_WRB_TREADY_MASK      GENMASK(26, 24)
#define C2H_STAT_DMA_ENG_0_PLD_FIFO_IN_RDY_MASK        BIT(23)
#define C2H_STAT_DMA_ENG_0_QID_FIFO_IN_RDY_MASK        BIT(22)
#define C2H_STAT_DMA_ENG_0_ARB_FIFO_OUT_VLD_MASK       BIT(21)
#define C2H_STAT_DMA_ENG_0_ARB_FIFO_OUT_QID_MASK       GENMASK(20, 9)
#define C2H_STAT_DMA_ENG_0_WRB_FIFO_IN_RDY_MASK        BIT(8)
#define C2H_STAT_DMA_ENG_0_WRB_FIFO_OUT_CNT_MASK       GENMASK(7, 5)
#define C2H_STAT_DMA_ENG_0_WRB_SM_CS_MASK              BIT(4)
#define C2H_STAT_DMA_ENG_0_MAIN_SM_CS_MASK             GENMASK(3, 0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_1_ADDR                  0xB20
#define C2H_STAT_DMA_ENG_1_RSVD_1_MASK                 GENMASK(31, 29)
#define C2H_STAT_DMA_ENG_1_QID_FIFO_OUT_CNT_MASK       GENMASK(28, 18)
#define C2H_STAT_DMA_ENG_1_PLD_FIFO_OUT_CNT_MASK       GENMASK(17, 7)
#define C2H_STAT_DMA_ENG_1_PLD_ST_FIFO_CNT_MASK        GENMASK(6, 0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_2_ADDR                  0xB24
#define C2H_STAT_DMA_ENG_2_RSVD_1_MASK                 GENMASK(31, 29)
#define C2H_STAT_DMA_ENG_2_QID_FIFO_OUT_CNT_MASK       GENMASK(28, 18)
#define C2H_STAT_DMA_ENG_2_PLD_FIFO_OUT_CNT_MASK       GENMASK(17, 7)
#define C2H_STAT_DMA_ENG_2_PLD_ST_FIFO_CNT_MASK        GENMASK(6, 0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_3_ADDR                  0xB28
#define C2H_STAT_DMA_ENG_3_RSVD_1_MASK                 GENMASK(31, 24)
#define C2H_STAT_DMA_ENG_3_WRQ_FIFO_OUT_CNT_MASK       GENMASK(23, 19)
#define C2H_STAT_DMA_ENG_3_QID_FIFO_OUT_VLD_MASK       BIT(18)
#define C2H_STAT_DMA_ENG_3_PLD_FIFO_OUT_VLD_MASK       BIT(17)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_OUT_VLD_MASK    BIT(16)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_OUT_DATA_EOP_MASK BIT(15)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_OUT_DATA_AVL_IDX_ENABLE_MASK BIT(14)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_OUT_DATA_DROP_MASK BIT(13)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_OUT_DATA_ERR_MASK BIT(12)
#define C2H_STAT_DMA_ENG_3_DESC_CNT_FIFO_IN_RDY_MASK   BIT(11)
#define C2H_STAT_DMA_ENG_3_DESC_RSP_FIFO_IN_RDY_MASK   BIT(10)
#define C2H_STAT_DMA_ENG_3_PLD_PKT_ID_LARGER_0_MASK    BIT(9)
#define C2H_STAT_DMA_ENG_3_WRQ_VLD_MASK                BIT(8)
#define C2H_STAT_DMA_ENG_3_WRQ_RDY_MASK                BIT(7)
#define C2H_STAT_DMA_ENG_3_WRQ_FIFO_OUT_RDY_MASK       BIT(6)
#define C2H_STAT_DMA_ENG_3_WRQ_PACKET_OUT_DATA_DROP_MASK BIT(5)
#define C2H_STAT_DMA_ENG_3_WRQ_PACKET_OUT_DATA_ERR_MASK BIT(4)
#define C2H_STAT_DMA_ENG_3_WRQ_PACKET_OUT_DATA_MARKER_MASK BIT(3)
#define C2H_STAT_DMA_ENG_3_WRQ_PACKET_PRE_EOR_MASK     BIT(2)
#define C2H_STAT_DMA_ENG_3_WCP_FIFO_IN_RDY_MASK        BIT(1)
#define C2H_STAT_DMA_ENG_3_PLD_ST_FIFO_IN_RDY_MASK     BIT(0)
#define EQDMA_C2H_DBG_PFCH_ERR_CTXT_ADDR                   0xB2C
#define C2H_PFCH_ERR_CTXT_RSVD_1_MASK                  GENMASK(31, 14)
#define C2H_PFCH_ERR_CTXT_ERR_STAT_MASK                BIT(13)
#define C2H_PFCH_ERR_CTXT_CMD_WR_MASK                  BIT(12)
#define C2H_PFCH_ERR_CTXT_QID_MASK                     GENMASK(11, 1)
#define C2H_PFCH_ERR_CTXT_DONE_MASK                    BIT(0)
#define EQDMA_C2H_FIRST_ERR_QID_ADDR                       0xB30
#define C2H_FIRST_ERR_QID_RSVD_1_MASK                      GENMASK(31, 21)
#define C2H_FIRST_ERR_QID_ERR_TYPE_MASK                    GENMASK(20, 16)
#define C2H_FIRST_ERR_QID_RSVD_MASK                        GENMASK(15, 13)
#define C2H_FIRST_ERR_QID_QID_MASK                         GENMASK(12, 0)
#define EQDMA_STAT_NUM_WRB_IN_ADDR                         0xB34
#define STAT_NUM_WRB_IN_RSVD_1_MASK                        GENMASK(31, 16)
#define STAT_NUM_WRB_IN_WRB_CNT_MASK                       GENMASK(15, 0)
#define EQDMA_STAT_NUM_WRB_OUT_ADDR                        0xB38
#define STAT_NUM_WRB_OUT_RSVD_1_MASK                       GENMASK(31, 16)
#define STAT_NUM_WRB_OUT_WRB_CNT_MASK                      GENMASK(15, 0)
#define EQDMA_STAT_NUM_WRB_DRP_ADDR                        0xB3C
#define STAT_NUM_WRB_DRP_RSVD_1_MASK                       GENMASK(31, 16)
#define STAT_NUM_WRB_DRP_WRB_CNT_MASK                      GENMASK(15, 0)
#define EQDMA_STAT_NUM_STAT_DESC_OUT_ADDR                  0xB40
#define STAT_NUM_STAT_DESC_OUT_RSVD_1_MASK                 GENMASK(31, 16)
#define STAT_NUM_STAT_DESC_OUT_CNT_MASK                    GENMASK(15, 0)
#define EQDMA_STAT_NUM_DSC_CRDT_SENT_ADDR                  0xB44
#define STAT_NUM_DSC_CRDT_SENT_RSVD_1_MASK                 GENMASK(31, 16)
#define STAT_NUM_DSC_CRDT_SENT_CNT_MASK                    GENMASK(15, 0)
#define EQDMA_STAT_NUM_FCH_DSC_RCVD_ADDR                   0xB48
#define STAT_NUM_FCH_DSC_RCVD_RSVD_1_MASK                  GENMASK(31, 16)
#define STAT_NUM_FCH_DSC_RCVD_DSC_CNT_MASK                 GENMASK(15, 0)
#define EQDMA_STAT_NUM_BYP_DSC_RCVD_ADDR                   0xB4C
#define STAT_NUM_BYP_DSC_RCVD_RSVD_1_MASK                  GENMASK(31, 11)
#define STAT_NUM_BYP_DSC_RCVD_DSC_CNT_MASK                 GENMASK(10, 0)
#define EQDMA_C2H_WRB_COAL_CFG_ADDR                        0xB50
#define C2H_WRB_COAL_CFG_MAX_BUF_SZ_MASK                   GENMASK(31, 26)
#define C2H_WRB_COAL_CFG_TICK_VAL_MASK                     GENMASK(25, 14)
#define C2H_WRB_COAL_CFG_TICK_CNT_MASK                     GENMASK(13, 2)
#define C2H_WRB_COAL_CFG_SET_GLB_FLUSH_MASK                BIT(1)
#define C2H_WRB_COAL_CFG_DONE_GLB_FLUSH_MASK               BIT(0)
#define EQDMA_C2H_INTR_H2C_REQ_ADDR                        0xB54
#define C2H_INTR_H2C_REQ_RSVD_1_MASK                       GENMASK(31, 18)
#define C2H_INTR_H2C_REQ_CNT_MASK                          GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_MM_REQ_ADDR                     0xB58
#define C2H_INTR_C2H_MM_REQ_RSVD_1_MASK                    GENMASK(31, 18)
#define C2H_INTR_C2H_MM_REQ_CNT_MASK                       GENMASK(17, 0)
#define EQDMA_C2H_INTR_ERR_INT_REQ_ADDR                    0xB5C
#define C2H_INTR_ERR_INT_REQ_RSVD_1_MASK                   GENMASK(31, 18)
#define C2H_INTR_ERR_INT_REQ_CNT_MASK                      GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_ST_REQ_ADDR                     0xB60
#define C2H_INTR_C2H_ST_REQ_RSVD_1_MASK                    GENMASK(31, 18)
#define C2H_INTR_C2H_ST_REQ_CNT_MASK                       GENMASK(17, 0)
#define EQDMA_C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_ADDR        0xB64
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_RSVD_1_MASK       GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK_CNT_MASK          GENMASK(17, 0)
#define EQDMA_C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_ADDR       0xB68
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_RSVD_1_MASK      GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL_CNT_MASK         GENMASK(17, 0)
#define EQDMA_C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_ADDR    0xB6C
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_RSVD_1_MASK   GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX_CNT_MASK      GENMASK(17, 0)
#define EQDMA_C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_ADDR      0xB70
#define C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_RSVD_1_MASK     GENMASK(31, 18)
#define C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL_CNT_MASK        GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_ST_MSIX_ACK_ADDR                0xB74
#define C2H_INTR_C2H_ST_MSIX_ACK_RSVD_1_MASK               GENMASK(31, 18)
#define C2H_INTR_C2H_ST_MSIX_ACK_CNT_MASK                  GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_ST_MSIX_FAIL_ADDR               0xB78
#define C2H_INTR_C2H_ST_MSIX_FAIL_RSVD_1_MASK              GENMASK(31, 18)
#define C2H_INTR_C2H_ST_MSIX_FAIL_CNT_MASK                 GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_ST_NO_MSIX_ADDR                 0xB7C
#define C2H_INTR_C2H_ST_NO_MSIX_RSVD_1_MASK                GENMASK(31, 18)
#define C2H_INTR_C2H_ST_NO_MSIX_CNT_MASK                   GENMASK(17, 0)
#define EQDMA_C2H_INTR_C2H_ST_CTXT_INVAL_ADDR              0xB80
#define C2H_INTR_C2H_ST_CTXT_INVAL_RSVD_1_MASK             GENMASK(31, 18)
#define C2H_INTR_C2H_ST_CTXT_INVAL_CNT_MASK                GENMASK(17, 0)
#define EQDMA_C2H_STAT_WR_CMP_ADDR                         0xB84
#define C2H_STAT_WR_CMP_RSVD_1_MASK                        GENMASK(31, 18)
#define C2H_STAT_WR_CMP_CNT_MASK                           GENMASK(17, 0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_4_ADDR                  0xB88
#define C2H_STAT_DMA_ENG_4_RSVD_1_MASK                 GENMASK(31, 24)
#define C2H_STAT_DMA_ENG_4_WRQ_FIFO_OUT_CNT_MASK       GENMASK(23, 19)
#define C2H_STAT_DMA_ENG_4_QID_FIFO_OUT_VLD_MASK       BIT(18)
#define C2H_STAT_DMA_ENG_4_PLD_FIFO_OUT_VLD_MASK       BIT(17)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_OUT_VLD_MASK    BIT(16)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_OUT_DATA_EOP_MASK BIT(15)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_OUT_DATA_AVL_IDX_ENABLE_MASK BIT(14)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_OUT_DATA_DROP_MASK BIT(13)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_OUT_DATA_ERR_MASK BIT(12)
#define C2H_STAT_DMA_ENG_4_DESC_CNT_FIFO_IN_RDY_MASK   BIT(11)
#define C2H_STAT_DMA_ENG_4_DESC_RSP_FIFO_IN_RDY_MASK   BIT(10)
#define C2H_STAT_DMA_ENG_4_PLD_PKT_ID_LARGER_0_MASK    BIT(9)
#define C2H_STAT_DMA_ENG_4_WRQ_VLD_MASK                BIT(8)
#define C2H_STAT_DMA_ENG_4_WRQ_RDY_MASK                BIT(7)
#define C2H_STAT_DMA_ENG_4_WRQ_FIFO_OUT_RDY_MASK       BIT(6)
#define C2H_STAT_DMA_ENG_4_WRQ_PACKET_OUT_DATA_DROP_MASK BIT(5)
#define C2H_STAT_DMA_ENG_4_WRQ_PACKET_OUT_DATA_ERR_MASK BIT(4)
#define C2H_STAT_DMA_ENG_4_WRQ_PACKET_OUT_DATA_MARKER_MASK BIT(3)
#define C2H_STAT_DMA_ENG_4_WRQ_PACKET_PRE_EOR_MASK     BIT(2)
#define C2H_STAT_DMA_ENG_4_WCP_FIFO_IN_RDY_MASK        BIT(1)
#define C2H_STAT_DMA_ENG_4_PLD_ST_FIFO_IN_RDY_MASK     BIT(0)
#define EQDMA_C2H_STAT_DBG_DMA_ENG_5_ADDR                  0xB8C
#define C2H_STAT_DMA_ENG_5_RSVD_1_MASK                 GENMASK(31, 30)
#define C2H_STAT_DMA_ENG_5_WRB_SM_VIRT_CH_MASK         BIT(29)
#define C2H_STAT_DMA_ENG_5_WRB_FIFO_IN_REQ_MASK        GENMASK(28, 24)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_CNT_MASK       GENMASK(23, 22)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_LEN_MASK  GENMASK(21, 6)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_VIRT_CH_MASK BIT(5)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_VAR_DESC_MASK BIT(4)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_DROP_REQ_MASK BIT(3)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_NUM_BUF_OV_MASK BIT(2)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_MARKER_MASK BIT(1)
#define C2H_STAT_DMA_ENG_5_ARB_FIFO_OUT_DATA_HAS_CMPT_MASK BIT(0)
#define EQDMA_C2H_DBG_PFCH_QID_ADDR                        0xB90
#define C2H_PFCH_QID_RSVD_1_MASK                       GENMASK(31, 16)
#define C2H_PFCH_QID_ERR_CTXT_MASK                     BIT(15)
#define C2H_PFCH_QID_TARGET_MASK                       GENMASK(14, 12)
#define C2H_PFCH_QID_QID_OR_TAG_MASK                   GENMASK(11, 0)
#define EQDMA_C2H_DBG_PFCH_ADDR                            0xB94
#define C2H_PFCH_DATA_MASK                             GENMASK(31, 0)
#define EQDMA_C2H_INT_DBG_ADDR                             0xB98
#define C2H_INT_RSVD_1_MASK                            GENMASK(31, 8)
#define C2H_INT_INT_COAL_SM_MASK                       GENMASK(7, 4)
#define C2H_INT_INT_SM_MASK                            GENMASK(3, 0)
#define EQDMA_C2H_STAT_IMM_ACCEPTED_ADDR                   0xB9C
#define C2H_STAT_IMM_ACCEPTED_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_STAT_IMM_ACCEPTED_CNT_MASK                     GENMASK(17, 0)
#define EQDMA_C2H_STAT_MARKER_ACCEPTED_ADDR                0xBA0
#define C2H_STAT_MARKER_ACCEPTED_RSVD_1_MASK               GENMASK(31, 18)
#define C2H_STAT_MARKER_ACCEPTED_CNT_MASK                  GENMASK(17, 0)
#define EQDMA_C2H_STAT_DISABLE_CMP_ACCEPTED_ADDR           0xBA4
#define C2H_STAT_DISABLE_CMP_ACCEPTED_RSVD_1_MASK          GENMASK(31, 18)
#define C2H_STAT_DISABLE_CMP_ACCEPTED_CNT_MASK             GENMASK(17, 0)
#define EQDMA_C2H_PLD_FIFO_CRDT_CNT_ADDR                   0xBA8
#define C2H_PLD_FIFO_CRDT_CNT_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_PLD_FIFO_CRDT_CNT_CNT_MASK                     GENMASK(17, 0)
#define EQDMA_C2H_INTR_DYN_REQ_ADDR                        0xBAC
#define C2H_INTR_DYN_REQ_RSVD_1_MASK                       GENMASK(31, 18)
#define C2H_INTR_DYN_REQ_CNT_MASK                          GENMASK(17, 0)
#define EQDMA_C2H_INTR_DYN_MISC_ADDR                       0xBB0
#define C2H_INTR_DYN_MISC_RSVD_1_MASK                      GENMASK(31, 18)
#define C2H_INTR_DYN_MISC_CNT_MASK                         GENMASK(17, 0)
#define EQDMA_C2H_DROP_LEN_MISMATCH_ADDR                   0xBB4
#define C2H_DROP_LEN_MISMATCH_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_DROP_LEN_MISMATCH_CNT_MASK                     GENMASK(17, 0)
#define EQDMA_C2H_DROP_DESC_RSP_LEN_ADDR                   0xBB8
#define C2H_DROP_DESC_RSP_LEN_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_DROP_DESC_RSP_LEN_CNT_MASK                     GENMASK(17, 0)
#define EQDMA_C2H_DROP_QID_FIFO_LEN_ADDR                   0xBBC
#define C2H_DROP_QID_FIFO_LEN_RSVD_1_MASK                  GENMASK(31, 18)
#define C2H_DROP_QID_FIFO_LEN_CNT_MASK                     GENMASK(17, 0)
#define EQDMA_C2H_DROP_PLD_CNT_ADDR                        0xBC0
#define C2H_DROP_PLD_CNT_RSVD_1_MASK                       GENMASK(31, 18)
#define C2H_DROP_PLD_CNT_CNT_MASK                          GENMASK(17, 0)
#define EQDMA_C2H_CMPT_FORMAT_0_ADDR                       0xBC4
#define C2H_CMPT_FORMAT_0_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_0_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_1_ADDR                       0xBC8
#define C2H_CMPT_FORMAT_1_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_1_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_2_ADDR                       0xBCC
#define C2H_CMPT_FORMAT_2_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_2_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_3_ADDR                       0xBD0
#define C2H_CMPT_FORMAT_3_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_3_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_4_ADDR                       0xBD4
#define C2H_CMPT_FORMAT_4_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_4_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_5_ADDR                       0xBD8
#define C2H_CMPT_FORMAT_5_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_5_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_CMPT_FORMAT_6_ADDR                       0xBDC
#define C2H_CMPT_FORMAT_6_DESC_ERR_LOC_MASK                GENMASK(31, 16)
#define C2H_CMPT_FORMAT_6_COLOR_LOC_MASK                   GENMASK(15, 0)
#define EQDMA_C2H_PFCH_CACHE_DEPTH_ADDR                    0xBE0
#define C2H_PFCH_CACHE_DEPTH_MAX_STBUF_MASK                GENMASK(23, 16)
#define C2H_PFCH_CACHE_DEPTH_MASK                         GENMASK(7, 0)
#define EQDMA_C2H_WRB_COAL_BUF_DEPTH_ADDR                  0xBE4
#define C2H_WRB_COAL_BUF_DEPTH_RSVD_1_MASK                 GENMASK(31, 8)
#define C2H_WRB_COAL_BUF_DEPTH_BUFFER_MASK                 GENMASK(7, 0)
#define EQDMA_C2H_PFCH_CRDT_ADDR                           0xBE8
#define C2H_PFCH_CRDT_RSVD_1_MASK                          GENMASK(31, 1)
#define C2H_PFCH_CRDT_RSVD_2_MASK                          BIT(0)
#define EQDMA_C2H_STAT_HAS_CMPT_ACCEPTED_ADDR              0xBEC
#define C2H_STAT_HAS_CMPT_ACCEPTED_RSVD_1_MASK             GENMASK(31, 18)
#define C2H_STAT_HAS_CMPT_ACCEPTED_CNT_MASK                GENMASK(17, 0)
#define EQDMA_C2H_STAT_HAS_PLD_ACCEPTED_ADDR               0xBF0
#define C2H_STAT_HAS_PLD_ACCEPTED_RSVD_1_MASK              GENMASK(31, 18)
#define C2H_STAT_HAS_PLD_ACCEPTED_CNT_MASK                 GENMASK(17, 0)
#define EQDMA_C2H_PLD_PKT_ID_ADDR                          0xBF4
#define C2H_PLD_PKT_ID_CMPT_WAIT_MASK                      GENMASK(31, 16)
#define C2H_PLD_PKT_ID_DATA_MASK                           GENMASK(15, 0)
#define EQDMA_C2H_PLD_PKT_ID_1_ADDR                        0xBF8
#define C2H_PLD_PKT_ID_1_CMPT_WAIT_MASK                    GENMASK(31, 16)
#define C2H_PLD_PKT_ID_1_DATA_MASK                         GENMASK(15, 0)
#define EQDMA_C2H_DROP_PLD_CNT_1_ADDR                      0xBFC
#define C2H_DROP_PLD_CNT_1_RSVD_1_MASK                     GENMASK(31, 18)
#define C2H_DROP_PLD_CNT_1_CNT_MASK                        GENMASK(17, 0)
#define EQDMA_H2C_ERR_STAT_ADDR                            0xE00
#define H2C_ERR_STAT_RSVD_1_MASK                           GENMASK(31, 6)
#define H2C_ERR_STAT_PAR_ERR_MASK                          BIT(5)
#define H2C_ERR_STAT_SBE_MASK                              BIT(4)
#define H2C_ERR_STAT_DBE_MASK                              BIT(3)
#define H2C_ERR_STAT_NO_DMA_DS_MASK                        BIT(2)
#define H2C_ERR_STAT_SDI_MRKR_REQ_MOP_ERR_MASK             BIT(1)
#define H2C_ERR_STAT_ZERO_LEN_DS_MASK                      BIT(0)
#define EQDMA_H2C_ERR_MASK_ADDR                            0xE04
#define H2C_ERR_EN_MASK                          GENMASK(31, 0)
#define EQDMA_H2C_FIRST_ERR_QID_ADDR                       0xE08
#define H2C_FIRST_ERR_QID_RSVD_1_MASK                      GENMASK(31, 20)
#define H2C_FIRST_ERR_QID_ERR_TYPE_MASK                    GENMASK(19, 16)
#define H2C_FIRST_ERR_QID_RSVD_2_MASK                      GENMASK(15, 13)
#define H2C_FIRST_ERR_QID_QID_MASK                         GENMASK(12, 0)
#define EQDMA_H2C_DBG_REG0_ADDR                            0xE0C
#define H2C_REG0_NUM_DSC_RCVD_MASK                     GENMASK(31, 16)
#define H2C_REG0_NUM_WRB_SENT_MASK                     GENMASK(15, 0)
#define EQDMA_H2C_DBG_REG1_ADDR                            0xE10
#define H2C_REG1_NUM_REQ_SENT_MASK                     GENMASK(31, 16)
#define H2C_REG1_NUM_CMP_SENT_MASK                     GENMASK(15, 0)
#define EQDMA_H2C_DBG_REG2_ADDR                            0xE14
#define H2C_REG2_RSVD_1_MASK                           GENMASK(31, 16)
#define H2C_REG2_NUM_ERR_DSC_RCVD_MASK                 GENMASK(15, 0)
#define EQDMA_H2C_DBG_REG3_ADDR                            0xE18
#define H2C_REG3_RSVD_1_MASK                           BIT(31)
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
#define EQDMA_H2C_DBG_REG4_ADDR                            0xE1C
#define H2C_REG4_RDREQ_ADDR_MASK                       GENMASK(31, 0)
#define EQDMA_H2C_FATAL_ERR_EN_ADDR                        0xE20
#define H2C_FATAL_ERR_EN_RSVD_1_MASK                       GENMASK(31, 1)
#define H2C_FATAL_ERR_EN_H2C_MASK                          BIT(0)
#define EQDMA_H2C_REQ_THROT_PCIE_ADDR                      0xE24
#define H2C_REQ_THROT_PCIE_EN_REQ_MASK                     BIT(31)
#define H2C_REQ_THROT_PCIE_MASK                           GENMASK(30, 19)
#define H2C_REQ_THROT_PCIE_EN_DATA_MASK                    BIT(18)
#define H2C_REQ_THROT_PCIE_DATA_THRESH_MASK                GENMASK(17, 0)
#define EQDMA_H2C_ALN_DBG_REG0_ADDR                        0xE28
#define H2C_ALN_REG0_NUM_PKT_SENT_MASK                 GENMASK(15, 0)
#define EQDMA_H2C_REQ_THROT_AXIMM_ADDR                     0xE2C
#define H2C_REQ_THROT_AXIMM_EN_REQ_MASK                    BIT(31)
#define H2C_REQ_THROT_AXIMM_MASK                          GENMASK(30, 19)
#define H2C_REQ_THROT_AXIMM_EN_DATA_MASK                   BIT(18)
#define H2C_REQ_THROT_AXIMM_DATA_THRESH_MASK               GENMASK(17, 0)
#define EQDMA_C2H_MM_CTL_ADDR                              0x1004
#define C2H_MM_CTL_RESERVED1_MASK                          GENMASK(31, 9)
#define C2H_MM_CTL_ERRC_EN_MASK                            BIT(8)
#define C2H_MM_CTL_RESERVED0_MASK                          GENMASK(7, 1)
#define C2H_MM_CTL_RUN_MASK                                BIT(0)
#define EQDMA_C2H_MM_STATUS_ADDR                           0x1040
#define C2H_MM_STATUS_RSVD_1_MASK                          GENMASK(31, 1)
#define C2H_MM_STATUS_RUN_MASK                             BIT(0)
#define EQDMA_C2H_MM_CMPL_DESC_CNT_ADDR                    0x1048
#define C2H_MM_CMPL_DESC_CNT_C2H_CO_MASK                   GENMASK(31, 0)
#define EQDMA_C2H_MM_ERR_CODE_ENABLE_MASK_ADDR             0x1054
#define C2H_MM_ERR_CODE_ENABLE_RESERVED1_MASK         BIT(31)
#define C2H_MM_ERR_CODE_ENABLE_WR_UC_RAM_MASK         BIT(30)
#define C2H_MM_ERR_CODE_ENABLE_WR_UR_MASK             BIT(29)
#define C2H_MM_ERR_CODE_ENABLE_WR_FLR_MASK            BIT(28)
#define C2H_MM_ERR_CODE_ENABLE_RESERVED0_MASK         GENMASK(27, 2)
#define C2H_MM_ERR_CODE_ENABLE_RD_SLV_ERR_MASK        BIT(1)
#define C2H_MM_ERR_CODE_ENABLE_WR_SLV_ERR_MASK        BIT(0)
#define EQDMA_C2H_MM_ERR_CODE_ADDR                         0x1058
#define C2H_MM_ERR_CODE_RESERVED1_MASK                     GENMASK(31, 28)
#define C2H_MM_ERR_CODE_CIDX_MASK                          GENMASK(27, 12)
#define C2H_MM_ERR_CODE_RESERVED0_MASK                     GENMASK(11, 10)
#define C2H_MM_ERR_CODE_SUB_TYPE_MASK                      GENMASK(9, 5)
#define C2H_MM_ERR_CODE_MASK                              GENMASK(4, 0)
#define EQDMA_C2H_MM_ERR_INFO_ADDR                         0x105C
#define C2H_MM_ERR_INFO_VALID_MASK                         BIT(31)
#define C2H_MM_ERR_INFO_SEL_MASK                           BIT(30)
#define C2H_MM_ERR_INFO_RSVD_1_MASK                        GENMASK(29, 24)
#define C2H_MM_ERR_INFO_QID_MASK                           GENMASK(23, 0)
#define EQDMA_C2H_MM_PERF_MON_CTL_ADDR                     0x10C0
#define C2H_MM_PERF_MON_CTL_RSVD_1_MASK                    GENMASK(31, 4)
#define C2H_MM_PERF_MON_CTL_IMM_START_MASK                 BIT(3)
#define C2H_MM_PERF_MON_CTL_RUN_START_MASK                 BIT(2)
#define C2H_MM_PERF_MON_CTL_IMM_CLEAR_MASK                 BIT(1)
#define C2H_MM_PERF_MON_CTL_RUN_CLEAR_MASK                 BIT(0)
#define EQDMA_C2H_MM_PERF_MON_CYCLE_CNT0_ADDR              0x10C4
#define C2H_MM_PERF_MON_CYCLE_CNT0_CYC_CNT_MASK            GENMASK(31, 0)
#define EQDMA_C2H_MM_PERF_MON_CYCLE_CNT1_ADDR              0x10C8
#define C2H_MM_PERF_MON_CYCLE_CNT1_RSVD_1_MASK             GENMASK(31, 10)
#define C2H_MM_PERF_MON_CYCLE_CNT1_CYC_CNT_MASK            GENMASK(9, 0)
#define EQDMA_C2H_MM_PERF_MON_DATA_CNT0_ADDR               0x10CC
#define C2H_MM_PERF_MON_DATA_CNT0_DCNT_MASK                GENMASK(31, 0)
#define EQDMA_C2H_MM_PERF_MON_DATA_CNT1_ADDR               0x10D0
#define C2H_MM_PERF_MON_DATA_CNT1_RSVD_1_MASK              GENMASK(31, 10)
#define C2H_MM_PERF_MON_DATA_CNT1_DCNT_MASK                GENMASK(9, 0)
#define EQDMA_C2H_MM_DBG_ADDR                              0x10E8
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
#define EQDMA_H2C_MM_CTL_ADDR                              0x1204
#define H2C_MM_CTL_RESERVED1_MASK                          GENMASK(31, 9)
#define H2C_MM_CTL_ERRC_EN_MASK                            BIT(8)
#define H2C_MM_CTL_RESERVED0_MASK                          GENMASK(7, 1)
#define H2C_MM_CTL_RUN_MASK                                BIT(0)
#define EQDMA_H2C_MM_STATUS_ADDR                           0x1240
#define H2C_MM_STATUS_RSVD_1_MASK                          GENMASK(31, 1)
#define H2C_MM_STATUS_RUN_MASK                             BIT(0)
#define EQDMA_H2C_MM_CMPL_DESC_CNT_ADDR                    0x1248
#define H2C_MM_CMPL_DESC_CNT_H2C_CO_MASK                   GENMASK(31, 0)
#define EQDMA_H2C_MM_ERR_CODE_ENABLE_MASK_ADDR             0x1254
#define H2C_MM_ERR_CODE_ENABLE_RESERVED5_MASK         GENMASK(31, 30)
#define H2C_MM_ERR_CODE_ENABLE_WR_SLV_ERR_MASK        BIT(29)
#define H2C_MM_ERR_CODE_ENABLE_WR_DEC_ERR_MASK        BIT(28)
#define H2C_MM_ERR_CODE_ENABLE_RESERVED4_MASK         GENMASK(27, 23)
#define H2C_MM_ERR_CODE_ENABLE_RD_RQ_DIS_ERR_MASK     BIT(22)
#define H2C_MM_ERR_CODE_ENABLE_RESERVED3_MASK         GENMASK(21, 17)
#define H2C_MM_ERR_CODE_ENABLE_RD_DAT_POISON_ERR_MASK BIT(16)
#define H2C_MM_ERR_CODE_ENABLE_RESERVED2_MASK         GENMASK(15, 9)
#define H2C_MM_ERR_CODE_ENABLE_RD_FLR_ERR_MASK        BIT(8)
#define H2C_MM_ERR_CODE_ENABLE_RESERVED1_MASK         GENMASK(7, 6)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_ADR_ERR_MASK    BIT(5)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_PARA_MASK       BIT(4)
#define H2C_MM_ERR_CODE_ENABLE_RD_HDR_BYTE_ERR_MASK   BIT(3)
#define H2C_MM_ERR_CODE_ENABLE_RD_UR_CA_MASK          BIT(2)
#define H2C_MM_ERR_CODE_ENABLE_RD_HRD_POISON_ERR_MASK BIT(1)
#define H2C_MM_ERR_CODE_ENABLE_RESERVED0_MASK         BIT(0)
#define EQDMA_H2C_MM_ERR_CODE_ADDR                         0x1258
#define H2C_MM_ERR_CODE_RSVD_1_MASK                        GENMASK(31, 28)
#define H2C_MM_ERR_CODE_CIDX_MASK                          GENMASK(27, 12)
#define H2C_MM_ERR_CODE_RESERVED0_MASK                     GENMASK(11, 10)
#define H2C_MM_ERR_CODE_SUB_TYPE_MASK                      GENMASK(9, 5)
#define H2C_MM_ERR_CODE_MASK                              GENMASK(4, 0)
#define EQDMA_H2C_MM_ERR_INFO_ADDR                         0x125C
#define H2C_MM_ERR_INFO_VALID_MASK                         BIT(31)
#define H2C_MM_ERR_INFO_SEL_MASK                           BIT(30)
#define H2C_MM_ERR_INFO_RSVD_1_MASK                        GENMASK(29, 24)
#define H2C_MM_ERR_INFO_QID_MASK                           GENMASK(23, 0)
#define EQDMA_H2C_MM_PERF_MON_CTL_ADDR                     0x12C0
#define H2C_MM_PERF_MON_CTL_RSVD_1_MASK                    GENMASK(31, 4)
#define H2C_MM_PERF_MON_CTL_IMM_START_MASK                 BIT(3)
#define H2C_MM_PERF_MON_CTL_RUN_START_MASK                 BIT(2)
#define H2C_MM_PERF_MON_CTL_IMM_CLEAR_MASK                 BIT(1)
#define H2C_MM_PERF_MON_CTL_RUN_CLEAR_MASK                 BIT(0)
#define EQDMA_H2C_MM_PERF_MON_CYCLE_CNT0_ADDR              0x12C4
#define H2C_MM_PERF_MON_CYCLE_CNT0_CYC_CNT_MASK            GENMASK(31, 0)
#define EQDMA_H2C_MM_PERF_MON_CYCLE_CNT1_ADDR              0x12C8
#define H2C_MM_PERF_MON_CYCLE_CNT1_RSVD_1_MASK             GENMASK(31, 10)
#define H2C_MM_PERF_MON_CYCLE_CNT1_CYC_CNT_MASK            GENMASK(9, 0)
#define EQDMA_H2C_MM_PERF_MON_DATA_CNT0_ADDR               0x12CC
#define H2C_MM_PERF_MON_DATA_CNT0_DCNT_MASK                GENMASK(31, 0)
#define EQDMA_H2C_MM_PERF_MON_DATA_CNT1_ADDR               0x12D0
#define H2C_MM_PERF_MON_DATA_CNT1_RSVD_1_MASK              GENMASK(31, 10)
#define H2C_MM_PERF_MON_DATA_CNT1_DCNT_MASK                GENMASK(9, 0)
#define EQDMA_H2C_MM_DBG_ADDR                              0x12E8
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
#define EQDMA_C2H_CRDT_COAL_CFG_1_ADDR                     0x1400
#define C2H_CRDT_COAL_CFG_1_RSVD_1_MASK                    GENMASK(31, 18)
#define C2H_CRDT_COAL_CFG_1_PLD_FIFO_TH_MASK               GENMASK(17, 10)
#define C2H_CRDT_COAL_CFG_1_TIMER_TH_MASK                  GENMASK(9, 0)
#define EQDMA_C2H_CRDT_COAL_CFG_2_ADDR                     0x1404
#define C2H_CRDT_COAL_CFG_2_RSVD_1_MASK                    GENMASK(31, 24)
#define C2H_CRDT_COAL_CFG_2_FIFO_TH_MASK                   GENMASK(23, 16)
#define C2H_CRDT_COAL_CFG_2_RESERVED1_MASK                 GENMASK(15, 11)
#define C2H_CRDT_COAL_CFG_2_NT_TH_MASK                     GENMASK(10, 0)
#define EQDMA_C2H_PFCH_BYP_QID_ADDR                        0x1408
#define C2H_PFCH_BYP_QID_RSVD_1_MASK                       GENMASK(31, 12)
#define C2H_PFCH_BYP_QID_MASK                             GENMASK(11, 0)
#define EQDMA_C2H_PFCH_BYP_TAG_ADDR                        0x140C
#define C2H_PFCH_BYP_TAG_RSVD_1_MASK                       GENMASK(31, 20)
#define C2H_PFCH_BYP_TAG_BYP_QID_MASK                      GENMASK(19, 8)
#define C2H_PFCH_BYP_TAG_RSVD_2_MASK                       BIT(7)
#define C2H_PFCH_BYP_TAG_MASK                             GENMASK(6, 0)
#define EQDMA_C2H_WATER_MARK_ADDR                          0x1500
#define C2H_WATER_MARK_HIGH_WM_MASK                        GENMASK(31, 16)
#define C2H_WATER_MARK_LOW_WM_MASK                         GENMASK(15, 0)
#define SW_IND_CTXT_DATA_W7_VIRTIO_DSC_BASE_H_MASK        GENMASK(10, 0)
#define SW_IND_CTXT_DATA_W6_VIRTIO_DSC_BASE_M_MASK        GENMASK(31, 0)
#define SW_IND_CTXT_DATA_W5_VIRTIO_DSC_BASE_L_MASK        GENMASK(31, 11)
#define SW_IND_CTXT_DATA_W5_PASID_EN_MASK                 BIT(10)
#define SW_IND_CTXT_DATA_W5_PASID_H_MASK                  GENMASK(9, 0)
#define SW_IND_CTXT_DATA_W4_PASID_L_MASK                  GENMASK(31, 20)
#define SW_IND_CTXT_DATA_W4_HOST_ID_MASK                  GENMASK(19, 16)
#define SW_IND_CTXT_DATA_W4_IRQ_BYP_MASK                  BIT(15)
#define SW_IND_CTXT_DATA_W4_PACK_BYP_OUT_MASK             BIT(14)
#define SW_IND_CTXT_DATA_W4_VIRTIO_EN_MASK                BIT(13)
#define SW_IND_CTXT_DATA_W4_DIS_INTR_ON_VF_MASK           BIT(12)
#define SW_IND_CTXT_DATA_W4_INT_AGGR_MASK                 BIT(11)
#define SW_IND_CTXT_DATA_W4_VEC_MASK                      GENMASK(10, 0)
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
#define SW_IND_CTXT_DATA_W1_RSVD_1_MASK                   GENMASK(11, 9)
#define SW_IND_CTXT_DATA_W1_FETCH_MAX_MASK                GENMASK(8, 5)
#define SW_IND_CTXT_DATA_W1_AT_MASK                       BIT(4)
#define SW_IND_CTXT_DATA_W1_WBI_INTVL_EN_MASK             BIT(3)
#define SW_IND_CTXT_DATA_W1_WBI_CHK_MASK                  BIT(2)
#define SW_IND_CTXT_DATA_W1_FCRD_EN_MASK                  BIT(1)
#define SW_IND_CTXT_DATA_W1_QEN_MASK                      BIT(0)
#define SW_IND_CTXT_DATA_W0_RSV_MASK                      GENMASK(31, 29)
#define SW_IND_CTXT_DATA_W0_FNC_MASK                      GENMASK(28, 17)
#define SW_IND_CTXT_DATA_W0_IRQ_ARM_MASK                  BIT(16)
#define SW_IND_CTXT_DATA_W0_PIDX_MASK                     GENMASK(15, 0)
#define HW_IND_CTXT_DATA_W1_RSVD_1_MASK                   BIT(15)
#define HW_IND_CTXT_DATA_W1_FETCH_PND_MASK                GENMASK(14, 11)
#define HW_IND_CTXT_DATA_W1_EVT_PND_MASK                  BIT(10)
#define HW_IND_CTXT_DATA_W1_IDL_STP_B_MASK                BIT(9)
#define HW_IND_CTXT_DATA_W1_DSC_PND_MASK                  BIT(8)
#define HW_IND_CTXT_DATA_W1_RSVD_2_MASK                   GENMASK(7, 0)
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
#define PREFETCH_CTXT_DATA_W0_RSVD_MASK                   GENMASK(25, 22)
#define PREFETCH_CTXT_DATA_W0_PFCH_NEED_MASK              GENMASK(21, 16)
#define PREFETCH_CTXT_DATA_W0_NUM_PFCH_MASK               GENMASK(15, 10)
#define PREFETCH_CTXT_DATA_W0_VIRTIO_MASK                 BIT(9)
#define PREFETCH_CTXT_DATA_W0_VAR_DESC_MASK               BIT(8)
#define PREFETCH_CTXT_DATA_W0_PORT_ID_MASK                GENMASK(7, 5)
#define PREFETCH_CTXT_DATA_W0_BUF_SZ_IDX_MASK             GENMASK(4, 1)
#define PREFETCH_CTXT_DATA_W0_BYPASS_MASK                 BIT(0)
#define CMPL_CTXT_DATA_W6_RSVD_1_H_MASK                   GENMASK(7, 0)
#define CMPL_CTXT_DATA_W5_RSVD_1_L_MASK                   GENMASK(31, 23)
#define CMPL_CTXT_DATA_W5_PORT_ID_MASK                    GENMASK(22, 20)
#define CMPL_CTXT_DATA_W5_SH_CMPT_MASK                    BIT(19)
#define CMPL_CTXT_DATA_W5_VIO_EOP_MASK                    BIT(18)
#define CMPL_CTXT_DATA_W5_BADDR4_LOW_MASK                 GENMASK(17, 14)
#define CMPL_CTXT_DATA_W5_PASID_EN_MASK                   BIT(13)
#define CMPL_CTXT_DATA_W5_PASID_H_MASK                    GENMASK(12, 0)
#define CMPL_CTXT_DATA_W4_PASID_L_MASK                    GENMASK(31, 23)
#define CMPL_CTXT_DATA_W4_HOST_ID_MASK                    GENMASK(22, 19)
#define CMPL_CTXT_DATA_W4_DIR_C2H_MASK                    BIT(18)
#define CMPL_CTXT_DATA_W4_VIO_MASK                        BIT(17)
#define CMPL_CTXT_DATA_W4_DIS_INTR_ON_VF_MASK             BIT(16)
#define CMPL_CTXT_DATA_W4_INT_AGGR_MASK                   BIT(15)
#define CMPL_CTXT_DATA_W4_VEC_MASK                        GENMASK(14, 4)
#define CMPL_CTXT_DATA_W4_AT_MASK                         BIT(3)
#define CMPL_CTXT_DATA_W4_OVF_CHK_DIS_MASK                BIT(2)
#define CMPL_CTXT_DATA_W4_FULL_UPD_MASK                   BIT(1)
#define CMPL_CTXT_DATA_W4_TIMER_RUNNING_MASK              BIT(0)
#define CMPL_CTXT_DATA_W3_USER_TRIG_PEND_MASK             BIT(31)
#define CMPL_CTXT_DATA_W3_ERR_MASK                        GENMASK(30, 29)
#define CMPL_CTXT_DATA_W3_VALID_MASK                      BIT(28)
#define CMPL_CTXT_DATA_W3_CIDX_MASK                       GENMASK(27, 12)
#define CMPL_CTXT_DATA_W3_PIDX_H_MASK                     GENMASK(11, 0)
#define CMPL_CTXT_DATA_W2_PIDX_L_MASK                     GENMASK(31, 28)
#define CMPL_CTXT_DATA_W2_DESC_SIZE_MASK                  GENMASK(27, 26)
#define CMPL_CTXT_DATA_W2_BADDR4_HIGH_H_MASK              GENMASK(25, 0)
#define CMPL_CTXT_DATA_W1_BADDR4_HIGH_L_MASK              GENMASK(31, 0)
#define CMPL_CTXT_DATA_W0_QSIZE_IX_MASK                   GENMASK(31, 28)
#define CMPL_CTXT_DATA_W0_COLOR_MASK                      BIT(27)
#define CMPL_CTXT_DATA_W0_INT_ST_MASK                     GENMASK(26, 25)
#define CMPL_CTXT_DATA_W0_TIMER_IX_MASK                   GENMASK(24, 21)
#define CMPL_CTXT_DATA_W0_CNTER_IX_MASK                   GENMASK(20, 17)
#define CMPL_CTXT_DATA_W0_FNC_ID_MASK                     GENMASK(16, 5)
#define CMPL_CTXT_DATA_W0_TRIG_MODE_MASK                  GENMASK(4, 2)
#define CMPL_CTXT_DATA_W0_EN_INT_MASK                     BIT(1)
#define CMPL_CTXT_DATA_W0_EN_STAT_DESC_MASK               BIT(0)
#define INTR_CTXT_DATA_W3_FUNC_MASK                       GENMASK(29, 18)
#define INTR_CTXT_DATA_W3_RSVD_MASK                       GENMASK(17, 14)
#define INTR_CTXT_DATA_W3_PASID_EN_MASK                   BIT(13)
#define INTR_CTXT_DATA_W3_PASID_H_MASK                    GENMASK(12, 0)
#define INTR_CTXT_DATA_W2_PASID_L_MASK                    GENMASK(31, 23)
#define INTR_CTXT_DATA_W2_HOST_ID_MASK                    GENMASK(22, 19)
#define INTR_CTXT_DATA_W2_AT_MASK                         BIT(18)
#define INTR_CTXT_DATA_W2_PIDX_MASK                       GENMASK(17, 6)
#define INTR_CTXT_DATA_W2_PAGE_SIZE_MASK                  GENMASK(5, 3)
#define INTR_CTXT_DATA_W2_BADDR_4K_H_MASK                 GENMASK(2, 0)
#define INTR_CTXT_DATA_W1_BADDR_4K_M_MASK                 GENMASK(31, 0)
#define INTR_CTXT_DATA_W0_BADDR_4K_L_MASK                 GENMASK(31, 15)
#define INTR_CTXT_DATA_W0_COLOR_MASK                      BIT(14)
#define INTR_CTXT_DATA_W0_INT_ST_MASK                     BIT(13)
#define INTR_CTXT_DATA_W0_RSVD1_MASK                      BIT(12)
#define INTR_CTXT_DATA_W0_VEC_MASK                        GENMASK(11, 1)
#define INTR_CTXT_DATA_W0_VALID_MASK                      BIT(0)

#ifdef __cplusplus
}
#endif

#endif

