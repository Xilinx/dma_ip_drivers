/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2016-2022,  Xilinx, Inc.
 * All rights reserved.
 * Copyright (c) 2022-2026, Advanced Micro Devices, Inc. All rights reserved.
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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM xdma

#if !defined(_XDMA_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _XDMA_TRACE_H

#include <linux/tracepoint.h>

#include "libxdma.h"

TRACE_DEFINE_ENUM(DMA_BIDIRECTIONAL);
TRACE_DEFINE_ENUM(DMA_TO_DEVICE);
TRACE_DEFINE_ENUM(DMA_FROM_DEVICE);
TRACE_DEFINE_ENUM(DMA_NONE);

TRACE_DEFINE_ENUM(TRANSFER_STATE_NEW);
TRACE_DEFINE_ENUM(TRANSFER_STATE_SUBMITTED);
TRACE_DEFINE_ENUM(TRANSFER_STATE_COMPLETED);
TRACE_DEFINE_ENUM(TRANSFER_STATE_FAILED);
TRACE_DEFINE_ENUM(TRANSFER_STATE_ABORTED);

#define xdma_show_dir(dir)				\
	__print_symbolic(dir,				\
		{ DMA_BIDIRECTIONAL,	"BIDIR" },	\
		{ DMA_TO_DEVICE,	"H2C" },	\
		{ DMA_FROM_DEVICE,	"C2H" },	\
		{ DMA_NONE,		"NONE" })

#define xdma_show_state(state)					\
	__print_symbolic(state,					\
		{ TRANSFER_STATE_NEW,		"NEW" },	\
		{ TRANSFER_STATE_SUBMITTED,	"SUBMITTED" },	\
		{ TRANSFER_STATE_COMPLETED,	"COMPLETED" },	\
		{ TRANSFER_STATE_FAILED,	"FAILED" },	\
		{ TRANSFER_STATE_ABORTED,	"ABORTED" })

#define xdma_show_status(status)					\
	__print_flags(status,			"|",			\
		{XDMA_STAT_BUSY, 		"BUSY"},		\
		{XDMA_STAT_DESC_STOPPED,	"DESC_STOPPED"},	\
		{XDMA_STAT_DESC_COMPLETED,	"DESC_COMPLETED"},	\
		{XDMA_STAT_ALIGN_MISMATCH,	"ALIGN_MISMATCH"},	\
		{XDMA_STAT_MAGIC_STOPPED,	"MAGIC_STOPPED"},	\
		{XDMA_STAT_INVALID_LEN,		"INVALID_LEN"},		\
		{XDMA_STAT_IDLE_STOPPED,	"IDLE_STOPPED"},	\
		{XDMA_STAT_DESC_UNSUPP_REQ,	"DESC_UNSUPP_REQ"},	\
		{XDMA_STAT_DESC_COMPL_ABORT,	"DESC_COMPL_ABORT"},	\
		{XDMA_STAT_DESC_PARITY_ERR,	"DESC_PARITY_ERR"},	\
		{XDMA_STAT_DESC_HEADER_EP,	"DESC_HEADER_EP"},	\
		{XDMA_STAT_DESC_UNEXP_COMPL,	"DESC_UNEXP_COMPL"},	\
		{XDMA_STAT_C2H_R_DECODE_ERR,	"R_DECODE_ERR"},	\
		{XDMA_STAT_C2H_R_SLAVE_ERR,	"R_SLAVE_ERR"})

TRACE_EVENT(xdma_xfer_submit,

	TP_PROTO(struct xdma_engine *engine, struct xdma_transfer *xfer),

	TP_ARGS(engine, xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u64,	ep_addr)
		__field(	u32,	len)
		__field(	u8,	dir)
		__field(	u8,	channel)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->ep_addr	= xfer->ep_addr;
		__entry->len		= xfer->len;
		__entry->dir		= (u8)engine->dir;
		__entry->channel	= (u8)engine->channel;
	),

	TP_printk("%s%u xfer=%p ep_addr=0x%llx len=%u",
		  xdma_show_dir(__entry->dir),
		  __entry->channel,
		  __entry->xfer,
		  __entry->ep_addr,
		  __entry->len)
);

TRACE_EVENT(xdma_xfer_complete,

	TP_PROTO(struct xdma_engine *engine, struct xdma_transfer *xfer),

	TP_ARGS(engine, xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u64,	ep_addr)
		__field(	u32,	len)
		__field(	u32,	state)
		__field(	u32,	status)
		__field(	u8,	dir)
		__field(	u8,	channel)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->ep_addr	= xfer->ep_addr;
		__entry->len		= xfer->len;
		__entry->state		= (u32)xfer->state;
		__entry->status		= engine->status;
		__entry->dir		= (u8)engine->dir;
		__entry->channel	= (u8)engine->channel;
	),

	TP_printk("%s%u xfer=%p ep_addr=0x%llx len=%u state=%s status=%s",
		  xdma_show_dir(__entry->dir),
		  __entry->channel,
		  __entry->xfer,
		  __entry->ep_addr,
		  __entry->len,
		  xdma_show_state(__entry->state),
		  xdma_show_status(__entry->status)
	)
);

TRACE_EVENT(xdma_desc,

	TP_PROTO(struct xdma_transfer *xfer, u32 desc_idx, struct xdma_desc *desc),

	TP_ARGS(xfer, desc_idx, desc),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u32,	desc_idx)
		__field(	u32,	bytes)
		__field(	u32,	control)
		__field(	u32,	dst_addr_lo)
		__field(	u32,	dst_addr_hi)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->desc_idx	= desc_idx;
		__entry->bytes		= desc->bytes;
		__entry->control	= desc->control;
		__entry->dst_addr_lo	= le32_to_cpu(desc->dst_addr_lo);
		__entry->dst_addr_hi	= le32_to_cpu(desc->dst_addr_hi);
	),

	TP_printk("xfer=%p desc_idx=%u bytes=%u control=0x%08x dst_addr_lo=0x%08x dst_addr_hi=0x%08x",
		  __entry->xfer,
		  __entry->desc_idx,
		  __entry->bytes,
		  __entry->control,
		  __entry->dst_addr_lo,
		  __entry->dst_addr_hi
	)
);

TRACE_EVENT(xdma_desc_result,

	TP_PROTO(struct xdma_transfer *xfer, u32 desc_idx, struct xdma_result *result),

	TP_ARGS(xfer, desc_idx, result),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u32,	desc_idx)
		__field(	u32,	status)
		__field(	u32,	length)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->desc_idx	= desc_idx;
		__entry->status		= result->status;
		__entry->length		= result->length;
	),

	TP_printk("xfer=%p desc_idx=%u status=0x%08x length=%u",
		  __entry->xfer,
		  __entry->desc_idx,
		  __entry->status,
		  __entry->length
	)
);

TRACE_EVENT(xdma_desc_count,

	TP_PROTO(struct xdma_engine *engine, struct xdma_transfer *xfer),

	TP_ARGS(engine, xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u32,	desc_used)
		__field(	u32,	desc_num)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->desc_used	= engine->desc_used;
		__entry->desc_num	= xfer->desc_num;
	),

	TP_printk("xfer=%p desc_used=%u desc_num=%u",
		  __entry->xfer,
		  __entry->desc_used,
		  __entry->desc_num
	)
);

TRACE_EVENT(xdma_cancel_xfer,

	TP_PROTO(struct xdma_transfer *xfer),

	TP_ARGS(xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	u32,	state)
	),

	TP_fast_assign(
		__entry->xfer	= xfer;
		__entry->state	= (u32)xfer->state;
	),

	TP_printk("xfer=%p state=%s",
		  __entry->xfer,
		  xdma_show_state(__entry->state)
	)
);

TRACE_EVENT(xdma_engine_running,

	TP_PROTO(u8 running),

	TP_ARGS(running),

	TP_STRUCT__entry(
		__field(	u8,	running)
	),

	TP_fast_assign(
		__entry->running	= running;
	),

	TP_printk("running=%u", __entry->running)
);

TRACE_EVENT(xdma_debug_num,

	TP_PROTO(int num),

	TP_ARGS(num),

	TP_STRUCT__entry(
		__field(	int,	num)
	),

	TP_fast_assign(
		__entry->num	= num;
	),

	TP_printk("num=%d", __entry->num)
);

TRACE_EVENT(xdma_start_transfer_list,

	TP_PROTO(struct xdma_transfer *xfer),

	TP_ARGS(xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
	),

	TP_fast_assign(
		__entry->xfer	= xfer;
	),

	TP_printk("xfer=%p", __entry->xfer)

);

TRACE_EVENT(xdma_start_final_transfer,

	TP_PROTO(struct xdma_transfer *xfer),

	TP_ARGS(xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
	),

	TP_fast_assign(
		__entry->xfer	= xfer;
	),

	TP_printk("xfer=%p", __entry->xfer)

);

TRACE_EVENT(xdma_transfer_list_end,

	TP_PROTO(struct xdma_transfer *xfer),

	TP_ARGS(xfer),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
	),

	TP_fast_assign(
		__entry->xfer	= xfer;
	),

	TP_printk("xfer=%p", __entry->xfer)

);

TRACE_EVENT(xdma_engine_status,

	TP_PROTO(u32 status),

	TP_ARGS(status),

	TP_STRUCT__entry(
		__field(	u32,	status)
	),

	TP_fast_assign(
		__entry->status	= status;
	),

	TP_printk("status=%s, value=0x%08x",
		  xdma_show_status(__entry->status),
		  __entry->status
	)

);

TRACE_EVENT(xdma_transfer_chained,

	TP_PROTO(struct xdma_transfer *xfer, struct xdma_transfer *chained_onto),

	TP_ARGS(xfer, chained_onto),

	TP_STRUCT__entry(
		__field(	void *,	xfer)
		__field(	void *, chained_onto)
	),

	TP_fast_assign(
		__entry->xfer		= xfer;
		__entry->chained_onto 	= chained_onto;
	),

	TP_printk("xfer=%p chained onto last xfer=%p",
		  __entry->xfer, __entry->chained_onto)

);

#endif /* _XDMA_TRACE_H */

#undef  TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef  TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE libxdma_trace
#include <trace/define_trace.h>
