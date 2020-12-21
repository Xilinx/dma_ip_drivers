/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
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

#define QDMA_C2H_DEFAULT_BUF_SZ (4096)
#define DUMP_LINE_SZ			(81)
#define QDMA_Q_DUMP_MAX_QUEUES	(100)
#define QDMA_Q_DUMP_LINE_SZ	(24 * 1024)
#define QDMA_Q_LIST_LINE_SZ	(200)

static int xnl_dev_list(struct sk_buff *skb2, struct genl_info *info);

#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
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

	[XNL_ATTR_VERSION_INFO] =	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_DEVICE_TYPE]	=	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_IP_TYPE]	=	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_DEV_NUMQS] =          { .type = NLA_U32 },
	[XNL_ATTR_DEV_NUM_PFS] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_CHANNEL_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MAILBOX_ENABLE] = { .type = NLA_U32 },
	[XNL_ATTR_DEV_FLR_PRESENT] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_ST_ENABLE] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_ENABLE] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_CMPT_ENABLE] =	{ .type = NLA_U32 },

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
	[XNL_ATTR_MM_CHANNEL] =		{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_TRIG_MODE] =	{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_ENTRIES_CNT] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_START] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_END] =		{ .type = NLA_U32 },

	[XNL_ATTR_INTR_VECTOR_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_GL_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_FLOW_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_SLR_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_TDEST] =		{ .type = NLA_U32 },

	[XNL_ATTR_DEV_STM_BAR] =	{ .type = NLA_U32 },
	[XNL_ATTR_Q_STATE]   =		{ .type = NLA_U32 },
	[XNL_ATTR_ERROR]   =		{ .type = NLA_U32 },
	[XNL_ATTR_PING_PONG_EN]   =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV]		=	{ .type = NLA_BINARY,
					  .len = QDMA_DEV_ATTR_STRUCT_SIZE, },
	[XNL_ATTR_GLOBAL_CSR]		=	{ .type = NLA_BINARY,
				.len = QDMA_DEV_GLOBAL_CSR_STRUCT_SIZE, },
#ifdef ERR_DEBUG
	[XNL_ATTR_QPARAM_ERR_INFO] =    { .type = NLA_U32 },
#endif
};
#endif
#else
#if KERNEL_VERSION(5, 2, 0) >= LINUX_VERSION_CODE
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

	[XNL_ATTR_VERSION_INFO] =	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_DEVICE_TYPE]	=	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_IP_TYPE]	=	{ .type = NLA_NUL_STRING },
	[XNL_ATTR_DEV_NUMQS] =          { .type = NLA_U32 },
	[XNL_ATTR_DEV_NUM_PFS] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_CHANNEL_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MAILBOX_ENABLE] = { .type = NLA_U32 },
	[XNL_ATTR_DEV_FLR_PRESENT] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_ST_ENABLE] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_ENABLE] =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV_MM_CMPT_ENABLE] =	{ .type = NLA_U32 },

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
	[XNL_ATTR_MM_CHANNEL] =		{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_TRIG_MODE] =	{ .type = NLA_U32 },
	[XNL_ATTR_CMPT_ENTRIES_CNT] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_START] =	{ .type = NLA_U32 },
	[XNL_ATTR_RANGE_END] =		{ .type = NLA_U32 },

	[XNL_ATTR_INTR_VECTOR_IDX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_GL_MAX] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_FLOW_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_SLR_ID] =	{ .type = NLA_U32 },
	[XNL_ATTR_PIPE_TDEST] =		{ .type = NLA_U32 },

	[XNL_ATTR_DEV_STM_BAR] =	{ .type = NLA_U32 },
	[XNL_ATTR_Q_STATE]   =		{ .type = NLA_U32 },
	[XNL_ATTR_ERROR]   =		{ .type = NLA_U32 },
	[XNL_ATTR_PING_PONG_EN]   =	{ .type = NLA_U32 },
	[XNL_ATTR_APERTURE_SZ]   =	{ .type = NLA_U32 },
	[XNL_ATTR_DEV]		=	{ .type = NLA_BINARY,
					  .len = QDMA_DEV_ATTR_STRUCT_SIZE, },
	[XNL_ATTR_GLOBAL_CSR]		=	{ .type = NLA_BINARY,
				.len = QDMA_DEV_GLOBAL_CSR_STRUCT_SIZE, },
#ifdef ERR_DEBUG
	[XNL_ATTR_QPARAM_ERR_INFO] =    { .type = NLA_U32 },
#endif
};
#endif
#endif

static int xnl_respond_buffer_cmpt(struct genl_info *info, char *buf,
		int buflen, int error, long int cmpt_entries);

static int xnl_dev_info(struct sk_buff *, struct genl_info *);
static int xnl_dev_version_capabilities(struct sk_buff *skb2,
		struct genl_info *info);
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
static int xnl_config_reg_dump(struct sk_buff *, struct genl_info *);
static int xnl_q_read_pkt(struct sk_buff *, struct genl_info *);
static int xnl_q_read_udd(struct sk_buff *, struct genl_info *);
static int xnl_q_cmpt_read(struct sk_buff *, struct genl_info *);
static int xnl_intr_ring_dump(struct sk_buff *, struct genl_info *);
static int xnl_register_read(struct sk_buff *, struct genl_info *);
static int xnl_register_write(struct sk_buff *, struct genl_info *);
static int xnl_get_global_csr(struct sk_buff *skb2, struct genl_info *info);
static int xnl_get_queue_state(struct sk_buff *, struct genl_info *);
static int xnl_config_reg_info_dump(struct sk_buff *, struct genl_info *);
#ifdef ERR_DEBUG
static int xnl_err_induce(struct sk_buff *skb2, struct genl_info *info);
#endif

static struct genl_ops xnl_ops[] = {
	{
		.cmd = XNL_CMD_DEV_LIST,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_dev_list,
	},
	{
		.cmd = XNL_CMD_DEV_CAP,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_dev_version_capabilities,
	},
	{
		.cmd = XNL_CMD_DEV_INFO,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_dev_info,
	},
	{
		.cmd = XNL_CMD_DEV_STAT,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_dev_stat,
	},
	{
		.cmd = XNL_CMD_DEV_STAT_CLEAR,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_dev_stat_clear,
	},
	{
		.cmd = XNL_CMD_Q_LIST,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_list,
	},
	{
		.cmd = XNL_CMD_Q_ADD,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_add,
	},
	{
		.cmd = XNL_CMD_Q_START,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_start,
	},
	{
		.cmd = XNL_CMD_Q_STOP,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_stop,
	},
	{
		.cmd = XNL_CMD_Q_DEL,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_del,
	},
	{
		.cmd = XNL_CMD_Q_DUMP,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_dump,
	},
	{
		.cmd = XNL_CMD_Q_DESC,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_dump_desc,
	},
	{
		.cmd = XNL_CMD_REG_DUMP,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_config_reg_dump,
	},
	{
		.cmd = XNL_CMD_REG_INFO_READ,
	#ifdef RHEL_RELEASE_VERSION
	#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
	#endif
	#else
	#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
	#endif
	#endif
		.doit = xnl_config_reg_info_dump,
	},
	{
		.cmd = XNL_CMD_Q_CMPT,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_dump_cmpt,
	},
	{
		.cmd = XNL_CMD_Q_UDD,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_read_udd,
	},
	{
		.cmd = XNL_CMD_Q_RX_PKT,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_read_pkt,
	},
	{
		.cmd = XNL_CMD_Q_CMPT_READ,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_q_cmpt_read,
	},
	{
		.cmd = XNL_CMD_INTR_RING_DUMP,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_intr_ring_dump,
	},
	{
		.cmd = XNL_CMD_REG_RD,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_register_read,
	},
	{
		.cmd = XNL_CMD_REG_WRT,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_register_write,
	},
	{
		.cmd = XNL_CMD_GLOBAL_CSR,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_get_global_csr,
	},
	{
		.cmd = XNL_CMD_GET_Q_STATE,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
		.doit = xnl_get_queue_state,
	},
#ifdef ERR_DEBUG
	{
		.cmd = XNL_CMD_Q_ERR_INDUCE,
#ifdef RHEL_RELEASE_VERSION
#if RHEL_RELEASE_VERSION(8, 3) > RHEL_RELEASE_CODE
		.policy = xnl_policy,
#endif
#else
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
		.policy = xnl_policy,
#endif
#endif
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
		pr_err("failed to allocate skb %lu.\n", sz);
		return NULL;
	}

	p = genlmsg_put(skb, 0, info->snd_seq + 1, &xnl_family, 0, op);
	if (!p) {
		pr_err("skb too small.\n");
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
		pr_err("nla_put_string return %d.\n", rv);
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
		pr_err("nla_put return %d.\n", rv);
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
		pr_err("nla add dev_idx failed %d.\n", rv);
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
		pr_err("send portid %d failed %d.\n", info->snd_portid, rv);

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
	pr_info("\n"); {
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
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE
			if (xnl_policy[i].type == NLA_NUL_STRING) {
#else
			if (1) {
#endif

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

static int xnl_respond_buffer_cmpt(struct genl_info *info, char *buf,
		int buflen, int error, long int cmpt_entries)
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

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_ERROR, error);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_CMPT_ENTRIES_CNT,
			cmpt_entries);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);

	return rv;
}

int xnl_respond_buffer(struct genl_info *info, char *buf, int buflen, int error)
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

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_ERROR, error);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		nlmsg_free(skb);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);

	return rv;
}

static int xnl_respond_data(struct genl_info *info, void *buf, int buflen)
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
	int rv;

	if (buf) {
		memset(buf, 0, l);
		return buf;
	}

	pr_err("xnl OOM %d.\n", l);

	rv = snprintf(ebuf, XNL_ERR_BUFLEN, "ERR! xnl OOM %d.\n", l);

	xnl_respond_buffer(info, ebuf, XNL_ERR_BUFLEN, rv);

	return NULL;
}

static struct xlnx_pci_dev *xnl_rcv_check_xpdev(struct genl_info *info)
{
	u32 idx;
	struct xlnx_pci_dev *xpdev;
	char err[XNL_ERR_BUFLEN];
	int rv = 0;

	if (info == NULL)
		return NULL;

	if (!info->attrs[XNL_ATTR_DEV_IDX]) {
		snprintf(err, sizeof(err),
			"command %s missing attribute XNL_ATTR_DEV_IDX",
			xnl_op_str[info->genlhdr->cmd]);
		rv = -EINVAL;
		goto respond_error;
	}

	idx = nla_get_u32(info->attrs[XNL_ATTR_DEV_IDX]);

	xpdev = xpdev_find_by_idx(idx, err, sizeof(err));
	if (!xpdev) {
		rv = -EINVAL;
		/* err buffer populated by xpdev_find_by_idx*/
		goto respond_error;
	}

	return xpdev;

respond_error:
	xnl_respond_buffer(info, err, strlen(err), rv);
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

	if (!(f & XNL_F_QDIR_H2C) &&
			!(f & XNL_F_QDIR_C2H) && !(f & XNL_F_Q_CMPL)) {
		/* default to H2C */
		f |= XNL_F_QDIR_H2C;
	}

	memset(qconf, 0, sizeof(*qconf));
	qconf->st = (f & XNL_F_QMODE_ST) ? 1 : 0;

	if (f & XNL_F_QDIR_H2C)
		qconf->q_type = Q_H2C;
	else if (f & XNL_F_QDIR_C2H)
		qconf->q_type = Q_C2H;
	else
		qconf->q_type = Q_CMPT;

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

	xnl_respond_buffer(info, err, strlen(err), 0);
	return -EINVAL;
}

static struct xlnx_qdata *xnl_rcv_check_qidx(struct genl_info *info,
				struct xlnx_pci_dev *xpdev,
				struct qdma_queue_conf *qconf, char *buf,
				int buflen)
{
	char ebuf[XNL_ERR_BUFLEN];
	struct xlnx_qdata *qdata = xpdev_queue_get(xpdev, qconf->qidx,
					qconf->q_type, 1, ebuf, XNL_ERR_BUFLEN);

	if (!qdata) {
		snprintf(ebuf,
			XNL_ERR_BUFLEN,
			"ERR! qidx %u invalid.\n",
			qconf->qidx);
		xnl_respond_buffer(info, ebuf, XNL_ERR_BUFLEN, 0);
	}

	return qdata;
}

static int xnl_chk_attr(enum xnl_attr_t xnl_attr, struct genl_info *info,
				unsigned short qidx, char *buf, int buflen)
{
	int rv = 0;

	if (!info->attrs[xnl_attr]) {
		if (buf) {
			rv += snprintf(buf, buflen,
				"Missing attribute %s for qidx = %u\n",
				xnl_attr_str[xnl_attr],
				qidx);
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
	qconf->wb_status_en = (f & XNL_F_CMPL_STATUS_EN) ? 1 : 0;
	qconf->cmpl_status_acc_en = (f & XNL_F_CMPL_STATUS_ACC_EN) ? 1 : 0;
	qconf->cmpl_status_pend_chk = (f & XNL_F_CMPL_STATUS_PEND_CHK) ? 1 : 0;
	qconf->fetch_credit = (f & XNL_F_FETCH_CREDIT) ? 1 : 0;
	qconf->cmpl_stat_en = (f & XNL_F_CMPL_STATUS_DESC_EN) ? 1 : 0;
	qconf->cmpl_en_intr = (f & XNL_F_C2H_CMPL_INTR_EN) ? 1 : 0;
	qconf->cmpl_udd_en = (f & XNL_F_CMPL_UDD_EN) ? 1 : 0;
	qconf->cmpl_ovf_chk_dis = (f & XNL_F_CMPT_OVF_CHK_DIS) ? 1 : 0;

	if (qconf->q_type == Q_CMPT)
		qconf->cmpl_udd_en = 1;

	if (xnl_chk_attr(XNL_ATTR_QRNGSZ_IDX, info, qconf->qidx, NULL, 0) == 0)
		qconf->desc_rng_sz_idx = qconf->cmpl_rng_sz_idx =
				nla_get_u32(info->attrs[XNL_ATTR_QRNGSZ_IDX]);
	if (xnl_chk_attr(XNL_ATTR_C2H_BUFSZ_IDX, info,
			qconf->qidx, NULL, 0) == 0)
		qconf->c2h_buf_sz_idx =
			nla_get_u32(info->attrs[XNL_ATTR_C2H_BUFSZ_IDX]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_TIMER_IDX, info,
			qconf->qidx, NULL, 0) == 0)
		qconf->cmpl_timer_idx =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_TIMER_IDX]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_CNTR_IDX, info,
			qconf->qidx, NULL, 0) == 0)
		qconf->cmpl_cnt_th_idx =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_CNTR_IDX]);
	if (xnl_chk_attr(XNL_ATTR_MM_CHANNEL, info, qconf->qidx, NULL, 0) == 0)
		qconf->mm_channel =
			nla_get_u32(info->attrs[XNL_ATTR_MM_CHANNEL]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_DESC_SIZE,
				info, qconf->qidx, NULL, 0) == 0)
		qconf->cmpl_desc_sz =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_DESC_SIZE]);
	if (xnl_chk_attr(XNL_ATTR_SW_DESC_SIZE,
				info, qconf->qidx, NULL, 0) == 0)
		qconf->sw_desc_sz =
			nla_get_u32(info->attrs[XNL_ATTR_SW_DESC_SIZE]);
	if (xnl_chk_attr(XNL_ATTR_PING_PONG_EN,
					 info, qconf->qidx, NULL, 0) == 0)
		qconf->ping_pong_en = 1;
	if (xnl_chk_attr(XNL_ATTR_APERTURE_SZ,
					 info, qconf->qidx, NULL, 0) == 0)
		qconf->aperture_size =
			nla_get_u32(info->attrs[XNL_ATTR_APERTURE_SZ]);
	if (xnl_chk_attr(XNL_ATTR_CMPT_TRIG_MODE, info,
				qconf->qidx, NULL, 0) == 0)
		qconf->cmpl_trig_mode =
			nla_get_u32(info->attrs[XNL_ATTR_CMPT_TRIG_MODE]);
	else
		qconf->cmpl_trig_mode = 1;
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

	rv = xnl_respond_buffer(info, buf, strlen(buf), rv);

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
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_PCI_DEV,
				PCI_SLOT(pdev->devfn));
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_PCI_FUNC,
				PCI_FUNC(pdev->devfn));
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_CFG_BAR,
				conf.bar_num_config);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_USR_BAR,
				conf.bar_num_user);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_QSET_MAX, conf.qsets_max);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}
	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_QSET_QBASE,
			conf.qsets_base);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_send(skb, hdr, info);

	return rv;

free_skb:
	nlmsg_free(skb);
	return rv;
}

static int xnl_dev_version_capabilities(struct sk_buff *skb2,
		struct genl_info *info)
{
	struct sk_buff *skb;
	void *hdr;
	struct xlnx_pci_dev *xpdev;
	struct qdma_version_info ver_info;
	struct qdma_dev_attributes dev_attr;
	char buf[XNL_RESP_BUFLEN_MIN];
	int buflen = XNL_RESP_BUFLEN_MIN;
	int rv = 0;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;

	skb = xnl_msg_alloc(XNL_CMD_DEV_CAP, 0, &hdr, info);
	if (!skb)
		return -ENOMEM;

	rv = qdma_device_version_info(xpdev->dev_hndl, &ver_info);
	if (rv < 0) {
		pr_err("qdma_device_version_info() failed: %d", rv);
		goto free_skb;
	}

	rv = qdma_device_capabilities_info(xpdev->dev_hndl, &dev_attr);
	if (rv < 0) {
		pr_err("qdma_device_capabilities_info() failed: %d", rv);
		goto free_skb;
	}

	rv = snprintf(buf + rv, buflen,
			"=============Hardware Version============\n\n");
	rv += snprintf(buf + rv, buflen - rv,
			"RTL Version         : %s\n", ver_info.rtl_version_str);
	rv += snprintf(buf + rv,
			buflen - rv,
			"Vivado ReleaseID    : %s\n",
			ver_info.vivado_release_str);
	rv += snprintf(buf + rv,
			buflen - rv,
			"QDMA Device Type    : %s\n",
			ver_info.device_type_str);

	rv += snprintf(buf + rv,
			buflen - rv,
			"QDMA IP Type    : %s\n",
			ver_info.ip_str);

	rv += snprintf(buf + rv,
			buflen - rv,
			"============Software Version============\n\n");
	rv += snprintf(buf + rv,
			buflen - rv,
			"qdma driver version : %s\n\n",
			DRV_MODULE_VERSION);

	rv = xnl_msg_add_attr_str(skb, XNL_ATTR_VERSION_INFO, buf);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_str(skb, XNL_ATTR_DEVICE_TYPE,
			ver_info.device_type_str);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_str(skb, XNL_ATTR_IP_TYPE, ver_info.ip_str);
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_str() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_MM_ENABLE, dev_attr.mm_en);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_ST_ENABLE, dev_attr.st_en);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_MM_CMPT_ENABLE,
			dev_attr.mm_cmpt_en);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_NUMQS,
			dev_attr.num_qs);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_NUM_PFS, dev_attr.num_pfs);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_MM_CHANNEL_MAX,
			dev_attr.mm_channel_max);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_MAILBOX_ENABLE,
			dev_attr.mailbox_en);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEV_FLR_PRESENT,
			dev_attr.flr_present);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DEBUG_EN,
			dev_attr.debug_mode);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_DESC_ENGINE_MODE,
			dev_attr.desc_eng_mode);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		goto free_skb;
	}

	rv = xnl_msg_add_attr_data(skb, XNL_ATTR_DEV,
			(void *)&dev_attr, sizeof(struct qdma_dev_attributes));
	if (rv != 0) {
		pr_err("xnl_msg_add_attr_data() failed: %d", rv);
		return rv;
	}

	rv = xnl_msg_send(skb, hdr, info);
	return rv;

free_skb:
	nlmsg_free(skb);
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
	unsigned long long min_ping_pong_lat = 0;
	unsigned long long max_ping_pong_lat = 0;
	unsigned long long total_ping_pong_lat = 0;
	unsigned long long avg_ping_pong_lat = 0;
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
	qdma_device_get_ping_pong_min_lat(xpdev->dev_hndl,
							&min_ping_pong_lat);
	qdma_device_get_ping_pong_max_lat(xpdev->dev_hndl,
							&max_ping_pong_lat);
	qdma_device_get_ping_pong_tot_lat(xpdev->dev_hndl,
							&total_ping_pong_lat);

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
	pkts = min_ping_pong_lat;
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMIN1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (min_ping_pong_lat >> 32);
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMIN2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = min_ping_pong_lat;
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMIN1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (min_ping_pong_lat >> 32);
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMIN2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = max_ping_pong_lat;
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMAX1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (max_ping_pong_lat >> 32);
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATMAX2, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}

	if (stc2h_pkts != 0)
		avg_ping_pong_lat = total_ping_pong_lat / stc2h_pkts;
	else
		pr_err("No C2H packets to calculate avg\n");

	pkts = avg_ping_pong_lat;
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATAVG1, pkts);
	if (rv < 0) {
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}
	pkts = (avg_ping_pong_lat >> 32);
	rv = xnl_msg_add_attr_uint(skb,
				XNL_ATTR_DEV_STAT_PING_PONG_LATAVG2, pkts);
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

	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX, 0);

	kfree(buf);
	return rv;
}



static int xnl_get_queue_state(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	struct xlnx_qdata *qdata;
	int rv = 0;
	unsigned char is_qp;
	unsigned int q_flags;
	struct sk_buff *skb;
	void *hdr;
	struct qdma_q_state qstate;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	skb = xnl_msg_alloc(XNL_CMD_DEV_STAT, 0, &hdr, info);
	if (!skb)
		return -ENOMEM;


	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev) {
		nlmsg_free(skb);
		return -EINVAL;
	}

	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0) {
		nlmsg_free(skb);
		return -EINVAL;
	}
	if (is_qp)
		return -EINVAL;

	qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
				XNL_RESP_BUFLEN_MIN);
	if (!qdata) {
		nlmsg_free(skb);
		return -EINVAL;
	}

	rv = qdma_get_queue_state(xpdev->dev_hndl, qdata->qhndl, &qstate,
			buf, XNL_RESP_BUFLEN_MIN);
	if (rv < 0) {
		xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX, rv);
		nlmsg_free(skb);
		pr_err("qdma_get_queue_state() failed: %d", rv);
		return rv;
	}

	q_flags = 0;

	if (qstate.st)
		q_flags |= XNL_F_QMODE_ST;
	else
		q_flags |= XNL_F_QMODE_MM;


	if (qstate.q_type == Q_C2H)
		q_flags |= XNL_F_QDIR_C2H;
	else if (qstate.q_type == Q_H2C)
		q_flags |= XNL_F_QDIR_H2C;
	else
		q_flags |= XNL_F_Q_CMPL;

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_QFLAG, q_flags);
	if (rv < 0) {
		nlmsg_free(skb);
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_QIDX, qstate.qidx);
	if (rv < 0) {
		nlmsg_free(skb);
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}

	rv = xnl_msg_add_attr_uint(skb, XNL_ATTR_Q_STATE, qstate.qstate);
	if (rv < 0) {
		nlmsg_free(skb);
		pr_err("xnl_msg_add_attr_uint() failed: %d", rv);
		return rv;
	}


	rv = xnl_msg_send(skb, hdr, info);

	return rv;

}

static int xnl_q_list(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	char *buf;
	int rv = 0;
	uint32_t qmax = 0;
	uint32_t buflen = 0, max_buflen = 0;
	struct qdma_queue_count q_count;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return -EINVAL;

	buf = xnl_mem_alloc(XNL_RESP_BUFLEN_MIN, info);
	if (!buf)
		return -ENOMEM;
	buflen = XNL_RESP_BUFLEN_MIN;

	rv = qdma_get_queue_count(xpdev->dev_hndl, &q_count, buf, buflen);
	if (rv < 0) {
		rv += snprintf(buf, XNL_RESP_BUFLEN_MIN,
					   "Failed to get queue count\n");
		goto send_rsp;
	}

	qmax = q_count.h2c_qcnt + q_count.c2h_qcnt;

	if (!qmax) {
		rv += snprintf(buf, 8, "Zero Qs\n\n");
		goto send_rsp;
	}

	max_buflen = (qmax * QDMA_Q_LIST_LINE_SZ);
	kfree(buf);
	buf = xnl_mem_alloc(max_buflen, info);
	if (!buf)
		return -ENOMEM;

	buflen = max_buflen;
	rv = qdma_queue_list(xpdev->dev_hndl, buf, buflen);
	if (rv < 0) {
		pr_err("qdma_queue_list() failed: %d", rv);
		goto send_rsp;
	}

send_rsp:
	rv = xnl_respond_buffer(info, buf, max_buflen, rv);
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
	unsigned char dir;
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
		rv += snprintf(cur, end - cur, "Zero Qs\n");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, cur, end - cur, &is_qp);
	if (rv < 0)
		goto free_buf;

	qidx = qconf.qidx;

	rv = xnl_chk_attr(XNL_ATTR_NUM_Q, info, qidx, cur, end - cur);
	if (rv < 0)
		goto send_resp;
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(cur, end - cur, "Invalid q type received");
		goto send_resp;
	}

	dir = qconf.q_type;
	for (i = 0; i < num_q; i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;
add_q:
		if (qidx != QDMA_QUEUE_IDX_INVALID)
			qconf.qidx = qidx + i;
		rv = xpdev_queue_add(xpdev, &qconf, cur, end - cur);
		if (rv < 0) {
			pr_err("xpdev_queue_add() failed: %d\n", rv);
			goto send_resp;
		}
		cur = buf + strlen(buf);
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto add_q;
			}
		}
	}

	cur += snprintf(cur, end - cur, "Added %u Queues.\n", i);

send_resp:
	rv2 = xnl_respond_buffer(info, buf, strlen(buf), rv);
free_buf:
	kfree(buf);
	return rv < 0 ? rv : rv2;
}

static int xnl_q_buf_idx_get(struct xlnx_pci_dev *xpdev)
{
	struct global_csr_conf csr;
	int i, rv;

	memset(&csr, 0, sizeof(struct global_csr_conf));
	rv = qdma_global_csr_get(xpdev->dev_hndl, 0,
				 QDMA_GLOBAL_CSR_ARRAY_SZ,
				 &csr);
	if (rv < 0)
		return 0;

	for (i = 0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++) {
		if (csr.c2h_buf_sz[i] == QDMA_C2H_DEFAULT_BUF_SZ)
			return i;
	}

	return 0;
}

static int xnl_q_start(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	struct qdma_queue_conf qconf_old;
	char buf[XNL_RESP_BUFLEN_MIN];
	struct xlnx_qdata *qdata;
	int rv = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char dir;
	unsigned char is_bufsz_idx = 1;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs\n");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	qidx = qconf.qidx;

	rv = xnl_chk_attr(XNL_ATTR_NUM_Q, info, qidx, buf, XNL_RESP_BUFLEN_MIN);
	if (rv < 0)
		goto send_resp;
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	xnl_extract_extra_config_attr(info, &qconf);

	if (qconf.st && (qconf.q_type == Q_CMPT)) {
		rv += snprintf(buf, 40, "MM CMPL is valid only for MM Mode");
		goto send_resp;
	}

	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	if (!info->attrs[XNL_ATTR_C2H_BUFSZ_IDX])
		is_bufsz_idx = 0;

	if (!info->attrs[XNL_ATTR_MM_CHANNEL])
		qconf.mm_channel = 0;

	dir = qconf.q_type;
	for (i = qidx; i < (qidx + num_q); i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;
reconfig:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
					XNL_RESP_BUFLEN_MIN);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_get_config(xpdev->dev_hndl, qdata->qhndl,
				&qconf_old, buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0)
			goto send_resp;

		if (qconf.q_type != Q_CMPT) {
			if (qconf_old.st && qconf_old.q_type && !is_bufsz_idx)
				qconf.c2h_buf_sz_idx = xnl_q_buf_idx_get(xpdev);
		}
		rv = qdma_queue_config(xpdev->dev_hndl, qdata->qhndl,
				&qconf, buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0) {
			pr_err("qdma_queue_config failed: %d", rv);
			goto send_resp;
		}
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto reconfig;
			}
		}
	}

	rv = xpdev_nl_queue_start(xpdev, info, is_qp, qconf.q_type,
			qidx, num_q);
	if (rv < 0) {
		snprintf(buf, XNL_RESP_BUFLEN_MIN, "qdma%05x OOM.\n",
			xpdev->idx);
		goto send_resp;
	}

	return 0;

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN, rv);

	return rv;
}

static int xnl_q_stop(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	struct xlnx_qdata *qdata;
	int rv = 0, rv2 = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char dir;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs\n");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}

	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	dir = qconf.q_type;
	for (i = qidx; i < (qidx + num_q); i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;
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
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto stop_q;
			}
		}
	}
	rv2 = snprintf(buf + rv, XNL_RESP_BUFLEN_MIN - rv,
				  "Stopped Queues %u -> %u.\n", qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN, rv);
	return rv;
}

static int xnl_q_del(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	struct qdma_queue_conf qconf;
	char buf[XNL_RESP_BUFLEN_MIN];
	int rv = 0, rv2 = 0;
	unsigned char is_qp;
	unsigned short num_q;
	unsigned int i;
	unsigned short qidx;
	unsigned char dir;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (unlikely(!qdma_get_qmax(xpdev->dev_hndl))) {
		rv += snprintf(buf, 8, "Zero Qs\n");
		goto send_resp;
	}
	rv = qconf_get(&qconf, info, buf, XNL_RESP_BUFLEN_MIN, &is_qp);
	if (rv < 0)
		goto send_resp;

	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;

	dir = qconf.q_type;
	for (i = qidx; i < (qidx + num_q); i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;
del_q:
		qconf.qidx = i;
		rv = xpdev_queue_delete(xpdev, qconf.qidx, qconf.q_type,
					buf, XNL_RESP_BUFLEN_MIN);
		if (rv < 0) {
			pr_err("xpdev_queue_delete() failed: %d", rv);
			goto send_resp;
		}
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto del_q;
			}
		}
	}
	rv2 = snprintf(buf + rv, XNL_RESP_BUFLEN_MIN - rv,
				  "Deleted Queues %u -> %u.\n", qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN, rv);
	return rv;
}

static int xnl_config_reg_dump(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	char *buf;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	int rv = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);
	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;

	qdma_config_reg_dump(xpdev->dev_hndl, buf, buf_len);

	rv = xnl_respond_buffer(info, buf, buf_len, rv);

	kfree(buf);
	return rv;
}

static int xnl_config_reg_info_dump
	(struct sk_buff *skb2, struct genl_info *info)
{
	struct xlnx_pci_dev *xpdev;
	char *buf;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	int rv = 0;
	uint32_t reg_addr = 0;
	uint32_t num_regs = 0;

	if (info == NULL)
		return 0;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	if (info->attrs[XNL_ATTR_RSP_BUF_LEN])
		buf_len =  nla_get_u32(info->attrs[XNL_ATTR_RSP_BUF_LEN]);

	if (info->attrs[XNL_ATTR_REG_ADDR])
		reg_addr =  nla_get_u32(info->attrs[XNL_ATTR_REG_ADDR]);

	if (info->attrs[XNL_ATTR_NUM_REGS])
		num_regs =  nla_get_u32(info->attrs[XNL_ATTR_NUM_REGS]);

	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;

	qdma_config_reg_info_dump(xpdev->dev_hndl,
				reg_addr, num_regs, buf, buf_len);

	rv = xnl_respond_buffer(info, buf, buf_len, rv);

	kfree(buf);
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
	unsigned char dir;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	unsigned int buf_idx = 0;
	char banner[DUMP_LINE_SZ];

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

	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		kfree(buf);
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	if (num_q > QDMA_Q_DUMP_MAX_QUEUES) {
		pr_err("Can not dump more than %d queues\n",
			   QDMA_Q_DUMP_MAX_QUEUES);
		rv += snprintf(buf, 40, "Can not dump more than %d queues\n",
				QDMA_Q_DUMP_MAX_QUEUES);
		goto send_resp;
	}
	kfree(buf);
	buf_len = (num_q * QDMA_Q_DUMP_LINE_SZ);
	buf = xnl_mem_alloc(buf_len, info);
	if (!buf)
		return -ENOMEM;

	qidx = qconf.qidx;
	dir = qconf.q_type;

	for (i = 0; i < DUMP_LINE_SZ - 5; i++)
		snprintf(banner + i, DUMP_LINE_SZ - 5, "*");

	for (i = qidx; i < (qidx + num_q); i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;

dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;

		buf_idx += snprintf(buf + buf_idx,
				DUMP_LINE_SZ, "\n%s", banner);

		buf_idx += snprintf(buf + buf_idx, buf_len - buf_idx,
#ifndef __QDMA_VF__
				"\n%40s qdma%05x %s QID# %u\n",
#else
				"\n%40s qdmavf%05x %s QID# %u\n",
#endif
				"Context Dump for",
				  xpdev->idx,
				  q_type_list[qconf.q_type].name,
				  qconf.qidx);

		buf_idx += snprintf(buf + buf_idx,
				buf_len - buf_idx, "\n%s\n", banner);

		rv = qdma_queue_dump(xpdev->dev_hndl, qdata->qhndl,
				     buf + buf_idx,
				     buf_len - buf_idx);
		buf_idx = strlen(buf);
		if (rv < 0) {
			pr_err("qdma_queue_dump() failed: %d", rv);
			goto send_resp;
		}
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto dump_q;
			}
		}
	}
	rv = snprintf(buf + buf_idx, buf_len - buf_idx,
		      "Dumped Queues %u -> %u.\n", qidx, i - 1);
	buf_idx += rv;
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len, rv);
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
	unsigned char dir;
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
		rv = snprintf(ebuf, XNL_RESP_BUFLEN_MIN,
				"%s OOM %d.\n",
				__func__, buf_len);
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN, rv);
		return -ENOMEM;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);


	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	qidx = qconf.qidx;
	dir = qconf.q_type;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.q_type = dir;
dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump_desc(xpdev->dev_hndl,
					qdata->qhndl, v1, v2,
					buf + buf_idx, buf_len - buf_idx);
		buf_idx = strlen(buf);

		if (rv < 0) {
			pr_err("qdma_queue_dump_desc() failed: %d", rv);
			goto send_resp;
		}
		if (is_qp && (dir == qconf.q_type)) {
			qconf.q_type = (~qconf.q_type) & 0x1;
			goto dump_q;
		}
	}

	rv = snprintf(buf + buf_idx, buf_len - buf_idx,
		      "Dumped descs of queues %u -> %u.\n",
		      qidx, i - 1);
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len, rv);

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
	unsigned char dir;
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
		rv = snprintf(ebuf, XNL_RESP_BUFLEN_MIN,
				"%s OOM %d.\n",
				__func__, buf_len);
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN, rv);
		return -ENOMEM;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);


	if (qconf.q_type > Q_CMPT) {
		pr_err("Invalid q type received");
		rv += snprintf(buf, 40, "Invalid q type received");
		goto send_resp;
	}

	qidx = qconf.qidx;

	dir = qconf.q_type;
	for (i = qidx; i < (qidx + num_q); i++) {
		if (qconf.q_type != Q_CMPT)
			qconf.q_type = dir;
dump_q:
		qconf.qidx = i;
		qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf + buf_idx,
					buf_len - buf_idx);
		if (!qdata)
			goto send_resp;
		rv = qdma_queue_dump_cmpt(xpdev->dev_hndl,
					qdata->qhndl, v1, v2,
					buf + buf_idx, buf_len - buf_idx);
		buf_idx = strlen(buf);
		if (rv < 0) {
			pr_err("qdma_queue_dump_cmpt() failed: %d", rv);
			goto send_resp;
		}
		if (qconf.q_type != Q_CMPT) {
			if (is_qp && (dir == qconf.q_type)) {
				qconf.q_type = (~qconf.q_type) & 0x1;
				goto dump_q;
			}
		}
	}
	rv = snprintf(buf + buf_idx, buf_len - buf_idx,
		      "Dumped descs of queues %u -> %u.\n",
		      qidx, i - 1);
	buf_idx += rv;
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len, rv);

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
	rv = xnl_respond_buffer(info, buf, buf_len, rv);

	kfree(buf);
	return rv;
}

static int xnl_q_cmpt_read(struct sk_buff *skb2, struct genl_info *info)
{
	int rv = 0, err = 0;
	struct qdma_queue_conf qconf;
	char *buf = NULL;
	unsigned char is_qp = 0;
	struct xlnx_pci_dev *xpdev = NULL;
	struct xlnx_qdata *qdata = NULL;
	int buf_len = XNL_RESP_BUFLEN_MAX;
	u32 num_entries = 0;
	u8 *cmpt_entries = NULL, *cmpt_entry_list = NULL;
	u32 cmpt_entry_len = 0;
	u32 count = 0, diff_len = 0;
	struct qdma_queue_conf qconf_attr;

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

	rv = qconf_get(&qconf, info, buf, buf_len, &is_qp);
	if (rv < 0)
		goto send_resp;

	qconf.q_type = Q_CMPT;
	qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf, buf_len);
	if (!qdata)
		goto send_resp;

	rv = qdma_queue_get_config(xpdev->dev_hndl, qdata->qhndl,
			&qconf_attr, buf, buf_len);
	if (rv < 0)
		goto send_resp;

	rv = qdma_descq_read_cmpt_data(xpdev->dev_hndl,
					qdata->qhndl,
					&num_entries,
					&cmpt_entries,
					buf,
					buf_len);
	if (rv < 0)
		goto free_cmpt;

	if (num_entries != 0) {
		memset(buf, '\0', buf_len);
		cmpt_entry_list = cmpt_entries;
		cmpt_entry_len = 8 << qconf_attr.cmpl_desc_sz;
		for (count = 0; count < num_entries; count++) {
			hex_dump_to_buffer(cmpt_entry_list, cmpt_entry_len,
						32, 4, buf + diff_len,
						buf_len - diff_len, 0);
			diff_len = strlen(buf);
			if (cmpt_entry_len > 32) {
				diff_len += snprintf(buf + diff_len,
						buf_len - diff_len,
						" ");
				hex_dump_to_buffer(cmpt_entry_list + 32,
						cmpt_entry_len,
						32, 4, buf + diff_len,
						buf_len - diff_len, 0);
				diff_len = strlen(buf);
			}
			buf[diff_len++] = '\n';
			cmpt_entry_list += cmpt_entry_len;
		}
	}

free_cmpt:
	kfree(cmpt_entries);
send_resp:
	err = rv;
	rv = xnl_respond_buffer_cmpt(info, buf, buf_len,
					err, num_entries);
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
		rv = snprintf(ebuf, XNL_RESP_BUFLEN_MIN, "%s OOM %d.\n",
				__func__, XNL_RESP_BUFLEN_MAX);
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN, rv);
		return -ENOMEM;
	}

	qdata = xnl_rcv_check_qidx(info, xpdev, &qconf, buf,
				XNL_RESP_BUFLEN_MIN);
	if (!qdata)
		goto send_resp;
	err = nla_get_u32(info->attrs[XNL_ATTR_QPARAM_ERR_INFO]);

	if (qdma_queue_set_err_induction(xpdev->dev_hndl, qdata->qhndl, err,
					 buf, XNL_RESP_BUFLEN_MAX)) {
		rv += snprintf(buf + rv, XNL_RESP_BUFLEN_MAX,
					 "Failed to set induce err\n");
		goto send_resp;
	}
	rv += snprintf(buf + rv, XNL_RESP_BUFLEN_MAX,
				  "queue error induced\n");

send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MAX, rv);

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
		rv = snprintf(ebuf, XNL_RESP_BUFLEN_MIN,
				"%s OOM %d.\n",
				__func__, buf_len);
		xnl_respond_buffer(info, ebuf, XNL_RESP_BUFLEN_MIN, rv);
		return -ENOMEM;
	}

	if (!info->attrs[XNL_ATTR_NUM_Q]) {
		pr_warn("Missing attribute 'XNL_ATTR_NUM_Q'");
		return -1;
	}
	num_q = nla_get_u32(info->attrs[XNL_ATTR_NUM_Q]);

	qidx = qconf.qidx;
	for (i = qidx; i < (qidx + num_q); i++) {
		qconf.q_type = 1;
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
send_resp:
	rv = xnl_respond_buffer(info, buf, buf_len, rv);

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
			rv += snprintf(buf + rv, buf_len,
				"vector_idx %u is for error interrupt\n",
				vector_idx);
			goto send_resp;
		} else if (vector_idx == 1) {
			rv += snprintf(buf + rv, buf_len,
				"vector_idx %u is for user interrupt\n",
				vector_idx);
			goto send_resp;
		}
	} else {
		if (vector_idx == 0) {
			rv += snprintf(buf + rv, buf_len,
				"vector_idx %u is for user interrupt\n",
				vector_idx);
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
	rv = xnl_respond_buffer(info, buf, buf_len, rv);

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
		rv = qdma_device_read_config_register(xpdev->dev_hndl,
				reg_addr, &reg_val);
		if (rv < 0) {
			pr_err("Config bar register read failed with error = %d\n",
					rv);
			return rv;
		}
	} else if (bar_num == conf.bar_num_user) {
		rv = qdma_device_read_user_register(xpdev, reg_addr, &reg_val);
		if (rv < 0) {
			pr_err("AXI Master Lite bar register read failed with error = %d\n",
					rv);
			return rv;
		}
	} else if (bar_num == conf.bar_num_bypass) {
		rv = qdma_device_read_bypass_register(xpdev,
				reg_addr, &reg_val);
		if (rv < 0) {
			pr_err("AXI Bridge Master bar register read failed with error = %d\n",
					rv);
			return rv;
		}
	} else {
		rv += snprintf(buf + rv, XNL_RESP_BUFLEN_MIN,
				"Invalid bar number\n");
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
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN, err);
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
		rv = qdma_device_write_config_register(xpdev->dev_hndl,
				reg_addr, reg_val);
		if (rv < 0) {
			pr_err("Config bar register write failed with error = %d\n",
					rv);
			return rv;
		}
	} else if (bar_num == conf.bar_num_user) {
		rv = qdma_device_write_user_register(xpdev, reg_addr, reg_val);
		if (rv < 0) {
			pr_err("AXI Master Lite bar register write failed with error = %d\n",
					rv);
			return rv;
		}

	} else if (bar_num == conf.bar_num_bypass) {
		rv = qdma_device_write_bypass_register(xpdev,
				reg_addr, reg_val);
		if (rv < 0) {
			pr_err("AXI Bridge Master bar register write failed with error = %d\n",
					rv);
			return rv;
		}
	} else {
		rv += snprintf(buf + rv, XNL_RESP_BUFLEN_MIN,
					 "Invalid bar number\n");
		goto send_resp;
	}


send_resp:
	rv = xnl_respond_buffer(info, buf, XNL_RESP_BUFLEN_MIN, rv);
	return rv;
}

static int xnl_get_global_csr(struct sk_buff *skb2, struct genl_info *info)
{
	struct global_csr_conf *csr;
	struct xlnx_pci_dev *xpdev;
	int rv;
	u8 index = 0, count = 0;

	if (info == NULL)
		return -EINVAL;

	xnl_dump_attrs(info);

	xpdev = xnl_rcv_check_xpdev(info);
	if (!xpdev)
		return 0;

	csr = kmalloc(sizeof(struct global_csr_conf), GFP_KERNEL);
	if (!csr)
		return -ENOMEM;

	if (!info->attrs[XNL_ATTR_CSR_INDEX]) {
		pr_warn("Missing attribute 'XNL_ATTR_CSR_INDEX'");
		kfree(csr);
		return -EINVAL;
	}

	if (!info->attrs[XNL_ATTR_CSR_COUNT]) {
		pr_warn("Missing attribute 'XNL_ATTR_CSR_COUNT'");
		kfree(csr);
		return -EINVAL;
	}

	index = nla_get_u32(info->attrs[XNL_ATTR_CSR_INDEX]);
	count = nla_get_u32(info->attrs[XNL_ATTR_CSR_COUNT]);

	rv = qdma_global_csr_get(xpdev->dev_hndl, index, count, csr);
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
			xnl_ops, ARRAY_SIZE(xnl_ops));
#else
	rv = genl_register_family(&xnl_family);
#endif
	if (rv)
		pr_err("genl_register_family failed %d.\n", rv);

	return rv;
}

void  xlnx_nl_exit(void)
{
	int rv;

	rv = genl_unregister_family(&xnl_family);
	if (rv)
		pr_err("genl_unregister_family failed %d.\n", rv);
}
