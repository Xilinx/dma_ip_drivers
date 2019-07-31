/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2018-2019,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#ifndef __NL_USER_H__
#define __NL_USER_H__

#include <stdint.h>
#ifdef ERR_DEBUG
#include "qdma_nl.h"
#endif

#define DESC_SIZE_64B		3
#define MAX_KMALLOC_SIZE	(4*1024*1024)

struct xcmd_info;

enum q_parm_type {
	QPARM_IDX,
	QPARM_MODE,
	QPARM_DIR,
	QPARM_DESC,
	QPARM_CMPT,
	QPARM_CMPTSZ,
	QPARM_SW_DESC_SZ,
	QPARM_RNGSZ_IDX,
	QPARM_C2H_BUFSZ_IDX,
	QPARM_CMPT_TMR_IDX,
	QPARM_CMPT_CNTR_IDX,
	QPARM_CMPT_TRIG_MODE,
	QPARM_PIPE_GL_MAX,
	QPARM_PIPE_FLOW_ID,
	QPARM_PIPE_SLR_ID,
	QPARM_PIPE_TDEST,
#ifdef ERR_DEBUG
	QPARAM_ERR_NO,
#endif

	QPARM_MAX,
};

struct xcmd_q_parm {
	unsigned int sflags;
	uint32_t flags;
	uint32_t idx;
	uint32_t num_q;
	uint32_t range_start;
	uint32_t range_end;
	unsigned char sw_desc_sz;
	unsigned char cmpt_entry_size;
	unsigned char qrngsz_idx;
	unsigned char c2h_bufsz_idx;
	unsigned char cmpt_tmr_idx;
	unsigned char cmpt_cntr_idx;
	unsigned char cmpt_trig_mode;
	unsigned char is_qp;
	uint8_t pipe_gl_max;
	uint8_t pipe_flow_id;
	uint8_t pipe_slr_id;
	uint16_t pipe_tdest;
#ifdef ERR_DEBUG
	union {
		unsigned int err_info;
		struct {
			unsigned int err_no:31;
			unsigned int en:1;
		} err;
	};
#endif
};

struct xcmd_intr {
	unsigned int vector;
	int start_idx;
	int end_idx;
};
#define VERSION_INFO_STR_LENGTH            256
struct xcmd_dev_cap {
	char version_str[VERSION_INFO_STR_LENGTH];
	unsigned int num_qs;
	unsigned int num_pfs;
	unsigned int flr_present;
	unsigned int mm_en;
	unsigned int mm_cmpt_en;
	unsigned int st_en;
	unsigned int mailbox_en;
	unsigned int mm_channel_max;
};

struct xnl_cb {
	int fd;
	unsigned short family;
	unsigned int snd_seq;
	unsigned int rcv_seq;
};

int xnl_connect(struct xnl_cb *cb, int vf);
void xnl_close(struct xnl_cb *cb);
int xnl_send_cmd(struct xnl_cb *cb, struct xcmd_info *xcmd);

#endif /* ifndef __NL_USER_H__ */
