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

#include "qdma_mbox_protocol.h"

/** mailbox function status */
#define MBOX_FN_STATUS			0x0
/** shift value for mailbox function status in msg */
#define		S_MBOX_FN_STATUS_IN_MSG	0
/** mask value for mailbox function status in msg*/
#define		M_MBOX_FN_STATUS_IN_MSG	0x1
/** face value for mailbox function status in msg */
#define		F_MBOX_FN_STATUS_IN_MSG	0x1

/** shift value for out msg */
#define		S_MBOX_FN_STATUS_OUT_MSG	1
/** mask value for out msg */
#define		M_MBOX_FN_STATUS_OUT_MSG	0x1
/** face value for out msg */
#define		F_MBOX_FN_STATUS_OUT_MSG	(1 << S_MBOX_FN_STATUS_OUT_MSG)
/** shift value for status ack */
#define		S_MBOX_FN_STATUS_ACK	2	/* PF only, ack status */
/** mask value for status ack */
#define		M_MBOX_FN_STATUS_ACK	0x1
/** face value for status ack */
#define		F_MBOX_FN_STATUS_ACK	(1 << S_MBOX_FN_STATUS_ACK)
/** shift value for status src */
#define		S_MBOX_FN_STATUS_SRC	4	/* PF only, source func.*/
/** mask value for status src */
#define		M_MBOX_FN_STATUS_SRC	0xFFF
/** face value for status src */
#define		G_MBOX_FN_STATUS_SRC(x)	\
		(((x) >> S_MBOX_FN_STATUS_SRC) & M_MBOX_FN_STATUS_SRC)
/** face value for mailbox function status */
#define MBOX_FN_STATUS_MASK \
		(F_MBOX_FN_STATUS_IN_MSG | \
		 F_MBOX_FN_STATUS_OUT_MSG | \
		 F_MBOX_FN_STATUS_ACK)

/** mailbox function commands register */
#define MBOX_FN_CMD			0x4
/** shift value for send command */
#define		S_MBOX_FN_CMD_SND	0
/** mask value for send command */
#define		M_MBOX_FN_CMD_SND	0x1
/** face value for send command */
#define		F_MBOX_FN_CMD_SND	(1 << S_MBOX_FN_CMD_SND)
/** shift value for receive command */
#define		S_MBOX_FN_CMD_RCV	1
/** mask value for receive command */
#define		M_MBOX_FN_CMD_RCV	0x1
/** face value for receive command */
#define		F_MBOX_FN_CMD_RCV	(1 << S_MBOX_FN_CMD_RCV)
/** shift value for vf reset */
#define		S_MBOX_FN_CMD_VF_RESET	3	/* TBD PF only: reset VF */
/** mask value for vf reset */
#define		M_MBOX_FN_CMD_VF_RESET	0x1
/** mailbox isr vector register */
#define MBOX_ISR_VEC			0x8
/** shift value for isr vector */
#define		S_MBOX_ISR_VEC		0
/** mask value for isr vector */
#define		M_MBOX_ISR_VEC		0x1F
/** face value for isr vector */
#define		V_MBOX_ISR_VEC(x)	((x) & M_MBOX_ISR_VEC)
/** mailbox FN target register */
#define MBOX_FN_TARGET			0xC
/** shift value for FN target id */
#define		S_MBOX_FN_TARGET_ID	0
/** mask value for FN target id */
#define		M_MBOX_FN_TARGET_ID	0xFFF
/** face value for FN target id */
#define		V_MBOX_FN_TARGET_ID(x)	((x) & M_MBOX_FN_TARGET_ID)
/** mailbox isr enable register */
#define MBOX_ISR_EN			0x10
/** shift value for isr enable */
#define		S_MBOX_ISR_EN		0
/** mask value for isr enable */
#define		M_MBOX_ISR_EN		0x1
/** face value for isr enable */
#define		F_MBOX_ISR_EN		0x1
/** pf acknowledge base */
#define MBOX_PF_ACK_BASE		0x20
/** pf acknowledge step */
#define MBOX_PF_ACK_STEP		4
/** pf acknowledge count */
#define MBOX_PF_ACK_COUNT		8
/** mailbox incoming msg base */
#define MBOX_IN_MSG_BASE		0x800
/** mailbox outgoing msg base */
#define MBOX_OUT_MSG_BASE		0xc00
/** mailbox msg step */
#define MBOX_MSG_STEP			4
/** mailbox register max */
#define MBOX_MSG_REG_MAX		32

/**
 * enum mbox_msg_op - mailbox messages opcode
 */
#define MBOX_MSG_OP_RSP_OFFSET	0x80
enum mbox_msg_op {
	/** @MBOX_OP_BYE: vf offline, response not required*/
	MBOX_OP_VF_BYE,
	/** @MBOX_OP_HELLO: vf online */
	MBOX_OP_HELLO,
	/** @: FMAP programming request */
	MBOX_OP_FMAP,
	/** @MBOX_OP_CSR: global CSR registers request */
	MBOX_OP_CSR,
	/** @MBOX_OP_QREQ: request queues */
	MBOX_OP_QREQ,
	/** @MBOX_OP_QADD: notify of queue addition */
	MBOX_OP_QNOTIFY_ADD,
	/** @MBOX_OP_QNOTIFY_DEL: notify of queue deletion */
	MBOX_OP_QNOTIFY_DEL,
	/** @MBOX_OP_QACTIVE_CNT: get active q count */
	MBOX_OP_GET_QACTIVE_CNT,
	/** @MBOX_OP_QCTXT_WRT: queue context write */
	MBOX_OP_QCTXT_WRT,
	/** @MBOX_OP_QCTXT_RD: queue context read */
	MBOX_OP_QCTXT_RD,
	/** @MBOX_OP_QCTXT_CLR: queue context clear */
	MBOX_OP_QCTXT_CLR,
	/** @MBOX_OP_QCTXT_INV: queue context invalidate */
	MBOX_OP_QCTXT_INV,
	/** @MBOX_OP_INTR_CTXT_WRT: interrupt context write */
	MBOX_OP_INTR_CTXT_WRT,
	/** @MBOX_OP_INTR_CTXT_RD: interrupt context read */
	MBOX_OP_INTR_CTXT_RD,
	/** @MBOX_OP_INTR_CTXT_CLR: interrupt context clear */
	MBOX_OP_INTR_CTXT_CLR,
	/** @MBOX_OP_INTR_CTXT_INV: interrupt context invalidate */
	MBOX_OP_INTR_CTXT_INV,
	/** @MBOX_OP_RESET_PREPARE: PF to VF message for VF reset*/
	MBOX_OP_RESET_PREPARE,
	/** @MBOX_OP_RESET_DONE: PF reset done */
	MBOX_OP_RESET_DONE,
	/** @MBOX_OP_REG_LIST_READ: Read the register list */
	MBOX_OP_REG_LIST_READ,
	/** @MBOX_OP_PF_BYE: pf offline, response required */
	MBOX_OP_PF_BYE,
	/** @MBOX_OP_PF_RESET_VF_BYE: VF reset BYE, response required*/
	MBOX_OP_PF_RESET_VF_BYE,

	/** @MBOX_OP_HELLO_RESP: response to @MBOX_OP_HELLO */
	MBOX_OP_HELLO_RESP = 0x81,
	/** @MBOX_OP_FMAP_RESP: response to @MBOX_OP_FMAP */
	MBOX_OP_FMAP_RESP,
	/** @MBOX_OP_CSR_RESP: response to @MBOX_OP_CSR */
	MBOX_OP_CSR_RESP,
	/** @MBOX_OP_QREQ_RESP: response to @MBOX_OP_QREQ */
	MBOX_OP_QREQ_RESP,
	/** @MBOX_OP_QADD: notify of queue addition */
	MBOX_OP_QNOTIFY_ADD_RESP,
	/** @MBOX_OP_QNOTIFY_DEL: notify of queue deletion */
	MBOX_OP_QNOTIFY_DEL_RESP,
	/** @MBOX_OP_QACTIVE_CNT_RESP: get active q count */
	MBOX_OP_GET_QACTIVE_CNT_RESP,
	/** @MBOX_OP_QCTXT_WRT_RESP: response to @MBOX_OP_QCTXT_WRT */
	MBOX_OP_QCTXT_WRT_RESP,
	/** @MBOX_OP_QCTXT_RD_RESP: response to @MBOX_OP_QCTXT_RD */
	MBOX_OP_QCTXT_RD_RESP,
	/** @MBOX_OP_QCTXT_CLR_RESP: response to @MBOX_OP_QCTXT_CLR */
	MBOX_OP_QCTXT_CLR_RESP,
	/** @MBOX_OP_QCTXT_INV_RESP: response to @MBOX_OP_QCTXT_INV */
	MBOX_OP_QCTXT_INV_RESP,
	/** @MBOX_OP_INTR_CTXT_WRT_RESP: response to @MBOX_OP_INTR_CTXT_WRT */
	MBOX_OP_INTR_CTXT_WRT_RESP,
	/** @MBOX_OP_INTR_CTXT_RD_RESP: response to @MBOX_OP_INTR_CTXT_RD */
	MBOX_OP_INTR_CTXT_RD_RESP,
	/** @MBOX_OP_INTR_CTXT_CLR_RESP: response to @MBOX_OP_INTR_CTXT_CLR */
	MBOX_OP_INTR_CTXT_CLR_RESP,
	/** @MBOX_OP_INTR_CTXT_INV_RESP: response to @MBOX_OP_INTR_CTXT_INV */
	MBOX_OP_INTR_CTXT_INV_RESP,
	/** @MBOX_OP_RESET_PREPARE_RESP: response to @MBOX_OP_RESET_PREPARE */
	MBOX_OP_RESET_PREPARE_RESP,
	/** @MBOX_OP_RESET_DONE_RESP: response to @MBOX_OP_PF_VF_RESET */
	MBOX_OP_RESET_DONE_RESP,
	/** @MBOX_OP_REG_LIST_READ_RESP: response to @MBOX_OP_REG_LIST_READ */
	MBOX_OP_REG_LIST_READ_RESP,
	/** @MBOX_OP_PF_BYE_RESP: response to @MBOX_OP_PF_BYE */
	MBOX_OP_PF_BYE_RESP,
	/** @MBOX_OP_PF_RESET_VF_BYE_RESP:
	 * response to @MBOX_OP_PF_RESET_VF_BYE
	 */
	MBOX_OP_PF_RESET_VF_BYE_RESP,
	/** @MBOX_OP_MAX: total mbox opcodes*/
	MBOX_OP_MAX
};

/**
 * struct mbox_msg_hdr - mailbox message header
 */
struct mbox_msg_hdr {
	/** @op: opcode */
	uint8_t op;
	/** @status: execution status */
	char status;
	/** @src_func_id: src function */
	uint16_t src_func_id;
	/** @dst_func_id: dst function */
	uint16_t dst_func_id;
};

/**
 * struct mbox_msg_fmap - FMAP programming command
 */
struct mbox_msg_hello {
	/** @hdr: mailbox message header */
	struct mbox_msg_hdr hdr;
	/** @qbase: start queue number in the queue range */
	uint32_t qbase;
	/** @qmax: max queue number in the queue range(0-2k) */
	uint32_t qmax;
	/** @dev_cap: device capability */
	struct qdma_dev_attributes dev_cap;
	/** @dma_device_index: dma_device_index */
	uint32_t dma_device_index;
};

/**
 * struct mbox_msg_active_qcnt - get active queue count command
 */
struct mbox_msg_active_qcnt {
	/** @hdr: mailbox message header */
	struct mbox_msg_hdr hdr;
	/** @h2c_queues: number of h2c queues */
	uint32_t h2c_queues;
	/** @c2h_queues: number of c2h queues */
	uint32_t c2h_queues;
	/** @cmpt_queues: number of cmpt queues */
	uint32_t cmpt_queues;
};

/**
 * struct mbox_msg_fmap - FMAP programming command
 */
struct mbox_msg_fmap {
	/** @hdr: mailbox message header */
	struct mbox_msg_hdr hdr;
	/** @qbase: start queue number in the queue range */
	int qbase;
	/** @qmax: max queue number in the queue range(0-2k) */
	uint32_t qmax;
};

/**
 * struct mbox_msg_csr - mailbox csr reading message
 */
struct mbox_msg_csr {
	/** @hdr - mailbox message header */
	struct mbox_msg_hdr hdr;
	/** @csr_info: csr info data strucutre */
	struct qdma_csr_info csr_info;
};

/**
 * struct mbox_msg_q_nitfy - queue add/del notify message
 */
struct mbox_msg_q_nitfy {
	/** @hdr - mailbox message header */
	struct mbox_msg_hdr hdr;
	/** @qid_hw: queue ID */
	uint16_t qid_hw;
	/** @q_type: type of q */
	enum qdma_dev_q_type q_type;
};

/**
 * @struct - mbox_msg_qctxt
 * @brief queue context mailbox message header
 */
struct mbox_msg_qctxt {
	/** @hdr: mailbox message header*/
	struct mbox_msg_hdr hdr;
	/** @qid_hw: queue ID */
	uint16_t qid_hw;
	/** @st: streaming mode */
	uint8_t st:1;
	/** @c2h: c2h direction */
	uint8_t c2h:1;
	/** @cmpt_ctxt_type: completion context type */
	enum mbox_cmpt_ctxt_type cmpt_ctxt_type:2;
	/** @rsvd: reserved */
	uint8_t rsvd:4;
	/** union compiled_message - complete hw configuration */
	union {
		/** @descq_conf: mailbox message for queue context write*/
		struct mbox_descq_conf descq_conf;
		/** @descq_ctxt: mailbox message for queue context read*/
		struct qdma_descq_context descq_ctxt;
	};
};

/**
 * @struct - mbox_intr_ctxt
 * @brief queue context mailbox message header
 */
struct mbox_intr_ctxt {
	/** @hdr: mailbox message header*/
	struct mbox_msg_hdr hdr;
	/** interrupt context mailbox message */
	struct mbox_msg_intr_ctxt ctxt;
};

/**
 * @struct - mbox_read_reg_list
 * @brief read register mailbox message header
 */
struct mbox_read_reg_list {
	/** @hdr: mailbox message header*/
	struct mbox_msg_hdr hdr;
	/** @group_num: reg group to read */
	uint16_t group_num;
	/** @num_regs: number of registers to read */
	uint16_t num_regs;
	/** @reg_list: register list */
	struct qdma_reg_data reg_list[QDMA_MAX_REGISTER_DUMP];
};

union qdma_mbox_txrx {
		/** mailbox message header*/
		struct mbox_msg_hdr hdr;
		/** hello mailbox message */
		struct mbox_msg_hello hello;
		/** fmap mailbox message */
		struct mbox_msg_fmap fmap;
		/** interrupt context mailbox message */
		struct mbox_intr_ctxt intr_ctxt;
		/** queue context mailbox message*/
		struct mbox_msg_qctxt qctxt;
		/** global csr mailbox message */
		struct mbox_msg_csr csr;
		/** acive q count */
		struct mbox_msg_active_qcnt qcnt;
		/** q add/del notify message */
		struct mbox_msg_q_nitfy q_notify;
		/** reg list mailbox message */
		struct mbox_read_reg_list reg_read_list;
		/** buffer to hold raw data between pf and vf */
		uint32_t raw[MBOX_MSG_REG_MAX];
};


static inline uint32_t get_mbox_offset(void *dev_hndl, uint8_t is_vf)
{
	uint32_t mbox_base;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);
	mbox_base = (is_vf) ?
		hw->mbox_base_vf : hw->mbox_base_pf;

	return mbox_base;
}

static inline void mbox_pf_hw_clear_func_ack(void *dev_hndl, uint16_t func_id)
{
	int idx = func_id / 32; /* bitmask, uint32_t reg */
	int bit = func_id % 32;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, 0);

	/* clear the function's ack status */
	qdma_reg_write(dev_hndl,
			mbox_base + MBOX_PF_ACK_BASE + idx * MBOX_PF_ACK_STEP,
			(1 << bit));
}

static void qdma_mbox_memcpy(void *to, void *from, uint8_t size)
{
	uint8_t i;
	uint8_t *_to = (uint8_t *)to;
	uint8_t *_from = (uint8_t *)from;

	for (i = 0; i < size; i++)
		_to[i] = _from[i];
}

static void qdma_mbox_memset(void *to, uint8_t val, uint8_t size)
{
	uint8_t i;
	uint8_t *_to = (uint8_t *)to;

	for (i = 0; i < size; i++)
		_to[i] = val;
}

static int get_ring_idx(void *dev_hndl, uint16_t ring_sz, uint16_t *rng_idx)
{
	uint32_t rng_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int i, rv;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);
	rv = hw->qdma_global_csr_conf(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, rng_sz,
			QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ);

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (ring_sz == (rng_sz[i] - 1)) {
			*rng_idx = i;
			return QDMA_SUCCESS;
		}
	}

	qdma_log_error("%s: Ring size not found, err:%d\n",
				   __func__, -QDMA_ERR_MBOX_INV_RINGSZ);
	return -QDMA_ERR_MBOX_INV_RINGSZ;
}

static int get_buf_idx(void *dev_hndl,  uint16_t buf_sz, uint16_t *buf_idx)
{
	uint32_t c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int i, rv;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	rv = hw->qdma_global_csr_conf(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, c2h_buf_sz,
			QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ);
	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (c2h_buf_sz[i] == buf_sz) {
			*buf_idx = i;
			return QDMA_SUCCESS;
		}
	}

	qdma_log_error("%s: Buf index not found, err:%d\n",
				   __func__, -QDMA_ERR_MBOX_INV_BUFSZ);
	return -QDMA_ERR_MBOX_INV_BUFSZ;
}

static int get_cntr_idx(void *dev_hndl, uint8_t cntr_val, uint8_t *cntr_idx)
{
	uint32_t cntr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int i, rv;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	rv = hw->qdma_global_csr_conf(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, cntr_th,
			QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (cntr_th[i] == cntr_val) {
			*cntr_idx = i;
			return QDMA_SUCCESS;
		}
	}

	qdma_log_error("%s: Counter val not found, err:%d\n",
				   __func__, -QDMA_ERR_MBOX_INV_CNTR_TH);
	return -QDMA_ERR_MBOX_INV_CNTR_TH;
}

static int get_tmr_idx(void *dev_hndl, uint8_t tmr_val, uint8_t *tmr_idx)
{
	uint32_t tmr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int i, rv;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	rv = hw->qdma_global_csr_conf(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, tmr_th,
			QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (tmr_th[i] == tmr_val) {
			*tmr_idx = i;
			return QDMA_SUCCESS;
		}
	}

	qdma_log_error("%s: Timer val not found, err:%d\n",
				   __func__, -QDMA_ERR_MBOX_INV_TMR_TH);
	return -QDMA_ERR_MBOX_INV_TMR_TH;
}

static int mbox_compose_sw_context(void *dev_hndl,
				   struct mbox_msg_qctxt *qctxt,
				   struct qdma_descq_sw_ctxt *sw_ctxt)
{
	uint16_t rng_idx = 0;
	int rv = QDMA_SUCCESS;

	if (!qctxt || !sw_ctxt) {
		qdma_log_error("%s: qctxt=%p sw_ctxt=%p, err:%d\n",
						__func__,
						qctxt, sw_ctxt,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	rv = get_ring_idx(dev_hndl, qctxt->descq_conf.ringsz, &rng_idx);
	if (rv < 0) {
		qdma_log_error("%s: failed to get ring index, err:%d\n",
						__func__, rv);
		return rv;
	}
	/* compose sw context */
	sw_ctxt->vec = qctxt->descq_conf.intr_id;
	sw_ctxt->intr_aggr = qctxt->descq_conf.intr_aggr;

	sw_ctxt->ring_bs_addr = qctxt->descq_conf.ring_bs_addr;
	sw_ctxt->wbi_chk = qctxt->descq_conf.wbi_chk;
	sw_ctxt->wbi_intvl_en = qctxt->descq_conf.wbi_intvl_en;
	sw_ctxt->rngsz_idx = rng_idx;
	sw_ctxt->bypass = qctxt->descq_conf.en_bypass;
	sw_ctxt->wbk_en = qctxt->descq_conf.wbk_en;
	sw_ctxt->irq_en = qctxt->descq_conf.irq_en;
	sw_ctxt->is_mm = ~qctxt->st;
	sw_ctxt->mm_chn = 0;
	sw_ctxt->qen = 1;
	sw_ctxt->frcd_en = qctxt->descq_conf.forced_en;

	sw_ctxt->desc_sz = qctxt->descq_conf.desc_sz;

	/* pidx = 0; irq_ack = 0 */
	sw_ctxt->fnc_id = qctxt->descq_conf.func_id;
	sw_ctxt->irq_arm =  qctxt->descq_conf.irq_arm;

	if (qctxt->st && qctxt->c2h) {
		sw_ctxt->irq_en = 0;
		sw_ctxt->irq_arm = 0;
		sw_ctxt->wbk_en = 0;
		sw_ctxt->wbi_chk = 0;
	}

	return QDMA_SUCCESS;
}

static int mbox_compose_prefetch_context(void *dev_hndl,
					 struct mbox_msg_qctxt *qctxt,
				 struct qdma_descq_prefetch_ctxt *pfetch_ctxt)
{
	uint16_t buf_idx = 0;
	int rv = QDMA_SUCCESS;

	if (!qctxt || !pfetch_ctxt) {
		qdma_log_error("%s: qctxt=%p pfetch_ctxt=%p, err:%d\n",
					   __func__,
					   qctxt,
					   pfetch_ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	rv = get_buf_idx(dev_hndl, qctxt->descq_conf.bufsz, &buf_idx);
	if (rv < 0) {
		qdma_log_error("%s: failed to get buf index, err:%d\n",
					   __func__, -QDMA_ERR_INV_PARAM);
		return rv;
	}
	/* prefetch context */
	pfetch_ctxt->valid = 1;
	pfetch_ctxt->bypass = qctxt->descq_conf.en_bypass_prefetch;
	pfetch_ctxt->bufsz_idx = buf_idx;
	pfetch_ctxt->pfch_en = qctxt->descq_conf.pfch_en;

	return QDMA_SUCCESS;
}


static int mbox_compose_cmpt_context(void *dev_hndl,
				     struct mbox_msg_qctxt *qctxt,
				     struct qdma_descq_cmpt_ctxt *cmpt_ctxt)
{
	uint16_t rng_idx = 0;
	uint8_t cntr_idx = 0, tmr_idx = 0;
	int rv = QDMA_SUCCESS;

	if (!qctxt || !cmpt_ctxt) {
		qdma_log_error("%s: qctxt=%p cmpt_ctxt=%p, err:%d\n",
					   __func__, qctxt, cmpt_ctxt,
					   -QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	rv = get_cntr_idx(dev_hndl, qctxt->descq_conf.cnt_thres, &cntr_idx);
	if (rv < 0)
		return rv;
	rv = get_tmr_idx(dev_hndl, qctxt->descq_conf.timer_thres, &tmr_idx);
	if (rv < 0)
		return rv;
	rv = get_ring_idx(dev_hndl, qctxt->descq_conf.cmpt_ringsz, &rng_idx);
	if (rv < 0)
		return rv;
	/* writeback context */

	cmpt_ctxt->bs_addr = qctxt->descq_conf.cmpt_ring_bs_addr;
	cmpt_ctxt->en_stat_desc = qctxt->descq_conf.cmpl_stat_en;
	cmpt_ctxt->en_int = qctxt->descq_conf.cmpt_int_en;
	cmpt_ctxt->trig_mode = qctxt->descq_conf.triggermode;
	cmpt_ctxt->fnc_id = qctxt->descq_conf.func_id;
	cmpt_ctxt->timer_idx = tmr_idx;
	cmpt_ctxt->counter_idx = cntr_idx;
	cmpt_ctxt->color = 1;
	cmpt_ctxt->ringsz_idx = rng_idx;

	cmpt_ctxt->desc_sz = qctxt->descq_conf.cmpt_desc_sz;

	cmpt_ctxt->valid = 1;

	cmpt_ctxt->ovf_chk_dis = qctxt->descq_conf.dis_overflow_check;
	cmpt_ctxt->vec = qctxt->descq_conf.intr_id;
	cmpt_ctxt->int_aggr = qctxt->descq_conf.intr_aggr;

	return QDMA_SUCCESS;
}

static int mbox_clear_queue_contexts(void *dev_hndl, uint8_t dma_device_index,
			      uint16_t func_id, uint16_t qid_hw, uint8_t st,
			      uint8_t c2h,
			      enum mbox_cmpt_ctxt_type cmpt_ctxt_type)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	if (cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
					    NULL, QDMA_HW_ACCESS_CLEAR);
		if (rv < 0) {
			qdma_log_error("%s: clear cmpt ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}
	} else {
		rv = qdma_dev_qinfo_get(dma_device_index,
				func_id, &qbase, &qmax);
		if (rv < 0) {
			qdma_log_error("%s: failed to get qinfo, err:%d\n",
					__func__, rv);
			return rv;
		}

		q_range = qdma_dev_is_queue_in_range(dma_device_index,
						func_id, qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE) {
			qdma_log_error("%s: q_range invalid, err:%d\n",
						__func__, rv);
			return rv;
		}

		rv = hw->qdma_sw_ctx_conf(dev_hndl, c2h, qid_hw,
					  NULL, QDMA_HW_ACCESS_CLEAR);
		if (rv < 0) {
			qdma_log_error("%s: clear sw_ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		rv = hw->qdma_hw_ctx_conf(dev_hndl, c2h, qid_hw, NULL,
					       QDMA_HW_ACCESS_CLEAR);
		if (rv < 0) {
			qdma_log_error("%s: clear hw_ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		rv = hw->qdma_credit_ctx_conf(dev_hndl, c2h, qid_hw, NULL,
					       QDMA_HW_ACCESS_CLEAR);
		if (rv < 0) {
			qdma_log_error("%s: clear cr_ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		if (st && c2h) {
			rv = hw->qdma_pfetch_ctx_conf(dev_hndl, qid_hw,
						       NULL,
						       QDMA_HW_ACCESS_CLEAR);
			if (rv < 0) {
				qdma_log_error("%s:clear pfetch ctxt, err:%d\n",
						__func__, rv);
				return rv;
			}
		}

		if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
						     NULL,
						     QDMA_HW_ACCESS_CLEAR);
			if (rv < 0) {
				qdma_log_error("%s: clear cmpt ctxt, err:%d\n",
							__func__, rv);
				return rv;
			}
		}
	}

	return QDMA_SUCCESS;
}

static int mbox_invalidate_queue_contexts(void *dev_hndl,
		uint8_t dma_device_index, uint16_t func_id,
		uint16_t qid_hw, uint8_t st,
		uint8_t c2h, enum mbox_cmpt_ctxt_type cmpt_ctxt_type)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	if (cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw, NULL,
					    QDMA_HW_ACCESS_INVALIDATE);
		if (rv < 0) {
			qdma_log_error("%s: inv cmpt ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}
	} else {
		rv = qdma_dev_qinfo_get(dma_device_index, func_id,
				&qbase, &qmax);
		if (rv < 0) {
			qdma_log_error("%s: failed to get qinfo, err:%d\n",
						__func__, rv);
			return rv;
		}

		q_range = qdma_dev_is_queue_in_range(dma_device_index,
						func_id, qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE) {
			qdma_log_error("%s: Invalid qrange, err:%d\n",
							__func__, rv);
			return rv;
		}

		rv = hw->qdma_sw_ctx_conf(dev_hndl, c2h, qid_hw,
					  NULL, QDMA_HW_ACCESS_INVALIDATE);
		if (rv < 0) {
			qdma_log_error("%s: inv sw ctxt, err:%d\n",
							__func__, rv);
			return rv;
		}

		rv = hw->qdma_hw_ctx_conf(dev_hndl, c2h, qid_hw, NULL,
				QDMA_HW_ACCESS_INVALIDATE);
		if (rv < 0) {
			qdma_log_error("%s: clear hw_ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		rv = hw->qdma_credit_ctx_conf(dev_hndl, c2h, qid_hw, NULL,
				QDMA_HW_ACCESS_INVALIDATE);
		if (rv < 0) {
			qdma_log_error("%s: clear cr_ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		if (st && c2h) {
			rv = hw->qdma_pfetch_ctx_conf(dev_hndl, qid_hw,
						NULL,
						QDMA_HW_ACCESS_INVALIDATE);
			if (rv < 0) {
				qdma_log_error("%s: inv pfetch ctxt, err:%d\n",
						__func__, rv);
				return rv;
			}
		}

		if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
						NULL,
						QDMA_HW_ACCESS_INVALIDATE);
			if (rv < 0) {
				qdma_log_error("%s: inv cmpt ctxt, err:%d\n",
						__func__, rv);
				return rv;
			}
		}
	}

	return QDMA_SUCCESS;
}

static int mbox_write_queue_contexts(void *dev_hndl, uint8_t dma_device_index,
				     struct mbox_msg_qctxt *qctxt)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;
	struct qdma_descq_context descq_ctxt;
	uint16_t qid_hw = qctxt->qid_hw;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	rv = qdma_dev_qinfo_get(dma_device_index, qctxt->descq_conf.func_id,
				&qbase, &qmax);
	if (rv < 0)
		return rv;

	q_range = qdma_dev_is_queue_in_range(dma_device_index,
					     qctxt->descq_conf.func_id,
					     qctxt->qid_hw);
	if (q_range != QDMA_DEV_Q_IN_RANGE) {
		qdma_log_error("%s: Invalid qrange, err:%d\n",
							__func__, rv);
		return rv;
	}

	qdma_mbox_memset(&descq_ctxt, 0, sizeof(struct qdma_descq_context));

	if (qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = mbox_compose_cmpt_context(dev_hndl, qctxt,
			       &descq_ctxt.cmpt_ctxt);
		if (rv < 0)
			return rv;

		rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
					    NULL, QDMA_HW_ACCESS_CLEAR);
		if (rv < 0) {
			qdma_log_error("%s: clear cmpt ctxt, err:%d\n",
								__func__, rv);
			return rv;
		}

		rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
			     &descq_ctxt.cmpt_ctxt, QDMA_HW_ACCESS_WRITE);
		if (rv < 0) {
			qdma_log_error("%s: write cmpt ctxt, err:%d\n",
								__func__, rv);
			return rv;
		}

	} else {
		rv = mbox_compose_sw_context(dev_hndl, qctxt,
				&descq_ctxt.sw_ctxt);
		if (rv < 0)
			return rv;

		if (qctxt->st && qctxt->c2h) {
			rv = mbox_compose_prefetch_context(dev_hndl, qctxt,
						&descq_ctxt.pfetch_ctxt);
			if (rv < 0)
				return rv;
		}

		if ((qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = mbox_compose_cmpt_context(dev_hndl, qctxt,
							&descq_ctxt.cmpt_ctxt);
			if (rv < 0)
				return rv;
		}

		rv = mbox_clear_queue_contexts(dev_hndl, dma_device_index,
					qctxt->descq_conf.func_id,
					qctxt->qid_hw,
					qctxt->st,
					qctxt->c2h,
					qctxt->cmpt_ctxt_type);
		if (rv < 0)
			return rv;
		rv = hw->qdma_sw_ctx_conf(dev_hndl, qctxt->c2h, qid_hw,
					   &descq_ctxt.sw_ctxt,
					   QDMA_HW_ACCESS_WRITE);
		if (rv < 0) {
			qdma_log_error("%s: write sw ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}

		if (qctxt->st && qctxt->c2h) {
			rv = hw->qdma_pfetch_ctx_conf(dev_hndl, qid_hw,
						       &descq_ctxt.pfetch_ctxt,
						       QDMA_HW_ACCESS_WRITE);
			if (rv < 0) {
				qdma_log_error("%s:write pfetch ctxt, err:%d\n",
						__func__, rv);
				return rv;
			}
		}

		if ((qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = hw->qdma_cmpt_ctx_conf(dev_hndl, qid_hw,
						     &descq_ctxt.cmpt_ctxt,
						     QDMA_HW_ACCESS_WRITE);
			if (rv < 0) {
				qdma_log_error("%s: write cmpt ctxt, err:%d\n",
						__func__, rv);
				return rv;
			}
		}
	}
	return QDMA_SUCCESS;
}

static int mbox_read_queue_contexts(void *dev_hndl, uint16_t qid_hw,
			uint8_t st, uint8_t c2h,
			enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
			struct qdma_descq_context *ctxt)
{
	int rv;
	struct qdma_hw_access *hw = NULL;

	qdma_get_hw_access(dev_hndl, &hw);

	rv = hw->qdma_sw_ctx_conf(dev_hndl, c2h, qid_hw, &ctxt->sw_ctxt,
				  QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: read sw ctxt, err:%d\n",
					__func__, rv);
		return rv;
	}

	rv = hw->qdma_hw_ctx_conf(dev_hndl, c2h, qid_hw, &ctxt->hw_ctxt,
				  QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: read hw ctxt, err:%d\n",
					__func__, rv);
		return rv;
	}

	rv = hw->qdma_credit_ctx_conf(dev_hndl, c2h, qid_hw, &ctxt->cr_ctxt,
				      QDMA_HW_ACCESS_READ);
	if (rv < 0) {
		qdma_log_error("%s: read credit ctxt, err:%d\n",
					__func__, rv);
		return rv;
	}

	if (st && c2h) {
		rv = hw->qdma_pfetch_ctx_conf(dev_hndl,
					qid_hw, &ctxt->pfetch_ctxt,
					QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error("%s: read pfetch ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}
	}

	if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
	    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
		rv = hw->qdma_cmpt_ctx_conf(dev_hndl,
					qid_hw, &ctxt->cmpt_ctxt,
					QDMA_HW_ACCESS_READ);
		if (rv < 0) {
			qdma_log_error("%s: read cmpt ctxt, err:%d\n",
						__func__, rv);
			return rv;
		}
	}

	return QDMA_SUCCESS;
}

int qdma_mbox_pf_rcv_msg_handler(void *dev_hndl, uint8_t dma_device_index,
				 uint16_t func_id, uint32_t *rcv_msg,
				 uint32_t *resp_msg)
{
	union qdma_mbox_txrx *rcv =  (union qdma_mbox_txrx *)rcv_msg;
	union qdma_mbox_txrx *resp =  (union qdma_mbox_txrx *)resp_msg;
	struct mbox_msg_hdr *hdr = &rcv->hdr;
	struct qdma_hw_access *hw = NULL;
	int rv = QDMA_SUCCESS;
	int ret = 0;

	if (!rcv) {
		qdma_log_error("%s: rcv_msg=%p failure:%d\n",
						__func__, rcv,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}
	qdma_get_hw_access(dev_hndl, &hw);

	switch (rcv->hdr.op) {
	case MBOX_OP_VF_BYE:
	{
		struct qdma_fmap_cfg fmap;

		fmap.qbase = 0;
		fmap.qmax = 0;
		rv = hw->qdma_fmap_conf(dev_hndl, hdr->src_func_id, &fmap,
					QDMA_HW_ACCESS_WRITE);

		qdma_dev_entry_destroy(dma_device_index, hdr->src_func_id);

		ret = QDMA_MBOX_VF_OFFLINE;
	}
	break;
	case MBOX_OP_PF_RESET_VF_BYE:
	{
		struct qdma_fmap_cfg fmap;

		fmap.qbase = 0;
		fmap.qmax = 0;
		rv = hw->qdma_fmap_conf(dev_hndl, hdr->src_func_id, &fmap,
					QDMA_HW_ACCESS_WRITE);

		qdma_dev_entry_destroy(dma_device_index, hdr->src_func_id);

		ret = QDMA_MBOX_VF_RESET_BYE;
	}
	break;
	case MBOX_OP_HELLO:
	{
		struct mbox_msg_fmap *fmap = &rcv->fmap;
		struct qdma_fmap_cfg fmap_cfg;
		struct mbox_msg_hello *rsp_hello = &resp->hello;

		rv = qdma_dev_qinfo_get(dma_device_index, hdr->src_func_id,
				&fmap->qbase, &fmap->qmax);
		if (rv < 0)
			rv = qdma_dev_entry_create(dma_device_index,
					hdr->src_func_id);

		if (!rv) {
			rsp_hello->qbase = fmap->qbase;
			rsp_hello->qmax = fmap->qmax;
			rsp_hello->dma_device_index = dma_device_index;
			hw->qdma_get_device_attributes(dev_hndl,
						       &rsp_hello->dev_cap);
		}
		qdma_mbox_memset(&fmap_cfg, 0,
				 sizeof(struct qdma_fmap_cfg));
		hw->qdma_fmap_conf(dev_hndl, hdr->src_func_id, &fmap_cfg,
				   QDMA_HW_ACCESS_WRITE);

		ret = QDMA_MBOX_VF_ONLINE;
	}
	break;
	case MBOX_OP_FMAP:
	{
		struct mbox_msg_fmap *fmap = &rcv->fmap;
		struct qdma_fmap_cfg fmap_cfg;

		fmap_cfg.qbase = fmap->qbase;
		fmap_cfg.qmax = fmap->qmax;

		rv = hw->qdma_fmap_conf(dev_hndl, hdr->src_func_id,
				     &fmap_cfg, QDMA_HW_ACCESS_WRITE);
		if (rv < 0) {
			qdma_log_error("%s: failed to write fmap, err:%d\n",
						__func__, rv);
			return rv;
		}
	}
	break;
	case MBOX_OP_CSR:
	{
		struct mbox_msg_csr *rsp_csr = &resp->csr;
		struct qdma_dev_attributes dev_cap;

		uint32_t ringsz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t bufsz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t tmr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t cntr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		int i;

		rv = hw->qdma_global_csr_conf(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, ringsz,
				QDMA_CSR_RING_SZ, QDMA_HW_ACCESS_READ);
		if (rv < 0)
			goto exit_func;

		hw->qdma_get_device_attributes(dev_hndl, &dev_cap);

		if (dev_cap.st_en) {
			rv = hw->qdma_global_csr_conf(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, bufsz,
				QDMA_CSR_BUF_SZ, QDMA_HW_ACCESS_READ);
			if (rv < 0 &&
				(rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
				goto exit_func;
		}

		if (dev_cap.st_en || dev_cap.mm_cmpt_en) {
			rv = hw->qdma_global_csr_conf(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, tmr_th,
				QDMA_CSR_TIMER_CNT, QDMA_HW_ACCESS_READ);
			if (rv < 0 &&
				(rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
				goto exit_func;

			rv = hw->qdma_global_csr_conf(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, cntr_th,
				QDMA_CSR_CNT_TH, QDMA_HW_ACCESS_READ);
			if (rv < 0 &&
				(rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
				goto exit_func;
		}

		for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
			rsp_csr->csr_info.ringsz[i] = ringsz[i] &
					0xFFFF;
			if (!rv) {
				rsp_csr->csr_info.bufsz[i] = bufsz[i] & 0xFFFF;
				rsp_csr->csr_info.timer_cnt[i] = tmr_th[i] &
						0xFF;
				rsp_csr->csr_info.cnt_thres[i] = cntr_th[i] &
						0xFF;
			}
		}

		if (rv == -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED)
			rv = QDMA_SUCCESS;
	}
	break;
	case MBOX_OP_QREQ:
	{
		struct mbox_msg_fmap *fmap = &rcv->fmap;

		rv = qdma_dev_update(dma_device_index,
					  hdr->src_func_id,
					  fmap->qmax, &fmap->qbase);
		if (rv == 0)
			rv = qdma_dev_qinfo_get(dma_device_index,
						hdr->src_func_id,
						&resp->fmap.qbase,
						&resp->fmap.qmax);
		if (rv < 0)
			rv = -QDMA_ERR_MBOX_NUM_QUEUES;
		else {
			struct qdma_fmap_cfg fmap_cfg;

			qdma_mbox_memset(&fmap_cfg, 0,
					 sizeof(struct qdma_fmap_cfg));
			hw->qdma_fmap_conf(dev_hndl, hdr->src_func_id,
					&fmap_cfg, QDMA_HW_ACCESS_WRITE);
		}
	}
	break;
	case MBOX_OP_QNOTIFY_ADD:
	{
		struct mbox_msg_q_nitfy *q_notify = &rcv->q_notify;
		enum qdma_dev_q_range q_range;

		q_range = qdma_dev_is_queue_in_range(
				dma_device_index,
				q_notify->hdr.src_func_id,
				q_notify->qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			rv = -QDMA_ERR_MBOX_INV_QID;
		else
			rv = qdma_dev_increment_active_queue(
					dma_device_index,
					q_notify->hdr.src_func_id,
					q_notify->q_type);
	}
	break;
	case MBOX_OP_QNOTIFY_DEL:
	{
		struct mbox_msg_q_nitfy *q_notify = &rcv->q_notify;
		enum qdma_dev_q_range q_range;

		q_range = qdma_dev_is_queue_in_range(
				dma_device_index,
				q_notify->hdr.src_func_id,
				q_notify->qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			rv = -QDMA_ERR_MBOX_INV_QID;
		else
			rv = qdma_dev_decrement_active_queue(
					dma_device_index,
					q_notify->hdr.src_func_id,
					q_notify->q_type);
	}
	break;
	case MBOX_OP_GET_QACTIVE_CNT:
	{
		rv = qdma_get_device_active_queue_count(
				dma_device_index,
				rcv->hdr.src_func_id,
				QDMA_DEV_Q_TYPE_H2C);

		resp->qcnt.h2c_queues = rv;

		rv = qdma_get_device_active_queue_count(
				dma_device_index,
				rcv->hdr.src_func_id,
				QDMA_DEV_Q_TYPE_C2H);

		resp->qcnt.c2h_queues = rv;

		rv = qdma_get_device_active_queue_count(
				dma_device_index,
				rcv->hdr.src_func_id,
				QDMA_DEV_Q_TYPE_CMPT);

		resp->qcnt.cmpt_queues = rv;
	}
	break;
	case MBOX_OP_INTR_CTXT_WRT:
	{
		struct mbox_msg_intr_ctxt *ictxt = &rcv->intr_ctxt.ctxt;
		struct qdma_indirect_intr_ctxt *ctxt;
		uint8_t i;
		uint32_t ring_index;

		for (i = 0; i < ictxt->num_rings; i++) {
			ring_index = ictxt->ring_index_list[i];

			ctxt = &ictxt->ictxt[i];
			rv = hw->qdma_indirect_intr_ctx_conf(dev_hndl,
						      ring_index,
						      NULL,
						      QDMA_HW_ACCESS_CLEAR);
			if (rv < 0)
				resp->hdr.status = rv;
			rv = hw->qdma_indirect_intr_ctx_conf(dev_hndl,
						      ring_index, ctxt,
						      QDMA_HW_ACCESS_WRITE);
			if (rv < 0)
				resp->hdr.status = rv;
		}
	}
	break;
	case MBOX_OP_INTR_CTXT_RD:
	{
		struct mbox_msg_intr_ctxt *rcv_ictxt = &rcv->intr_ctxt.ctxt;
		struct mbox_msg_intr_ctxt *rsp_ictxt = &resp->intr_ctxt.ctxt;
		uint8_t i;
		uint32_t ring_index;

		for (i = 0; i < rcv_ictxt->num_rings; i++) {
			ring_index = rcv_ictxt->ring_index_list[i];

			rv = hw->qdma_indirect_intr_ctx_conf(dev_hndl,
						      ring_index,
						      &rsp_ictxt->ictxt[i],
						      QDMA_HW_ACCESS_READ);
			if (rv < 0)
				resp->hdr.status = rv;

		}
	}
	break;
	case MBOX_OP_INTR_CTXT_CLR:
	{
		int i;
		struct mbox_msg_intr_ctxt *ictxt = &rcv->intr_ctxt.ctxt;

		for (i = 0; i < ictxt->num_rings; i++) {
			rv = hw->qdma_indirect_intr_ctx_conf(
					dev_hndl,
					ictxt->ring_index_list[i],
					NULL, QDMA_HW_ACCESS_CLEAR);
			if (rv < 0)
				resp->hdr.status = rv;
		}
	}
	break;
	case MBOX_OP_INTR_CTXT_INV:
	{
		struct mbox_msg_intr_ctxt *ictxt = &rcv->intr_ctxt.ctxt;
		int i;

		for (i = 0; i < ictxt->num_rings; i++) {
			rv = hw->qdma_indirect_intr_ctx_conf(
					dev_hndl,
					ictxt->ring_index_list[i],
					NULL, QDMA_HW_ACCESS_INVALIDATE);
			if (rv < 0)
				resp->hdr.status = rv;
		}
	}
	break;
	case MBOX_OP_QCTXT_INV:
	{
		struct mbox_msg_qctxt *qctxt = &rcv->qctxt;

		rv = mbox_invalidate_queue_contexts(dev_hndl,
							dma_device_index,
							hdr->src_func_id,
							qctxt->qid_hw,
							qctxt->st,
							qctxt->c2h,
							qctxt->cmpt_ctxt_type);
	}
	break;
	case MBOX_OP_QCTXT_CLR:
	{
		struct mbox_msg_qctxt *qctxt = &rcv->qctxt;

		rv = mbox_clear_queue_contexts(dev_hndl,
						dma_device_index,
						hdr->src_func_id,
						qctxt->qid_hw,
						qctxt->st,
						qctxt->c2h,
						qctxt->cmpt_ctxt_type);
	}
	break;
	case MBOX_OP_QCTXT_RD:
	{
		struct mbox_msg_qctxt *qctxt = &rcv->qctxt;

		rv = mbox_read_queue_contexts(dev_hndl, qctxt->qid_hw,
						qctxt->st,
						qctxt->c2h,
						qctxt->cmpt_ctxt_type,
						&resp->qctxt.descq_ctxt);
	}
	break;
	case MBOX_OP_QCTXT_WRT:
	{
		struct mbox_msg_qctxt *qctxt = &rcv->qctxt;

		qctxt->descq_conf.func_id = hdr->src_func_id;
		rv = mbox_write_queue_contexts(dev_hndl,
				dma_device_index, qctxt);
	}
	break;
	case MBOX_OP_RESET_PREPARE_RESP:
		return QDMA_MBOX_VF_RESET;
	case MBOX_OP_RESET_DONE_RESP:
		return QDMA_MBOX_PF_RESET_DONE;
	case MBOX_OP_REG_LIST_READ:
	{
		struct mbox_read_reg_list *rcv_read_reg_list =
						&rcv->reg_read_list;
		struct mbox_read_reg_list *rsp_read_reg_list =
						&resp->reg_read_list;

		rv = hw->qdma_read_reg_list((void *)dev_hndl, 1,
				 rcv_read_reg_list->group_num,
				&rsp_read_reg_list->num_regs,
				rsp_read_reg_list->reg_list);

		if (rv < 0 || rsp_read_reg_list->num_regs == 0) {
			rv = -QDMA_ERR_MBOX_REG_READ_FAILED;
			goto exit_func;
		}

	}
	break;
	case MBOX_OP_PF_BYE_RESP:
		return QDMA_MBOX_PF_BYE;
	default:
		qdma_log_error("%s: op=%d invalid, err:%d\n",
						__func__,
						rcv->hdr.op,
						-QDMA_ERR_MBOX_INV_MSG);
		return -QDMA_ERR_MBOX_INV_MSG;
	break;
	}

exit_func:
	resp->hdr.op = rcv->hdr.op + MBOX_MSG_OP_RSP_OFFSET;
	resp->hdr.dst_func_id = rcv->hdr.src_func_id;
	resp->hdr.src_func_id = func_id;

	resp->hdr.status = rv;

	return ret;
}

int qmda_mbox_compose_vf_online(uint16_t func_id,
				uint16_t qmax, int *qbase, uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_HELLO;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = (uint32_t)*qbase;
	msg->fmap.qmax = qmax;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_offline(uint16_t func_id,
				 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_VF_BYE;
	msg->hdr.src_func_id = func_id;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_reset_offline(uint16_t func_id,
				 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_PF_RESET_VF_BYE;
	msg->hdr.src_func_id = func_id;

	return QDMA_SUCCESS;
}



int qdma_mbox_compose_vf_qreq(uint16_t func_id,
			      uint16_t qmax, int qbase, uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QREQ;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = qbase;
	msg->fmap.qmax = qmax;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_notify_qadd(uint16_t func_id,
				     uint16_t qid_hw,
				     enum qdma_dev_q_type q_type,
				     uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QNOTIFY_ADD;
	msg->hdr.src_func_id = func_id;
	msg->q_notify.qid_hw = qid_hw;
	msg->q_notify.q_type = q_type;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_get_device_active_qcnt(uint16_t func_id,
		uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_GET_QACTIVE_CNT;
	msg->hdr.src_func_id = func_id;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_notify_qdel(uint16_t func_id,
				     uint16_t qid_hw,
				     enum qdma_dev_q_type q_type,
				    uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QNOTIFY_DEL;
	msg->hdr.src_func_id = func_id;
	msg->q_notify.qid_hw = qid_hw;
	msg->q_notify.q_type = q_type;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_fmap_prog(uint16_t func_id,
				   uint16_t qmax, int qbase,
				   uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
					__func__, raw_data,
					-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_FMAP;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = (uint32_t)qbase;
	msg->fmap.qmax = qmax;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_qctxt_write(uint16_t func_id,
			uint16_t qid_hw, uint8_t st, uint8_t c2h,
			enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
			struct mbox_descq_conf *descq_conf,
			uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_WRT;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	qdma_mbox_memcpy(&msg->qctxt.descq_conf, descq_conf,
	       sizeof(struct mbox_descq_conf));

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_qctxt_read(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_RD;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_qctxt_invalidate(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_INV;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_qctxt_clear(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_CLR;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_csr_read(uint16_t func_id,
			       uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_CSR;
	msg->hdr.src_func_id = func_id;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_reg_read(uint16_t func_id,
					uint16_t group_num,
					uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_REG_LIST_READ;
	msg->hdr.src_func_id = func_id;
	msg->reg_read_list.group_num = group_num;

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_intr_ctxt_write(uint16_t func_id,
					 struct mbox_msg_intr_ctxt *intr_ctxt,
					 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_WRT;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_intr_ctxt_read(uint16_t func_id,
					struct mbox_msg_intr_ctxt *intr_ctxt,
					uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_RD;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_intr_ctxt_clear(uint16_t func_id,
					 struct mbox_msg_intr_ctxt *intr_ctxt,
					 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_CLR;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return QDMA_SUCCESS;
}

int qdma_mbox_compose_vf_intr_ctxt_invalidate(uint16_t func_id,
				      struct mbox_msg_intr_ctxt *intr_ctxt,
				      uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data) {
		qdma_log_error("%s: raw_data=%p, err:%d\n",
						__func__, raw_data,
						-QDMA_ERR_INV_PARAM);
		return -QDMA_ERR_INV_PARAM;
	}

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_INV;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return QDMA_SUCCESS;
}

uint8_t qdma_mbox_is_msg_response(uint32_t *send_data, uint32_t *rcv_data)
{
	union qdma_mbox_txrx *tx_msg = (union qdma_mbox_txrx *)send_data;
	union qdma_mbox_txrx *rx_msg = (union qdma_mbox_txrx *)rcv_data;

	return ((tx_msg->hdr.op + MBOX_MSG_OP_RSP_OFFSET) == rx_msg->hdr.op) ?
			1 : 0;
}

int qdma_mbox_vf_response_status(uint32_t *rcv_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	return msg->hdr.status;
}

uint8_t qdma_mbox_vf_func_id_get(uint32_t *rcv_data, uint8_t is_vf)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;
	uint16_t func_id;

	if (is_vf)
		func_id = msg->hdr.dst_func_id;
	else
		func_id = msg->hdr.src_func_id;

	return func_id;
}

int qdma_mbox_vf_active_queues_get(uint32_t *rcv_data,
		enum qdma_dev_q_type q_type)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;
	int queues = 0;

	if (q_type == QDMA_DEV_Q_TYPE_H2C)
		queues = msg->qcnt.h2c_queues;

	if (q_type == QDMA_DEV_Q_TYPE_C2H)
		queues = msg->qcnt.c2h_queues;

	if (q_type == QDMA_DEV_Q_TYPE_CMPT)
		queues = msg->qcnt.cmpt_queues;

	return queues;
}


uint8_t qdma_mbox_vf_parent_func_id_get(uint32_t *rcv_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	return msg->hdr.src_func_id;
}

int qdma_mbox_vf_dev_info_get(uint32_t *rcv_data,
	struct qdma_dev_attributes *dev_cap, uint32_t *dma_device_index)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	*dev_cap = msg->hello.dev_cap;
	*dma_device_index = msg->hello.dma_device_index;

	return msg->hdr.status;
}

int qdma_mbox_vf_qinfo_get(uint32_t *rcv_data, int *qbase, uint16_t *qmax)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	*qbase = msg->fmap.qbase;
	*qmax = msg->fmap.qmax;

	return msg->hdr.status;
}

int qdma_mbox_vf_csr_get(uint32_t *rcv_data, struct qdma_csr_info *csr)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	qdma_mbox_memcpy(csr, &msg->csr.csr_info, sizeof(struct qdma_csr_info));

	return msg->hdr.status;

}

int qdma_mbox_vf_reg_list_get(uint32_t *rcv_data,
		uint16_t *num_regs, struct qdma_reg_data *reg_list)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	*num_regs = msg->reg_read_list.num_regs;
	qdma_mbox_memcpy(reg_list, &(msg->reg_read_list.reg_list),
			(*num_regs * sizeof(struct qdma_reg_data)));

	return msg->hdr.status;

}

int qdma_mbox_vf_context_get(uint32_t *rcv_data,
			     struct qdma_descq_context *ctxt)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	qdma_mbox_memcpy(ctxt, &msg->qctxt.descq_ctxt,
			 sizeof(struct qdma_descq_context));

	return msg->hdr.status;
}

int qdma_mbox_vf_intr_context_get(uint32_t *rcv_data,
				  struct mbox_msg_intr_ctxt *ictxt)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	qdma_mbox_memcpy(ictxt, &msg->intr_ctxt.ctxt,
			 sizeof(struct mbox_msg_intr_ctxt));

	return msg->hdr.status;
}

void qdma_mbox_pf_hw_clear_ack(void *dev_hndl)
{
	uint32_t v;
	uint32_t reg;
	int i;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, 0);

	reg = mbox_base + MBOX_PF_ACK_BASE;

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);
	if ((v & F_MBOX_FN_STATUS_ACK) == 0)
		return;

	for (i = 0; i < MBOX_PF_ACK_COUNT; i++, reg += MBOX_PF_ACK_STEP) {
		v = qdma_reg_read(dev_hndl, reg);

		if (!v)
			continue;

		/* clear the ack status */
		qdma_reg_write(dev_hndl, reg, v);
	}
}

int qdma_mbox_send(void *dev_hndl, uint8_t is_vf, uint32_t *raw_data)
{
	int i;
	uint32_t reg = MBOX_OUT_MSG_BASE;
	uint32_t v;
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;
	uint16_t dst_func_id = msg->hdr.dst_func_id;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);
	if (v & F_MBOX_FN_STATUS_OUT_MSG)
		return -QDMA_ERR_MBOX_SEND_BUSY;

	if (!is_vf)
		qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_TARGET,
				V_MBOX_FN_TARGET_ID(dst_func_id));

	for (i = 0; i < MBOX_MSG_REG_MAX; i++, reg += MBOX_MSG_STEP)
		qdma_reg_write(dev_hndl, mbox_base + reg, raw_data[i]);

	/* clear the outgoing ack */
	if (!is_vf)
		mbox_pf_hw_clear_func_ack(dev_hndl, dst_func_id);


	qdma_log_debug("%s %s tx from_id=%d, to_id=%d, opcode=0x%x\n", __func__,
			is_vf?"VF":"PF", msg->hdr.src_func_id,
			msg->hdr.dst_func_id, msg->hdr.op);
	qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_CMD, F_MBOX_FN_CMD_SND);

	return QDMA_SUCCESS;
}

int qdma_mbox_rcv(void *dev_hndl, uint8_t is_vf, uint32_t *raw_data)
{
	uint32_t reg = MBOX_IN_MSG_BASE;
	uint32_t v = 0;
	int all_zero_msg = 1;
	int i;
	uint32_t from_id = 0;
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);

	if (!(v & M_MBOX_FN_STATUS_IN_MSG))
		return -QDMA_ERR_MBOX_NO_MSG_IN;

	if (!is_vf) {
		from_id = G_MBOX_FN_STATUS_SRC(v);
		qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_TARGET, from_id);
	}

	for (i = 0; i < MBOX_MSG_REG_MAX; i++, reg += MBOX_MSG_STEP) {
		raw_data[i] = qdma_reg_read(dev_hndl, mbox_base + reg);
		/* if rcv'ed message is all zero, stop and disable the mbox,
		 * the h/w mbox is not working properly
		 */
		if (raw_data[i])
			all_zero_msg = 0;
	}

	/* ack'ed the sender */
	qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_CMD, F_MBOX_FN_CMD_RCV);
	if (all_zero_msg) {
		qdma_log_error("%s: Message recv'd is all zeros. failure:%d\n",
					__func__,
					-QDMA_ERR_MBOX_ALL_ZERO_MSG);
		return -QDMA_ERR_MBOX_ALL_ZERO_MSG;
	}


	qdma_log_debug("%s %s fid=%d, opcode=0x%x\n", __func__,
				   is_vf?"VF":"PF", msg->hdr.dst_func_id,
				   msg->hdr.op);
	if (!is_vf && (from_id != msg->hdr.src_func_id))
		msg->hdr.src_func_id = from_id;

	return QDMA_SUCCESS;
}

void qdma_mbox_hw_init(void *dev_hndl, uint8_t is_vf)
{
	uint32_t v;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	if (is_vf) {
		v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);
		if (v & M_MBOX_FN_STATUS_IN_MSG)
			qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_CMD,
				    F_MBOX_FN_CMD_RCV);
	} else
		qdma_mbox_pf_hw_clear_ack(dev_hndl);
}

void qdma_mbox_enable_interrupts(void *dev_hndl, uint8_t is_vf)
{
	int vector = 0x0;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_VEC, vector);
	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_EN, 0x1);
}

void qdma_mbox_disable_interrupts(void *dev_hndl, uint8_t is_vf)
{
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_EN, 0x0);
}


int qdma_mbox_compose_vf_reset_message(uint32_t *raw_data, uint8_t src_funcid,
				uint8_t dest_funcid)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_RESET_PREPARE;
	msg->hdr.src_func_id = src_funcid;
	msg->hdr.dst_func_id = dest_funcid;
	return 0;
}

int qdma_mbox_compose_pf_reset_done_message(uint32_t *raw_data,
					uint8_t src_funcid, uint8_t dest_funcid)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_RESET_DONE;
	msg->hdr.src_func_id = src_funcid;
	msg->hdr.dst_func_id = dest_funcid;
	return 0;
}

int qdma_mbox_compose_pf_offline(uint32_t *raw_data, uint8_t src_funcid,
				uint8_t dest_funcid)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_PF_BYE;
	msg->hdr.src_func_id = src_funcid;
	msg->hdr.dst_func_id = dest_funcid;
	return 0;
}

int qdma_mbox_vf_rcv_msg_handler(uint32_t *rcv_msg, uint32_t *resp_msg)
{
	union qdma_mbox_txrx *rcv =  (union qdma_mbox_txrx *)rcv_msg;
	union qdma_mbox_txrx *resp =  (union qdma_mbox_txrx *)resp_msg;
	int rv = 0;

	switch (rcv->hdr.op) {
	case MBOX_OP_RESET_PREPARE:
		resp->hdr.op = rcv->hdr.op + MBOX_MSG_OP_RSP_OFFSET;
		resp->hdr.dst_func_id = rcv->hdr.src_func_id;
		resp->hdr.src_func_id = rcv->hdr.dst_func_id;
		rv = QDMA_MBOX_VF_RESET;
		break;
	case MBOX_OP_RESET_DONE:
		resp->hdr.op = rcv->hdr.op + MBOX_MSG_OP_RSP_OFFSET;
		resp->hdr.dst_func_id = rcv->hdr.src_func_id;
		resp->hdr.src_func_id = rcv->hdr.dst_func_id;
		rv = QDMA_MBOX_PF_RESET_DONE;
		break;
	case MBOX_OP_PF_BYE:
		resp->hdr.op = rcv->hdr.op + MBOX_MSG_OP_RSP_OFFSET;
		resp->hdr.dst_func_id = rcv->hdr.src_func_id;
		resp->hdr.src_func_id = rcv->hdr.dst_func_id;
		rv = QDMA_MBOX_PF_BYE;
		break;
	default:
		break;
	}
	return rv;
}

uint8_t qdma_mbox_out_status(void *dev_hndl, uint8_t is_vf)
{
	uint32_t v;
	uint32_t mbox_base = get_mbox_offset(dev_hndl, is_vf);

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);
	if (v & F_MBOX_FN_STATUS_OUT_MSG)
		return 1;
	else
		return 0;
}
