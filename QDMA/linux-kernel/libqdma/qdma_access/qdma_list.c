/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
 */

#include "qdma_list.h"

void qdma_list_init_head(struct qdma_list_head *head)
{
	if (head)
		head->prev = head->next = head;
}

void qdma_list_add_tail(struct qdma_list_head *node,
			  struct qdma_list_head *head)
{
	head->prev->next = node;
	node->next = head;
	node->prev = head->prev;
	head->prev = node;
}

void qdma_list_insert_before(struct qdma_list_head *new_node,
				    struct qdma_list_head *node)
{
	node->prev->next = new_node;
	new_node->prev = node->prev;
	new_node->next = node;
	node->prev = new_node;
}

void qdma_list_insert_after(struct qdma_list_head *new_node,
				   struct qdma_list_head *node)
{
	new_node->prev = node;
	new_node->next = node->next;
	node->next->prev = new_node;
	node->next = new_node;
}


void qdma_list_del(struct qdma_list_head *node)
{
	if (node) {
		if (node->prev)
			node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
	}
}
