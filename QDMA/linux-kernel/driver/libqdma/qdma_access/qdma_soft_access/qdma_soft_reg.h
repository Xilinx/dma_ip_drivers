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

#ifndef __QDMA_SOFT_REG_H__
#define __QDMA_SOFT_REG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * User defined helper macros for masks and shifts. If the same macros are
 * defined in linux kernel code , then undefined them and used the user
 * defined macros
 */
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


#define QDMA_H2C_THROT_DATA_THRESH       0x4000
#define QDMA_THROT_EN_DATA               1
#define QDMA_THROT_EN_REQ                0
#define QDMA_H2C_THROT_REQ_THRESH        0x60

/*
 * Q Context programming (indirect)
 */

#define QDMA_REG_IND_CTXT_REG_COUNT                         8
#define QDMA_REG_IND_CTXT_WCNT_1                            1
#define QDMA_REG_IND_CTXT_WCNT_2                            2
#define QDMA_REG_IND_CTXT_WCNT_3                            3
#define QDMA_REG_IND_CTXT_WCNT_4                            4
#define QDMA_REG_IND_CTXT_WCNT_5                            5
#define QDMA_REG_IND_CTXT_WCNT_6                            6
#define QDMA_REG_IND_CTXT_WCNT_7                            7
#define QDMA_REG_IND_CTXT_WCNT_8                            8

/* ------------------------- QDMA_TRQ_SEL_IND (0x00800) ----------------*/
#define QDMA_OFFSET_IND_CTXT_DATA                           0x804
#define QDMA_OFFSET_IND_CTXT_MASK                           0x824
#define QDMA_OFFSET_IND_CTXT_CMD                            0x844
#define     QDMA_IND_CTXT_CMD_BUSY_MASK                     0x1

/** QDMA_IND_REG_SEL_FMAP */
#define QDMA_FMAP_CTXT_W1_QID_MAX_MASK                      GENMASK(11, 0)
#define QDMA_FMAP_CTXT_W0_QID_MASK                          GENMASK(10, 0)

/** QDMA_IND_REG_SEL_SW_C2H */
/** QDMA_IND_REG_SEL_SW_H2C */
#define QDMA_SW_CTXT_W4_INTR_AGGR_MASK                      BIT(11)
#define QDMA_SW_CTXT_W4_VEC_MASK                            GENMASK(10, 0)
#define QDMA_SW_CTXT_W3_DSC_H_MASK                          GENMASK(31, 0)
#define QDMA_SW_CTXT_W2_DSC_L_MASK                          GENMASK(31, 0)
#define QDMA_SW_CTXT_W1_IS_MM_MASK                          BIT(31)
#define QDMA_SW_CTXT_W1_MRKR_DIS_MASK                       BIT(30)
#define QDMA_SW_CTXT_W1_IRQ_REQ_MASK                        BIT(29)
#define QDMA_SW_CTXT_W1_ERR_WB_SENT_MASK                    BIT(28)
#define QDMA_SW_CTXT_W1_ERR_MASK                            GENMASK(27, 26)
#define QDMA_SW_CTXT_W1_IRQ_NO_LAST_MASK                    BIT(25)
#define QDMA_SW_CTXT_W1_PORT_ID_MASK                        GENMASK(24, 22)
#define QDMA_SW_CTXT_W1_IRQ_EN_MASK                         BIT(21)
#define QDMA_SW_CTXT_W1_WBK_EN_MASK                         BIT(20)
#define QDMA_SW_CTXT_W1_MM_CHN_MASK                         BIT(19)
#define QDMA_SW_CTXT_W1_BYP_MASK                            BIT(18)
#define QDMA_SW_CTXT_W1_DSC_SZ_MASK                         GENMASK(17, 16)
#define QDMA_SW_CTXT_W1_RNG_SZ_MASK                         GENMASK(15, 12)
#define QDMA_SW_CTXT_W1_FETCH_MAX_MASK                      GENMASK(7, 5)
#define QDMA_SW_CTXT_W1_AT_MASK                             BIT(4)
#define QDMA_SW_CTXT_W1_WB_INT_EN_MASK                      BIT(3)
#define QDMA_SW_CTXT_W1_WBI_CHK_MASK                        BIT(2)
#define QDMA_SW_CTXT_W1_FCRD_EN_MASK                        BIT(1)
#define QDMA_SW_CTXT_W1_QEN_MASK                            BIT(0)
#define QDMA_SW_CTXT_W0_FUNC_ID_MASK                        GENMASK(24, 17)
#define QDMA_SW_CTXT_W0_IRQ_ARM_MASK                        BIT(16)
#define QDMA_SW_CTXT_W0_PIDX                                GENMASK(15, 0)



#define QDMA_PFTCH_CTXT_W1_VALID_MASK                       BIT(13)
#define QDMA_PFTCH_CTXT_W1_SW_CRDT_H_MASK                   GENMASK(12, 0)
#define QDMA_PFTCH_CTXT_W0_SW_CRDT_L_MASK                   GENMASK(31, 29)
#define QDMA_PFTCH_CTXT_W0_Q_IN_PFETCH_MASK                 BIT(28)
#define QDMA_PFTCH_CTXT_W0_PFETCH_EN_MASK                   BIT(27)
#define QDMA_PFTCH_CTXT_W0_ERR_MASK                         BIT(26)
#define QDMA_PFTCH_CTXT_W0_PORT_ID_MASK                     GENMASK(7, 5)
#define QDMA_PFTCH_CTXT_W0_BUF_SIZE_IDX_MASK                GENMASK(4, 1)
#define QDMA_PFTCH_CTXT_W0_BYPASS_MASK                      BIT(0)




#define QDMA_COMPL_CTXT_W4_INTR_AGGR_MASK                   BIT(15)
#define QDMA_COMPL_CTXT_W4_INTR_VEC_MASK                    GENMASK(14, 4)
#define QDMA_COMPL_CTXT_W4_AT_MASK                          BIT(3)
#define QDMA_COMPL_CTXT_W4_OVF_CHK_DIS_MASK                 BIT(2)
#define QDMA_COMPL_CTXT_W4_FULL_UPDT_MASK                   BIT(1)
#define QDMA_COMPL_CTXT_W4_TMR_RUN_MASK                     BIT(0)
#define QDMA_COMPL_CTXT_W3_USR_TRG_PND_MASK                 BIT(31)
#define QDMA_COMPL_CTXT_W3_ERR_MASK                         GENMASK(30, 29)
#define QDMA_COMPL_CTXT_W3_VALID_MASK                       BIT(28)
#define QDMA_COMPL_CTXT_W3_CIDX_MASK                        GENMASK(27, 12)
#define QDMA_COMPL_CTXT_W3_PIDX_H_MASK                      GENMASK(11, 0)
#define QDMA_COMPL_CTXT_W2_PIDX_L_MASK                      GENMASK(31, 28)
#define QDMA_COMPL_CTXT_W2_DESC_SIZE_MASK                   GENMASK(27, 26)
#define QDMA_COMPL_CTXT_W2_BADDR_64_H_MASK                  GENMASK(25, 0)
#define QDMA_COMPL_CTXT_W1_BADDR_64_L_MASK                  GENMASK(31, 6)
#define QDMA_COMPL_CTXT_W0_RING_SZ_MASK                     GENMASK(31, 28)
#define QDMA_COMPL_CTXT_W0_COLOR_MASK                       BIT(27)
#define QDMA_COMPL_CTXT_W0_INT_ST_MASK                      GENMASK(26, 25)
#define QDMA_COMPL_CTXT_W0_TIMER_IDX_MASK                   GENMASK(24, 21)
#define QDMA_COMPL_CTXT_W0_COUNTER_IDX_MASK                 GENMASK(20, 17)
#define QDMA_COMPL_CTXT_W0_FNC_ID_MASK                      GENMASK(12, 5)
#define QDMA_COMPL_CTXT_W0_TRIG_MODE_MASK                   GENMASK(4, 2)
#define QDMA_COMPL_CTXT_W0_EN_INT_MASK                      BIT(1)
#define QDMA_COMPL_CTXT_W0_EN_STAT_DESC_MASK                BIT(0)

/** QDMA_IND_REG_SEL_HW_C2H */
/** QDMA_IND_REG_SEL_HW_H2C */
#define QDMA_HW_CTXT_W1_FETCH_PEND_MASK                     GENMASK(14, 11)
#define QDMA_HW_CTXT_W1_EVENT_PEND_MASK                     BIT(10)
#define QDMA_HW_CTXT_W1_IDL_STP_B_MASK                      BIT(9)
#define QDMA_HW_CTXT_W1_DSC_PND_MASK                        BIT(8)
#define QDMA_HW_CTXT_W0_CRD_USE_MASK                        GENMASK(31, 16)
#define QDMA_HW_CTXT_W0_CIDX_MASK                           GENMASK(15, 0)

/** QDMA_IND_REG_SEL_CR_C2H */
/** QDMA_IND_REG_SEL_CR_H2C */
#define QDMA_CR_CTXT_W0_CREDT_MASK                          GENMASK(15, 0)

/** QDMA_IND_REG_SEL_INTR */


#define QDMA_INTR_CTXT_W2_AT_MASK                           BIT(18)
#define QDMA_INTR_CTXT_W2_PIDX_MASK                         GENMASK(17, 6)
#define QDMA_INTR_CTXT_W2_PAGE_SIZE_MASK                    GENMASK(5, 3)
#define QDMA_INTR_CTXT_W2_BADDR_64_MASK                     GENMASK(2, 0)
#define QDMA_INTR_CTXT_W1_BADDR_64_MASK                     GENMASK(31, 0)
#define QDMA_INTR_CTXT_W0_BADDR_64_MASK                     GENMASK(31, 15)
#define QDMA_INTR_CTXT_W0_COLOR_MASK                        BIT(14)
#define QDMA_INTR_CTXT_W0_INT_ST_MASK                       BIT(13)
#define QDMA_INTR_CTXT_W0_VEC_ID_MASK                       GENMASK(11, 1)
#define QDMA_INTR_CTXT_W0_VALID_MASK                        BIT(0)





/* ------------------------ QDMA_TRQ_SEL_GLBL (0x00200)-------------------*/
#define QDMA_OFFSET_GLBL_RNG_SZ                             0x204
#define QDMA_OFFSET_GLBL_SCRATCH                            0x244
#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define QDMA_OFFSET_GLBL_DSC_CFG                            0x250
#define     QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK               GENMASK(2, 0)
#define     QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK            GENMASK(5, 3)
#define QDMA_OFFSET_GLBL_DSC_ERR_STS                        0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MSK                        0x258
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG0                       0x25C
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG1                       0x260
#define QDMA_OFFSET_GLBL_TRQ_ERR_STS                        0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MSK                        0x268
#define QDMA_OFFSET_GLBL_TRQ_ERR_LOG                        0x26C
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT0                       0x270
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT1                       0x274
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG2                       0x27C
#define QDMA_OFFSET_GLBL_INTERRUPT_CFG                      0x2C4
#define     QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK            BIT(0)
#define     QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK              BIT(1)

/* ------------------------- QDMA_TRQ_SEL_C2H (0x00A00) ------------------*/
#define QDMA_OFFSET_C2H_TIMER_CNT                           0xA00
#define QDMA_OFFSET_C2H_CNT_TH                              0xA40
#define QDMA_OFFSET_C2H_QID2VEC_MAP_QID                     0xA80
#define QDMA_OFFSET_C2H_QID2VEC_MAP                         0xA84
#define QDMA_OFFSET_C2H_STAT_S_AXIS_C2H_ACCEPTED            0xA88
#define QDMA_OFFSET_C2H_STAT_S_AXIS_CMPT_ACCEPTED           0xA8C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_PKT_ACCEPTED          0xA90
#define QDMA_OFFSET_C2H_STAT_AXIS_PKG_CMP                   0xA94
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ACCEPTED              0xA98
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_CMP                   0xA9C
#define QDMA_OFFSET_C2H_STAT_WRQ_OUT                        0xAA0
#define QDMA_OFFSET_C2H_STAT_WPL_REN_ACCEPTED               0xAA4
#define QDMA_OFFSET_C2H_STAT_TOTAL_WRQ_LEN                  0xAA8
#define QDMA_OFFSET_C2H_STAT_TOTAL_WPL_LEN                  0xAAC
#define QDMA_OFFSET_C2H_BUF_SZ                              0xAB0
#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define QDMA_OFFSET_C2H_FATAL_ERR_ENABLE                    0xB00
#define QDMA_OFFSET_C2H_ERR_INT                             0xB04
#define QDMA_OFFSET_C2H_PFETCH_CFG                          0xB08
#define     QDMA_C2H_EVT_QCNT_TH_MASK                       GENMASK(31, 25)
#define     QDMA_C2H_PFCH_QCNT_MASK                         GENMASK(24, 18)
#define     QDMA_C2H_NUM_PFCH_MASK                          GENMASK(17, 9)
#define     QDMA_C2H_PFCH_FL_TH_MASK                        GENMASK(8, 0)
#define QDMA_OFFSET_C2H_INT_TIMER_TICK                      0xB0C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_DROP_ACCEPTED         0xB10
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ERR_ACCEPTED          0xB14
#define QDMA_OFFSET_C2H_STAT_DESC_REQ                       0xB18
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_0                0xB1C
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_1                0xB20
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_2                0xB24
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_3                0xB28
#define QDMA_OFFSET_C2H_DBG_PFCH_ERR_CTXT                   0xB2C
#define QDMA_OFFSET_C2H_FIRST_ERR_QID                       0xB30
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_IN                    0xB34
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_OUT                   0xB38
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_DRP                   0xB3C
#define QDMA_OFFSET_C2H_STAT_NUM_STAT_DESC_OUT              0xB40
#define QDMA_OFFSET_C2H_STAT_NUM_DSC_CRDT_SENT              0xB44
#define QDMA_OFFSET_C2H_STAT_NUM_FCH_DSC_RCVD               0xB48
#define QDMA_OFFSET_C2H_STAT_NUM_BYP_DSC_RCVD               0xB4C
#define QDMA_OFFSET_C2H_WRB_COAL_CFG                        0xB50
#define     QDMA_C2H_MAX_BUF_SZ_MASK                        GENMASK(31, 26)
#define     QDMA_C2H_TICK_VAL_MASK                          GENMASK(25, 14)
#define     QDMA_C2H_TICK_CNT_MASK                          GENMASK(13, 2)
#define     QDMA_C2H_SET_GLB_FLUSH_MASK                     BIT(1)
#define     QDMA_C2H_DONE_GLB_FLUSH_MASK                    BIT(0)
#define QDMA_OFFSET_C2H_INTR_H2C_REQ                        0xB54
#define QDMA_OFFSET_C2H_INTR_C2H_MM_REQ                     0xB58
#define QDMA_OFFSET_C2H_INTR_ERR_INT_REQ                    0xB5C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_REQ                     0xB60
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK        0xB64
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL       0xB68
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX    0xB6C
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_CTXT_INVAL      0xB70
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_ACK                0xB74
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_FAIL               0xB78
#define QDMA_OFFSET_C2H_INTR_C2H_ST_NO_MSIX                 0xB7C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_CTXT_INVAL              0xB80
#define QDMA_OFFSET_C2H_STAT_WR_CMP                         0xB84
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_4                0xB88
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_5                0xB8C
#define QDMA_OFFSET_C2H_DBG_PFCH_QID                        0xB90
#define QDMA_OFFSET_C2H_DBG_PFCH                            0xB94
#define QDMA_OFFSET_C2H_INT_DEBUG                           0xB98
#define QDMA_OFFSET_C2H_STAT_IMM_ACCEPTED                   0xB9C
#define QDMA_OFFSET_C2H_STAT_MARKER_ACCEPTED                0xBA0
#define QDMA_OFFSET_C2H_STAT_DISABLE_CMP_ACCEPTED           0xBA4
#define QDMA_OFFSET_C2H_PAYLOAD_FIFO_CRDT_CNT               0xBA8
#define QDMA_OFFSET_C2H_PFETCH_CACHE_DEPTH                  0xBE0
#define QDMA_OFFSET_C2H_CMPT_COAL_BUF_DEPTH                 0xBE4

/* ------------------------- QDMA_TRQ_SEL_H2C (0x00E00) ------------------*/
#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define QDMA_OFFSET_H2C_FIRST_ERR_QID                       0xE08
#define QDMA_OFFSET_H2C_DBG_REG0                            0xE0C
#define QDMA_OFFSET_H2C_DBG_REG1                            0xE10
#define QDMA_OFFSET_H2C_DBG_REG2                            0xE14
#define QDMA_OFFSET_H2C_DBG_REG3                            0xE18
#define QDMA_OFFSET_H2C_DBG_REG4                            0xE1C
#define QDMA_OFFSET_H2C_FATAL_ERR_EN                        0xE20
#define QDMA_OFFSET_H2C_REQ_THROT                           0xE24
#define     QDMA_H2C_REQ_THROT_EN_REQ_MASK                  BIT(31)
#define     QDMA_H2C_REQ_THRESH_MASK                        GENMASK(25, 17)
#define     QDMA_H2C_REQ_THROT_EN_DATA_MASK                 BIT(16)
#define     QDMA_H2C_DATA_THRESH_MASK                       GENMASK(15, 0)


/* ------------------------- QDMA_TRQ_SEL_H2C_MM (0x1200) ----------------*/
#define QDMA_OFFSET_H2C_MM_CONTROL                          0x1204
#define QDMA_OFFSET_H2C_MM_CONTROL_W1S                      0x1208
#define QDMA_OFFSET_H2C_MM_CONTROL_W1C                      0x120C
#define QDMA_OFFSET_H2C_MM_STATUS                           0x1240
#define QDMA_OFFSET_H2C_MM_STATUS_RC                        0x1244
#define QDMA_OFFSET_H2C_MM_COMPLETED_DESC_COUNT             0x1248
#define QDMA_OFFSET_H2C_MM_ERR_CODE_EN_MASK                 0x1254
#define QDMA_OFFSET_H2C_MM_ERR_CODE                         0x1258
#define QDMA_OFFSET_H2C_MM_ERR_INFO                         0x125C
#define QDMA_OFFSET_H2C_MM_PERF_MON_CONTROL                 0x12C0
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_0           0x12C4
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_1           0x12C8
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_0            0x12CC
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_1            0x12D0
#define QDMA_OFFSET_H2C_MM_DEBUG                            0x12E8

/* ------------------------- QDMA_TRQ_SEL_C2H_MM (0x1000) ----------------*/
#define QDMA_OFFSET_C2H_MM_CONTROL                          0x1004
#define QDMA_OFFSET_C2H_MM_CONTROL_W1S                      0x1008
#define QDMA_OFFSET_C2H_MM_CONTROL_W1C                      0x100C
#define QDMA_OFFSET_C2H_MM_STATUS                           0x1040
#define QDMA_OFFSET_C2H_MM_STATUS_RC                        0x1044
#define QDMA_OFFSET_C2H_MM_COMPLETED_DESC_COUNT             0x1048
#define QDMA_OFFSET_C2H_MM_ERR_CODE_EN_MASK                 0x1054
#define QDMA_OFFSET_C2H_MM_ERR_CODE                         0x1058
#define QDMA_OFFSET_C2H_MM_ERR_INFO                         0x105C
#define QDMA_OFFSET_C2H_MM_PERF_MON_CONTROL                 0x10C0
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_0           0x10C4
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_1           0x10C8
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_0            0x10CC
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_1            0x10D0
#define QDMA_OFFSET_C2H_MM_DEBUG                            0x10E8

/* ------------------------- QDMA_TRQ_SEL_GLBL1 (0x0) -----------------*/
#define QDMA_OFFSET_CONFIG_BLOCK_ID                         0x0
#define     QDMA_CONFIG_BLOCK_ID_MASK                       GENMASK(31, 16)


/* ------------------------- QDMA_TRQ_SEL_GLBL2 (0x00100) ----------------*/
#define QDMA_OFFSET_GLBL2_ID                                0x100
#define QDMA_OFFSET_GLBL2_PF_BARLITE_INT                    0x104
#define     QDMA_GLBL2_PF3_BAR_MAP_MASK                     GENMASK(23, 18)
#define     QDMA_GLBL2_PF2_BAR_MAP_MASK                     GENMASK(17, 12)
#define     QDMA_GLBL2_PF1_BAR_MAP_MASK                     GENMASK(11, 6)
#define     QDMA_GLBL2_PF0_BAR_MAP_MASK                     GENMASK(5, 0)
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_INT                 0x108
#define QDMA_OFFSET_GLBL2_PF_BARLITE_EXT                    0x10C
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_EXT                 0x110
#define QDMA_OFFSET_GLBL2_CHANNEL_INST                      0x114
#define QDMA_OFFSET_GLBL2_CHANNEL_MDMA                      0x118
#define     QDMA_GLBL2_ST_C2H_MASK                          BIT(17)
#define     QDMA_GLBL2_ST_H2C_MASK                          BIT(16)
#define     QDMA_GLBL2_MM_C2H_MASK                          BIT(8)
#define     QDMA_GLBL2_MM_H2C_MASK                          BIT(0)
#define QDMA_OFFSET_GLBL2_CHANNEL_STRM                      0x11C
#define QDMA_OFFSET_GLBL2_CHANNEL_QDMA_CAP                  0x120
#define     QDMA_GLBL2_MULTQ_MAX_MASK                       GENMASK(11, 0)
#define QDMA_OFFSET_GLBL2_CHANNEL_PASID_CAP                 0x128
#define QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET                  0x12C
#define QDMA_OFFSET_GLBL2_SYSTEM_ID                         0x130
#define QDMA_OFFSET_GLBL2_MISC_CAP                          0x134

#define     QDMA_GLBL2_DEVICE_ID_MASK                       GENMASK(31, 28)
#define     QDMA_GLBL2_VIVADO_RELEASE_MASK                  GENMASK(27, 24)
#define     QDMA_GLBL2_VERSAL_IP_MASK                       GENMASK(23, 20)
#define     QDMA_GLBL2_RTL_VERSION_MASK                     GENMASK(19, 16)
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ0                      0x1B8
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ1                      0x1BC
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR0                     0x1C0
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR1                     0x1C4
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD0                     0x1C8
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD1                     0x1CC

/* used for VF bars identification */
#define QDMA_OFFSET_VF_USER_BAR_ID                          0x1018
#define QDMA_OFFSET_VF_CONFIG_BAR_ID                        0x1014

/* FLR programming */
#define QDMA_OFFSET_VF_REG_FLR_STATUS                       0x1100
#define QDMA_OFFSET_PF_REG_FLR_STATUS                       0x2500
#define     QDMA_FLR_STATUS_MASK                            0x1

/* VF qdma version */
#define QDMA_OFFSET_VF_VERSION                              0x1014
#define QDMA_OFFSET_PF_VERSION                              0x2414
#define     QDMA_GLBL2_VF_UNIQUE_ID_MASK                    GENMASK(31, 16)
#define     QDMA_GLBL2_VF_DEVICE_ID_MASK                    GENMASK(15, 12)
#define     QDMA_GLBL2_VF_VIVADO_RELEASE_MASK               GENMASK(11, 8)
#define     QDMA_GLBL2_VF_VERSAL_IP_MASK                    GENMASK(7, 4)
#define     QDMA_GLBL2_VF_RTL_VERSION_MASK                  GENMASK(3, 0)


/* ------------------------- QDMA_TRQ_SEL_QUEUE_PF (0x18000) ----------------*/

#define QDMA_OFFSET_DMAP_SEL_INT_CIDX                       0x18000
#define QDMA_OFFSET_DMAP_SEL_H2C_DSC_PIDX                   0x18004
#define QDMA_OFFSET_DMAP_SEL_C2H_DSC_PIDX                   0x18008
#define QDMA_OFFSET_DMAP_SEL_CMPT_CIDX                      0x1800C

#define QDMA_OFFSET_VF_DMAP_SEL_INT_CIDX                    0x3000
#define QDMA_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX                0x3004
#define QDMA_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX                0x3008
#define QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX                   0x300C

#define     QDMA_DMA_SEL_INT_SW_CIDX_MASK                   GENMASK(15, 0)
#define     QDMA_DMA_SEL_INT_RING_IDX_MASK                  GENMASK(23, 16)
#define     QDMA_DMA_SEL_DESC_PIDX_MASK                     GENMASK(15, 0)
#define     QDMA_DMA_SEL_IRQ_EN_MASK                        BIT(16)
#define     QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK                  BIT(28)
#define     QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK             BIT(27)
#define     QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK                GENMASK(26, 24)
#define     QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK                 GENMASK(23, 20)
#define     QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK              GENMASK(19, 16)
#define     QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK                GENMASK(15, 0)

/* ------------------------- Hardware Errors ------------------------------ */
#define TOTAL_LEAF_ERROR_AGGREGATORS                        7

#define QDMA_OFFSET_GLBL_ERR_INT                            0xB04
#define     QDMA_GLBL_ERR_FUNC_MASK                         GENMASK(7, 0)
#define     QDMA_GLBL_ERR_VEC_MASK                          GENMASK(22, 12)
#define     QDMA_GLBL_ERR_ARM_MASK                          BIT(24)

#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define     QDMA_GLBL_ERR_RAM_SBE_MASK                      BIT(0)
#define     QDMA_GLBL_ERR_RAM_DBE_MASK                      BIT(1)
#define     QDMA_GLBL_ERR_DSC_MASK                          BIT(2)
#define     QDMA_GLBL_ERR_TRQ_MASK                          BIT(3)
#define     QDMA_GLBL_ERR_ST_C2H_MASK                       BIT(8)
#define     QDMA_GLBL_ERR_ST_H2C_MASK                       BIT(11)

#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define     QDMA_C2H_ERR_MTY_MISMATCH_MASK                  BIT(0)
#define     QDMA_C2H_ERR_LEN_MISMATCH_MASK                  BIT(1)
#define     QDMA_C2H_ERR_QID_MISMATCH_MASK                  BIT(3)
#define     QDMA_C2H_ERR_DESC_RSP_ERR_MASK                  BIT(4)
#define     QDMA_C2H_ERR_ENG_WPL_DATA_PAR_ERR_MASK          BIT(6)
#define     QDMA_C2H_ERR_MSI_INT_FAIL_MASK                  BIT(7)
#define     QDMA_C2H_ERR_ERR_DESC_CNT_MASK                  BIT(9)
#define     QDMA_C2H_ERR_PORTID_CTXT_MISMATCH_MASK          BIT(10)
#define     QDMA_C2H_ERR_PORTID_BYP_IN_MISMATCH_MASK        BIT(11)
#define     QDMA_C2H_ERR_CMPT_INV_Q_ERR_MASK                BIT(12)
#define     QDMA_C2H_ERR_CMPT_QFULL_ERR_MASK                BIT(13)
#define     QDMA_C2H_ERR_CMPT_CIDX_ERR_MASK                 BIT(14)
#define     QDMA_C2H_ERR_CMPT_PRTY_ERR_MASK                 BIT(15)
#define     QDMA_C2H_ERR_ALL_MASK                           0xFEDB

#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define     QDMA_C2H_FATAL_ERR_MTY_MISMATCH_MASK            BIT(0)
#define     QDMA_C2H_FATAL_ERR_LEN_MISMATCH_MASK            BIT(1)
#define     QDMA_C2H_FATAL_ERR_QID_MISMATCH_MASK            BIT(3)
#define     QDMA_C2H_FATAL_ERR_TIMER_FIFO_RAM_RDBE_MASK     BIT(4)
#define     QDMA_C2H_FATAL_ERR_PFCH_II_RAM_RDBE_MASK        BIT(8)
#define     QDMA_C2H_FATAL_ERR_CMPT_CTXT_RAM_RDBE_MASK      BIT(9)
#define     QDMA_C2H_FATAL_ERR_PFCH_CTXT_RAM_RDBE_MASK      BIT(10)
#define     QDMA_C2H_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE_MASK  BIT(11)
#define     QDMA_C2H_FATAL_ERR_INT_CTXT_RAM_RDBE_MASK       BIT(12)
#define     QDMA_C2H_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE_MASK BIT(14)
#define     QDMA_C2H_FATAL_ERR_TUSER_FIFO_RAM_RDBE_MASK     BIT(15)
#define     QDMA_C2H_FATAL_ERR_QID_FIFO_RAM_RDBE_MASK       BIT(16)
#define     QDMA_C2H_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE_MASK   BIT(17)
#define     QDMA_C2H_FATAL_ERR_WPL_DATA_PAR_MASK            BIT(18)
#define     QDMA_C2H_FATAL_ERR_ALL_MASK                     0x7DF1B

#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define     QDMA_H2C_ERR_ZERO_LEN_DESC_MASK                 BIT(0)
#define     QDMA_H2C_ERR_CSI_MOP_MASK                       BIT(1)
#define     QDMA_H2C_ERR_NO_DMA_DSC_MASK                    BIT(2)
#define     QDMA_H2C_ERR_SBE_MASK                           BIT(3)
#define     QDMA_H2C_ERR_DBE_MASK                           BIT(4)
#define     QDMA_H2C_ERR_ALL_MASK                           0x1F

#define QDMA_OFFSET_GLBL_DSC_ERR_STAT                       0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MASK                       0x258
#define     QDMA_GLBL_DSC_ERR_POISON_MASK                   BIT(0)
#define     QDMA_GLBL_DSC_ERR_UR_CA_MASK                    BIT(1)
#define     QDMA_GLBL_DSC_ERR_PARAM_MASK                    BIT(2)
#define     QDMA_GLBL_DSC_ERR_ADDR_MASK                     BIT(3)
#define     QDMA_GLBL_DSC_ERR_TAG_MASK                      BIT(4)
#define     QDMA_GLBL_DSC_ERR_FLR_MASK                      BIT(5)
#define     QDMA_GLBL_DSC_ERR_TIMEOUT_MASK                  BIT(9)
#define     QDMA_GLBL_DSC_ERR_DAT_POISON_MASK               BIT(16)
#define     QDMA_GLBL_DSC_ERR_FLR_CANCEL_MASK               BIT(19)
#define     QDMA_GLBL_DSC_ERR_DMA_MASK                      BIT(20)
#define     QDMA_GLBL_DSC_ERR_DSC_MASK                      BIT(21)
#define     QDMA_GLBL_DSC_ERR_RQ_CANCEL_MASK                BIT(22)
#define     QDMA_GLBL_DSC_ERR_DBE_MASK                      BIT(23)
#define     QDMA_GLBL_DSC_ERR_SBE_MASK                      BIT(24)
#define     QDMA_GLBL_DSC_ERR_ALL_MASK                      0x1F9023F

#define QDMA_OFFSET_GLBL_TRQ_ERR_STAT                       0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MASK                       0x268
#define     QDMA_GLBL_TRQ_ERR_UNMAPPED_MASK                 BIT(0)
#define     QDMA_GLBL_TRQ_ERR_QID_RANGE_MASK                BIT(1)
#define     QDMA_GLBL_TRQ_ERR_VF_ACCESS_MASK                BIT(2)
#define     QDMA_GLBL_TRQ_ERR_TCP_TIMEOUT_MASK              BIT(3)
#define     QDMA_GLBL_TRQ_ERR_ALL_MASK                      0xF

#define QDMA_OFFSET_RAM_SBE_STAT                            0xF4
#define QDMA_OFFSET_RAM_SBE_MASK                            0xF0
#define     QDMA_SBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     QDMA_SBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     QDMA_SBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(9)
#define     QDMA_SBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(10)
#define     QDMA_SBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(11)
#define     QDMA_SBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(12)
#define     QDMA_SBE_ERR_FUNC_MAP_MASK                      BIT(13)
#define     QDMA_SBE_ERR_DSC_HW_CTXT_MASK                   BIT(14)
#define     QDMA_SBE_ERR_DSC_CRD_RCV_MASK                   BIT(15)
#define     QDMA_SBE_ERR_DSC_SW_CTXT_MASK                   BIT(16)
#define     QDMA_SBE_ERR_DSC_CPLI_MASK                      BIT(17)
#define     QDMA_SBE_ERR_DSC_CPLD_MASK                      BIT(18)
#define     QDMA_SBE_ERR_PASID_CTXT_RAM_MASK                BIT(19)
#define     QDMA_SBE_ERR_TIMER_FIFO_RAM_MASK                BIT(20)
#define     QDMA_SBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT(21)
#define     QDMA_SBE_ERR_QID_FIFO_RAM_MASK                  BIT(22)
#define     QDMA_SBE_ERR_TUSER_FIFO_RAM_MASK                BIT(23)
#define     QDMA_SBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT(24)
#define     QDMA_SBE_ERR_INT_QID2VEC_RAM_MASK               BIT(25)
#define     QDMA_SBE_ERR_INT_CTXT_RAM_MASK                  BIT(26)
#define     QDMA_SBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(27)
#define     QDMA_SBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(28)
#define     QDMA_SBE_ERR_WRB_CTXT_RAM_MASK                  BIT(29)
#define     QDMA_SBE_ERR_PFCH_LL_RAM_MASK                   BIT(30)
#define     QDMA_SBE_ERR_H2C_PEND_FIFO_MASK                 BIT(31)
#define     QDMA_SBE_ERR_ALL_MASK                           0xFFFFFF11

#define QDMA_OFFSET_RAM_DBE_STAT                            0xFC
#define QDMA_OFFSET_RAM_DBE_MASK                            0xF8
#define     QDMA_DBE_ERR_MI_H2C0_DAT_MASK                   BIT(0)
#define     QDMA_DBE_ERR_MI_C2H0_DAT_MASK                   BIT(4)
#define     QDMA_DBE_ERR_H2C_RD_BRG_DAT_MASK                BIT(9)
#define     QDMA_DBE_ERR_H2C_WR_BRG_DAT_MASK                BIT(10)
#define     QDMA_DBE_ERR_C2H_RD_BRG_DAT_MASK                BIT(11)
#define     QDMA_DBE_ERR_C2H_WR_BRG_DAT_MASK                BIT(12)
#define     QDMA_DBE_ERR_FUNC_MAP_MASK                      BIT(13)
#define     QDMA_DBE_ERR_DSC_HW_CTXT_MASK                   BIT(14)
#define     QDMA_DBE_ERR_DSC_CRD_RCV_MASK                   BIT(15)
#define     QDMA_DBE_ERR_DSC_SW_CTXT_MASK                   BIT(16)
#define     QDMA_DBE_ERR_DSC_CPLI_MASK                      BIT(17)
#define     QDMA_DBE_ERR_DSC_CPLD_MASK                      BIT(18)
#define     QDMA_DBE_ERR_PASID_CTXT_RAM_MASK                BIT(19)
#define     QDMA_DBE_ERR_TIMER_FIFO_RAM_MASK                BIT(20)
#define     QDMA_DBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT(21)
#define     QDMA_DBE_ERR_QID_FIFO_RAM_MASK                  BIT(22)
#define     QDMA_DBE_ERR_TUSER_FIFO_RAM_MASK                BIT(23)
#define     QDMA_DBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT(24)
#define     QDMA_DBE_ERR_INT_QID2VEC_RAM_MASK               BIT(25)
#define     QDMA_DBE_ERR_INT_CTXT_RAM_MASK                  BIT(26)
#define     QDMA_DBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT(27)
#define     QDMA_DBE_ERR_PFCH_CTXT_RAM_MASK                 BIT(28)
#define     QDMA_DBE_ERR_WRB_CTXT_RAM_MASK                  BIT(29)
#define     QDMA_DBE_ERR_PFCH_LL_RAM_MASK                   BIT(30)
#define     QDMA_DBE_ERR_H2C_PEND_FIFO_MASK                 BIT(31)
#define     QDMA_DBE_ERR_ALL_MASK                           0xFFFFFF11

#define QDMA_OFFSET_MBOX_BASE_VF                            0x1000
#define QDMA_OFFSET_MBOX_BASE_PF                            0x2400

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_SOFT_REG_H__ */
