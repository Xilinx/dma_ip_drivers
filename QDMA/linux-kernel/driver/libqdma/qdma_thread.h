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

#ifndef LIBQDMA_QDMA_THREAD_H_
#define LIBQDMA_QDMA_THREAD_H_
/**
 * @file
 * @brief This file contains the declarations for qdma thread handlers
 *
 */

/** qdma_descq forward declaration */
struct qdma_descq;

/*****************************************************************************/
/**
 * qdma_threads_create() - create qdma threads
 * This functions creates two threads for each cpu in the system or number of
 * number of thread requested by param num_threads and assigns the
 * thread handlers
 * 1: queue processing thread
 * 2: queue completion handler thread
 *
 * @param[in] num_threads - number of threads to be created
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_threads_create(unsigned int num_threads);

/*****************************************************************************/
/**
 * qdma_threads_destroy() - destroy all the qdma threads created
 *                          during system initialization
 *
 * @return	none
 *****************************************************************************/
void qdma_threads_destroy(void);

/*****************************************************************************/
/**
 * qdma_thread_remove_work() - handler to remove the attached work thread
 *
 * @param[in]	descq:	pointer to qdma_descq
 *
 * @return	none
 *****************************************************************************/
void qdma_thread_remove_work(struct qdma_descq *descq);

/*****************************************************************************/
/**
 * qdma_thread_add_work() - handler to add a work thread
 *
 * @param[in]	descq:	pointer to qdma_descq
 *
 * @return	none
 *****************************************************************************/
void qdma_thread_add_work(struct qdma_descq *descq);

#endif /* LIBQDMA_QDMA_THREAD_H_ */
