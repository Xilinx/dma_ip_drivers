/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
 * All rights reserved.
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

#ifndef __QDMA_ST_C2H_H__
#define __QDMA_ST_C2H_H__
/**
 * @file
 * @brief This file contains the declarations for qdma st c2h processing
 *
 */
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include "qdma_descq.h"
#ifdef ERR_DEBUG
#include "qdma_nl.h"
#endif

/**
 * @struct - qdma_sdesc_info
 * @brief	qdma descriptor information
 */
struct qdma_sdesc_info {
	/** pointer to next descriptor  */
	struct qdma_sdesc_info *next;
	/**
	 * @union - desciptor flags
	 */
	union {
		/** 8 flag bits  */
		u8 fbits;
		/**
		 * @struct - flags
		 * @brief	desciptor flags
		 */
		struct flags {
			/** is descriptor valid */
			u8 valid:1;
			/** start of the packet */
			u8 sop:1;
			/** end of the packet */
			u8 eop:1;
			/** filler for 5 bits */
			u8 filler:5;
		} f;
	};
	/** reserved 3 bits */
	u8 rsvd[3];
	/** consumer index */
	unsigned int cidx;
};

/**
 * struct qdma_sw_pg_sg - qdma page scatter gather request
 *
 */
struct qdma_sw_pg_sg {
	/** @pg_base: pointer to current page */
	struct page *pg_base;
	/** @pg_dma_base_addr: dma address of the allocated page */
	dma_addr_t pg_dma_base_addr;
	/** @pg_offset: page offset for all pages */
	unsigned int pg_offset;
};

/**
 * @struct - qdma_flq
 * @brief qdma free list q page allocation book keeping
 */
struct qdma_flq {
	/** RO: size of the decriptor */
	unsigned int size;
	/** RO: c2h buffer size */
	unsigned int desc_buf_size;
	/** RO: number of pages */
	unsigned int num_pages;
	/** RO: Mask for number of pages */
	unsigned int num_pgs_mask;
	/** RO: number of buffers per page */
	unsigned int num_bufs_per_pg;
	/** RO: number of currently allocated page index */
	unsigned int alloc_idx;
	/** RO: number of currently recycled page index */
	unsigned int recycle_idx;
	/** RO: max page offset */
	unsigned int max_pg_offset;
	/** RO: page order */
	unsigned int buf_pg_mask;
	/** RO: desc page order */
	unsigned char desc_pg_order;
	/** RO: desc page shift */
	unsigned char desc_pg_shift;
	/** RO: page shift */
	unsigned char buf_pg_shift;
	/** RO: pointer to qdma c2h decriptor */
	struct qdma_c2h_desc *desc;

	/** RW: total # of udd outstanding */
	unsigned int udd_cnt;
	/** RW: total # of packet outstanding */
	unsigned int pkt_cnt;
	/** RW: total # of pkt payload length outstanding */
	unsigned int pkt_dlen;
	/** RW: # of available Rx buffers */
	unsigned int avail;
	/** RW: # of times buffer allocation failed */
	unsigned long alloc_fail;
	/** RW: # of RX Buffer DMA Mapping failures */
	unsigned long mapping_err;
	/** RW: consumer index */
	unsigned int cidx;
	/** RW: producer index */
	unsigned int pidx;
	/** RW: pending pidxes */
	unsigned int pidx_pend;
	/** RW: Page list */
	struct qdma_sw_pg_sg *pg_sdesc;
	/** RW: sw scatter gather list */
	struct qdma_sw_sg *sdesc;
	/** RW: sw descriptor info */
	struct qdma_sdesc_info *sdesc_info;
};

/*****************************************************************************/
/**
 * qdma_descq_rxq_read() - read from the rx queue
 *
 * @param[in]	descq:		pointer to qdma_descq
 * @param[in]	req:		queue request
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_rxq_read(struct qdma_descq *descq, struct qdma_request *req);

/**
 * qdma_descq_dump_cmpt() - dump the completion queue descriptors
 *
 * @param[in]	descq:		pointer to qdma_descq
 * @param[in]	start:		start completion descriptor index
 * @param[in]	end:		end completion descriptor index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_descq_dump_cmpt(struct qdma_descq *descq, int start, int end,
		char *buf, int buflen);

/*****************************************************************************/
/**
 * incr_cmpl_desc_cnt() - update the interrupt cidx
 *
 * @param[in]   descq:      pointer to qdma_descq
 * @param[in]   cnt:        increment value
 *
 *****************************************************************************/
void incr_cmpl_desc_cnt(struct qdma_descq *descq, unsigned int cnt);

/*****************************************************************************/
/**
 * descq_flq_free_resource() - handler to free the pages for the request
 *
 * @param[in]	descq:		pointer to qdma_descq
 *
 * @return	none
 *****************************************************************************/
void descq_flq_free_resource(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * descq_flq_alloc_resource() - handler to allocate the pages for the request
 *
 * @param[in]	descq:		pointer to qdma_descq
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int descq_flq_alloc_resource(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * descq_process_completion_st_c2h() - handler to process the st c2h
 *				completion request
 *
 * @param[in]	descq:		pointer to qdma_descq
 * @param[in]	budget:		number of descriptors to process
 * @param[in]	upd_cmpl:	if update completion required
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int descq_process_completion_st_c2h(struct qdma_descq *descq, int budget,
					bool upd_cmpl);

/*****************************************************************************/
/**
 * descq_st_c2h_read() - handler to process the st c2h read request
 *
 * @param[in]	descq:		pointer to qdma_descq
 * @param[in]	req:		pointer to read request
 * @param[in]	update_pidx:		flag to update the request
 * @param[in]	refill:		flag to indicate whether to refill the flq
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int descq_st_c2h_read(struct qdma_descq *descq, struct qdma_request *req,
			bool update_pidx, bool refill);

#endif /* ifndef __QDMA_ST_C2H_H__ */
