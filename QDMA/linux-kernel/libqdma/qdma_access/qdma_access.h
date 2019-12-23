/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef QDMA_ACCESS_H_
#define QDMA_ACCESS_H_

#include "qdma_access_export.h"
#include "qdma_platform_env.h"
#include "qdma_access_errors.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA common library interface definitions
 *
 * Header file *qdma_access.h* defines data structures and function signatures
 * exported by QDMA common library.
 */

/* QDMA HW version string array length */
#define QDMA_HW_VERSION_STRING_LEN			32


/** Maximum data vectors to be used for each function
 * TODO: Please note that for 2018.2 only one vector would be used
 * per pf and only one ring would be created for this vector
 * It is also assumed that all functions have the same number of data vectors
 * and currently different number of vectors per PF is not supported
 */
#define QDMA_NUM_DATA_VEC_FOR_INTR_CXT  1

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

struct qdma_hw_version_info {
	/** @rtl_version - RTL Version */
	enum qdma_rtl_version rtl_version;
	/** @vivado_release - Vivado Release id */
	enum qdma_vivado_release_id vivado_release;
	/** @versal_ip_state - Versal IP state */
	enum qdma_versal_ip_type versal_ip_type;
	/** @device_type - Device Type */
	enum qdma_device_type device_type;
	/** @qdma_rtl_version_str - RTL Version string*/
	char qdma_rtl_version_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_vivado_release_id_str - Vivado Release id string*/
	char qdma_vivado_release_id_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_device_type_str - Qdma device type string*/
	char qdma_device_type_str[QDMA_HW_VERSION_STRING_LEN];
	/** @qdma_versal_ip_state_str - Versal IP state string*/
	char qdma_versal_ip_type_str[QDMA_HW_VERSION_STRING_LEN];
};

/**
 * struct mbox_csr_info - Global CSR info data structure
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
 * enum qdma_dev_type - To hold qdma device type
 */
enum qdma_dev_type {
	QDMA_DEV_PF,
	QDMA_DEV_VF
};

/**
 * enum status_type - To hold enable/disable status type
 */
enum status_type {
	DISABLE = 0,
	ENABLE = 1,
};

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
					uint8_t *user_bar);
	int (*qdma_get_function_number)(void *dev_hndl, uint8_t *func_id);
	int (*qdma_get_version)(void *dev_hndl, uint8_t is_vf,
				struct qdma_hw_version_info *version_info);
	int (*qdma_get_device_attributes)(void *dev_hndl,
					struct qdma_dev_attributes *dev_info);
	int (*qdma_hw_error_intr_setup)(void *dev_hndl, uint16_t func_id,
					uint8_t err_intr_index);
	int (*qdma_hw_error_intr_rearm)(void *dev_hndl);
	int (*qdma_hw_error_enable)(void *dev_hndl,
					enum qdma_error_idx err_idx);
	const char *(*qdma_hw_get_error_name)(enum qdma_error_idx err_idx);
	int (*qdma_hw_error_process)(void *dev_hndl);
	int (*qdma_dump_config_regs)(void *dev_hndl, uint8_t is_vf, char *buf,
					uint32_t buflen);
	int (*qdma_dump_queue_context)(void *dev_hndl, uint16_t hw_qid,
		uint8_t st, uint8_t c2h, char *buf, uint32_t buflen);
	int (*qdma_is_legacy_intr_pend)(void *dev_hndl);
	int (*qdma_clear_pend_legacy_intr)(void *dev_hndl);
	int (*qdma_legacy_intr_conf)(void *dev_hndl, enum status_type enable);
	int (*qdma_initiate_flr)(void *dev_hndl, uint8_t is_vf);
	int (*qdma_is_flr_done)(void *dev_hndl, uint8_t is_vf, uint8_t *done);
	int (*qdma_get_error_code)(int acc_err_code);

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
 * qdma_dump_config_regs() - Function to get qdma config register dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @is_vf:      Whether PF or VF
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_dump_config_regs(void *dev_hndl, uint8_t is_vf,
	char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_dump_queue_context() - Function to get qdma queue context dump in a
 * buffer
 *
 * @dev_hndl:   device handle
 * @hw_qid:     queue id
 * @buf :       pointer to buffer to be filled
 * @buflen :    Length of the buffer
 *
 * Return:	Length up-till the buffer is filled -success and < 0 - failure
 *****************************************************************************/
int qdma_dump_queue_context(void *dev_hndl, uint16_t hw_qid,
	uint8_t st, uint8_t c2h, char *buf, uint32_t buflen);

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

#ifdef __cplusplus
}
#endif

#endif /* QDMA_ACCESS_H_ */
