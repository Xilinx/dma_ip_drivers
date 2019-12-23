/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef QDMA_CPM_REG_H_
#define QDMA_CPM_REG_H_

#include "qdma_reg.h"
#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------- QDMA_TRQ_SEL_IND (0x00800) ----------------*/
#define QDMA_CPM_OFFSET_IND_CTXT_MASK				0x814
#define QDMA_CPM_OFFSET_IND_CTXT_CMD				0x824


/* ------------------------ indirect register context fields -----------*/

#define QDMA_CPM_IND_CTXT_DATA_NUM_REGS				4
/** QDMA_IND_REG_SEL_FMAP */
#define QDMA_CPM_FMAP_CTXT_W0_QID_MAX_MASK			GENMASK(22, 11)

/** QDMA_IND_REG_SEL_SW_C2H */
/** QDMA_IND_REG_SEL_SW_H2C */
#define QDMA_CPM_SW_CTXT_W1_FUNC_ID_MASK			GENMASK(11, 4)

/** QDMA_IND_REG_SEL_CMPT */
#define QDMA_CPM_COMPL_CTXT_BADDR_GET_H_MASK		GENMASK_ULL(63, 42)
#define QDMA_CPM_COMPL_CTXT_BADDR_GET_M_MASK		GENMASK_ULL(41, 10)
#define QDMA_CPM_COMPL_CTXT_BADDR_GET_L_MASK		GENMASK_ULL(9, 6)

#define QDMA_CPM_COMPL_CTXT_W3_FULL_UPDT_MASK                   BIT(29)
#define QDMA_CPM_COMPL_CTXT_W3_TMR_RUN_MASK                     BIT(28)
#define QDMA_CPM_COMPL_CTXT_W3_USR_TRG_PND_MASK                 BIT(27)
#define QDMA_CPM_COMPL_CTXT_W3_ERR_MASK                         GENMASK(26, 25)
#define QDMA_CPM_COMPL_CTXT_W3_VALID_MASK                       BIT(24)
#define QDMA_CPM_COMPL_CTXT_W3_CIDX_MASK                        GENMASK(23, 8)
#define QDMA_CPM_COMPL_CTXT_W3_PIDX_H_MASK                      GENMASK(7, 0)
#define QDMA_CPM_COMPL_CTXT_W2_PIDX_L_MASK                      GENMASK(31, 24)
#define QDMA_CPM_COMPL_CTXT_W2_DESC_SIZE_MASK                   GENMASK(23, 22)
#define QDMA_CPM_COMPL_CTXT_W2_BADDR_64_H_MASK                  GENMASK(21, 0)
#define QDMA_CPM_COMPL_CTXT_W1_BADDR_64_M_MASK                  GENMASK(31, 0)
#define QDMA_CPM_COMPL_CTXT_W0_BADDR_64_L_MASK                  GENMASK(31, 28)
#define QDMA_CPM_COMPL_CTXT_W0_RING_SZ_MASK                     GENMASK(27, 24)
#define QDMA_CPM_COMPL_CTXT_W0_COLOR_MASK                       BIT(23)
#define QDMA_CPM_COMPL_CTXT_W0_INT_ST_MASK                      GENMASK(22, 21)
#define QDMA_CPM_COMPL_CTXT_W0_TIMER_IDX_MASK                   GENMASK(20, 17)
#define QDMA_CPM_COMPL_CTXT_W0_COUNTER_IDX_MASK                 GENMASK(16, 13)

/** QDMA_IND_REG_SEL_HW_C2H */
/** QDMA_IND_REG_SEL_HW_H2C */
#define QDMA_CPM_HW_CTXT_W1_FETCH_PEND_MASK                     BIT(10)

/** QDMA_IND_REG_SEL_CR_C2H */
/** QDMA_IND_REG_SEL_CR_H2C */
#define QDMA_CPM_CR_CTXT_W0_CREDT_MASK                          GENMASK(15, 0)

/** QDMA_IND_REG_SEL_INTR */
#define QDMA_CPM_INTR_CTXT_BADDR_GET_H_MASK		GENMASK_ULL(63, 35)
#define QDMA_CPM_INTR_CTXT_BADDR_GET_L_MASK		GENMASK_ULL(34, 12)

#define QDMA_CPM_INTR_CTXT_W2_PIDX_MASK			GENMASK(11, 0)
#define QDMA_CPM_INTR_CTXT_W1_PAGE_SIZE_MASK	GENMASK(31, 29)
#define QDMA_CPM_INTR_CTXT_W1_BADDR_64_MASK		GENMASK(28, 0)
#define QDMA_CPM_INTR_CTXT_W0_BADDR_64_MASK		GENMASK(31, 9)
#define QDMA_CPM_INTR_CTXT_W0_COLOR_MASK		BIT(8)
#define QDMA_CPM_INTR_CTXT_W0_INT_ST_MASK		BIT(7)
#define QDMA_CPM_INTR_CTXT_W0_VEC_ID_MASK		GENMASK(5, 1)


/*---------------------------------------------------------------------*/
/*
 * Function registers
 */

#define		QDMA_CPM_REG_TRQ_SEL_FMAP_BASE		0x400
#define		QDMA_CPM_REG_TRQ_SEL_FMAP_STEP		4
#define		QDMA_CPM_REG_TRQ_SEL_FMAP_COUNT		256

#define		SEL_FMAP_QID_BASE_SHIFT			0
#define		SEL_FMAP_QID_BASE_MASK			0x7FFU
#define		SEL_FMAP_QID_MAX_SHIFT			11
#define		SEL_FMAP_QID_MAX_MASK			0xFFFU

#define		QDMA_CPM_REG_C2H_QID2VEC_MAP_QID	0xa80
#define		QDMA_CPM_QID2VEC_C2H_VECTOR		GENMASK(7, 0)
#define		QDMA_CPM_QID2VEC_C2H_COAL_EN		BIT(8)
#define		QDMA_CPM_QID2VEC_H2C_VECTOR		GENMASK(16, 9)
#define		QDMA_CPM_QID2VEC_H2C_COAL_EN		BIT(17)

#define QDMA_CPM_REG_C2H_QID2VEC_MAP			0xa84
/*---------------------------------------------------------------------*/




/* ------------------------- QDMA_TRQ_SEL_QUEUE_PF (0x18000) ----------------*/

#define QDMA_CPM_OFFSET_DMAP_SEL_INT_CIDX                       0x6400
#define QDMA_CPM_OFFSET_DMAP_SEL_H2C_DSC_PIDX                   0x6404
#define QDMA_CPM_OFFSET_DMAP_SEL_C2H_DSC_PIDX                   0x6408
#define QDMA_CPM_OFFSET_DMAP_SEL_CMPT_CIDX                      0x640C




#ifdef __cplusplus
}
#endif

#endif /* QDMA_CPM_REG_H_ */
