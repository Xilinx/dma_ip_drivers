/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
 *
 * BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QDMA_LIST_H_
#define __QDMA_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA common library provided list implementation definitions
 *
 * Header file *qdma_list.h* defines APIs for creating and managing list.
 */

/**
 * struct qdma_list_head - data type for creating a list node
 */
struct qdma_list_head {
	struct qdma_list_head *prev;
	struct qdma_list_head *next;
	void *priv;
};

#define QDMA_LIST_HEAD_INIT(name) { &(name), &(name), NULL }

#define QDMA_LIST_HEAD(name) \
	struct qdma_list_head name = QDMA_LIST_HEAD_INIT(name)

#define QDMA_LIST_GET_DATA(node) (node->priv)
#define QDMA_LIST_SET_DATA(node, data) ((node)->priv = data)


#define qdma_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


#define qdma_list_is_last_entry(entry, head) ((entry)->next == (head))

#define qdma_list_is_empty(head) ((head)->next == (head))

/*****************************************************************************/
/**
 * qdma_list_init_head(): Init the list head
 *
 * @head:     head of the list
 *
 * Return:	None
 *****************************************************************************/
void qdma_list_init_head(struct qdma_list_head *head);

/*****************************************************************************/
/**
 * qdma_list_add_tail(): add the given @node at the end of the list with @head
 *
 * @node:     new entry which has to be added at the end of the list with @head
 * @head:     head of the list
 *
 * This API needs to be called with holding the lock to the list
 *
 * Return:	None
 *****************************************************************************/
void qdma_list_add_tail(struct qdma_list_head *node,
			  struct qdma_list_head *head);

/*****************************************************************************/
/**
 * qdma_list_insert_before(): add the given @node at the before a @node
 *
 * @new_node:     new entry which has to be added before @node
 * @node:         reference node in the list
 *
 * This API needs to be called with holding the lock to the list
 *
 * Return:	None
 *****************************************************************************/
void qdma_list_insert_before(struct qdma_list_head *new_node,
				    struct qdma_list_head *node);

/*****************************************************************************/
/**
 * qdma_list_insert_after(): add the given @node at the after a @node
 *
 * @new_node:     new entry which has to be added after @node
 * @node:         reference node in the list
 *
 * This API needs to be called with holding the lock to the list
 *
 * Return:	None
 *****************************************************************************/
void qdma_list_insert_after(struct qdma_list_head *new_node,
				   struct qdma_list_head *node);

/*****************************************************************************/
/**
 * qdma_list_del(): delete an node from the list
 *
 * @node:     node in a list
 *
 * This API needs to be called with holding the lock to the list
 *
 * Return:	None
 *****************************************************************************/
void qdma_list_del(struct qdma_list_head *node);

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_LIST_H_ */
