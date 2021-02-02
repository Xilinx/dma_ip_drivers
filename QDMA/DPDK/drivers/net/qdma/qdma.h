/*-
 * BSD LICENSE
 *
 * Copyright(c) 2017-2021 Xilinx, Inc. All rights reserved.
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

#ifndef __QDMA_H__
#define __QDMA_H__

#include <stdbool.h>
#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_ethdev_driver.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_cycles.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>
#include "qdma_user.h"
#include "qdma_resource_mgmt.h"
#include "qdma_mbox.h"
#include "rte_pmd_qdma.h"
#include "qdma_log.h"

#define QDMA_NUM_BARS          (6)
#define DEFAULT_PF_CONFIG_BAR  (0)
#define DEFAULT_VF_CONFIG_BAR  (0)
#define BAR_ID_INVALID         (-1)

#define QDMA_FUNC_ID_INVALID    0xFFFF

#define DEFAULT_QUEUE_BASE	(0)

#define QDMA_MAX_BURST_SIZE (128)
#define QDMA_MIN_RXBUFF_SIZE	(256)

/* Descriptor Rings aligned to 4KB boundaries - only supported value */
#define QDMA_ALIGN	(4096)

#define DEFAULT_TIMER_CNT_TRIG_MODE_TIMER	(5)
#define DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER	(30)

#define MIN_RX_PIDX_UPDATE_THRESHOLD (1)
#define MIN_TX_PIDX_UPDATE_THRESHOLD (1)
#define DEFAULT_MM_CMPT_CNT_THRESHOLD	(2)
#define QDMA_TXQ_PIDX_UPDATE_INTERVAL	(1000) //100 uSec

/** Delays **/
#define MAILBOX_PF_MSG_DELAY		(20)
#define MAILBOX_VF_MSG_DELAY		(10)
#define MAILBOX_PROG_POLL_COUNT		(1250)

#define WB_TIMEOUT			(100000)
#define RESET_TIMEOUT		(60000)
#define SHUTDOWN_TIMEOUT	(60000)

#define QDMA_MAX_BUFLEN     (2048 * 10)

#ifdef spin_lock_init
#undef spin_lock_init
#endif
#define spin_lock_init(sl) rte_spinlock_init(sl)

/** Completion Context config */
#define CMPT_DEFAULT_COLOR_BIT           (1)
#define CMPT_CNTXT_DESC_SIZE_8B          (0)
#define CMPT_CNTXT_DESC_SIZE_16B         (1)
#define CMPT_CNTXT_DESC_SIZE_32B         (2)
#define CMPT_CNTXT_DESC_SIZE_64B         (3)

/** SOFTWARE DESC CONTEXT */
#define SW_DESC_CNTXT_8B_BYPASS_DMA	    (0)
#define SW_DESC_CNTXT_16B_BYPASS_DMA	    (1)
#define SW_DESC_CNTXT_32B_BYPASS_DMA	    (2)
#define SW_DESC_CNTXT_64B_BYPASS_DMA	    (3)

#define SW_DESC_CNTXT_C2H_STREAM_DMA        (0)
#define SW_DESC_CNTXT_H2C_STREAM_DMA        (1)
#define SW_DESC_CNTXT_MEMORY_MAP_DMA        (2)

#define DEFAULT_QDMA_CMPT_DESC_LEN (RTE_PMD_QDMA_CMPT_DESC_LEN_8B)


enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

enum reset_state_t {
	RESET_STATE_IDLE,
	RESET_STATE_RECV_PF_RESET_REQ,
	RESET_STATE_RECV_PF_RESET_DONE,
	RESET_STATE_INVALID
};

/** MM Write-back status structure **/
struct __attribute__ ((packed)) wb_status
{
	volatile uint16_t	pidx; /** in C2H WB **/
	volatile uint16_t	cidx; /** Consumer-index **/
	uint32_t	rsvd2; /** Reserved. **/
};

struct qdma_pkt_stats {
	uint64_t pkts;
	uint64_t bytes;
};

/*
 * Structure associated with each CMPT queue.
 */
struct qdma_cmpt_queue {
	struct qdma_ul_cmpt_ring *cmpt_ring;
	struct wb_status    *wb_status;
	struct qdma_q_cmpt_cidx_reg_info cmpt_cidx_info;
	struct rte_eth_dev	*dev;

	uint16_t	cmpt_desc_len;
	uint16_t	nb_cmpt_desc;
	uint32_t	queue_id; /**< CMPT queue index. */

	uint8_t		status:1;
	uint8_t		st_mode:1; /**< dma-mode: MM or ST */
	uint8_t		dis_overflow_check:1;
	uint8_t		func_id;
	uint16_t	port_id; /**< Device port identifier. */
	int8_t		ringszidx;
	int8_t		threshidx;
	int8_t		timeridx;
	int8_t		triggermode;
	/* completion descriptor memzone */
	const struct rte_memzone *cmpt_mz;
};

/**
 * Structure associated with each RX queue.
 */
struct qdma_rx_queue {
	struct rte_mempool	*mb_pool; /**< mbuf pool to populate RX ring. */
	void			*rx_ring; /**< RX ring virtual address */
	union qdma_ul_st_cmpt_ring	*cmpt_ring;
	struct wb_status	*wb_status;
	struct rte_mbuf		**sw_ring; /**< address of RX software ring. */
	struct rte_eth_dev	*dev;

	uint16_t		rx_tail;
	uint16_t		cmpt_desc_len;
	uint16_t		rx_buff_size;
	uint16_t		nb_rx_desc; /**< number of RX descriptors. */
	uint16_t		nb_rx_cmpt_desc;
	uint32_t		queue_id; /**< RX queue index. */
	uint64_t		mbuf_initializer; /**< value to init mbufs */

	struct qdma_q_pidx_reg_info	q_pidx_info;
	struct qdma_q_cmpt_cidx_reg_info cmpt_cidx_info;
	struct qdma_pkt_stats	stats;

	uint16_t		port_id; /**< Device port identifier. */
	uint8_t			status:1;
	uint8_t			err:1;
	uint8_t			st_mode:1; /**< dma-mode: MM or ST */
	uint8_t			dump_immediate_data:1;
	uint8_t			rx_deferred_start:1;
	uint8_t			en_prefetch:1;
	uint8_t			en_bypass:1;
	uint8_t			en_bypass_prefetch:1;
	uint8_t			dis_overflow_check:1;

	union qdma_ul_st_cmpt_ring cmpt_data[QDMA_MAX_BURST_SIZE];

	enum rte_pmd_qdma_bypass_desc_len	bypass_desc_sz:7;
	uint8_t			func_id; /**< RX queue index. */
	uint32_t		ep_addr;

	int8_t			ringszidx;
	int8_t			cmpt_ringszidx;
	int8_t			buffszidx;
	int8_t			threshidx;
	int8_t			timeridx;
	int8_t			triggermode;

	const struct rte_memzone *rx_mz;
	/* C2H stream mode, completion descriptor result */
	const struct rte_memzone *rx_cmpt_mz;

#ifdef QDMA_LATENCY_OPTIMIZED
	/**< pend_pkt_moving_avg: average rate of packets received */
	unsigned int pend_pkt_moving_avg;
	/**< pend_pkt_avg_thr_hi: higher average threshold */
	unsigned int pend_pkt_avg_thr_hi;
	/**< pend_pkt_avg_thr_lo: lower average threshold */
	unsigned int pend_pkt_avg_thr_lo;
	/**< sorted_c2h_cntr_idx: sorted c2h counter index */
	unsigned char sorted_c2h_cntr_idx;
	/**< c2h_cntr_monitor_cnt: c2h counter stagnant monitor count */
	unsigned char c2h_cntr_monitor_cnt;
#endif //QDMA_LATENCY_OPTIMIZED
};

/**
 * Structure associated with each TX queue.
 */
struct qdma_tx_queue {
	void				*tx_ring; /* TX ring virtual address*/
	struct wb_status		*wb_status;
	struct rte_mbuf			**sw_ring;/* SW ring virtual address*/
	struct rte_eth_dev		*dev;
	uint16_t			tx_fl_tail;
	uint16_t			tx_desc_pend;
	uint16_t			nb_tx_desc; /* No of TX descriptors.*/
	rte_spinlock_t			pidx_update_lock;
	struct qdma_q_pidx_reg_info	q_pidx_info;
	uint64_t			offloads; /* Tx offloads */

	uint8_t				st_mode:1;/* dma-mode: MM or ST */
	uint8_t				tx_deferred_start:1;
	uint8_t				en_bypass:1;
	uint8_t				status:1;
	enum rte_pmd_qdma_bypass_desc_len		bypass_desc_sz:7;
	uint16_t			port_id; /* Device port identifier. */
	uint8_t				func_id; /* RX queue index. */
	int8_t				ringszidx;

	struct qdma_pkt_stats stats;

	uint32_t			ep_addr;
	uint32_t			queue_id; /* TX queue index. */
	uint32_t			num_queues; /* TX queue index. */
	const struct rte_memzone	*tx_mz;
};

struct qdma_vf_info {
	uint16_t	func_id;
};

struct queue_info {
	uint32_t	queue_mode:1;
	uint32_t	rx_bypass_mode:2;
	uint32_t	tx_bypass_mode:1;
	uint32_t	cmpt_desc_sz:7;
	uint8_t		immediate_data_state:1;
	uint8_t		dis_cmpt_ovf_chk:1;
	uint8_t		en_prefetch:1;
	enum rte_pmd_qdma_bypass_desc_len rx_bypass_desc_sz:7;
	enum rte_pmd_qdma_bypass_desc_len tx_bypass_desc_sz:7;
	uint8_t		timer_count;
	int8_t		trigger_mode;
};

struct qdma_pci_dev {
	int config_bar_idx;
	int user_bar_idx;
	int bypass_bar_idx;
	void *bar_addr[QDMA_NUM_BARS]; /* memory mapped I/O addr for BARs */

	/* Driver Attributes */
	uint32_t qsets_en;  /* no. of queue pairs enabled */
	uint32_t queue_base;
	uint8_t func_id;  /* Function id */

	/* DMA identifier used by the resource manager
	 * for the DMA instances used by this driver
	 */
	uint32_t dma_device_index;

	/* Device capabilities */
	struct qdma_dev_attributes dev_cap;

	uint8_t cmpt_desc_len;
	uint8_t c2h_bypass_mode;
	uint8_t h2c_bypass_mode;
	uint8_t trigger_mode;
	uint8_t timer_count;

	uint8_t dev_configured:1;
	uint8_t is_vf:1;
	uint8_t is_master:1;
	uint8_t en_desc_prefetch:1;

	/* Reset state */
	uint8_t reset_in_progress;
	enum reset_state_t reset_state;

	/* Hardware version info*/
	uint32_t vivado_rel:4;
	uint32_t rtl_version:4;
	uint32_t device_type:4;
	uint32_t ip_type:4;

	struct queue_info *q_info;
	struct qdma_dev_mbox mbox;
	uint8_t init_q_range;

	uint32_t g_ring_sz[QDMA_NUM_RING_SIZES];
	uint32_t g_c2h_cnt_th[QDMA_NUM_C2H_COUNTERS];
	uint32_t g_c2h_buf_sz[QDMA_NUM_C2H_BUFFER_SIZES];
	uint32_t g_c2h_timer_cnt[QDMA_NUM_C2H_TIMERS];
#ifdef QDMA_LATENCY_OPTIMIZED
	uint32_t sorted_idx_c2h_cnt_th[QDMA_NUM_C2H_COUNTERS];
#endif //QDMA_LATENCY_OPTIMIZED
	void	**cmpt_queues;
	/*Pointer to QDMA access layer function pointers*/
	struct qdma_hw_access *hw_access;

	struct qdma_vf_info *vfinfo;
	uint8_t vf_online_count;

	int16_t tx_qid_statid_map[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	int16_t rx_qid_statid_map[RTE_ETHDEV_QUEUE_STAT_CNTRS];
};

void qdma_dev_ops_init(struct rte_eth_dev *dev);
uint32_t qdma_read_reg(uint64_t reg_addr);
void qdma_write_reg(uint64_t reg_addr, uint32_t val);
void qdma_txq_pidx_update(void *arg);
int qdma_pf_csr_read(struct rte_eth_dev *dev);
int qdma_vf_csr_read(struct rte_eth_dev *dev);

uint8_t qmda_get_desc_sz_idx(enum rte_pmd_qdma_bypass_desc_len);
int qdma_vf_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t qid);
int qdma_vf_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);
int qdma_vf_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t qid);
int qdma_vf_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t qid);

int qdma_init_rx_queue(struct qdma_rx_queue *rxq);
void qdma_reset_tx_queue(struct qdma_tx_queue *txq);
void qdma_reset_rx_queue(struct qdma_rx_queue *rxq);

void qdma_clr_rx_queue_ctxts(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode);
void qdma_inv_rx_queue_ctxts(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode);
void qdma_clr_tx_queue_ctxts(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode);
void qdma_inv_tx_queue_ctxts(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode);
int qdma_identify_bars(struct rte_eth_dev *dev);
int qdma_get_hw_version(struct rte_eth_dev *dev);

/* implemented in rxtx.c */
uint16_t qdma_recv_pkts_st(struct qdma_rx_queue *rxq, struct rte_mbuf **rx_pkts,
				uint16_t nb_pkts);
uint16_t qdma_recv_pkts_mm(struct qdma_rx_queue *rxq, struct rte_mbuf **rx_pkts,
				uint16_t nb_pkts);
uint16_t qdma_xmit_pkts_st(struct qdma_tx_queue *txq, struct rte_mbuf **tx_pkts,
				uint16_t nb_pkts);
uint16_t qdma_xmit_pkts_mm(struct qdma_tx_queue *txq, struct rte_mbuf **tx_pkts,
				uint16_t nb_pkts);

uint32_t qdma_pci_read_reg(struct rte_eth_dev *dev, uint32_t bar, uint32_t reg);
void qdma_pci_write_reg(struct rte_eth_dev *dev, uint32_t bar,
				uint32_t reg, uint32_t val);

int index_of_array(uint32_t *arr, uint32_t n, uint32_t element);

int qdma_check_kvargs(struct rte_devargs *devargs,
			struct qdma_pci_dev *qdma_dev);

static inline const
struct rte_memzone *qdma_zone_reserve(struct rte_eth_dev *dev,
					const char *ring_name,
					uint32_t queue_id,
					uint32_t ring_size,
					int socket_id)
{
	char z_name[RTE_MEMZONE_NAMESIZE];
	snprintf(z_name, sizeof(z_name), "%s%s%d_%u",
			dev->device->driver->name, ring_name,
			dev->data->port_id, queue_id);
	return rte_memzone_reserve_aligned(z_name, (uint64_t)ring_size,
						socket_id, 0, QDMA_ALIGN);
}

bool is_qdma_supported(struct rte_eth_dev *dev);
bool is_vf_device_supported(struct rte_eth_dev *dev);
bool is_pf_device_supported(struct rte_eth_dev *dev);

void qdma_check_errors(void *arg);
#endif /* ifndef __QDMA_H__ */
