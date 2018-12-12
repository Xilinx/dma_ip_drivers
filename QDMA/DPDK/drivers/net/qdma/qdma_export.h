/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2018 Xilinx, Inc. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QDMA_EXPORT_H__
#define __QDMA_EXPORT_H__

#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>

enum rte_xdebug_type {
	ETH_XDEBUG_QDMA_GLOBAL_CSR,
	ETH_XDEBUG_QUEUE_CONTEXT,
	ETH_XDEBUG_QUEUE_STRUCT,
	ETH_XDEBUG_QUEUE_DESC_DUMP,
	ETH_XDEBUG_MAX,
};

enum rte_xdebug_desc_type {
	ETH_XDEBUG_DESC_C2H,
	ETH_XDEBUG_DESC_H2C,
	ETH_XDEBUG_DESC_CMPT,
	ETH_XDEBUG_DESC_MAX,
};
struct rte_xdebug_desc_param {
	uint16_t queue;
	int start;
	int end;
	enum rte_xdebug_desc_type type;
};

int qdma_xdebug(uint8_t port_id, enum rte_xdebug_type type,
			void *params);

#endif /* ifndef __QDMA_EXPORT_H__ */
