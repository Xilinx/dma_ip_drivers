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

#ifndef __QDMA_ACCESS_COMMON_H_
#define __QDMA_ACCESS_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qdma_access_export.h"
#include "qdma_access_errors.h"

/* QDMA HW version string array length */
#define QDMA_HW_VERSION_STRING_LEN			32

#define ENABLE_INIT_CTXT_MEMORY			1

#ifdef GCC_COMPILER
static inline uint32_t get_trailing_zeros(uint64_t x)
{
	uint32_t rv =
		__builtin_ffsll(x) - 1;
	return rv;
}
#else
static inline uint32_t get_trailing_zeros(uint64_t value)
{
	uint32_t pos = 0;

	if ((value & 0xffffffff) == 0) {
		pos += 32;
		value >>= 32;
	}
	if ((value & 0xffff) == 0) {
		pos += 16;
		value >>= 16;
	}
	if ((value & 0xff) == 0) {
		pos += 8;
		value >>= 8;
	}
	if ((value & 0xf) == 0) {
		pos += 4;
		value >>= 4;
	}
	if ((value & 0x3) == 0) {
		pos += 2;
		value >>= 2;
	}
	if ((value & 0x1) == 0)
		pos += 1;

	return pos;
}
#endif

#define FIELD_SHIFT(mask)       get_trailing_zeros(mask)
#define FIELD_SET(mask, val)    ((val << FIELD_SHIFT(mask)) & mask)
#define FIELD_GET(mask, reg)    ((reg & mask) >> FIELD_SHIFT(mask))


/* CSR Default values */
#define DEFAULT_MAX_DSC_FETCH               6
#define DEFAULT_WRB_INT                     QDMA_WRB_INTERVAL_128
#define DEFAULT_PFCH_STOP_THRESH            256
#define DEFAULT_PFCH_NUM_ENTRIES_PER_Q      8
#define DEFAULT_PFCH_MAX_Q_CNT              16
#define DEFAULT_C2H_INTR_TIMER_TICK         25
#define DEFAULT_CMPT_COAL_TIMER_CNT         5
#define DEFAULT_CMPT_COAL_TIMER_TICK        25
#define DEFAULT_CMPT_COAL_MAX_BUF_SZ        32

#define QDMA_BAR_NUM                        6

/** Maximum data vectors to be used for each function
 * TODO: Please note that for 2018.2 only one vector would be used
 * per pf and only one ring would be created for this vector
 * It is also assumed that all functions have the same number of data vectors
 * and currently different number of vectors per PF is not supported
 */
#define QDMA_NUM_DATA_VEC_FOR_INTR_CXT  1

enum ind_ctxt_cmd_op {
	QDMA_CTXT_CMD_CLR,
	QDMA_CTXT_CMD_WR,
	QDMA_CTXT_CMD_RD,
	QDMA_CTXT_CMD_INV
};

enum ind_ctxt_cmd_sel {
	QDMA_CTXT_SEL_SW_C2H,
	QDMA_CTXT_SEL_SW_H2C,
	QDMA_CTXT_SEL_HW_C2H,
	QDMA_CTXT_SEL_HW_H2C,
	QDMA_CTXT_SEL_CR_C2H,
	QDMA_CTXT_SEL_CR_H2C,
	QDMA_CTXT_SEL_CMPT,
	QDMA_CTXT_SEL_PFTCH,
	QDMA_CTXT_SEL_INT_COAL,
	QDMA_CTXT_SEL_PASID_RAM_LOW,
	QDMA_CTXT_SEL_PASID_RAM_HIGH,
	QDMA_CTXT_SEL_TIMER,
	QDMA_CTXT_SEL_FMAP,
};

/* polling a register */
#define	QDMA_REG_POLL_DFLT_INTERVAL_US	10		    /* 10us per poll */
#define	QDMA_REG_POLL_DFLT_TIMEOUT_US	(500*1000)	/* 500ms */

/** Constants */
#define QDMA_NUM_RING_SIZES                                 16
#define QDMA_NUM_C2H_TIMERS                                 16
#define QDMA_NUM_C2H_BUFFER_SIZES                           16
#define QDMA_NUM_C2H_COUNTERS                               16
#define QDMA_MM_CONTROL_RUN                                 0x1
#define QDMA_MM_CONTROL_STEP                                0x100
#define QDMA_MAGIC_NUMBER                                   0x1fd3
#define QDMA_PIDX_STEP                                      0x10
#define QDMA_CMPT_CIDX_STEP                                 0x10
#define QDMA_INT_CIDX_STEP                                  0x10


/** QDMA_IND_REG_SEL_PFTCH */
#define QDMA_PFTCH_CTXT_SW_CRDT_GET_H_MASK                  GENMASK(15, 3)
#define QDMA_PFTCH_CTXT_SW_CRDT_GET_L_MASK                  GENMASK(2, 0)

/** QDMA_IND_REG_SEL_CMPT */
#define QDMA_COMPL_CTXT_BADDR_GET_H_MASK                    GENMASK_ULL(63, 38)
#define QDMA_COMPL_CTXT_BADDR_GET_L_MASK                    GENMASK_ULL(37, 12)
#define QDMA_COMPL_CTXT_PIDX_GET_H_MASK                     GENMASK(15, 4)
#define QDMA_COMPL_CTXT_PIDX_GET_L_MASK                     GENMASK(3, 0)

#define QDMA_INTR_CTXT_BADDR_GET_H_MASK                     GENMASK_ULL(63, 61)
#define QDMA_INTR_CTXT_BADDR_GET_M_MASK                     GENMASK_ULL(60, 29)
#define QDMA_INTR_CTXT_BADDR_GET_L_MASK                     GENMASK_ULL(28, 12)

#define     QDMA_GLBL2_MM_CMPT_EN_MASK                      BIT(2)
#define     QDMA_GLBL2_FLR_PRESENT_MASK                     BIT(1)
#define     QDMA_GLBL2_MAILBOX_EN_MASK                      BIT(0)

#define QDMA_REG_IND_CTXT_REG_COUNT                         8

/* ------------------------ indirect register context fields -----------*/
union qdma_ind_ctxt_cmd {
	uint32_t word;
	struct {
		uint32_t busy:1;
		uint32_t sel:4;
		uint32_t op:2;
		uint32_t qid:11;
		uint32_t rsvd:14;
	} bits;
};

#define QDMA_IND_CTXT_DATA_NUM_REGS                         8

/**
 * struct qdma_indirect_ctxt_regs - Inirect Context programming registers
 */
struct qdma_indirect_ctxt_regs {
	uint32_t qdma_ind_ctxt_data[QDMA_IND_CTXT_DATA_NUM_REGS];
	uint32_t qdma_ind_ctxt_mask[QDMA_IND_CTXT_DATA_NUM_REGS];
	union qdma_ind_ctxt_cmd cmd;
};

/**
 * struct qdma_fmap_cfg - fmap config data structure
 */
struct qdma_fmap_cfg {

	/** @qbase - queue base for the function */
	uint16_t qbase;
	/** @qmax - maximum queues in the function */
	uint16_t qmax;
};

/**
 * struct qdma_qid2vec - qid to vector mapping data structure
 */
struct qdma_qid2vec {

	/** @c2h_vector - For direct interrupt, it is the interrupt
	 * vector index of msix table;
	 * for indirect interrupt, it is the ring index
	 */
	uint8_t c2h_vector;
	/** @c2h_en_coal - C2H Interrupt aggregation enable */
	uint8_t c2h_en_coal;
	/** @h2c_vector - For direct interrupt, it is the interrupt
	 * vector index of msix table;
	 * for indirect interrupt, it is the ring index
	 */
	uint8_t h2c_vector;
	/** @h2c_en_coal - H2C Interrupt aggregation enable */
	uint8_t h2c_en_coal;
};

/**
 * struct qdma_descq_sw_ctxt - descq SW context config data structure
 */
struct qdma_descq_sw_ctxt {

	/** @ring_bs_addr - ring base address */
	uint64_t ring_bs_addr;
	/** @vec - vector number */
	uint16_t vec;
	/** @pidx - initial producer index */
	uint16_t pidx;
	/** @irq_arm - Interrupt Arm */
	uint8_t irq_arm;
	/** @fnc_id - Function ID */
	uint8_t fnc_id;
	/** @qen - Indicates that the queue is enabled */
	uint8_t qen;
	/** @frcd_en -Enable fetch credit */
	uint8_t frcd_en;
	/** @wbi_chk -Writeback/Interrupt after pending check */
	uint8_t wbi_chk;
	/** @wbi_intvl_en -Write back/Interrupt interval */
	uint8_t wbi_intvl_en;
	/** @at - Address tanslation */
	uint8_t at;
	/** @fetch_max - Maximum number of descriptor fetches outstanding */
	uint8_t fetch_max;
	/** @rngsz_idx - Descriptor ring size index */
	uint8_t rngsz_idx;
	/** @desc_sz -Descriptor fetch size */
	uint8_t desc_sz;
	/** @bypass - bypass enable */
	uint8_t bypass;
	/** @mm_chn - MM channel */
	uint8_t mm_chn;
	/** @wbk_en -Writeback enable */
	uint8_t wbk_en;
	/** @irq_en -Interrupt enable */
	uint8_t irq_en;
	/** @port_id -Port_id */
	uint8_t port_id;
	/** @irq_no_last - No interrupt was sent */
	uint8_t irq_no_last;
	/** @err - Error status */
	uint8_t err;
	/** @err_wb_sent -writeback/interrupt was sent for an error */
	uint8_t err_wb_sent;
	/** @irq_req - Interrupt due to error waiting to be sent */
	uint8_t irq_req;
	/** @mrkr_dis - Marker disable */
	uint8_t mrkr_dis;
	/** @is_mm - MM mode */
	uint8_t is_mm;
	/** @intr_aggr - interrupt aggregation enable */
	uint8_t intr_aggr;
	/** @pasid_en - PASID Enable */
	uint8_t pasid_en;
	/** @dis_intr_on_vf - Disbale interrupt with VF */
	uint8_t dis_intr_on_vf;
	/** @virtio_en - Queue is in Virtio Mode */
	uint8_t virtio_en;
	/** @pack_byp_out - descs on desc output interface can be packed */
	uint8_t pack_byp_out;
	/** @irq_byp - IRQ Bypass mode */
	uint8_t irq_byp;
	/** @host_id - Host ID */
	uint8_t host_id;
	/** @pasid - PASID */
	uint32_t pasid;
	/** @virtio_dsc_base - Virtio Desc Base Address */
	uint64_t virtio_dsc_base;
};

/**
 * struct qdma_descq_hw_ctxt - descq hw context config data structure
 */
struct qdma_descq_hw_ctxt {
	/** @cidx - consumer index */
	uint16_t cidx;
	/** @crd_use - credits consumed */
	uint16_t crd_use;
	/** @dsc_pend - descriptors pending */
	uint8_t dsc_pend;
	/** @idl_stp_b -Queue invalid and no descriptors pending */
	uint8_t idl_stp_b;
	/** @evt_pnd - Event pending */
	uint8_t evt_pnd;
	/** @fetch_pnd -Descriptor fetch pending */
	uint8_t fetch_pnd;
};

/**
 * struct qdma_descq_credit_ctxt - descq credit context config data structure
 */
struct qdma_descq_credit_ctxt {

	/** @credit -Fetch credits received. */
	uint32_t credit;
};

/**
 * struct qdma_descq_prefetch_ctxt - descq pfetch context config data structure
 */
struct qdma_descq_prefetch_ctxt {
	/** @sw_crdt -Software credit */
	uint16_t sw_crdt;
	/** @bypass - bypass enable */
	uint8_t bypass;
	/** @bufsz_idx - c2h buffer size index */
	uint8_t bufsz_idx;
	/** @port_id - port ID */
	uint8_t port_id;
	/** @var_desc - Variable Descriptor */
	uint8_t var_desc;
	/** @num_pftch - Number of descs prefetched */
	uint16_t num_pftch;
	/** @err -Error detected on this queue */
	uint8_t err;
	/** @pfch_en - Enable prefetch */
	uint8_t pfch_en;
	/** @pfch - Queue is in prefetch */
	uint8_t pfch;
	/** @valid - context is valid */
	uint8_t valid;
};

/**
 * struct qdma_descq_cmpt_ctxt - descq completion context config data structure
 */
struct qdma_descq_cmpt_ctxt {
	/** @bs_addr - completion ring base address */
	uint64_t bs_addr;
	/** @vec - Interrupt Vector */
	uint16_t vec;
	/** @pidx_l - producer index low */
	uint16_t pidx;
	/** @cidx - consumer index */
	uint16_t cidx;
	/** @en_stat_desc - Enable Completion Status writes */
	uint8_t en_stat_desc;
	/** @en_int - Enable Completion interrupts */
	uint8_t en_int;
	/** @trig_mode - Interrupt and Completion Status Write Trigger Mode */
	uint8_t trig_mode;
	/** @fnc_id - Function ID */
	uint8_t fnc_id;
	/** @counter_idx - Index to counter register */
	uint8_t counter_idx;
	/** @timer_idx - Index to timer register */
	uint8_t timer_idx;
	/** @in_st - Interrupt State */
	uint8_t in_st;
	/** @color - initial color bit to be used on Completion */
	uint8_t color;
	/** @ringsz_idx - Completion ring size index to ring size registers */
	uint8_t ringsz_idx;
	/** @desc_sz  -descriptor size */
	uint8_t desc_sz;
	/** @valid  - context valid */
	uint8_t valid;
	/** @err - error status */
	uint8_t err;
	/**
	 * @user_trig_pend - user logic initiated interrupt is
	 * pending to be generate
	 */
	uint8_t user_trig_pend;
	/** @timer_running - timer is running on this queue */
	uint8_t timer_running;
	/** @full_upd - Full update */
	uint8_t full_upd;
	/** @ovf_chk_dis - Completion Ring Overflow Check Disable */
	uint8_t ovf_chk_dis;
	/** @at -Address Translation */
	uint8_t at;
	/** @int_aggr -Interrupt Aggregation */
	uint8_t int_aggr;
	/** @dis_intr_on_vf - Disbale interrupt with VF */
	uint8_t dis_intr_on_vf;
	/** @vio - queue is in VirtIO mode */
	uint8_t vio;
	/** @dir_c2h - DMA direction is C2H */
	uint8_t dir_c2h;
	/** @host_id - Host ID */
	uint8_t host_id;
	/** @pasid - PASID */
	uint32_t pasid;
	/** @pasid_en - PASID Enable */
	uint8_t pasid_en;
	/** @vio_eop - Virtio End-of-packet */
	uint8_t vio_eop;
	/** @sh_cmpt - Shared Completion Queue */
	uint8_t sh_cmpt;
};

/**
 * struct qdma_indirect_intr_ctxt - indirect interrupt context config data
 * structure
 */
struct qdma_indirect_intr_ctxt {
	/** @baddr_4k -Base address of Interrupt Aggregation Ring */
	uint64_t baddr_4k;
	/** @vec - Interrupt vector index in msix table */
	uint16_t vec;
	/** @pidx - Producer Index */
	uint16_t pidx;
	/** @valid - context valid */
	uint8_t valid;
	/** @int_st -Interrupt State */
	uint8_t int_st;
	/** @color - Color bit */
	uint8_t color;
	/** @page_size - Interrupt Aggregation Ring size */
	uint8_t page_size;
	/** @at - Address translation */
	uint8_t at;
	/** @host_id - Host ID */
	uint8_t host_id;
	/** @pasid - PASID */
	uint32_t pasid;
	/** @pasid_en - PASID Enable */
	uint8_t pasid_en;
	/** @func_id - Function ID */
	uint16_t func_id;
};

struct qdma_hw_version_info {
	/** @rtl_version - RTL Version */
	enum qdma_rtl_version rtl_version;
	/** @vivado_release - Vivado Release id */
	enum qdma_vivado_release_id vivado_release;
	/** @versal_ip_state - Versal IP state */
	enum qdma_ip_type ip_type;
	/** @device_type - Device Type */
	enum qdma_device_type device_type;
	/** @qdma_rtl_version_str - RTL Version string*/
	char qdma_rtl_version_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_vivado_release_id_str - Vivado Release id string*/
	char qdma_vivado_release_id_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_device_type_str - Qdma device type string*/
	char qdma_device_type_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_versal_ip_state_str - Versal IP state string*/
	char qdma_ip_type_str[QDMA_HW_VERSION_STRING_LEN];
};

#define CTXT_ENTRY_NAME_SZ        64
struct qctx_entry {
	char		name[CTXT_ENTRY_NAME_SZ];
	uint32_t	value;
};

/**
 * @struct - qdma_descq_context
 * @brief	queue context information
 */
struct qdma_descq_context {
	struct qdma_qid2vec qid2vec;
	struct qdma_fmap_cfg fmap;
	struct qdma_descq_sw_ctxt sw_ctxt;
	struct qdma_descq_hw_ctxt hw_ctxt;
	struct qdma_descq_credit_ctxt cr_ctxt;
	struct qdma_descq_prefetch_ctxt pfetch_ctxt;
	struct qdma_descq_cmpt_ctxt cmpt_ctxt;
};

/**
 * struct qdma_q_pidx_reg_info - Software PIDX register fields
 */
struct qdma_q_pidx_reg_info {
	/** @pidx - Producer Index */
	uint16_t pidx;
	/** @irq_en - Interrupt enable */
	uint8_t irq_en;
};

/**
 * struct qdma_q_intr_cidx_reg_info - Interrupt Ring CIDX register fields
 */
struct qdma_intr_cidx_reg_info {
	/** @sw_cidx - Software Consumer Index */
	uint16_t sw_cidx;
	/** @rng_idx - Ring Index of the Interrupt Aggregation ring */
	uint8_t rng_idx;
};

/**
 * struct qdma_q_cmpt_cidx_reg_info - CMPT CIDX register fields
 */
struct qdma_q_cmpt_cidx_reg_info {
	/** @wrb_cidx - CMPT Consumer Index */
	uint16_t wrb_cidx;
	/** @counter_idx - Counter Threshold Index */
	uint8_t counter_idx;
	/** @timer_idx - Timer Count Index */
	uint8_t timer_idx;
	/** @trig_mode - Trigger mode */
	uint8_t trig_mode;
	/** @wrb_en - Enable status descriptor for CMPT */
	uint8_t wrb_en;
	/** @irq_en - Enable Interrupt for CMPT */
	uint8_t irq_en;
};


/**
 * struct qdma_csr_info - Global CSR info data structure
 */
struct qdma_csr_info {
	/** @ringsz: ring size values */
	uint16_t ringsz[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** @bufsz: buffer size values */
	uint16_t bufsz[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** @timer_cnt: timer threshold values */
	uint8_t timer_cnt[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** @cnt_thres: counter threshold values */
	uint8_t cnt_thres[QDMA_GLOBAL_CSR_ARRAY_SZ];
	/** @wb_intvl: writeback interval */
	uint8_t wb_intvl;
};

#define QDMA_MAX_REGISTER_DUMP	14

/**
 * struct qdma_reg_data - Structure to
 * hold address value and pair
 */
struct qdma_reg_data {
	/** @reg_addr: register address */
	uint32_t reg_addr;
	/** @reg_val: register value */
	uint32_t reg_val;
};

/**
 * enum qdma_hw_access_type - To hold hw access type
 */
enum qdma_hw_access_type {
	QDMA_HW_ACCESS_READ,
	QDMA_HW_ACCESS_WRITE,
	QDMA_HW_ACCESS_CLEAR,
	QDMA_HW_ACCESS_INVALIDATE,
	QDMA_HW_ACCESS_MAX
};

/**
 * enum qdma_global_csr_type - To hold global csr type
 */
enum qdma_global_csr_type {
	QDMA_CSR_RING_SZ,
	QDMA_CSR_TIMER_CNT,
	QDMA_CSR_CNT_TH,
	QDMA_CSR_BUF_SZ,
	QDMA_CSR_MAX
};

/**
 * enum status_type - To hold enable/disable status type
 */
enum status_type {
	DISABLE = 0,
	ENABLE = 1,
};

/**
 * enum qdma_reg_read_type - Indicates reg read type
 */
enum qdma_reg_read_type {
	/** @QDMA_REG_READ_PF_ONLY: Read the register for PFs only */
	QDMA_REG_READ_PF_ONLY,
	/** @QDMA_REG_READ_VF_ONLY: Read the register for VFs only */
	QDMA_REG_READ_VF_ONLY,
	/** @QDMA_REG_READ_PF_VF: Read the register for both PF and VF */
	QDMA_REG_READ_PF_VF,
	/** @QDMA_REG_READ_MAX: Reg read enum max */
	QDMA_REG_READ_MAX
};

/**
 * enum qdma_reg_read_groups - Indicates reg read groups
 */
enum qdma_reg_read_groups {
	/** @QDMA_REG_READ_GROUP_1: Read the register from  0x000 to 0x288 */
	QDMA_REG_READ_GROUP_1,
	/** @QDMA_REG_READ_GROUP_2: Read the register from 0x400 to 0xAFC */
	QDMA_REG_READ_GROUP_2,
	/** @QDMA_REG_READ_GROUP_3: Read the register from 0xB00 to 0xE28 */
	QDMA_REG_READ_GROUP_3,
	/** @QDMA_REG_READ_GROUP_4: Read the register Mailbox Registers */
	QDMA_REG_READ_GROUP_4,
	/** @QDMA_REG_READ_GROUP_MAX: Reg read max groups */
	QDMA_REG_READ_GROUP_MAX
};

void qdma_write_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, const uint32_t *values);

void qdma_read_csr_values(void *dev_hndl, uint32_t reg_offst,
		uint32_t idx, uint32_t cnt, uint32_t *values);

int dump_reg(char *buf, int buf_sz, uint32_t raddr,
		const char *rname, uint32_t rval);

int hw_monitor_reg(void *dev_hndl, uint32_t reg, uint32_t mask,
		uint32_t val, uint32_t interval_us,
		uint32_t timeout_us);

void qdma_memset(void *to, uint8_t val, uint32_t size);

int qdma_acc_reg_dump_buf_len(void *dev_hndl,
		enum qdma_ip_type ip_type, int *buflen);

int qdma_acc_reg_info_len(void *dev_hndl,
		enum qdma_ip_type ip_type, int *buflen, int *num_regs);

int qdma_acc_context_buf_len(void *dev_hndl,
		enum qdma_ip_type ip_type, uint8_t st,
		enum qdma_dev_q_type q_type, uint32_t *buflen);

int qdma_acc_get_num_config_regs(void *dev_hndl,
		enum qdma_ip_type ip_type, uint32_t *num_regs);

/*
 * struct qdma_hw_access - Structure to hold HW access function pointers
 */
struct qdma_hw_access {
	int (*qdma_set_default_global_csr)(void *dev_hndl);
	int (*qdma_global_csr_conf)(void *dev_hndl, uint8_t index,
					uint8_t count, uint32_t *csr_val,
					enum qdma_global_csr_type csr_type,
					enum qdma_hw_access_type access_type);
	int (*qdma_global_writeback_interval_conf)(void *dev_hndl,
					enum qdma_wrb_interval *wb_int,
					enum qdma_hw_access_type access_type);
	int (*qdma_init_ctxt_memory)(void *dev_hndl);
	int (*qdma_qid2vec_conf)(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
				 struct qdma_qid2vec *ctxt,
				 enum qdma_hw_access_type access_type);
	int (*qdma_fmap_conf)(void *dev_hndl, uint16_t func_id,
					struct qdma_fmap_cfg *config,
					enum qdma_hw_access_type access_type);
	int (*qdma_sw_ctx_conf)(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
					struct qdma_descq_sw_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_pfetch_ctx_conf)(void *dev_hndl, uint16_t hw_qid,
					struct qdma_descq_prefetch_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_cmpt_ctx_conf)(void *dev_hndl, uint16_t hw_qid,
					struct qdma_descq_cmpt_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_hw_ctx_conf)(void *dev_hndl, uint8_t c2h, uint16_t hw_qid,
					struct qdma_descq_hw_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_credit_ctx_conf)(void *dev_hndl, uint8_t c2h,
					uint16_t hw_qid,
					struct qdma_descq_credit_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_indirect_intr_ctx_conf)(void *dev_hndl, uint16_t ring_index,
					struct qdma_indirect_intr_ctxt *ctxt,
					enum qdma_hw_access_type access_type);
	int (*qdma_queue_pidx_update)(void *dev_hndl, uint8_t is_vf,
				uint16_t qid,
				uint8_t is_c2h,
				const struct qdma_q_pidx_reg_info *reg_info);
	int (*qdma_queue_cmpt_cidx_read)(void *dev_hndl, uint8_t is_vf,
				uint16_t qid,
				struct qdma_q_cmpt_cidx_reg_info *reg_info);
	int (*qdma_queue_cmpt_cidx_update)(void *dev_hndl, uint8_t is_vf,
			uint16_t qid,
			const struct qdma_q_cmpt_cidx_reg_info *reg_info);
	int (*qdma_queue_intr_cidx_update)(void *dev_hndl, uint8_t is_vf,
				uint16_t qid,
				const struct qdma_intr_cidx_reg_info *reg_info);
	int (*qdma_mm_channel_conf)(void *dev_hndl, uint8_t channel,
				uint8_t is_c2h, uint8_t enable);
	int (*qdma_get_user_bar)(void *dev_hndl, uint8_t is_vf,
				uint8_t func_id, uint8_t *user_bar);
	int (*qdma_get_function_number)(void *dev_hndl, uint8_t *func_id);
	int (*qdma_get_version)(void *dev_hndl, uint8_t is_vf,
				struct qdma_hw_version_info *version_info);
	int (*qdma_get_device_attributes)(void *dev_hndl,
					struct qdma_dev_attributes *dev_info);
	int (*qdma_hw_error_intr_setup)(void *dev_hndl, uint16_t func_id,
					uint8_t err_intr_index);
	int (*qdma_hw_error_intr_rearm)(void *dev_hndl);
	int (*qdma_hw_error_enable)(void *dev_hndl,
			uint32_t err_idx);
	const char *(*qdma_hw_get_error_name)(uint32_t err_idx);
	int (*qdma_hw_error_process)(void *dev_hndl);
	int (*qdma_dump_config_regs)(void *dev_hndl, uint8_t is_vf, char *buf,
					uint32_t buflen);
	int (*qdma_dump_reg_info)(void *dev_hndl, uint32_t reg_addr,
				  uint32_t num_regs,
				  char *buf,
				  uint32_t buflen);
	int (*qdma_dump_queue_context)(void *dev_hndl,
			uint8_t st,
			enum qdma_dev_q_type q_type,
			struct qdma_descq_context *ctxt_data,
			char *buf, uint32_t buflen);
	int (*qdma_read_dump_queue_context)(void *dev_hndl,
			uint16_t qid_hw,
			uint8_t st,
			enum qdma_dev_q_type q_type,
			char *buf, uint32_t buflen);
	int (*qdma_dump_intr_context)(void *dev_hndl,
			struct qdma_indirect_intr_ctxt *intr_ctx,
			int ring_index,
			char *buf, uint32_t buflen);
	int (*qdma_is_legacy_intr_pend)(void *dev_hndl);
	int (*qdma_clear_pend_legacy_intr)(void *dev_hndl);
	int (*qdma_legacy_intr_conf)(void *dev_hndl, enum status_type enable);
	int (*qdma_initiate_flr)(void *dev_hndl, uint8_t is_vf);
	int (*qdma_is_flr_done)(void *dev_hndl, uint8_t is_vf, uint8_t *done);
	int (*qdma_get_error_code)(int acc_err_code);
	int (*qdma_read_reg_list)(void *dev_hndl, uint8_t is_vf,
			uint16_t reg_rd_group,
			uint16_t *total_regs,
			struct qdma_reg_data *reg_list);
	int (*qdma_dump_config_reg_list)(void *dev_hndl,
			uint32_t num_regs,
			struct qdma_reg_data *reg_list,
			char *buf, uint32_t buflen);
	uint32_t mbox_base_pf;
	uint32_t mbox_base_vf;
	uint32_t qdma_max_errors;
};

/*****************************************************************************/
/**
 * qdma_hw_access_init() - Function to get the QDMA hardware
 *			access function pointers
 *	This function should be called once per device from
 *	device_open()/probe(). Caller shall allocate memory for
 *	qdma_hw_access structure and store pointer to it in their
 *	per device structure. Config BAR validation will be done
 *	inside this function
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 * @hw_access: qdma_hw_access structure pointer.
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_hw_access_init(void *dev_hndl, uint8_t is_vf,
				struct qdma_hw_access *hw_access);

/*****************************************************************************/
/**
 * qdma_acc_dump_config_regs() - Function to get qdma config registers
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @ip_type:	QDMA IP Type
 * @reg_data:  pointer to register data to be filled
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_get_config_regs(void *dev_hndl, uint8_t is_vf,
		enum qdma_ip_type ip_type,
		uint32_t *reg_data);

/*****************************************************************************/
/**
 * qdma_acc_dump_config_regs() - Function to get qdma config register dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @ip_type:	QDMA IP Type
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		enum qdma_ip_type ip_type,
		char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_acc_dump_reg_info() - Function to get qdma reg info in a buffer
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP Type
 * @reg_addr:   Register Address
 * @num_regs:   Number of Registers
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_reg_info(void *dev_hndl,
		enum qdma_ip_type ip_type, uint32_t reg_addr,
		uint32_t num_regs, char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_acc_dump_queue_context() - Function to dump qdma queue context data in a
 * buffer where context information is already available in 'ctxt_data'
 * structure pointer buffer
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP Type
 * @st:		ST or MM
 * @q_type:	Queue Type
 * @ctxt_data:	Context Data
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_dump_queue_context(void *dev_hndl,
		enum qdma_ip_type ip_type,
		uint8_t st,
		enum qdma_dev_q_type q_type,
		struct qdma_descq_context *ctxt_data,
		char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_acc_read_dump_queue_context() - Function to read and dump the queue
 * context in a buffer
 *
 * @dev_hndl:   device handle
 * @ip_type:	QDMA IP Type
 * @qid_hw:     queue id
 * @st:		ST or MM
 * @q_type:	Queue Type
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_acc_read_dump_queue_context(void *dev_hndl,
				enum qdma_ip_type ip_type,
				uint16_t qid_hw,
				uint8_t st,
				enum qdma_dev_q_type q_type,
				char *buf, uint32_t buflen);


/*****************************************************************************/
/**
 * qdma_acc_dump_config_reg_list() - Dump the registers
 *
 * @dev_hndl:		device handle
 * @ip_type:		QDMA IP Type
 * @total_regs :	Max registers to read
 * @reg_list :		array of reg addr and reg values
 * @buf :		pointer to buffer to be filled
 * @buflen :		Length of the buffer
 *
 * Return: returns the platform specific error code
 *****************************************************************************/
int qdma_acc_dump_config_reg_list(void *dev_hndl,
		enum qdma_ip_type ip_type,
		uint32_t num_regs,
		struct qdma_reg_data *reg_list,
		char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_get_error_code() - function to get the qdma access mapped
 *				error code
 *
 * @acc_err_code: qdma access error code
 *
 * Return:   returns the platform specific error code
 *****************************************************************************/
int qdma_get_error_code(int acc_err_code);

/*****************************************************************************/
/**
 * qdma_fetch_version_details() - Function to fetch the version details from the
 *  version register value
 *
 * @is_vf           :    Whether PF or VF
 * @version_reg_val :    Value of the version register
 * @version_info :       Pointer to store the version details.
 *
 * Return:	Nothing
 *****************************************************************************/
void qdma_fetch_version_details(uint8_t is_vf, uint32_t version_reg_val,
		struct qdma_hw_version_info *version_info);


#ifdef __cplusplus
}
#endif

#endif /* QDMA_ACCESS_COMMON_H_ */
