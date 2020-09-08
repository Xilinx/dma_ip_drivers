/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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

#include "qdma_resource_mgmt.h"
#include "qdma_platform.h"
#include "qdma_list.h"
#include "qdma_access_errors.h"

#ifdef ENABLE_WPP_TRACING
#include "qdma_resource_mgmt.tmh"
#endif

struct qdma_resource_entry {
	int qbase;
	uint32_t total_q;
	struct qdma_list_head node;
};

/** per function entry */
struct qdma_dev_entry {
	uint16_t func_id;
	uint32_t active_h2c_qcnt;
	uint32_t active_c2h_qcnt;
	uint32_t active_cmpt_qcnt;
	struct qdma_resource_entry entry;
};

/** for hodling the qconf_entry structure */
struct qdma_resource_master {
	/** DMA device index this resource belongs to */
	uint32_t dma_device_index;
	/** starting pci bus number this resource belongs to */
	uint32_t pci_bus_start;
	/** ending pci bus number this resource belongs to */
	uint32_t pci_bus_end;
	/** total queue this resource manager handles */
	uint32_t total_q;
	/** queue base from which this resource manger handles */
	int qbase;
	/** for attaching to master resource list */
	struct qdma_list_head node;
	/** for holding device entries */
	struct qdma_list_head dev_list;
	/** for holding free resource list */
	struct qdma_list_head free_list;
	/** active queue count per resource*/
	uint32_t active_qcnt;
};

static QDMA_LIST_HEAD(master_resource_list);

static struct qdma_resource_master *qdma_find_master_resource_entry(
		uint32_t bus_start, uint32_t bus_end)
{
	struct qdma_list_head *entry, *tmp;

	qdma_resource_lock_take();
	qdma_list_for_each_safe(entry, tmp, &master_resource_list) {
		struct qdma_resource_master *q_resource =
			(struct qdma_resource_master *)
			QDMA_LIST_GET_DATA(entry);

		if (q_resource->pci_bus_start == bus_start &&
			q_resource->pci_bus_end == bus_end) {
			qdma_resource_lock_give();
			return q_resource;
		}
	}
	qdma_resource_lock_give();

	return NULL;
}

static struct qdma_resource_master *qdma_get_master_resource_entry(
		uint32_t dma_device_index)
{
	struct qdma_list_head *entry, *tmp;

	qdma_resource_lock_take();
	qdma_list_for_each_safe(entry, tmp, &master_resource_list) {
		struct qdma_resource_master *q_resource =
			(struct qdma_resource_master *)
				QDMA_LIST_GET_DATA(entry);

		if (q_resource->dma_device_index == dma_device_index) {
			qdma_resource_lock_give();
			return q_resource;
		}
	}
	qdma_resource_lock_give();

	return NULL;
}

static struct qdma_dev_entry *qdma_get_dev_entry(uint32_t dma_device_index,
						uint16_t func_id)
{
	struct qdma_list_head *entry, *tmp;
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);

	if (!q_resource)
		return NULL;

	qdma_resource_lock_take();
	qdma_list_for_each_safe(entry, tmp, &q_resource->dev_list) {
		struct qdma_dev_entry *dev_entry = (struct qdma_dev_entry *)
			QDMA_LIST_GET_DATA(entry);

		if (dev_entry->func_id == func_id) {
			qdma_resource_lock_give();
			return dev_entry;
		}
	}
	qdma_resource_lock_give();

	return NULL;
}

static struct qdma_resource_entry *qdma_free_entry_create(int q_base,
							  uint32_t total_q)
{
	struct qdma_resource_entry *entry = (struct qdma_resource_entry *)
		qdma_calloc(1, sizeof(struct qdma_resource_master));
	if (entry == NULL)
		return NULL;

	entry->total_q = total_q;
	entry->qbase = q_base;

	return entry;
}

static void qdma_submit_to_free_list(struct qdma_dev_entry *dev_entry,
				     struct qdma_list_head *head)
{
	struct qdma_resource_entry *streach_node = NULL;
	struct qdma_list_head *entry, *tmp;
	/* create a new node to be added to empty free list */
	struct qdma_resource_entry *new_node = NULL;

	if (!dev_entry->entry.total_q)
		return;

	if (qdma_list_is_empty(head)) {
		new_node = qdma_free_entry_create(dev_entry->entry.qbase,
				dev_entry->entry.total_q);
		if (new_node == NULL)
			return;
		QDMA_LIST_SET_DATA(&new_node->node, new_node);
		qdma_list_add_tail(&new_node->node, head);
		/* reset device entry q resource params */
		dev_entry->entry.qbase = -1;
		dev_entry->entry.total_q = 0;
	} else {
		qdma_list_for_each_safe(entry, tmp, head) {
			struct qdma_resource_entry *node =
				(struct qdma_resource_entry *)
					QDMA_LIST_GET_DATA(entry);

			/* insert the free slot at appropriate place */
			if ((node->qbase > dev_entry->entry.qbase) ||
				qdma_list_is_last_entry(entry, head)) {
				new_node = qdma_free_entry_create(
						dev_entry->entry.qbase,
						dev_entry->entry.total_q);
				if (new_node == NULL)
					return;
				QDMA_LIST_SET_DATA(&new_node->node, new_node);
				if (node->qbase > dev_entry->entry.qbase)
					qdma_list_insert_before(&new_node->node,
								&node->node);
				else
					qdma_list_add_tail(&new_node->node,
							   head);
				/* reset device entry q resource params */
				dev_entry->entry.qbase = -1;
				dev_entry->entry.total_q = 0;
				break;
			}
		}
	}

	/* de-fragment (merge contiguous resource chunks) if possible */
	qdma_list_for_each_safe(entry, tmp, head) {
		struct qdma_resource_entry *node =
		(struct qdma_resource_entry *)QDMA_LIST_GET_DATA(entry);

		if (!streach_node)
			streach_node = node;
		else {
			if ((streach_node->qbase + streach_node->total_q) ==
					(uint32_t)node->qbase) {
				streach_node->total_q += node->total_q;
				qdma_list_del(&node->node);
				qdma_memfree(node);
			} else
				streach_node = node;
		}
	}
}

/**
 * qdma_resource_entry() - return the best free list entry node that can
 *                         accommodate the new request
 */
static struct qdma_resource_entry *qdma_get_resource_node(uint32_t qmax,
							  int qbase,
				   struct qdma_list_head *free_list_head)
{
	struct qdma_list_head *entry, *tmp;
	struct qdma_resource_entry *best_fit_node = NULL;

	/* try to honor requested qbase */
	if (qbase >= 0) {
		qdma_list_for_each_safe(entry, tmp, free_list_head) {
			struct qdma_resource_entry *node =
			(struct qdma_resource_entry *)QDMA_LIST_GET_DATA(entry);

			if ((qbase >= node->qbase) &&
					(node->qbase + node->total_q) >=
					(qbase + qmax)) {
				best_fit_node = node;
				goto fragment_free_list;
			}
		}
	}
	best_fit_node = NULL;

	/* find a best node to accommodate q resource request */
	qdma_list_for_each_safe(entry, tmp, free_list_head) {
		struct qdma_resource_entry *node =
		(struct qdma_resource_entry *)QDMA_LIST_GET_DATA(entry);

		if (node->total_q >= qmax) {
			if (!best_fit_node || (best_fit_node->total_q >=
					node->total_q)) {
				best_fit_node = node;
				qbase = best_fit_node->qbase;
			}
		}
	}

fragment_free_list:
	if (!best_fit_node)
		return NULL;

	if ((qbase == best_fit_node->qbase) &&
			(qmax == best_fit_node->total_q))
		return best_fit_node;

	/* split free resource node accordingly */
	if ((qbase == best_fit_node->qbase) &&
			(qmax != best_fit_node->total_q)) {
		/*
		 * create an extra node to hold the extra queues from this node
		 */
		struct qdma_resource_entry *new_entry = NULL;
		int lqbase = best_fit_node->qbase + qmax;
		uint32_t lqmax = best_fit_node->total_q - qmax;

		new_entry = qdma_free_entry_create(lqbase, lqmax);
		if (new_entry == NULL)
			return NULL;
		QDMA_LIST_SET_DATA(&new_entry->node, new_entry);
		qdma_list_insert_after(&new_entry->node,
				       &best_fit_node->node);
		best_fit_node->total_q -= lqmax;
	} else if ((qbase > best_fit_node->qbase) &&
			((qbase + qmax) == (best_fit_node->qbase +
					best_fit_node->total_q))) {
		/*
		 * create an extra node to hold the extra queues from this node
		 */
		struct qdma_resource_entry *new_entry = NULL;
		int lqbase = best_fit_node->qbase;
		uint32_t lqmax = qbase - best_fit_node->qbase;

		new_entry = qdma_free_entry_create(lqbase, lqmax);
		if (new_entry == NULL)
			return NULL;
		QDMA_LIST_SET_DATA(&new_entry->node, new_entry);
		qdma_list_insert_before(&new_entry->node,
					&best_fit_node->node);
		best_fit_node->total_q = qmax;
		best_fit_node->qbase = qbase;
	} else {
		/*
		 * create two extra node to hold the extra queues from this node
		 */
		struct qdma_resource_entry *new_entry = NULL;
		int lqbase = best_fit_node->qbase;
		uint32_t lqmax = qbase - best_fit_node->qbase;

		new_entry = qdma_free_entry_create(lqbase, lqmax);
		if (new_entry == NULL)
			return NULL;
		QDMA_LIST_SET_DATA(&new_entry->node, new_entry);
		qdma_list_insert_before(&new_entry->node,
					&best_fit_node->node);

		best_fit_node->qbase = qbase;
		best_fit_node->total_q -= lqmax;

		lqbase = best_fit_node->qbase + qmax;
		lqmax = best_fit_node->total_q - qmax;

		new_entry = qdma_free_entry_create(lqbase, lqmax);
		if (new_entry == NULL)
			return NULL;
		QDMA_LIST_SET_DATA(&new_entry->node, new_entry);
		qdma_list_insert_after(&new_entry->node,
				       &best_fit_node->node);
		best_fit_node->total_q = qmax;
	}

	return best_fit_node;
}

static int qdma_request_q_resource(struct qdma_dev_entry *dev_entry,
				    uint32_t new_qmax, int new_qbase,
				    struct qdma_list_head *free_list_head)
{
	uint32_t qmax = dev_entry->entry.total_q;
	int qbase = dev_entry->entry.qbase;
	struct qdma_resource_entry *free_entry_node = NULL;
	int rv = QDMA_SUCCESS;

	/* submit already allocated queues back to free list before requesting
	 * new resource
	 */
	qdma_submit_to_free_list(dev_entry, free_list_head);

	if (!new_qmax)
		return 0;
	/* check if the request can be accomodated */
	free_entry_node = qdma_get_resource_node(new_qmax, new_qbase,
						 free_list_head);
	if (free_entry_node == NULL) {
		/* request cannot be accommodated. Restore the dev_entry */
		free_entry_node = qdma_get_resource_node(qmax, qbase,
							 free_list_head);
		rv = -QDMA_ERR_RM_NO_QUEUES_LEFT;
		qdma_log_error("%s: Not enough queues, err:%d\n", __func__,
					   -QDMA_ERR_RM_NO_QUEUES_LEFT);
		if (free_entry_node == NULL) {
			dev_entry->entry.qbase = -1;
			dev_entry->entry.total_q = 0;

			return rv;
		}
	}

	dev_entry->entry.qbase = free_entry_node->qbase;
	dev_entry->entry.total_q = free_entry_node->total_q;

	qdma_list_del(&free_entry_node->node);
	qdma_memfree(free_entry_node);

	return rv;
}

int qdma_master_resource_create(uint32_t bus_start, uint32_t bus_end,
		int q_base, uint32_t total_q, uint32_t *dma_device_index)
{
	struct qdma_resource_master *q_resource;
	struct qdma_resource_entry *free_entry;
	static int index;

	q_resource = qdma_find_master_resource_entry(bus_start, bus_end);
	if (q_resource) {
		*dma_device_index = q_resource->dma_device_index;
		qdma_log_debug("%s: Resource already created", __func__);
		qdma_log_debug("for this device(%d)\n",
				q_resource->dma_device_index);
		return -QDMA_ERR_RM_RES_EXISTS;
	}

	*dma_device_index = index;

	q_resource = (struct qdma_resource_master *)qdma_calloc(1,
		sizeof(struct qdma_resource_master));
	if (!q_resource) {
		qdma_log_error("%s: no memory for q_resource, err:%d\n",
					__func__,
					-QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	free_entry = (struct qdma_resource_entry *)
		qdma_calloc(1, sizeof(struct qdma_resource_entry));
	if (!free_entry) {
		qdma_memfree(q_resource);
		qdma_log_error("%s: no memory for free_entry, err:%d\n",
					__func__,
					-QDMA_ERR_NO_MEM);
		return -QDMA_ERR_NO_MEM;
	}

	qdma_resource_lock_take();
	q_resource->dma_device_index = index;
	q_resource->pci_bus_start = bus_start;
	q_resource->pci_bus_end = bus_end;
	q_resource->total_q = total_q;
	q_resource->qbase = q_base;
	qdma_list_init_head(&q_resource->dev_list);
	qdma_list_init_head(&q_resource->free_list);
	QDMA_LIST_SET_DATA(&q_resource->node, q_resource);
	QDMA_LIST_SET_DATA(&q_resource->free_list, q_resource);
	qdma_list_add_tail(&q_resource->node, &master_resource_list);


	free_entry->total_q = total_q;
	free_entry->qbase = q_base;
	QDMA_LIST_SET_DATA(&free_entry->node, free_entry);
	qdma_list_add_tail(&free_entry->node, &q_resource->free_list);
	qdma_resource_lock_give();

	qdma_log_debug("%s: New master resource created at %d",
		__func__, index);
	++index;

	return QDMA_SUCCESS;
}

void qdma_master_resource_destroy(uint32_t dma_device_index)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_list_head *entry, *tmp;

	if (!q_resource)
		return;
	qdma_resource_lock_take();
	if (!qdma_list_is_empty(&q_resource->dev_list)) {
		qdma_resource_lock_give();
		return;
	}
	qdma_list_for_each_safe(entry, tmp, &q_resource->free_list) {
		struct qdma_resource_entry *free_entry =
			(struct qdma_resource_entry *)
				QDMA_LIST_GET_DATA(entry);

		qdma_list_del(&free_entry->node);
		qdma_memfree(free_entry);
	}
	qdma_list_del(&q_resource->node);
	qdma_memfree(q_resource);
	qdma_resource_lock_give();
}


int qdma_dev_entry_create(uint32_t dma_device_index, uint16_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
					__func__,
					-QDMA_ERR_RM_RES_NOT_EXISTS);
		return -QDMA_ERR_RM_RES_NOT_EXISTS;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);
	if (!dev_entry) {
		qdma_resource_lock_take();
		dev_entry = (struct qdma_dev_entry *)
			qdma_calloc(1, sizeof(struct qdma_dev_entry));
		if (dev_entry == NULL) {
			qdma_resource_lock_give();
			qdma_log_error("%s: Insufficient memory, err:%d\n",
						__func__,
						-QDMA_ERR_NO_MEM);
			return -QDMA_ERR_NO_MEM;
		}
		dev_entry->func_id = func_id;
		dev_entry->entry.qbase = -1;
		dev_entry->entry.total_q = 0;
		QDMA_LIST_SET_DATA(&dev_entry->entry.node, dev_entry);
		qdma_list_add_tail(&dev_entry->entry.node,
				   &q_resource->dev_list);
		qdma_resource_lock_give();
		qdma_log_info("%s: Created the dev entry successfully\n",
						__func__);
	} else {
		qdma_log_error("%s: Dev entry already created, err = %d\n",
						__func__,
						-QDMA_ERR_RM_DEV_EXISTS);
		return -QDMA_ERR_RM_DEV_EXISTS;
	}

	return QDMA_SUCCESS;
}

void qdma_dev_entry_destroy(uint32_t dma_device_index, uint16_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found.\n", __func__);
		return;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);
	if (!dev_entry) {
		qdma_log_error("%s: Dev entry not found\n", __func__);
		return;
	}
	qdma_resource_lock_take();
	qdma_submit_to_free_list(dev_entry, &q_resource->free_list);

	qdma_list_del(&dev_entry->entry.node);
	qdma_memfree(dev_entry);
	qdma_resource_lock_give();
}

int qdma_dev_update(uint32_t dma_device_index, uint16_t func_id,
		    uint32_t qmax, int *qbase)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;
	int rv;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
				__func__, -QDMA_ERR_RM_RES_NOT_EXISTS);
		return -QDMA_ERR_RM_RES_NOT_EXISTS;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry) {
		qdma_log_error("%s: Dev Entry not found, err: %d\n",
					__func__,
					-QDMA_ERR_RM_DEV_NOT_EXISTS);
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}

	qdma_resource_lock_take();

	/* if any active queue on device, no more new qmax
	 * configuration allowed
	 */
	if (dev_entry->active_h2c_qcnt ||
			dev_entry->active_c2h_qcnt ||
			dev_entry->active_cmpt_qcnt) {
		qdma_resource_lock_give();
		qdma_log_error("%s: Qs active. Config blocked, err: %d\n",
				__func__, -QDMA_ERR_RM_QMAX_CONF_REJECTED);
		return -QDMA_ERR_RM_QMAX_CONF_REJECTED;
	}

	rv = qdma_request_q_resource(dev_entry, qmax, *qbase,
				&q_resource->free_list);

	*qbase = dev_entry->entry.qbase;
	qdma_resource_lock_give();


	return rv;
}

int qdma_dev_qinfo_get(uint32_t dma_device_index, uint16_t func_id,
		       int *qbase, uint32_t *qmax)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
				__func__, -QDMA_ERR_RM_RES_NOT_EXISTS);
		return -QDMA_ERR_RM_RES_NOT_EXISTS;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry) {
		qdma_log_debug("%s: Dev Entry not created yet\n", __func__);
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}

	qdma_resource_lock_take();
	*qbase = dev_entry->entry.qbase;
	*qmax = dev_entry->entry.total_q;
	qdma_resource_lock_give();

	return QDMA_SUCCESS;
}

enum qdma_dev_q_range qdma_dev_is_queue_in_range(uint32_t dma_device_index,
						 uint16_t func_id,
						 uint32_t qid_hw)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;
	uint32_t qmax;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
				__func__, -QDMA_ERR_RM_RES_NOT_EXISTS);
		return QDMA_DEV_Q_OUT_OF_RANGE;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry) {
		qdma_log_error("%s: Dev entry not found, err: %d\n",
				__func__, -QDMA_ERR_RM_DEV_NOT_EXISTS);
		return QDMA_DEV_Q_OUT_OF_RANGE;
	}

	qdma_resource_lock_take();
	qmax = dev_entry->entry.qbase + dev_entry->entry.total_q;
	if (dev_entry->entry.total_q && (qid_hw < qmax) &&
			((int)qid_hw >= dev_entry->entry.qbase)) {
		qdma_resource_lock_give();
		return QDMA_DEV_Q_IN_RANGE;
	}
	qdma_resource_lock_give();

	return QDMA_DEV_Q_OUT_OF_RANGE;
}

int qdma_dev_increment_active_queue(uint32_t dma_device_index, uint16_t func_id,
				    enum qdma_dev_q_type q_type)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;
	int rv = QDMA_SUCCESS;
	uint32_t *active_qcnt = NULL;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
				__func__, -QDMA_ERR_RM_RES_NOT_EXISTS);
		return -QDMA_ERR_RM_RES_NOT_EXISTS;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry) {
		qdma_log_error("%s: Dev Entry not found, err: %d\n",
					__func__,
					-QDMA_ERR_RM_DEV_NOT_EXISTS);
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}

	qdma_resource_lock_take();
	switch (q_type) {
	case QDMA_DEV_Q_TYPE_H2C:
		active_qcnt = &dev_entry->active_h2c_qcnt;
		break;
	case QDMA_DEV_Q_TYPE_C2H:
		active_qcnt = &dev_entry->active_c2h_qcnt;
		break;
	case QDMA_DEV_Q_TYPE_CMPT:
		active_qcnt = &dev_entry->active_cmpt_qcnt;
		break;
	default:
		rv = -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}

	if (active_qcnt && (dev_entry->entry.total_q < ((*active_qcnt) + 1))) {
		qdma_resource_lock_give();
		return -QDMA_ERR_RM_NO_QUEUES_LEFT;
	}

	if (active_qcnt) {
		*active_qcnt = (*active_qcnt) + 1;
		q_resource->active_qcnt++;
	}
	qdma_resource_lock_give();

	return rv;
}


int qdma_dev_decrement_active_queue(uint32_t dma_device_index, uint16_t func_id,
				    enum qdma_dev_q_type q_type)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;
	int rv = QDMA_SUCCESS;

	if (!q_resource) {
		qdma_log_error("%s: Queue resource not found, err: %d\n",
				__func__,
			   -QDMA_ERR_RM_RES_NOT_EXISTS);
		return -QDMA_ERR_RM_RES_NOT_EXISTS;
	}

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry) {
		qdma_log_error("%s: Dev entry not found, err: %d\n",
				__func__, -QDMA_ERR_RM_DEV_NOT_EXISTS);
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}

	qdma_resource_lock_take();
	switch (q_type) {
	case QDMA_DEV_Q_TYPE_H2C:
		if (dev_entry->active_h2c_qcnt)
			dev_entry->active_h2c_qcnt--;
		break;
	case QDMA_DEV_Q_TYPE_C2H:
		if (dev_entry->active_c2h_qcnt)
			dev_entry->active_c2h_qcnt--;
		break;
	case QDMA_DEV_Q_TYPE_CMPT:
		if (dev_entry->active_cmpt_qcnt)
			dev_entry->active_cmpt_qcnt--;
		break;
	default:
		rv = -QDMA_ERR_RM_DEV_NOT_EXISTS;
	}
	q_resource->active_qcnt--;
	qdma_resource_lock_give();

	return rv;
}

uint32_t qdma_get_active_queue_count(uint32_t dma_device_index)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	uint32_t q_cnt;

	if (!q_resource)
		return QDMA_SUCCESS;

	qdma_resource_lock_take();
	q_cnt = q_resource->active_qcnt;
	qdma_resource_lock_give();

	return q_cnt;
}

int qdma_get_device_active_queue_count(uint32_t dma_device_index,
					uint16_t func_id,
					enum qdma_dev_q_type q_type)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_resource_entry(dma_device_index);
	struct qdma_dev_entry *dev_entry;
	uint32_t dev_active_qcnt = 0;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(dma_device_index, func_id);

	if (!dev_entry)
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;

	qdma_resource_lock_take();
	switch (q_type) {
	case QDMA_DEV_Q_TYPE_H2C:
		dev_active_qcnt = dev_entry->active_h2c_qcnt;
		break;
	case QDMA_DEV_Q_TYPE_C2H:
		dev_active_qcnt = dev_entry->active_c2h_qcnt;
		break;
	case QDMA_DEV_Q_TYPE_CMPT:
		dev_active_qcnt = dev_entry->active_cmpt_qcnt;
		break;
	default:
		dev_active_qcnt = 0;
	}
	qdma_resource_lock_give();

	return dev_active_qcnt;
}
