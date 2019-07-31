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
#include "qdma_user_reg_dump.h"

#define QDMA_CFG_BAR_SIZE 0xB400
#define QDMA_USR_BAR_SIZE 0x100

/* Macros for reading device capability */
#define QDMA_GLBL2_MISC_CAP                       0x134
#define     QDMA_GLBL2_MM_CMPT_EN_MASK            0x4
#define QDMA_GLBL2_CHANNEL_MDMA                   0x118
#define     QDMA_GLBL2_ST_C2H_MASK                0x10000
#define     QDMA_GLBL2_ST_H2C_MASK                0x20000
#define     QDMA_GLBL2_MM_C2H_MASK                0x100
#define     QDMA_GLBL2_MM_H2C_MASK                0x1

struct xdev_info {
	unsigned char bus;
	unsigned char dev;
	unsigned char func;
	unsigned char config_bar;
	unsigned char user_bar;
};

static struct xreg_info qdma_dmap_regs[] = {
/* QDMA_TRQ_SEL_QUEUE_PF (0x6400) */
	{"DMAP_SEL_INT_CIDX", 0x6400, 512, 0x10, 0, 0, QDMA_MM_ST_MODE},
	{"DMAP_SEL_H2C_DSC_PIDX", 0x6404, 512, 0x10, 0, 0,  QDMA_MM_ST_MODE},
	{"DMAP_SEL_C2H_DSC_PIDX", 0x6408, 512, 0x10, 0, 0, QDMA_MM_ST_MODE},
	{"DMAP_SEL_CMPT_CIDX", 0x640C, 512, 0x10, 0, 0, QDMA_MM_ST_MODE},
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

static void dump_config_regs(uint32_t *bar, struct xreg_info *reg_list, struct xdev_info *xdev, struct xcmd_info *xcmd)
{
	struct xreg_info *xreg = reg_list;
	int32_t rv = 0;
	uint32_t val = 0;
	struct xnl_cb cb;
	int mm_en = 0, st_en = 0, mm_cmpt_en = 0, mailbox_en = 0;
	unsigned int temp = 0;
	unsigned char op;

	/* Get the capabilities of the Device */
	op = xcmd->op;
	xcmd->op = XNL_CMD_DEV_CAP;
	rv = xnl_connect(&cb, 0);
	if (rv < 0)
		return;

	rv = xnl_send_cmd(&cb, xcmd);
	if(rv != 0)
		return;

	xnl_close(&cb);

	xcmd->op = op;
	mm_en = xcmd->u.cap.mm_en;
	mm_cmpt_en = xcmd->u.cap.mm_cmpt_en;
	st_en = xcmd->u.cap.st_en;
	mailbox_en = xcmd->u.cap.mailbox_en;

	for (xreg = reg_list; strlen(xreg->name); xreg++) {

		if ((GET_CAPABILITY_MASK(mm_en,	st_en, mm_cmpt_en,
				mailbox_en) & xreg->mode) == 0)
		    continue;

		if (!xreg->len) {
			if (xreg->repeat) {
				if (xdev == NULL)
					print_repeated_reg(bar, xreg, 0, xreg->repeat, NULL, NULL);
				else
					print_repeated_reg(NULL, xreg, 0, xreg->repeat, xdev, xcmd);
			} else {
				uint32_t addr = xreg->addr;
				if (xdev == NULL) {
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
			if (xdev == NULL) {
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
		if (barno == xdev->config_bar)
			dump_config_regs(NULL, reg_list, xdev, xcmd);
		else
			dump_regs(NULL, reg_list, xdev, xcmd);
		return;
	}

	if (barno == xdev->config_bar)
		dump_config_regs(bar, reg_list, NULL, xcmd);
	else
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
	struct xnl_cb cb;
	unsigned char op;
	int32_t v;

	rv = xnl_connect(&cb, xcmd->vf);
	if (rv < 0)
		return -EINVAL;

	op = xcmd->op;
	xcmd->op = XNL_CMD_DEV_INFO;
	rv = xnl_send_cmd(&cb, xcmd);
	xcmd->op = op;

	memset(&xdev, 0, sizeof(struct xdev_info));
	xdev.bus = (xcmd->if_bdf >> 12);
	xdev.dev = ((xcmd->if_bdf >> 4) & 0x1F);
	xdev.func = (xcmd->if_bdf & 0x07);
	xdev.config_bar = xcmd->attrs[XNL_ATTR_DEV_CFG_BAR];
	xdev.user_bar = xcmd->attrs[XNL_ATTR_DEV_USR_BAR];

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
