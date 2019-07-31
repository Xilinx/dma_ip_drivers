/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
 */

#include "qdma_access_errors.h"
#include "qdma_resource_mgmt.h"
#include "qdma_platform.h"
#include "qdma_list.h"

struct qdma_resource_entry {
	int qbase;
	uint32_t total_q;
	struct qdma_list_head node;
};

struct qdma_dev_entry {
	uint16_t  func_id;
	uint32_t active_qcnt;
	struct qdma_resource_entry entry;
};

/** for hodling the qconf_entry structure */
struct qdma_resource_master {
	/** pci bus number this reousrce belongs to */
	uint32_t pci_bus_num;
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
	/** active queue count */
	uint32_t active_qcnt;
};

static QDMA_LIST_HEAD(master_resource_list);

static struct qdma_resource_master *qdma_get_master_reousrce_entry(
							uint32_t pci_bus_num)
{
	struct qdma_list_head *entry, *tmp;

	qdma_resource_lock_take();
	qdma_list_for_each_safe(entry, tmp, &master_resource_list) {
		struct qdma_resource_master *q_resource =
				QDMA_LIST_GET_DATA(entry);

		if (q_resource->pci_bus_num == pci_bus_num) {
			qdma_resource_lock_give();
			return q_resource;
		}
	}
	qdma_resource_lock_give();

	return NULL;
}

static struct qdma_dev_entry *qdma_get_dev_entry(uint32_t pci_bus_num,
						uint16_t func_id)
{
	struct qdma_list_head *entry, *tmp;
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);

	if (!q_resource)
		return NULL;

	qdma_resource_lock_take();
	qdma_list_for_each_safe(entry, tmp, &q_resource->dev_list) {
		struct qdma_dev_entry *dev_entry = QDMA_LIST_GET_DATA(entry);

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
	struct qdma_resource_entry *entry =
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
		struct qdma_resource_entry *node = QDMA_LIST_GET_DATA(entry);

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
					QDMA_LIST_GET_DATA(entry);

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
		struct qdma_resource_entry *node = QDMA_LIST_GET_DATA(entry);

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
		return QDMA_SUCCESS;
	/* check if the request can be accomodated */
	free_entry_node = qdma_get_resource_node(new_qmax, new_qbase,
						 free_list_head);
	if (free_entry_node == NULL) {
		/* request cannot be accommodated. Restore the dev_entry */
		free_entry_node = qdma_get_resource_node(qmax, qbase,
							 free_list_head);
		rv = -QDMA_ERR_RM_NO_QUEUES_LEFT;
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

int qdma_master_resource_create(uint32_t pci_bus_num, int qbase,
				 uint32_t total_q)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_resource_entry *free_entry;

	if (!q_resource)
		q_resource = qdma_calloc(1,
					 sizeof(struct qdma_resource_master));
	else
		return -QDMA_ERR_RM_RES_EXISTS;

	if (!q_resource)
		return -QDMA_ERR_NO_MEM;

	qdma_resource_lock_take();
	free_entry = qdma_calloc(1, sizeof(struct qdma_resource_entry));
	if (!free_entry) {
		qdma_memfree(q_resource);
		return -QDMA_ERR_NO_MEM;
	}

	q_resource->pci_bus_num = pci_bus_num;
	q_resource->total_q = total_q;
	q_resource->qbase = qbase;
	qdma_list_init_head(&q_resource->dev_list);
	qdma_list_init_head(&q_resource->free_list);
	QDMA_LIST_SET_DATA(&q_resource->node, q_resource);
	QDMA_LIST_SET_DATA(&q_resource->free_list, q_resource);
	qdma_list_add_tail(&q_resource->node, &master_resource_list);


	free_entry->total_q = total_q;
	free_entry->qbase = qbase;
	QDMA_LIST_SET_DATA(&free_entry->node, free_entry);
	qdma_list_add_tail(&free_entry->node, &q_resource->free_list);
	qdma_resource_lock_give();

	return QDMA_SUCCESS;
}

void qdma_master_resource_destroy(uint32_t pci_bus_num)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
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
				QDMA_LIST_GET_DATA(entry);

		qdma_list_del(&free_entry->node);
		qdma_memfree(free_entry);
	}
	qdma_list_del(&q_resource->node);
	qdma_memfree(q_resource);
	qdma_resource_lock_give();
}


int qdma_dev_entry_create(uint32_t pci_bus_num, uint16_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);
	if (!dev_entry) {
		qdma_resource_lock_take();
		dev_entry = qdma_calloc(1, sizeof(struct qdma_dev_entry));
		if (dev_entry == NULL) {
			qdma_resource_lock_give();
			return -QDMA_ERR_NO_MEM;
		}
		dev_entry->func_id = func_id;
		dev_entry->entry.qbase = -1;
		dev_entry->entry.total_q = 0;
		QDMA_LIST_SET_DATA(&dev_entry->entry.node, dev_entry);
		qdma_list_add_tail(&dev_entry->entry.node,
				   &q_resource->dev_list);
		qdma_resource_lock_give();
	} else
		return -QDMA_ERR_RM_DEV_EXISTS;

	return QDMA_SUCCESS;
}

void qdma_dev_entry_destroy(uint32_t pci_bus_num, uint32_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource)
		return;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);
	if (!dev_entry)
		return;
	qdma_resource_lock_take();
	qdma_submit_to_free_list(dev_entry, &q_resource->free_list);

	qdma_list_del(&dev_entry->entry.node);
	qdma_memfree(dev_entry);
	qdma_resource_lock_give();
}

int qdma_dev_update(uint32_t pci_bus_num, uint32_t func_id,
		    uint32_t qmax, int *qbase)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;
	int rv;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;

	qdma_resource_lock_take();

	/* if any active queue on device, no more new qmax
	 * configuration allowed
	 */
	if (dev_entry->active_qcnt) {
		qdma_resource_lock_give();
		return -QDMA_ERR_RM_QMAX_CONF_REJECTED;
	}

	rv = qdma_request_q_resource(dev_entry, qmax, *qbase,
				&q_resource->free_list);

	*qbase = dev_entry->entry.qbase;
	qdma_resource_lock_give();


	return rv;
}

int qdma_dev_qinfo_get(uint32_t pci_bus_num, uint32_t func_id,
		       int *qbase, uint32_t *qmax)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;

	qdma_resource_lock_take();
	*qbase = dev_entry->entry.qbase;
	*qmax = dev_entry->entry.total_q;
	qdma_resource_lock_give();

	return QDMA_SUCCESS;
}

enum qdma_dev_q_range qdma_dev_is_queue_in_range(uint32_t pci_bus_num,
						 uint32_t func_id,
						 uint32_t qid_hw)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;
	uint32_t qmax;

	if (!q_resource)
		return QDMA_DEV_Q_OUT_OF_RANGE;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return QDMA_DEV_Q_OUT_OF_RANGE;

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

int qdma_dev_increment_active_queue(uint32_t pci_bus_num, uint32_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;

	qdma_resource_lock_take();
	dev_entry->active_qcnt++;
	q_resource->active_qcnt++;
	qdma_resource_lock_give();

	return QDMA_SUCCESS;
}


int qdma_dev_decrement_active_queue(uint32_t pci_bus_num, uint32_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;

	if (!q_resource)
		return -QDMA_ERR_RM_RES_NOT_EXISTS;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return -QDMA_ERR_RM_DEV_NOT_EXISTS;

	qdma_resource_lock_take();
	if (dev_entry->active_qcnt) {
		dev_entry->active_qcnt--;
		q_resource->active_qcnt--;
	}
	qdma_resource_lock_give();

	return QDMA_SUCCESS;
}

uint32_t qdma_get_active_queue_count(uint32_t pci_bus_num)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	uint32_t q_cnt;

	if (!q_resource)
		return 0;

	qdma_resource_lock_take();
	q_cnt = q_resource->active_qcnt;
	qdma_resource_lock_give();

	return q_cnt;
}

uint32_t qdma_get_device_active_queue_count(uint32_t pci_bus_num,
					uint32_t func_id)
{
	struct qdma_resource_master *q_resource =
			qdma_get_master_reousrce_entry(pci_bus_num);
	struct qdma_dev_entry *dev_entry;
	uint32_t dev_active_qcnt = 0;

	if (!q_resource)
		return 0;

	dev_entry = qdma_get_dev_entry(pci_bus_num, func_id);

	if (!dev_entry)
		return 0;

	qdma_resource_lock_take();
	dev_active_qcnt = dev_entry->active_qcnt;
	qdma_resource_lock_give();

	return dev_active_qcnt;
}
