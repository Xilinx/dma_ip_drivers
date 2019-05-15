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

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <net/genetlink.h>

#include "libqdma/libqdma_export.h"
#include "qdma_mod.h"
#include "qdma_nl.h"
#include "nl.h"
#include "version.h"

static int xnl_dev_list(struct sk_buff *skb_2, struct genl_info *info);

static struct nla_policy xnl_policy[XNL_ATTR_MAX] = {
	[XNL_ATTR_GENMSG] =		{ .type = NLA_NUL_STRING },

	[XNL_ATTR_DRV_INFO] =		{ .type = NLA_NUL_STRING },

	[XNL_ATTR_DEV_IDX] =		{ .type = NLA_U32 },
	[XNL_ATTR_PCI_BUS] =		{ .type = NLA_U32 },
	[XNL_ATTR_PCI_DEV] =		{ .type = NLA_U32 },
	[XNL_ATTR_PCI_FUNC] =		{ .type = NLA_U32 },
	[XNL_ATTR_DEV_CFG_BAR] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_USR_BAR] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_QSET_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_QSET_QBASE] =	{ .type = NLA_U32 },

	[XNL_ATTR_REG_BAR_NUM] =	{ .type = NLA_U32 },
	[XNL_ATTR_REG_ADDR] =		{ .type = NLA_U32 },
	[XNL_ATTR_REG_VAL] =		{ .type = NLA_U32 },

	[XNL_ATTR_QIDX] =		{ .type = NLA_U32 },
	[XNL_ATTR_NUM_Q] =		{ .type = NLA_U32 },
	[XNL_ATTR_QFLAG] =		{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_DESC_SIZE] =	{ .type = NLA_U32 },
	[XNL_ATTR_SW_DESC_SIZE] =	{ .type = NLA_U32 },
	[XNL_ATTR_QRNGSZ_IDX] =		{ .type = NLA_U32 },
	[XNL_ATTR_C2H_BUFSZ_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_TIMER_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_CNTR_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_TRIG_MODE] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_START] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_END] =		{ .type = NLA_U32 },

	[XNL_ATTR_INTR_VECTOR_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_GL_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_FLOW_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_SLR_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_TDEST] =		{ .type = NLA_U32 },

	[XNL_ATTR_DEV_STM_BAR] =	{ .type = NLA_U32 },
#ifdef ERR_DEBUG
	[XNL_ATTR_QPARAM_ERR_INFO] =    { .type = NLA_U32 },
#endif
};

static int xnl_dev_list(struct sk_buff *, struct genl_info *);
static int xnl_dev_info(struct sk_buff *, struct genl_info *);
static int xnl_dev_version(struct sk_buff *skb2, struct genl_info *info);
static int xnl_dev_stat(struct sk_buff *, struct genl_info *);
static int xnl_dev_stat_clear(struct sk_buff *, struct genl_info *);
static int xnl_q_list(struct sk_buff *, struct genl_info *);
static int xnl_q_add(struct sk_buff *, struct genl_info *);
static int xnl_q_start(struct sk_buff *, struct genl_info *);
static int xnl_q_stop(struct sk_buff *, struct genl_info *);
static int xnl_q_del(struct sk_buff *, struct genl_info *);
static int xnl_q_dump(struct sk_buff *, struct genl_info *);
static int xnl_q_dump_desc(struct sk_buff *, struct genl_info *);
static int xnl_q_dump_cmpt(struct sk_buff *, struct genl_info *);
static int xnl_q_read_pkt(struct sk_buff *, struct genl_info *);
static int xnl_q_read_udd(struct sk_buff *, struct genl_info *);
static int xnl_intr_ring_dump(struct sk_buff *, struct genl_info *);
static int xnl_register_read(struct sk_buff *, struct genl_info *);
static int xnl_register_write(struct sk_buff *, struct genl_info *);
static int xnl_get_global_csr(struct sk_buff *skb2, struct genl_info *info);

#ifdef ERR_DEBUG
static int xnl_err_induce(struct sk_buff *skb2, struct genl_info *info);
#endif

struct genl_ops xnl_ops[] = {
	{
		.cmd = XNL_CMD_DEV_LIST,
		.policy = xnl_policy,
		.doit = xnl_dev_list,
	},
	{
		.cmd = XNL_CMD_VERSION,
		.policy = xnl_policy,
		.doit = xnl_dev_version,
	},
	{
		.cmd = XNL_CMD_DEV_INFO,
		.policy = xnl_policy,
		.doit = xnl_dev_info,
	},
	{
		.cmd = XNL_CMD_DEV_STAT,
		.policy = xnl_policy,
		.doit = xnl_dev_stat,
	},
	{
		.cmd = XNL_CMD_DEV_STAT_CLEAR,
		.policy = xnl_policy,
		.doit = xnl_dev_stat_clear,
	},
	{
		.cmd = XNL_CMD_Q_LIST,
		.policy = xnl_policy,
		.doit = xnl_q_list,
	},
	{
		.cmd = XNL_CMD_Q_ADD,
		.policy = xnl_policy,
		.doit = xnl_q_add,
	},
	{
		.cmd = XNL_CMD_Q_START,
		.policy = xnl_policy,
		.doit = xnl_q_start,
	},
	{
		.cmd = XNL_CMD_Q_STOP,
		.policy = xnl_policy,
		.doit = xnl_q_stop,
	},
	{
		.cmd = XNL_CMD_Q_DEL,
		.policy = xnl_policy,
		.doit = xnl_q_del,
	},
	{
		.cmd = XNL_CMD_Q_DUMP,
		.policy = xnl_policy,
		.doit = xnl_q_dump,
	},
	{
		.cmd = XNL_CMD_Q_DESC,
		.policy = xnl_policy,
		.doit = xnl_q_dump_desc,
	},
	{
		.cmd = XNL_CMD_Q_CMPT,
		.policy = xnl_policy,
		.doit = xnl_q_dump_cmpt,
	},
	{
		.cmd = XNL_CMD_Q_UDD,
		.policy = xnl_policy,
		.doit = xnl_q_read_udd,
	},
	{
		.cmd = XNL_CMD_Q_RX_PKT,
		.policy = xnl_policy,
		.doit = xnl_q_read_pkt,
	},
	{
		.cmd = XNL_CMD_INTR_RING_DUMP,
		.policy = xnl_policy,
		.doit = xnl_intr_ring_dump,
	},
	{
		.cmd = XNL_CMD_REG_RD,
		.policy = xnl_policy,
		.doit = xnl_register_read,
	},
	{
		.cmd = XNL_CMD_REG_WRT,
		.policy = xnl_policy,
		.doit = xnl_register_write,
	},
	{
		.cmd = XNL_CMD_GLOBAL_CSR,
		.policy = xnl_policy,
		.doit = xnl_get_global_csr,
	},
#ifdef ERR_DEBUG
	{
		.cmd = XNL_CMD_Q_ERR_INDUCE,
		.policy = xnl_policy,
		.doit = xnl_err_induce,
	}
#endif
};

static struct genl_family xnl_family = {
#ifdef GENL_ID_GENERATE
	.id = GENL_ID_GENERATE,
#endif
	.hdrsize = 0,
#ifdef __QDMA_VF__
	.name = XNL_NAME_VF,
#else
	.name = XNL_NAME_PF,
#endif
#ifndef __GENL_REG_FAMILY_OPS_FUNC__
	.ops = xnl_ops,
	.n_ops = ARRAY_SIZE(xnl_ops),
#endif
	.maxattr = XNL_ATTR_MAX - 1,
};

static struct sk_buff *xnl_msg_alloc(enum xnl_op_t op, int min_sz,
				void **hdr, struct genl_info *info)
{
	struct sk_buff *skb;
	void *p;
	unsigned long sz = min_sz < NLMSG_GOODSIZE ? NLMSG_GOODSIZE : min_sz;

	skb = genlmsg_new(sz, GFP_KERNEL);
	if (!skb) {
		pr_info("OOM %lu.\n", sz);
		return NULL;
	}

	p = genlmsg_put(skb, 0, info->snd_seq + 1, &xnl_family, 0, op);
	if (!p) {
		pr_info("skb too small.\n");
		nlmsg_free(skb);
		return NULL;
	}

	*hdr = p;
	return skb;
}

static inline int xnl_msg_add_attr_str(struct sk_buff *skb,
					enum xnl_attr_t type, char *s)
{
	int rv;

	rv = nla_put_string(skb, type, s);
	if (rv != 0) {
		pr_info("nla_put_string return %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}

static inline int xnl_msg_add_attr_data(struct sk_buff *skb,
		enum xnl_attr_t type, void *s, unsigned int len)
{
	int rv;

	rv = nla_put(skb, type, len, s);
	if (rv != 0) {
		pr_info("nla_put return %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}


static inline int xnl_msg_add_attr_uint(struct sk_buff *skb,
					enum xnl_attr_t type, u32 v)
{
	int rv;

	rv = nla_put_u32(skb, type, v);
	if (rv != 0) {
		pr_info("nla add dev_idx failed %d.\n", rv);
		return -EINVAL;
	}
	return 0;
}

static inline int xnl_msg_send(struct sk_buff *skb_tx, void *hdr,
				struct genl_info *info)
{
	int rv;

	genlmsg_end(skb_tx, hdr);

	rv = genlmsg_unicast(genl_info_net(info), skb_tx, info->snd_portid);
	if (rv)
		pr_info("send portid %d failed %d.\n", info->snd_portid, rv);

	return 0;
}

#ifdef DEBUG
static int xnl_dump_attrs(struct genl_info *info)
{
	int i;

	pr_info("snd_seq 0x%x, portid 0x%x.\n",
		info->snd_seq, info->snd_portid);
#if 0
	print_hex_dump_bytes("nlhdr", DUMP_PREFIX_OFFSET, info->nlhdr,
			sizeof(struct nlmsghdr));
	pr_info("\n");
	print_hex_dump_bytes("genlhdr", DUMP_PREFIX_OFFSET, info->genlhdr,
			sizeof(struct genlmsghdr));
	pr_info("\n");
#endif

	pr_info("nlhdr: len %u, type %u, flags 0x%x, seq 0x%x, pid %u.\n",
		info->nlhdr->nlmsg_len,
		info->nlhdr->nlmsg_type,
		info->nlhdr->nlmsg_flags,
		info->nlhdr->nlmsg_seq,
		info->nlhdr->nlmsg_pid);
	pr_info("genlhdr: cmd 0x%x %s, version %u, reserved 0x%x.\n",
		info->genlhdr->cmd, xnl_op_str[info->genlhdr->cmd],
		info->genlhdr->version,
		info->genlhdr->reserved);

	for (i = 0; i < XNL_ATTR_MAX; i++) {
		struct nlattr *na = info->attrs[i];

		if (na) {
			if (xnl_policy[i].type == NLA_NUL_STRING) {
				char *s = (char *)nla_data(na);

				if (s)
					pr_info("attr %d, %s, str %s.\n",
						i, xnl_attr_str[i], s);
				else
					pr_info("attr %d, %s, str NULL.\n",
						i, xnl_attr_str[i]);

			} else {
				u32 v = nla_get_u32(na);

				pr_info("attr %s, u32 0x%x.\n",
					xnl_attr_str[i], v);
			}
		}
	}

	return 0;
}
#else
#define xnl_dump_attrs(info)
#endif

int xnl_respond_buffer(struct genl_info *info, char *buf, int buflen)
{
	struct sk_buff *skb;
	void *hdr;
	int rv;

	skb = xnl_msg_alloc(info->genlhdr->cmd, buflen, &hdr, info);
	if (!skb)
		return -ENOMEM;

	rv = xnl_msg_add_attr_str(skb, XNL_ATTR_GENMSG, buf);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);

	return rv;
}

int xnl_respond_data(struct genl_info *info, void *buf, int buflen)
{
	struct sk_buff *skb;
	void *hdr;
	int rv;

	skb = xnl_msg_alloc(info->genlhdr->cmd, buflen, &hdr, info);
	if (!skb)
		return -ENOMEM;

	rv = xnl_msg_add_attr_data(skb, XNL_ATTR_GLOBAL_CSR, buf, buflen);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_data() failed: %d", rv);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);
	return rv;
}

static char *xnl_mem_alloc(int l, struct genl_info *info)
{
	char ebuf[XNL_ERR_BUFLEN];
	char *buf = kmalloc(l, GFP_KERNEL);

	if (buf) {
		memset(buf, 0, l);
		return buf;
	}

	pr_info("xnl OOM %d.\n", l);

	snprintf(ebuf, XNL_ERR_BUFLEN, "ERR! xnl OOM %d.\n", l);

	xnl_respond_buffer(info, ebuf, XNL_ERR_BUFLEN);

	return NULL;
}

static struct xlnx_pci_dev *xnl_rcv_check_xpdev(struct genl_info *info)
{
	u32 idx;
	struct xlnx_pci_dev *xpdev;
	char err[XNL_ERR_BUFLEN];
	int rv;

	if (info == NULL)
		return NULL;

	if (!info->attrs[XNL_ATTR_DEV_IDX]) {
		rv = snprintf(err, sizeof(err),
						"command %s missing attribute XNL_ATTR_DEV_IDX",
						xnl_op_str[info->genlhdr->cmd]);
		if (rv <= 0)
			return NULL;
		goto respond_error;
	}
	idx = nla_get_u32(info->attrs[XNL_ATTR_DEV_IDX]);

	xpdev = xpdev_find_by_idx(idx, err, sizeof(err));
	if (!xpdev) {
		/* err buffer populated by xpdev_find_by_idx*/
		goto respond_error;
	}

	return xpdev;

respond_error:
	xnl_respond_buffer(info, err, strlen(err));
	return NULL;
}

static int qconf_get(struct qdma_queue_conf *qconf, struct genl_info *info,
			char *err, int errlen, unsigned char *is_qp)
{
	u32 f = 0;

	if (!qconf || !info)
		return -EINVAL;

	if (!info->attrs[XNL_ATTR_QFLAG]) {
		snprintf(err, errlen, "Missing attribute 'XNL_ATTR_QFLAG'\n");
		goto respond_error;
	}
	f = nla_get_u32(info->attrs[XNL_ATTR_QFLAG]);
	if ((f & XNL_F_QMODE_ST) && (f & XNL_F_QMODE_MM)) {
		snprintf(err, errlen, "ERR! Both ST and MM mode set.\n");
		goto respond_error;
	} else if (!(f & XNL_F_QMODE_ST) && !(f & XNL_F_QMODE_MM)) {
		/* default to MM */
		f |= XNL_F_QMODE_MM;
	}

	if (!(f & XNL_F_QDIR_H2C) && !(f & XNL_F_QDIR_C2H)) {
		/* default to H2C */
		f |= XNL_F_QDIR_H2C;
	}

	memset(qconf, 0, sizeof(*qconf));
	qconf->st = (f & XNL_F_QMODE_ST) ? 1 : 0;
	qconf->c2h = (f & XNL_F_QDIR_C2H) ? 1 : 0;
	*is_qp = ((f & XNL_F_QDIR_BOTH) == XNL_F_QDIR_BOTH) ? 1 : 0;

	if (!info->attrs[XNL_ATTR_QIDX]) {
		snprintf(err, errlen, "Missing attribute 'XNL_ATTR_QIDX'");
		goto respond_error;
	}
	qconf->qidx = nla_get_u32(info->attrs[XNL_ATTR_QIDX]);
	if (qconf->qidx == XNL_QIDX_INVALID)
		qconf->qidx = QDMA_QUEUE_IDX_INVALID;

	return 0;

respond_error:

	xnl_respond_buffer(info, err, strlen(err));
	return -EINVAL;
}

static struct xlnx_qdata *xnl_rcv_check_qidx(struct genl_info *info,
				struct xlnx_pci_dev *xpdev,
				struct qdma_queue_conf *qconf, char *buf,
				int buflen)
{
	char ebuf[XNL_ERR_BUFLEN];
	struct xlnx_qdata *qdata = xpdev_queue_get(xpdev, qconf->qidx,
					qconf->c2h, 1, ebuf, XNL_ERR_BUFLEN);

	if (!qdata) {
		snprintf(ebuf, XNL_ERR_BUFLEN,
			"ERR! qidx %u invalid.\n", qconf->qidx);
		xnl_respond_buffer(info, ebuf, XNL_ERR_BUFLEN);
	}

	return qdata;
}

static int xnl_chk_attr(enum xnl_attr_t xnl_attr, struct genl_info *info,
				unsigned short qidx, char *buf)
{
	int rv = 0;

	if (!info->attrs[xnl_attr]) {
		if (buf) {
			rv += sprintf(buf,
				"Missing attribute %s for qidx = %u\n",
				xnl_attr_str[xnl_attr],
				qidx);
			buf[rv] = '\0';
		}
		rv = -1;
	}

	return rv;
}

static void xnl_extract_extra_config_attr(struct genl_info *info,
					struct qdma_queue_conf *qconf)
{
	u32 f = nla_get_u32(info->attrs[XNL_ATTR_QFLAG]);

	qconf->desc_bypass = (f & XNL_F_DESC_BYPASS_EN) ? 1 : 0;
	qconf->pfetch_bypass = (f & XNL_F_PFETCH_BYPASS_EN) ? 1 : 0;
	qconf->pfetch_en = (f & XNL_F_PFETCH_EN) ? 1 : 0;
	qconf->cmpl_status_en = (f & XNL_F_CMPL_STATUS_EN) ? 1 : 0;
	qconf->cmpl_status_acc_en = (f & XNL_F_CMPL_STATUS_ACC_EN) ? 1 : 0;
	qconf->cmpl_status_pend_chk = (f & XNL_F_CMPL_STATUS_PEND_CHK) ? 1 : 0;
	qconf->fetch_credit = (f & XNL_F_FETCH_CREDIT) ? 1 : 0;
	qconf->cmpl_stat_en = (f & XNL_F_CMPL_STATUS_DESC_EN) ? 1 : 0;
	qconf->cmpl_en_intr = (f & XNL_F_C2H_CMPL_INTR_EN) ? 1 : 0;
	qconf->cmpl_udd_en = (f & XNL_F_CMPL_UDD_EN) ? 1 : 0;
	qconf->cmpl_ovf_chk_dis = (f & XNL_F_CMPT_OVF_CHK_DIS) ? 1 : 0;

	if (xnl_chk_attr(XNL_ATTR_QRNGSZ_IDX, info, qconf->qidx, NULL) == 0)
		qconf->desc_rng_sz_idx = qconf->cmpl_rng_sz_idx =
				nla_get_u32(info->attrs[XNL_ATTR_QRNGSZ_IDX]);
	if (xnl_chk_attr(XNL_ATTR_C2H_BUFSZ_IDX, info, qconf->qidx, NULL) == 0)
		qconf->c2h_buf_sz_idx =
			nla_get_u32(info->attrs[XNL_ATTR_C2H_BUFSZ_IDX]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_TIMER_IDX, info, qconf->qidx, NULL) == 0)
		qconf->cmpl_timer_idx =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_TIMER_IDX]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_CNTR_IDX, info, qconf->qidx, NULL) == 0)
		qconf->cmpl_cnt_th_idx =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_CNTR_IDX]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_DESC_SIZE,
				info, qconf->qidx, NULL) == 0)
		qconf->cmpl_desc_sz =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_DESC_SIZE]);
	if (xnl_chk_attr(XNL_ATTR_SW_DESC_SIZE,
				info, qconf->qidx, NULL) == 0)
		qconf->sw_desc_sz =
			nla_get_u32(info->attrs[XNL_ATTR_SW_DESC_SIZE]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_TRIG_MODE, info, qconf->qidx, NULL) == 0)
		qconf->cmpl_trig_mode =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_TRIG_MODE]);
	else
		qconf->cmpl_trig_mode = 1;
	if (xnl_chk_attr(XNL_ATTR_PIPE_GL_MAX,
			 info, qconf->pipe_gl_max, NULL) == 0)
		qconf->pipe_gl_max =
			nla_get_u32(info->attrs[XNL_ATTR_PIPE_GL_MAX]);
	if (xnl_chk_attr(XNL_ATTR_PIPE_FLOW_ID,
				info, qconf->pipe_flow_id, NULL) == 0)
		qconf->pipe_flow_id =
			nla_get_u32(info->attrs[XNL_ATTR_PIPE_FLOW_ID]);
	if (xnl_chk_attr(XNL_ATTR_PIPE_SLR_ID,
				info, qconf->pipe_slr_id, NULL) == 0)
		qconf->pipe_slr_id =
			nla_get_u32(info->attrs[XNL_ATTR_PIPE_SLR_ID]);
	if (xnl_chk_attr(XNL_ATTR_PIPE_TDEST,
				info, qconf->pipe_tdest, NULL) == 0)
		qconf->pipe_tdest =
			nla_get_u32(info->attrs[XNL_ATTR_PIPE_TDEST]);
}

static int xnl_dev_list(struct sk_buff *skb2, struct genl_info *info)
{
	char *buf;
	int rv;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MAX, info);
	if (!buf)
		return -ENOMEM;

	rv = xpdev_list_dump(buf, XNL_RESP_BUFLEN_MAX);
	if (rv < 0) {
		pr_err("xpdev_list_dump() failed: %d", rv);
		goto free_msg_buff;
	}
	rv = xnl_respond_buffer(info, buf, strlen(buf));

free_msg_buff:
	kfree(buf);
	return rv;
}

static int xnl_dev_info(struct sk_buff *skb2, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct xlnx_pci_dev *xpdev;
	struct pci_dev *pdev;
	struct qdma_dev_conf conf;
	int rv;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;
	pdev = xpdev->pdev;

	rv = qdma_device_get_config(xpdev->dev_hndl, &conf, NULL, 0);
	if (rv < 0)
		return rv;

	skb = xnl_msg_alloc(XNL_CMD_DEV_INFO, 0, &hdr, info);
	if (!skb)
		return -ENOMEM;

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_PCI_BUS,
				pdev->bus->number);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_PCI_DEV,
				PCI_SLOT(pdev->devfn));
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_PCI_FUNC,
				PCI_FUNC(pdev->devfn));
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_CFG_BAR,
				conf.bar_num_config);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_USR_BAR,
				conf.bar_num_user);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_QSET_MAX, conf.qsets_max);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_QSET_QBASE, conf.qsets_base);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STM_BAR,
				   conf.bar_num_stm);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);

	return rv;
}

static int xnl_dev_version(struct sk_buff *skb2, struct genl_info *info)
{
    struct xlnx_pci_dev *xpdev;
    struct qdma_version_info ver_info;
    char *buf;
    int rv = 0;

    if (info == NULL)
        return -EINVAL;

    xnl_dump_attrs(info);

    xpdev = xnl_rcv_check_xpdev(info);
    if (!xpdev)
        return -EINVAL;

    buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MAX, info);
    if (!buf)
        return -ENOMEM;

    rv = qdma_device_version_info(xpdev->dev_hndl, &ver_info);
    if (rv < 0) {
        pr_err("qdma_device_version_info() failed: %d", rv);
        goto free_buf;
    }

    rv = sprintf(buf + rv,"=============Hardware Version============\n\n");
    rv += sprintf(buf + rv,"RTL Version       : %s\n",ver_info.rtl_version_str);
    rv += sprintf(buf + rv,"Vivado ReleaseID  : %s\n",ver_info.vivado_release_str);
    rv += sprintf(buf + rv,"Everest IP        : %s\n\n",ver_info.everest_ip_str);
    rv += sprintf(buf + rv,"============Software Version============\n\n");
    rv += sprintf(buf + rv,"qdma driver version       : %s\n",DRV_MODULE_VERSION);
    buf[rv] = '\0';

    rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX);

free_buf:
    kfree(buf);
    return rv;
}

static int xnl_dev_stat(struct sk_buff *skb2, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct xlnx_pci_dev *xpdev;
	int rv;
	unsigned long long mmh2c_pkts = 0;
	unsigned long long mmc2h_pkts = 0;
	unsigned long long sth2c_pkts = 0;
	unsigned long long stc2h_pkts = 0;
	unsigned int pkts;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;

	skb = xnl_msg_alloc(XNL_CMD_DEV_STAT, 0, &hdr, info);
	if (!skb)
		return -ENOMEM;

	qdma_device_get_mmh2c_pkts(xpdev->dev_hndl, &mmh2c_pkts);
	qdma_device_get_mmc2h_pkts(xpdev->dev_hndl, &mmc2h_pkts);
	qdma_device_get_sth2c_pkts(xpdev->dev_hndl, &sth2c_pkts);
	qdma_device_get_stc2h_pkts(xpdev->dev_hndl, &stc2h_pkts);
	pkts = mmh2c_pkts;
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_MMH2C_PKTS1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (mmh2c_pkts >> 32);
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_MMH2C_PKTS2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = mmc2h_pkts;
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_MMC2H_PKTS1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (mmc2h_pkts >> 32);
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_MMC2H_PKTS2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = sth2c_pkts;
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_STH2C_PKTS1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (sth2c_pkts >> 32);
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_STH2C_PKTS2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = stc2h_pkts;
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_STC2H_PKTS1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (stc2h_pkts >> 32);
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_STAT_STC2H_PKTS2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	rv = xnl_msg_send(skb, hdr, info);

	return rv;
}

static int xnl_dev_stat_clear(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	int rv;
	char *buf;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;
	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MIN, info);
	if (!buf)
		return -ENOMEM;

	qdma_device_clear_stats(xpdev->dev_hndl);

	buf[0] = '\0';
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX);

	kfree(buf);
	return rv;
}

static int xnl_q_list(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	char *buf;
	int rv = 0;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;

	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MAX, info);
	if (!buf)
		return -ENOMEM;
	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs");
		goto send_rsp;
	}

	rv = qdma_queue_list(xpdev->dev_hndl, buf, XNL_RESP_BUFLEN_MAX);
	if (rv < 0) {
		pr_err("qdma_queue_list() failed: %d", rv);
		goto free_buf;
	}

send_rsp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX);

free_buf:
	kfree(buf);
	return rv;
}

static int xnl_q_add(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev = NULL;
	struct qdma_queue_conf qconf;
	char *buf, *cur, *end;
	int rv = 0;
	int rv2 = 0;
	unsigned char is_qp;
	unsigned int num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;
	int buf_len = XNL_RESP_BUFLEN_MAX;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;
	cur = buf;
	end = buf + buf_len;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		pr_info("0 sized Qs\n");
		rv += snprintf(cur, end - cur, "Zero Qs");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, cur, end - cur, &is_qp);
	if (rv < 0)
		goto free_buf;


	qidx = qconf.qidx;

	rv = xnl_chk_attr(XNL_ATTR_NUM_Q, info, qidx, cur);
	if (rv < 0)
		goto send_resp;
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	is_c2h = qconf.c2h;
	for (i = 0; i < num_q; i++) {
		qconf.c2h = is_c2h;
add_q:
		if (qidx != QDMA_QUEUE_IDX_INVALID)
			qconf.qidx = qidx + i;
		rv = xpdev_queue_add(xpdev, &qconf, cur, end - cur);
		if (rv < 0) {
			pr_err("xpdev_queue_add() failed: %d\n", rv);
			goto send_resp;
		}
		cur = buf + strlen(buf);
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto add_q;
		}
	}
	cur += snprintf(cur, end - cur, "Added %d Queues.\n", i);

send_resp:
	rv2 = xnl_respond_buffer(info, buf, strlen(buf));
free_buf:
	kfree(buf);
	return rv < 0 ? rv : rv2;
}

static int xnl_q_start(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	struct xlnx_qdata *qdata;
	int rv = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	qidx = qconf.qidx;

	rv = xnl_chk_attr(XNL_ATTR_NUM_Q, info, qidx, buf);
	if (rv < 0)
		goto send_resp;
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	xnl_extract_extra_config_attr(info, &qconf);
	is_c2h = qconf.c2h;

	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
reconfig:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
					XNL_RESP_BUFLEN_MIN);
		if (!qdata)
			goto send_resp;

		rv = qdma_queue_reconfig(xpdev->dev_hndl, qdata->qhndl, &qconf,
						buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0) {
			pr_err("qdma_queue_reconfig() failed: %d", rv);
			goto send_resp;
		}
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto reconfig;
		}
	}

	rv = xpdev_nl_queue_start(xpdev, info, is_qp, is_c2h, qidx, num_q);
	if (rv < 0) {
		snprintf(buf, XNL_RESP_BUFLEN_MIN, "qdma%05x OOM.\n",
			xpdev->idx);
		goto send_resp;
	}

	return 0;

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN);

	return rv;
}

static int xnl_q_stop(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	struct xlnx_qdata *qdata;
	int rv = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	is_c2h = qconf.c2h;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
stop_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
					XNL_RESP_BUFLEN_MIN);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_stop(xpdev->dev_hndl, qdata->qhndl,
				     buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0) {
			pr_err("qdma_queue_stop() failed: %d", rv);
			goto send_resp;
		}
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto stop_q;
		}
	}
	rv += sprintf(buf + rv, "Stopped Queues %d -> %d.\n", qidx, i - 1);
send_resp:
	buf[rv] = '\0';
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN);
	return rv;
}

static int xnl_q_del(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	int rv = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	is_c2h = qconf.c2h;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
del_q:
		qconf.qidx = i;
		rv = xpdev_queue_delete(xpdev, qconf.qidx, qconf.c2h,
					buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0) {
			pr_err("xpdev_queue_delete() failed: %d", rv);
			goto send_resp;
		}
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto del_q;
		}
	}
	rv += sprintf(buf + rv, "Deleted Queues %d -> %d.\n", qidx, i - 1);
send_resp:
	buf[rv] = '\0';
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN);
	return rv;
}

static int xnl_q_dump(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct xlnx_qdata *qdata;
	char *buf;
	char ebuf[XNL_RESP_BUFLEN_MIN];
	int rv;
	unsigned char is_qp;
	unsigned int num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	unsigned int buf_idx = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	rv = qconf_get(&qconf, info, ebuf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		return rv;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);
	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		kfree(buf);
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	is_c2h = qconf.c2h;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump(xpdev->dev_hndl, qdata->qhndl,
				     buf + buf_idx,
				     buf_len - buf_idx);
		if (rv < 0) {
			pr_err("qdma_queue_dump() failed: %d", rv);
			goto send_resp;
		}
		buf_idx = strlen(buf);
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto dump_q;
		}
	}
	snprintf(buf + buf_idx, buf_len - buf_idx,
		"Dumped Queues %d -> %d.\n", qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
}

static int xnl_q_dump_desc(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct xlnx_qdata *qdata;
	u32 v1;
	u32 v2;
	char *buf;
	char ebuf[XNL_RESP_BUFLEN_MIN];
	int rv;
	unsigned char is_qp;
	unsigned int num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	unsigned int buf_idx = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	v1 = nla_get_u32(info->attrs[XNL_ATTR_RANGE_START]);
	v2 = nla_get_u32(info->attrs[XNL_ATTR_RANGE_END]);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl)))
		return 0;
	rv = qconf_get(&qconf, info, ebuf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		return rv;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf) {
		rv = sprintf(ebuf, "%s OOM %d.\n",
				__func__, buf_len);

		ebuf[rv] = '\0';
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN);
		return -ENOMEM;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	is_c2h = qconf.c2h;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump_desc(xpdev->dev_hndl,
					qdata->qhndl, v1, v2,
					buf + buf_idx, buf_len - buf_idx);
		if (rv < 0) {
			pr_err("qdma_queue_dump_desc() failed: %d", rv);
			goto send_resp;
		}
		buf_idx = strlen(buf);
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto dump_q;
		}
	}
	snprintf(buf + buf_idx, buf_len - buf_idx,
		"Dumped descs of queues %d -> %d.\n",
		qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
}

static int xnl_q_dump_cmpt(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct xlnx_qdata *qdata;
	u32 v1;
	u32 v2;
	char *buf;
	char ebuf[XNL_RESP_BUFLEN_MIN];
	int rv;
	unsigned char is_qp;
	unsigned int num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char is_c2h;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	unsigned int buf_idx = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	v1 = nla_get_u32(info->attrs[XNL_ATTR_RANGE_START]);
	v2 = nla_get_u32(info->attrs[XNL_ATTR_RANGE_END]);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl)))
		return 0;

	rv = qconf_get(&qconf, info, ebuf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		return rv;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf) {
		rv = sprintf(ebuf, "%s OOM %d.\n",
				__func__, buf_len);
		ebuf[rv] = '\0';
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN);
		return -ENOMEM;
	}
	pr_info("response buf_len = %d", buf_len);

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	is_c2h = qconf.c2h;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = is_c2h;
dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump_cmpt(xpdev->dev_hndl,
					qdata->qhndl, v1, v2,
					buf + buf_idx, buf_len - buf_idx);
		if (rv < 0) {
			pr_err("qdma_queue_dump_cmpt() failed: %d", rv);
			goto send_resp;
		}
		buf_idx = strlen(buf);
		if (is_qp && (is_c2h == qconf.c2h)) {
			qconf.c2h = ~qconf.c2h;
			goto dump_q;
		}
	}
	snprintf(buf + buf_idx, buf_len - buf_idx,
		"Dumped descs of queues %d -> %d.\n",
		qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
}

static int xnl_q_read_udd(struct sk_buff *skb2, struct genl_info *info)
{
	int rv = 0;
	struct qdma_queue_conf qconf;
	char *buf;
	unsigned char is_qp;
	struct xlnx_pci_dev *xpdev;
	struct xlnx_qdata *qdata;
	int buf_len = XNL_RESP_BUFLEN_MAX;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MAX, info);
	if (!buf)
		return -ENOMEM;

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev) {
		kfree(buf);
		return -EINVAL;
	}

	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MAX, &is_qp);
	if (rv < 0)
		goto send_resp;

	qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
			XNL_RESP_BUFLEN_MAX);
	if (!qdata)
		goto send_resp;

	rv = qdma_descq_get_cmpt_udd(xpdev->dev_hndl, qdata->qhndl,  buf,
			XNL_RESP_BUFLEN_MAX);
	if (rv < 0)
		goto send_resp;

send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
}

#ifdef ERR_DEBUG
static int xnl_err_induce(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct xlnx_qdata *qdata;
	char *buf;
	char ebuf[XNL_RESP_BUFLEN_MIN];
	unsigned char is_qp;
	int rv;
	u32 err;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;
	rv = qconf_get(&qconf, info, ebuf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		return rv;

	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MAX, info);
	if (!buf) {
		rv = sprintf(ebuf, "%s OOM %d.\n",
				__func__, XNL_RESP_BUFLEN_MAX);
		ebuf[rv] = '\0';
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN);
		return -ENOMEM;
	}

	qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
				XNL_RESP_BUFLEN_MIN);
	if (!qdata)
		goto send_resp;
	err = nla_get_u32(info->attrs[XNL_ATTR_QPARAM_ERR_INFO]);

	if (qdma_queue_set_err_induction(xpdev->dev_hndl, qdata->qhndl, err,
					 buf, XNL_RESP_BUFLEN_MAX)) {
		rv += sprintf(buf + rv, "Failed to set induce err\n");
		goto send_resp;
	}
	rv += sprintf(buf + rv, "queue error induced\n");
	buf[rv] = '\0';

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX);

	kfree(buf);
	return rv;
}
#endif

static int xnl_q_read_pkt(struct sk_buff *skb2, struct genl_info *info)
{
#if 0
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct xlnx_qdata *qdata;
	char *buf;
	char ebuf[XNL_RESP_BUFLEN_MIN];
	int rv;
	unsigned char is_qp;
	unsigned int num_q;
	unsigned int i;
	unsigned short qidx;
	int buf_len = XNL_RESP_BUFLEN_MAX;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	rv = qconf_get(&qconf, info, ebuf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		return rv;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf) {
		rv = sprintf(ebuf, "%s OOM %d.\n",
				__func__, buf_len);
		ebuf[rv] = '\0';
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN);
		return -ENOMEM;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.c2h = 1;
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
						buf_len);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump_rx_packet(xpdev->dev_hndl, qdata->qhndl,
						buf, buf_len);
		if (rv < 0) {
			pr_err("qdma_queue_dump_rx_packet) failed: %d", rv);
			goto send_resp;
		}
	}
	buf[rv] = '\0';
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
#endif
	pr_info("NOT supported.\n");
	return -EINVAL;
}

static int xnl_intr_ring_dump(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	char *buf;
	unsigned int vector_idx = 0;
	int start_idx = 0, end_idx = 0;
	int rv = 0;
	int buf_len = XNL_RESP_BUFLEN_MAX;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (!info->attrs[XNL_ATTR_INTR_VECTOR_IDX]) {
		pr_warn("Missing attribute 'XNL_ATTR_INTR_VECTOR_IDX'");
		return -1;
	}
	vector_idx = nla_get_u32(info->attrs[XNL_ATTR_INTR_VECTOR_IDX]);
	start_idx = nla_get_u32(info->attrs[XNL_ATTR_INTR_VECTOR_START_IDX]);
	end_idx = nla_get_u32(info->attrs[XNL_ATTR_INTR_VECTOR_END_IDX]);

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;

	if (xpdev->idx == 0) {
		if (vector_idx == 0) {
			rv += sprintf(buf + rv,
				"vector_idx %d is for error interrupt\n",
				vector_idx);
			buf[rv] = '\0';
			goto send_resp;
		} else if (vector_idx == 1) {
			rv += sprintf(buf + rv,
				"vector_idx %d is for user interrupt\n",
				vector_idx);
			buf[rv] = '\0';
			goto send_resp;
		}
	} else {
		if (vector_idx == 0) {
			rv += sprintf(buf + rv,
				"vector_idx %d is for user interrupt\n",
				vector_idx);
			buf[rv] = '\0';
			goto send_resp;
		}
	}

	rv = qdma_intr_ring_dump(xpdev->dev_hndl,
					vector_idx, start_idx,
					end_idx, buf, buf_len);
	if (rv < 0) {
		pr_err("qdma_intr_ring_dump() failed: %d", rv);
		goto send_resp;
	}

send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len);

	kfree(buf);
	return rv;
}

static int xnl_register_read(struct sk_buff *skb2, struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct xlnx_pci_dev *xpdev;
	struct qdma_dev_conf conf;
	char buf[XNL_RESP_BUFLEN_MIN];
	unsigned int bar_num = 0, reg_addr = 0;
	uint32_t reg_val = 0;
	int rv = 0, err = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	rv = qdma_device_get_config(xpdev->dev_hndl, &conf, NULL, 0);
	if (rv < 0)
		return rv;

	skb = xnl_msg_alloc(XNL_CMD_REG_RD, 0, &hdr, info);
	if (!skb)
		return -ENOMEM;

	if (!info->attrs[XNL_ATTR_REG_BAR_NUM]) {
		pr_warn("Missing attribute 'XNL_ATTR_REG_BAR_NUM'");
		return -EINVAL;
	}

	if (!info->attrs[XNL_ATTR_REG_ADDR]) {
		pr_warn("Missing attribute 'XNL_ATTR_REG_ADDR'");
		return -EINVAL;
	}

	bar_num = nla_get_u32(info->attrs[XNL_ATTR_REG_BAR_NUM]);
	reg_addr = nla_get_u32(info->attrs[XNL_ATTR_REG_ADDR]);

	if (bar_num == conf.bar_num_config) {
		reg_val = qdma_device_read_config_register(xpdev->dev_hndl,
				reg_addr);
	} else if (bar_num == conf.bar_num_user) {
		reg_val = qdma_device_read_user_register(xpdev, reg_addr);
	} else if (bar_num == conf.bar_num_bypass) {
		reg_val = qdma_device_read_bypass_register(xpdev, reg_addr);
	} else {
		rv += sprintf(buf + rv, "Invalid bar number\n");
		buf[rv] = '\0';
		goto send_resp;
	}

	err = xnl_msg_add_attr_uint(skb, XNL_ATTR_REG_VAL, reg_val);
	if (err < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return err;
	}

	err = xnl_msg_send(skb, hdr, info);
	return err;

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN);
	nlmsg_free(skb);
	return rv;
}

static int xnl_register_write(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_dev_conf conf;
	char buf[XNL_RESP_BUFLEN_MIN];
	unsigned int bar_num = 0, reg_addr = 0;
	uint32_t reg_val = 0;
	int rv = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	rv = qdma_device_get_config(xpdev->dev_hndl, &conf, NULL, 0);
	if (rv < 0)
		return rv;

	if (!info->attrs[XNL_ATTR_REG_BAR_NUM]) {
		pr_warn("Missing attribute 'XNL_ATTR_REG_BAR_NUM'");
		return -EINVAL;
	}

	if (!info->attrs[XNL_ATTR_REG_ADDR]) {
		pr_warn("Missing attribute 'XNL_ATTR_REG_ADDR'");
		return -EINVAL;
	}

	if (!info->attrs[XNL_ATTR_REG_VAL]) {
		pr_warn("Missing attribute 'XNL_ATTR_REG_VAL'");
		return -EINVAL;
	}

	bar_num = nla_get_u32(info->attrs[XNL_ATTR_REG_BAR_NUM]);
	reg_addr = nla_get_u32(info->attrs[XNL_ATTR_REG_ADDR]);
	reg_val = nla_get_u32(info->attrs[XNL_ATTR_REG_VAL]);

	if (bar_num == conf.bar_num_config) {
		qdma_device_write_config_register(xpdev->dev_hndl,
				reg_addr, reg_val);
	} else if (bar_num == conf.bar_num_user) {
		qdma_device_write_user_register(xpdev, reg_addr, reg_val);
	} else if (bar_num == conf.bar_num_bypass) {
		qdma_device_write_bypass_register(xpdev, reg_addr, reg_val);
	} else {
		rv += sprintf(buf + rv, "Invalid bar number\n");
		buf[rv] = '\0';
		goto send_resp;
	}

	buf[rv] = '\0';

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN);
	return rv;
}
static int xnl_get_global_csr(struct sk_buff *skb2, struct genl_info *info)
{
	struct global_csr_conf *csr;
	struct xlnx_pci_dev *xpdev;
	int rv;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	csr = kmalloc(sizeof(struct global_csr_conf), GFP_KERNEL);
	if (!csr)
		return -ENOMEM;

	rv = qdma_global_csr_get(xpdev->dev_hndl, csr);
	if (rv < 0) {
		pr_err("qdma_global_csr_get() failed: %d", rv);
		goto free_msg_buff;
	}

	rv = xnl_respond_data(info,
		(void *)csr, sizeof(struct global_csr_conf));

free_msg_buff:
	kfree(csr);
	return rv;
}

int xlnx_nl_init(void)
{
	int rv;
#ifdef __GENL_REG_FAMILY_OPS_FUNC__
	rv = genl_register_family_with_ops(&xnl_family,
										xnl_ops,
										ARRAY_SIZE(xnl_ops));
#else
	rv = genl_register_family(&xnl_family);
#endif
	if (rv)
		pr_info("genl_register_family failed %d.\n", rv);

	return rv;
}

void  xlnx_nl_exit(void)
{
	int rv;

	rv = genl_unregister_family(&xnl_family);
	if (rv)
		pr_info("genl_unregister_family failed %d.\n", rv);
}
