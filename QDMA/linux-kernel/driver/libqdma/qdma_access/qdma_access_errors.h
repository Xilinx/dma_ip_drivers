/*
 * Copyright(c) 2019-2020 Xilinx, Inc. All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
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
