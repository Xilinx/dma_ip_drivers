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

#ifndef __QDMA_ACCESS_ERRORS_H_
#define __QDMA_ACCESS_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: QDMA common library error codes definitions
 *
 * Header file *qdma_access_errors.h* defines error codes for common library
 */

struct err_code_map {
	int acc_err_code;
	int err_code;
};

#define QDMA_HW_ERR_NOT_DETECTED		1

enum qdma_access_error_codes {
	QDMA_SUCCESS = 0,
	QDMA_ERR_INV_PARAM,
	QDMA_ERR_NO_MEM,
	QDMA_ERR_HWACC_BUSY_TIMEOUT,
	QDMA_ERR_HWACC_INV_CONFIG_BAR,
	QDMA_ERR_HWACC_NO_PEND_LEGCY_INTR,
	QDMA_ERR_HWACC_BAR_NOT_FOUND,
	QDMA_ERR_HWACC_FEATURE_NOT_SUPPORTED,   /* 7 */

	QDMA_ERR_RM_RES_EXISTS,				/* 8 */
	QDMA_ERR_RM_RES_NOT_EXISTS,
	QDMA_ERR_RM_DEV_EXISTS,
	QDMA_ERR_RM_DEV_NOT_EXISTS,
	QDMA_ERR_RM_NO_QUEUES_LEFT,
	QDMA_ERR_RM_QMAX_CONF_REJECTED,		/* 13 */

	QDMA_ERR_MBOX_FMAP_WR_FAILED,		/* 14 */
	QDMA_ERR_MBOX_NUM_QUEUES,
	QDMA_ERR_MBOX_INV_QID,
	QDMA_ERR_MBOX_INV_RINGSZ,
	QDMA_ERR_MBOX_INV_BUFSZ,
	QDMA_ERR_MBOX_INV_CNTR_TH,
	QDMA_ERR_MBOX_INV_TMR_TH,
	QDMA_ERR_MBOX_INV_MSG,
	QDMA_ERR_MBOX_SEND_BUSY,
	QDMA_ERR_MBOX_NO_MSG_IN,
	QDMA_ERR_MBOX_REG_READ_FAILED,
	QDMA_ERR_MBOX_ALL_ZERO_MSG,			/* 25 */
};

#ifdef __cplusplus
}
#endif

#endif /* __QDMA_ACCESS_ERRORS_H_ */
