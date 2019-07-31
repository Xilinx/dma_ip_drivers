/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
 */

#include "qdma_mbox_protocol.h"

#include "qdma_platform.h"
#include "qdma_resource_mgmt.h"

/**
 * mailbox registers
 */
#define MBOX_BASE_VF		0x1000
#define MBOX_BASE_PF		0x2400

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
	/** @MBOX_OP_BYE: vf offline */
	MBOX_OP_BYE,
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
	uint8_t cmpt_ctxt_type:2;
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
		/** q add/del notify message */
		struct mbox_msg_q_nitfy q_notify;
		/** buffer to hold raw data between pf and vf */
		uint32_t raw[MBOX_MSG_REG_MAX];
};

static void qdma_mbox_memcpy(void *to, void *from, uint32_t size)
{
	uint32_t i;
	uint8_t *_to = (uint8_t *)to;
	uint8_t *_from = (uint8_t *)from;

	for (i = 0; i < size; i++)
		_to[i] = _from[i];
}

static void qdma_mbox_memset(void *to, uint8_t val, uint32_t size)
{
	uint32_t i;
	uint8_t *_to = (uint8_t *)to;

	for (i = 0; i < size; i++)
		_to[i] = val;
}

static int get_ring_idx(void *dev_hndl, uint16_t ring_sz, uint16_t *rng_idx)
{
	uint32_t rng_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int i;
	int rv = qdma_get_global_ring_sizes(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, rng_sz);

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (ring_sz == (rng_sz[i] - 1)) {
			*rng_idx = i;
			return 0;
		}
	}

	return -QDMA_ERR_MBOX_INV_RINGSZ;
}

static int get_buf_idx(void *dev_hndl,  uint16_t buf_sz, uint16_t *buf_idx)
{
	uint32_t c2h_buf_sz[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int rv = qdma_get_global_buffer_sizes(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, c2h_buf_sz);
	int i;

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (c2h_buf_sz[i] == buf_sz) {
			*buf_idx = i;
			return 0;
		}
	}

	return -QDMA_ERR_MBOX_INV_BUFSZ;
}

static int get_cntr_idx(void *dev_hndl, uint8_t cntr_val, uint8_t *cntr_idx)
{
	uint32_t cntr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int rv = qdma_get_global_counter_threshold(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, cntr_th);
	int i;

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (cntr_th[i] == cntr_val) {
			*cntr_idx = i;
			return 0;
		}
	}

	return -QDMA_ERR_MBOX_INV_CNTR_TH;
}

static int get_tmr_idx(void *dev_hndl, uint8_t tmr_val, uint8_t *tmr_idx)
{
	uint32_t tmr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = { 0 };
	int rv = qdma_get_global_timer_count(dev_hndl, 0,
			QDMA_GLOBAL_CSR_ARRAY_SZ, tmr_th);
	int i;

	if (rv)
		return rv;
	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (tmr_th[i] == tmr_val) {
			*tmr_idx = i;
			return 0;
		}
	}

	return -QDMA_ERR_MBOX_INV_TMR_TH;
}

static int mbox_compose_sw_context(void *dev_hndl,
				   struct mbox_msg_qctxt *qctxt,
				   struct qdma_descq_sw_ctxt *sw_ctxt)
{
	uint16_t rng_idx = 0;
	int rv = 0;

	if (!qctxt || !sw_ctxt)
		return -QDMA_ERR_INV_PARAM;

	rv = get_ring_idx(dev_hndl, qctxt->descq_conf.ringsz, &rng_idx);
	if (rv < 0)
		return rv;
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

	return 0;
}

static int mbox_compose_prefetch_context(void *dev_hndl,
					 struct mbox_msg_qctxt *qctxt,
				 struct qdma_descq_prefetch_ctxt *pfetch_ctxt)
{
	uint16_t buf_idx = 0;
	int rv = 0;

	if (!qctxt || !pfetch_ctxt)
		return -QDMA_ERR_INV_PARAM;
	rv = get_buf_idx(dev_hndl, qctxt->descq_conf.bufsz, &buf_idx);
	if (rv < 0)
		return rv;
	/* prefetch context */
	pfetch_ctxt->valid = 1;
	pfetch_ctxt->bypass = qctxt->descq_conf.en_bypass_prefetch;
	pfetch_ctxt->bufsz_idx = buf_idx;
	pfetch_ctxt->pfch_en = qctxt->descq_conf.pfch_en;

	return 0;
}


static int mbox_compose_cmpt_context(void *dev_hndl,
				     struct mbox_msg_qctxt *qctxt,
				     struct qdma_descq_cmpt_ctxt *cmpt_ctxt)
{
	uint16_t rng_idx = 0;
	uint8_t cntr_idx = 0, tmr_idx = 0;
	int rv = 0;

	if (!qctxt || !cmpt_ctxt)
		return -QDMA_ERR_INV_PARAM;
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

	return 0;
}

static int mbox_clear_queue_contexts(void *dev_hndl, uint8_t pci_bus_num,
			      uint16_t func_id, uint16_t qid_hw, uint8_t st,
			      uint8_t c2h,
			      enum mbox_cmpt_ctxt_type cmpt_ctxt_type)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;

	if (cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = qdma_cmpt_context_clear(dev_hndl, qid_hw);
		if (rv < 0)
			return rv;
	} else {
		rv = qdma_dev_qinfo_get(pci_bus_num, func_id, &qbase, &qmax);
		if (rv < 0)
			return rv;

		q_range = qdma_dev_is_queue_in_range(pci_bus_num,
						func_id, qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			return -QDMA_ERR_MBOX_INV_QID;

		rv = qdma_sw_context_clear(dev_hndl, c2h, qid_hw);
		if (rv < 0)
			return rv;

		if (st && c2h) {
			rv = qdma_pfetch_context_clear(dev_hndl, qid_hw);
			if (rv < 0)
				return rv;
		}

		if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = qdma_cmpt_context_clear(dev_hndl, qid_hw);
			if (rv < 0)
				return rv;
		}
	}

	return 0;
}

static int mbox_invalidate_queue_contexts(void *dev_hndl, uint8_t pci_bus_num,
			      uint16_t func_id, uint16_t qid_hw, uint8_t st,
			      uint8_t c2h,
			      enum mbox_cmpt_ctxt_type cmpt_ctxt_type)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;

	if (cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = qdma_cmpt_context_invalidate(dev_hndl, qid_hw);
		if (rv < 0)
			return rv;
	} else {
		rv = qdma_dev_qinfo_get(pci_bus_num, func_id, &qbase, &qmax);
		if (rv < 0)
			return rv;

		q_range = qdma_dev_is_queue_in_range(pci_bus_num,
						func_id, qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			return -QDMA_ERR_MBOX_INV_QID;

		rv = qdma_sw_context_invalidate(dev_hndl, c2h, qid_hw);
		if (rv < 0)
			return rv;

		if (st && c2h) {
			rv = qdma_pfetch_context_invalidate(dev_hndl, qid_hw);
			if (rv < 0)
				return rv;
		}

		if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = qdma_cmpt_context_invalidate(dev_hndl, qid_hw);
			if (rv < 0)
				return rv;
		}
	}

	return 0;
}

static int mbox_write_queue_contexts(void *dev_hndl, uint8_t pci_bus_num,
				     struct mbox_msg_qctxt *qctxt)
{
	int rv;
	int qbase;
	uint32_t qmax;
	enum qdma_dev_q_range q_range;
	struct qdma_descq_context descq_ctxt;
	uint16_t qid_hw = qctxt->qid_hw;


	rv = qdma_dev_qinfo_get(pci_bus_num, qctxt->descq_conf.func_id,
				&qbase, &qmax);
	if (rv < 0)
		return rv;

	q_range = qdma_dev_is_queue_in_range(pci_bus_num,
					     qctxt->descq_conf.func_id,
					     qctxt->qid_hw);
	if (q_range != QDMA_DEV_Q_IN_RANGE)
		return -QDMA_ERR_MBOX_INV_QID;

	qdma_mbox_memset(&descq_ctxt, 0, sizeof(struct qdma_descq_context));

	if (qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_CTXT_ONLY) {
		rv = mbox_compose_cmpt_context(dev_hndl, qctxt,
			       &descq_ctxt.cmpt_ctxt);
		if (rv < 0)
			return rv;

		rv = qdma_cmpt_context_clear(dev_hndl, qid_hw);
		if (rv < 0)
			return rv;

		rv = qdma_cmpt_context_write(dev_hndl, qid_hw,
			     &descq_ctxt.cmpt_ctxt);
		if (rv < 0)
			return rv;

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

		rv = mbox_clear_queue_contexts(dev_hndl, pci_bus_num,
					qctxt->descq_conf.func_id,
					qctxt->qid_hw,
					qctxt->st,
					qctxt->c2h,
					qctxt->cmpt_ctxt_type);
		if (rv < 0)
			return rv;
		rv = qdma_sw_context_write(dev_hndl, qctxt->c2h, qid_hw,
					   &descq_ctxt.sw_ctxt);
		if (rv < 0)
			return rv;

		if (qctxt->st && qctxt->c2h) {
			rv = qdma_pfetch_context_write(dev_hndl, qid_hw,
						       &descq_ctxt.pfetch_ctxt);
			if (rv < 0)
				return rv;
		}

		if ((qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
		    (qctxt->cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
			rv = qdma_cmpt_context_write(dev_hndl, qid_hw,
						     &descq_ctxt.cmpt_ctxt);
			if (rv < 0)
				return rv;
		}
	}
	return 0;
}

static int mbox_read_queue_contexts(void *dev_hndl, uint16_t qid_hw,
			uint8_t st, uint8_t c2h,
			enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
			struct qdma_descq_context *ctxt)
{
	int rv;

	rv = qdma_sw_context_read(dev_hndl, c2h, qid_hw, &ctxt->sw_ctxt);
	if (rv < 0)
		return rv;

	rv = qdma_hw_context_read(dev_hndl, c2h, qid_hw, &ctxt->hw_ctxt);
	if (rv < 0)
		return rv;

	rv = qdma_credit_context_read(dev_hndl, c2h, qid_hw, &ctxt->cr_ctxt);
	if (rv < 0)
		return rv;

	if (st && c2h) {
		rv = qdma_pfetch_context_read(dev_hndl,
					qid_hw, &ctxt->pfetch_ctxt);
		if (rv < 0)
			return rv;
	}

	if ((cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_MM) ||
	    (cmpt_ctxt_type == QDMA_MBOX_CMPT_WITH_ST)) {
		rv = qdma_cmpt_context_read(dev_hndl,
					qid_hw, &ctxt->cmpt_ctxt);
		if (rv < 0)
			return rv;
	}

	return 0;
}

int qdma_mbox_pf_rcv_msg_handler(void *dev_hndl, uint8_t pci_bus_num,
				 uint16_t func_id, uint32_t *rcv_msg,
				 uint32_t *resp_msg)
{
	union qdma_mbox_txrx *rcv =  (union qdma_mbox_txrx *)rcv_msg;
	union qdma_mbox_txrx *resp =  (union qdma_mbox_txrx *)resp_msg;
	struct mbox_msg_hdr *hdr = &rcv->hdr;
	int rv = 0;
	int ret = 0;

	if (!rcv)
		return -QDMA_ERR_INV_PARAM;

	switch (rcv->hdr.op) {
	case MBOX_OP_BYE:
	{
		struct qdma_fmap_cfg fmap;

		fmap.qbase = 0;
		fmap.qmax = 0;
		rv = qdma_fmap_write(dev_hndl, hdr->src_func_id, &fmap);

		qdma_dev_entry_destroy(pci_bus_num,
				hdr->src_func_id);

		ret = QDMA_MBOX_VF_OFFLINE;
	}
	break;
	case MBOX_OP_HELLO:
	{
		struct mbox_msg_fmap *fmap = &rcv->fmap;
		struct qdma_fmap_cfg fmap_cfg;
		struct mbox_msg_hello *rsp_hello = &resp->hello;

		rv = qdma_dev_qinfo_get(pci_bus_num,
					hdr->src_func_id,
					&fmap->qbase,
					&fmap->qmax);
		if (rv < 0) {
			rv = qdma_dev_entry_create(pci_bus_num,
						   hdr->src_func_id);
		}
		if (!rv) {
			rsp_hello->qbase = fmap->qbase;
			rsp_hello->qmax = fmap->qmax;
			qdma_get_device_attributes(dev_hndl,
							&rsp_hello->dev_cap);
		}
		qdma_mbox_memset(&fmap_cfg, 0,
				 sizeof(struct qdma_fmap_cfg));
		qdma_fmap_write(dev_hndl, hdr->src_func_id, &fmap_cfg);

		ret = QDMA_MBOX_VF_ONLINE;
	}
	break;
	case MBOX_OP_FMAP:
	{
		struct mbox_msg_fmap *fmap = &rcv->fmap;
		struct qdma_fmap_cfg fmap_cfg;

		fmap_cfg.qbase = fmap->qbase;
		fmap_cfg.qmax = fmap->qmax;

		rv = qdma_fmap_write(dev_hndl, hdr->src_func_id,
				     &fmap_cfg);
		if (rv < 0)
			rv = -QDMA_ERR_MBOX_FMAP_WR_FAILED;
	}
	break;
	case MBOX_OP_CSR:
	{
		struct mbox_msg_csr *rsp_csr = &resp->csr;

		uint32_t ringsz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t bufsz[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t tmr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		uint32_t cntr_th[QDMA_GLOBAL_CSR_ARRAY_SZ] = {0};
		int i;

		rv = qdma_get_global_ring_sizes(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, ringsz);
		if (rv < 0)
			goto exit_func;

		rv = qdma_get_global_buffer_sizes(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, bufsz);
		if (rv < 0 && (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
			goto exit_func;

		rv = qdma_get_global_timer_count(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, tmr_th);
		if (rv < 0 && (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
			goto exit_func;

		rv = qdma_get_global_counter_threshold(dev_hndl, 0,
				QDMA_GLOBAL_CSR_ARRAY_SZ, cntr_th);
		if (rv < 0 && (rv != -QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED))
			goto exit_func;

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

		rv = qdma_dev_update(pci_bus_num,
					  hdr->src_func_id,
					  fmap->qmax, &fmap->qbase);
		if (rv == 0)
			rv = qdma_dev_qinfo_get(pci_bus_num,
						hdr->src_func_id,
						&resp->fmap.qbase,
						&resp->fmap.qmax);
		if (rv < 0)
			rv = -QDMA_ERR_MBOX_NUM_QUEUES;
		else {
			struct qdma_fmap_cfg fmap_cfg;

			qdma_mbox_memset(&fmap_cfg, 0,
					 sizeof(struct qdma_fmap_cfg));
			qdma_fmap_write(dev_hndl, hdr->src_func_id,
					&fmap_cfg);
		}
	}
	break;
	case MBOX_OP_QNOTIFY_ADD:
	{
		struct mbox_msg_q_nitfy *q_notify = &rcv->q_notify;
		enum qdma_dev_q_range q_range;

		q_range = qdma_dev_is_queue_in_range(
				pci_bus_num,
				q_notify->hdr.src_func_id,
				q_notify->qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			rv = -QDMA_ERR_MBOX_INV_QID;
		else
			rv = qdma_dev_increment_active_queue(
					pci_bus_num,
					q_notify->hdr.src_func_id);
	}
	break;
	case MBOX_OP_QNOTIFY_DEL:
	{
		struct mbox_msg_q_nitfy *q_notify = &rcv->q_notify;
		enum qdma_dev_q_range q_range;

		q_range = qdma_dev_is_queue_in_range(
				pci_bus_num,
				q_notify->hdr.src_func_id,
				q_notify->qid_hw);
		if (q_range != QDMA_DEV_Q_IN_RANGE)
			rv = -QDMA_ERR_MBOX_INV_QID;
		else
			rv = qdma_dev_decrement_active_queue(
					pci_bus_num,
					q_notify->hdr.src_func_id);
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
			rv = qdma_indirect_intr_context_clear(dev_hndl,
						      ring_index);
			if (rv < 0)
				resp->hdr.status = rv;
			rv = qdma_indirect_intr_context_write(dev_hndl,
						      ring_index, ctxt);
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

			rv = qdma_indirect_intr_context_read(dev_hndl,
						      ring_index,
						      &rsp_ictxt->ictxt[i]);
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
			rv = qdma_indirect_intr_context_clear(
					dev_hndl,
					ictxt->ring_index_list[i]);
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
			rv = qdma_indirect_intr_context_invalidate(
					dev_hndl,
					ictxt->ring_index_list[i]);
			if (rv < 0)
				resp->hdr.status = rv;
		}
	}
	break;
	case MBOX_OP_QCTXT_INV:
	{
		struct mbox_msg_qctxt *qctxt = &rcv->qctxt;

		rv = mbox_invalidate_queue_contexts(dev_hndl,
							pci_bus_num,
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
						pci_bus_num,
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
		rv = mbox_write_queue_contexts(dev_hndl, pci_bus_num,
					       qctxt);
	}
	break;
	default:
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

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_HELLO;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = (uint32_t)*qbase;
	msg->fmap.qmax = qmax;

	return 0;
}

int qdma_mbox_compose_vf_offline(uint16_t func_id,
				 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_BYE;
	msg->hdr.src_func_id = func_id;

	return 0;
}

int qdma_mbox_compose_vf_qreq(uint16_t func_id,
			      uint16_t qmax, int qbase, uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QREQ;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = qbase;
	msg->fmap.qmax = qmax;

	return 0;
}

int qdma_mbox_compose_vf_notify_qadd(uint16_t func_id,
				     uint16_t qid_hw, uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QNOTIFY_ADD;
	msg->hdr.src_func_id = func_id;
	msg->q_notify.qid_hw = qid_hw;

	return 0;
}

int qdma_mbox_compose_vf_notify_qdel(uint16_t func_id,
				     uint16_t qid_hw, uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QNOTIFY_DEL;
	msg->hdr.src_func_id = func_id;
	msg->q_notify.qid_hw = qid_hw;

	return 0;
}

int qdma_mbox_compose_vf_fmap_prog(uint16_t func_id,
				   uint16_t qmax, int qbase,
				   uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_FMAP;
	msg->hdr.src_func_id = func_id;
	msg->fmap.qbase = (uint32_t)qbase;
	msg->fmap.qmax = qmax;

	return 0;
}

int qdma_mbox_compose_vf_qctxt_write(uint16_t func_id,
			uint16_t qid_hw, uint8_t st, uint8_t c2h,
			enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
			struct mbox_descq_conf *descq_conf,
			uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_WRT;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	qdma_mbox_memcpy(&msg->qctxt.descq_conf, descq_conf,
	       sizeof(struct mbox_descq_conf));

	return 0;
}

int qdma_mbox_compose_vf_qctxt_read(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_RD;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return 0;
}

int qdma_mbox_compose_vf_qctxt_invalidate(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_INV;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return 0;
}

int qdma_mbox_compose_vf_qctxt_clear(uint16_t func_id,
				uint16_t qid_hw, uint8_t st, uint8_t c2h,
				enum mbox_cmpt_ctxt_type cmpt_ctxt_type,
				uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_QCTXT_CLR;
	msg->hdr.src_func_id = func_id;
	msg->qctxt.qid_hw = qid_hw;
	msg->qctxt.c2h = c2h;
	msg->qctxt.st = st;
	msg->qctxt.cmpt_ctxt_type = cmpt_ctxt_type;

	return 0;
}

int qdma_mbox_compose_csr_read(uint16_t func_id,
			       uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_CSR;
	msg->hdr.src_func_id = func_id;

	return 0;
}

int qdma_mbox_compose_vf_intr_ctxt_write(uint16_t func_id,
					 struct mbox_msg_intr_ctxt *intr_ctxt,
					 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_WRT;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return 0;
}

int qdma_mbox_compose_vf_intr_ctxt_read(uint16_t func_id,
					struct mbox_msg_intr_ctxt *intr_ctxt,
					uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_RD;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return 0;
}

int qdma_mbox_compose_vf_intr_ctxt_clear(uint16_t func_id,
					 struct mbox_msg_intr_ctxt *intr_ctxt,
					 uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_CLR;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return 0;
}

int qdma_mbox_compose_vf_intr_ctxt_invalidate(uint16_t func_id,
				      struct mbox_msg_intr_ctxt *intr_ctxt,
				      uint32_t *raw_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;

	if (!raw_data)
		return -QDMA_ERR_INV_PARAM;

	qdma_mbox_memset(raw_data, 0, sizeof(union qdma_mbox_txrx));
	msg->hdr.op = MBOX_OP_INTR_CTXT_INV;
	msg->hdr.src_func_id = func_id;
	qdma_mbox_memcpy(&msg->intr_ctxt.ctxt, intr_ctxt,
	       sizeof(struct mbox_msg_intr_ctxt));

	return 0;
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

uint8_t qdma_mbox_vf_func_id_get(uint32_t *rcv_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	return msg->hdr.dst_func_id;
}

uint8_t qdma_mbox_vf_parent_func_id_get(uint32_t *rcv_data)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	return msg->hdr.src_func_id;
}

int qdma_mbox_vf_dev_info_get(uint32_t *rcv_data,
			struct qdma_dev_attributes *dev_cap)
{
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)rcv_data;

	dev_cap->num_pfs = msg->hello.dev_cap.num_pfs;
	dev_cap->num_qs = msg->hello.dev_cap.num_qs;
	dev_cap->flr_present = msg->hello.dev_cap.flr_present;
	dev_cap->st_en = msg->hello.dev_cap.st_en;
	dev_cap->mm_en = msg->hello.dev_cap.mm_en;
	dev_cap->mm_cmpt_en = msg->hello.dev_cap.mm_cmpt_en;
	dev_cap->mailbox_en = msg->hello.dev_cap.mailbox_en;
	dev_cap->mm_channel_max = msg->hello.dev_cap.mm_channel_max;

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

static inline void mbox_pf_hw_clear_func_ack(void *dev_hndl,
					     uint8_t func_id)
{
	int idx = func_id / 32; /* bitmask, uint32_t reg */
	int bit = func_id % 32;
	uint32_t mbox_base = MBOX_BASE_PF;

	/* clear the function's ack status */
	qdma_reg_write(dev_hndl,
		       mbox_base + MBOX_PF_ACK_BASE + idx * MBOX_PF_ACK_STEP,
		(1 << bit));
}

void qdma_mbox_pf_hw_clear_ack(void *dev_hndl)
{
	uint32_t v;
	uint32_t mbox_base = MBOX_BASE_PF;
	uint32_t reg = mbox_base + MBOX_PF_ACK_BASE;
	int i;

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
	uint32_t mbox_base = (is_vf) ? MBOX_BASE_VF : MBOX_BASE_PF;

	if (!is_vf)
		qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_TARGET,
				V_MBOX_FN_TARGET_ID(dst_func_id));

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);
	if (v & F_MBOX_FN_STATUS_OUT_MSG)
		return -QDMA_ERR_MBOX_SEND_BUSY;

	for (i = 0; i < MBOX_MSG_REG_MAX; i++, reg += MBOX_MSG_STEP)
		qdma_reg_write(dev_hndl, mbox_base + reg, raw_data[i]);

	/* clear the outgoing ack */
	if (!is_vf)
		mbox_pf_hw_clear_func_ack(dev_hndl, dst_func_id);

	qdma_reg_write(dev_hndl, mbox_base + MBOX_FN_CMD, F_MBOX_FN_CMD_SND);

	return QDMA_SUCCESS;
}

int qdma_mbox_rcv(void *dev_hndl, uint8_t is_vf, uint32_t *raw_data)
{
	uint32_t reg = MBOX_IN_MSG_BASE;
	uint32_t v = 0;
	int all_zero_msg = 1;
	int i;
	unsigned int from_id = 0;
	union qdma_mbox_txrx *msg = (union qdma_mbox_txrx *)raw_data;
	uint32_t mbox_base = (is_vf) ? MBOX_BASE_VF : MBOX_BASE_PF;

	v = qdma_reg_read(dev_hndl, mbox_base + MBOX_FN_STATUS);


	if (!(v & M_MBOX_FN_STATUS_IN_MSG))
		return -QDMA_ERR_MOBX_NO_MSG_IN;

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
	if (all_zero_msg)
		return -QDMA_ERR_MBOX_ALL_ZERO_MSG;

	if (!is_vf && (from_id != msg->hdr.src_func_id))
		msg->hdr.src_func_id = from_id;


	return QDMA_SUCCESS;
}

void qdma_mbox_hw_init(void *dev_hndl, uint8_t is_vf)
{
	uint32_t v;
	uint32_t mbox_base = (is_vf) ? MBOX_BASE_VF : MBOX_BASE_PF;

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
	uint32_t mbox_base = (is_vf) ? MBOX_BASE_VF : MBOX_BASE_PF;

	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_VEC, vector);
	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_EN, 0x1);
}

void qdma_mbox_disable_interrupts(void *dev_hndl, uint8_t is_vf)
{
	uint32_t mbox_base = (is_vf) ? MBOX_BASE_VF : MBOX_BASE_PF;

	qdma_reg_write(dev_hndl, mbox_base + MBOX_ISR_EN, 0x0);
}

