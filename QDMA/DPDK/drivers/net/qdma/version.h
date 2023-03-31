/*-
 * BSD LICENSE
 *
 * Copyright (c) 2017-2022 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __QDMA_VERSION_H__
#define __QDMA_VERSION_H__

#define qdma_stringify1(x...)	#x
#define qdma_stringify(x...)	qdma_stringify1(x)

#define QDMA_PMD_MAJOR		2023
#define QDMA_PMD_MINOR		1
#define QDMA_PMD_PATCHLEVEL	1

#define QDMA_PMD_VERSION      \
	qdma_stringify(QDMA_PMD_MAJOR) "." \
	qdma_stringify(QDMA_PMD_MINOR) "." \
	qdma_stringify(QDMA_PMD_PATCHLEVEL)

#define QDMA_PMD_VERSION_NUMBER  \
	((QDMA_PMD_MAJOR) * 1000 + (QDMA_PMD_MINOR) * 100 + QDMA_PMD_PATCHLEVEL)

#endif /* ifndef __QDMA_VERSION_H__ */
