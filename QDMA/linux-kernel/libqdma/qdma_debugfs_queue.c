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


#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdma_debugfs_queue.h"
#include "libqdma_export.h"
#include "qdma_regs.h"
#include "qdma_context.h"
#include "qdma_descq.h"
#include "qdma_regs.h"
#include <linux/uaccess.h>

#ifdef DEBUGFS
#define DEBUGFS_QUEUE_DESC_SZ	(100)
#define DEBUGFS_QUEUE_INFO_SZ	(256)
#define DEBUGFS_QUEUE_CTXT_SZ	(2 * 4096)

#define DEBUGFS_CTXT_ELEM(reg, pos, size)   \
	((reg >> pos) & ~(~0 << size))

#define DBGFS_QUEUE_INFO_SZ	256
#define DBGFS_ERR_BUFLEN    (64)

enum dbgfs_queue_info_type {
	DBGFS_QINFO_INFO = 0,
	DBGFS_QINFO_CNTXT = 1,
	DBGFS_QINFO_DESC = 2,
	DBGFS_QINFO_END,
};

enum dbgfs_cntxt_word {
	DBGFS_CNTXT_W0 = 0,
	DBGFS_CNTXT_W1 = 1,
	DBGFS_CNTXT_W2 = 2,
	DBGFS_CNTXT_W3 = 3,
	DBGFS_CNTXT_W4 = 4,
	DBGFS_CNTXT_W5 = 5,
	DBGFS_CNTXT_W6 = 6,
	DBGFS_CNTXT_W7 = 7,
};

enum dbgfs_cmpt_queue_info_type {
	DBGFS_CMPT_QINFO_INFO = 0,
	DBGFS_CMPT_QINFO_CNTXT = 1,
	DBGFS_CMPT_QINFO_DESC = 2,
	DBGFS_CMPT_QINFO_END,
};

struct dbgfs_q_dbgf {
	char name[DBGFS_DBG_FNAME_SZ];
	struct file_operations fops;
};

struct dbgfs_q_priv {
	unsigned long dev_hndl;
	unsigned long qhndl;
	char *data;
	int datalen;
};

struct dbgfs_qctxt_entry {
	char name[DBGFS_CTXT_ENTRY_NAME_SZ];
	unsigned short word;
	u32 pos;
	u32 len;
};

enum dbgfs_desc_type {
	DBGFS_DESC_TYPE_C2H = 0,
	DBGFS_DESC_TYPE_H2C = DBGFS_DESC_TYPE_C2H,
	DBGFS_DESC_TYPE_CMPT = 1,
	DBGFS_DESC_TYPE_END = 2,
};


static struct dbgfs_qctxt_entry app_ctxt[] = {
	/** format:
	 *{<name>, <word>, <start_bit_index>, <len>}
	 */

	{"PIDX", DBGFS_CNTXT_W0, S_DESC_CTXT_W0_PIDX,
		L_DESC_CTXT_W0_PIDX},
	{"IRQ Arm", DBGFS_CNTXT_W0, S_DESC_CTXT_W0_F_INTR_ARM, 1},
	{"Function Id", DBGFS_CNTXT_W0, S_DESC_CTXT_W0_FUNC_ID,
		L_DESC_CTXT_W0_FUNC_ID},

	{"Queue Enable", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_F_QEN, 1},
	{"Fetch Credit Enable", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_FCRD_EN, 1},
	{"Write back/Intr Check", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_CMPL_STATUS_PEND_CHK, 1},
	{"Write back Acc Enable", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_CMPL_STATUS_ACC_EN, 1},
	{"Address Translation", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_AT, 1},
	{"Fetch Max", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_FETCH_MAX, L_DESC_CTXT_W1_FETCH_MAX},
	{"Ring Size", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_RNG_SZ,
		L_DESC_CTXT_W1_RNG_SZ},
	{"Descriptor Size", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_DSC_SZ,
		L_DESC_CTXT_W1_DSC_SZ},
	{"Bypass Enable", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_F_BYP, 1},
	{"MM Channel", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_F_MM_CHN, 1},
	{"Writeback Enable", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_CMPL_STATUS_EN, 1},
	{"Interrupt Enable", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_IRQ_EN, 1},
	{"Port Id", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_PORT_ID,
		L_DESC_CTXT_W1_PORT_ID},
	{"Interrupt No Last", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_IRQ_NO_LAST, 1},
	{"Error", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_ERR,
		L_DESC_CTXT_W1_ERR},
	{"Writeback Error Sent", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_CMPL_STATUS_ERR_SNT, 1},
	{"IRQ Request", DBGFS_CNTXT_W1, S_DESC_CTXT_W1_F_IRQ_REQ, 1},
	{"Marker Disable", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_MRKR_DIS, 1},
	{"Is Memory Mapped", DBGFS_CNTXT_W1,
		S_DESC_CTXT_W1_F_IS_MM, 1},
	{"Interrupt Aggregation", DBGFS_CNTXT_W4,
		S_DESC_CTXT_W1_F_INTR_AGGR, 1},

	{"Descriptor Ring Base Addr (Low)", DBGFS_CNTXT_W2, 0, 32},

	{"Descriptor Ring Base Addr (High)", DBGFS_CNTXT_W3, 0, 32},
};

static struct dbgfs_qctxt_entry hw_ctxt[] = {
	{"CIDX", DBGFS_CNTXT_W0, 0, 16},
	{"Credits Consumed", DBGFS_CNTXT_W0, 16, 16},

	{"Descriptors Pending", DBGFS_CNTXT_W1, 8, 1},
	{"Queue Invalid No Desc Pending", DBGFS_CNTXT_W1, 9, 1},
	{"Eviction Pending", DBGFS_CNTXT_W1, 10, 1},
	{"Fetch Peding", DBGFS_CNTXT_W1, 10, 1},
};

static struct dbgfs_qctxt_entry credit_ctxt[] = {
	{"Credit", DBGFS_CNTXT_W0, 0, 16},
};

static struct dbgfs_qctxt_entry cmpt_ctxt[] = {
	{"Enable Status Desc Update", DBGFS_CNTXT_W0,
		S_CMPT_CTXT_W0_F_EN_STAT_DESC, 1},
	{"Enable Interrupt", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_F_EN_INT, 1},
	{"Trigger Mode", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_TRIG_MODE,
		L_CMPT_CTXT_W0_TRIG_MODE},
	{"Function Id", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_FNC_ID,
		L_CMPT_CTXT_W0_FNC_ID},
	{"Counter Index", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_COUNTER_IDX,
		L_CMPT_CTXT_W0_COUNTER_IDX},
	{"Timer Index", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_TIMER_IDX,
		L_CMPT_CTXT_W0_TIMER_IDX},
	{"Interrupt State", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_INT_ST,
		L_CMPT_CTXT_W0_INT_ST},
	{"Color", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_F_COLOR, 1},
	{"Ring Size", DBGFS_CNTXT_W0, S_CMPT_CTXT_W0_RNG_SZ,
		L_CMPT_CTXT_W0_RNG_SZ},

	{"Base Address (Low)", DBGFS_CNTXT_W1, 0, 32},

	{"Base Address (High)", DBGFS_CNTXT_W2, S_CMPT_CTXT_W2_BADDR_64,
		L_CMPT_CTXT_W2_BADDR_64},
	{"Descriptor Size", DBGFS_CNTXT_W2, S_CMPT_CTXT_W2_DESC_SIZE,
		L_CMPT_CTXT_W2_DESC_SIZE},
	{"PIDX (Low)", DBGFS_CNTXT_W2, S_CMPT_CTXT_W2_PIDX_L,
		L_CMPT_CTXT_W3_PIDX_H},

	{"PIDX (High)", DBGFS_CNTXT_W3, S_CMPT_CTXT_W3_PIDX_H,
		L_CMPT_CTXT_W3_PIDX_H},
	{"CIDX", DBGFS_CNTXT_W3, S_CMPT_CTXT_W3_CIDX, L_CMPT_CTXT_W3_CIDX},
	{"Valid", DBGFS_CNTXT_W3, S_CMPT_CTXT_W3_F_VALID, 1},
	{"ERROR", DBGFS_CNTXT_W3, S_CMPT_CTXT_W3_ERR, L_CMPT_CTXT_W3_ERR},
	{"Trigger Pending", DBGFS_CNTXT_W3, S_CMPT_CTXT_W3_F_TRIG_PEND, 1},

	{"Timer Running", DBGFS_CNTXT_W4, S_CMPT_CTXT_W4_F_TMR_RUNNING, 1},
	{"Full Update", DBGFS_CNTXT_W4, S_CMPT_CTXT_W4_F_FULL_UPDATE, 1},
	{"Over Flow Check Disable", DBGFS_CNTXT_W4,
		S_CMPT_CTXT_W4_F_OVF_CHK_DIS, 1},
	{"Address Translation", DBGFS_CNTXT_W4, S_CMPT_CTXT_W4_F_AT, 1},
};

/**TBD: enable them when RTL2 is stable */
static struct dbgfs_qctxt_entry c2h_pftch_ctxt[] = {
	{"Bypass", DBGFS_CNTXT_W0, S_PFTCH_W0_F_BYPASS, 1},
	{"Buffer Size Index", DBGFS_CNTXT_W0, S_PFTCH_W0_BUF_SIZE_IDX,
		L_PFTCH_W0_BUF_SIZE_IDX},
	{"Port Id", DBGFS_CNTXT_W0, S_PFTCH_W0_PORT_ID, L_PFTCH_W0_PORT_ID},
	{"Error", DBGFS_CNTXT_W0, S_PFTCH_W0_F_ERR, 1},
	{"Prefetch Enable", DBGFS_CNTXT_W0, S_PFTCH_W0_F_EN_PFTCH, 1},
	{"In Prefetch", DBGFS_CNTXT_W0, S_PFTCH_W0_F_Q_IN_PFTCH, 1},
	{"Software Credit (Low)", DBGFS_CNTXT_W0, S_PFTCH_W0_SW_CRDT_L,
		L_PFTCH_W0_SW_CRDT_L},

	{"Software Credit (High)", DBGFS_CNTXT_W1, S_PFTCH_W1_SW_CRDT_H,
		L_PFTCH_W1_SW_CRDT_H},
	{"Valid", DBGFS_CNTXT_W1, S_PFTCH_W1_F_VALID, 1},
};

/** structure to hold file ops */
static struct dbgfs_q_dbgf qf[DBGFS_QINFO_END];

/** structure to hold file ops */
static struct dbgfs_q_dbgf cmpt_qf[DBGFS_CMPT_QINFO_END];
int q_dbg_file_open(struct inode *inode, struct file *fp);
int q_dbg_file_release(struct inode *inode, struct file *fp);
int qdbg_info_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type);
int qdbg_desc_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type);
int qdbg_cntxt_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type);

/*****************************************************************************/
/**
 * cmpt_q_dbg_file_open() - static function that provides generic open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int cmpt_q_dbg_file_open(struct inode *inode, struct file *fp)
{
	return q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * cmpt_q_dbg_file_release() - static function that provides generic release
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int cmpt_q_dbg_file_release(struct inode *inode, struct file *fp)
{
	return q_dbg_file_release(inode, fp);
}

/*****************************************************************************/
/**
 * cmpt_q_dbg_file_read() - static function that provides common read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 * @param[in]	type: information type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t cmpt_q_dbg_file_read(struct file *fp,
		char __user *user_buffer,
		size_t count, loff_t *ppos,
		enum dbgfs_queue_info_type type)
{
	char *buf = NULL;
	int buf_len = 0;
	int len = 0;
	int rv = 0;
	struct dbgfs_q_priv *qpriv =
		(struct dbgfs_q_priv *)fp->private_data;

	if (qpriv->data == NULL && qpriv->datalen == 0) {
		if (type == DBGFS_QINFO_INFO) {
			rv = qdbg_info_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_CMPT);
		} else if (type == DBGFS_QINFO_CNTXT) {
			rv = qdbg_cntxt_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_CMPT);
		} else if (type == DBGFS_QINFO_DESC) {
			rv = qdbg_desc_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_CMPT);
		}

		if (rv < 0)
			goto cmpt_q_dbg_file_read_exit;

		qpriv->datalen = rv;
		qpriv->data = buf;
	}

	buf = qpriv->data;
	len = qpriv->datalen;

	if (!buf)
		goto cmpt_q_dbg_file_read_exit;

	/** write to user buffer */
	if (*ppos >= len) {
		rv = 0;
		goto cmpt_q_dbg_file_read_exit;
	}

	if (*ppos + count > len)
		count = len - *ppos;

	if (copy_to_user(user_buffer, buf + *ppos, count)) {
		rv = -EFAULT;
		goto cmpt_q_dbg_file_read_exit;
	}

	*ppos += count;

	pr_debug("cmpt q read size %zu\n", count);

	return count;

cmpt_q_dbg_file_read_exit:
	kfree(buf);
	qpriv->data = NULL;
	qpriv->datalen = 0;
	return rv;
}

/*****************************************************************************/
/**
 * cmpt_q_info_open() - static function that executes info open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int cmpt_q_info_open(struct inode *inode, struct file *fp)
{
	return cmpt_q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * cmpt_q_info_read() - static function that executes info read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t cmpt_q_info_read(struct file *fp, char __user *user_buffer,
			size_t count, loff_t *ppos)
{
	return cmpt_q_dbg_file_read(fp, user_buffer, count, ppos,
			DBGFS_QINFO_INFO);
}

/*****************************************************************************/
/**
 * cmpt_q_cntxt_open() - static function that executes info open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int cmpt_q_cntxt_open(struct inode *inode, struct file *fp)
{
	return cmpt_q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * cmpt_q_cntxt_read() - static function that executes info read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t cmpt_q_cntxt_read(struct file *fp, char __user *user_buffer,
			size_t count, loff_t *ppos)
{
	return cmpt_q_dbg_file_read(fp, user_buffer, count, ppos,
			DBGFS_QINFO_CNTXT);
}

/*****************************************************************************/
/**
 * cmpt_q_desc_open() - static function that executes descriptor open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int cmpt_q_desc_open(struct inode *inode, struct file *fp)
{
	return cmpt_q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * cmpt_q_desc_read() - static function that executes descriptor read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t cmpt_q_desc_read(struct file *fp, char __user *user_buffer,
			size_t count, loff_t *ppos)
{
	return cmpt_q_dbg_file_read(fp, user_buffer, count, ppos,
			DBGFS_QINFO_DESC);
}

/*****************************************************************************/
/**
 * create_cmpt_q_dbg_files() - static function to create cmpt queue dbg files
 *
 * @param[in]	queue_root:	debugfs root for a queue
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int create_cmpt_q_dbg_files(struct qdma_descq *descq, struct dentry *queue_root)
{
	struct dentry  *fp[DBGFS_QINFO_END] = { NULL };
	struct file_operations *fops = NULL;
	int i = 0;

	memset(cmpt_qf, 0, sizeof(struct dbgfs_q_dbgf) * DBGFS_CMPT_QINFO_END);

	for (i = 0; i < DBGFS_CMPT_QINFO_END; i++) {
		fops = &cmpt_qf[i].fops;
		fops->owner = THIS_MODULE;
		switch (i) {
		case DBGFS_CMPT_QINFO_INFO:
			snprintf(cmpt_qf[i].name, DBGFS_DBG_FNAME_SZ,
							"%s", "info");
			fops->open = cmpt_q_info_open;
			fops->read = cmpt_q_info_read;
			fops->release = cmpt_q_dbg_file_release;
			break;
		case DBGFS_CMPT_QINFO_CNTXT:
			snprintf(cmpt_qf[i].name, DBGFS_DBG_FNAME_SZ,
							"%s", "cntxt");
			fops->open = cmpt_q_cntxt_open;
			fops->read = cmpt_q_cntxt_read;
			fops->release = cmpt_q_dbg_file_release;
			break;
		case DBGFS_CMPT_QINFO_DESC:
			snprintf(cmpt_qf[i].name, DBGFS_DBG_FNAME_SZ,
							"%s", "desc");
			fops->open = cmpt_q_desc_open;
			fops->read = cmpt_q_desc_read;
			fops->release = cmpt_q_dbg_file_release;
			break;
		}
	}

	for (i = 0; i < DBGFS_CMPT_QINFO_END; i++) {
		fp[i] = debugfs_create_file(cmpt_qf[i].name, 0644, queue_root,
					descq, &cmpt_qf[i].fops);
		if (!fp[i])
			return -1;
	}
	return 0;
}

/*****************************************************************************/
/**
 * q_dbg_file_open() - generic queue debug file open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int q_dbg_file_open(struct inode *inode, struct file *fp)
{
	int dev_id = -1;
	int qidx = -1;
	struct dbgfs_q_priv *priv = NULL;
	int rv = 0;
	int c2h = 0;
	unsigned char dev_name[QDMA_DEV_NAME_SZ] = {0};
	unsigned char *lptr = NULL, *rptr = NULL;
	struct dentry *direction_dir = NULL;
	struct dentry *qid_dir = NULL;
	struct dentry *qroot_dir = NULL;
	struct dentry *dev_dir = NULL;
	struct qdma_descq *descq = NULL;

	if (!inode || !fp)
		return -EINVAL;
	descq = inode->i_private;
	if (!descq)
		return -EINVAL;

	direction_dir = fp->f_path.dentry->d_parent;
	qid_dir = direction_dir->d_parent;
	qroot_dir = qid_dir->d_parent;
	dev_dir = qroot_dir->d_parent;

	/* check the direction */
	if (!strncmp((const char *)direction_dir->d_iname,
				"c2h", strlen("c2h")) ||
			!strncmp((const char *)direction_dir->d_iname,
				"cmpt", strlen("cmpt")))
		c2h = 1;

	/* convert this string as integer */
	rv = kstrtoint((const char *)qid_dir->d_iname, 0, &qidx);
	if (rv < 0) {
		rv = -ENODEV;
		return rv;
	}

	/* convert colon sepearted b:d:f to hex */
	rptr = dev_dir->d_iname;
	lptr = dev_name;
	while (*rptr) {
		if (*rptr == ':') {
			rptr++;
			continue;
		}
		*lptr++ = *rptr++;
	}

	/* convert this string as hex integer */
	rv = kstrtoint((const char *)dev_name, 16, &dev_id);
	if (rv < 0) {
		rv = -ENODEV;
		return rv;
	}

	priv = (struct dbgfs_q_priv *) kzalloc(sizeof(struct dbgfs_q_priv),
			GFP_KERNEL);
	if (!priv) {
		rv = -ENOMEM;
		return rv;
	}

	priv->dev_hndl = (unsigned long)descq->xdev;
	priv->qhndl = qdma_device_get_id_from_descq(descq->xdev, descq);
	if (priv->qhndl < 0) {
		kfree(priv);
		return -EINVAL;
	}

	fp->private_data = priv;

	return 0;
}

/*****************************************************************************/
/**
 * q_dbg_file_release() - function that provides generic release
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int q_dbg_file_release(struct inode *inode, struct file *fp)
{
	kfree(fp->private_data);

	fp->private_data = NULL;

	return 0;
}

/*****************************************************************************/
/**
 * qdbg_parse_ctxt_to_buf() - parses queue context to human readable format
 *
 * @param[in]	ctxt: raw context info
 * @param[in]	entries: context entries structure pointer
 * @param[in]	num_entries:	number of context entries
 * @param[out]	buf: buffer to write parsed context
 * @param[in]	buflen: buffer len
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int qdbg_parse_ctxt_to_buf(u32 *ctxt,
		struct dbgfs_qctxt_entry *entries,
		int num_entries,
		char *buf, int buflen)
{
	int i = 0, w;
	u32 v;
	u64 mask = 0;
	int len = 0;

	for (i = num_entries - 1; i >= 0; i--) {
		w = entries[i].word;
		v = ((ctxt[w] >> entries[i].pos) &
				(~((~mask) << entries[i].len)));
		len += snprintf(buf + len, buflen - len,
				"\t%-47s %#-10x %u\n",
				entries[i].name, v, v);
	}
	len += snprintf(buf + len, buflen - len, "\n");

	return len;
}

/*****************************************************************************/
/**
 * qdbg_cntxt_read() - reads queue context for a queue
 *
 * @param[in]	dev_hndl:	xdev device handle
 * @param[in]	id: queue handle
 * @param[out]	buf: buffer to collect the context info
 * @param[in]	buflen: buffer len
 * @param[in]	type: context type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int qdbg_cntxt_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type)
{
	int rv = 0;
	int len = 0;
	int num_entries = 0;
	char *buf = NULL;
	int buflen = DEBUGFS_QUEUE_CTXT_SZ;
	struct hw_descq_context ctxt;
	struct qdma_descq *descq = NULL;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	if (!xdev)
		return -EINVAL;

	if (xdev_check_hndl(__func__, xdev->conf.pdev, dev_hndl) < 0)
		return -EINVAL;

	/* allocate memory */
	buf = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/** get descq by id */
	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 0);
	if (!descq) {
		kfree(buf);
		return QDMA_ERR_INVALID_QIDX;
	}

	/** initialize the context */
	memset(&ctxt, 0, sizeof(struct hw_descq_context));
	/** read the descq context for the given qid */
	rv = qdma_descq_context_read(descq->xdev, descq->qidx_hw,
			descq->conf.st, descq->conf.c2h, &ctxt);
	if (rv < 0) {
		len += sprintf(buf + len, "%s read context failed %d.\n",
				descq->conf.name, rv);
		buf[len] = '\0';

		*data = buf;
		*data_len = buflen;
		return rv;
	}

	if (type == DBGFS_DESC_TYPE_CMPT) {
		/** convert CMPT context to human readable text */
		len += snprintf(buf + len, buflen - len, "CMPT CTXT:\n");
		num_entries = sizeof(cmpt_ctxt)/sizeof(cmpt_ctxt[0]);
		len += qdbg_parse_ctxt_to_buf(ctxt.cmpt, cmpt_ctxt,
				num_entries, buf + len, buflen - len);
	} else {
		/** convert SW context to human readable text */
		len += snprintf(buf + len, buflen - len, "SOFTWARE CTXT:\n");
		num_entries = sizeof(app_ctxt)/sizeof(app_ctxt[0]);
		len += qdbg_parse_ctxt_to_buf(ctxt.sw, app_ctxt,
				num_entries, buf + len, buflen - len);

		/** convert hardware context to human readable text */
		len += snprintf(buf + len, buflen - len, "HARDWARE CTXT:\n");
		num_entries = sizeof(hw_ctxt)/sizeof(hw_ctxt[0]);
		len += qdbg_parse_ctxt_to_buf(ctxt.hw, hw_ctxt,
				num_entries, buf + len, buflen - len);
		if (!(descq->conf.st && descq->conf.c2h))
			goto cntxt_exit;

		/** convert credit context to human readable text */
		len += snprintf(buf + len, buflen - len, "CREDIT CTXT:\n");
		num_entries = sizeof(credit_ctxt)/sizeof(credit_ctxt[0]);
		len += qdbg_parse_ctxt_to_buf(ctxt.cr, credit_ctxt,
				num_entries, buf + len, buflen - len);

		if (type == DBGFS_DESC_TYPE_C2H) {
			/** convert prefetch context to human readable text */
			len += snprintf(buf + len, buflen - len,
							"PREFETCH CTXT:\n");
			num_entries =
			sizeof(c2h_pftch_ctxt)/sizeof(c2h_pftch_ctxt[0]);
			len += qdbg_parse_ctxt_to_buf(ctxt.prefetch,
							c2h_pftch_ctxt,
					num_entries, buf + len, buflen - len);
		}
	}

cntxt_exit:
	buf[len] = '\0';

	*data = buf;
	*data_len = buflen;

	return len;
}

/*****************************************************************************/
/**
 * qdbg_info_read() - reads queue info for a queue
 *
 * @param[in]	dev_hndl:	xdev device handle
 * @param[in]	id: queue handle
 * @param[out]	data: buffer pointer to collect the queue info
 * @param[out]	data_len: buffer len pointer
 * @param[in]	type: ring type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int qdbg_info_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type)
{
	int len = 0;
	char *buf = NULL;
	int buflen = DEBUGFS_QUEUE_INFO_SZ;
	struct qdma_descq *descq = NULL;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;

	/** allocate memory */
	buf = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 0);
	if (!descq) {
		kfree(buf);
		return QDMA_ERR_INVALID_QIDX;
	}

	len = qdma_descq_dump_state(descq, buf + len, buflen - len);

	*data = buf;
	*data_len = buflen;

	return len;
}

/*****************************************************************************/
/**
 * qdbg_desc_read() - reads descriptors of a queue
 *
 * @param[in]	dev_hndl:	xdev device handle
 * @param[in]	id: queue handle
 * @param[out]	data: buffer pointer to collect the queue descriptors
 * @param[out]	data_len: buffer len pointer
 * @param[in]	type: ring type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int qdbg_desc_read(unsigned long dev_hndl, unsigned long id, char **data,
		int *data_len, enum dbgfs_desc_type type)
{
	int len = 0;
	int rngsz = 0;
	struct qdma_descq *descq = NULL;
	struct xlnx_dma_dev *xdev = (struct xlnx_dma_dev *)dev_hndl;
	char *buf = NULL;
	int buflen = 0;

	descq = qdma_device_get_descq_by_id(xdev, id, buf, buflen, 0);
	if (!descq)
		return QDMA_ERR_INVALID_QIDX;

	/** get the ring size */
	if (type != DBGFS_DESC_TYPE_CMPT)
		rngsz = descq->conf.rngsz;
	else
		rngsz = descq->conf.rngsz_cmpt;

	/** 128 bytes is to accomodate header printed in the begining */
	buflen = (rngsz * DEBUGFS_QUEUE_DESC_SZ) + 128;

	/* allocate memory */
	buf = (char *) kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (type != DBGFS_DESC_TYPE_CMPT) {
		len += qdma_queue_dump_desc(dev_hndl, id,
				0, rngsz-1, buf + len, buflen - len);
	} else {
		len += qdma_queue_dump_cmpt(dev_hndl, id,
				0, rngsz-1, buf + len, buflen - len);
	}

	*data = buf;
	*data_len = buflen;

	return len;
}

/*****************************************************************************/
/**
 * q_dbg_file_read() - static function that provides common read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 * @param[in]	type: information type
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t q_dbg_file_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos, enum dbgfs_queue_info_type type)
{
	char *buf = NULL;
	int buf_len = 0, len = 0, rv = 0;
	struct dbgfs_q_priv *qpriv = (struct dbgfs_q_priv *)fp->private_data;

	if (qpriv->data == NULL && qpriv->datalen == 0) {
		if (type == DBGFS_QINFO_INFO) {
			rv = qdbg_info_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_C2H);
		} else if (type == DBGFS_QINFO_CNTXT) {
			rv = qdbg_cntxt_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_C2H);
		} else if (type == DBGFS_QINFO_DESC) {
			rv = qdbg_desc_read(qpriv->dev_hndl, qpriv->qhndl,
					&buf, &buf_len, DBGFS_DESC_TYPE_C2H);
		}

		if (rv < 0)
			goto q_dbg_file_read_exit;

		qpriv->datalen = rv;
		qpriv->data = buf;
	}

	buf = qpriv->data;
	len = qpriv->datalen;

	if (!buf)
		goto q_dbg_file_read_exit;

	/** write to user buffer */
	if (*ppos >= len) {
		rv = 0;
		goto q_dbg_file_read_exit;
	}

	if (*ppos + count > len)
		count = len - *ppos;

	if (copy_to_user(user_buffer, buf + *ppos, count)) {
		rv = -EFAULT;
		goto q_dbg_file_read_exit;
	}

	*ppos += count;
	rv = count;

	pr_debug("number of bytes written to user space is %zu\n", count);

q_dbg_file_read_exit:
	kfree(buf);
	qpriv->data = NULL;
	qpriv->datalen = 0;
	return rv;
}

/*****************************************************************************/
/**
 * q_info_open() - static function that executes info file open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int q_info_open(struct inode *inode, struct file *fp)
{
	return q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * q_info_read() - static function that executes info file read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t q_info_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos)
{
	return q_dbg_file_read(fp, user_buffer, count, ppos, DBGFS_QINFO_INFO);
}

/*****************************************************************************/
/**
 * q_cntxt_open() - static function that executes cntxt file open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int q_cntxt_open(struct inode *inode, struct file *fp)
{
	return q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * q_cntxt_read() - static function that performs cntxt file read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t q_cntxt_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos)
{
	return q_dbg_file_read(fp, user_buffer, count, ppos, DBGFS_QINFO_CNTXT);
}

/*****************************************************************************/
/**
 * q_desc_open() - static function that executes desc file open
 *
 * @param[in]	inode:	pointer to file inode
 * @param[in]	fp:	pointer to file structure
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
static int q_desc_open(struct inode *inode, struct file *fp)
{
	return q_dbg_file_open(inode, fp);
}

/*****************************************************************************/
/**
 * q_desc_read() - static function that executes desc read
 *
 * @param[in]	fp:	pointer to file structure
 * @param[out]	user_buffer: pointer to user buffer
 * @param[in]	count: size of data to read
 * @param[in/out]	ppos: pointer to offset read
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
static ssize_t q_desc_read(struct file *fp, char __user *user_buffer,
		size_t count, loff_t *ppos)
{
	return q_dbg_file_read(fp, user_buffer, count, ppos, DBGFS_QINFO_DESC);
}

/*****************************************************************************/
/**
 * create_q_dbg_files() - static function to create queue debug files
 *
 * @param[in]	queue_root:	debugfs root for a queue
 *
 * @return	>0: size read
 * @return	<0: error
 *****************************************************************************/
int create_q_dbg_files(struct qdma_descq *descq, struct dentry *queue_root)
{
	struct dentry  *fp[DBGFS_QINFO_END] = { NULL };
	struct file_operations *fops = NULL;
	int i = 0;

	memset(qf, 0, sizeof(struct dbgfs_q_dbgf) * DBGFS_QINFO_END);

	for (i = 0; i < DBGFS_QINFO_END; i++) {
		fops = &qf[i].fops;
		fops->owner = THIS_MODULE;
		switch (i) {
		case DBGFS_QINFO_INFO:
			snprintf(qf[i].name, DBGFS_DBG_FNAME_SZ, "%s", "info");
			fops->open = q_info_open;
			fops->read = q_info_read;
			fops->release = q_dbg_file_release;
			break;
		case DBGFS_QINFO_CNTXT:
			snprintf(qf[i].name, DBGFS_DBG_FNAME_SZ, "%s", "cntxt");
			fops->open = q_cntxt_open;
			fops->read = q_cntxt_read;
			fops->release = q_dbg_file_release;
			break;
		case DBGFS_QINFO_DESC:
			snprintf(qf[i].name, DBGFS_DBG_FNAME_SZ, "%s", "desc");
			fops->open = q_desc_open;
			fops->read = q_desc_read;
			fops->release = q_dbg_file_release;
			break;
		}
	}

	for (i = 0; i < DBGFS_QINFO_END; i++) {
		fp[i] = debugfs_create_file(qf[i].name, 0644, queue_root,
				descq, &qf[i].fops);
		if (!fp[i])
			return -1;
	}
	return 0;
}

/*****************************************************************************/
/**
 * dbgfs_queue_init() - queue initialization function
 *
 * @param[in]	conf:	queue configuration
 * @param[in]	pair_conf:	pair queue configuration
 * @param[in]	dbgfs_queues_root:	root directory for all queues
 *
 * @return	0: success
 * @return	<0: error
 *****************************************************************************/
int dbgfs_queue_init(struct qdma_queue_conf *conf,
		struct qdma_descq *pairq,
		struct dentry *dbgfs_queues_root)
{
	char qname[16] = {0};
	char qdir[8] = {0};
	struct dentry *dbgfs_qidx_root = NULL;
	struct dentry *dbgfs_queue_root = NULL;
	struct dentry *dbgfs_cmpt_queue_root = NULL;
	struct qdma_descq *descq = container_of(conf, struct qdma_descq, conf);
	int rv = 0;

	if (!descq)
		return -EINVAL;
	snprintf(qname, 16, "%u", conf->qidx);

	spin_lock(&descq->xdev->qidx_lock);
	/** create queue root only if it is not created */
	if (pairq->dbgfs_qidx_root) {
		dbgfs_qidx_root = pairq->dbgfs_qidx_root;
	} else {
		/* create a directory for the queue in debugfs */
		dbgfs_qidx_root = debugfs_create_dir(qname,
				dbgfs_queues_root);
		if (!dbgfs_qidx_root) {
			pr_err("Failed to create queue [%s] directory\n",
					qname);
			spin_unlock(&descq->xdev->qidx_lock);
			return -1;
		}
	}

	/* create a directory for direction */
	if (conf->c2h)
		snprintf(qdir, 8, "%s", "c2h");
	else
		snprintf(qdir, 8, "%s", "h2c");

	dbgfs_queue_root = debugfs_create_dir(qdir,
			dbgfs_qidx_root);
	if (!dbgfs_queue_root) {
		pr_err("Failed to create %s directory under %s",
				qdir, qname);
		goto dbgfs_queue_init_fail;
	}

	if (conf->c2h && conf->st) {
		/* create a directory for the cmpt in debugfs */
		dbgfs_cmpt_queue_root = debugfs_create_dir("cmpt",
				dbgfs_qidx_root);
		if (!dbgfs_cmpt_queue_root) {
			pr_err("Failed to create cmpt directory under %s",
					qname);
			goto dbgfs_queue_init_fail;
		}
	}

	/* intialize fops and create all the files */
	rv = create_q_dbg_files(descq, dbgfs_queue_root);
	if (rv < 0) {
		pr_err("Failed to create qdbg files, removing %s dir\n",
				qdir);
		debugfs_remove_recursive(dbgfs_queue_root);
		goto dbgfs_queue_init_fail;
	}

	if (dbgfs_cmpt_queue_root) {
		rv = create_cmpt_q_dbg_files(descq, dbgfs_cmpt_queue_root);
		if (rv < 0) {
			pr_err("Failed to create cmptq dbg files,removing cmpt dir\n");
			debugfs_remove_recursive(dbgfs_cmpt_queue_root);
			goto dbgfs_queue_init_fail;
		}
	}

	descq->dbgfs_qidx_root = dbgfs_qidx_root;
	descq->dbgfs_queue_root = dbgfs_queue_root;
	descq->dbgfs_cmpt_queue_root = dbgfs_cmpt_queue_root;
	spin_unlock(&descq->xdev->qidx_lock);

	return 0;

dbgfs_queue_init_fail:
	if (pairq->dbgfs_qidx_root) {
		spin_unlock(&descq->xdev->qidx_lock);
		return -1;
	}
	pr_err("Failed to init q debug files, removing [%s] dir\n", qname);
	debugfs_remove_recursive(dbgfs_qidx_root);
	spin_unlock(&descq->xdev->qidx_lock);
	return -1;
}

/*****************************************************************************/
/**
 * dbgfs_queue_exit() - debugfs queue teardown function
 *
 * @param[in]	conf:	queue configuration
 * @param[in]	conf:	pair queue configuration
 *
 *****************************************************************************/
void dbgfs_queue_exit(struct qdma_queue_conf *conf,
		struct qdma_descq *pairq)
{
	struct qdma_descq *descq = container_of(conf, struct qdma_descq, conf);

	if (!descq)
		return;
	debugfs_remove_recursive(descq->dbgfs_queue_root);
	debugfs_remove_recursive(descq->dbgfs_cmpt_queue_root);
	descq->dbgfs_queue_root = NULL;
	descq->dbgfs_cmpt_queue_root = NULL;
	if (!pairq)
		debugfs_remove_recursive(descq->dbgfs_qidx_root);

	descq->dbgfs_qidx_root = NULL;
}

#endif
