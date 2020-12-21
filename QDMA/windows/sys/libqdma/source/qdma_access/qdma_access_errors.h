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
