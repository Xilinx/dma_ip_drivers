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

#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <sys/ioctl.h>
#include </usr/include/pthread.h>
#include "dmautils.h"
#include "qdma_nl.h"

static struct option const long_opts[] = {
	{"config", required_argument, NULL, 'c'},
	{0, 0, 0, 0}
};

static void prep_pci_dump(void);

static void usage(const char *name)
{
	int i = 0;
	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

	fprintf(stdout, "  -%c (--%s) config file that has configration for IO\n",
		long_opts[i].val, long_opts[i].name);
}

static unsigned int num_trailing_blanks(char *word)
{
    unsigned int i = 0;
    unsigned int slen = strlen(word);

    if (!slen) return 0;
    while (isspace(word[slen - i - 1])) {
        i++;
    }

    return i;
}

static char * strip_blanks(char *word, long unsigned int *banlks)
{
    char *p = word;
    unsigned int i = 0;

    while (isblank(p[0])) {
	p++;
	i++;
    }
    if (banlks)
	*banlks = i;

    return p;
}

static unsigned int copy_value(char *src, char *dst, unsigned int max_len)
{
    char *p = src;
    unsigned int i = 0;

    while (max_len && !isspace(p[0])) {
        dst[i] = p[0];
        p++;
        i++;
        max_len--;
    }

    return i;
}

static char * strip_comments(char *word)
{
    size_t numblanks;
    char *p = strip_blanks(word, &numblanks);

    if (p[0] == '#')
	return NULL;
    else
	p = strtok(word, "#");

    return p;
}

#define PCI_DUMP_CMD_LEN 100
#define QDMA_GLBL_MAX_ENTRIES  (16)

enum q_mode {
	Q_MODE_MM,
	Q_MODE_ST,
	Q_MODES
};

enum q_dir {
	Q_DIR_H2C,
	Q_DIR_C2H,
	Q_DIR_BI,
	Q_DIRS
};


struct io_info {
	int pid;
	char q_name[20];
	char trig_mode[10];
	unsigned char q_ctrl;
	unsigned int q_added;
	unsigned int q_started;
	int fd;
	unsigned int pf;
	unsigned int qid;
	enum q_mode mode;
	enum q_dir dir;
	unsigned int idx_tmr;
	unsigned int idx_cnt;
	unsigned int idx_rngsz;
	unsigned int pfetch_en;
	unsigned int pkt_sz;
	unsigned int cmptsz;
	unsigned int thread_id;
};

#define container_of(ptr, type, member) ({                      \
        const struct iocb *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

static unsigned int io_exit = 0;
static unsigned int force_exit = 0;
static unsigned int num_q = 0;
static unsigned int pkt_sz = 0;
static unsigned int tsecs = 0;
struct io_info info[8];
static char cfg_name[20];
static unsigned int pci_bus = 0;
static unsigned int pci_dev = 0;
static unsigned int vf_perf = 0;
static char *dmactl_dev_prefix_str;
char *pf_dmactl_prefix_str = "qdma";
char *vf_dmactl_prefix_str = "qdmavf";
int base_pid;
enum q_mode mode = Q_MODE_ST;
enum q_dir dir = Q_DIR_BI;
unsigned int num_pf = 0;
unsigned int pf_start = 0;
unsigned int q_start = 0;
unsigned int idx_rngsz = 0;
unsigned int idx_tmr = 0;
unsigned int idx_cnt = 0;
unsigned int pfetch_en = 0;
unsigned int cmptsz = 0;
char trigmode[10];
char pci_dump[PCI_DUMP_CMD_LEN];
unsigned int dump_en = 0;
static struct timespec g_ts_start;
int *child_pid_lst = NULL;
unsigned int glbl_rng_sz[QDMA_GLBL_MAX_ENTRIES];

static int setup_thrd_env(struct io_info *_info, unsigned char is_new_fd);

static int arg_read_int(char *s, uint32_t *v)
{
    char *p = NULL;


    *v = strtoul(s, &p, 0);
    if (*p && (*p != '\n') && !isblank(*p)) {
	printf("Error:something not right%s %s %s",s, p, isblank(*p)? "true": "false");
        return -EINVAL;
    }
    return 0;
}

static int arg_read_int_array(char *s, unsigned int *v, unsigned int max_arr_size)
{
    unsigned int slen = strlen(s);
    unsigned int trail_blanks = num_trailing_blanks(s);
    char *str = (char *)malloc(slen - trail_blanks + 1);
    char *elem;
    int cnt = 0;

    memset(str, '\0', slen + 1);
    strncpy(str, s + 1, slen - trail_blanks - 2);
    str[slen] = '\0';

    elem = strtok(str, " ,");/* space or comma separated */
    while (elem != NULL) {
	    int ret;

        ret = arg_read_int(elem, &v[cnt]);
        if (ret < 0) {
            printf("ERROR: Invalid array element %sin %s\n", elem, s);
            exit(0);
        }
        cnt++;
        elem = strtok(NULL, " ,");
        if (cnt > (int)max_arr_size) { /* to avoid out of bounds */
            printf("ERROR: More than expected number of elements in %s - expected = %u\n",
                   str, max_arr_size);
            exit(0);
        }
    }
    free(str);

    return cnt;
}

static int get_array_len(char *s)
{
    int i, len = 0;

    if (strlen(s) < 2)
        return -EINVAL;
    if ((s[0] != '(') && (s[strlen(s) - 1] != ')'))
        return -EINVAL;
    if ((s[0] == '(') && (s[1] == ')'))
        return 0;
    for (i = 0; i < (int)strlen(s); i++) {
        if ((s[i] == ' ') || (s[i] == ',')) /* space or comma separated */
                len++;
        if (s[i] == ')')
            break;
    }

    return (len + 1);

}

static void dump_thrd_info(struct io_info *_info) {
	printf("q_name = %s\n", info->q_name);
	printf("dir = %d\n", _info->dir);
	printf("mode = %d\n", _info->mode);
	printf("idx_cnt = %u\n", _info->idx_cnt);
	printf("idx_rngsz = %u\n", _info->idx_rngsz);
	printf("idx_tmr = %u\n", _info->idx_tmr);
	printf("pf = %x\n", _info->pf);
	printf("qid = %u\n", _info->qid);
	printf("fd = %d\n", _info->fd);
	printf("trig_mode = %s\n", _info->trig_mode);
	printf("q_ctrl = %u\n", _info->q_ctrl);
	printf("q_added = %u\n", _info->q_added);
	printf("q_started = %u\n", _info->q_started);
}

static void xnl_dump_response(const char *resp)
{
	printf("%s", resp);
}

static int qdma_register_write(unsigned char is_vf,
		unsigned int pf, int bar, unsigned long reg,
		unsigned long value)
{
	struct xcmd_info xcmd;
	struct xcmd_reg *regcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));

	regcmd = &xcmd.req.reg;
	xcmd.op = XNL_CMD_REG_WRT;
	xcmd.vf = is_vf;
	xcmd.if_bdf = pf;
	xcmd.log_msg_dump = xnl_dump_response;
	regcmd->bar = bar;
	regcmd->reg = reg;
	regcmd->val = value;
	regcmd->sflags = XCMD_REG_F_BAR_SET |
		XCMD_REG_F_REG_SET |
		XCMD_REG_F_VAL_SET;

	ret = qdma_reg_write(&xcmd);
	if (ret < 0)
		printf("QDMA_REG_WRITE Failed, ret :%d\n", ret);

	return ret;
}

static int qdma_register_read(unsigned char is_vf,
		unsigned int pf, int bar, unsigned long reg,
		unsigned int *reg_val)
{
	struct xcmd_info xcmd;
	struct xcmd_reg *regcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));

	regcmd = &xcmd.req.reg;
	xcmd.op = XNL_CMD_REG_RD;
	xcmd.vf = is_vf;
	xcmd.if_bdf = pf;
	xcmd.log_msg_dump = xnl_dump_response;
	regcmd->bar = bar;
	regcmd->reg = reg;
	regcmd->sflags = XCMD_REG_F_BAR_SET |
		XCMD_REG_F_REG_SET;

	ret = qdma_reg_read(&xcmd);
	if (ret < 0)
		printf("QDMA_REG_READ Failed, ret :%d\n", ret);

	*reg_val = regcmd->val;

	return ret;
}

static int qdma_prepare_reg_dump(struct xcmd_info *xcmd,
		unsigned char is_vf, struct io_info *info)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_REG_DUMP;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;

	return 0;
}

static int qdma_registers_dump(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_reg_dump(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	ret = qdma_reg_dump(&xcmd);
	if (ret < 0)
		printf("Q_ failed, ret :%d\n", ret);

	return ret;
}

static int qdma_prepare_q_add(struct xcmd_info *xcmd,
		unsigned char is_vf, struct io_info *info)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_ADD;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;
	qparm->idx = info->qid;
	qparm->num_q = 1;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;

	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	qparm->sflags = qparm->flags;

	return 0;
}

static int qdma_add_queue(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_add(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	ret = qdma_q_add(&xcmd);
	if (ret < 0)
		printf("Q_ADD failed, ret :%d\n", ret);

	return ret;
}

static int qdma_prepare_q_start(struct xcmd_info *xcmd,
		unsigned char is_vf, struct io_info *info)
{
	struct xcmd_q_parm *qparm;


	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}
	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_START;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;
	qparm->idx = info->qid;
	qparm->num_q = 1;
	qparm->fetch_credit = Q_ENABLE_C2H_FETCH_CREDIT;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;

	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	qparm->qrngsz_idx = info->idx_rngsz;

	if ((info->dir == Q_DIR_C2H) && (info->mode == Q_MODE_ST)) {
		if (cmptsz)
			qparm->cmpt_entry_size = info->cmptsz;
		else
			qparm->cmpt_entry_size = XNL_ST_C2H_CMPT_DESC_SIZE_8B;
		qparm->cmpt_tmr_idx = info->idx_tmr;
		qparm->cmpt_cntr_idx = info->idx_cnt;

		if (!strcmp(info->trig_mode, "every"))
			qparm->cmpt_trig_mode = 1;
		else if (!strcmp(info->trig_mode, "usr_cnt"))
			qparm->cmpt_trig_mode = 2;
		else if (!strcmp(info->trig_mode, "usr"))
			qparm->cmpt_trig_mode = 3;
		else if (!strcmp(info->trig_mode, "usr_tmr"))
			qparm->cmpt_trig_mode=4;
		else if (!strcmp(info->trig_mode, "cntr_tmr"))
			qparm->cmpt_trig_mode=5;
		else if (!strcmp(info->trig_mode, "dis"))
			qparm->cmpt_trig_mode = 0;
		else {
			printf("Error: unknown q trigmode %s.\n", info->trig_mode);
			return -EINVAL;
		}

		if (pfetch_en)
			qparm->flags |= XNL_F_PFETCH_EN;
	}

	qparm->flags |= (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
			XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
			XNL_F_FETCH_CREDIT);

	qparm->sflags |= (1 << QPARM_PING_PONG_EN);

	return 0;
}

static int qdma_start_queue(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_start(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	ret = qdma_q_start(&xcmd);
	if (ret < 0)
		printf("Q_START failed, ret :%d\n", ret);

	return ret;
}

static int qdma_prepare_q_stop(struct xcmd_info *xcmd, unsigned char is_vf,
		struct io_info *info)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd)
		return -EINVAL;

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_STOP;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;
	qparm->idx = info->qid;
	qparm->num_q = 1;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;

	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	return 0;
}

static int qdma_stop_queue(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_stop(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	ret = qdma_q_stop(&xcmd);
	if (ret < 0)
		printf("Q_STOP failed, ret :%d\n", ret);

	return ret;
}

static int qdma_prepare_q_del(struct xcmd_info *xcmd, unsigned char is_vf,
		struct io_info *info)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_DEL;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;
	qparm->idx = info->qid;
	qparm->num_q = 1;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;

	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	return 0;
}

static int qdma_del_queue(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_del(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	ret = qdma_q_del(&xcmd);
	if (ret < 0)
		printf("Q_DEL failed, ret :%d\n", ret);

	return ret;
}

static int qdma_prepare_q_dump(struct xcmd_info *xcmd, unsigned char is_vf,
		struct io_info *info)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_DUMP;
	xcmd->vf = is_vf;
	xcmd->if_bdf = info->pf;
	xcmd->log_msg_dump = xnl_dump_response;
	qparm->idx = info->qid;
	qparm->num_q = 1;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;

	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	return 0;
}

static int qdma_dump_queue(unsigned char is_vf, struct io_info *info)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_dump(&xcmd, is_vf, info);
	if (ret < 0)
		return ret;

	return qdma_q_dump(&xcmd);
}


static void create_thread_info(void)
{
	unsigned int base = 0;
	unsigned int q_ctrl = 1;
	unsigned int i, k;
	int last_fd = -1;
	unsigned char is_new_fd = 1;
	struct io_info *_info = info;

	prep_pci_dump();

	if ((mode == Q_MODE_ST) && (dir != Q_DIR_H2C)) {
		qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x50, cmptsz);
	}

	for (k = 0; k < num_pf; k++) {
		for (i = 0 ; i < num_q; i++) {
			is_new_fd = 1;
			snprintf(_info[base].q_name, 20, "%s%02x%02x%01x-%s-%u",
			dmactl_dev_prefix_str, pci_bus, pci_dev,
			pf_start+k, (mode == Q_MODE_MM) ? "MM" : "ST", q_start + i);

			_info[base].dir = Q_DIR_H2C;
			_info[base].mode = mode;
			_info[base].idx_rngsz = idx_rngsz;
			_info[base].pf = (pci_bus << 12) | (pci_dev << 4) | (pf_start + k);
			_info[base].qid = q_start + i;
			_info[base].q_ctrl = q_ctrl;
			_info[base].pkt_sz = pkt_sz;
			last_fd = setup_thrd_env(&_info[base], is_new_fd);
			_info[base].fd = last_fd;
			is_new_fd = 0;
			// Adding the Queue in the C2H direction also
			_info[base].dir = Q_DIR_C2H;
			_info[base].pfetch_en = pfetch_en;
			_info[base].idx_cnt = idx_cnt;
			_info[base].idx_tmr = idx_tmr;
			_info[base].cmptsz = cmptsz;
			strncpy(_info[base].trig_mode, trigmode, 10);
			last_fd = setup_thrd_env(&_info[base], is_new_fd);
			_info[base].thread_id = base;
			base++;
		}

	}
	// Reset ST Traffic generator
	qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x08, 0);
	// Set the loopback bit for the ST traffic generator
	qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x08, 1);

	usleep(1000);

}

static void parse_config_file(const char *cfg_fname)
{
	char *linebuf = NULL;
	char *realbuf;
	FILE *fp;
	size_t linelen = 0;
	size_t numread;
	size_t numblanks;
	unsigned int linenum = 0;
	char *config, *value;
	unsigned int dir_factor = 1;
	char rng_sz[100] = {'\0'};
	char rng_sz_path[512] = {'\0'};
    	int rng_sz_fd, ret = 0;

	fp = fopen(cfg_fname, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		linenum++;
		linebuf = strip_comments(linebuf);
		if (!linebuf)
			continue;
		realbuf = strip_blanks(linebuf, &numblanks);
		linelen -= numblanks;
		if (0 == linelen)
			continue;
		config = strtok(realbuf, "=");
		value = strtok(NULL, "=");
		if (!strncmp(config, "name", 3)) {
			copy_value(value, cfg_name, 20);
		} else if (!strncmp(config, "pf_range", 8)) {
			char *pf_range_start = strtok(value, ":");
			char *pf_range_end = strtok(NULL, ":");
			unsigned int start;
			unsigned int end;
			if (arg_read_int(pf_range_start, &start)) {
				printf("Error: Invalid pf range start:%s\n", pf_range_start);
				goto prase_cleanup;
			}
			if (arg_read_int(pf_range_end, &end)) {
				printf("Error: Invalid pf range end:%s\n", pf_range_end);
				goto prase_cleanup;
			}

			pf_start = start;
			num_pf = end - start + 1;
		} else if (!strncmp(config, "q_range", 7)) {
			char *q_range_start = strtok(value, ":");
			char *q_range_end = strtok(NULL, ":");
			unsigned int start;
			unsigned int end;
			if (arg_read_int(q_range_start, &start)) {
				printf("Error: Invalid q range start:%s\n", q_range_start);
				goto prase_cleanup;
			}
			if (arg_read_int(q_range_end, &end)) {
				printf("Error: Invalid q range end:%s\n", q_range_end);
				goto prase_cleanup;
			}

			q_start = start;
			num_q = end - start + 1;
		} else if (!strncmp(config, "rngidx", 6)) {
			if (arg_read_int(value, &idx_rngsz)) {
				printf("Error: Invalid idx_rngsz:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "tmr_idx", 7)) {
			if (arg_read_int(value, &idx_tmr)) {
				printf("Error: Invalid idx_tmr:%s\n", value);
				goto prase_cleanup;
			}
		}
		if (!strncmp(config, "cntr_idx", 8)) {
		    if (arg_read_int(value, &idx_cnt)) {
			printf("Error: Invalid idx_cnt:%s\n", value);
			goto prase_cleanup;
		    }
		} else if (!strncmp(config, "pfetch_en", 9)) {
		    if (arg_read_int(value, &pfetch_en)) {
			printf("Error: Invalid pfetch_en:%s\n", value);
			goto prase_cleanup;
		    }
		} else if (!strncmp(config, "cmptsz", 5)) {
		    if (arg_read_int(value, &cmptsz)) {
			printf("Error: Invalid cmptsz:%s\n", value);
			goto prase_cleanup;
		    }
		} else if (!strncmp(config, "dump_en", 5)) {
		    if (arg_read_int(value, &dump_en)) {
			printf("Error: Invalid dump_en:%s\n", value);
			goto prase_cleanup;
		    }
		} else if (!strncmp(config, "trig_mode", 9)) {
		    copy_value(value, trigmode, 10);
		} else if (!strncmp(config, "runtime", 9)) {
			if (arg_read_int(value, &tsecs)) {
				printf("Error: Invalid tsecs:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "pkt_sz", 6)) {
			if (arg_read_int(value, &pkt_sz)) {
				printf("Error: Invalid pkt_sz:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "pci_bus", 7)) {
			char *p;

			pci_bus = strtoul(value, &p, 16);
			if (*p && (*p != '\n')) {
				printf("Error: bad parameter \"%s\", integer expected", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "pci_dev", 7)) {
			char *p;

			pci_dev = strtoul(value, &p, 16);
			if (*p && (*p != '\n')) {
				printf("Error: bad parameter \"%s\", integer expected", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "vf_perf", 7)) {
			char *p;

			vf_perf = strtoul(value, &p, 16);
			if (*p && (*p != '\n')) {
				printf("Error: bad parameter \"%s\", integer expected", value);
				goto prase_cleanup;
			}

		}
	}
	fclose(fp);
	if (vf_perf == 0) {
		dmactl_dev_prefix_str = pf_dmactl_prefix_str;
	} else {
		dmactl_dev_prefix_str = vf_dmactl_prefix_str;
	}

	if (!pci_bus && !pci_dev) {
		printf("Error: PCI bus information not provided\n");
		exit(1);
	}

	snprintf(rng_sz_path, 200,"dma-ctl %s%05x global_csr | grep Global| cut -d : -f 2 > glbl_rng_sz",
			 dmactl_dev_prefix_str, (pci_bus << 12) | (pci_dev << 4) | pf_start);
	printf("%s\n", rng_sz_path);
	system(rng_sz_path);
	memset(rng_sz_path, 0, 200);
	snprintf(rng_sz_path, 200, "glbl_rng_sz");

	rng_sz_fd = open(rng_sz_path, O_RDONLY);
	if (rng_sz_fd < 0) {
		printf("Could not open %s\n", rng_sz_path);
		exit(1);
	}
	ret = read(rng_sz_fd, &rng_sz[1], 100);
	if (ret < 0) {
		printf("Error: Could not read the file\n");
		exit(1);
	}
	close(rng_sz_fd);
	/* below 2 line a work around to use arg_read_int_array */
	rng_sz[0] = ' ';
	rng_sz[strlen(rng_sz)] = ' ';
	ret = arg_read_int_array(rng_sz, glbl_rng_sz, QDMA_GLBL_MAX_ENTRIES);
	if (ret <= 0) {
		printf("Error: Invalid ring size array:%s\n", rng_sz);
		exit(1);
	}

	create_thread_info();

	return;

prase_cleanup:
	fclose(fp);
	exit(-1);
}

/* Subtract timespec t2 from t1
 *
 * Both t1 and t2 must already be normalized
 * i.e. 0 <= nsec < 1000000000
 */
static int timespec_check(struct timespec *t)
{
	if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
		return -1;
	return 0;

}

static void timespec_sub(struct timespec *t1, struct timespec *t2)
{
	if (timespec_check(t1) < 0) {
		fprintf(stderr, "invalid time #1: %lld.%.9ld.\n",
			(long long)t1->tv_sec, t1->tv_nsec);
		return;
	}
	if (timespec_check(t2) < 0) {
		fprintf(stderr, "invalid time #2: %lld.%.9ld.\n",
			(long long)t2->tv_sec, t2->tv_nsec);
		return;
	}
	t1->tv_sec -= t2->tv_sec;
	t1->tv_nsec -= t2->tv_nsec;
	if (t1->tv_nsec >= 1000000000) {
		t1->tv_sec++;
		t1->tv_nsec -= 1000000000;
	} else if (t1->tv_nsec < 0) {
		t1->tv_sec--;
		t1->tv_nsec += 1000000000;
	}
}

static void io_proc_cleanup(struct io_info *_info)
{
	int s;
	char reg_cmd[100] = {'\0'};

	if (dump_en) {
		s = qdma_dump_queue(vf_perf, _info);
		if (s < 0) {
			printf("Failed: queue_dump\nerrcode = %d\n", s);
		}
	}

	s = qdma_stop_queue(vf_perf, _info);
	if (s < 0) {
		printf("Failed: queue_stop\nerrcode = %d\n", s);
	}
	_info->q_started = 0;

	system(reg_cmd);

	s = qdma_del_queue(vf_perf, _info);
	if (s < 0) {
		printf("Failed: queue_del\nerrcode = %d\n", s);
	}
	_info->q_added = 0;
}

static void *io_thread(void *argp)
{

	struct io_info *_info = (struct io_info *)argp;
	char *buffer = NULL;

	unsigned int io_sz = _info->pkt_sz;

	posix_memalign((void **)&buffer, 4096 /*alignment */ , io_sz + 4096);
	if (!buffer) {
		printf("OOM \n");
		return NULL;
	}

	do {

		struct timespec ts_cur;

		if (tsecs) {
			if (clock_gettime(CLOCK_MONOTONIC, &ts_cur) != 0)
				break;
			timespec_sub(&ts_cur, &g_ts_start);
			if (ts_cur.tv_sec >= tsecs)
				break;
		}

		write(_info->fd, buffer ,io_sz);
		read(_info->fd, buffer ,io_sz);


	} while (tsecs && !force_exit);

	free(buffer);

	return NULL;
}

static void prep_pci_dump(void)
{
    memset(pci_dump, '\0', PCI_DUMP_CMD_LEN);
    snprintf(pci_dump, PCI_DUMP_CMD_LEN, "lspci -s %02x:%02x.%01x -vvv", pci_bus, pci_dev, pf_start);
}

static int setup_thrd_env(struct io_info *_info, unsigned char is_new_fd)
{
	int s;

	/* add queue */
	s = qdma_add_queue(vf_perf, _info);
	if (s < 0) {
		exit(1);
	}
	_info->q_added++;

	/* start queue */
	/* start queue */
	s = qdma_start_queue(vf_perf, _info);
	if (s < 0)
		exit(1);
	_info->q_started++;

	if (is_new_fd) {
		char node[25] = {'\0'};

		snprintf(node, 25, "/dev/%s", _info->q_name);
		_info->fd = open(node, O_RDWR);
		if (_info->fd < 0) {
			printf("Error: Cannot find %s\n", node);
			exit(1);
		}
	}

	return _info->fd;
}

static void dump_result()
{
	char reg_cmd[100] = {'\0'};

	snprintf(reg_cmd, 100, "dma-ctl %s%05x stat",
		 dmactl_dev_prefix_str, info[0].pf);
	system(reg_cmd);
	memset(reg_cmd, 0, 100);
	snprintf(reg_cmd, 100, "dma-ctl %s%05x stat clear",
		 dmactl_dev_prefix_str, info[0].pf);
	system(reg_cmd);
	memset(reg_cmd, 0, 100);

}

static int is_valid_fd(int fd)
{
    return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

static void cleanup(void) {

	if (getpid() != base_pid)
		return;

	for (int i=0;i<num_q; i++) {
		if (info[i].fd > 0 && is_valid_fd(info[i].fd))
			close(info[i].fd);
		info[i].dir = Q_DIR_H2C;
		io_proc_cleanup(&info[i]);
		info[i].dir = Q_DIR_C2H;
		io_proc_cleanup(&info[i]);
	}
	dump_result();
}

int main(int argc, char *argv[])
{

	int cmd_opt;
	char *cfg_fname = NULL;
	unsigned int i = 0;
	cpu_set_t set;
	while ((cmd_opt = getopt_long(argc, argv, "vhxc:c:", long_opts,
			    NULL)) != -1) {
		switch (cmd_opt) {
		case 0:
			/* long option */
			break;
		case 'c':
			/* config file name */
			cfg_fname = strdup(optarg);
			break;
		default:
			usage(argv[0]);
			exit(0);
			break;
		}
	}
	if (cfg_fname == NULL)
		return 1;

	parse_config_file(cfg_fname);
	atexit(cleanup);

	printf("dmautils(%u) threads\n", num_q);
	child_pid_lst = calloc(num_q, sizeof(int));
	base_pid = getpid();

	CPU_ZERO(&set);
	CPU_SET(0, &set);
        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        	printf("setaffinity for thrd%u failed\n", info[i].thread_id);

	for (i = 0; i < num_q; i++) {
		if (getpid() == base_pid)
			child_pid_lst[i] = fork();
		else
			break;
	}

	clock_gettime(CLOCK_MONOTONIC, &g_ts_start);
	if (getpid() == base_pid) {
		for(i = 0; i < num_q; i++) {
			waitpid(child_pid_lst[i], NULL, 0);
		}
		free(child_pid_lst);
	        child_pid_lst = NULL;

		qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x08, 0);

	} else {
		info[i-1].pid = getpid();
		io_thread(&info[i-1]);
	}
	return 0;
}

