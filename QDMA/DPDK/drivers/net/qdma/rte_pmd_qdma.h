/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019-2021 Xilinx, Inc. All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup rte_pmd_qdma_enums Enumerations
 */
/** @defgroup rte_pmd_qdma_struct Data Structures
 */
/** @defgroup rte_pmd_qdma_func Functions
 */

/**
 * Bypass modes in C2H direction
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_rx_bypass_mode {
	/** C2H bypass mode disabled */
	RTE_PMD_QDMA_RX_BYPASS_NONE = 0,
	/** C2H cache bypass mode */
	RTE_PMD_QDMA_RX_BYPASS_CACHE = 1,
	/** C2H simple bypass mode */
	RTE_PMD_QDMA_RX_BYPASS_SIMPLE = 2,
	/** C2H bypass mode invalid */
	RTE_PMD_QDMA_RX_BYPASS_MAX
};

/**
 * Bypass modes in H2C direction
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_tx_bypass_mode {
	/** H2C bypass mode disabled */
	RTE_PMD_QDMA_TX_BYPASS_NONE = 0,
	/** H2C bypass mode enabled */
	RTE_PMD_QDMA_TX_BYPASS_ENABLE = 1,
	/** H2C bypass mode invalid */
	RTE_PMD_QDMA_TX_BYPASS_MAX
};

/**
 * Enum to specify the direction i.e. TX or RX
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_dir_type {
	/** H2C direction */
	RTE_PMD_QDMA_TX = 0,
	/** C2H direction */
	RTE_PMD_QDMA_RX,
	/** Invalid Direction */
	RTE_PMD_QDMA_DIR_TYPE_MAX
};

/**
 * Enum to specify the PCIe function type i.e. PF or VF
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_pci_func_type {
	/** Physical Function */
	RTE_PMD_QDMA_PCI_FUNC_PF,
	/** Virtual Function */
	RTE_PMD_QDMA_PCI_FUNC_VF,
	/** Invalid PCI Function */
	RTE_PMD_QDMA_PCI_FUNC_TYPE_MAX,
};

/**
 * Enum to specify the queue mode
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_queue_mode_t {
	/** Memory mapped queue mode */
	RTE_PMD_QDMA_MEMORY_MAPPED_MODE,
	/** Streaming queue mode */
	RTE_PMD_QDMA_STREAMING_MODE,
	/** Invalid queue mode */
	RTE_PMD_QDMA_QUEUE_MODE_MAX,
};

/**
 * Enum to specify the completion trigger mode
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_tigger_mode_t {
	/** Trigger mode disabled */
	RTE_PMD_QDMA_TRIG_MODE_DISABLE,
	/** Trigger mode every */
	RTE_PMD_QDMA_TRIG_MODE_EVERY,
	/** Trigger mode user count */
	RTE_PMD_QDMA_TRIG_MODE_USER_COUNT,
	/** Trigger mode user */
	RTE_PMD_QDMA_TRIG_MODE_USER,
	/** Trigger mode timer */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER,
	/** Trigger mode timer + count */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT,
	/** Trigger mode invalid */
	RTE_PMD_QDMA_TRIG_MODE_MAX,
};

/**
 * Enum to specify the completion descriptor length
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_cmpt_desc_len {
	/** 8B Completion descriptor */
	RTE_PMD_QDMA_CMPT_DESC_LEN_8B = 8,
	/** 16B Completion descriptor */
	RTE_PMD_QDMA_CMPT_DESC_LEN_16B = 16,
	/** 32B Completion descriptor */
	RTE_PMD_QDMA_CMPT_DESC_LEN_32B = 32,
	/** 64B Completion descriptor */
	RTE_PMD_QDMA_CMPT_DESC_LEN_64B = 64,
	/** Invalid Completion descriptor */
	RTE_PMD_QDMA_CMPT_DESC_LEN_MAX,
};

/**
 * Enum to specify the bypass descriptor length
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_bypass_desc_len {
	/** 8B Bypass descriptor */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_8B = 8,
	/** 16B Bypass descriptor */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_16B = 16,
	/** 32B Bypass descriptor */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_32B = 32,
	/** 64B Bypass descriptor */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_64B = 64,
	/** Invalid Bypass descriptor */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_MAX,
};

/**
 * Enum to specify the debug request type
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_xdebug_type {
	/** Debug Global registers */
	RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR,
	/** Debug Device specific structure */
	RTE_PMD_QDMA_XDEBUG_QDMA_DEVICE_STRUCT,
	/** Debug Queue information */
	RTE_PMD_QDMA_XDEBUG_QUEUE_INFO,
	/** Debug descriptor */
	RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP,
	/** Invalid debug type */
	RTE_PMD_QDMA_XDEBUG_MAX,
};

/**
 * Enum to specify the queue ring for debug
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_xdebug_desc_type {
	/** Debug C2H ring descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_C2H,
	/** Debug H2C ring descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_H2C,
	/** Debug CMPT ring descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_CMPT,
	/** Invalid debug type */
	RTE_PMD_QDMA_XDEBUG_DESC_MAX,
};

/**
 * Enum to specify the QDMA device type
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_device_type {
	/** QDMA Soft device e.g. UltraScale+ IP's  */
	RTE_PMD_QDMA_DEVICE_SOFT,
	/** QDMA Versal device */
	RTE_PMD_QDMA_DEVICE_VERSAL,
	/** Invalid QDMA device  */
	RTE_PMD_QDMA_DEVICE_NONE
};

/**
 * Enum to specify the QDMA IP type
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_ip_type {
	/** Versal Hard IP  */
	RTE_PMD_QDMA_VERSAL_HARD_IP,
	/** Versal Soft IP  */
	RTE_PMD_QDMA_VERSAL_SOFT_IP,
	/** QDMA Soft IP  */
	RTE_PMD_QDMA_SOFT_IP,
	/** EQDMA Soft IP  */
	RTE_PMD_EQDMA_SOFT_IP,
	/** Invalid IP type  */
	RTE_PMD_QDMA_NONE_IP
};




/**
 * Structure to hold the QDMA device attributes
 *
 * @ingroup rte_pmd_qdma_struct
 */
struct rte_pmd_qdma_dev_attributes {
	/** Number of PFs*/
	uint8_t num_pfs;
	/** Number of Queues */
	uint16_t num_qs;
	/** Indicates whether FLR supported or not */
	uint8_t flr_present:1;
	/** Indicates whether ST mode supported or not */
	uint8_t st_en:1;
	/** Indicates whether MM mode supported or not */
	uint8_t mm_en:1;
	/** Indicates whether MM with Completions supported or not */
	uint8_t mm_cmpt_en:1;
	/** Indicates whether Mailbox supported or not */
	uint8_t mailbox_en:1;
	/** Debug mode is enabled/disabled for IP */
	uint8_t debug_mode:1;
	/** Descriptor Engine mode:
	 * Internal only/Bypass only/Internal & Bypass
	 */
	uint8_t desc_eng_mode:2;
	/** Number of MM channels */
	uint8_t mm_channel_max;

	/** To indicate support of
	 * overflow check disable in CMPT ring
	 */
	uint8_t cmpt_ovf_chk_dis:1;
	/** To indicate support of 64 bytes
	 * C2H/H2C descriptor format
	 */
	uint8_t sw_desc_64b:1;
	/** To indicate support of 64 bytes
	 * CMPT descriptor format
	 */
	uint8_t cmpt_desc_64b:1;
	/** To indicate support of
	 * counter + timer trigger mode
	 */
	uint8_t cmpt_trig_count_timer:1;
	/** Device Type */
	enum rte_pmd_qdma_device_type device_type;
	/** Versal IP Type */
	enum rte_pmd_qdma_ip_type ip_type;
};


/******************************************************************************/
/**
 * Dumps the QDMA configuration registers for the given port
 *
 * @param	port_id Port ID
 *
 * @return	'0' on success and "< 0" on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dbg_regdump(uint8_t port_id);

/******************************************************************************/
/**
 * Dumps the QDMA register field information for a given register offset
 *
 * @param	port_id Port ID
 * @param	reg_addr Register Address
 *
 * @return	'0' on success and "< 0" on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dbg_reg_info_dump(uint8_t port_id,
			uint32_t num_regs, uint32_t reg_addr);

/******************************************************************************/
/**
 * Dumps the device specific SW structure for the given port
 *
 * @param	port_id Port ID
 *
 * @return	'0' on success and "< 0" on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dbg_qdevice(uint8_t port_id);

/******************************************************************************/
/**
 * Dumps the queue contexts and queue specific SW
 * structures for the given queue ID
 *
 * @param	port_id Port ID
 * @param	queue  Queue ID relative to the Port
 *
 * @return	'0' on success and "< 0" on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dbg_qinfo(uint8_t port_id, uint16_t queue);

/******************************************************************************/
/**
 * Dumps the Queue descriptors
 *
 * @param	port_id Port ID
 * @param	queue  Queue ID relative to the Port
 * @param	start  Start index of the descriptor to dump
 * @param	end    End index of the descriptor to dump
 * @param	type   Descriptor type
 *
 * @return	'0' on success and "< 0" on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dbg_qdesc(uint8_t port_id, uint16_t queue, int start,
			int end, enum rte_pmd_qdma_xdebug_desc_type type);

/******************************************************************************/
/**
 * Returns the BAR indices of the QDMA BARs
 *
 * @param	port_id Port ID
 * @param	config_bar_idx Config BAR index
 * @param	user_bar_idx   AXI Master Lite BAR(user bar) index
 * @param	bypass_bar_idx AXI Bridge Master BAR(bypass bar) index
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	None
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_get_bar_details(int port_id, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx);

/******************************************************************************/
/**
 * Returns queue base for given port
 *
 * @param	port_id Port ID
 * @param	queue_base Queue base
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note    Application can call this API only after successful
 *          call to rte_eh_dev_configure() API
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_get_queue_base(int port_id, uint32_t *queue_base);

/******************************************************************************/
/**
 * Retrieves PCIe function type i.e. PF or VF
 *
 * @param	port_id Port ID
 * @param	func_type Indicates PCIe function type
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note    Returns the PCIe function type i.e. PF or VF of the given port
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_get_pci_func_type(int port_id,
		enum rte_pmd_qdma_pci_func_type *func_type);

/******************************************************************************/
/**
 * Returns immediate data state	i.e. whether enabled or disabled,
 * for the specified queue
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	state Pointer to the state specifying whether
 *			immediate data is enabled or not
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this function after
 *		rte_eth_rx_queue_setup() is called.
 *		API is applicable for streaming queues only.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_get_immediate_data_state(int port_id, uint32_t qid,
			int *state);

/******************************************************************************/
/**
 * Sets queue interface mode for the specified queue
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	mode Queue interface mode to be set
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before
 *		rte_eth_tx_queue_setup()/rte_eth_rx_queue_setup() API.
 *		By default, all queues are setup in streaming mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_queue_mode(int port_id, uint32_t qid,
			enum rte_pmd_qdma_queue_mode_t mode);

/******************************************************************************/
/**
 * Sets immediate data state i.e. enable or disable, for the specified queue.
 * If enabled, the user defined data in the completion
 * ring are dumped in to a queue specific file
 * "q_<qid>_immmediate_data.txt" in the local directory.
 *
 *@param	port_id Port ID
 *@param	qid  Queue ID
 *@param	state Immediate data state to be set.
 *			Set '0' to disable and '1' to enable.
 *
 *@return	'0' on success and '< 0' on failure
 *
 *@note		Application can call this API after successful
 *		call to rte_eth_rx_queue_setup() API.
 *		This API is applicable for streaming queues only.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_immediate_data_state(int port_id, uint32_t qid,
			uint8_t state);

/******************************************************************************/
/**
 * Enables or disables the overflow check (whether PIDX is overflowing
 * the CIDX) performed by QDMA on the completion descriptor ring of specified
 * queue.
 *
 * @param	port_id Port ID
 * @param	qid	   Queue ID
 * @param	enable '1' to enable and '0' to disable the overflow check
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_overflow_check(int port_id, uint32_t qid,
			uint8_t enable);

/******************************************************************************/
/**
 * Configures the completion ring descriptor size
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	size Descriptor size to be configured
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API
 *		when queue is in streaming mode, and before
 *		rte_pmd_qdma_dev_cmptq_setup when queue is in
 *		memory mapped mode.
 *		By default, the completion desciptor size is set to 8 bytes.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_descriptor_size(int port_id, uint32_t qid,
			enum rte_pmd_qdma_cmpt_desc_len size);

/******************************************************************************/
/**
 * Configures the trigger mode for completion ring CIDX updates
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	mode Trigger mode to be configured
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 *		By default, trigger mode is set to
 *		RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_trigger_mode(int port_id, uint32_t qid,
			enum rte_pmd_qdma_tigger_mode_t mode);

/******************************************************************************/
/**
 * Configures the timer interval in microseconds to trigger
 * the completion ring CIDX updates
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	value Timer interval for completion trigger to be configured
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_timer(int port_id, uint32_t qid, uint32_t value);

/******************************************************************************/
/**
 * Enables or disables prefetch of the descriptors by prefetch engine
 *
 *@param	port_id Port ID
 *@param	qid     Queue ID
 *@param	enable  '1' to enable and '0' to disable the descriptor prefetch
 *
 *@return	'0' on success and '< 0' on failure
 *
 *@note		Application can call this API after successful call to
 *		rte_eth_rx_queue_setup() API, but before calling
 *		rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_set_c2h_descriptor_prefetch(int port_id, uint32_t qid,
			uint8_t enable);

/*****************************************************************************/
/**
 * Sets the PCIe endpoint memory offset at which to
 * perform DMA operation for the specified queue operating
 * in memory mapped mode.
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	dir Direction i.e. Tx or Rx
 * @param	addr Destination address for Tx, Source address for Rx
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	This API can be called before Tx/Rx burst API's
 *		(rte_eth_tx_burst() and rte_eth_rx_burst()) are called.
 * @ingroup rte_pmd_qdma_func
 *****************************************************************************/
int rte_pmd_qdma_set_mm_endpoint_addr(int port_id, uint32_t qid,
			enum rte_pmd_qdma_dir_type dir, uint32_t addr);

/******************************************************************************/
/**
 * Sets the Tx bypass mode and bypass descriptor size for the specified queue
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	bypass_mode Bypass mode to be set
 * @param	size Bypass descriptor size to be set
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before tx_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_configure_tx_bypass(int port_id, uint32_t qid,
			enum rte_pmd_qdma_tx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * Sets the Rx bypass mode and bypass descriptor size for the specified queue
 *
 * @param	port_id Port ID
 * @param	qid  Queue ID
 * @param	bypass_mode Bypass mode to be set
 * @param	size Bypass descriptor size to be set
 *
 * @return	'0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API.
 *		By default, all queues are configured in internal mode
 *		i.e. bypass disabled.
 *		If size is specified zero, then the bypass descriptor size is
 *		set to the one used in internal mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_configure_rx_bypass(int port_id, uint32_t qid,
			enum rte_pmd_qdma_rx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * Retrive the device capabilities
 *
 * @param   port_id Port ID
 * @param   dev_attr Pointer to the device capabilities structure
 *
 * @return  '0' on success and '< 0' on failure
 *
 * @note	None.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_get_device_capabilities(int port_id,
			struct rte_pmd_qdma_dev_attributes *dev_attr);

/******************************************************************************/
/**
 * Allocate and set up a completion queue for memory mapped mode
 *
 * @param   port_id Port ID
 * @param   qid     Queue ID
 * @param   nb_cmpt_desc Completion queue ring size
 * @param   socket_id    The socket_id argument is the socket identifier
 *			in case of NUMA. Its value can be SOCKET_ID_ANY
 *			if there is no NUMA constraint for the DMA memory
 *			allocated for the transmit descriptors of the ring.
 *
 * @return  '0' on success and '< 0' on failure
 *
 * @note	Application can call this API after successful call to
 *		rte_eth_dev_configure() and rte_pmd_qdma_set_queue_mode()
 *		for queues in memory mapped mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_setup(int port_id, uint32_t cmpt_queue_id,
					uint16_t nb_cmpt_desc,
					unsigned int socket_id);

/******************************************************************************/
/**
 * Start the MM completion queue
 *
 * @param   port_id Port ID
 * @param   qid Queue ID
 *
 * @return  '0' on success and '< 0' on failure
 *
 * @note    Application can call this API after successful call to
 *          rte_pmd_qdma_dev_cmptq_setup() API when queue is in
 *          memory mapped mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_start(int port_id, uint32_t qid);

/******************************************************************************/
/**
 * Stop the MM completion queue
 *
 * @param   port_id Port ID
 * @param   qid  Queue ID
 *
 * @return  '0' on success and '< 0' on failure
 *
 * @note    Application can call this API after successful call to
 *          rte_pmd_qdma_dev_cmptq_start() API when queue is in
 *          memory mapped mode.
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_stop(int port_id, uint32_t qid);

/*****************************************************************************/
/**
 * Process the MM Completion queue entries
 *
 * @param   port_id Port ID
 * @param   qid  Queue ID
 * @param   cmpt_buff  User buffer pointer to store the completion data
 * @param   nb_entries Number of compeltion entries to process
 *
 * @return  'number of entries processed' on success and '< 0' on failure
 *
 * @note    Application can call this API after successful call to
 *	    rte_pmd_qdma_dev_cmptq_start() API
 * @ingroup rte_pmd_qdma_func
 ******************************************************************************/
uint16_t rte_pmd_qdma_mm_cmpt_process(int port_id, uint32_t qid,
		void *cmpt_buff, uint16_t nb_entries);

/*****************************************************************************/
/**
 * DPDK PMD function to close the device.
 *
 * @param   port_id Port ID
 *
 * @return  '0' on success and '< 0' on failure
 *
 ******************************************************************************/
int rte_pmd_qdma_dev_close(uint16_t port_id);

#ifdef __cplusplus
}
#endif
#endif /* ifndef __RTE_PMD_QDMA_EXPORT_H__ */
