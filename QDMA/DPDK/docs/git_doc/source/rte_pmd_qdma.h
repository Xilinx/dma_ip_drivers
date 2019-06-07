/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2019 Xilinx, Inc. All rights reserved.
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

#ifndef __RTE_PMD_QDMA_EXPORT_H__
#define __RTE_PMD_QDMA_EXPORT_H__

#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>


/**
 * DOC: Xilinx QDMA DPDK Interface Definitions
 *
 * Header file *rte_pmd_qdma.h* defines data structures and functions
 * exported by QDMA DPDK PMD.
 *
 * These APIs are subject to change.
 */


/**
 * enum rte_pmd_qdma_rx_bypass_mode - Supported bypass modes in C2H direction
 *
 * Rx (C2H) queue can be configured in one of these bypass modes.
 *
 */
enum rte_pmd_qdma_rx_bypass_mode {
	/**
	 * @RTE_PMD_QDMA_RX_BYPASS_NONE : Bypass mode is disabled i.e.,
	 * internal QDMA mode is enabled
	 */
	RTE_PMD_QDMA_RX_BYPASS_NONE = 0,
	/**
	 * @RTE_PMD_QDMA_RX_BYPASS_CACHE : Cache bypass mode,
	 * where in the prefetch engine keeps the order of the descriptors
	 * to pair with the C2H data packet from the user
	 */
	RTE_PMD_QDMA_RX_BYPASS_CACHE = 1,
	/**
	 * @RTE_PMD_QDMA_RX_BYPASS_SIMPLE : Simple bypass mode,
	 * where in the user application keeps the order of the descriptors
	 * to pair with the C2H data packet from the user
	 */
	RTE_PMD_QDMA_RX_BYPASS_SIMPLE = 2,
	/** @RTE_PMD_QDMA_RX_BYPASS_MAX : Unsupported value */
	RTE_PMD_QDMA_RX_BYPASS_MAX
};

/**
 * enum rte_pmd_qdma_tx_bypass_mode - Supported bypass modes in H2C direction
 *
 * Tx (H2C) queue can be configured in one of these bypass modes.
 *
 */
enum rte_pmd_qdma_tx_bypass_mode {
	/**
	 * @RTE_PMD_QDMA_TX_BYPASS_NONE : Bypass mode is disabled i.e.,
	 * internal QDMA mode is enabled
	 */
	RTE_PMD_QDMA_TX_BYPASS_NONE = 0,
	/** @RTE_PMD_QDMA_TX_BYPASS_ENABLE : H2C bypass mode */
	RTE_PMD_QDMA_TX_BYPASS_ENABLE = 1,
	/** @RTE_PMD_QDMA_TX_BYPASS_MAX : Unsupported value */
	RTE_PMD_QDMA_TX_BYPASS_MAX
};

/**
 * enum rte_pmd_qdma_dir_type - Enum to specify the direction i.e. TX or RX
 */
enum rte_pmd_qdma_dir_type {
	/** @RTE_PMD_QDMA_TX : H2C or Tx direction */
	RTE_PMD_QDMA_TX = 0,
	/** @RTE_PMD_QDMA_RX : C2H or Rx direction */
	RTE_PMD_QDMA_RX,
	/** @RTE_PMD_QDMA_DIR_TYPE_MAX : Unsupported value */
	RTE_PMD_QDMA_DIR_TYPE_MAX
};

/**
 * enum rte_pmd_qdma_pci_func_type - Indicates the type of PCIe function
 * i.e., PF or VF
 */
enum rte_pmd_qdma_pci_func_type {
	/** @RTE_PMD_QDMA_PCI_FUNC_PF : PCIe Physical Function */
	RTE_PMD_QDMA_PCI_FUNC_PF,
	/** @RTE_PMD_QDMA_PCI_FUNC_VF : PCIe Virtual Function */
	RTE_PMD_QDMA_PCI_FUNC_VF,
	/** @RTE_PMD_QDMA_PCI_FUNC_TYPE_MAX : Unsupported value */
	RTE_PMD_QDMA_PCI_FUNC_TYPE_MAX,
};

/**
 * enum rte_pmd_qdma_queue_mode_t - Indicates queue interface i.e.,
 * Memory mapped or Streaming
 *
 * Each queue must be configured in one of these modes.
 */
enum rte_pmd_qdma_queue_mode_t {
	/** @RTE_PMD_QDMA_MEMORY_MAPPED_MODE : Memory mapped mode */
	RTE_PMD_QDMA_MEMORY_MAPPED_MODE,
	/** @RTE_PMD_QDMA_STREAMING_MODE : Streaming mode */
	RTE_PMD_QDMA_STREAMING_MODE,
	/** @RTE_PMD_QDMA_QUEUE_MODE_MAX : Unsupported value */
	RTE_PMD_QDMA_QUEUE_MODE_MAX,
};

/**
 * enum rte_pmd_qdma_tigger_mode_t - Indicates the completion trigger mode
 */
enum rte_pmd_qdma_tigger_mode_t {
	/** @RTE_PMD_QDMA_TRIG_MODE_DISABLE : Completions are not trigerred */
	RTE_PMD_QDMA_TRIG_MODE_DISABLE,
	/**
	 * @RTE_PMD_QDMA_TRIG_MODE_EVERY : Completions are trigerred
	 * on every event
	 */
	RTE_PMD_QDMA_TRIG_MODE_EVERY,
	/**
	 * @RTE_PMD_QDMA_TRIG_MODE_USER_COUNT : Completions are trigerred
	 * when specified count of packets is reached
	 */
	RTE_PMD_QDMA_TRIG_MODE_USER_COUNT,
	/**
	 * @RTE_PMD_QDMA_TRIG_MODE_USER : Completions are trigerred
	 * on user defined event
	 */
	RTE_PMD_QDMA_TRIG_MODE_USER,
	/**
	 * @RTE_PMD_QDMA_TRIG_MODE_USER_TIMER : Completions are trigerred
	 * on every expiry of the specified time
	 */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER,
	/**
	 * @RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT : Completions are trigerred
	 * either on expiry of specified time or when specified count of packets
	 * is reached, whichever is earlier.
	 */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT,
	/** @RTE_PMD_QDMA_TRIG_MODE_MAX : Unsupported value */
	RTE_PMD_QDMA_TRIG_MODE_MAX,
};

/**
 * enum rte_pmd_qdma_cmpt_desc_len - Indicates the completion descriptor size
 */
enum rte_pmd_qdma_cmpt_desc_len {
	/**
	 * @RTE_PMD_QDMA_CMPT_DESC_LEN_8B : Completion descriptor size of 8 Bytes
	 */
	RTE_PMD_QDMA_CMPT_DESC_LEN_8B = 8,
	/**
	 * @RTE_PMD_QDMA_CMPT_DESC_LEN_16B : Completion descriptor size of 16 Bytes
	 */
	RTE_PMD_QDMA_CMPT_DESC_LEN_16B = 16,
	/**
	 * @RTE_PMD_QDMA_CMPT_DESC_LEN_32B : Completion descriptor size of 32 Bytes
	 */
	RTE_PMD_QDMA_CMPT_DESC_LEN_32B = 32,
	/**
	 * @RTE_PMD_QDMA_CMPT_DESC_LEN_64B : Completion descriptor size of 64 Bytes
	 */
	RTE_PMD_QDMA_CMPT_DESC_LEN_64B = 64,
	/** @RTE_PMD_QDMA_CMPT_DESC_LEN_MAX : Unsupported value */
	RTE_PMD_QDMA_CMPT_DESC_LEN_MAX
};

/**
 * enum rte_pmd_qdma_bypass_desc_len - Indicates the descriptor size
 * of the queue configured in bypass mode
 */
enum rte_pmd_qdma_bypass_desc_len {
	/**
	 * @RTE_PMD_QDMA_BYPASS_DESC_LEN_8B : Bypass descriptor size of 8 Bytes
	 */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_8B = 8,
	/**
	 * @RTE_PMD_QDMA_BYPASS_DESC_LEN_16B : Bypass descriptor size of 16 Bytes
	 */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_16B = 16,
	/**
	 * @RTE_PMD_QDMA_BYPASS_DESC_LEN_32B : Bypass descriptor size of 32 Bytes
	 */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_32B = 32,
	/**
	 * @RTE_PMD_QDMA_BYPASS_DESC_LEN_64B : Bypass descriptor size of 64 Bytes
	 */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_64B = 64,
	/** @RTE_PMD_QDMA_BYPASS_DESC_LEN_MAX : Unsupported value */
	RTE_PMD_QDMA_BYPASS_DESC_LEN_MAX
};

/**
 * enum rte_pmd_qdma_xdebug_type - Indicates the type debug information to dump
 */
enum rte_pmd_qdma_xdebug_type {
	/** @RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR : Dump QDMA Global CSR registers */
	RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR,
	/** @RTE_PMD_QDMA_XDEBUG_QUEUE_CONTEXT : Dump a queue's contexts */
	RTE_PMD_QDMA_XDEBUG_QUEUE_CONTEXT,
	/**
	 * @RTE_PMD_QDMA_XDEBUG_QUEUE_STRUCT : Dump a queue specific
	 * software information
	 */
	RTE_PMD_QDMA_XDEBUG_QUEUE_STRUCT,
	/**
	 * @RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP : Dump descriptor information
	 * of a queue
	 */
	RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP,
	/** @RTE_PMD_QDMA_XDEBUG_MAX : Unsupported value */
	RTE_PMD_QDMA_XDEBUG_MAX,
};

/**
 * enum rte_pmd_qdma_xdebug_desc_type - Indicates the type of
 * queue descriptor ring to dump
 */
enum rte_pmd_qdma_xdebug_desc_type {
	/** @RTE_PMD_QDMA_XDEBUG_DESC_C2H : Dump C2H queue's descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_C2H,
	/** @RTE_PMD_QDMA_XDEBUG_DESC_H2C : Dump H2C queue's descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_H2C,
	/** @RTE_PMD_QDMA_XDEBUG_DESC_CMPT : Dump completion queue's descriptor */
	RTE_PMD_QDMA_XDEBUG_DESC_CMPT,
	/** @RTE_PMD_QDMA_XDEBUG_DESC_MAX : Unsupported value */
	RTE_PMD_QDMA_XDEBUG_DESC_MAX,
};

/**
 * struct rte_pmd_qdma_xdebug_desc_param - Defines the parameters needed to
 * dump descriptor information when RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP
 * is selected
 */
struct rte_pmd_qdma_xdebug_desc_param {
	/** @queue : Queue id of the ring whose descriptor dump is required */
	uint16_t queue;
	/** @start : First index in the ring from where to start the dump */
	int start;
	/** @end : Last index in the descriptor ring to dump */
	int end;
	/** @type : Type of queue descriptor ring to dump */
	enum rte_pmd_qdma_xdebug_desc_type type;
};

/*Functions*/
/******************************************************************************/
/**
 * rte_pmd_qdma_xdebug - Dumps the debug information for the specified
 * debug type
 *
 * @port_id: Port ID
 * @type: Debug Type
 * @params: It has different significance based on debug type.
 * Below is the meaning of params for different debug types.
 *  - RTE_PMD_QDMA_XDEBUG_QDMA_GLOBAL_CSR : Not used
 *  - RTE_PMD_QDMA_XDEBUG_QUEUE_CONTEXT   : Pointer to variable specifying
 *    Queue Id
 *  - RTE_PMD_QDMA_XDEBUG_QUEUE_STRUCT    : Pointer to variable specifying
 *    Queue Id
 *  - RTE_PMD_QDMA_XDEBUG_QUEUE_DESC_DUMP : Pointer to variable of type
 *    struct rte_pmd_qdma_xdebug_desc_param
 *
 * Return: '0' on success, '< 0' on failure
 ******************************************************************************/
int rte_pmd_qdma_xdebug(uint8_t port_id, enum rte_pmd_qdma_xdebug_type type,
			void *params);

/******************************************************************************/
/**
 * rte_pmd_qdma_get_bar_details - Returns the BAR indices of the QDMA BARs
 *
 * @portid : Port ID
 * @config_bar_idx : Config BAR index
 * @user_bar_idx   : User BAR index
 * @bypass_bar_idx : Bypass BAR index
 *
 * Return: '0' on success, '< 0' on failure
 ******************************************************************************/
int rte_pmd_qdma_get_bar_details(int portid, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx);

/******************************************************************************/
/**
 * rte_pmd_qdma_get_queue_base - Returns queue base for given port
 *
 * @portid : Port ID
 * @queue_base : queue base
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API only after successful
 * call to rte_eth_dev_configure() API.
 ******************************************************************************/
int rte_pmd_qdma_get_queue_base(int portid, uint32_t *queue_base);

/******************************************************************************/
/**
 * rte_pmd_qdma_get_pci_func_type - Retrieves PCI function type i.e. PF or VF
 *
 * @portid : Port ID
 * @func_type : Indicates pci function type
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: This API returns the PCIe function type i.e. PF or VF
 * of the given port.
 ******************************************************************************/
int rte_pmd_qdma_get_pci_func_type(int portid,
		enum rte_pmd_qdma_pci_func_type *func_type);

/******************************************************************************/
/**
 * rte_pmd_qdma_get_immediate_data_state - Returns immediate data state
 * i.e. whether enabled or disabled, for the specified queue
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @state : Pointer to the state specifying whether
 * immediate data is enabled or not
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this function after
 * rte_eth_rx_queue_setup() is called.
 * This API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_get_immediate_data_state(int portid, uint32_t qid,
			int *state);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_queue_mode - Sets queue interface mode for the
 * specified queue
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @mode : Queue interface mode to be set
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API after
 * successful call to rte_eth_dev_configure() but,
 * before calling rte_eth_tx_queue_setup()/rte_eth_rx_queue_setup() API.
 * By default, all queues are setup in Streaming mode.
 ******************************************************************************/
int rte_pmd_qdma_set_queue_mode(int portid, uint32_t qid,
			enum rte_pmd_qdma_queue_mode_t mode);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_immediate_data_state - Sets immediate data state
 *			i.e. enable or disable, for the specified queue.
 *			If enabled, the user defined data in the completion
 *			ring are dumped in to a queue specific file
 *			"q_<qid>_immmediate_data.txt" in the local directory.
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @state :Immediate data state to be set. Set '0' to disable and '1' to enable
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API after
 * successful call to rte_eth_rx_queue_setup() API.
 * This API is applicable for streaming queues only.
 ******************************************************************************/
int rte_pmd_qdma_set_immediate_data_state(int portid, uint32_t qid,
			uint8_t state);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_cmpt_overflow_check - Enables or disables the
 * overflow check (whether PIDX is overflowing the CIDX) performed by QDMA
 * on the completion descriptor ring of specified queue.
 *
 * @portid : Port ID
 * @qid	   : Queue ID
 * @enable :  '1' to enable and '0' to disable the overflow check
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API after
 * successful call to rte_eth_rx_queue_setup() API,
 * but before calling rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_overflow_check(int portid, uint32_t qid,
			uint8_t enable);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_cmpt_descriptor_size - Configures the completion ring
 * descriptor size
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @size : Descriptor size to be configured
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API after successful call to
 * rte_eth_dev_configure() but before rte_eth_rx_queue_setup() API
 * when queue is in streaming mode, and before
 * rte_pmd_qdma_dev_cmptq_setup when queue is in
 * memory mapped mode.
 * By default, the completion desciptor size is set to 8 bytes.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_descriptor_size(int portid, uint32_t qid,
			enum rte_pmd_qdma_cmpt_desc_len size);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_cmpt_trigger_mode - Configures the trigger mode for
 * completion ring CIDX updates
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @mode : Trigger mode to be configured
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API
 * before calling rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 * By default, trigger mode is set to RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_trigger_mode(int portid, uint32_t qid,
			enum rte_pmd_qdma_tigger_mode_t mode);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_cmpt_timer - Configures the timer interval in microseconds
 * to trigger the completion ring CIDX updates
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @value : Timer interval for completion trigger to be configured
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API
 * before calling rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_cmpt_timer(int portid, uint32_t qid, uint32_t value);

/******************************************************************************/
/**
 * rte_pmd_qdma_set_c2h_descriptor_prefetch - Enables or disables
 * prefetch of the descriptors by prefetch engine
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @enable:'1' to enable and '0' to disable the descriptor prefetch
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: Application can call this API
 * after successful call to rte_eth_rx_queue_setup() API,
 * but before calling rte_eth_rx_queue_start() or rte_eth_dev_start() API.
 ******************************************************************************/
int rte_pmd_qdma_set_c2h_descriptor_prefetch(int portid, uint32_t qid,
			uint8_t enable);

/*****************************************************************************/
/**
 * rte_pmd_qdma_set_mm_endpoint_addr - Sets the PCIe endpoint memory offset
 * at which to perform DMA operation for the specified queue
 * operating in memory mapped mode.
 *
 * @portid : Port ID
 * @qid : Queue ID
 * @dir : Queue direction i.e. Tx or Rx
 * @addr : Destination address for Tx, Source address for Rx
 *
 * Return: '0' on success, '< 0' on failure
 *
 * Note: This API can be called before
 * Tx/Rx burst API's (rte_eth_tx_burst() and rte_eth_rx_burst()) are called.
 *****************************************************************************/
int rte_pmd_qdma_set_mm_endpoint_addr(int portid, uint32_t qid,
			enum rte_pmd_qdma_dir_type dir, uint32_t addr);

/******************************************************************************/
/**
 * rte_pmd_qdma_configure_tx_bypass - Sets the TX bypass mode and
 * bypass descriptor size for the specified queue
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 * @bypass_mode : Bypass mode to be set
 * @size : Bypass descriptor size to be set
 *
 * Return: '0' on success, '< 0' on failure.
 *
 * Note: Application can call this API
 * after successful call to rte_eth_dev_configure() but
 * before calling rte_eth_tx_queue_setup() API.
 * By default, all queues are configured in internal mode i.e. bypass disabled.
 * If size is specified zero, then the bypass descriptor size
 * is set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_tx_bypass(int portid, uint32_t qid,
			enum rte_pmd_qdma_tx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * rte_pmd_qdma_configure_rx_bypass - Sets the RX bypass mode and
 * bypass descriptor size for the specified queue
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 * @bypass_mode : Bypass mode to be set
 * @size : Bypass descriptor size to be set
 *
 * Return: '0' on success, '< 0' on failure.
 *
 * Note: Application can call this API after
 * successful call to rte_eth_dev_configure() but
 * before calling rte_eth_rx_queue_setup() API.
 * By default, all queues are configured in internal mode i.e. bypass disabled.
 * If size is specified zero, then the bypass descriptor size
 * is set to the one used in internal mode.
 ******************************************************************************/
int rte_pmd_qdma_configure_rx_bypass(int portid, uint32_t qid,
			enum rte_pmd_qdma_rx_bypass_mode bypass_mode,
			enum rte_pmd_qdma_bypass_desc_len size);

/******************************************************************************/
/**
 * rte_pmd_qdma_dev_cmptq_setup - Allocate and set up a completion queue
 *			for memory mapped mode.
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 * @nb_cmpt_desc: Completion queue ring size.
 * @socket_id : The socket_id argument is the socket identifier
 *			in case of NUMA. Its value can be SOCKET_ID_ANY
 *			if there is no NUMA constraint for the DMA memory
 *			allocated for the transmit descriptors of the ring.
 *
 * Return:  '0' on success and '< 0' on failure.
 *
 * Note: Application can call this API after successful call to
 * rte_eth_dev_configure() and rte_pmd_qdma_set_queue_mode()
 * for queues in memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_setup(int portid, uint32_t qid,
					uint16_t nb_cmpt_desc,
					unsigned int socket_id);

/******************************************************************************/
/**
 * rte_pmd_qdma_dev_cmptq_start - Start the MM completion queue.
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 *
 * Return:  '0' on success and '< 0' on failure.
 *
 * Note: Application can call this API after successful call to
 * rte_pmd_qdma_dev_cmptq_setup() API when queue is in
 * memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_start(int portid, uint32_t qid);

/******************************************************************************/
/**
 * rte_pmd_qdma_dev_cmptq_stop - Stop the MM completion queue.
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 *
 * Return:  '0' on success and '< 0' on failure.
 *
 * Note: Application can call this API after successful call to
 * rte_pmd_qdma_dev_cmptq_start() API when queue is in
 * memory mapped mode.
 ******************************************************************************/
int rte_pmd_qdma_dev_cmptq_stop(int portid, uint32_t qid);

/*****************************************************************************/
/**
 * rte_pmd_qdma_mm_cmpt_process - Process the MM Completion queue entries.
 *
 * @portid : Port ID.
 * @qid : Queue ID.
 * @cmpt_buff : User buffer pointer to store the completion data.
 * @nb_entries: Number of compeltion entries to process.
 *
 * Return:  'number of entries processed' on success and '< 0' on failure.
 *
 * Note: Application can call this API after successful call to
 * rte_pmd_qdma_dev_cmptq_start() API.
 ******************************************************************************/
uint16_t rte_pmd_qdma_mm_cmpt_process(int portid, uint32_t qid, void *cmpt_buff,
		uint16_t nb_entries);

#endif /* ifndef __RTE_PMD_QDMA_EXPORT_H__ */
