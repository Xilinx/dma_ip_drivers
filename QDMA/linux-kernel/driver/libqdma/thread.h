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

#ifndef __XDMA_KTHREAD_H__
#define __XDMA_KTHREAD_H__
/**
 * @file
 * @brief This file contains the declarations for qdma kernel threads
 *
 */
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/cpuset.h>
#include <linux/signal.h>
#include "qdma_compat.h"

/**
 * @struct - qdma_kthread
 * @brief	qdma thread book keeping parameters
 */
struct qdma_kthread {
	/**  thread lock*/
	spinlock_t lock;
	/**  name of the thread */
	char name[26];
	/**  cpu number for which the thread associated with */
	unsigned short cpu;
	/**  thread id */
	unsigned short id;
	/**  thread sleep timeout value */
	unsigned int kth_timeout;
	/**  flags for thread */
	unsigned long flag;
	/**  thread wait queue */
	qdma_wait_queue waitq;
	/* flag to indicate scheduling of thread */
	unsigned int schedule;
	/**  kernel task structure associated with thread*/
	struct task_struct *task;
	/**  thread work list count */
	unsigned int work_cnt;
	/**  thread work list count */
	struct list_head work_list;
	/**  thread initialization handler */
	int (*finit)(struct qdma_kthread *thp);
	/**  thread pending handler */
	int (*fpending)(struct list_head *work_item);
	/**  thread peocessing handler */
	int (*fproc)(struct list_head *work_item);
	/**  thread done handler */
	int (*fdone)(struct qdma_kthread *thp);
};

/*****************************************************************************/
/**
 * qdma_kthread_dump() - handler to dump the thread information
 *
 * @param[in]	thp:		pointer to qdma_kthread
 * @param[in]	detail:		flag to indicate whether details required or not
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	length of the buffer
 *****************************************************************************/
int qdma_kthread_dump(struct qdma_kthread *thp, char *buf, int buflen,
			int detail);

#ifdef DEBUG_THREADS
#define lock_thread(thp)	\
	do { \
		pr_debug("locking thp %s ...\n", (thp)->name); \
		spin_lock(&(thp)->lock); \
	} while (0)

#define unlock_thread(thp)	\
	do { \
		pr_debug("unlock thp %s ...\n", (thp)->name); \
		spin_unlock(&(thp)->lock); \
	} while (0)

#define qdma_kthread_wakeup(thp)	\
	do { \
		pr_debug("signaling thp %s ...\n", (thp)->name); \
		thp->schedule = 1; \
		qdma_waitq_wakeup(&thp->waitq); \
	} while (0)

#define pr_debug_thread(fmt, ...) pr_debug(fmt, __VA_ARGS__)

#else
/** lock thread macro */
#define lock_thread(thp)		spin_lock(&(thp)->lock)
/** un lock thread macro */
#define unlock_thread(thp)		spin_unlock(&(thp)->lock)
/** macro to wake up the qdma k thread */
#define qdma_kthread_wakeup(thp) \
	do { \
		thp->schedule = 1; \
		qdma_waitq_wakeup(&thp->waitq); \
	} while (0)
/** pr_debug_thread */
#define pr_debug_thread(fmt, ...)
#endif

/*****************************************************************************/
/**
 * qdma_kthread_start() - handler to start the kernel thread
 *
 * @param[in]	thp:	pointer to qdma_kthread
 * @param[in]	name:	name for the thread
 * @param[in]	id:		thread id
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_kthread_start(struct qdma_kthread *thp, char *name, int id);

/*****************************************************************************/
/**
 * qdma_kthread_stop() - handler to stop the kernel thread
 *
 * @param[in]	thp:	pointer to qdma_kthread
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qdma_kthread_stop(struct qdma_kthread *thp);

#endif /* #ifndef __XDMA_KTHREAD_H__ */
