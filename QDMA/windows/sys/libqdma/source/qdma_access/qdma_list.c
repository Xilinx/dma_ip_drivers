/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
