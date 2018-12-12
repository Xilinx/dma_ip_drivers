/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
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

#include <linux/gfp.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "libqdma_config.h"
#include "qdma_qconf_mgr.h"

/** for holding the Q configuration list */
static struct qconf_entry_head qconf_list;
/** mutex for protecting the qconf_list */
static DEFINE_MUTEX(qconf_mutex);

/* insert new to the position specified by previous */
static inline void list_insert(struct list_head *new, struct list_head *prev)
{
	new->next = prev;
	new->prev = prev->prev;
	prev->prev->next = new;
	prev->prev = new;
}

/*****************************************************************************/
/**
 * xdev_dump_qconf- dump the q configuration
 * @param[in]	xdev:	device type to be used PF/VF
 *
 * @return
 *****************************************************************************/
void xdev_dump_qconf(u32 xdev)
{
	struct qconf_entry *_qconf = NULL;
	struct qconf_entry *_tmp = NULL;
	struct list_head *listhead;
	const char *list_str[][8] = {{"vf"}, {"pf"}, {"vf-free"} };
	int end = 0;

	if (xdev == PCI_TYPE_PF)
		listhead = &qconf_list.pf_list;
	else
		listhead = &qconf_list.vf_list;

	pr_info("Dumping %s list\n", *list_str[xdev]);

	list_for_each_entry_safe(_qconf, _tmp, listhead, list_head) {
		end = ((int)(_qconf->qbase + _qconf->qmax)) - 1;
		if (end < 0)
			end = 0;
		pr_info("func_id = %u, qmax = %u, qbase = %u~%u, cfg_state = %c\n",
				_qconf->func_id, _qconf->qmax, _qconf->qbase,
				end,
				_qconf->cfg_state ? 'c' : 'i');
	}
}

/*****************************************************************************/
/**
 * xdev_dump_freelist - dump the freelist
 *
 * @return
 *****************************************************************************/
void xdev_dump_freelist(void)
{
	struct free_entry *_free = NULL;
	struct free_entry *_tmp = NULL;
	struct list_head *listhead = &qconf_list.vf_free_list;

	pr_info("Dumping free list\n");

	list_for_each_entry_safe(_free, _tmp, listhead, list_head) {
		pr_info("free-cnt = %u, qbase-next = %d\n",
				_free->free, _free->next_qbase);
	}
}

static struct qconf_entry *find_func_qentry(u32 xdev, u8 func_id)
{
	struct qconf_entry *_qconf = NULL;
	struct qconf_entry *_tmp = NULL;
	struct list_head *listhead = &qconf_list.vf_list;

	if (xdev)
		listhead = &qconf_list.pf_list;

	list_for_each_entry_safe(_qconf, _tmp, listhead, list_head) {
		if (_qconf->func_id == func_id)
			return _qconf;
	}

	return NULL;
}

struct free_entry *create_free_entry(u32 qmax, u32 qbase)
{
	struct free_entry *_free_entry = NULL;

	_free_entry = (struct free_entry *)kzalloc(sizeof(struct free_entry),
			GFP_KERNEL);
	if (!_free_entry)
		return _free_entry;
	_free_entry->free = qmax;
	_free_entry->next_qbase = qbase;

	return _free_entry;
}

static void cleanup_free_list(void)
{
	struct free_entry *_free_entry = NULL;
	struct free_entry *_free_tmp = NULL;

	mutex_lock(&qconf_mutex);
	list_for_each_entry_safe(_free_entry, _free_tmp,
			&qconf_list.vf_free_list, list_head) {
		list_del(&_free_entry->list_head);
		kfree(_free_entry);
	}
	mutex_unlock(&qconf_mutex);

}

/** initialize the free list */
static int init_vf_free_list(void)
{
	struct free_entry *_free_entry = NULL;
	u32 free = 0;
	u32 next_qbase = 0;

	free = qconf_list.vf_qmax;
	next_qbase = qconf_list.vf_qbase;

	cleanup_free_list();

	_free_entry = create_free_entry(free, next_qbase);
	mutex_lock(&qconf_mutex);
	list_add_tail(&_free_entry->list_head, &qconf_list.vf_free_list);
	mutex_unlock(&qconf_mutex);

	return 0;
}

/** finds a first fit to accomodate the qmax.
 * This function returns the qbase of the requested qmax
 * if found
 */
static int find_first_fit(u32 qmax, u32 *qbase)
{
	struct free_entry *_free_entry = NULL;
	struct free_entry *_tmp = NULL;
	int err = 0;
	u8 found = 0;

	pr_debug("Get first fit %u\n", qmax);
	mutex_lock(&qconf_mutex);
	list_for_each_entry_safe(_free_entry, _tmp,
			&qconf_list.vf_free_list, list_head) {
		if (_free_entry->free >= qmax) {
			*qbase = _free_entry->next_qbase;
			_free_entry->free -= qmax;
			_free_entry->next_qbase += qmax;
			/** reduce cfgd_free */
			qconf_list.qcnt_cfgd_free -= qmax;
			found = 1;
			break;
		}
	}
	mutex_unlock(&qconf_mutex);

	if (!found) {
		pr_warn("No free slot found free/qmax = %u/%u\n",
				_free_entry->free, qmax);
		err = -EINVAL;
	}

	return err;
}

/** To defragment the free list if any. if it finds a continous
 * range of qbases, merge both the entries into one.
 */
static int defrag_free_list(void)
{
	struct free_entry *_free = NULL;
	struct free_entry *_tmp = NULL;
	struct free_entry *_prev = NULL;
	struct list_head *listhead = &qconf_list.vf_free_list;
	int defrag_cnt = 0;

	pr_debug("Defragmenting free list\n");
	mutex_lock(&qconf_mutex);
	list_for_each_entry_safe(_free, _tmp, listhead, list_head) {
		if (_prev) {
			/** is it a continous range?? */
			if ((_prev->next_qbase + _prev->free)
					== _free->next_qbase) {
				_free->free += _prev->free;
				_free->next_qbase = _prev->next_qbase;
				list_del(&_prev->list_head);
				kfree(_prev);
				defrag_cnt++;
			}
		}
		_prev = _free;
	}
	mutex_unlock(&qconf_mutex);

	if (defrag_cnt)
		pr_debug("Defragmented %d entries\n", defrag_cnt);

	return defrag_cnt;
}

/**
 * function to update the free list. First find the place to
 * fit the free entry by comparing the qbases. Once found,
 * insert the new free entry and insert it before the found one
 */
static int update_free_list(struct qconf_entry *entry)
{
	struct free_entry *_free_entry = NULL;
	struct free_entry *_free_new = NULL;
	struct free_entry *_tmp = NULL;
	struct list_head *listhead = &qconf_list.vf_free_list;

	/** consider only user configured values */
	if (entry->cfg_state != Q_CONFIGURED)
		return 0;

	mutex_lock(&qconf_mutex);
	list_for_each_entry_safe(_free_entry, _tmp, listhead, list_head) {
		/** add it before the next bigger qbase */
		if (entry->qbase < _free_entry->next_qbase) {
			_free_new = create_free_entry(entry->qmax,
					entry->qbase);
			list_insert(&_free_new->list_head,
					&_free_entry->list_head);
			/** update the cfgd_free */
			qconf_list.qcnt_cfgd_free += entry->qmax;
			break;
		}
	}
	mutex_unlock(&qconf_mutex);

	defrag_free_list();

	return 0;
}

/** grab from initial qconf, make qmax=0, qbase=0 cfg_state as zeroed*/
static int grab_from_init_qconf(struct list_head *listhead, u8 func_id)
{
	struct qconf_entry *_qconf = NULL;
	struct qconf_entry *_tmp = NULL;
	int grab_cnt = 0;
	int ret = 0;

	mutex_lock(&qconf_mutex);
	if (qconf_list.qcnt_init_used > qconf_list.qcnt_cfgd_free) {
		grab_cnt = qconf_list.qcnt_init_used
			- qconf_list.qcnt_cfgd_free;
		qconf_list.qcnt_init_used -= grab_cnt;
	}

	ret = grab_cnt;

	pr_debug("grab_cnt = %u init_used = %u cfgd_free = %u\n",
			grab_cnt, qconf_list.qcnt_init_used,
			qconf_list.qcnt_cfgd_free);

	if (grab_cnt) {
		list_for_each_entry_safe(_qconf, _tmp, listhead, list_head) {
			/** already grabbed */
			if (!_qconf->qmax || (_qconf->cfg_state != Q_INITIAL))
				continue;
			if (!grab_cnt--)
				break;
			_qconf->qmax = 0;
			_qconf->qbase = 0;
			/** if grabbing from the same func_id,
			 * reduce the grab count by 1
			 */
			if (unlikely(_qconf->func_id == func_id))
				ret -= 1;
		}
	}
	mutex_unlock(&qconf_mutex);

	return ret;
}

/*****************************************************************************/
/**
 * xdev_set_qmax() - sets the qmax value of VF device from virtfn/qdma/qmax
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 * @param[in]	qmax:		qmax value to set
 * @param[out]	qbase:		pointer to return qbase value
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_set_qmax(u32 xdev, u8 func_id, u32 qmax, u32 *qbase)
{
	struct qconf_entry *func_entry = NULL;
	struct list_head *listhead = &qconf_list.vf_list;
	u32 num_qs_free = qconf_list.qcnt_cfgd_free;
	int err = 0;
	u32 _qbase = 0;
	int grab_cnt = 0;
	u32 cur_qbase = 0;
	u32 cur_qmax = 0;

	pr_debug("Setting qmax func_id = %u, qmax = %u\n", func_id, qmax);

	if (xdev)
		listhead = &qconf_list.pf_list;

	func_entry = find_func_qentry(xdev, func_id);
	if (!func_entry) {
		pr_err("Set qmax request on non available device %u\n",
				func_id);
		return -EINVAL;
	}
	cur_qmax = func_entry->qmax;
	cur_qbase = func_entry->qbase;
	if (func_entry->cfg_state == Q_CONFIGURED) {
		update_free_list(func_entry);
		num_qs_free = qconf_list.qcnt_cfgd_free;
	}

	if (qmax > num_qs_free) {
		pr_warn("No free qs!, requested/free = %u/%u\n",
				qmax, num_qs_free);
		return -EINVAL;
	}

	func_entry->qbase = 0;
	err = find_first_fit(qmax, &_qbase);
	if (err < 0) {
		if (qmax)
			pr_info("Not able to find a fit for qmax = %u\n",
					qmax);
		else
			pr_info("0 q size, func_id = %u\n", func_id);

		if (func_entry->cfg_state == Q_CONFIGURED) {
			/** if failed to get a qbase, revert it to old values */
			func_entry->qmax = cur_qmax;
			func_entry->qbase = cur_qbase;
		}

		return err;
	}

	func_entry->cfg_state = Q_CONFIGURED;
	func_entry->qmax = qmax;
	func_entry->qbase = _qbase;
	*qbase = _qbase;
	mutex_lock(&qconf_mutex);
	list_del(&func_entry->list_head);
	list_add_tail(&func_entry->list_head, listhead);
	mutex_unlock(&qconf_mutex);
	grab_cnt = grab_from_init_qconf(listhead, func_id);

	return grab_cnt;
}

/*****************************************************************************/
/**
 * xdev_del_qconf - delete the q configuration entry of funcid
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_del_qconf(u32 xdev, u8 func_id)
{
	struct qconf_entry *_qconf = NULL;

	_qconf = find_func_qentry(xdev, func_id);
	if (_qconf)
		_qconf->cfg_state = Q_INITIAL;

	return 0;
}

/*****************************************************************************/
/**
 * xdev_create_qconf - add the q configuration entry of funcid during hello
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
struct qconf_entry *xdev_create_qconf(u32 xdev, u8 func_id)
{
	struct qconf_entry *_qconf = NULL;
	struct list_head *listhead = &qconf_list.vf_list;

	pr_debug("Creating func_id = %u\n", func_id);
	if (xdev)
		listhead = &qconf_list.pf_list;

	_qconf = (struct qconf_entry *)kzalloc(sizeof(struct qconf_entry),
			GFP_KERNEL);
	if (!_qconf)
		return _qconf;

	_qconf->func_id = func_id;
	_qconf->cfg_state = Q_INITIAL;
	if (qconf_list.qcnt_cfgd_free) {
		/* _qconf->qmax = 1; */
		/* _qconf->qbase = 2047 - qconf_list.qcnt_init_used; */
		_qconf->qmax = 1;
		_qconf->qbase = 0;
	}

	mutex_lock(&qconf_mutex);
	if (qconf_list.qcnt_init_used < qconf_list.vf_qmax)
		qconf_list.qcnt_init_used++;
	list_add_tail(&_qconf->list_head, listhead);
	mutex_unlock(&qconf_mutex);

	atomic_inc(&qconf_list.vf_cnt);

	return _qconf;
}

/*****************************************************************************/
/**
 * xdev_destroy_qconf - destroys the qconf entry during bye
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_destroy_qconf(u32 xdev, u8 func_id)
{
	struct qconf_entry *_qconf = NULL;

	pr_debug("Destroying func_id = %u\n", func_id);
	_qconf = find_func_qentry(xdev, func_id);
	if (_qconf) {
		mutex_lock(&qconf_mutex);
		list_del(&_qconf->list_head);
		mutex_unlock(&qconf_mutex);
		update_free_list(_qconf);
		if (_qconf->cfg_state == Q_INITIAL)
			qconf_list.qcnt_init_used -= _qconf->qmax;
		kfree(_qconf);
		atomic_dec(&qconf_list.vf_cnt);
	}

	return 0;
}

/*****************************************************************************/
/**
 * xdev_qconf_init - initialize the q configuration structures
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_qconf_init(void)
{
	INIT_LIST_HEAD(&qconf_list.vf_list);
	INIT_LIST_HEAD(&qconf_list.pf_list);
	INIT_LIST_HEAD(&qconf_list.vf_free_list);
	atomic_set(&qconf_list.vf_cnt, 0);
	qconf_list.vf_qmax = TOTAL_VF_QS;
	qconf_list.pf_qmax = TOTAL_QDMA_QS - qconf_list.vf_qmax;
	qconf_list.vf_qbase = qconf_list.pf_qmax;
	qconf_list.qcnt_cfgd_free = qconf_list.vf_qmax;
	qconf_list.qcnt_init_free = qconf_list.vf_qmax;
	qconf_list.qcnt_init_used = 0;
	init_vf_free_list();

	return 0;
}

/*****************************************************************************/
/**
 * xdev_qconf_cleanup - cleanup the q configuration
 *
 * @return
 *****************************************************************************/
void xdev_qconf_cleanup(void)
{
	struct qconf_entry *_qconf = NULL;
	struct qconf_entry *_tmp = NULL;
	struct list_head *listhead = &qconf_list.vf_list;

	mutex_lock(&qconf_mutex);
	list_for_each_entry_safe(_qconf, _tmp,
			listhead, list_head) {
		list_del(&_qconf->list_head);
		kfree(_qconf);
	}

	_qconf = NULL;
	listhead = &qconf_list.pf_list;
	list_for_each_entry_safe(_qconf, _tmp,
			listhead, list_head) {
		list_del(&_qconf->list_head);
		kfree(_qconf);
	}
	mutex_unlock(&qconf_mutex);

	cleanup_free_list();
}

/*****************************************************************************/
/**
 * qconf_get_qmax	- get the current qmax value of VF/PF
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	card_id:	for differntiation the qdma card
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qconf_get_qmax(u32 xdev, u32 card_id)
{
	int qmax = 0;

	mutex_lock(&qconf_mutex);
	if (xdev == PCI_TYPE_PF)
		qmax = qconf_list.pf_qmax;
	else if (xdev == PCI_TYPE_VF)
		qmax = qconf_list.vf_qmax;
	else
		pr_err("Invalid q id type specified\n");
	mutex_unlock(&qconf_mutex);

	return qmax;
}

/*****************************************************************************/
/**
 * is_vfqmax_configurable	- checks if the vfqmax is configurable.
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: if not configurable
 * @return	1: if configurable
 *****************************************************************************/
int is_vfqmax_configurable(void)
{
	return (atomic_read(&qconf_list.vf_cnt) == 0);
}

/*****************************************************************************/
/**
 * set_vf_qmax	- set the value for vf_qmax;
 *
 * @param[in]	qmax:	qmax value to set
 *
 * @return: last set qmax value before qmax.
 *****************************************************************************/
int set_vf_qmax(u32 qmax)
{
	u32 last_qmax = qconf_list.vf_qmax;

	mutex_lock(&qconf_mutex);
	qconf_list.vf_qmax = qmax;
	qconf_list.pf_qmax = TOTAL_QDMA_QS - qconf_list.vf_qmax;
	qconf_list.vf_qbase = qconf_list.pf_qmax;
	qconf_list.qcnt_cfgd_free = qconf_list.vf_qmax;
	qconf_list.qcnt_init_free = qconf_list.vf_qmax;
	qconf_list.qcnt_init_used = 0;
	mutex_unlock(&qconf_mutex);

	pr_debug("vf_qmax/pf_qmax/vf_qbase/cfgd_free/init_free/init_used %u/%u/%u/%u/%u/%u\n",
			qconf_list.vf_qmax, qconf_list.pf_qmax,
			qconf_list.vf_qbase, qconf_list.qcnt_cfgd_free,
			qconf_list.qcnt_init_free, qconf_list.qcnt_init_used);

	init_vf_free_list();

	return last_qmax;
}
