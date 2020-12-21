/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2018-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#include <string.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>

#include "cmd_parse.h"
#include "dmautils.h"

static int (*xnl_proc_fn[XNL_CMD_MAX])(struct xcmd_info *xcmd) = {
	qdma_dev_list_dump,      /* XNL_CMD_DEV_LIST */
	qdma_dev_info,           /* XNL_CMD_DEV_INFO */
	qdma_dev_stat,           /* XNL_CMD_DEV_STAT */
	qdma_dev_stat_clear,     /* XNL_CMD_DEV_STAT_CLEAR */
	qdma_reg_dump,           /* XNL_CMD_REG_DUMP */
	qdma_reg_read,           /* XNL_CMD_REG_RD */
	qdma_reg_write,          /* XNL_CMD_REG_WRT */
	qdma_reg_info_read,      /* XNL_CMD_REG_INFO_READ */
	qdma_dev_q_list_dump,    /* XNL_CMD_Q_LIST */
	qdma_q_add,              /* XNL_CMD_Q_ADD */
	qdma_q_start,            /* XNL_CMD_Q_START */
	qdma_q_stop,             /* XNL_CMD_Q_STOP */
	qdma_q_del,              /* XNL_CMD_Q_DEL */
	qdma_q_dump,             /* XNL_CMD_Q_DUMP */
	qdma_q_desc_dump,        /* XNL_CMD_Q_DESC */
	qdma_q_desc_dump,        /* XNL_CMD_Q_CMPT */
	NULL,                    /* XNL_CMD_Q_RX_PKT */
	qdma_q_cmpt_read,        /* XNL_CMD_Q_CMPT_READ */
#ifdef ERR_DEBUG
	NULL,                    /* XNL_CMD_Q_ERR_INDUCE */
#endif
	qdma_dev_intr_ring_dump, /* XNL_CMD_INTR_RING_DUMP */
	NULL,                    /* XNL_CMD_Q_UDD */
	qdma_dev_get_global_csr, /* XNL_CMD_GLOBAL_CSR */
	qdma_dev_cap,            /* XNL_CMD_DEV_CAP */
	NULL                     /* XNL_CMD_GET_Q_STATE */
};

static const char *desc_engine_mode[] = {
	"Internal and Bypass mode",
	"Bypass only mode",
	"Inernal only mode"
};

static void dump_dev_cap(struct xcmd_info *xcmd)
{
	printf("%s", xcmd->resp.cap.version_str);
	printf("=============Hardware Capabilities============\n\n");
	printf("Number of PFs supported                : %u\n", xcmd->resp.cap.num_pfs);
	printf("Total number of queues supported       : %u\n", xcmd->resp.cap.num_qs);
	printf("MM channels                            : %u\n", xcmd->resp.cap.mm_channel_max);
	printf("FLR Present                            : %s\n", xcmd->resp.cap.flr_present ? "yes":"no");
	printf("ST enabled                             : %s\n",	xcmd->resp.cap.st_en ? "yes":"no");
	printf("MM enabled                             : %s\n", xcmd->resp.cap.mm_en ? "yes":"no");
	printf("Mailbox enabled                        : %s\n", xcmd->resp.cap.mailbox_en ? "yes":"no");
	printf("MM completion enabled                  : %s\n", xcmd->resp.cap.mm_cmpt_en ? "yes":"no");
	printf("Debug Mode enabled                     : %s\n", xcmd->resp.cap.debug_mode ? "yes":"no");

	if (xcmd->resp.cap.desc_eng_mode < sizeof(desc_engine_mode) / sizeof(desc_engine_mode[0])) {
		printf("Desc Engine Mode                       : %s\n",
			   desc_engine_mode[xcmd->resp.cap.desc_eng_mode]);
	}else {
		printf("Desc Engine Mode                       : INVALID\n");
	}
}

static void dump_dev_info(struct xcmd_info *xcmd)
{
	printf("=============Device Information============\n");
	printf("PCI                                    : %02x:%02x.%01x\n",
	       xcmd->resp.dev_info.pci_bus,
	       xcmd->resp.dev_info.pci_dev,
	       xcmd->resp.dev_info.dev_func);
	printf("HW q base                              : %u\n", xcmd->resp.dev_info.qbase);
	printf("Max queues                             : %u\n",	xcmd->resp.dev_info.qmax);
	printf("Config bar                             : %u\n", xcmd->resp.dev_info.config_bar);
	printf("AXI Master Lite bar                    : %u\n", xcmd->resp.dev_info.user_bar);
}

static void dump_dev_stat(struct xcmd_info *xcmd)
{
	unsigned long long mmh2c_pkts;
	unsigned long long mmc2h_pkts;
	unsigned long long sth2c_pkts;
	unsigned long long stc2h_pkts;
	unsigned long long min_ping_pong_lat = 0;
	unsigned long long max_ping_pong_lat = 0;
	unsigned long long avg_ping_pong_lat = 0;

	mmh2c_pkts = xcmd->resp.dev_stat.mm_h2c_pkts;
	mmc2h_pkts = xcmd->resp.dev_stat.mm_c2h_pkts;
	sth2c_pkts = xcmd->resp.dev_stat.st_h2c_pkts;
	stc2h_pkts = xcmd->resp.dev_stat.st_c2h_pkts;
	min_ping_pong_lat = xcmd->resp.dev_stat.ping_pong_lat_min;
	max_ping_pong_lat = xcmd->resp.dev_stat.ping_pong_lat_max;
	avg_ping_pong_lat = xcmd->resp.dev_stat.ping_pong_lat_avg;

	printf("qdma%s%05x:statistics\n", xcmd->vf ? "vf" : "", xcmd->if_bdf);
	printf("Total MM H2C packets processed = %llu\n", mmh2c_pkts);
	printf("Total MM C2H packets processed = %llu\n", mmc2h_pkts);
	printf("Total ST H2C packets processed = %llu\n", sth2c_pkts);
	printf("Total ST C2H packets processed = %llu\n", stc2h_pkts);
	printf("Min Ping Pong Latency = %llu\n", min_ping_pong_lat);
	printf("Max Ping Pong Latency = %llu\n", max_ping_pong_lat);
	printf("Avg Ping Pong Latency = %llu\n", avg_ping_pong_lat);
}

static void dump_dev_global_csr(struct xcmd_info *xcmd)
{
	printf("Global Ring Sizes:");
	for ( int i=0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		printf("%d ",xcmd->resp.csr.ring_sz[i]);
	printf("\nC2H Timer Counters:");
	for ( int i=0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		printf("%d ",xcmd->resp.csr.c2h_timer_cnt[i]);
	printf("\nC2H Counter Thresholds:");
	for ( int i=0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		printf("%d ",xcmd->resp.csr.c2h_cnt_th[i]);
	printf("\nC2H Buf Sizes:");
	for ( int i=0; i < QDMA_GLOBAL_CSR_ARRAY_SZ; i++)
		printf("%d ",xcmd->resp.csr.c2h_cnt_th[i]);
	printf("\nWriteback Interval:%d\n",xcmd->resp.csr.wb_intvl);

}

static void xnl_dump_response(const char *resp)
{
	printf("%s", resp);
}

void xnl_dump_cmd_resp(struct xcmd_info *xcmd)
{

	switch(xcmd->op) {
        case XNL_CMD_DEV_CAP:
        	dump_dev_cap(xcmd);
		break;
        case XNL_CMD_DEV_INFO:
        	dump_dev_info(xcmd);
		break;
        case XNL_CMD_DEV_STAT:
        	dump_dev_stat(xcmd);
		break;
        case XNL_CMD_REG_RD:
		printf("qdma%s%05x, %02x:%02x.%02x, bar#%u, 0x%x = 0x%x.\n",
				xcmd->vf ? "vf" :"",
				xcmd->if_bdf, xcmd->resp.dev_info.pci_bus,
				xcmd->resp.dev_info.pci_dev,
				xcmd->resp.dev_info.dev_func, xcmd->req.reg.bar,
				xcmd->req.reg.reg, xcmd->req.reg.val);
		break;
        case XNL_CMD_REG_WRT:
		printf("qdma%s%05x, %02x:%02x.%02x, bar#%u, reg 0x%x, read back 0x%x.\n",
			   xcmd->vf ? "vf" :"",
			   xcmd->if_bdf, xcmd->resp.dev_info.pci_bus,
			   xcmd->resp.dev_info.pci_dev,
			   xcmd->resp.dev_info.dev_func, xcmd->req.reg.bar,
			   xcmd->req.reg.reg, xcmd->req.reg.val);
		break;
	case XNL_CMD_GLOBAL_CSR:
			dump_dev_global_csr(xcmd);
		break;
	case XNL_CMD_REG_INFO_READ:
		break;
	default:
		break;
	}
}

static int xnl_proc_cmd(struct xcmd_info *xcmd)
{
	xcmd->log_msg_dump = xnl_dump_response;
	if (xnl_proc_fn[xcmd->op])
		return xnl_proc_fn[xcmd->op](xcmd);

	return -EOPNOTSUPP;
}

int main(int argc, char *argv[])
{
	struct xcmd_info xcmd;
	int rv = 0;

	memset(&xcmd, 0, sizeof(xcmd));

	rv = parse_cmd(argc, argv, &xcmd);
	if (rv < 0)
		return rv;
	rv = xnl_proc_cmd(&xcmd);
	if (rv < 0)
		return rv;
	xnl_dump_cmd_resp(&xcmd);

	return 0;
}
