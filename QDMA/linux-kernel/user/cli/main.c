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

#include <string.h>
#include <stdio.h>

#include "nl_user.h"
#include "cmd_parse.h"
#include "reg_cmd.h"

static void dump_dev_info(struct xcmd_info *xcmd)
{
	printf("%s", xcmd->u.cap.version_str);
	printf("=============Hardware Capabilities============\n\n");
	printf("Number of PFs supported                : %d\n", xcmd->u.cap.num_pfs);
	printf("Total number of queues supported       : %d\n", xcmd->u.cap.num_qs);
	printf("MM channels                            : %d\n", xcmd->u.cap.mm_channel_max);
	printf("FLR Present                            : %s\n", xcmd->u.cap.flr_present ? "yes":"no");
	printf("ST enabled                             : %s\n",	xcmd->u.cap.st_en ? "yes":"no");
	printf("MM enabled                             : %s\n", xcmd->u.cap.mm_en ? "yes":"no");
	printf("Mailbox enabled                        : %s\n", xcmd->u.cap.mailbox_en ? "yes":"no");
	printf("MM completion enabled                  : %s\n\n", xcmd->u.cap.mm_cmpt_en ? "yes":"no");
}
int main(int argc, char *argv[])
{
	struct xnl_cb cb;
	struct xcmd_info xcmd;
	unsigned char op;
	int rv = 0;

	memset(&xcmd, 0, sizeof(xcmd));

	rv = parse_cmd(argc, argv, &xcmd);
	if (rv < 0)
		return rv;

#if 0
	printf("cmd op %s, 0x%x, ifname %s.\n",
		xnl_op_str[xcmd.op], xcmd.op, xcmd.ifname);
#endif

	memset(&cb, 0, sizeof(struct xnl_cb));

	if (xcmd.op == XNL_CMD_DEV_LIST) {
		/* try pf nl server */
		rv = xnl_connect(&cb, 0);
		if (!rv)
			rv = xnl_send_cmd(&cb, &xcmd);
		xnl_close(&cb);

		/* try vf nl server */
		memset(&cb, 0, sizeof(struct xnl_cb));
		rv = xnl_connect(&cb, 1);
		if (!rv)
			rv = xnl_send_cmd(&cb, &xcmd);
		xnl_close(&cb);

		goto close;
	} else if ((xcmd.op == XNL_CMD_REG_DUMP) ||
			(xcmd.op == XNL_CMD_REG_RD) ||
			(xcmd.op == XNL_CMD_REG_WRT)) {
		rv = proc_reg_cmd(&xcmd);
		return rv;
	} else {
		/* for all other command, query the target device info first */
		rv = xnl_connect(&cb, xcmd.vf);
		if (rv < 0)
			goto close;

		op = xcmd.op;
		xcmd.op = XNL_CMD_DEV_INFO;
		rv = xnl_send_cmd(&cb, &xcmd);
		xcmd.op = op;
		if (rv < 0)
			goto close;
		else
			rv = xnl_send_cmd(&cb, &xcmd);

		if (xcmd.op == XNL_CMD_DEV_CAP)
			dump_dev_info(&xcmd);
	}

close:
	xnl_close(&cb);
	return rv;
}
