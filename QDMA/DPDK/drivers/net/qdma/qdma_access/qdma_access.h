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

#ifndef QDMA_ACCESS_H_
#define QDMA_ACCESS_H_

#include "qdma_access_export.h"
#include "qdma_platform_env.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA common library interface definitions
 *
 * Header file *qdma_access.h* defines data structures and function signatures
 * exported by QDMA common library.
 */

/** QDMA Context array size */
#define QDMA_FMAP_NUM_WORDS                    2
#define QDMA_SW_CONTEXT_NUM_WORDS              5
#define QDMA_PFETCH_CONTEXT_NUM_WORDS          2
#define QDMA_CMPT_CONTEXT_NUM_WORDS            5
#define QDMA_HW_CONTEXT_NUM_WORDS              2
#define QDMA_CR_CONTEXT_NUM_WORDS              1
#define QDMA_IND_INTR_CONTEXT_NUM_WORDS        3

/** Error codes */
#define QDMA_SUCCESS                           0
#define QDMA_CONTEXT_BUSY_TIMEOUT_ERR          1
#define QDMA_INVALID_PARAM_ERR                 2
#define QDMA_INVALID_CONFIG_BAR                3
#define QDMA_BUSY_TIMEOUT_ERR                  4
#define QDMA_NO_PENDING_LEGACY_INTERRUPT       5
#define QDMA_BAR_NOT_FOUND                     6
#define QDMA_FEATURE_NOT_SUPPORTED             7
#define QDMA_HW_ERR_NOT_DETECTED               1


#define QDMA_DEV_PF     0
#define QDMA_DEV_VF     1

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
	uint32_t qbase:11;
	/** @ rsvd0 - Reserved */
	uint32_t rsvd0:21;
	/** @qmax - maximum queues in the function */
	uint32_t qmax:12;
	/** @ rsvd1 - Reserved */
	uint32_t rsvd1:20;
};

/**
 * struct qdma_descq_sw_ctxt - descq SW context config data structure
 */
struct qdma_descq_sw_ctxt {

	/** @pidx - initial producer index */
	uint32_t pidx:16;
	/** @irq_arm - Interrupt Arm */
	uint32_t irq_arm:1;
	/** @fnc_id - Function ID */
	uint32_t fnc_id:8;
	/** @rsvd0 - Reserved */
	uint32_t rsvd0:7;
	/** @qen - Indicates that the queue is enabled */
	uint32_t qen:1;
	/** @frcd_en -Enable fetch credit */
	uint32_t frcd_en:1;
	/** @wbi_chk -Writeback/Interrupt after pending check */
	uint32_t wbi_chk:1;
	/** @wbi_intvl_en -Write back/Interrupt interval */
	uint32_t wbi_intvl_en:1;
	/** @at - Address tanslation */
	uint32_t at:1;
	/** @fetch_max - Maximum number of descriptor fetches outstanding */
	uint32_t fetch_max:3;
	/** @rsvd1 - Reserved */
	uint32_t rsvd1:4;
	/** @rngsz_idx - Descriptor ring size index */
	uint32_t rngsz_idx:4;
	/** @desc_sz -Descriptor fetch size */
	uint32_t desc_sz:2;
	/** @bypass - bypass enable */
	uint32_t bypass:1;
	/** @mm_chn - MM channel */
	uint32_t mm_chn:1;
	/** @wbk_en -Writeback enable */
	uint32_t wbk_en:1;
	/** @irq_en -Interrupt enable */
	uint32_t irq_en:1;
	/** @port_id -Port_id */
	uint32_t port_id:3;
	/** @irq_no_last - No interrupt was sent */
	uint32_t irq_no_last:1;
	/** @err - Error status */
	uint32_t err:2;
	/** @err_wb_sent -writeback/interrupt was sent for an error */
	uint32_t err_wb_sent:1;
	/** @irq_req - Interrupt due to error waiting to be sent */
	uint32_t irq_req:1;
	/** @mrkr_dis - Marker disable */
	uint32_t mrkr_dis:1;
	/** @is_mm - MM mode */
	uint32_t is_mm:1;
	/** @ring_bs_addr - ring base address */
	uint64_t ring_bs_addr;
	/** @vec - vector number */
	uint32_t vec:11;
	/** @intr_aggr - interrupt aggregation enable */
	uint32_t intr_aggr:1;
};

/**
 * struct qdma_descq_hw_ctxt - descq hw context config data structure
 */
struct qdma_descq_hw_ctxt {
	/** @cidx - consumer index */
	uint32_t cidx:16;
	/** @crd_use - credits consumed */
	uint32_t crd_use:16;
	/** @rsvd - Reserved */
	uint32_t rsvd:8;
	/** @dsc_pend - descriptors pending */
	uint32_t dsc_pend:1;
	/** @idl_stp_b -Queue invalid and no descriptors pending */
	uint32_t idl_stp_b:1;
	/** @evt_pnd - Event pending */
	uint32_t evt_pnd:1;
	/** @fetch_pnd -Descriptor fetch pending */
	uint32_t fetch_pnd:4;
	/** @rsvd1 - reserved */
	uint32_t rsvd1:1;
};

/**
 * struct qdma_descq_credit_ctxt - descq credit context config data structure
 */
struct qdma_descq_credit_ctxt {

	/** @credit -Fetch credits received. */
	uint32_t credit:16;
	/** @rsvd - reserved */
	uint32_t rsvd:16;
};

/**
 * struct qdma_descq_prefetch_ctxt - descq pfetch context config data structure
 */
struct qdma_descq_prefetch_ctxt {
	/** @bypass - bypass enable */
	uint32_t bypass:1;
	/** @bufsz_idx - c2h buffer size index */
	uint32_t bufsz_idx:4;
	/** @port_id - port ID */
	uint32_t port_id:3;
	/** @rsvd0 - reserved */
	uint32_t rsvd0:18;
	/** @err -Error detected on this queue */
	uint32_t err:1;
	/** @pfch_en - Enable prefetch */
	uint32_t pfch_en:1;
	/** @pfch - Queue is in prefetch */
	uint32_t pfch:1;
	/** @sw_crdt -Software credit */
	uint32_t sw_crdt:16;
	/** @valid - context is valid */
	uint32_t valid:1;
};

/**
 * struct qdma_descq_cmpt_ctxt - descq completion context config data structure
 */
struct qdma_descq_cmpt_ctxt {
	/** @en_stat_desc - Enable Completion Status writes */
	uint32_t en_stat_desc:1;
	/** @en_int - Enable Completion interrupts */
	uint32_t en_int:1;
	/** @trig_mode - Interrupt and Completion Status Write Trigger Mode */
	uint32_t trig_mode:3;
	/** @fnc_id - Function ID */
	uint32_t fnc_id:8;
	/** @rsvd - Reserved */
	uint32_t rsvd:4;
	/** @counter_idx - Index to counter register */
	uint32_t counter_idx:4;
	/** @timer_idx - Index to timer register */
	uint32_t timer_idx:4;
	/** @in_st - Interrupt State */
	uint32_t in_st:2;
	/** @color - initial color bit to be used on Completion */
	uint32_t color:1;
	/** @ringsz_idx - Completion ring size index to ring size registers */
	uint32_t ringsz_idx:4;
	/** @rsvd1 - Reserved */
	uint32_t rsvd1:6;
	/** @bs_addr - completion ring base address */
	uint64_t bs_addr;
	/** @desc_sz  -descriptor size */
	uint32_t desc_sz:2;
	/** @pidx_l - producer index low */
	uint32_t pidx:16;
	/** @cidx - consumer index */
	uint32_t cidx:16;
	/** @valid  - context valid */
	uint32_t valid:1;
	/** @err - error status */
	uint32_t err:2;
	/**
	 * @user_trig_pend - user logic initiated interrupt is
	 * pending to be generate
	 */
	uint32_t user_trig_pend:1;
	/** @timer_running - timer is running on this queue */
	uint32_t timer_running:1;
	/** @full_upd - Full update */
	uint32_t full_upd:1;
	/** @ovf_chk_dis - Completion Ring Overflow Check Disable */
	uint32_t ovf_chk_dis:1;
	/** @at -Address Translation */
	uint32_t at:1;
	/** @vec - Interrupt Vector */
	uint32_t vec:11;
	/** @int_aggr -Interrupt Aggregration */
	uint32_t int_aggr:1;
	/** @rsvd2 - Reserved */
	uint32_t rsvd2:17;
};

/**
 * struct qdma_indirect_intr_ctxt - indirect interrupt context config data
 * structure
 */
struct qdma_indirect_intr_ctxt {
	/** @valid - context valid */
	uint32_t valid:1;
	/** @vec - Interrupt vector index in msix table */
	uint32_t vec:11;
	/** @rsvd - Reserved */
	uint32_t rsvd:1;
	/** @int_st -Interrupt State */
	uint32_t int_st:1;
	/** @color - Color bit */
	uint32_t color:1;
	/** @baddr -Base address of Interrupt Aggregation Ring */
	uint64_t baddr_4k;
	/** @page_size - Interrupt Aggregation Ring size */
	uint32_t page_size:3;
	/** @pidx - Producer Index */
	uint32_t pidx:12;
	/** @at - Address translation */
	uint32_t at:1;
};

/**
 * @struct - qdma_descq_context
 * @brief	queue context information
 */
struct qdma_descq_context {
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
	uint32_t pidx:16;
	/** @irq_en - Interrupt enable */
	uint32_t irq_en:1;
	/** @rsvd - Reserved */
	uint32_t rsvd:15;
};

/**
 * struct qdma_q_intr_cidx_reg_info - Interrupt Ring CIDX register fields
 */
struct qdma_intr_cidx_reg_info {
	/** @sw_cidx - Software Consumer Index */
	uint32_t sw_cidx:16;
	/** @rng_idx - Ring Index of the Interrupt Aggregation ring */
	uint32_t rng_idx:8;
	/** @rsvd - Reserved */
	uint32_t rsvd:8;
};

/**
 * struct qdma_q_cmpt_cidx_reg_info - CMPT CIDX register fields
 */
struct qdma_q_cmpt_cidx_reg_info {
	/** @wrb_cidx - CMPT Consumer Index */
	uint32_t wrb_cidx:16;
	/** @counter_idx - Counter Threshold Index */
	uint32_t counter_idx:4;
	/** @timer_idx - Timer Count Index */
	uint32_t timer_idx:4;
	/** @trig_mode - Trigger mode */
	uint32_t trig_mode:3;
	/** @wrb_en - Enable status descriptor for CMPT */
	uint32_t wrb_en:1;
	/** @irq_en - Enable Interrupt for CMPT */
	uint32_t irq_en:1;
	/** @rsvd - Reserved */
	uint32_t rsvd:3;
};

struct qdma_hw_version_info {
	/** @rtl_version - RTL Version */
	enum qdma_rtl_version rtl_version;
	/** @vivado_release_id - Vivado Release id */
	enum qdma_vivado_release_id vivado_release;
	/** @everest_ip - Everest IP Version */
	enum qdma_everest_ip everest_ip;
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


/*****************************************************************************/
/**
 * qdma_fmap_write() - create fmap context and program it
 *
 * @dev_hndl:   device handle
 * @func_id:    function id of the device
 * @config:	pointer to the fmap data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_fmap_write(void *dev_hndl, uint16_t func_id,
		   const struct qdma_fmap_cfg *config);

/*****************************************************************************/
/**
 * qdma_fmap_read() - read fmap context
 *
 * @dev_hndl:   device handle
 * @func_id:    function id of the device
 * @config:	pointer to the output fmap data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_fmap_read(void *dev_hndl, uint16_t func_id,
			 struct qdma_fmap_cfg *config);

/*****************************************************************************/
/**
 * qdma_fmap_clear() - clear fmap context
 *
 * @dev_hndl:   device handle
 * @func_id:    function id of the device
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_fmap_clear(void *dev_hndl, uint16_t func_id);

/*****************************************************************************/
/**
 * qdma_sw_context_write() - create sw context and program it
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the SW context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_sw_context_write(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 const struct qdma_descq_sw_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_sw_context_read() - read sw context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_sw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid,
			 struct qdma_descq_sw_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_sw_context_clear() - clear sw context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_sw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_sw_context_invalidate() - invalidate sw context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_sw_context_invalidate(void *dev_hndl, uint8_t c2h,
			       uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_pfetch_context_write() - create prefetch context and program it
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the prefetch context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_pfetch_context_write(void *dev_hndl, uint16_t hw_qid,
			     const struct qdma_descq_prefetch_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_pfetch_context_read() - read prefetch context
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_pfetch_context_read(void *dev_hndl, uint16_t hw_qid,
			     struct qdma_descq_prefetch_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_pfetch_context_clear() - clear prefetch context
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_pfetch_context_clear(void *dev_hndl, uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_pfetch_context_invalidate() - invalidate prefetch context
 *
 * @dev_hndl:   device handle
 * @hw_qid:    hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_pfetch_context_invalidate(void *dev_hndl, uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_cmpt_context_write() - create completion context and program it
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the cmpt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_cmpt_context_write(void *dev_hndl, uint16_t hw_qid,
			   const struct qdma_descq_cmpt_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_cmpt_context_read() - read completion context
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	    pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_cmpt_context_read(void *dev_hndl, uint16_t hw_qid,
			   struct qdma_descq_cmpt_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_cmpt_context_clear() - clear completion context
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_cmpt_context_clear(void *dev_hndl, uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_cmpt_context_invalidate() - invalidate completion context
 *
 * @dev_hndl:   device handle
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_cmpt_context_invalidate(void *dev_hndl, uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_hw_context_read() - read hw context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_hw_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid, struct qdma_descq_hw_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_hw_context_clear() - clear hardware context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_hw_context_clear(void *dev_hndl, uint8_t c2h,
			  uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_hw_context_invalidate() - invalidate hardware context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_hw_context_invalidate(void *dev_hndl, uint8_t c2h,
			       uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_credit_context_read() - read credit context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 * @ctxt:	    pointer to the context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_credit_context_read(void *dev_hndl, uint8_t c2h,
			 uint16_t hw_qid, struct qdma_descq_credit_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_credit_context_clear() - clear credit context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_credit_context_clear(void *dev_hndl, uint8_t c2h, uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_credit_context_invalidate() - invalidate credit context
 *
 * @dev_hndl:   device handle
 * @c2h:        is c2h queue
 * @hw_qid:     hardware qid of the queue
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_credit_context_invalidate(void *dev_hndl, uint8_t c2h,
		uint16_t hw_qid);

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_write() - create indirect interrupt context
 *                                and program it
 *
 * @dev_hndl:   device handle
 * @ring_index: indirect interrupt ring index
 * @ctxt:	pointer to the interrupt context data strucutre
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_indirect_intr_context_write(void *dev_hndl, uint16_t ring_index,
			       const struct qdma_indirect_intr_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_read() - read indirect interrupt context
 *
 * @dev_hndl:   device handle
 * @ring_index: indirect interrupt ring index
 * @ctxt:	pointer to the output context data
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_indirect_intr_context_read(void *dev_hndl, uint16_t ring_index,
			       struct qdma_indirect_intr_ctxt *ctxt);

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_clear() - clear indirect interrupt context
 *
 * @dev_hndl:   device handle
 * @ring_index:  indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_indirect_intr_context_clear(void *dev_hndl, uint16_t ring_index);

/*****************************************************************************/
/**
 * qdma_indirect_intr_context_invalidate() - invalidate indirect interrupt
 * context
 *
 * @dev_hndl:   device handle
 * @ring_index:     indirect interrupt ring index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_indirect_intr_context_invalidate(void *dev_hndl, uint16_t ring_index);

/*****************************************************************************/
/**
 * qdma_set_default_global_csr() - function to set the global CSR register to
 * default values. The value can be modified later by using the set/get csr
 * functions
 *
 * @dev_hndl:   device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_default_global_csr(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_set_global_ring_sizes() - function to set the global ring size array
 *
 * @dev_hndl:   device handle
 * @index: Index from where the values needs to written
 * @count: number of entries to be written
 * @glbl_rng_sz: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_global_ring_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		const uint32_t *glbl_rng_sz);

/*****************************************************************************/
/**
 * qdma_get_global_ring_sizes() - function to get the global rng_sz array
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_rng_sz: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_global_ring_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_rng_sz);

/*****************************************************************************/
/**
 * qdma_set_global_timer_count() - function to set the timer values
 *
 * @dev_hndl:   device handle
 * @glbl_tmr_cnt: pointer to the array having the values to write
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_global_timer_count(void *dev_hndl, uint8_t index, uint8_t count,
		const uint32_t *glbl_tmr_cnt);

/*****************************************************************************/
/**
 * qdma_get_global_timer_count() - function to get the timer values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_tmr_cnt: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_global_timer_count(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_tmr_cnt);

/*****************************************************************************/
/**
 * qdma_set_global_counter_threshold() - function to set the counter threshold
 * values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 * @glbl_cnt_th: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_cnt_th);

/*****************************************************************************/
/**
 * qdma_get_global_counter_threshold() - function to get the counter threshold
 * values
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_cnt_th: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_global_counter_threshold(void *dev_hndl, uint8_t index,
		uint8_t count, uint32_t *glbl_cnt_th);

/*****************************************************************************/
/**
 * qdma_set_global_buffer_sizes() - function to set the buffer sizes
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to written
 * @count:	 number of entries to be written
 * @glbl_buf_sz: pointer to the array having the values to write
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_global_buffer_sizes(void *dev_hndl, uint8_t index,
		uint8_t count, const uint32_t *glbl_buf_sz);

/*****************************************************************************/
/**
 * qdma_get_global_buffer_sizes() - function to get the buffer sizes
 *
 * @dev_hndl:   device handle
 * @index:	 Index from where the values needs to read
 * @count:	 number of entries to be read
 * @glbl_buf_sz: pointer to array to hold the values read
 *
 * (index + count) shall not be more than 16
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_global_buffer_sizes(void *dev_hndl, uint8_t index, uint8_t count,
		uint32_t *glbl_buf_sz);

/*****************************************************************************/
/**
 * qdma_set_global_writeback_interval() -  function to set the writeback
 * interval
 *
 * @dev_hndl:   device handle
 * @wb_int:		 Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_set_global_writeback_interval(void *dev_hndl,
		enum qdma_wrb_interval wb_int);

/*****************************************************************************/
/**
 * qdma_get_global_writeback_interval() -  function to get the writeback
 * interval
 *
 * @dev_hndl:   device handle
 * @wb_int:		 pointer to the data to hold Writeback Interval
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_global_writeback_interval(void *dev_hndl,
		enum qdma_wrb_interval *wb_int);

/*****************************************************************************/
/**
 * qdma_queue_pidx_update() - function to update the desc PIDX
 *
 * @dev_hndl:   device handle
 * @is_vf:	 Whether PF or VF
 * @qid:	 Queue id relative to the PF/VF calling this api
 * @is_c2h:	 Whether H2c or C2h?
 * @reg_info:	 data needed for the PIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_queue_pidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		uint8_t is_c2h, const struct qdma_q_pidx_reg_info *reg_info);

/*****************************************************************************/
/**
 * qdma_queue_cmpt_cidx_update() - function to update the CMPT CIDX update
 *
 * @dev_hndl:   device handle
 * @is_vf:	 Whether PF or VF
 * @qid:	 Queue id relative to the PF/VF calling this api
 * @reg_info:		data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_queue_cmpt_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_q_cmpt_cidx_reg_info *reg_info);

/*****************************************************************************/
/**
 * qdma_queue_intr_cidx_update() - function to update the CMPT CIDX update
 *
 * @dev_hndl:   device handle
 * @is_vf:	 Whether PF or VF
 * @qid:	 Queue id relative to the PF/VF calling this api
 * @reg_info:		data needed for the CIDX register update
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_queue_intr_cidx_update(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		const struct qdma_intr_cidx_reg_info *reg_info);

/*****************************************************************************/
/**
 * qdma_queue_cmpt_cidx_read() - function to read the CMPT CIDX register
 *
 * @dev_hndl:   device handle
 * @is_vf:	 Whether PF or VF
 * @qid:	 Queue id relative to the PF/VF calling this api
 * @reg_info: pointer to array to hold the values read
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_queue_cmpt_cidx_read(void *dev_hndl, uint8_t is_vf, uint16_t qid,
		struct qdma_q_cmpt_cidx_reg_info *reg_info);

/*****************************************************************************/
/**
 * qdma_mm_channel_enable() - Function to enable the MM channel
 *
 * @dev_hndl: device handle
 * @channel: MM channel number
 * @is_c2h: Whether H2c or C2h?
 *
 * Presently, we have only 1 MM channel
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
int qdma_mm_channel_enable(void *dev_hndl, uint8_t channel, uint8_t is_c2h);

/*****************************************************************************/
/**
 * qdma_mm_channel_disable() - Function to disable the MM channel
 *
 * @dev_hndl:   device handle
 * @channel:  MM channel number
 * @is_c2h: Whether H2c or C2h?
 *
 * Presently, we have only 1 MM channel
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
int qdma_mm_channel_disable(void *dev_hndl, uint8_t channel, uint8_t is_c2h);

/*****************************************************************************/
/**
 * qdma_is_config_bar() - function for the config bar verification
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_is_config_bar(void *dev_hndl, uint8_t is_vf);

/*****************************************************************************/
/**
 * qdma_get_user_bar() - Function to get the user bar number
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 * @user_bar: pointer to hold the user bar number
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_user_bar(void *dev_hndl, uint8_t is_vf, uint8_t *user_bar);

/*****************************************************************************/
/**
 * qdma_get_function_number() - Function to get the function number
 *
 * @dev_hndl: device handle
 * @func_id: pointer to hold the function id
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_function_number(void *dev_hndl, uint8_t *func_id);

/*****************************************************************************/
/**
 * qdma_get_version() - Function to get the qdma version
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 * @version_info: pointer to hold the version info
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_version(void *dev_hndl, uint8_t is_vf,
		struct qdma_hw_version_info *version_info);

/*****************************************************************************/
/**
 * qdma_get_vivado_release_id() - Function to get the vivado release id in
 * string format
 *
 * @vivado_release_id: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
const char *qdma_get_vivado_release_id(
				enum qdma_vivado_release_id vivado_release_id);

/*****************************************************************************/
/**
 * qdma_get_everest_ip() - Function to get the everest ip version in
 * string format
 *
 * @everest_ip: Everest IP
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
const char *qdma_get_everest_ip(enum qdma_everest_ip everest_ip);

/*****************************************************************************/
/**
 * qdma_get_rtl_version() - Function to get the rtl_version in
 * string format
 *
 * @rtl_version: Vivado release ID
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
const char *qdma_get_rtl_version(enum qdma_rtl_version rtl_version);

/*****************************************************************************/
/**
 * qdma_get_device_attributes() - Function to get the qdma device atributes
 *
 * @dev_hndl: device handle
 * @dev_info: pointer to hold the device info
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_get_device_attributes(void *dev_hndl,
		struct qdma_dev_attributes *dev_info);

/*****************************************************************************/
/**
 * qdma_error_interrupt_setup() - Function to set up the qdma error
 * interrupt module
 *
 * @dev_hndl: device handle
 * @func_id:  Function id
 * @err_intr_index: Interrupt vector
 * @rearm: rearm or not
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_error_interrupt_setup(void *dev_hndl, uint16_t func_id,
		uint8_t err_intr_index);

/*****************************************************************************/
/**
 * qdma_err_intr_rearm() - Function to re-arm the error interrupt
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_error_interrupt_rearm(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_error_interrupt_enable() - Function to enable a particular interrupt
 *
 * @dev_hndl: device handle
 * @err_idx: error index
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_error_enable(void *dev_hndl, enum qdma_error_idx err_idx);

/*****************************************************************************/
/**
 * qdma_get_error_name() - Function to get the error in string format
 *
 * @err_idx: error index
 *
 * Return: string - success and NULL on failure
 *****************************************************************************/
const char *qdma_get_error_name(enum qdma_error_idx err_idx);

/*****************************************************************************/
/**
 * qdma_error_interrupt_process() - Function to find the error that got
 * triggered and call the handler qdma_hw_error_handler of that
 * particular error.
 *
 * @dev_hndl: device handle
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_error_process(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_dump_config_regs() - Function to get qdma config register dump in a
 * buffer
 *
 * @dev_hndl: device handle
 * @buf : pointer to buffer to be filled
 * @buflen : Length of the buffer
 *
 * Return:Length up-till the buffer is filled -success and
 * -ERROR_INSUFFICIENT_BUFFER_SPACE- failure
 *****************************************************************************/
int qdma_dump_config_regs(void *dev_hndl, uint8_t is_vf,
		char *buf, uint32_t buflen);

/*****************************************************************************/
/**
 * qdma_get_legacy_intr_status() - function to get lgcy_intr_pending status bit
 *
 * @dev_hndl: device handle
 *
 * Return: legacy interrupt pending status bit value
 *****************************************************************************/
int qdma_is_legacy_interrupt_pending(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_clr_pending_legacy_intr() - function to clear lgcy_intr_pending bit
 *
 * @dev_hndl: device handle
 *
 * Return: void
 *****************************************************************************/
int qdma_clear_pending_legacy_intrrupt(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_disable_legacy_intr() - function to disable legacy interrupt
 *
 * @dev_hndl: device handle
 *
 * Return: void
 *****************************************************************************/
int qdma_disable_legacy_interrupt(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_enable_legacy_intr() - function to enable legacy interrupt
 *
 * @dev_hndl: device handle
 *
 * Return: void
 *****************************************************************************/
int qdma_enable_legacy_interrupt(void *dev_hndl);

/*****************************************************************************/
/**
 * qdma_initiate_flr() - function to initiate Function Level Reset
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 *
 * Return:	0   - success and < 0 - failure
 *****************************************************************************/
int qdma_initiate_flr(void *dev_hndl, uint8_t is_vf);

/*****************************************************************************/
/**
 * qdma_is_flr_done() - function to check whether the FLR is done or not
 *
 * @dev_hndl: device handle
 * @is_vf: Whether PF or VF
 * @done: if FLR process completed ,  done is 1 else 0.
 *
 * Return:   0   - success and < 0 - failure
 *****************************************************************************/
int qdma_is_flr_done(void *dev_hndl, uint8_t is_vf, uint8_t *done);

#ifdef __cplusplus
}
#endif

#endif /* QDMA_ACCESS_H_ */
