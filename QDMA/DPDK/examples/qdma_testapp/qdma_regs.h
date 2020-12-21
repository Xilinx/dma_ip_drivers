/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2020 Xilinx, Inc. All rights reserved.
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

/** Target definations **/
#define QDMA_TRQ_SEL_GLBL	0x00000200
#define QDMA_TRQ_SEL_FMAP	0x00000400
#define QDMA_TRQ_SEL_IND	0x00000800
#define QDMA_TRQ_SEL_C2H	0x00000A00
#define QDMA_TRQ_SEL_H2H	0x00000E00
#define QDMA_TRQ_SEL_C2H_MM0	0x00001000
#define QDMA_TRQ_SEL_H2C_MM0	0x00001200
#define QDMA_TRQ_SEL_QUEUE_PF	0x00006400

#define QDMA_CONFIG_BLOCK_ID	0x1fd00000UL
/** Global registers **/
#define QDMA_GLBL_RING_SZ	0x04
#define QDMA_GLBL_SCRATCH	0x44
#define QDMA_GLBL_WB_ACC	0x50
#define QDMA_RING_SZ_MSK	0x0000ffff
#define QDMA_WB_ACC_MSK		0x00000007

/** Fmap registers **/
#define QID_BASE_MSK		(0x000007ff)
#define QID_MAX_MSK		(0x003ff800)
#define QID_MAX_SHIFT_B			(11)

/** Queue Indirect programming commands **/

#define QDMA_IND_Q_PRG_OFF	(0x4)

#define QDMA_CNTXT_CMD_RD	(2)

#define QDMA_CNTXT_SEL_DESC_SW_C2H	(0)
#define QDMA_CNTXT_SEL_DESC_SW_H2C	(1)
#define QDMA_CNTXT_SEL_DESC_HW_C2H	(2)
#define QDMA_CNTXT_SEL_DESC_HW_H2C	(3)
#define QDMA_CNTXT_SEL_DESC_CMPT	(6)
#define QDMA_CNTXT_SEL_PFTCH	(7)

#define QID_SHIFT_B		(7)
#define OP_CODE_SHIFT_B		(5)
#define CTXT_SEL_SHIFT_B	(1)
#define BUSY_BIT_MSK		(1)

#define WB_EN_SHIFT_B		(20)
#define MM_CHAN_SHIFT_B		(19)
#define MM_DESC_SZ_SHIFT_B	(17)
#define ST_H2C_DESC_SZ_SHIFT_B	(16)
#define DESC_RING_SZ_SHIFT_B	(12)
#define ST_H2C_DESC_SZ_SHIFT_B	(16)
#define MM_DESC_SZ_WB_SHIFT_B	(29)
#define C2H_WB_CTXT_V_SHIFT_B	(24)

/** C2H target registers **/
#define QDMA_C2H_CNT_TH_BASE	0x40
#define QDMA_C2H_BUF_SZ_BASE	0xB0

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


/** Queue Indirect programming registers **/
struct __attribute__ ((packed)) q_ind_prg
{
	uint32_t ctxt_data[8];
	uint32_t ctxt_mask[8];
	uint32_t ctxt_cmd;
};

union __attribute__ ((packed)) h2c_c2h_ctxt
{
	struct  __attribute__ ((packed))  ctxt_data
	{
		uint32_t data[5];
	} c_data;
	struct  __attribute__ ((packed)) ctxt_fields
	{
		uint16_t pidx;
		uint16_t irq_ack:1;
		uint16_t fnc_id:8;
		uint16_t rsv0:7;
		uint16_t qen:1;
		uint16_t fcrd_en:1;
		uint16_t wbi_chk:1;
		uint16_t wbi_acc_en:1;
		uint16_t at:1;
		uint16_t fetch_max:3;
		uint16_t rsv1:4;
		uint16_t rng_sz:4;
		uint16_t dsc_sz:2;
		uint16_t byp:1;
		uint16_t mm_chn:1;
		uint16_t wbk_en:1;
		uint16_t irq_en:1;
		uint16_t port_id:3;
		uint16_t irq_no_last:1;
		uint16_t err:2;
		uint16_t err_wb_sent:1;
		uint16_t irq_req:1;
		uint16_t mrkr_dis:1;
		uint16_t is_mm:1;
		uint64_t dsc_base;
		uint16_t int_vec:11;
		uint16_t int_aggr:1;
	} c_fields;
};

union __attribute__ ((packed)) c2h_cmpt_ctxt
{
	struct  __attribute__ ((packed))  c2h_cmpt_data
	{
		uint32_t data[5];
	} c_data;
	struct  __attribute__ ((packed)) c2h_cmpt_fields
	{
		uint32_t en_stat_desc:1;
		uint32_t en_int:1;
		uint32_t trig_mode:3;
		uint32_t fnc_id:12;
		uint32_t count_idx:4;
		uint32_t timer_idx:4;
		uint32_t int_st:2;
		uint32_t color:1;
		uint32_t size:4;
		uint32_t cmpt_dsc_base_l;
		uint32_t cmpt_dsc_base_h:26;
		uint32_t desc_sz:2;
		uint32_t pidx:16;
		uint32_t cidx:16;
		uint32_t valid:1;
		uint32_t err:2;
		uint32_t usr_trig_pend:1;
		uint32_t timer_run:1;
		uint32_t full_upd:1;
		uint32_t ovf_chk_dis:1;
		uint32_t at:1;
	} c_fields;
};

union __attribute__ ((packed)) h2c_c2h_hw_ctxt
{
	struct  __attribute__ ((packed))  hw_ctxt_data
	{
		uint32_t data[2];
	} c_data;
	struct  __attribute__ ((packed)) hw_ctxt_fields
	{
		uint32_t cidx:16;
		uint32_t crd_use:16;
		uint32_t rsvd0:8;
		uint32_t pnd:1;
		uint32_t idl_stp_b:1;
		uint32_t event_pend:1;
		uint32_t fetch_pend:4;
		uint32_t rsvd1:1;
	} c_fields;
};

union __attribute__ ((packed)) prefetch_ctxt
{
	struct __attribute__ ((packed)) pref_ctxt_data
	{
		uint32_t data[2];
	} c_data;

	struct __attribute__ ((packed)) pref_ctxt_fields
	{
		uint8_t bypass:1;
		uint8_t buf_sz_idx:4;
		uint8_t port_id:3;
		uint32_t rsvd:18;
		uint8_t err:1;
		uint8_t pfch_en:1;
		uint8_t pfch:1;
		uint16_t sw_crdt:16;
		uint8_t valid:1;
	} c_fields;
};

#define PIDX_MSK	(0)
#define Q_STATUS_MSK	(0)
#define Q_STATUS_EN_MSK (3)
#define Q_STATUS_RST_MSK (1)
#define WBI_CHK_MSK	(6)
#define WBI_ACC_EN_MSK	(7)
#define FUNC_ID_MSK	(8)
#define RING_SZ_MSK	(16)
#define DESC_SZ_MSK	(16)
