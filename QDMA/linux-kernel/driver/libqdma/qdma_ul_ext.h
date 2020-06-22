#ifndef QDMA_UL_EXT_H__
#define QDMA_UL_EXT_H__
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

/**
 * @file
 * @brief This file provides the interface definitions for bypass extension
 *        modules to be plugged in
 */

#include "libqdma_export.h"

/**
 * struct qdma_q_desc_list - descriptor list
 */
struct qdma_q_desc_list {
	/** @desc: pointer to the descriptor */
	void *desc;
	/** @next: pointer to the next list element */
	struct qdma_q_desc_list *next;
};

/*****************************************************************************/
/**
 * qdma_sgl_find_offset() - find the sgl offset for the request under process
 *
 * @req: request under process
 * @sg_p: pointer to the entry in the sgl list
 * @sg_offset: offset in the entry of sgl list
 *
 * Return: 0: success; <0: on any error
 *
 *****************************************************************************/
int qdma_sgl_find_offset(struct qdma_request *req, struct qdma_sw_sg **sg_p,
		unsigned int *sg_offset);

/*****************************************************************************/
/**
 * qdma_update_request() - update the request current processed info
 *
 * @q_hndl: handle to the q with which bypass module can request descriptors
 * @req: request under process
 * @num_desc: number of descriptors consumed consumed in the process
 * @data_cnt: amount of data processed in the request
 * @sg_offset: offset in the @sg
 * @sg: next sg to be serviced
 *
 *****************************************************************************/
void qdma_update_request(void *q_hndl, struct qdma_request *req,
		unsigned int num_desc,
		unsigned int data_cnt,
		unsigned int sg_offset,
		void *sg);

/*****************************************************************************/
/**
 * qdma_q_desc_get() - request @desc_cnt number of descriptors for the q
 *                     specified by @q_hndl
 *
 * @q_hndl: handle to the q with which bypass module can request descriptors
 * @desc_cnt: number of descriptors required
 * @desc_list: list of descriptors to be provided for this request
 *
 * Return: 0: success; <0: if number of requested descriptors not available
 *
 *****************************************************************************/
int qdma_q_desc_get(void *q_hndl, const unsigned int desc_cnt,
		struct qdma_q_desc_list **desc_list);

/*****************************************************************************/
/**
 * qdma_q_init_pointers() - update the pidx/cidx pointers of the q specified
 *				by @q_hndl
 *
 * @q_hndl: handle to the q with which bypass module can request descriptors
 *
 * Return: 0: success; <0: on failure
 *
 *****************************************************************************/
int qdma_q_init_pointers(void *q_hndl);

#endif /* QDMA_UL_EXT_H__ */
