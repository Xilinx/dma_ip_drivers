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

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_thread.h"

#include <linux/kernel.h>

#include "qdma_descq.h"
#include "thread.h"
#include "xdev.h"

/* ********************* global variables *********************************** */

static unsigned int thread_cnt;
/** completion status threads */
static struct qdma_kthread *cs_threads;

static spinlock_t	qcnt_lock;
static unsigned int cpu_count;
static unsigned int *per_cpu_qcnt;

/* ********************* static function declarations *********************** */

static int qdma_thread_cmpl_status_pend(struct list_head *work_item);
static int qdma_thread_cmpl_status_proc(struct list_head *work_item);

/* ********************* static function definitions ************************ */
static int qdma_thread_cmpl_status_pend(struct list_head *work_item)
{
	struct qdma_descq *descq = list_entry(work_item, struct qdma_descq,
						cmplthp_list);
	int pend = 0;

	lock_descq(descq);
	pend = !list_empty(&descq->pend_list) || !list_empty(&descq->work_list);
	unlock_descq(descq);

	return pend;
}

static int qdma_thread_cmpl_status_proc(struct list_head *work_item)
{
	struct qdma_descq *descq;

	descq = list_entry(work_item, struct qdma_descq, cmplthp_list);
	qdma_descq_service_cmpl_update(descq, 0, 1);
	return 0;
}

/* ********************* public function definitions ************************ */

void qdma_thread_remove_work(struct qdma_descq *descq)
{
	struct qdma_kthread *cmpl_thread;
	int cpu_idx = cpu_count;


	lock_descq(descq);
	cmpl_thread = descq->cmplthp;
	descq->cmplthp = NULL;

	if (descq->cpu_assigned) {
		descq->cpu_assigned = 0;
		cpu_idx = descq->intr_work_cpu;
	}

	pr_debug("%s removing from thread %s, %u.\n",
		descq->conf.name, cmpl_thread ? cmpl_thread->name : "?",
		cpu_idx);

	unlock_descq(descq);

	if (cpu_idx < cpu_count) {
		spin_lock(&qcnt_lock);
		per_cpu_qcnt[cpu_idx]--;
		spin_unlock(&qcnt_lock);
	}

	if (cmpl_thread) {
		lock_thread(cmpl_thread);
		list_del(&descq->cmplthp_list);
		cmpl_thread->work_cnt--;
		unlock_thread(cmpl_thread);
	}
}

void qdma_thread_add_work(struct qdma_descq *descq)
{
	struct qdma_kthread *thp = cs_threads;
	unsigned int v = 0;
	int i, idx = thread_cnt;

	if (descq->xdev->conf.qdma_drv_mode != POLL_MODE) {
		spin_lock(&qcnt_lock);
		idx = cpu_count - 1;
		v = per_cpu_qcnt[idx];
		for (i = idx - 1; i >= 0  && v; i--) {
			if (per_cpu_qcnt[i] < v) {
				idx = i;
				v = per_cpu_qcnt[i];
			}
		}

		per_cpu_qcnt[idx]++;
		spin_unlock(&qcnt_lock);

		lock_descq(descq);
		descq->cpu_assigned = 1;
		descq->intr_work_cpu = idx;
		unlock_descq(descq);

		pr_debug("%s 0x%p assigned to cpu %u.\n",
			descq->conf.name, descq, idx);

		return;
	}

	/* Polled mode only */
	for (i = 0; i < thread_cnt; i++, thp++) {
		lock_thread(thp);
		if (idx == thread_cnt) {
			v = thp->work_cnt;
			idx = i;
		} else if (!thp->work_cnt) {
			idx = i;
			unlock_thread(thp);
			break;
		} else if (thp->work_cnt < v)
			idx = i;
		unlock_thread(thp);
	}

	thp = cs_threads + idx;
	lock_thread(thp);
	list_add_tail(&descq->cmplthp_list, &thp->work_list);
	descq->intr_work_cpu = idx;
	thp->work_cnt++;
	unlock_thread(thp);

	pr_debug("%s 0x%p assigned to cmpl status thread %s,%u.\n",
		descq->conf.name, descq, thp->name, thp->work_cnt);

	lock_descq(descq);
	descq->cmplthp = thp;
	unlock_descq(descq);
}

int qdma_threads_create(unsigned int num_threads)
{
	struct qdma_kthread *thp;
	int i;
	int rv;

	if (thread_cnt) {
		pr_warn("threads already created!");
		return 0;
	}
	spin_lock_init(&qcnt_lock);

	cpu_count = num_online_cpus();
	per_cpu_qcnt = kzalloc(cpu_count * sizeof(unsigned int), GFP_KERNEL);
	if (!per_cpu_qcnt)
		return -ENOMEM;

	thread_cnt = (num_threads == 0) ? cpu_count : num_threads;

	cs_threads = kzalloc(thread_cnt * sizeof(struct qdma_kthread),
					GFP_KERNEL);
	if (!cs_threads)
		return -ENOMEM;

	/* N dma writeback monitoring threads */
	thp = cs_threads;
	for (i = 0; i < thread_cnt; i++, thp++) {
		thp->cpu = i;
		thp->kth_timeout = 0;
		rv = qdma_kthread_start(thp, "qdma_cmpl_status_th", i);
		if (rv < 0)
			goto cleanup_threads;
		thp->fproc = qdma_thread_cmpl_status_proc;
		thp->fpending = qdma_thread_cmpl_status_pend;
	}

	return 0;

cleanup_threads:
	kfree(cs_threads);
	cs_threads = NULL;
	thread_cnt = 0;

	return rv;
}

void qdma_threads_destroy(void)
{
	int i;
	struct qdma_kthread *thp;

	if (per_cpu_qcnt) {
		spin_lock(&qcnt_lock);
		kfree(per_cpu_qcnt);
		per_cpu_qcnt = NULL;
		spin_unlock(&qcnt_lock);
	}

	if (!thread_cnt)
		return;

	/* N dma writeback monitoring threads */
	thp = cs_threads;
	for (i = 0; i < thread_cnt; i++, thp++)
		if (thp->fproc)
			qdma_kthread_stop(thp);

	kfree(cs_threads);
	cs_threads = NULL;
	thread_cnt = 0;
}
