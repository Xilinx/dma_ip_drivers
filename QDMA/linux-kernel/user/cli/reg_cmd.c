/*
 * This file is part of the QDMA userspace application
 * to enable the user to execute the QDMA functionality
 *
 * Copyright (c) 2018-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "reg_cmd.h"
#include "cmd_parse.h"
#include "xdev_regs.h"

#define QDMA_CFG_BAR_SIZE 0xB400
#define QDMA_USR_BAR_SIZE 0x100
#define STM_BAR_SIZE	0x0200003C

struct xdev_info {
	unsigned char bus;
	unsigned char dev;
	unsigned char func;
	unsigned char config_bar;
	unsigned char user_bar;
	char stm_bar;
};

static struct xreg_info qdma_dmap_regs[] = {
/* QDMA_TRQ_SEL_QUEUE_PF (0x6400) */
	{"DMAP_SEL_INT_CIDX",				0x6400, 512, 0x10, 0, 0,},
	{"DMAP_SEL_H2C_DSC_PIDX",			0x6404, 512, 0x10, 0, 0,},
	{"DMAP_SEL_C2H_DSC_PIDX",			0x6408, 512, 0x10, 0, 0,},
	{"DMAP_SEL_CMPT_CIDX",				0x640C, 512, 0x10, 0, 0,},
	{"", 0, 0, 0 }
};

/*
 * INTERNAL: for debug testing only
 */
static struct xreg_info stm_regs[] = {
	{"H2C_DATA_0",					0x02000000, 0, 0, 0, 0,},
	{"H2C_DATA_1",					0x02000004, 0, 0, 0, 0,},
	{"H2C_DATA_2",					0x02000008, 0, 0, 0, 0,},
	{"H2C_DATA_3",					0x0200000C, 0, 0, 0, 0,},
	{"H2C_DATA_4",					0x02000010, 0, 0, 0, 0,},
	{"IND_CTXT_CMD",				0x02000014, 0, 0, 0, 0,},
	{"STM_REV",					0x02000018, 0, 0, 0, 0,},
	{"CAM_CLR_STATUS",				0x0200001C, 0, 0, 0, 0,},
	{"C2H_DATA8",					0x02000020, 0, 0, 0, 0,},
	{"H2C_DATA_5",					0x02000024, 0, 0, 0, 0,},
	{"STATUS",					0x0200002C, 0, 0, 0, 0,},
	{"H2C_MODE",					0x02000030, 0, 0, 0, 0,},
	{"H2C_STATUS",					0x02000034, 0, 0, 0, 0,},
	{"C2H_MODE",					0x02000038, 0, 0, 0, 0,},
	{"C2H_STATUS",					0x0200003C, 0, 0, 0, 0,},
	{"H2C_PORT0_DEBUG0",                            0x02000040, 0, 0, 0, 0,},
	{"H2C_PORT1_DEBUG0",                            0x02000044, 0, 0, 0, 0,},
	{"H2C_PORT2_DEBUG0",                            0x02000048, 0, 0, 0, 0,},
	{"C2H_PORT0_DEBUG0",                            0x02000050, 0, 0, 0, 0,},
	{"C2H_PORT1_DEBUG0",                            0x02000054, 0, 0, 0, 0,},
	{"C2H_PORT2_DEBUG0",                            0x02000058, 0, 0, 0, 0,},
	{"H2C_DEBUG0",                                  0x02000060, 0, 0, 0, 0,},
	{"H2C_DEBUG1",                                  0x02000064, 0, 0, 0, 0,},
	{"AWERR",                                       0x02000068, 0, 0, 0, 0,},
	{"ARERR",                                       0x0200006C, 0, 0, 0, 0,},
	{"H2C_PORT0_DEBUG1",                            0x02000070, 0, 0, 0, 0,},
	{"H2C_PORT1_DEBUG1",                            0x02000074, 0, 0, 0, 0,},
	{"H2C_PORT2_DEBUG1",                            0x02000078, 0, 0, 0, 0,},
	{"", 0, 0, 0 }
};


/*
 * Register I/O through mmap of BAR0.
 */

/* /sys/bus/pci/devices/0000:<bus>:<dev>.<func>/resource<bar#> */
#define get_syspath_bar_mmap(s, bus,dev,func,bar) \
	snprintf(s, sizeof(s), \
		"/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%u", \
		bus, dev, func, bar)

static uint32_t *mmap_bar(char *fname, size_t len, int prot)
{
	int fd;
	uint32_t *bar;

	fd = open(fname, (prot & PROT_WRITE) ? O_RDWR : O_RDONLY);
	if (fd < 0)
		return NULL;

	bar = mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	close(fd);

	return bar == MAP_FAILED ? NULL : bar;
}

static int32_t reg_read_mmap(struct xdev_info *xdev,
			     unsigned char barno,
			     struct xcmd_info *xcmd,
			     unsigned int *val)
{
	uint32_t *bar;
	struct xnl_cb cb;
	char fname[256];
	int rv = 0;

	memset(&cb, 0, sizeof(struct xnl_cb));

	get_syspath_bar_mmap(fname, xdev->bus, xdev->dev, xdev->func, barno);

	bar = mmap_bar(fname, xcmd->u.reg.reg + 4, PROT_READ);
	if (!bar) {
		if (xcmd->op == XNL_CMD_REG_WRT)
			xcmd->op = XNL_CMD_REG_RD;

		rv = xnl_connect(&cb, 0);
		if (rv < 0)
			return -1;
		rv = xnl_send_cmd(&cb, xcmd);
		if(rv != 0)
			return -1;
		xnl_close(&cb);

		*val = le32toh(xcmd->u.reg.val);
		return rv;
	}

	*val = le32toh(bar[xcmd->u.reg.reg / 4]);
	munmap(bar, xcmd->u.reg.reg + 4);

	return rv;
}

static int32_t reg_write_mmap(struct xdev_info *xdev,
			      unsigned char barno,
			      struct xcmd_info *xcmd)
{
	uint32_t *bar;
	struct xnl_cb cb;
	char fname[256];
	int rv = 0;

	memset(&cb, 0, sizeof(struct xnl_cb));
	get_syspath_bar_mmap(fname, xdev->bus, xdev->dev, xdev->func, barno);

	bar = mmap_bar(fname, xcmd->u.reg.reg + 4, PROT_WRITE);
	if (!bar) {
		rv = xnl_connect(&cb, 0);
		if (rv < 0)
			return -1;
		rv = xnl_send_cmd(&cb, xcmd);
		if (rv != 0)
			return -1;
		xnl_close(&cb);

		return 0;
	}

	bar[xcmd->u.reg.reg / 4] = htole32(xcmd->u.reg.val);
	munmap(bar, xcmd->u.reg.reg + 4);
	return 0;
}

static void print_repeated_reg(uint32_t *bar, struct xreg_info *xreg,
		unsigned start, unsigned limit, struct xdev_info *xdev, struct xcmd_info *xcmd)
{
	int i;
	int end = start + limit;
	int step = xreg->step ? xreg->step : 4;
	uint32_t val;
	int32_t rv = 0;

	for (i = start; i < end; i++) {
		uint32_t addr = xreg->addr + (i * step);
		char name[40];
		int l = sprintf(name, "%s_%d",
				xreg->name, i);

		if (xcmd == NULL) {
			val = le32toh(bar[addr / 4]);
		} else {
			xcmd->u.reg.reg = addr;
			rv = reg_read_mmap(xdev, xcmd->u.reg.bar, xcmd, &val);
			if (rv < 0) {
				printf("\n");
				continue;
			}
		}
		printf("[%#7x] %-47s %#-10x %u\n",
			addr, name, val, val);
	}
}

static void dump_regs(uint32_t *bar, struct xreg_info *reg_list, struct xdev_info *xdev, struct xcmd_info *xcmd)
{
	struct xreg_info *xreg = reg_list;
	uint32_t val;
	int32_t rv = 0;

	for (xreg = reg_list; strlen(xreg->name); xreg++) {
		if (!xreg->len) {
			if (xreg->repeat) {
				if (xcmd == NULL)
					print_repeated_reg(bar, xreg, 0, xreg->repeat, NULL, NULL);
				else
					print_repeated_reg(NULL, xreg, 0, xreg->repeat, xdev, xcmd);
			} else {
				uint32_t addr = xreg->addr;
				if (xcmd == NULL) {
					val = le32toh(bar[addr / 4]);
				} else {
					xcmd->u.reg.reg = addr;
					rv = reg_read_mmap(xdev, xcmd->u.reg.bar, xcmd, &val);
					if (rv < 0)
						continue;
				}
				printf("[%#7x] %-47s %#-10x %u\n",
					addr, xreg->name, val, val);
			}
		} else {
			uint32_t addr = xreg->addr;
			if (xcmd == NULL) {
				uint32_t val = le32toh(bar[addr / 4]);
			} else {
				xcmd->u.reg.reg = addr;
				rv = reg_read_mmap(xdev, xcmd->u.reg.bar, xcmd, &val);
				if (rv < 0)
					continue;
			}

			uint32_t v = (val >> xreg->shift) &
					((1 << xreg->len) - 1);

			printf("    %*u:%u %-47s %#-10x %u\n",
				xreg->shift < 10 ? 3 : 2,
				xreg->shift + xreg->len - 1,
				xreg->shift, xreg->name, v, v);
		}
	}
}

static void reg_dump_mmap(struct xdev_info *xdev, unsigned char barno,
			struct xreg_info *reg_list, unsigned int max,  struct xcmd_info *xcmd)
{
	uint32_t *bar;
	char fname[256];

	get_syspath_bar_mmap(fname, xdev->bus, xdev->dev, xdev->func, barno);

	bar = mmap_bar(fname, max, PROT_READ);
	if (!bar) {
		xcmd->op = XNL_CMD_REG_RD;
		xcmd->u.reg.bar = barno;
		dump_regs(NULL, reg_list, xdev, xcmd);
		return;
	}

	dump_regs(bar, reg_list, NULL, NULL);
	munmap(bar, max);
}

static void reg_dump_range(struct xdev_info *xdev, unsigned char barno,
			 unsigned int max, struct xreg_info *reg_list,
			 unsigned int start, unsigned int limit,  struct xcmd_info *xcmd)
{
	struct xreg_info *xreg = reg_list;
	uint32_t *bar;
	char fname[256];

	get_syspath_bar_mmap(fname, xdev->bus, xdev->dev, xdev->func, barno);

	bar = mmap_bar(fname, max, PROT_READ);
	if (!bar) {
		xcmd->op = XNL_CMD_REG_RD;
		xcmd->u.reg.bar = barno;
		for (xreg = reg_list; strlen(xreg->name); xreg++) {
			print_repeated_reg(NULL, xreg, start, limit, xdev, xcmd);
		}
		return;
	}

	for (xreg = reg_list; strlen(xreg->name); xreg++) {
		print_repeated_reg(bar, xreg, start, limit, NULL, NULL);
	}

	munmap(bar, max);
}

static inline void print_seperator(void)
{
	char buffer[81];

	memset(buffer, '#', 80);
	buffer[80] = '\0';

	fprintf(stdout, "%s\n", buffer);
}

int is_valid_addr(unsigned char bar_no, unsigned int reg_addr)
{
	struct xreg_info *rinfo = (bar_no == 0) ? qdma_config_regs:
			qdma_user_regs;
	unsigned int i;
	unsigned int size = (bar_no == 0) ?
			(sizeof(qdma_config_regs) / sizeof(struct xreg_info)):
			(sizeof(qdma_user_regs) / sizeof(struct xreg_info));

	for (i = 0; i < size; i++)
		if (reg_addr == rinfo[i].addr)
			return 1;

	return 0;
}

int proc_reg_cmd(struct xcmd_info *xcmd)
{
	struct xcmd_reg *regcmd = &xcmd->u.reg;
	struct xdev_info xdev;
	int32_t rv = 0;
	unsigned int mask = (1 << XNL_ATTR_PCI_BUS) | (1 << XNL_ATTR_PCI_DEV) |
			(1 << XNL_ATTR_PCI_FUNC) | (1 << XNL_ATTR_DEV_CFG_BAR) |
			(1 << XNL_ATTR_DEV_USR_BAR);
	unsigned int barno;
	int32_t v;

	if ((xcmd->attr_mask & mask) != mask) {
		fprintf(stderr, "%s: device info missing, 0x%x/0x%x.\n",
			__FUNCTION__, xcmd->attr_mask, mask);
		return -EINVAL;
	}

	memset(&xdev, 0, sizeof(struct xdev_info));
	xdev.bus = xcmd->attrs[XNL_ATTR_PCI_BUS];
	xdev.dev = xcmd->attrs[XNL_ATTR_PCI_DEV];
	xdev.func = xcmd->attrs[XNL_ATTR_PCI_FUNC];
	xdev.config_bar = xcmd->attrs[XNL_ATTR_DEV_CFG_BAR];
	xdev.user_bar = xcmd->attrs[XNL_ATTR_DEV_USR_BAR];
	xdev.stm_bar = xcmd->attrs[XNL_ATTR_DEV_STM_BAR];

	barno = (regcmd->sflags & XCMD_REG_F_BAR_SET) ?
			 regcmd->bar : xdev.config_bar;

	switch (xcmd->op) {
	case XNL_CMD_REG_RD:
		rv = reg_read_mmap(&xdev, barno, xcmd, &v);
		if (rv < 0)
			return -1;
		fprintf(stdout, "qdma%05x, %02x:%02x.%02x, bar#%u, 0x%x = 0x%x.\n",
			xcmd->if_bdf, xdev.bus, xdev.dev, xdev.func, barno,
			regcmd->reg, v);
		break;
	case XNL_CMD_REG_WRT:
		v = reg_write_mmap(&xdev, barno, xcmd);
		if (v < 0)
			return -1;
		rv = reg_read_mmap(&xdev, barno, xcmd, &v);
		if (rv < 0)
			return -1;
		fprintf(stdout, "qdma%05x, %02x:%02x.%02x, bar#%u, reg 0x%x -> 0x%x, read back 0x%x.\n",
			xcmd->if_bdf, xdev.bus, xdev.dev, xdev.func, barno,
			regcmd->reg, regcmd->val, v);
		break;
	case XNL_CMD_REG_DUMP:
		print_seperator();
		fprintf(stdout, "###\t\tqdma%05x, pci %02x:%02x.%02x, reg dump\n",
			xcmd->if_bdf, xdev.bus, xdev.dev, xdev.func);
		print_seperator();

		fprintf(stdout, "\nUSER BAR #%d\n", xdev.user_bar);
		reg_dump_mmap(&xdev, xdev.user_bar, qdma_user_regs,
				QDMA_USR_BAR_SIZE, xcmd);

		if (xdev.stm_bar != -1) {
			fprintf(stdout, "\nSTM BAR #%d\n", xdev.stm_bar);
			reg_dump_mmap(&xdev, xdev.stm_bar, stm_regs,
				      STM_BAR_SIZE, xcmd);
		}

		fprintf(stdout, "\nCONFIG BAR #%d\n", xdev.config_bar);
		reg_dump_mmap(&xdev, xdev.config_bar, qdma_config_regs,
				QDMA_CFG_BAR_SIZE, xcmd);
		reg_dump_range(&xdev, xdev.config_bar, QDMA_CFG_BAR_SIZE,
				qdma_dmap_regs, regcmd->range_start,
				regcmd->range_end, xcmd);
		break;
	default:
		break;
	}
	return 0;
}
