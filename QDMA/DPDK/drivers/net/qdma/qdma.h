/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2018 Xilinx, Inc. All rights reserved.
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

#ifndef __QDMA_H__
#define __QDMA_H__

#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>
#include "qdma_user.h"

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
#define PMD_DRV_LOG(level, fmt, args...) \
	RTE_LOG(level, PMD, "%s(): " fmt "\n", __func__, ## args)
#else
#define PMD_DRV_LOG(level, fmt, args...) do { } while (0)
#endif

#define FMAP_CNTXT (1)

#define NUM_BARS	(6)
#define DEFAULT_PF_CONFIG_BAR  (0)
#define DEFAULT_VF_CONFIG_BAR  (0)
#define BAR_ID_INVALID -1

#define QDMA_QUEUES_NUM_MAX (2048)
#define QDMA_MAX_BURST_SIZE (256)
#define QDMA_MIN_RXBUFF_SIZE	(256)

#define RX_STATUS_EOP (1)

#define QDMA_BUF_ALIGN	(4096)
/* Descriptor Rings aligned to 4KB boundaries - only supported value */
#define QDMA_ALIGN	(4096)

/* If this RX_BUF_SIZE, modified then the same value need to be modified
 * in the testapp commands.h file "MAX_BUF_SIZE" macro.
 * This is maintained equal to the descriptor Bufffer size
 */
#define RX_BUF_SIZE	4096

/* QDMA IP absolute maximum */
#define QDMA_PF_MAX             4       /* 4 PFs */
#define QDMA_VF_MAX             252
#define QDMA_FUNC_ID_INVALID    (QDMA_PF_MAX + QDMA_VF_MAX)

/** MBOX OPCODE **/
#define QDMA_MBOX_OPCD_HI	(1)
#define QDMA_MBOX_OPCD_BYE	(2)
#define QDMA_MBOX_OPCD_QADD	(3)
#define QDMA_MBOX_OPCD_QDEL	(4)

#define MBOX_POLL_FRQ 500000

#define DEFAULT_QUEUE_BASE	(0)

/*Corresponding to 100ns (1tick = 4ns for 250MHz user clock)*/
#define DEFAULT_C2H_INTR_TIMER_TICK		(25)
#define DEFAULT_TIMER_CNT_TRIG_MODE_TIMER	(5)
#define DEFAULT_TIMER_CNT_TRIG_MODE_COUNT_TIMER	(30)
#define DEFAULT_MAX_DESC_FETCH	(3) /* Max nuber of descriptors to fetch in one
				     * request is 8 * 2^val; Max value is 6;
				     * Keep same as PCIe MRRS
				     */
#define DEFAULT_WB_ACC_INT	(4) // Calculated as 4 * 2^val; Max value is 7
#define MAX_WB_ACC_INT		(7) // Calculated as 4 * 2^val; Max value is 7

#define DEFAULT_H2C_THROTTLE	(0x14000) // Setting throttle to 16K buffer size

#define MIN_RX_PIDX_UPDATE_THRESHOLD (1)
#define MIN_TX_PIDX_UPDATE_THRESHOLD (1)
#define QDMA_TXQ_PIDX_UPDATE_INTERVAL	(1000) //100 uSec

#define DEFAULT_PFCH_STOP_THRESH	(256)
#define DEFAULT_PFCH_NUM_ENTRIES_PER_Q	(8)
#define DEFAULT_PFCH_MAX_Q_CNT		(16)
#define DEFAULT_PFCH_EVICTION_Q_CNT	(14)

#define DEFAULT_CMPT_COAL_TIMER_CNT	(5)
#define DEFAULT_CMPT_COAL_TIMER_TICK	(25)
#define DEFAULT_CMPT_COAL_MAX_BUF_SZ	(32)

/** Delays **/
#define CONTEXT_PROG_DELAY		(1)
#define MAILBOX_PF_MSG_DELAY		(20)
#define PF_RESET_DELAY			(1)
#define PF_START_DELAY			(1)
#define MAILBOX_VF_MSG_DELAY		(10)
#define VF_RESET_DELAY			(200)
#define CONTEXT_PROG_POLL_COUNT		(10)
#define MAILBOX_PROG_POLL_COUNT		(1250)

#define WB_TIMEOUT			(UINT16_MAX * 4)

#ifdef spin_lock_init
#undef spin_lock_init
#endif
#define spin_lock_init(sl) rte_spinlock_init(sl)

enum cmpt_desc_len {
	CMPT_DESC_LEN_8B = 8,
	CMPT_DESC_LEN_16B = 16,
	CMPT_DESC_LEN_32B = 32,
	CMPT_DESC_LEN_64B = 64
};

enum bypass_desc_len {
	BYPASS_DESC_LEN_8B = 8,
	BYPASS_DESC_LEN_16B = 16,
	BYPASS_DESC_LEN_32B = 32,
	BYPASS_DESC_LEN_64B = 64
};

enum c2h_bypass_mode {
	C2H_BYPASS_MODE_NONE = 0,
	C2H_BYPASS_MODE_CACHE = 1,
	C2H_BYPASS_MODE_SIMPLE = 2,
	C2H_BYPASS_MODE_MAX = C2H_BYPASS_MODE_SIMPLE,
};

#define DEFAULT_QDMA_CMPT_DESC_LEN (CMPT_DESC_LEN_8B)

struct qdma_pkt_stats {
	uint64_t pkts;
	uint64_t bytes;
};

/**
 * Structure associated with each RX queue.
 */
struct qdma_rx_queue {
	struct rte_mempool	*mb_pool; /**< mbuf pool to populate RX ring. */
	void			*rx_ring; /**< RX ring virtual address */
	struct c2h_cmpt_ring	*cmpt_ring;
	struct wb_status	*wb_status;
	struct rte_mbuf		**sw_ring; /**< address of RX software ring. */
	volatile uint32_t	*rx_pidx;
	volatile uint32_t	*rx_cidx;

	uint16_t		rx_tail;
	uint16_t		c2h_pidx;
	uint16_t		rx_cmpt_tail;
	uint16_t		cmpt_desc_len;
	uint16_t		rx_buff_size;
	uint16_t		nb_rx_desc; /**< number of RX descriptors. */
	uint16_t		nb_rx_cmpt_desc;
	uint32_t		queue_id; /**< RX queue index. */

	struct qdma_pkt_stats	stats;

	uint32_t		ep_addr;

	uint8_t			st_mode:1; /**< dma-mode: MM or ST */
	uint8_t			rx_deferred_start:1;
	uint8_t			en_prefetch:1;
	uint8_t                 en_bypass:1;
	uint8_t                 dump_immediate_data:1;
	uint8_t                 en_bypass_prefetch:1;
	uint8_t                 dis_overflow_check:1;
	uint8_t                 status:1;
	enum bypass_desc_len	bypass_desc_sz;

	uint16_t		port_id; /**< Device port identifier. */
	uint16_t		func_id; /**< RX queue index. */


	int8_t			ringszidx;
	int8_t			cmpt_ringszidx;
	int8_t			buffszidx;
	int8_t			threshidx;
	int8_t			timeridx;
	int8_t			triggermode;

	const struct rte_memzone *rx_mz;
	/* C2H stream mode, completion descriptor result */
	const struct rte_memzone *rx_cmpt_mz;
};

/**
 * Structure associated with each TX queue.
 */
struct qdma_tx_queue {
	void				*tx_ring; /* TX ring virtual address*/
	struct wb_status		*wb_status;
	struct rte_mbuf			**sw_ring;/* SW ring virtual address*/
	volatile uint32_t		*tx_pidx;
	uint16_t			tx_tail;
	uint16_t			tx_fl_tail;
	uint16_t			tx_desc_pend;
	uint16_t			nb_tx_desc; /* No of TX descriptors.*/
	rte_spinlock_t			pidx_update_lock;

	uint8_t				st_mode:1;/* dma-mode: MM or ST */
	uint8_t				tx_deferred_start:1;
	uint8_t                         en_bypass:1;
	uint8_t                         status:1;
	enum bypass_desc_len		bypass_desc_sz;
	uint16_t			func_id; /* RX queue index. */
	uint16_t			port_id; /* Device port identifier. */
	int8_t				ringszidx;

	struct				qdma_pkt_stats stats;

	uint32_t			ep_addr;
	uint32_t			queue_id; /* TX queue index. */
	uint32_t			num_queues; /* TX queue index. */
	const struct rte_memzone	*tx_mz;
};

struct qdma_vf_queue_info {
	uint32_t mode;
};

struct qdma_vf_info {
	uint32_t	qbase;
	uint32_t	qmax;
	uint16_t	func_id;
	struct	qdma_vf_queue_info *vfqinfo;
};

struct queue_info {
	uint32_t queue_mode:1;
	uint32_t rx_bypass_mode:2;
	uint32_t tx_bypass_mode:1;
	uint32_t cmpt_desc_sz:7;
	enum bypass_desc_len rx_bypass_desc_sz:7;
	enum bypass_desc_len tx_bypass_desc_sz:7;
};

struct qdma_pci_dev {
	struct pci_dev *pdev; /* PCI device pointer */
	unsigned long magic;
	int config_bar_idx;
	int user_bar_idx;
	int bypass_bar_idx;
	struct rte_eth_dev *eth_dev;

	/* Driver Attributes */
	uint32_t qsets_max; /* max. of queue pairs */
	uint32_t qsets_en;  /* no. of queue pairs enabled */
	uint8_t st_mode_en;		/* Streaming mode enabled? */
	uint8_t mm_mode_en;		/* Memory mapped mode enabled? */

	uint16_t pf;
	uint8_t is_vf;
	uint8_t en_desc_prefetch;
	uint8_t cmpt_desc_len;
	uint8_t c2h_bypass_mode;
	uint8_t h2c_bypass_mode;

	void *bar_addr[NUM_BARS]; /* memory mapped I/O addr for BARs */
	unsigned long bar_len[NUM_BARS]; /* length of BAR regions */
	unsigned int flags;

	volatile uint32_t	*c2h_mm_control;
	volatile uint32_t	*h2c_mm_control;
	unsigned int queue_base;
	uint8_t wb_acc_int;
	uint8_t trigger_mode;
	uint16_t timer_count;

	/* Hardware version info*/
	uint32_t everest_ip:1;
	uint32_t vivado_rel:4;
	uint32_t rtl_version:8;
	struct qdma_vf_info *vfinfo;
	struct queue_info *q_info;
};

struct __attribute__ ((packed)) qdma_mbox_data {
	uint8_t opcode;
	uint16_t debug_funid:12;   /* Used for debug information only*/
	uint16_t filler:4;
	uint8_t err;
	uint32_t data[31];
};

struct __attribute__ ((packed)) qadd_msg {
	uint32_t qid;
	uint32_t st:1;
	uint32_t c2h:1;
	uint32_t prefetch:1;
	uint32_t  en_bypass:1;
	uint32_t  en_bypass_prefetch:1;
	uint32_t  dis_overflow_check:1;
	uint32_t  bypass_desc_sz_idx:4;
	uint32_t ringsz;
	uint32_t bufsz;
	uint32_t thresidx;
	uint32_t timeridx;
	uint32_t triggermode;
	uint64_t ring_bs_addr;
	uint64_t cmpt_ring_bs_addr;
	uint32_t cmpt_ringszidx;
	uint8_t  cmpt_desc_fmt;
};

enum get_param_type {
	CONFIG_BAR = 0,
	USER_BAR,
	BYPASS_BAR
};
enum get_queue_param_type {
	CHECK_DUMP_IMMEDIATE_DATA = 0x10
};

enum dev_param_type {
	QUEUE_BASE = 0,
	TIMER_COUNT,
	TRIGGER_MODE,
};

enum queue_param_type {
	QUEUE_MODE = 0x10,
	DESC_PREFETCH,
	TX_DST_ADDRESS,
	RX_SRC_ADDRESS,
	DIS_OVERFLOW_CHECK,
	DUMP_IMMEDIATE_DATA,
	RX_BYPASS_MODE,
	RX_BYPASS_DESC_SIZE,
	TX_BYPASS_MODE,
	TX_BYPASS_DESC_SIZE,
	CMPT_DESC_SIZE
};

/*
 * queue_mode_t - queue modes
 */
enum queue_mode_t {
	MEMORY_MAPPED_MODE,
	STREAMING_MODE
};

/*
 * tigger_mode_t - trigger modes
 */
enum tigger_mode_t {
	TRIG_MODE_DISABLE,
	TRIG_MODE_EVERY,
	TRIG_MODE_USER_COUNT,
	TRIG_MODE_USER,
	TRIG_MODE_USER_TIMER,
	TRIG_MODE_USER_TIMER_COUNT,
	TRIG_MODE_MAX = TRIG_MODE_USER_TIMER_COUNT,
};

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

void qdma_dev_ops_init(struct rte_eth_dev *dev);
uint32_t qdma_read_reg(uint64_t addr);
void qdma_write_reg(uint64_t addr, uint32_t val);
void qdma_txq_pidx_update(void *arg);

int qdma_queue_ctxt_tx_prog(struct rte_eth_dev *dev, uint32_t qid,
	uint32_t mode, uint32_t ringszidx, uint16_t func_id, uint64_t phys_addr,
	uint8_t en_bypass, uint8_t  bypass_desc_sz_idx);

int qdma_queue_ctxt_rx_prog(struct rte_eth_dev *dev, uint32_t qid,
				uint32_t mode, uint8_t prefetch,
				uint32_t ringszidx, uint32_t cmpt_ringszidx,
				uint32_t bufszidx, uint32_t threshidx,
				uint32_t timeridx, uint32_t triggermode,
				uint16_t func_id, uint64_t phys_addr,
				uint64_t cmpt_phys_addr, uint8_t cmpt_desc_fmt,
				uint8_t en_bypass, uint8_t en_bypass_prefetch,
				uint8_t  bypass_desc_sz_idx,
				uint8_t dis_overflow_check);

int qdma_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
				uint16_t nb_rx_desc, unsigned int socket_id,
				const struct rte_eth_rxconf *rx_conf,
				struct rte_mempool *mb_pool);

int qdma_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
				uint16_t nb_tx_desc, unsigned int socket_id,
				const struct rte_eth_txconf *tx_conf);
void qdma_dev_rx_queue_release(void *rqueue);
void qdma_dev_tx_queue_release(void *tqueue);
uint8_t qmda_get_desc_sz_idx(enum bypass_desc_len);
int qdma_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t rx_queue_id);
int qdma_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t rx_queue_id);
int qdma_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t tx_queue_id);
int qdma_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t tx_queue_id);
int qdma_vf_dev_rx_queue_start(struct rte_eth_dev *dev, uint16_t rx_queue_id);
int qdma_vf_dev_rx_queue_stop(struct rte_eth_dev *dev, uint16_t rx_queue_id);
int qdma_vf_dev_tx_queue_start(struct rte_eth_dev *dev, uint16_t tx_queue_id);
int qdma_vf_dev_tx_queue_stop(struct rte_eth_dev *dev, uint16_t tx_queue_id);

int qdma_start_rx_queue(struct qdma_rx_queue *rxq);
void qdma_start_tx_queue(struct qdma_tx_queue *txq);
void qdma_reset_tx_queue(struct qdma_tx_queue *txq);
void qdma_reset_rx_queue(struct qdma_rx_queue *rxq);

void qdma_reset_rx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode);
void qdma_inv_rx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode);
void qdma_reset_tx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode);
void qdma_inv_tx_queues(struct rte_eth_dev *dev, uint32_t qid, uint32_t mode);
int qdma_set_fmap(struct qdma_pci_dev *qdma_dev, uint16_t devfn,
			uint32_t q_base, uint32_t q_count);
int qdma_identify_bars(struct rte_eth_dev *dev);
int qdma_get_hw_version(struct rte_eth_dev *dev);
/* implemented in rxtx.c */
uint16_t qdma_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
				uint16_t nb_pkts);
uint16_t qdma_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
				uint16_t nb_pkts);
int qdma_dev_rx_descriptor_done(void *rx_queue, uint16_t exp_count);

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
void qdma_desc_dump(struct rte_eth_dev *dev, uint32_t qid);

int index_of_array(uint32_t *arr, uint32_t n, uint32_t element);
int get_param(struct rte_eth_dev *dev, enum get_param_type param, void *value);
int get_queue_param(struct rte_eth_dev *dev, uint32_t qid,
			enum get_queue_param_type param, void *value);
int update_param(struct rte_eth_dev *dev, enum dev_param_type param,
			uint16_t value);
int update_queue_param(struct rte_eth_dev *dev, uint32_t qid,
				enum queue_param_type param, uint32_t value);
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
	snprintf(z_name, sizeof(z_name), "%s%s%d_%d",
			dev->device->driver->name, ring_name,
			dev->data->port_id, queue_id);
	return rte_memzone_reserve_aligned(z_name, (uint64_t)ring_size,
						socket_id, 0, QDMA_ALIGN);
}
#endif /* ifndef __QDMA_H__ */
