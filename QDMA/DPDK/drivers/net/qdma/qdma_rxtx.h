/*-
 * BSD LICENSE
 *
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
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
#ifndef QDMA_DPDK_RXTX_H_
#define QDMA_DPDK_RXTX_H_

#include "qdma_access_export.h"

/*Supporting functions for user logic pluggability*/
uint16_t qdma_get_rx_queue_id(void *queue_hndl);
void qdma_get_device_info(void *queue_hndl,
		enum qdma_device_type *device_type,
		enum qdma_ip_type *ip_type);
struct qdma_ul_st_h2c_desc *get_st_h2c_desc(void *queue_hndl);
struct qdma_ul_mm_desc *get_mm_h2c_desc(void *queue_hndl);
uint32_t get_mm_c2h_ep_addr(void *queue_hndl);
uint32_t get_mm_h2c_ep_addr(void *queue_hndl);
uint32_t get_mm_buff_size(void *queue_hndl);

#endif /* QDMA_DPDK_RXTX_H_ */
