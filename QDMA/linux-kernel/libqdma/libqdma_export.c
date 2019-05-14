/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
 * All rights reserved.
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

/**
 * @file
 * @brief This file contains the definitions for libqdma interfaces
 *
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "libqdma_export.h"

#include "qdma_descq.h"
#include "qdma_device.h"
#include "qdma_thread.h"
#include "qdma_regs.h"
#include "qdma_context.h"
#include "qdma_intr.h"
#include "qdma_st_c2h.h"
#include "thread.h"
#include "version.h"

#ifdef DEBUGFS
#include "qdma_debugfs_queue.h"

/** debugfs root */
struct dentry *qdma_debugfs_root;
#endif

#define QDMA_Q_PEND_LIST_COMPLETION_TIMEOUT 1000 /* msec */

struct drv_mode_name mode_name_list[] = {
	{ AUTO_MODE,			"auto"},
	{ POLL_MODE,			"poll"},
	{ DIRECT_INTR_MODE,		"direct interrupt"},
	{ INDIRECT_INTR_MODE,	"indirect interrupt"},
	{ LEGACY_INTR_MODE,		"legacy interrupt"}
};
/* ********************* static function definitions ************************ */
/*****************************************************************************/
/**
 * qdma_request_wait_for_cmpl() - static function to monitor the
 *								wait completion
 *
 * @param[in]	xdev:	pointer to xlnx_dma_dev structure
 * @param[in]	descq:	pointer to qdma_descq structure
 * @param[in]	req:	qdma request
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int qdma_request_wait_for_cmpl(struct xlnx_dma_dev *xdev,
			struct qdma_descq *descq, struct qdma_request *req)
{
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);

	/** if timeout is mentioned in the request,
	 *  wait until the timeout occurs or wait until the
	 *  call back is completed
	 */

	if (req->timeout_ms)
		qdma_waitq_wait_event_timeout(cb->wq, cb->done,
			msecs_to_jiffies(req->timeout_ms));
	else
		qdma_waitq_wait_event(cb->wq, cb->done);

	lock_descq(descq);
	/** if the call back is not done, request timed out
	 *  delete the request list
	 */
	if (!cb->done)
		list_del(&cb->list);

	/** if the call back is not done but the status is updated
	 *  return i/o error
	 */
	if (!cb->done || cb->status) {
		pr_info("%s: req 0x%p, %c,%u,%u/%u,0x%llx, done %d, err %d, tm %u.\n",
				descq->conf.name,
				req, req->write ? 'W' : 'R',
				cb->offset,
				cb->left,
				req->count,
				req->ep_addr,
				cb->done,
				cb->status,
			req->timeout_ms);
		qdma_descq_dump(descq, NULL, 0, 1);
		unlock_descq(descq);

		return -EIO;
	}

	unlock_descq(descq);
	return 0;
}

/*****************************************************************************/
/**
 * qdma_request_submit_st_c2h() - static function to handle the
 *							st c2h request
 *
 * @param[in]	xdev:	pointer to xlnx_dma_dev structure
 * @param[in]	descq:	pointer to qdma_descq structure
 * @param[in]	req:	qdma request
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static ssize_t qdma_request_submit_st_c2h(struct xlnx_dma_dev *xdev,
			struct qdma_descq *descq, struct qdma_request *req)
{
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);
	int wait = req->fp_done ? 0 : 1;
	int rv = 0;

	/** make sure that qdev is not NULL, else return error */
	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	pr_debug("%s: data len %u, sgl 0x%p, sgl cnt %u, tm %u ms.\n",
			descq->conf.name,
			req->count, req->sgl, req->sgcnt, req->timeout_ms);

	/** get the request count */
	cb->left = req->count;

	lock_descq(descq);
	if (descq->q_stop_wait) {
		unlock_descq(descq);
		return 0;
	}
	if ((descq->q_state == Q_STATE_ONLINE) &&
			!descq->q_stop_wait) {
		/* add to pend list even before cidx/pidx update as it could
		 *  cause an interrupt and may miss processing of writeback
		 */
		list_add_tail(&cb->list, &descq->pend_list);
		/* any rcv'ed packet not yet read ? */
		/** read the data from the device */
		descq_st_c2h_read(descq, req, 1, 1);
		if (!cb->left) {
			list_del(&cb->list);
			unlock_descq(descq);
			return req->count;
		}
		descq->pend_list_empty = 0;
		unlock_descq(descq);
	} else {
		unlock_descq(descq);
		pr_info("%s descq %s NOT online.\n",
			xdev->conf.name, descq->conf.name);
		return -EINVAL;
	}

	/** if there is a completion thread associated,
	 *  wake up the completion thread to process the
	 *  completion status
	 */
	if (descq->cmplthp)
		qdma_kthread_wakeup(descq->cmplthp);

	if (!wait) {
		pr_debug("%s: cb 0x%p, data len 0x%x NO wait.\n",
			descq->conf.name, cb, req->count);
		return 0;
	}

	/** wait for the request completion */
	rv = qdma_request_wait_for_cmpl(xdev, descq, req);
	if (rv < 0) {
		if (!req->dma_mapped)
			sgl_unmap(xdev->conf.pdev, req->sgl, req->sgcnt,
				DMA_FROM_DEVICE);
		return rv;
	}

	/** Once the request completion received,
	 *  return with the number of processed requests
	 */
	return req->count - cb->left;
}

/* ********************* public function definitions ************************ */

/*****************************************************************************/
/**
 * qdma_queue_get_config() - retrieve the configuration of a queue
 *
 * @param[in]	dev_hndl:	dev_hndl retured from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
struct qdma_queue_conf *qdma_queue_get_config(unsigned long dev_hndl,
				unsigned long id, char *buf, int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, buf, buflen, 0);

	/** make sure that descq is not NULL
	 *  return error is it is null else return the config data
	 */
	if (descq)
		return &descq->conf;

	return NULL;
}

/*****************************************************************************/
/**
 * qdma_queue_dump() - display a queue's state in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump(unsigned long dev_hndl, unsigned long id, char *buf,
				int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl, id,
					buf, buflen, 0);
	struct hw_descq_context ctxt;
	int len = 0;
	int rv;
#ifndef __QDMA_VF__
	int ring_index = 0;
	u32 intr_ctxt[4];
	int i = 0;
#endif

	if (!descq)
		return -EINVAL;

	/** Make sure that buf and buflen is not invalid */
	if (!buf || !buflen)
		return QDMA_ERR_INVALID_INPUT_PARAM;

	/** read the descq data to dump */
	qdma_descq_dump(descq, buf, buflen, 1);
	len = strlen(buf);

	/** read the descq context for the given qid */
	rv = qdma_descq_context_read(descq->xdev, descq->qidx_hw,
				descq->conf.st, descq->conf.c2h, &ctxt);
	if (rv < 0) {
		len += sprintf(buf + len, "%s read context failed %d.\n",
				descq->conf.name, rv);
		return rv;
	}

	/** format the output for all contexts */
	len += sprintf(buf + len,
	"\tSW CTXT:    [4]:0x%08x [3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
	ctxt.sw[4], ctxt.sw[3], ctxt.sw[2], ctxt.sw[1], ctxt.sw[0]);

	len += sprintf(buf + len,
			"\tHW CTXT:    [1]:0x%08x [0]:0x%08x\n",
			ctxt.hw[1], ctxt.hw[0]);

	len += sprintf(buf + len,
			"\tCR CTXT:    0x%08x\n", ctxt.cr[0]);

	/** incase of ST C2H,
	 *  add completion and prefetch context to the output
	 */
	if (descq->conf.c2h && descq->conf.st) {
		len += sprintf(buf + len,
	"\tCMPT CTXT:  [4]:0x%08x [3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
	ctxt.cmpt[4], ctxt.cmpt[3], ctxt.cmpt[2], ctxt.cmpt[1], ctxt.cmpt[0]);

		len += sprintf(buf + len,
			"\tPFTCH CTXT: [1]:0x%08x [0]:0x%08x\n",
			ctxt.prefetch[1], ctxt.prefetch[0]);
	}

#ifndef __QDMA_VF__
	/** if interrupt aggregation is enabled
	 *  add the interrupt context
	 */
	if ((descq->xdev->conf.qdma_drv_mode == INDIRECT_INTR_MODE) ||
			(descq->xdev->conf.qdma_drv_mode == AUTO_MODE)) {
		for (i = 0; i < QDMA_NUM_DATA_VEC_FOR_INTR_CXT; i++) {
			ring_index = get_intr_ring_index(
					descq->xdev,
					(i + descq->xdev->dvec_start_idx));
			rv = qdma_intr_context_read(descq->xdev,
						ring_index,
						4,
						intr_ctxt);
			if (rv < 0) {
				len += sprintf(buf + len,
					"%s read intr context failed %d.\n",
					descq->conf.name, rv);
				return rv;
			}

			len += sprintf(buf + len,
			"\tINTR_CTXT[%d]:	[3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
			ring_index,
			intr_ctxt[3],
			intr_ctxt[2],
			intr_ctxt[1],
			intr_ctxt[0]);
		}
	}

	if (descq->xdev->stm_en) {
		if (descq->conf.st &&
		    (descq->conf.c2h || descq->conf.desc_bypass)) {
			struct stm_descq_context stm_hw;

			memset(&stm_hw, 0, sizeof(stm_hw));
			/** read stm context */
			rv = qdma_descq_stm_read(descq->xdev, descq->qidx_hw,
						 descq->conf.pipe_flow_id,
						 descq->conf.c2h, false, true,
						 &stm_hw);
			if (rv < 0) {
				len += sprintf(buf + len,
						"%s read stm-context failed %d.\n",
					       descq->conf.name, rv);
				return rv;
			}

			len += sprintf(buf + len,
				       "\tSTM CTXT:    [5]:0x%08x [4]:0x%08x [3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
				       stm_hw.stm[5], stm_hw.stm[4],
				       stm_hw.stm[3], stm_hw.stm[2],
				       stm_hw.stm[1], stm_hw.stm[0]);

			/** read stm can */
			rv = qdma_descq_stm_read(descq->xdev, descq->qidx_hw,
						 descq->conf.pipe_flow_id,
						 descq->conf.c2h, false, false,
						 &stm_hw);
			if (rv < 0) {
				len += sprintf(buf + len,
						"%s read stm-can failed %d.\n",
					    descq->conf.name, rv);
				return rv;
			}

			len += sprintf(buf + len,
				       "\tSTM CAN:     [4]:0x%08x [3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
				       stm_hw.stm[4], stm_hw.stm[3],
				       stm_hw.stm[2], stm_hw.stm[1],
				       stm_hw.stm[0]);

			/** read stm c2h map or h2c map */
			rv = qdma_descq_stm_read(descq->xdev, descq->qidx_hw,
						 descq->conf.pipe_flow_id,
						 descq->conf.c2h, true, false,
						 &stm_hw);
			if (rv < 0) {
				len += sprintf(buf + len,
						"%s read stm-map failed %d.\n",
					    descq->conf.name, rv);
				return rv;
			}

			if (descq->conf.c2h)
				len += sprintf(buf + len,
					       "\tSTM C2H MAP: [0]:0x%08x\n",
					       stm_hw.stm[0]);
			else
				len += sprintf(buf + len,
					       "\tSTM H2C MAP: [4]:0x%08x [3]:0x%08x [2]:0x%08x [1]:0x%08x [0]:0x%08x\n",
					       stm_hw.stm[4], stm_hw.stm[3],
					       stm_hw.stm[2], stm_hw.stm[1],
					       stm_hw.stm[0]);
		}
	}
#endif
	len += sprintf(buf + len,
			"\ttotal descriptor processed:    %llu\n",
			descq->total_cmpl_descs);
	/** set the buffer end with \0 and return the buffer length */
	return len;
}

/*****************************************************************************/
/**
 * qdma_queue_dump_desc() - display a queue's descriptor ring from index start
 *							~ end in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	start:		start index
 * @param[in]	end:		end index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump_desc(unsigned long dev_hndl, unsigned long id,
			unsigned int start, unsigned int end, char *buf,
			int buflen)
{
	struct qdma_descq *descq = NULL;
	int len = 0;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/** Assume that sufficient buflen is provided
	 *  make sure that device handle provided is valid
	 */
	if (!xdev || !buf || !buflen)
		return QDMA_ERR_INVALID_INPUT_PARAM;

	/** get the descq details based on the qid provided */
	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 1);
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	/** dump the queue state */
	len = qdma_descq_dump_state(descq, buf, buflen);
	if (descq->q_state != Q_STATE_ONLINE)
		return len;

	/** dump the queue descriptor state */
	len += qdma_descq_dump_desc(descq, start, end, buf + len, buflen - len);
	return len;
}

/*****************************************************************************/
/**
 * qdma_queue_dump_cmpt() - display a queue's descriptor ring from index start
 *							~ end in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	start:		start index
 * @param[in]	end:		end index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_dump_cmpt(unsigned long dev_hndl, unsigned long id,
			unsigned int start, unsigned int end, char *buf,
			int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, buf, buflen, 1);
	int len = 0;

	/** make sure that descq is not NULL, else return error */
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	/** dump the descriptor state */
	len = qdma_descq_dump_state(descq, buf, buflen);
	/** if the descriptor is in online state,
	 *  then, dump the completion state
	 */
	if (descq->q_state == Q_STATE_ONLINE)
		len += qdma_descq_dump_cmpt(descq, start, end, buf + len,
				buflen - len);

	return len;
}

/* TODO: Currently this interface is not being used by any application.
 * Enable the code when needed !
 */
#if 0
/*****************************************************************************/
/**
 * qdma_queue_get_buf_sz() - to get queue's ch_buf_sz
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_get_buf_sz(unsigned long dev_hndl, unsigned long id,
			char *buf,
			int buflen)
{
	struct qdma_descq *descq = NULL;
	int len = 0;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/** Assume that sufficient buflen is provided
	 *  make sure that device handle provided is valid
	 */
	if (!xdev || !buf || !buflen)
		return QDMA_ERR_INVALID_INPUT_PARAM;

	/** get the descq details based on the qid provided */
	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 1);
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	/** dump the queue state */
	len = qdma_descq_dump_state(descq, buf, buflen);
	if (descq->q_state != Q_STATE_ONLINE)
		return len;

	/** dump the queue buf_sz */

	/* reg = QDMA_REG_C2H_BUF_SZ_BASE + (descq->conf.c2h_buf_sz_idx)*4; */
	/* buf_sz = __read_reg(xdev, reg); */
	buflen = snprintf(buf, buflen, "%d", descq->conf.c2h_bufsz);
	return descq->conf.c2h_bufsz;
}
#endif
/*****************************************************************************/
/**
 * qdma_queue_remove() - remove a queue (i.e., offline, NOT ready for dma)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_remove(unsigned long dev_hndl, unsigned long id, char *buf,
			int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq =
		qdma_device_get_descq_by_id(xdev, id, buf, buflen, 1);
#ifdef DEBUGFS
	struct qdma_descq *pair_descq =
		qdma_device_get_pair_descq_by_id(xdev, id, buf, buflen, 1);
#endif
	struct qdma_dev *qdev = xdev_2_qdev(xdev);

	/** make sure that qdev is not NULL, else return error */
	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	/** make sure that descq is not NULL, else return error */
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	if (descq->q_state != Q_STATE_ENABLED) {
		if (buf && buflen) {
			int len = snprintf(buf, buflen,
					"queue %s, id %u cannot be deleted. Invalid q state\n",
					descq->conf.name, descq->conf.qidx);

			buf[len] = '\0';
		}
		return QDMA_ERR_INVALID_DESCQ_STATE;

	}

#ifdef DEBUGFS
	if (pair_descq)
		/** if pair_descq is not NULL, it means the queue
		 * is in ENABLED state
		 */
		dbgfs_queue_exit(&descq->conf, pair_descq);
	else
		dbgfs_queue_exit(&descq->conf, NULL);
#endif
	qdma_descq_cleanup(descq);

	lock_descq(descq);
	descq->q_state = Q_STATE_DISABLED;
	unlock_descq(descq);

	spin_lock(&qdev->lock);
	if (descq->conf.c2h)
		qdev->c2h_qcnt--;
	else
		qdev->h2c_qcnt--;
	spin_unlock(&qdev->lock);
#ifndef __QDMA_VF__
	if (xdev->conf.qdma_drv_mode == LEGACY_INTR_MODE)
		intr_legacy_clear(descq);
#endif
	if (buf && buflen) {
		int len = snprintf(buf, buflen, "queue %s, id %u deleted.\n",
				descq->conf.name, descq->conf.qidx);
		buf[len] = '\0';
	}
	pr_debug("queue %s, id %u deleted.\n",
				descq->conf.name, descq->conf.qidx);

	return QDMA_OPERATION_SUCCESSFUL;
}

/*****************************************************************************/
/**
 * qdma_queue_config() - configure the queue with qcong parameters
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qid:		queue id
 * @param[in]	qconf:		queue configuration parameters
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_config(unsigned long dev_hndl, unsigned long qid,
			struct qdma_queue_conf *qconf, char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_descq *descq = NULL;

	/** make sure that qdev is not NULL, else return error */
	if  (!qdev) {
		pr_err("dev %s, qdev null.\n",
			dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	/** get the descq for the given qid */
	descq = qdma_device_get_descq_by_id(xdev, qid, NULL, 0, 0);
	if (!descq) {
		pr_err("Invalid queue ID! qid=%lu, max=%u\n", qid, qdev->qmax);
		return QDMA_ERR_INVALID_QIDX;
	}

	lock_descq(descq);
	/** if descq is not in disabled state, return error as
	 *  queue is enabled/in use, else enable the descq
	 */
	if (descq->q_state != Q_STATE_DISABLED) {
		pr_err("queue_%lu already configured!\n", qid);
		unlock_descq(descq);
		return -EINVAL;
	}
	descq->q_state = Q_STATE_ENABLED;
	unlock_descq(descq);

	spin_lock(&qdev->lock);
	/** increment the queue count */
	if (qconf->c2h)
		qdev->c2h_qcnt += 1;
	else
		qdev->h2c_qcnt += 1;
	spin_unlock(&qdev->lock);

	/** configure descriptor queue */
	qdma_descq_config(descq, qconf, 0);

	return QDMA_OPERATION_SUCCESSFUL;
}

/*****************************************************************************/
/**
 * qdma_queue_list() - display all configured queues in a string buffer
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	success: if optional message buffer used then strlen of buf,
 *	otherwise QDMA_OPERATION_SUCCESSFUL
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_list(unsigned long dev_hndl, char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_descq *descq = NULL;
	char *cur = buf;
	char * const end = buf + buflen;
	int i;

	/** make sure that qdev is not NULL, else return error */
	if  (!qdev) {
		pr_err("dev %s, qdev null.\n", dev_name(&xdev->conf.pdev->dev));
		return QDMA_ERR_INVALID_QDMA_DEVICE;
	}

	/** make sure that input buffer is not empty, else return error */
	if (!buf || !buflen) {
		pr_warn("invalid argument: buf=%p, buflen=%d", buf, buflen);
		return QDMA_ERR_INVALID_INPUT_PARAM;
	}

	cur += snprintf(cur, end - cur, "H2C Q: %u, C2H Q: %u.\n",
				qdev->h2c_qcnt, qdev->c2h_qcnt);
	if (cur >= end)
		goto handle_truncation;

	/** traverse through the h2c and c2h queue list
	 *  and dump the descriptors
	 */
	if (qdev->h2c_qcnt) {
		descq = qdev->h2c_descq;
		for (i = 0; i < qdev->qmax; i++, descq++) {
			lock_descq(descq);
			if (descq->q_state != Q_STATE_DISABLED)
				cur +=
				qdma_descq_dump(descq, cur, end - cur, 0);
			unlock_descq(descq);

			if (cur >= end)
				goto handle_truncation;
		}
	}

	if (qdev->c2h_qcnt) {
		descq = qdev->c2h_descq;
		for (i = 0; i < qdev->qmax; i++, descq++) {
			lock_descq(descq);
			if (descq->q_state != Q_STATE_DISABLED)
				cur +=
				qdma_descq_dump(descq, cur, end - cur, 0);
			unlock_descq(descq);

			if (cur >= end)
				goto handle_truncation;
		}
	}

	return cur - buf;

handle_truncation:

	return cur - buf;
}

/*****************************************************************************/
/**
 * qdma_queue_reconfig() - reconfigure the existing queue with
 *							modified configuration
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		existing queue id
 * @param[in]	qconf:		queue configuration parameters
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer, where the error message should
 *                              be appended. This buffer needs to be null
 *                              terminated.
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_reconfig(unsigned long dev_hndl, unsigned long id,
				struct qdma_queue_conf *qconf,
				char *buf, int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					 id, buf, buflen, 1);

	/** make sure that descq is not NULL, else return error */
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	lock_descq(descq);
	/** if descq is not in disabled state then,
	 *  return error as queue configuration can not be changed
	 */
	if (descq->q_state != Q_STATE_ENABLED) {
		pr_info("%s invalid state, q_state %d.\n",
			descq->conf.name, descq->q_state);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen - l,
				"%s invalid state, q_state %d.\n",
				descq->conf.name, descq->q_state);
			buf[l] = '\0';
		}
		unlock_descq(descq);
		return QDMA_ERR_INVALID_DESCQ_STATE;
	}
	/** Update the qconfig with new configuration provided */
	qdma_descq_config(descq, qconf, 1);
	unlock_descq(descq);

	return 0;
}

/*****************************************************************************/
/**
 * qdma_queue_add() - add a queue
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	qconf:		queue configuration parameters
 * @param[in]	qhndl:	list of unsigned long values that are the opaque qhndl
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer

 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_add(unsigned long dev_hndl, struct qdma_queue_conf *qconf,
			unsigned long *qhndl, char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_dev *qdev = xdev_2_qdev(xdev);
	struct qdma_descq *descq;
	struct qdma_descq *pairq;
	unsigned int qcnt;
	char *cur = buf;
	char * const end = buf + buflen;
	int rv = QDMA_OPERATION_SUCCESSFUL;

	/** If qconf is NULL, return error*/
	if (!qconf)
		return QDMA_ERR_INVALID_INPUT_PARAM;

#ifdef __QDMA_VF__
	/** allow only Q0 for if the VF Qmax is not set */
	if ((xdev->conf.cur_cfg_state == QMAX_CFG_INITIAL) &&
			(qconf->qidx > 0)) {
		rv = QDMA_ERR_INVALID_QIDX;
		if (buf && buflen) {
			cur += snprintf(cur, end - cur,
					"qdma%05x invalid idx %u >= 1.\n",
					xdev->conf.bdf, qconf->qidx);
			if (cur >= end)
				goto handle_truncation;
		}
		return rv;
	}
#endif

	/** If qhndl is NULL, return error*/
	if (!qhndl) {
		pr_warn("qhndl NULL.\n");
		rv = QDMA_ERR_INVALID_QIDX;
		if (buf && buflen) {
			cur += snprintf(cur, end - cur,
					"%s, add, qhndl NULL.\n",
					xdev->conf.name);
			if (cur >= end)
				goto handle_truncation;
		}
		return rv;
	}

	/** reset qhandle to an invalid value
	 * can't use 0 or NULL here because queue idx 0 has same value
	 */
	*qhndl = QDMA_QUEUE_IDX_INVALID;

	/** check if the requested mode is enabled?
	 *  the request modes are read from the HW
	 *  before serving any request, first check if the
	 *  HW has the capability or not, else return error
	 */
	if ((qconf->st && !xdev->st_mode_en) ||
	    (!qconf->st && !xdev->mm_mode_en)) {
		pr_warn("%s, %s mode not enabled.\n",
			xdev->conf.name, qconf->st ? "ST" : "MM");
		rv = QDMA_ERR_INTERFACE_NOT_ENABLED_IN_DEVICE;
		if (buf && buflen) {
			cur += snprintf(cur, end - cur,
				"qdma%05x %s mode not enabled.\n",
				xdev->conf.bdf, qconf->st ? "ST" : "MM");
			if (cur >= end)
				goto handle_truncation;
		}
		return rv;
	}

	spin_lock(&qdev->lock);
	/** if incase the qidx is not QDMA_QUEUE_IDX_INVALID
	 *  then, make sure that the qidx range falls between
	 *  0 - qdev->qmax, else return error
	 */
	if ((qconf->qidx != QDMA_QUEUE_IDX_INVALID) &&
	    (qconf->qidx >= qdev->qmax)) {
		spin_unlock(&qdev->lock);
		rv = QDMA_ERR_INVALID_QIDX;
		if (buf && buflen) {
			cur += snprintf(cur, end - cur,
				"qdma%05x invalid idx %u >= %u.\n",
				xdev->conf.bdf, qconf->qidx, qdev->qmax);
			if (cur >= end)
				goto handle_truncation;
		}
		return rv;
	}

	/** check if any free qidx available
	 *  if qcnt is >= qdev->qmax, return error as
	 *  no free queues found and descq is full
	 */
	qcnt = qconf->c2h ? qdev->c2h_qcnt : qdev->h2c_qcnt;
	if (qcnt >= qdev->qmax) {
		spin_unlock(&qdev->lock);
		pr_warn("No free queues %u/%u.\n", qcnt, qdev->qmax);
		rv = QDMA_ERR_DESCQ_FULL;
		if (buf && buflen) {
			cur += snprintf(cur, end - cur,
					"qdma%05x No free queues %u/%u.\n",
					xdev->conf.bdf, qcnt, qdev->qmax);
			if (cur >= end)
				goto handle_truncation;
		}
		return rv;
	}

	/** add to the count first, need to rewind if failed later*/
	if (qconf->c2h)
		qdev->c2h_qcnt++;
	else
		qdev->h2c_qcnt++;
	spin_unlock(&qdev->lock);

	if (qconf->c2h) {
		descq = qdev->c2h_descq;
		pairq = qdev->h2c_descq;
	} else {
		descq = qdev->h2c_descq;
		pairq = qdev->c2h_descq;
	}
	/** need to allocate a free qidx if it has an invalid id
	 *  ie. qidx is not specified in the add request
	 */
	if (qconf->qidx == QDMA_QUEUE_IDX_INVALID) {
		int i;

		/** loop through the qlist till qmax and find an empty descq*/
		for (i = 0; i < qdev->qmax; i++, descq++, pairq++) {
			/** make sure the queue pair are the same mode*/
			lock_descq(pairq);
			if ((pairq->q_state != Q_STATE_DISABLED)
					&& qconf->st != pairq->conf.st) {
				unlock_descq(pairq);
				continue;
			}
			unlock_descq(pairq);

			lock_descq(descq);
			if (descq->q_state != Q_STATE_DISABLED) {
				unlock_descq(descq);
				continue;
			}
			/** set the descq as enabled*/
			descq->q_state = Q_STATE_ENABLED;
			/** assign the qidx */
			qconf->qidx = i;
			unlock_descq(descq);

			break;
		}

		/** we are reached here means no free descq found
		 *  decrement the queue count and return error
		 */
		if (i == qdev->qmax) {
			pr_warn("no free %s qp found, %u.\n",
				qconf->st ? "ST" : "MM", qdev->qmax);
			rv = QDMA_ERR_DESCQ_FULL;
			if (buf && buflen) {
				cur += snprintf(cur, end - cur,
					"qdma%05x no %s QP, %u.\n",
					xdev->conf.bdf,
					qconf->st ? "ST" : "MM", qdev->qmax);
				if (cur >= end)
					goto handle_truncation;
			}
			goto rewind_qcnt;
		}
	} else { /** qidx specified in the given add request*/
		/** find the queue pair for the given qidx*/
		pairq += qconf->qidx;
		descq += qconf->qidx;

		/** make sure the queue pair are the same mode*/
		lock_descq(pairq);
		if ((pairq->q_state != Q_STATE_DISABLED)
				&& (qconf->st != pairq->conf.st)) {
			unlock_descq(pairq);
			rv = -EINVAL;
			if (buf && buflen) {
				cur += snprintf(cur, end - cur,
					"Need to have same mode for Q pair.\n");
				if (cur >= end)
					goto handle_truncation;
			}
			goto rewind_qcnt;
		}
		unlock_descq(pairq);

		lock_descq(descq);
		/** if the descq for the given qidx is already in enabled state,
		 *  then the queue is in use, return error
		 */
		if (descq->q_state != Q_STATE_DISABLED) {
			unlock_descq(descq);
			pr_info("descq idx %u already added.\n", qconf->qidx);
			rv = QDMA_ERR_DESCQ_IDX_ALREADY_ADDED;
			if (buf && buflen) {
				cur += snprintf(cur, end - cur,
						"q idx %u already added.\n",
						qconf->qidx);
			}
			goto rewind_qcnt;
		}
		descq->q_state = Q_STATE_ENABLED;
		unlock_descq(descq);
	}

	/** prepare the queue resources*/
	rv = qdma_device_prep_q_resource(xdev);
	if (rv < 0) {
#ifdef __QDMA_VF__
		lock_descq(descq);
		descq->q_state = Q_STATE_DISABLED;
		unlock_descq(descq);
#endif
		goto rewind_qcnt;
	}
#ifndef __QDMA_VF__
	if (xdev->conf.qdma_drv_mode == LEGACY_INTR_MODE) {
		rv = intr_legacy_setup(descq);
		if ((rv > 0) && buf && buflen) {
			/** support only 1 queue in legacy interrupt mode */
			intr_legacy_clear(descq);
			descq->q_state = Q_STATE_DISABLED;
			pr_debug("qdma%05x - Q%u - No free queues %u/%u.\n",
					xdev->conf.bdf, descq->conf.qidx,
					rv, 1);
			rv = -EINVAL;
			cur += snprintf(cur, end - cur,
					"qdma%05x No free queues %u/%u.\n",
					xdev->conf.bdf, qcnt, 1);
			goto rewind_qcnt;
		} else if ((rv < 0) && buf && buflen) {
			rv = -EINVAL;
			descq->q_state = Q_STATE_DISABLED;
			pr_debug("qdma%05x Legacy interrupt setup failed.\n",
					xdev->conf.bdf);
			cur += snprintf(cur, end - cur,
					"qdma%05x Legacy interrupt setup failed.\n",
					xdev->conf.bdf);
			goto rewind_qcnt;
		}
	}
#endif

	/** fill in config. info */
	qdma_descq_config(descq, qconf, 0);

	/** copy back the name in config*/
	memcpy(qconf->name, descq->conf.name, QDMA_QUEUE_NAME_MAXLEN);
	*qhndl = (unsigned long)descq->conf.qidx;
	if (qconf->c2h)
		*qhndl += qdev->qmax;

#ifdef DEBUGFS
	rv = dbgfs_queue_init(&descq->conf, pairq, xdev->dbgfs_queues_root);
	if (rv < 0) {
		pr_err("failed to create queue debug files for the queueu %d\n",
				descq->conf.qidx);
	}
#endif

	pr_debug("added %s, %s, qidx %u.\n",
		descq->conf.name, qconf->c2h ? "C2H" : "H2C", qconf->qidx);
	if (buf && buflen) {
		cur += snprintf(cur, end - cur, "%s %s added.\n",
			descq->conf.name, qconf->c2h ? "C2H" : "H2C");
		if (cur >= end)
			goto handle_truncation;
	}

	return QDMA_OPERATION_SUCCESSFUL;

rewind_qcnt:
	spin_lock(&qdev->lock);
	if (qconf->c2h)
		qdev->c2h_qcnt--;
	else
		qdev->h2c_qcnt--;
	spin_unlock(&qdev->lock);

	return rv;

handle_truncation:
	*buf = '\0';
	return rv;
}

/*****************************************************************************/
/**
 * qdma_queue_start() - start a queue (i.e, online, ready for dma)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer, where the error message should
 *                              be appended. This buffer needs to be null
 *                              terminated.
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_start(unsigned long dev_hndl, unsigned long id,
		     char *buf, int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					 id, buf, buflen, 1);
	int rv;

	/** make sure that descq is not NULL, else return error*/
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	/** complete the queue configuration*/
	rv = qdma_descq_config_complete(descq);
	if (rv < 0) {
		pr_err("%s 0x%x setup failed.\n",
			descq->conf.name, descq->qidx_hw);
		if (buf && buflen) {
			int l = strlen(buf);
			l += snprintf(buf + l, buflen - l,
				"%s config failed.\n", descq->conf.name);
		}
		goto free_resource;
	}

	lock_descq(descq);
	/** if the descq is not enabled,
	 *  it is in invalid state, return error
	 */
	if (descq->q_state != Q_STATE_ENABLED) {
		pr_info("%s invalid state, q_status%d.\n",
			descq->conf.name, descq->q_state);
		if (buf && buflen) {
			int l = strlen(buf);
			l += snprintf(buf + l, buflen - l,
				"%s invalid state, q_state %d.\n",
				descq->conf.name, descq->q_state);
		}
		unlock_descq(descq);
		return QDMA_ERR_INVALID_DESCQ_STATE;
	}
	unlock_descq(descq);
	/** allocate the queue resources*/
	rv = qdma_descq_alloc_resource(descq);
	if (rv < 0) {
		if (buf && buflen) {
			int l = strlen(buf);
			l += snprintf(buf + l, buflen - l,
				"%s alloc resource failed.\n",
				descq->conf.name);
				buf[l] = '\0';
		}
		goto free_resource;
	}

	/** program the hw contexts*/
	rv = qdma_descq_prog_hw(descq);
	if (rv < 0) {
		pr_err("%s 0x%x setup failed.\n",
			descq->conf.name, descq->qidx_hw);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen,
				"%s prog. context failed.\n",
				descq->conf.name);
			buf[l] = '\0';
		}
		goto clear_context;
	}

	/** Interrupt mode */
	if (descq->xdev->num_vecs) {
		unsigned long flags;

		spin_lock_irqsave(&descq->xdev->lock, flags);
		list_add_tail(&descq->intr_list,
		&descq->xdev->dev_intr_info_list[descq->intr_id].intr_list);
		spin_unlock_irqrestore(&descq->xdev->lock, flags);
	}

	qdma_thread_add_work(descq);

	if (buf && buflen) {
		rv = snprintf(buf, buflen, "%s started\n", descq->conf.name);
		if (rv <= 0 || rv >= buflen)
			goto clear_context;
	}
	/** set the descq to online state*/
	lock_descq(descq);
	descq->q_state = Q_STATE_ONLINE;
	unlock_descq(descq);

	return QDMA_OPERATION_SUCCESSFUL;

clear_context:
	qdma_descq_context_clear(descq->xdev, descq->qidx_hw, descq->conf.st,
				descq->conf.c2h, 1);
free_resource:
	qdma_descq_free_resource(descq);

	return rv;
}

#ifndef __QDMA_VF__
/*****************************************************************************/
/**
 * qdma_queue_prog_stm() - Program STM for queue (context, map, etc)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer, where the error message should
 *                              be appended. This buffer needs to be null
 *                              terminated.
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_prog_stm(unsigned long dev_hndl, unsigned long id,
			char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq =
		qdma_device_get_descq_by_id((struct xlnx_dma_dev *)dev_hndl,
					    id, buf, buflen, 1);
	int rv;

	/** make sure that descq is not NULL, else return error*/
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	if (!descq->conf.st) {
		pr_info("%s Skipping STM prog for MM queue.\n",
			descq->conf.name);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen - l,
				      "%s Skipping STM prog for MM queue.\n",
				      descq->conf.name);
		}
		return QDMA_OPERATION_SUCCESSFUL;
	}

	if (!xdev->stm_en) {
		pr_info("%s No STM present; stm_rev %d.\n",
			descq->conf.name, xdev->stm_rev);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen - l,
				      "%s No STM present; stm_rev %d.\n",
				      descq->conf.name, xdev->stm_rev);
		}
		return QDMA_ERR_INVALID_PCI_DEV;
	}

	lock_descq(descq);
	/** if the descq is not online,
	 *  it is in invalid state, return error
	 */
	if (descq->q_state != Q_STATE_ONLINE) {
		pr_info("%s invalid state, q_status%d.\n",
			descq->conf.name, descq->q_state);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen - l,
				      "%s invalid state, q_state %d.\n",
				      descq->conf.name, descq->q_state);
		}
		unlock_descq(descq);
		return QDMA_ERR_INVALID_DESCQ_STATE;
	}

	unlock_descq(descq);

	/** program the stm */
	rv = qdma_descq_prog_stm(descq, false);
	if (rv < 0) {
		pr_err("%s 0x%x STM setup failed.\n",
		       descq->conf.name, descq->qidx_hw);
		if (buf && buflen) {
			int l = strlen(buf);

			l += snprintf(buf + l, buflen - l,
				      "%s prog. STM failed.\n",
				      descq->conf.name);
		}
		return rv;
	}
	return QDMA_OPERATION_SUCCESSFUL;
}
#endif

/*****************************************************************************/
/**
 * qdma_queue_stop() - stop a queue (i.e., offline, NOT ready for dma)
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	id:		queue index
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_queue_stop(unsigned long dev_hndl, unsigned long id, char *buf,
			int buflen)
{
	struct qdma_descq *descq = qdma_device_get_descq_by_id(
					(struct xlnx_dma_dev *)dev_hndl,
					id, buf, buflen, 1);
	struct qdma_sgt_req_cb *cb, *tmp;
	struct qdma_request *req;
	unsigned int pend_list_empty = 0;

	/** make sure that descq is not NULL, else return error */
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	lock_descq(descq);
		/** if the descq not online donot proceed */
	if (descq->q_state != Q_STATE_ONLINE) {
		pr_info("%s invalid state, q_state %d.\n",
		descq->conf.name, descq->q_state);
		if (buf && buflen) {
			int l = snprintf(buf, buflen,
					 "queue %s, idx %u stop failed.\n",
					 descq->conf.name, descq->conf.qidx);
			if (l <= 0 || l >= buflen) {
				unlock_descq(descq);
				return QDMA_ERR_INVALID_INPUT_PARAM;
			}
		}
		unlock_descq(descq);
		return QDMA_ERR_INVALID_DESCQ_STATE;
	}
	pend_list_empty = descq->pend_list_empty;

	descq->q_stop_wait = 1;
	unlock_descq(descq);
	if (!pend_list_empty) {
		qdma_waitq_wait_event_timeout(descq->pend_list_wq,
			descq->pend_list_empty,
			msecs_to_jiffies(QDMA_Q_PEND_LIST_COMPLETION_TIMEOUT));
	}
	lock_descq(descq);
	/** free the descq by updating the state */
	descq->q_state = Q_STATE_ENABLED;
	descq->q_stop_wait = 0;
	list_for_each_entry_safe(cb, tmp, &descq->pend_list, list) {
		req = (struct qdma_request *)cb;
		cb->req_state = QDMA_REQ_COMPLETE;
		cb->done = 1;
		cb->status = -ENXIO;
		if (req->fp_done) {
			list_del(&cb->list);
			req->fp_done(req, 0, -ENXIO);
		} else
			qdma_waitq_wakeup(&cb->wq);
	}
	list_for_each_entry_safe(cb, tmp, &descq->work_list, list) {
		req = (struct qdma_request *)cb;
		cb->req_state = QDMA_REQ_COMPLETE;
		cb->done = 1;
		cb->status = -ENXIO;
		if (req->fp_done) {
			list_del(&cb->list);
			req->fp_done(req, 0, -ENXIO);
		} else
			qdma_waitq_wakeup(&cb->wq);
	}
	unlock_descq(descq);

	/** remove the work thread associated with the current queue */
	qdma_thread_remove_work(descq);

	/** clear the queue context */
	qdma_descq_context_clear(descq->xdev, descq->qidx_hw, descq->conf.st,
				descq->conf.c2h, 0);

	lock_descq(descq);
	/** if the device is in direct/indirect interrupt mode,
	 *  delete the interrupt list for the queue
	 */
	if ((descq->xdev->conf.qdma_drv_mode != POLL_MODE) &&
		(descq->xdev->conf.qdma_drv_mode != LEGACY_INTR_MODE)) {
		unsigned long flags;

		spin_lock_irqsave(&descq->xdev->lock, flags);
		list_del(&descq->intr_list);
		spin_unlock_irqrestore(&descq->xdev->lock, flags);
	}
#ifndef __QDMA_VF__
	/** clear stm context/maps */
	if (descq->xdev->stm_en)
		qdma_descq_prog_stm(descq, true);
#endif

	/** free the queue resources */
	qdma_descq_free_resource(descq);
	/** free the descq by updating the state */
	descq->total_cmpl_descs = 0;
	unlock_descq(descq);

	/** fill the return buffer indicating that queue is stopped */
	if (buf && buflen) {
		int len = snprintf(buf, buflen, "queue %s, idx %u stopped.\n",
				descq->conf.name, descq->conf.qidx);
		if (len <= 0 || len >= buflen)
			return QDMA_ERR_INVALID_INPUT_PARAM;
	}
	return QDMA_OPERATION_SUCCESSFUL;
}

/*****************************************************************************/
/**
 * qdma_intr_ring_dump() - display the interrupt ring info of a vector
 *
 * @param[in]	dev_hndl:	dev_hndl returned from qdma_device_open()
 * @param[in]	vector_idx:	vector number
 * @param[in]	start_idx:	interrupt ring start idx
 * @param[in]	end_idx:	interrupt ring end idx
 * @param[in]	buflen:		length of the input buffer
 * @param[out]	buf:		message buffer
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int qdma_intr_ring_dump(unsigned long dev_hndl, unsigned int vector_idx,
	int start_idx, int end_idx, char *buf, int buflen)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_intr_ring *ring_entry;
	struct intr_coal_conf *coal_entry;
	int counter = 0;
	int len = 0;
	u32 data[2];

	/** if, interrupt aggregation is not enabled,
	 *  interrupt ring is not created, return error
	 */
	if ((xdev->conf.qdma_drv_mode != INDIRECT_INTR_MODE) &&
			(xdev->conf.qdma_drv_mode != AUTO_MODE)) {
		pr_info("Interrupt aggregation not enabled\n");
		if (buf)  {
			len += sprintf(buf + len,
					"Interrupt aggregation not enabled\n");
			buf[len] = '\0';
		}
		return -1;
	}

	/** make sure that vector index is with in the
	 *  start and end vector limit, else return error
	 */
	if ((vector_idx < xdev->dvec_start_idx) ||
		(vector_idx >=
		(xdev->dvec_start_idx + QDMA_NUM_DATA_VEC_FOR_INTR_CXT))) {
		pr_info("Vector idx %d is invalid. Shall be in range: %d -  %d.\n",
				vector_idx,
				xdev->dvec_start_idx,
				(xdev->dvec_start_idx +
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT - 1));
		if (buf)  {
			len += sprintf(buf + len,
				"Vector idx %d is invalid. Shall be in range: %d -  %d.\n",
				vector_idx,
				xdev->dvec_start_idx,
				(xdev->dvec_start_idx +
				QDMA_NUM_DATA_VEC_FOR_INTR_CXT - 1));
			buf[len] = '\0';
		}
		return -1;
	}

	/** get the intr entry based on vector index */
	coal_entry = xdev->intr_coal_list + (vector_idx - xdev->dvec_start_idx);

	/** make sure that intr ring entry indexes
	 *  given are with in the range
	 */
	if (start_idx > coal_entry->intr_rng_num_entries) {
		pr_info("start_idx %d is invalid. Shall be less than: %d\n",
					start_idx,
					coal_entry->intr_rng_num_entries);
		if (buf)  {
			len += sprintf(buf + len,
					"start_idx %d is invalid. Shall be less than: %d\n",
					start_idx,
					coal_entry->intr_rng_num_entries);
			buf[len] = '\0';
		}
		return -1;
	}

	if (end_idx == -1 || end_idx >= coal_entry->intr_rng_num_entries)
		end_idx = coal_entry->intr_rng_num_entries - 1;

	if (start_idx == -1)
		start_idx = 0;

	if (start_idx > end_idx) {
		pr_info("start_idx can't be greater than end_idx\n");
		if (buf)  {
			len += sprintf(buf + len,
					"start_idx can't be greater than end_idx\n");
			buf[len] = '\0';
		}
		return -1;
	}

	/** read the ring entries based on the range given and
	 * update the input buffer with details
	 */
	for (counter = start_idx; counter <= end_idx; counter++) {
		ring_entry = coal_entry->intr_ring_base + counter;
		memcpy(data, ring_entry, sizeof(u32) * 2);
		if (buf) {
			len += sprintf(buf + len,
				       "intr_ring_entry = %d: 0x%08x 0x%08x\n",
				       counter, data[1], data[0]);
			buf[len] = '\0';
		}
	}

	return 0;
}

 /*****************************************************************************/
 /**
  * qdma_software_version_info  Provides the qdma software version
  *
  * @param[out]   software_version:   libqdma software version
  *
  * @return  0: success
  * @return  <0: error
  *****************************************************************************/
int qdma_software_version_info(char *software_version)
{
	if (!software_version)
		return -EINVAL;

	sprintf(software_version, "%s", LIBQDMA_VERSION_STR);

	return 0;
}


#ifdef __QDMA_VF__
 /*****************************************************************************/
 /**
  * qdma_vf_qconf    call for VF to request qmax number of Qs
  *
  * @param[in]   dev_hndl:   dev_hndl returned from qdma_device_open()
  * @param[in]   qmax:   number of qs requested by vf
  *
  * @return  0: success
  * @return  <0: error
  *****************************************************************************/
int qdma_vf_qconf(unsigned long dev_hndl, int qmax)
{
	int err = 0;
	u32 qbase = 0;
	struct qdma_dev *qdev;
	u32 last_qbase = 0;
	u32 last_qmax = 0;

	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev) {
		pr_err("Invalid dev handle\n");
		return -EINVAL;
	}

	qdev = xdev_2_qdev(xdev);

	if (!qdev) {
		pr_err("Invalid qdev\n");
		return -EINVAL;
	}

	/** save the last qbase values for restoring
	 * if the qconf command failed
	 */
	last_qbase = xdev->conf.qsets_base;
	last_qmax = xdev->conf.qsets_max;

	qdma_device_cleanup(xdev);
	err = device_set_qconf(xdev, qmax, &qbase);
	if (err < 0) {
		pr_err("Setting qconf failed\n");
		/** upon failure, go back to the last set qmax value */
		xdev->conf.qsets_base = last_qbase;
		xdev->conf.qsets_max = last_qmax;
		qdma_device_init(xdev);
		return err;
	}
	xdev->conf.qsets_base = qbase;
	xdev->conf.qsets_max = qmax;
	err  = qdma_device_init(xdev);
	if (err < 0) {
		pr_warn("qdma_init failed %d.\n", err);
		qdma_device_cleanup(xdev);
	}

	if (err < 0) {
		pr_err("Failed to set qmax %d for %s\n",
				qmax, dev_name(&xdev->conf.pdev->dev));
		return err;
	}

	return err;
}
#endif

/*****************************************************************************/
/**
 * sgl_unmap() - unmap the sg list from host pages
 *
 * @param[in]	pdev:	pointer to struct pci_dev
 * @param[in]	sg:		qdma sg request list
 * @param[in]	sgcnt:	number of sg lists
 * @param[in]	dir:	direction of the dma transfer
 *			DMA_BIDIRECTIONAL = 0,	DMA_TO_DEVICE = 1,
 *			DMA_FROM_DEVICE = 2, DMA_NONE = 3,
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
void sgl_unmap(struct pci_dev *pdev, struct qdma_sw_sg *sg, unsigned int sgcnt,
		 enum dma_data_direction dir)
{
	int i;

	/** unmap the sg list and set the dma_addr to 0 all sg entries */
	for (i = 0; i < sgcnt; i++, sg++) {
		if (!sg->pg)
			break;
		if (sg->dma_addr) {
			pci_unmap_page(pdev, sg->dma_addr - sg->offset,
							PAGE_SIZE, dir);
			sg->dma_addr = 0UL;
		}
	}
}

/*****************************************************************************/
/**
 * sgl_map() - map sg list to host pages
 *
 * @param[in]	pdev:	pointer to struct pci_dev
 * @param[in]	sg:		qdma sg request list
 * @param[in]	sgcnt:	number of sg lists
 * @param[in]	dir:	direction of the dma transfer
 *			DMA_BIDIRECTIONAL = 0,	DMA_TO_DEVICE = 1,
 *			DMA_FROM_DEVICE = 2, DMA_NONE = 3,
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int sgl_map(struct pci_dev *pdev, struct qdma_sw_sg *sgl, unsigned int sgcnt,
		enum dma_data_direction dir)
{
	int i;
	struct qdma_sw_sg *sg = sgl;

	/** Map the sg list onto a dma pages where
	 *  each page has max of PAGE_SIZE i.e 4K
	 */
	for (i = 0; i < sgcnt; i++, sg++) {
		/* !! TODO  page size !! */
		sg->dma_addr = pci_map_page(pdev, sg->pg, 0, PAGE_SIZE, dir);
		if (unlikely(pci_dma_mapping_error(pdev, sg->dma_addr))) {
			pr_info("map sgl failed, sg %d, %u.\n", i, sg->len);
			if (i)
				sgl_unmap(pdev, sgl, i, dir);
			return -EIO;
		}
		sg->dma_addr += sg->offset;
	}

	return 0;
}

/*****************************************************************************/
/**
 * qdma_request_submit() - submit a scatter-gather list of data for dma
 * operation (for both read and write)
 * This is a callback function called from upper layer(character device)
 * to handle the read/write request on the queues
 *
 * @param[in]	dev_hndl:	hndl retured from qdma_device_open()
 * @param[in]	id:			queue index
 * @param[in]	req:		qdma request
 *
 * @return	# of bytes transferred
 * @return	<0: error
 *****************************************************************************/
ssize_t qdma_request_submit(unsigned long dev_hndl, unsigned long id,
			struct qdma_request *req)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq =
		qdma_device_get_descq_by_id(xdev, id, NULL, 0, 1);
	struct qdma_sgt_req_cb *cb = qdma_req_cb_get(req);
	enum dma_data_direction dir;
	int wait = req->fp_done ? 0 : 1;
	int rv = 0;

	if (!descq)
		return -EINVAL;

	pr_debug("%s %s-%s, data len %u, sg cnt %u.\n",
		descq->conf.name, descq->conf.st ? "ST" : "MM",
		descq->conf.c2h ? "C2H" : "H2C", req->count, req->sgcnt);

	/** Identify the direction of the transfer */
	dir = descq->conf.c2h ?  DMA_FROM_DEVICE : DMA_TO_DEVICE;

	/** If write request is given on the C2H direction
	 *  OR, a read request given on non C2H direction
	 *  then, its an invalid request, return error in this case
	 */
	if ((req->write && descq->conf.c2h) ||
	    (!req->write && !descq->conf.c2h)) {
		pr_info("%s: bad direction, %c.\n",
			descq->conf.name, req->write ? 'W' : 'R');
		return -EINVAL;
	}

	/** Reset the local cb request with 0's */
	memset(cb, 0, QDMA_REQ_OPAQUE_SIZE);
	/** Initialize the wait queue */
	qdma_waitq_init(&cb->wq);

	pr_debug("%s: data len %u, ep 0x%llx, sgl 0x%p, sgl cnt %u, tm %u ms.\n",
		descq->conf.name, req->count, req->ep_addr, req->sgl,
		req->sgcnt, req->timeout_ms);

	/** If the request is streaming mode C2H, invoke the
	 *  handler to perform the read operation
	 */
	if (descq->conf.st && descq->conf.c2h)
		return qdma_request_submit_st_c2h(xdev, descq, req);

	if (!req->dma_mapped) {
		rv = sgl_map(xdev->conf.pdev,  req->sgl, req->sgcnt, dir);
		if (rv < 0) {
			pr_info("%s map sgl %u failed, %u.\n",
				descq->conf.name, req->sgcnt, req->count);
			goto unmap_sgl;
		}
		cb->unmap_needed = 1;
	}

	lock_descq(descq);
	/**  if the descq is already in online state*/
	if (descq->q_state != Q_STATE_ONLINE) {
		unlock_descq(descq);
		pr_info("%s descq %s NOT online.\n",
			xdev->conf.name, descq->conf.name);
		rv = -EINVAL;
		goto unmap_sgl;
	}
	list_add_tail(&cb->list, &descq->work_list);
	descq->pend_req_desc += ((req->count + PAGE_SIZE - 1) >> PAGE_SHIFT);
	unlock_descq(descq);

	pr_debug("%s: cb 0x%p submitted.\n", descq->conf.name, cb);

	qdma_descq_proc_sgt_request(descq);

	if (!wait)
		return 0;

	rv = qdma_request_wait_for_cmpl(xdev, descq, req);
	if (rv < 0)
		goto unmap_sgl;

	return cb->offset;

unmap_sgl:
	if (!req->dma_mapped)
		sgl_unmap(xdev->conf.pdev,  req->sgl, req->sgcnt, dir);

	return rv;
}

/*****************************************************************************/
/**
 * qdma_batch_request_submit() - submit a batch of scatter-gather list of data
 *  for dma operation (for both read and write)
 * This is a callback function called from upper layer(character device)
 * to handle the read/write request on the queues
 *
 * @param[in]	dev_hndl:	hndl retured from qdma_device_open()
 * @param[in]	id:			queue index
 * @param[in]	count:		Number of qdma requests
 * @param[in]	reqv:		qdma request vector
 *
 * @return	# of bytes transferred
 * @return	<0: error
 *****************************************************************************/
ssize_t qdma_batch_request_submit(unsigned long dev_hndl, unsigned long id,
			  unsigned long count, struct qdma_request **reqv)
{
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	struct qdma_descq *descq =
		qdma_device_get_descq_by_id(xdev, id, NULL, 0, 0);
	struct qdma_sgt_req_cb *cb;
	enum dma_data_direction dir;
	int rv = 0;
	unsigned long i;
	struct qdma_request *req;
	int st_c2h = 0;

	if (!descq)
		return -EINVAL;

	st_c2h = (descq->conf.st && descq->conf.c2h) ? 1 : 0;

	/** Identify the direction of the transfer */
	dir = descq->conf.c2h ?  DMA_FROM_DEVICE : DMA_TO_DEVICE;

	req = reqv[0];
	/** If write request is given on the C2H direction
	 *  OR, a read request given on non C2H direction
	 *  then, its an invalid request, return error in this case
	 */
	if ((req->write && descq->conf.c2h)
			|| (!req->write && !descq->conf.c2h)) {
		pr_info("%s: bad direction, %c.\n", descq->conf.name,
				req->write ? 'W' : 'R');
		return -EINVAL;
	}
	if (!req->fp_done) {
		pr_info("%s: missing fp_done.\n", descq->conf.name);
		return -EINVAL;
	}

	if (st_c2h) {
		for (i = 0; i < count; i++) {
			req = reqv[i];
			cb = qdma_req_cb_get(req);
			/** Reset the local cb request with 0's */
			memset(cb, 0, QDMA_REQ_OPAQUE_SIZE);

			rv = qdma_request_submit_st_c2h(xdev, descq, req);
			if ((rv < 0) || (rv == req->count))
				req->fp_done(req, rv, rv);
		}

		return 0;

	} else {
		struct pci_dev *pdev = xdev->conf.pdev;

		for (i = 0; i < count; i++) {
			req = reqv[i];
			cb = qdma_req_cb_get(req);
			/** Reset the local cb request with 0's */
			memset(cb, 0, QDMA_REQ_OPAQUE_SIZE);

			if (!req->dma_mapped) {
				rv = sgl_map(pdev, req->sgl, req->sgcnt, dir);
				if (unlikely(rv < 0)) {
					pr_info("%s map sgl %u failed, %u.\n",
						descq->conf.name,
						req->sgcnt,
						req->count);
					req->fp_done(req, 0, rv);
				}
				cb->unmap_needed = 1;
			}
		}
	}

	lock_descq(descq);
	/**  if the descq is already in online state*/
	if (unlikely(descq->q_state != Q_STATE_ONLINE)) {
		unlock_descq(descq);
		pr_info("%s descq %s NOT online.\n", xdev->conf.name,
				descq->conf.name);
		return -EINVAL;
	}

	for (i = 0; i < count; i++) {
		req = reqv[i];
		cb = qdma_req_cb_get(req);

		list_add_tail(&cb->list, &descq->work_list);
	}
	unlock_descq(descq);

	qdma_descq_proc_sgt_request(descq);

	return 0;
}

/*****************************************************************************/
/**
 * libqdma_init()       initialize the QDMA core library
 *
 * @param[in] num_threads - number of threads to be created each for request
 *  processing and writeback processing
 *
 * @return	0:	success
 * @return	<0:	error
 *****************************************************************************/
int libqdma_init(enum qdma_drv_mode qdma_drv_mode, unsigned int num_threads)
{

	/** Make sure that the size of qdma scatter gather request size
	 *  is less than the QDMA_REQ_OPAQUE_SIZE
	 *  If not, return error
	 */
	if (sizeof(struct qdma_sgt_req_cb) > QDMA_REQ_OPAQUE_SIZE) {
		pr_err("dma req. opaque data size too big %lu > %d.\n",
			sizeof(struct qdma_sgt_req_cb), QDMA_REQ_OPAQUE_SIZE);
		return -1;
	}
	if (sizeof(struct qdma_flq) > QDMA_FLQ_SIZE) {
		pr_err("qdma flq data size too big %lu > %d",
		       sizeof(struct qdma_flq), QDMA_FLQ_SIZE);
		return -1;
	}

	if (qdma_drv_mode == LEGACY_INTR_MODE)
		intr_legacy_init();

	/** Create the qdma threads */
	qdma_threads_create(num_threads);
#ifdef DEBUGFS
	return qdma_debugfs_init(&qdma_debugfs_root);
#else
	return 0;
#endif
}

/*****************************************************************************/
/**
 * libqdma_exit()	cleanup the QDMA core library before exiting
 *
 * @return	none
 *****************************************************************************/
void libqdma_exit(void)
{
#ifdef DEBUGFS
	qdma_debugfs_exit(qdma_debugfs_root);
#endif
	/** Destroy the qdma threads */
	qdma_threads_destroy();
}

#ifdef __LIBQDMA_MOD__
/** for module support only */
#include "version.h"

static char version[] =
	DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

/*****************************************************************************/
/**
 * libqdma_mod_init()	libqdma module initialization routine
 *
 * Returns: none
 *****************************************************************************/
static int __init libqdma_mod_init(void)
{
	pr_info("%s", version);

	/** invoke libqdma_init to initialize the libqdma library */
	return libqdma_init();
}

/*****************************************************************************/
/**
 * libqdma_mod_exit()       cleanup the QDMA core library before exiting
 *
 * Returns: none
 *****************************************************************************/
static void __exit libqdma_mod_exit(void)
{
	/** invoke libqdma_exit to cleanup the libqdma library resources */
	libqdma_exit();
}

module_init(libqdma_mod_init);
module_exit(libqdma_mod_exit);
#endif /* ifdef __LIBQDMA_MOD__ */
