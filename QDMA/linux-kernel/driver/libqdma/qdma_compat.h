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
 * @brief This file is used to allow the driver to be compiled under multiple
 *	versions of Linux with as few obtrusive in-line ifdef's as possible.
 */

#ifndef __QDMA_COMPAT_H
#define __QDMA_COMPAT_H

#include <linux/version.h>
#include <asm/barrier.h>

/**
 * if linux kernel version is < 3.19.0
 * then define the dma_rmb and dma_wmb
 */
#if KERNEL_VERSION(3, 19, 0) > LINUX_VERSION_CODE

#ifndef dma_rmb
#define dma_rmb		rmb
#endif /* #ifndef dma_rmb */

#ifndef dma_wmb
#define dma_wmb		wmb
#endif /* #ifndef dma_wmb */

#endif

#ifdef RHEL_RELEASE_VERSION
#define qdma_wait_queue                 wait_queue_head_t
#define qdma_waitq_init                 init_waitqueue_head
#define qdma_waitq_wakeup               wake_up_interruptible
#define qdma_waitq_wait_event           wait_event_interruptible
#define qdma_waitq_wait_event_timeout   wait_event_interruptible_timeout
#else
/* use simple wait queue (swaitq) with kernels > 4.6.0 but < 4.19.0  */
#if ((KERNEL_VERSION(4, 6, 0) <= LINUX_VERSION_CODE) && \
		(KERNEL_VERSION(4, 19, 0) >= LINUX_VERSION_CODE))
#include <linux/swait.h>

#define qdma_wait_queue                 struct swait_queue_head
#define qdma_waitq_init                 init_swait_queue_head
#define qdma_waitq_wakeup               swake_up
#define qdma_waitq_wait_event           swait_event_interruptible
#define qdma_waitq_wait_event_timeout   swait_event_interruptible_timeout

#else
#include <linux/wait.h>

#define qdma_wait_queue                 wait_queue_head_t
#define qdma_waitq_init                 init_waitqueue_head
#define qdma_waitq_wakeup               wake_up_interruptible
#define qdma_waitq_wait_event           wait_event_interruptible
#define qdma_waitq_wait_event_timeout   wait_event_interruptible_timeout

#endif  /* swaitq */
#endif

/* timer */
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
#define qdma_timer_setup(timer, fp_handler, data) \
		timer_setup(timer, fp_handler, 0)

#define qdma_timer_start(timer, expires) \
		mod_timer(timer, round_jiffies(jiffies + (expires)))

#else
#define qdma_timer_setup(timer, fp_handler, priv)	\
	do { \
		init_timer(timer); \
		timer->data = (unsigned long)priv; \
		timer->function = fp_handler; \
		del_timer(timer); \
	} while (0)

#define qdma_timer_start(timer, timeout) \
	do { \
		del_timer(timer); \
		timer->expires = jiffies + (timeout); \
		add_timer(timer); \
	} while (0)


#endif /* timer */


#endif /* #ifndef __QDMA_COMPAT_H */
