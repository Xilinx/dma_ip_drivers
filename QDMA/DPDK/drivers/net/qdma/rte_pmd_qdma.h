/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef __RTE_PMD_QDMA_EXPORT_H__
#define __RTE_PMD_QDMA_EXPORT_H__

#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>

/*Enums*/

enum rte_pmd_qdma_rx_bypass_mode {
	RTE_PMD_QDMA_RX_BYPASS_NONE = 0,
	RTE_PMD_QDMA_RX_BYPASS_CACHE = 1,
	RTE_PMD_QDMA_RX_BYPASS_SIMPLE = 2,
	RTE_PMD_QDMA_RX_BYPASS_MAX
};

enum rte_pmd_qdma_tx_bypass_mode {
	RTE_PMD_QDMA_TX_BYPASS_NONE = 0,
	RTE_PMD_QDMA_TX_BYPASS_ENABLE = 1,
	RTE_PMD_QDMA_TX_BYPASS_MAX
};

///Enum to specify the direction i.e. TX or RX
enum rte_pmd_qdma_dir_type {
	RTE_PMD_QDMA_TX = 0,
	RTE_PMD_QDMA_RX,
	RTE_PMD_QDMA_DIR_TYPE_MAX
};

enum rte_pmd_qdma_pci_func_type {
	RTE_PMD_QDMA_PCI_FUNC_PF,
	RTE_PMD_QDMA_PCI_FUNC_VF,
	RTE_PMD_QDMA_PCI_FUNC_TYPE_MAX,
};

/**
 * queue_mode_t - queue modes
 */
enum rte_pmd_qdma_queue_mode_t {
	RTE_PMD_QDMA_MEMORY_MAPPED_MODE,
	RTE_PMD_QDMA_STREAMING_MODE,
	RTE_PMD_QDMA_QUEUE_MODE_MAX,
};

/*
 * tigger_mode_t - trigger modes
 */
enum rte_pmd_qdma_tigger_mode_t {
	RTE_PMD_QDMA_TRIG_MODE_DISABLE,
	RTE_PMD_QDMA_TRIG_MODE_EVERY,
	RTE_PMD_QDMA_TRIG_MODE_USER_COUNT,
	RTE_PMD_QDMA_TRIG_MODE_USER,
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER,
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT,
	RTE_PMD_QDMA_TRIG_MODE_MAX,
};

enum rte_pmd_qdma_cmpt_desc_len {
	RTE_PMD_QDMA_CMPT_DESC_LEN_8B = 8,
	RTE_PMD_QDMA_CMPT_DESC_LEN_16B = 16,
	RTE_PMD_QDMA_CMPT_DESC_LEN_32B = 32,
	RTE_PMD_QDMA_CMPT_DESC_LEN_64B = 64,
	RTE_PMD_QDMA_CMPT_DESC_LEN_MAX,
};

enum rte_pmd_qdma_bypass_desc_len {
	RTE_PMD_QDMA_BYPASS_DESC_LEN_8B = 8,
	RTE_PMD_QDMA_BYPASS_DESC_LEN_16B = 16,
	RTE_PMD_QDMA_BYPASS_DESC_LEN_32B = 32,
	RTE_PMD_QDMA_BYPASS_DESC_LEN_64B = 64,
	RTE_PMD_QDMA_BYPASS_DESC_LEN_MAX,
};

enum rte_pmd_qdma_xdebug_type {
	RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR,
	RTE_PMD_QDMA_XDEBUG_QDMA_DEVICE_STRUCT,
	RTE_PMD_QDMA_XDEBUG_QUEUE_INFO,
	RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP,
	RTE_PMD_QDMA_XDEBUG_MAX,
};

enum rte_pmd_qdma_xdebug_desc_type {
	RTE_PMD_QDMA_XDEBUG_DESC_C2H,
	RTE_PMD_QDMA_XDEBUG_DESC_H2C,
	RTE_PMD_QDMA_XDEBUG_DESC_CMPT,
	RTE_PMD_QDMA_XDEBUG_DESC_MAX,
};

enum rte_pmd_qdma_device_type {
	/** @RTE_PMD_QDMA_DEVICE_SOFT - UltraScale+ IP's  */
	RTE_PMD_QDMA_DEVICE_SOFT,
	/** @RTE_PMD_QDMA_DEVICE_VERSAL -VERSAL IP  */
	RTE_PMD_QDMA_DEVICE_VERSAL,
	/** @RTE_PMD_QDMA_DEVICE_VERSAL_CPM5 - VERSAL CPM5  */
	RTE_PMD_QDMA_DEVICE_VERSAL_CPM5,
	/** @RTE_PMD_QDMA_DEVICE_NONE - Not a valid device  */
	RTE_PMD_QDMA_DEVICE_NONE
};

enum rte_pmd_qdma_versal_ip_type {
	/** @RTE_PMD_QDMA_VERSAL_HARD_IP - Hard IP  */
	RTE_PMD_QDMA_VERSAL_HARD_IP,
	/** @RTE_PMD_QDMA_VERSAL_SOFT_IP - Soft IP  */
	RTE_PMD_QDMA_VERSAL_SOFT_IP,
	/** @RTE_PMD_QDMA_VERSAL_NONE - Not versal device  */
	RTE_PMD_QDMA_VERSAL_NONE
};

struct rte_pmd_qdma_xdebug_desc_param {
	uint16_t queue;
	int start;
	int end;
	enum rte_pmd_qdma_xdebug_desc_type type;
};

/**
 * struct rte_pmd_qdma_dev_attributes - QDMA device attributes
 */
struct rte_pmd_qdma_dev_attributes {
	/** @num_pfs - Num of PFs*/
	uint8_t num_pfs;
	/** @num_qs - Num of Queues */
	uint16_t num_qs;
	/** @flr_present - FLR resent or not? */
	uint8_t flr_present:1;
	/** @st_en - ST mode supported or not? */
	uint8_t st_en:1;
	/** @mm_en - MM mode supported or not? */
	uint8_t mm_en:1;
	/** @mm_cmpt_en - MM with Completions supported or not? */
	uint8_t mm_cmpt_en:1;
	/** @mailbox_en - Mailbox supported or not? */
	uint8_t mailbox_en:1;
	/** @mm_channel_max - Num of MM channels */
	uint8_t mm_channel_max;

	/** @cmpt_ovf_chk_dis - To indicate support of
	 *overflow check disable in CMPT ring
	 */
	uint8_t cmpt_ovf_chk_dis:1;
	/** @sw_desc_64b - To indicate support of 64 bytes
	 *C2H/H2C descriptor format
	 */
	uint8_t sw_desc_64b:1;
	/** @cmpt_desc_64b - To indicate support of 64 bytes
	 *CMPT descriptor format
	 */
	uint8_t cmpt_desc_64b:1;
	/** @cmpt_trig_count_timer - To indicate support of
	 *counter + timer trigger mode
	 */
	uint8_t cmpt_trig_count_timer:1;
	/** @device_type - Device Type */
	enum rte_pmd_qdma_device_type device_type;
	/** @versal_ip_type - Versal IP Type */
	enum rte_pmd_qdma_versal_ip_type versal_ip_type;
};

/*Functions*/
/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dbg_regdump
 * Description:		Dumps the QDMA configuration registers
 *			for the given port.
 *
 * @param	portid : Port ID
 *
 * @return	'0' on success and "< 0" on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_dbg_regdump(uint8_t port_id);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dbg_qdevice
 * Description:		Dumps the device specific SW structure
 *			for the given port.
 *
 * @param	portid : Port ID
 *
 * @return	'0' on success and "< 0" on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_dbg_qdevice(uint8_t port_id);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dbg_qinfo
 * Description:		Dumps the queue contexts and queue specific SW
 *			structures for the given queue ID.
 *
 * @param	portid : Port ID
 * @param	queue  : Queue ID relative to the Port
 *
 * @return	'0' on success and "< 0" on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_dbg_qinfo(uint8_t port_id, uint16_t queue);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dbg_qdesc
 * Description:		Dumps the Queue descriptors.
 *
 * @param	portid : Port ID
 * @param	queue  : Queue ID relative to the Port
 * @param	start  : start index of the descriptor to dump
 * @param	end    : end index of the descriptor to dump
 * @param	type   : Descriptor type
 *
 * @return	'0' on success and "< 0" on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_dbg_qdesc(uint8_t port_id, uint16_t queue, int start,
			int end, enum rte_pmd_qdma_xdebug_desc_type type);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_bar_details
 * Description:		Returns the BAR indices of the QDMA BARs
 *
 * @param	portid : Port ID
 * @param	config_bar_idx : Config BAR index
 * @param	user_bar_idx   : User BAR index
 * @param	bypass_bar_idx : Bypass BAR index
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_get_bar_details(int portid, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_queue_base
 * Description:		Returns queue base for given port
 *
 * @param	portid : Port ID.
 * @param	queue_base : queue base.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Application can call this API only after successful
 *          call to rte_eh_dev_configure() API.
 ******************************************************************************/
int rte_pmd_qdma_get_queue_base(int portid, uint32_t *queue_base);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_pci_func_type
 * Description:		Retrieves PCIe function type i.e. PF or VF
 *
 * @param	portid : Port ID.
 * @param	func_type : Indicates PCIe function type.
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note    Returns the PCIe function type i.e. PF or VF of the given port.
 ******************************************************************************/
int rte_pmd_qdma_get_pci_func_type(int portid,
		enum rte_pmd_qdma_pci_func_type *func_type);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_immediate_data_state
 * Description:		Returns immediate data state
 *			i.e. whether enabled or disabled, for the specified
 *			queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	state : Pointer to the state specifying whether
 *			immediate data is enabled or not
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this function after
 *		rte_eth_rx_queue_setup() is called.
 *		API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_get_immediate_data_state(int portid, uint32_t qid,
			int *state);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_queue_mode
 * Description:		Sets queue interface mode for the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	mode : Queue interface mode to be set
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before
 *		rte_eth_tx_queue_setup()/rte_eth_rx_queue_setup() API.
 *		By default, all queues are setup in streaming mode.
 ******************************************************************************/
int rte_pmd_qdma_set_queue_mode(int portid, uint32_t qid,
			enum rte_pmd_qdma_queue_mode_t mode);

/******************************************************************************/
/**
 *Function Name:	rte_pmd_qdma_set_immediate_data_state
 *Description:		Sets immediate data state
 *			i.e. enable or disable, for the specified queue.
 *			If enabled, the user defined data in the completion
 *			ring are dumped in to a queue specific file
 *			"q_<qid>_immmediate_data.txt" in the local directory.
 *
 *@param	portid : Port ID.
 *@param	qid : Queue ID.
 *@param	value :	Immediate data state to be set.
 *			Set '0' to disable and '1' to enable
 *
 *@return	'0' on success and '< 0' on failure.
 *
 *@note		Application can call this API after successful
 *		call to rte_eth_rx_queue_setup() API.
 *		This API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_set_immediate_data_state(int portid, uint32_t qid,
			uint8_t state);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_overflow_check
 * Description:		Enables or disables the overflow check
 *			(whether PIDX is overflowing the CIDX) performed by
 *			QDMA on the completion descriptor ring of specified
 *			queue.
 *
 * @param	portid : Port ID.
 * @param	qid	   : Queue ID.
 * @param	enable :  '1' to enable and '0' to disable the overflow check
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_overflow_check(int portid, uint32_t qid,
			uint8_t enable);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_descriptor_size
 * Description:		Configures the completion ring descriptor size
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	size : Descriptor size to be configured
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API
 *		when queue is in streaming mode, and before
 *		rte_pmd_qdma_dev_cmptq_setup when queue is in
 *		memory mapped mode.
 *		By default, the completion desciptor size is set to 8 bytes.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_descriptor_size(int portid, uint32_t qid,
			enum rte_pmd_qdma_cmpt_desc_len size);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_trigger_mode
 * Description:		Configures the trigger mode for completion ring CIDX
 *			updates
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	mode : Trigger mode to be configured
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 *		By default, trigger mode is set to
 *		RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_trigger_mode(int portid, uint32_t qid,
			enum rte_pmd_qdma_tigger_mode_t mode);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_cmpt_timer
 * Description:		Configures the timer interval in microseconds to trigger
 *			the completion ring CIDX updates
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	value : Timer interval for completion trigger to be configured
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_timer(int portid, uint32_t qid, uint32_t value);

/******************************************************************************/
/**
 *Function Name:	rte_pmd_qdma_set_c2h_descriptor_prefetch
 *Description:		Enables or disables prefetch of the descriptors by
 *			prefetch engine
 *
 *@param	portid : Port ID.
 *@param	qid : Queue ID.
 *@param	enable:'1' to enable and '0' to disable the descriptor prefetch
 *
 *@return	'0' on success and '< 0' on failure.
 *
 *@note		Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_c2h_descriptor_prefetch(int portid, uint32_t qid,
			uint8_t enable);

/*****************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_set_mm_endpoint_addr
 * Description:		Sets the PCIe endpoint memory offset at which to
 *			perform DMA operation for the specified queue operating
 *			in memory mapped mode.
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	dir : direction i.e. Tx or Rx.
 * @param	addr : Destination address for Tx, Source address for Rx
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	This API can be called before Tx/Rx burst API's
 *		(rte_eth_tx_burst() and rte_eth_rx_burst()) are called.
 *****************************************************************************/
int rte_pmd_qdma_set_mm_endpoint_addr(int portid, uint32_t qid,
			enum rte_pmd_qdma_dir_type dir, uint32_t addr);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_tx_bypass
 * Description:		Sets the Tx bypass mode and bypass descriptor size
 *			for the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	bypass_mode : Bypass mode to be set
 * @param	size : Bypass descriptor size to be set
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before tx_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_tx_bypass(int portid, uint32_t qid,
			enum rte_pmd_qdma_tx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_configure_rx_bypass
 * Description:		Sets the Rx bypass mode and bypass descriptor size for
 *			the specified queue
 *
 * @param	portid : Port ID.
 * @param	qid : Queue ID.
 * @param	bypass_mode : Bypass mode to be set
 * @param	size : Bypass descriptor size to be set
 *
 * @return	'0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_rx_bypass(int portid, uint32_t qid,
			enum rte_pmd_qdma_rx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_get_device_capabilities
 * Description:		Retrive the device capabilities
 *
 * @param   portid : Port ID.
 * @param   dev_attr:Pointer to the device capabilities structure
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	None.
 ******************************************************************************/
int rte_pmd_qdma_get_device_capabilities(int portid,
			struct rte_pmd_qdma_dev_attributes *dev_attr);

/******************************************************************************/
/**
 * Function Name:	rte_pmd_qdma_dev_cmptq_setup
 * Description:		Allocate and set up a completion queue for memory
 *			mapped mode.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 * @param   nb_cmpt_desc: Completion queue ring size.
 * @param   socket_id : The socket_id argument is the socket identifier
 *			in case of NUMA. Its value can be SOCKET_ID_ANY
 *			if there is no NUMA constraint for the DMA memory
 *			allocated for the transmit descriptors of the ring.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() and rte_pmd_qdma_set_queue_mode()
 *		for queues in memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_setup(int portid, uint32_t qid,
					uint16_t nb_cmpt_desc,
					unsigned int socket_id);

/******************************************************************************/
/**
 * Function Name:   rte_pmd_qdma_dev_cmptq_start
 * Description:     Start the MM completion queue.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note    Application can call this API after successful call to
 *          rte_pmd_qdma_dev_cmptq_setup() API when queue is in
 *          memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_start(int portid, uint32_t qid);

/******************************************************************************/
/**
 * Function Name:   rte_pmd_qdma_dev_cmptq_stop
 * Description:     Stop the MM completion queue.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 *
 * @return  '0' on success and '< 0' on failure.
 *
 * @note    Application can call this API after successful call to
 *          rte_pmd_qdma_dev_cmptq_start() API when queue is in
 *          memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_stop(int portid, uint32_t qid);

/*****************************************************************************/
/**
 * Function Name:   rte_pmd_qdma_mm_cmpt_process
 * Description:     Process the MM Completion queue entries.
 *
 * @param   portid : Port ID.
 * @param   qid : Queue ID.
 * @param   cmpt_buff : User buffer pointer to store the completion data.
 * @param   nb_entries: Number of compeltion entries to process.
 *
 * @return  'number of entries processed' on success and '< 0' on failure.
 *
 * @note    Application can call this API after successful call to
 *	    rte_pmd_qdma_dev_cmptq_start() API.
 ******************************************************************************/
uint16_t rte_pmd_qdma_mm_cmpt_process(int portid, uint32_t qid, void *cmpt_buff,
		uint16_t nb_entries);

#endif /* ifndef __RTE_PMD_QDMA_EXPORT_H__ */
