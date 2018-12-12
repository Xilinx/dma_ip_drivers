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

#ifndef __QDMA_REGS_H__
#define __QDMA_REGS_H__

#define QDMA_BAR_NUM		(6)

#define QDMA_GLOBAL_CSR_ARRAY_SZ        (16)

extern uint32_t g_ring_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
extern uint32_t g_c2h_cnt_th[QDMA_GLOBAL_CSR_ARRAY_SZ];
extern uint32_t g_c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ];
extern uint32_t g_c2h_timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ];

/** Target definations **/
#define QDMA_TRQ_SEL_GLBL	0x00000200
#define QDMA_TRQ_SEL_FMAP	0x00000400
#define QDMA_TRQ_SEL_IND	0x00000800
#define QDMA_TRQ_SEL_C2H	0x00000A00
#define QDMA_TRQ_SEL_H2C	0x00000E00
#define QDMA_TRQ_SEL_C2H_MM0	0x00001000
#define QDMA_TRQ_SEL_H2C_MM0	0x00001200
#define QDMA_TRQ_EXT		0x00002400
//#define QDMA_TRQ_SEL_QUEUE_PF	0x00001800 /* for old Bitstreams **/
#define QDMA_TRQ_SEL_QUEUE_PF	0x00018000
#define QDMA_TRQ_EXT_VF		0x00001000
#define QDMA_TRQ_SEL_QUEUE_VF	0x00003000

#define QDMA_CONFIG_BLOCK_ID	0x1fd30000UL
/** Global registers **/
#define QDMA_GLBL_RING_SZ	0x04
#define QDMA_GLBL_SCRATCH	0x44
#define QDMA_GLBL_DSC_CFG	0x50
#define QDMA_RING_SZ_MSK	0x0000ffff
#define QDMA_GLBL_DSC_CFG_WB_ACC_MSK	0x00000007
#define QDMA_GLBL_DSC_CFG_MAX_DSC_FTCH_MSK	0x00000038
#define QDMA_GLBL_DSC_CFG_WB_ACC_SHFT	0
#define QDMA_GLBL_DSC_CFG_MAX_DSC_FTCH_SHFT	3
#define QDMA_GLBL2_PF_BARLITE_INT      0x104
#define QDMA_GLBL2_PF_VF_BARLITE_INT   0x108
#define QDMA_GLBL2_PF_BARLITE_EXT      0x10c
#define QDMA_GLBL2_VF_BARLITE_EXT      0x1018
#define QDMA_PF_RTL_VER		       0x134
#define QDMA_VF_RTL_VER		       0x1014
#define	PF_RTL_VERSION_MASK                0xFF0000
#define	PF_RTL_VERSION_SHIFT               16
#define	PF_VIVADO_RELEASE_ID_MASK          0x0F000000
#define	PF_VIVADO_RELEASE_ID_SHIFT         24
#define	PF_EVEREST_IP_MASK                 0x10000000
#define	PF_EVEREST_IP_SHIFT                28
#define	VF_RTL_VERSION_MASK                0xFF
#define	VF_RTL_VERSION_SHIFT               0
#define	VF_VIVADO_RELEASE_ID_MASK          0x0F00
#define	VF_VIVADO_RELEASE_ID_SHIFT         8
#define	VF_EVEREST_IP_MASK                 0x1000
#define	VF_EVEREST_IP_SHIFT                12

/** Fmap registers **/
#define QID_BASE_MSK		(0x000007ff)
#define QID_MAX_MSK		(0x003ff800)
#define QID_MAX_SHIFT_B			(11)

/** Queue Indirect programming commands **/

#define QDMA_IND_Q_PRG_OFF	(0x4)

#define QDMA_CTXT_CMD_CLR	(0)
#define QDMA_CTXT_CMD_WR	(1)
#define QDMA_CTXT_CMD_RD	(2)
#define QDMA_CTXT_CMD_INV	(3)

#define MASK_0BIT            (0x0)
#define MASK_32BIT           (0xffffffffUL)

#define QDMA_CTXT_SEL_DESC_SW_C2H	(0)
#define QDMA_CTXT_SEL_DESC_SW_H2C	(1)
#define QDMA_CTXT_SEL_DESC_HW_C2H	(2)
#define QDMA_CTXT_SEL_DESC_HW_H2C	(3)
#define QDMA_CTXT_SEL_CR_C2H	(4)
#define QDMA_CTXT_SEL_CR_H2C	(5)
#define QDMA_CTXT_SEL_DESC_CMPT	(6)
#define QDMA_CTXT_SEL_PFTCH	(7)
#define QDMA_CTXT_SEL_FMAP	(12)

#define QID_SHIFT_B		(7)
#define OP_CODE_SHIFT_B		(5)
#define CTXT_SEL_SHIFT_B	(1)
#define BUSY_BIT_MSK		(1)

#define QDMA_CIDX_STEP      (0x10)
#define QDMA_PIDX_STEP      (0x10)

#define MM_CHAN_SHIFT_B		(19)
#define DESC_RING_SZ_SHIFT_B	(12)
#define ST_H2C_DESC_SZ_SHIFT_B	(16)
#define MM_DESC_SZ_WB_SHIFT_B	(29)
//#define C2H_WB_CTXT_V_SHIFT_B	(30)


/** C2H target registers **/
#define QDMA_C2H_CNT_TH_BASE	0x40
#define QDMA_C2H_BUF_SZ_BASE	0xB0
#define QDMA_C2H_TIMER_BASE		0x00

#define QDMA_C2H_PFCH_CACHE_DEPTH_OFFSET 0x1E0
#define QDMA_C2H_CMPT_COAL_BUF_DEPTH_OFFSET 0x1E4

#define QDMA_C2H_PFCH_CFG_OFFSET		0x108
#define QDMA_C2H_PFCH_CFG_STOP_THRESH_SHIFT		(0)
#define QDMA_C2H_PFCH_CFG_NUM_ENTRIES_PER_Q_SHIFT	(8)
#define QDMA_C2H_PFCH_CFG_MAX_Q_CNT_SHIFT			(16)
#define QDMA_C2H_PFCH_CFG_EVICTION_Q_CNT_SHIFT		(25)

#define QDMA_C2H_INT_TIMER_TICK_OFFSET  0x10C

#define QDMA_C2H_CMPT_COAL_CFG_OFFSET	0x150
#define QDMA_C2H_CMPT_COAL_TIMER_CNT_VAL_SHIFT	(2)
#define QDMA_C2H_CMPT_COAL_TIMER_TICK_VAL_SHIFT		(14)
#define QDMA_C2H_CMPT_COAL_MAX_BUF_SZ_SHIFT		(26)

/** H2C target registers **/
#define QDMA_H2C_DATA_THRESHOLD_OFFSET	0x24 //Base address is QDMA_TRQ_SEL_H2C

/** PF Queue index registers */
#define QDMA_H2C_PIDX_Q_OFF	(0x04)
#define QDMA_C2H_PIDX_Q_OFF	(0x08)
#define QDMA_SEL_CMPT_CIDX_Q_OFF (0x0c)


/** QDMA Target registers **/
#define QDMA_C2H_MM0_CONTROL	0x00000004
#define QDMA_H2C_MM0_CONTROL	0x00000004
#define QDMA_MM_CTRL_START	(1 << 0)

/** QDMA Descriptor definations **/
#define QDMA_DESC_SOP	0x1
#define QDMA_DESC_EOP	0x1
#define QDMA_DESC_VALID	0x1

/** Mailbox Register **/
#define QDMA_MBOX_FSR		0x00
#define QDMA_MBOX_FCMD		0x04
#define QDMA_MBOX_TRGT_FN	0x0c
#define QDMA_MBOX_ACK_BASE	0x20
#define QDMA_MBOX_IMSG		0x800
#define QDMA_MBOX_OMSG		0xc00

/** Mailbox commands **/
#define QDMA_MBOX_CMD_SEND	(1)
#define QDMA_MBOX_CMD_RECV	(2)

/* Driver visible Attribute Space 0x100 */
#define QDMA_REG_GLBL_PF_BARLITE_INT             0x104
#define		PF_BARLITE_INT_3_SHIFT               18
#define		PF_BARLITE_INT_3_MASK                0xFC0000
#define		PF_BARLITE_INT_2_SHIFT               12
#define		PF_BARLITE_INT_2_MASK                0x3F000
#define		PF_BARLITE_INT_1_SHIFT               6
#define		PF_BARLITE_INT_1_MASK                0xFC0
#define		PF_BARLITE_INT_0_SHIFT               0
#define		PF_BARLITE_INT_0_MASK                0x3F

#define QDMA_REG_GLBL_QMAX                       0x120
#define QDMA_REG_GLBL_MISC_CAP                   0x134
#define QDMA_REG_GLBL_MDMA_CHANNEL               0x118
#define     MDMA_CHANNEL_ST_C2H_ENABLED_SHIFT    16
#define     MDMA_CHANNEL_ST_C2H_ENABLED_MASK     0x10000
#define     MDMA_CHANNEL_ST_H2C_ENABLED_SHIFT    17
#define     MDMA_CHANNEL_ST_H2C_ENABLED_MASK     0x20000
#define     MDMA_CHANNEL_MM_C2H_ENABLED_SHIFT    8
#define     MDMA_CHANNEL_MM_C2H_ENABLED_MASK     0x100
#define     MDMA_CHANNEL_MM_H2C_ENABLED_SHIFT    0
#define     MDMA_CHANNEL_MM_H2C_ENABLED_MASK     0x1

/** Completion Descriptor config */
#define CMPT_STATUS_DESC_EN          (1)
#define CMPT_STATUS_DESC_EN_SHIFT    (27)
#define CMPT_TRIGGER_MODE_SHIFT      (24)
#define CMPT_TIMER_CNT_SHIFT         (20)
#define CMPT_THREHOLD_CNT_SHIFT      (16)

/** Prefetch Context config */
#define PREFETCH_EN                     (1)
#define PREFETCH_EN_SHIFT               (27)
#define PREFETCH_BYPASS_SHIFT           (0)
#define BUFF_SIZE_INDEX_SHIFT           (1)
#define VALID_CNTXT                     (1)
#define VALID_CNTXT_SHIFT               (13)

/** Completion Context config */
#define CMPT_CNTXT_EN_STAT_DESC          (1)
#define CMPT_CNTXT_EN_STAT_DESC_SHIFT    (0)
#define CMPT_CNTXT_COLOR_BIT             (1)
#define CMPT_CNTXT_COLOR_BIT_SHIFT       (27)
#define CMPT_CNTXT_TRIGGER_MODE_SHIFT    (2)
#define CMPT_CNTXT_TIMER_INDEX_SHIFT     (21)
#define CMPT_CNTXT_THRESHOLD_MODE_SHIFT  (17)
#define CMPT_CNTXT_FUNC_ID_SHIFT          (5)
#define CMPT_CNTXT_RING_SIZE_INDEX_SHIFT (28)
#define CMPT_CNTXT_DESC_SIZE_8B          (0)
#define CMPT_CNTXT_DESC_SIZE_16B         (1)
#define CMPT_CNTXT_DESC_SIZE_32B         (2)
#define CMPT_CNTXT_DESC_SIZE_64B         (3)
#define CMPT_CNTXT_DESC_SIZE_SHIFT       (90 - (2 * 32))
#define CMPT_CNTXT_CTXT_VALID            (1)
#define CMPT_CNTXT_OVF_CHK_DIS_SHIFT      (130 - (4 * 32))
#define CMPT_CNTXT_CTXT_VALID_SHIFT      (124 - (3 * 32))
#define CMPT_CNTX_FULL_UPDT		(1)
#define CMPT_CNTX_FULL_UPDT_SHIFT	(129 - (4 * 32))

/** SOFTWARE DESC CONTEXT */
#define SW_DESC_CNTXT_WB_EN                 (1)
#define SW_DESC_CNTXT_WB_EN_SHIFT_B         (52 - (1 * 32))
#define SW_DESC_CNTXT_8B_BYPASS_DMA	    (0)
#define SW_DESC_CNTXT_16B_BYPASS_DMA	    (1)
#define SW_DESC_CNTXT_32B_BYPASS_DMA	    (2)
#define SW_DESC_CNTXT_64B_BYPASS_DMA	    (3)
#define SW_DESC_CNTXT_MEMORY_MAP_DMA        (2)
#define SW_DESC_CNTXT_H2C_STREAM_DMA        (1)
#define SW_DESC_CNTXT_C2H_STREAM_DMA        (0)
#define SW_DESC_CNTXT_BYPASS_SHIFT	    (50 - (1 * 32))
#define SW_DESC_CNTXT_DESC_SZ_SHIFT         (48 - (1 * 32))
#define SW_DESC_CNTXT_FUNC_ID_SHIFT         (17)
#define SW_DESC_CNTXT_RING_SIZE_ID_SHIFT    (44 - (1 * 32))
#define SW_DESC_CNTXT_QUEUE_ENABLE          (1)
#define SW_DESC_CNTXT_QUEUE_EN_SHIFT        (32 - (1 * 32))
#define SW_DESC_CNTXT_FETCH_CREDIT_EN       (1)
#define SW_DESC_CNTXT_FETCH_CREDIT_EN_SHIFT (33 - (1 * 32))
#define SW_DESC_CNTXT_WBI_CHK               (1)
#define SW_DESC_CNTXT_WBI_CHK_SHIFT         (34 - (1 * 32))
#define SW_DESC_CNTXT_WBI_INTERVAL          (1)
#define SW_DESC_CNTXT_WBI_INTERVAL_SHIFT    (35 - (1 * 32))
#define SW_DESC_CNTXT_IS_MM                 (1)
#define SW_DESC_CNTXT_IS_MM_SHIFT           (63 - (1 * 32))

/** QDMA Global registers **/
struct __attribute__ ((packed)) qdma_global_regs
{
	volatile uint32_t ring_sz[16];
	volatile uint32_t status[16];
	volatile uint32_t config[16];
	volatile uint32_t wb_acc;
	volatile uint32_t pf_scratch_reg;
};

#define NUM_CONTEXT_REGS	8
/** Queue Indirect programming registers **/
struct __attribute__ ((packed)) queue_ind_prg
{
	volatile uint32_t ctxt_data[NUM_CONTEXT_REGS];
	volatile uint32_t ctxt_mask[NUM_CONTEXT_REGS];
	volatile uint32_t ctxt_cmd;
};

/** MM Write-back status structure **/
struct __attribute__ ((packed)) wb_status
{
	volatile uint16_t	pidx; /** in C2H WB **/
	volatile uint16_t	cidx; /** Consumer-index **/
	uint32_t	rsvd2; /** Reserved. **/
};

/** ST C2H Descriptor **/
struct __attribute__ ((packed)) qdma_c2h_desc
{
	volatile uint64_t	dst_addr;
};

#define S_H2C_DESC_F_SOP		1
#define S_H2C_DESC_F_EOP		2

/* pld_len and flags members are part of custom descriptor format needed
 * by example design for ST loopback and desc bypass
 */

/** ST H2C Descriptor **/
struct __attribute__ ((packed)) qdma_h2c_desc
{
	volatile uint16_t	cdh_flags;
	volatile uint16_t	pld_len;
	volatile uint16_t	len;
	volatile uint16_t	flags;
	volatile uint64_t	src_addr;
};

/** MM Descriptor **/
struct __attribute__ ((packed)) qdma_mm_desc
{
	volatile uint64_t	src_addr;
	volatile uint64_t	len:28;
	volatile uint64_t	dv:1;
	volatile uint64_t	sop:1;
	volatile uint64_t	eop:1;
	volatile uint64_t	rsvd:33;
	volatile uint64_t	dst_addr;
	volatile uint64_t	rsvd2;

};

/** MBOX STATUS **/
struct __attribute__ ((packed)) mbox_fsr
{
	volatile uint8_t	imsg_stat:1;
	volatile uint8_t	omsg_stat:1;
	volatile uint8_t	ack_stat:2;
	volatile uint16_t	curr_src_fn:8;
	volatile uint32_t	rsvd:20;
};

struct __attribute__ ((packed)) mbox_ack_stat
{
	volatile uint32_t ack[8];
};

struct __attribute__ ((packed)) mbox_imsg
{
	volatile uint32_t imsg[32];
};

struct __attribute__ ((packed)) mbox_omsg
{
	volatile uint32_t omsg[32];
};

/** C2H/H2C Descriptor context structure **/

#define PIDX_MSK	(0)
#define Q_STATUS_MSK	(0)
#define Q_STATUS_EN_MSK (3)
#define Q_STATUS_RST_MSK (1)
#define WBI_CHK_MSK	(6)
#define WBI_ACC_EN_MSK	(7)
#define FUNC_ID_MSK	(8)
#define RING_SZ_MSK	(16)
#define DESC_SZ_MSK	(16)

#endif /* ifndef __QDMA_REGS_H__ */
