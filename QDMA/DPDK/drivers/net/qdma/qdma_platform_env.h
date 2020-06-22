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

#ifndef QDMA_PLATFORM_ENV_H_
#define QDMA_PLATFORM_ENV_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rte_log.h>

#define QDMA_SNPRINTF_S(arg1, arg2, arg3, ...) \
		snprintf(arg1, arg3, ##__VA_ARGS__)

#ifdef RTE_LIBRTE_QDMA_DEBUG_DRIVER
#define qdma_log_info(x_, ...) rte_log(RTE_LOG_INFO,\
		RTE_LOGTYPE_USER1, x_, ##__VA_ARGS__)
#define qdma_log_warning(x_, ...) rte_log(RTE_LOG_WARNING,\
		RTE_LOGTYPE_USER1, x_, ##__VA_ARGS__)
#define qdma_log_debug(x_, ...) rte_log(RTE_LOG_DEBUG,\
		RTE_LOGTYPE_USER1, x_, ##__VA_ARGS__)
#define qdma_log_error(x_, ...) rte_log(RTE_LOG_ERR,\
		RTE_LOGTYPE_USER1, x_, ##__VA_ARGS__)
#else
#define qdma_log_info(x_, ...) do { } while (0)
#define qdma_log_warning(x_, ...) do { } while (0)
#define qdma_log_debug(x_, ...) do { } while (0)
#define qdma_log_error(x_, ...) do { } while (0)
#endif

#endif /* QDMA_PLATFORM_ENV_H_ */
