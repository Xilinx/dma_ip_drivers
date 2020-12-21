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

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_parse.h"
#include "qdma_nl.h"
#include "version.h"

static int read_range(int argc, char *argv[], int i, unsigned int *v1,
			unsigned int *v2);

static const char *progname;
#define DESC_SIZE_64B		3

#define Q_ADD_ATTR_IGNORE_MASK ~((1 << QPARM_IDX)  | \
				(1 << QPARM_MODE)  | \
				(1 << QPARM_DIR))
#define Q_START_ATTR_IGNORE_MASK ((1 << QPARM_MODE)  | \
				(1 << QPARM_DESC) | \
				(1 << QPARM_CMPT))
#define Q_STOP_ATTR_IGNORE_MASK ~((1 << QPARM_IDX) | \
				(1 << QPARM_DIR))
#define Q_DEL_ATTR_IGNORE_MASK ~((1 << QPARM_IDX)  | \
				(1 << QPARM_DIR))
#define Q_DUMP_ATTR_IGNORE_MASK ~((1 << QPARM_IDX)  | \
				(1 << QPARM_DIR) | \
				(1 << QPARM_DESC) | \
				(1 << QPARM_CMPT))
#define Q_DUMP_PKT_ATTR_IGNORE_MASK ~((1 << QPARM_IDX)  | \
					(1 << QPARM_DIR))
#define Q_H2C_ATTR_IGNORE_MASK ((1 << QPARM_C2H_BUFSZ_IDX) | \
				(1 << QPARM_CMPT_TMR_IDX) | \
				(1 << QPARM_CMPT_CNTR_IDX) | \
				(1 << QPARM_CMPT_TRIG_MODE) | \
				(1 << QPARM_CMPTSZ))
#define Q_CMPT_READ_ATTR_IGNORE_MASK ~((1 << QPARM_IDX) | \
				      (1 << QPARM_DIR))
#define Q_ADD_FLAG_IGNORE_MASK ~(XNL_F_QMODE_ST | \
				XNL_F_QMODE_MM | \
				XNL_F_QDIR_BOTH | XNL_F_Q_CMPL)
#define Q_START_FLAG_IGNORE_MASK (XNL_F_QMODE_ST | \
				XNL_F_QMODE_MM)
#define Q_STOP_FLAG_IGNORE_MASK ~(XNL_F_QMODE_ST | \
					XNL_F_QMODE_MM | \
					XNL_F_QDIR_BOTH | XNL_F_Q_CMPL)
#define Q_DEL_FLAG_IGNORE_MASK  ~(XNL_F_QMODE_ST | \
					XNL_F_QMODE_MM | \
					XNL_F_QDIR_BOTH | XNL_F_Q_CMPL)
#define Q_DUMP_FLAG_IGNORE_MASK  ~(XNL_F_QMODE_ST | \
					XNL_F_QMODE_MM | \
					XNL_F_QDIR_BOTH | XNL_F_Q_CMPL)
#define Q_DUMP_PKT_FLAG_IGNORE_MASK ~(XNL_F_QMODE_ST | \
					XNL_F_QMODE_MM | \
					XNL_F_QDIR_C2H)
#define Q_H2C_FLAG_IGNORE_MASK  (XNL_F_C2H_CMPL_INTR_EN | \
				XNL_F_CMPL_UDD_EN)

#define Q_CMPT_READ_FLAG_IGNORE_MASK  ~(XNL_F_QMODE_ST | \
					XNL_F_QMODE_MM | \
					XNL_F_QDIR_BOTH | XNL_F_Q_CMPL)

#ifdef ERR_DEBUG
char *qdma_err_str[qdma_errs] = {
	"err_ram_sbe",
	"err_ram_dbe",
	"err_dsc",
	"err_trq",
	"err_h2c_mm_0",
	"err_h2c_mm_1",
	"err_c2h_mm_0",
	"err_c2h_mm_1",
	"err_c2h_st",
	"ind_ctxt_cmd_err",
	"err_bdg",
	"err_h2c_st",
	"poison",
	"ur_ca",
	"param",
	"addr",
	"tag",
	"flr",
	"timeout",
	"dat_poison",
	"flr_cancel",
	"dma",
	"dsc",
	"rq_cancel",
	"dbe",
	"sbe",
	"unmapped",
	"qid_range",
	"vf_access_err",
	"tcp_timeout",
	"mty_mismatch",
	"len_mismatch",
	"qid_mismatch",
	"desc_rsp_err",
	"eng_wpl_data_par_err",
	"msi_int_fail",
	"err_desc_cnt",
	"portid_ctxt_mismatch",
	"portid_byp_in_mismatch",
	"cmpt_inv_q_err",
	"cmpt_qfull_err",
	"cmpt_cidx_err",
	"cmpt_prty_err",
	"fatal_mty_mismatch",
	"fatal_len_mismatch",
	"fatal_qid_mismatch",
	"timer_fifo_ram_rdbe",
	"fatal_eng_wpl_data_par_err",
	"pfch_II_ram_rdbe",
	"cmpt_ctxt_ram_rdbe",
	"pfch_ctxt_ram_rdbe",
	"desc_req_fifo_ram_rdbe",
	"int_ctxt_ram_rdbe",
	"cmpt_coal_data_ram_rdbe",
	"tuser_fifo_ram_rdbe",
	"qid_fifo_ram_rdbe",
	"payload_fifo_ram_rdbe",
	"wpl_data_par_err",
	"zero_len_desc_err",
	"csi_mop_err",
	"no_dma_dsc_err",
	"sb_mi_h2c0_dat",
	"sb_mi_c2h0_dat",
	"sb_h2c_rd_brg_dat",
	"sb_h2c_wr_brg_dat",
	"sb_c2h_rd_brg_dat",
	"sb_c2h_wr_brg_dat",
	"sb_func_map",
	"sb_dsc_hw_ctxt",
	"sb_dsc_crd_rcv",
	"sb_dsc_sw_ctxt",
	"sb_dsc_cpli",
	"sb_dsc_cpld",
	"sb_pasid_ctxt_ram",
	"sb_timer_fifo_ram",
	"sb_payload_fifo_ram",
	"sb_qid_fifo_ram",
	"sb_tuser_fifo_ram",
	"sb_wrb_coal_data_ram",
	"sb_int_qid2vec_ram",
	"sb_int_ctxt_ram",
	"sb_desc_req_fifo_ram",
	"sb_pfch_ctxt_ram",
	"sb_wrb_ctxt_ram",
	"sb_pfch_ll_ram",
	"sb_h2c_pend_fifo",
	"db_mi_h2c0_dat",
	"db_mi_c2h0_dat",
	"db_h2c_rd_brg_dat",
	"db_h2c_wr_brg_dat",
	"db_c2h_rd_brg_dat",
	"db_c2h_wr_brg_dat",
	"db_func_map",
	"db_dsc_hw_ctxt",
	"db_dsc_crd_rcv",
	"db_dsc_sw_ctxt",
	"db_dsc_cpli",
	"db_dsc_cpld",
	"db_pasid_ctxt_ram",
	"db_timer_fifo_ram",
	"db_payload_fifo_ram",
	"db_qid_fifo_ram",
	"db_tuser_fifo_ram",
	"db_wrb_coal_data_ram",
	"db_int_qid2vec_ram",
	"db_int_ctxt_ram",
	"db_desc_req_fifo_ram",
	"db_pfch_ctxt_ram",
	"db_wrb_ctxt_ram",
	"db_pfch_ll_ram",
	"db_h2c_pend_fifo",
};
#endif

static void __attribute__((noreturn)) usage(FILE *fp)
{
	fprintf(fp, "Usage: %s [dev|qdma[vf]<N>] [operation] \n", progname);
	fprintf(fp, "\tdev [operation]: system wide FPGA operations\n");
	fprintf(fp, 
		"\t\tlist                    list all qdma functions\n");
	fprintf(fp,
		"\tqdma[N] [operation]: per QDMA FPGA operations\n");
	fprintf(fp,
		"\t\tcap....                 lists the Hardware and Software version and capabilities\n"
		"\t\tstat                    statistics of qdma[N] device\n"
		"\t\tstat clear              clear all statistics data of qdma[N} device\n"
		"\t\tglobal_csr              dump the Global CSR of qdma[N} device\n"
		"\t\tq list                  list all queues\n"
		"\t\tq add idx <N> [mode <mm|st>] [dir <h2c|c2h|bi|cmpt>] - add a queue\n"
		"\t\t                                                  *mode default to mm\n"
		"\t\t                                                  *dir default to h2c\n"
			"\t\tq add list <start_idx> <num_Qs> [mode <mm|st>] [dir <h2c|c2h|bi|cmpt>] - add multiple queues at once\n"
	        "\t\tq start idx <N> [dir <h2c|c2h|bi|cmpt>] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>]\n"
		   "                                    [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [sw_desc_sz <3>]\n"
	        "                                    [mm_chn <0|1>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en] [dis_cmpl_status]\n"
	        "                                    [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en]\n"
			"                                    [cmpl_ovf_dis] [fetch_credit  <h2c|c2h|bi|none>] [dis_cmpl_status] [c2h_cmpl_intr_en] [aperture_sz <aperture size power of 2>]- start a single queue\n"
	        "\t\tq start list <start_idx> <num_Qs> [dir <h2c|c2h|bi|cmpt>] [idx_bufsz <0:15>] [idx_tmr <0:15>]\n"
			"                                    [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [sw_desc_sz <3>]\n"
	        "                                    [mm_chn <0|1>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en] [dis_cmpl_status]\n"
	        "                                    [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [cmpl_ovf_dis]\n"
			"                                    [fetch_credit <h2c|c2h|bi|none>] [dis_cmpl_status] [c2h_cmpl_intr_en] [aperture_sz <aperture size power of 2>]- start multiple queues at once\n"
	        "\t\tq stop idx <N> dir [<h2c|c2h|bi|cmpt>] - stop a single queue\n"
	        "\t\tq stop list <start_idx> <num_Qs> dir [<h2c|c2h|bi|cmpt>] - stop list of queues at once\n"
	        "\t\tq del idx <N> dir [<h2c|c2h|bi|cmpt>] - delete a queue\n"
	        "\t\tq del list <start_idx> <num_Qs> dir [<h2c|c2h|bi|cmpt>] - delete list of queues at once\n"
		"\t\tq dump idx <N> dir [<h2c|c2h|bi|cmpt>]   dump queue param\n"
		"\t\tq dump list <start_idx> <num_Qs> dir [<h2c|c2h|bi|cmpt>] - dump queue param\n"
		"\t\tq dump idx <N> dir [<h2c|c2h|bi>] desc <x> <y> - dump desc ring entry x ~ y\n"
		"\t\tq dump list <start_idx> <num_Qs> dir [<h2c|c2h|bi>] desc <x> <y> - dump desc ring entry x ~ y\n"
		"\t\tq dump idx <N> dir [<h2c|c2h|bi|cmpt>] cmpt <x> <y> - dump cmpt ring entry x ~ y\n"
		"\t\tq dump list <start_idx> <num_Qs> dir [<h2c|c2h|bi|cmpt>] cmpt <x> <y> - dump cmpt ring entry x ~ y\n"
		"\t\tq cmpt_read idx <N> - read the completion data\n"
#ifdef ERR_DEBUG
		"\t\tq err help - help to induce errors  \n"
		"\t\tq err idx <N> [<err <[1|0]>>] dir <[h2c|c2h|bi]> - induce errors on q idx <N>  \n"
#endif
		);
	fprintf(fp,
		"\t\treg dump [dmap <Q> <N>]          - register dump. Only dump dmap registers if dmap is specified.\n"
		"\t\t                                   specify dmap range to dump: Q=queue, N=num of queues\n"
		"\t\treg read [bar <N>] <addr>        - read a register\n"
		"\t\treg write [bar <N>] <addr> <val> - write a register\n"
		"\t\treg info bar <N> <addr> [num_regs <M>] - dump detailed fields information of a register\n");
	fprintf(fp,
		"\t\tintring dump vector <N> <start_idx> <end_idx> - interrupt ring dump for vector number <N>  \n"
		"\t\t                                                for intrrupt entries :<start_idx> --- <end_idx>\n");

	exit(fp == stderr ? 1 : 0);
}

static int arg_read_int(char *s, uint32_t *v)
{
    char *p;

    *v = strtoul(s, &p, 0);
    if (*p) {
        warnx("bad parameter \"%s\", integer expected", s);
        return -EINVAL;
    }
    return 0;
}

static int parse_ifname(char *name, struct xcmd_info *xcmd)
{
	int len = strlen(name);
	int pos, i;
	uint32_t v;
	char *p;

	/* qdmaN of qdmavfN*/
	if (len > 11) {
		warnx("interface name %s too long, expect qdma<N>.\n", name);
		return -EINVAL;
	}
	if (strncmp(name, "qdma", 4)) {
		warnx("bad interface name %s, expect qdma<N>.\n", name);
		return -EINVAL;
	}
	if (name[4] == 'v' && name[5] == 'f') {
		xcmd->vf = 1;
		pos = 6;
	} else {
		xcmd->vf = 0;
		pos = 4;
	}
	for (i = pos; i < len; i++) {
		if (!isxdigit(name[i])) {
			warnx("%s unexpected <qdmaN>, %d.\n", name, i);
			return -EINVAL;
		}
	}

	v = strtoul(name + pos, &p, 16);
	if (*p) {
		warnx("bad parameter \"%s\", integer expected", name + pos);
		return -EINVAL;
	}

	xcmd->if_bdf = v;
	return 0;
}

#define get_next_arg(argc, argv, i) \
	if (++(*i) >= argc) { \
		warnx("%s missing parameter after \"%s\".\n", __FUNCTION__, argv[--(*i)]); \
		return -EINVAL; \
	}

#define __get_next_arg(argc, argv, i) \
	if (++i >= argc) { \
		warnx("%s missing parameter aft \"%s\".\n", __FUNCTION__, argv[--i]); \
		return -EINVAL; \
	}

static int next_arg_read_int(int argc, char *argv[], int *i, unsigned int *v)
{
	get_next_arg(argc, argv, i);
	return arg_read_int(argv[*i], v);
}

static int validate_regcmd(enum xnl_op_t qcmd, struct xcmd_reg	*regcmd)
{
	int invalid = 0;

	switch(qcmd) {
		case XNL_CMD_REG_DUMP:
			break;
		case XNL_CMD_REG_RD:
		case XNL_CMD_REG_INFO_READ:
		case XNL_CMD_REG_WRT:
			if ((regcmd->bar != 0) && (regcmd->bar != 2)) {
				printf("dmactl: bar %u number out of range\n",
				       regcmd->bar);
				invalid = -EINVAL;
				break;
			}
			break;
		default:
			invalid = -EINVAL;
			break;
	}

	return invalid;
}

static int parse_reg_cmd(int argc, char *argv[], int i, struct xcmd_info *xcmd)
{
	struct xcmd_reg	*regcmd = &xcmd->req.reg;
	int rv;
	int args_valid;

	/*
	 * reg dump
	 * reg read [bar <N>] <addr> 
	 * reg write [bar <N>] <addr> <val> 
	 */

	memset(regcmd, 0, sizeof(struct xcmd_reg));
	if (!strcmp(argv[i], "dump")) {
		xcmd->op = XNL_CMD_REG_DUMP;
		i++;

		if (i < argc) {
			if (!strcmp(argv[i], "dmap")) {
				get_next_arg(argc, argv, &i);
				rv = read_range(argc, argv, i, &regcmd->range_start,
					&regcmd->range_end);
				if (rv < 0)
					return rv;
				i = rv;
			}
		}

	} else if (!strcmp(argv[i], "read")) {
		xcmd->op = XNL_CMD_REG_RD;

		get_next_arg(argc, argv, &i);
		if (!strcmp(argv[i], "bar")) {
			rv = next_arg_read_int(argc, argv, &i, &regcmd->bar);
			if (rv < 0)
				return rv;
			regcmd->sflags |= XCMD_REG_F_BAR_SET;
			get_next_arg(argc, argv, &i);
		}
		rv = arg_read_int(argv[i], &regcmd->reg);
		if (rv < 0)
			return rv;
		regcmd->sflags |= XCMD_REG_F_REG_SET;

		i++;

	} else if (!strcmp(argv[i], "write")) {
		xcmd->op = XNL_CMD_REG_WRT;

		get_next_arg(argc, argv, &i);
		if (!strcmp(argv[i], "bar")) {
			rv = next_arg_read_int(argc, argv, &i, &regcmd->bar);
			if (rv < 0)
				return rv;
			regcmd->sflags |= XCMD_REG_F_BAR_SET;
			get_next_arg(argc, argv, &i);
		}
		rv = arg_read_int(argv[i], &xcmd->req.reg.reg);
		if (rv < 0)
			return rv;
		regcmd->sflags |= XCMD_REG_F_REG_SET;

		rv = next_arg_read_int(argc, argv, &i, &xcmd->req.reg.val);
		if (rv < 0)
			return rv;
		regcmd->sflags |= XCMD_REG_F_VAL_SET;

		i++;
	} else if (!strcmp(argv[i], "info")) {
		xcmd->op = XNL_CMD_REG_INFO_READ;
		get_next_arg(argc, argv, &i);
		if (!strcmp(argv[i], "bar")) {
			rv = next_arg_read_int(argc, argv, &i, &regcmd->bar);
			if (rv < 0)
				return rv;
			regcmd->sflags |= XCMD_REG_F_BAR_SET;
			get_next_arg(argc, argv, &i);
		}
		rv = arg_read_int(argv[i], &xcmd->req.reg.reg);
		if (rv < 0)
			return rv;
		regcmd->sflags |= XCMD_REG_F_REG_SET;
		i++;

		if (i < argc) {
			if (!strcmp(argv[i], "num_regs")) {
				rv = next_arg_read_int(argc, argv, &i, &regcmd->range_end);
				if (rv < 0)
					return rv;
			}
		} else
			regcmd->range_end = 1;
		i++;
	}

	args_valid = validate_regcmd(xcmd->op, regcmd);

	return (args_valid == 0) ? i : args_valid;
}

static int read_range(int argc, char *argv[], int i, unsigned int *v1,
			unsigned int *v2)
{
	int rv;

	/* range */
	rv = arg_read_int(argv[i], v1);
	if (rv < 0)
		return rv;

	get_next_arg(argc, argv, &i);
	rv = arg_read_int(argv[i], v2);
	if (rv < 0)
		return rv;

	if (v2 < v1) {
		warnx("invalid range %u ~ %u.\n", *v1, *v2);
		return -EINVAL;
	}

	return ++i;
}

/** 1:1 mapping to entries in q_parm_type of nl_user.h */
static char *qparm_type_str[QPARM_MAX] = {
	"idx",
	"mode",
	"dir",
	"desc",
	"cmpt",
	"cmptsz",
	"sw_desc_sz",
	"idx_ringsz",
	"idx_bufsz",
	"idx_tmr",
	"idx_cntr",
	"trigmode",
#ifdef ERR_DEBUG
	"err_no"
#endif
};

/** 1:1 mapping for flags from qdma_nl.h #defines flags */
static char *qflag_type_str[MAX_QFLAGS] = {
	"mode",
	"mode",
	"dir",
	"dir",
	"pfetch_en",
	"bypass",
	"fetch_credit",
	"fetch_credit",
	"dis_cmpl_status_acc",
	"dis_cmpl_status",
	"dis_cmpl_status_pend_chk",
	"dis_cmpl_status",
	"c2h_cmpl_intr_en",
	"c2h_udd_en",
	"pftch_bypass_en",
	"cmpl_ovf_dis",
	"en_mm_cmpl"
};

#define IS_SIZE_IDX_VALID(x) (x < 16)

static void print_ignored_params(uint32_t ignore_mask, uint8_t isflag,
		char *ignored_dir)
{
	unsigned int maxcount = isflag ? MAX_QFLAGS : QPARM_MAX;
	char **qparam = isflag ? qflag_type_str : qparm_type_str;
	char *pdesc = isflag ? "flag" : "attr";
	unsigned int i;

	for (i = 0; ignore_mask && (i < maxcount); i++) {
		if (ignore_mask & 0x01) {
			if(ignored_dir == NULL)
				warnx("Warn: Ignoring %s: %s", pdesc, qparam[i]);
			else
				warnx("Info: Ignoring %s \"%s\" for %s direction only", pdesc, qparam[i],
						ignored_dir);
		}
		ignore_mask >>= 1;
	}
}


static int validate_qcmd(enum xnl_op_t qcmd, struct xcmd_q_parm *qparm)
{
	int invalid = 0;
	switch(qcmd) {
		case XNL_CMD_Q_LIST:
			if (qparm->sflags)
				warnx("Warn: Ignoring all attributes and flags");
			break;
		case XNL_CMD_Q_ADD:
			print_ignored_params(qparm->sflags &
					     Q_ADD_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_ADD_FLAG_IGNORE_MASK, 1, NULL);
			break;
		case XNL_CMD_Q_START:
			if (!IS_SIZE_IDX_VALID(qparm->c2h_bufsz_idx)) {
				warnx("dmactl: C2H Buf index out of range");
				invalid = -EINVAL;
			}
			if (!IS_SIZE_IDX_VALID(qparm->qrngsz_idx)) {
				warnx("dmactl: Queue ring size index out of range");
				invalid = -EINVAL;
			}
			if (!IS_SIZE_IDX_VALID(qparm->cmpt_cntr_idx)) {
				warnx("dmactl: CMPT counter index out of range");
				invalid = -EINVAL;
			}
			if (!IS_SIZE_IDX_VALID(qparm->cmpt_tmr_idx)) {
				warnx("dmactl: CMPT timer index out of range");
				invalid = -EINVAL;
			}
			if (qparm->cmpt_entry_size >=
					XNL_ST_C2H_NUM_CMPT_DESC_SIZES) {
				warnx("dmactl: CMPT entry size out of range");
				invalid = -EINVAL;
			}
			if (qparm->flags & XNL_F_PFETCH_BYPASS_EN) {
				if (!(qparm->flags & XNL_F_DESC_BYPASS_EN)) {
					printf("Error:desc bypass enable must be enabled for configuring pfetch bypass enable\n");
					invalid = -EINVAL;
					break;
				}
			}
			if (qparm->flags & XNL_F_CMPL_UDD_EN) {
				if (!(qparm->sflags & (1 << QPARM_CMPTSZ))) {
					printf("Error: cmptsz required for enabling c2h udd packet\n");
					invalid = -EINVAL;
					break;
				}
			}

			print_ignored_params(qparm->sflags &
						Q_START_ATTR_IGNORE_MASK,
						0, NULL);

			if ((qparm->sflags & (1 << QPARM_SW_DESC_SZ))) {
				/* TODO: in 2018.3 RTL1 , only 64B sw_desc_size is supported */
				if (qparm->sw_desc_sz != DESC_SIZE_64B) {
					warnx("dmactl: desc size out of range");
					invalid = -EINVAL;
				}
			}
			break;
		case XNL_CMD_Q_STOP:
			print_ignored_params(qparm->sflags &
					     Q_STOP_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_STOP_FLAG_IGNORE_MASK, 1, NULL);
			break;
		case XNL_CMD_Q_DEL:
			print_ignored_params(qparm->sflags &
					     Q_DEL_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_DEL_FLAG_IGNORE_MASK, 1, NULL);
			break;
		case XNL_CMD_Q_CMPT:
		case XNL_CMD_Q_DUMP:
			if ((qparm->sflags & ((1 << QPARM_DESC) |
					(1 << QPARM_CMPT))) == ((1 << QPARM_DESC) |
							(1 << QPARM_CMPT))) {
				invalid = -EINVAL;
				printf("Error: Both desc and cmpt attr cannot be taken for Q DUMP\n");
				break;
			}
		case XNL_CMD_Q_DESC:
			print_ignored_params(qparm->sflags &
					     Q_DUMP_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_DUMP_FLAG_IGNORE_MASK, 1, NULL);
			break;
		case XNL_CMD_Q_RX_PKT:
			if (qparm->flags & XNL_F_QDIR_H2C) {
				printf("Rx dump packet is st c2h only command\n");
				invalid = -EINVAL;
				break;
			}
			print_ignored_params(qparm->sflags &
					     Q_DUMP_PKT_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_DUMP_PKT_FLAG_IGNORE_MASK, 1, NULL);
			break;
		case XNL_CMD_Q_CMPT_READ:
			print_ignored_params(qparm->sflags &
					     Q_CMPT_READ_ATTR_IGNORE_MASK, 0, NULL);
			print_ignored_params(qparm->flags &
					     Q_CMPT_READ_FLAG_IGNORE_MASK, 1, NULL);
			break;
#ifdef ERR_DEBUG
		case XNL_CMD_Q_ERR_INDUCE:
			break;
#endif
		default:
			invalid = -EINVAL;
			break;
	}

	return invalid;
}

#ifdef ERR_DEBUG
static unsigned char get_err_num(char *err)
{
	uint32_t i;

	for (i = 0; i < qdma_errs; i++)
		if (!strcmp(err, qdma_err_str[i]))
			break;

	return i;
}
#endif

static int read_qparm(int argc, char *argv[], int i, struct xcmd_q_parm *qparm,
			unsigned int f_arg_required)
{
	int rv;
	uint32_t v1;
	unsigned int f_arg_set = 0;;
	unsigned int mask;

	/*
	 * idx <val>
	 * list <start_idx> <num_q>
	 * ringsz <val>
	 * bufsz <val>
	 * mode <mm|st>
	 * dir <h2c|c2h|bi>
	 * cdev <0|1>
	 * bypass <0|1>
	 * desc <x> <y>
	 * cmpt <x> <y>
	 * cmptsz <0|1|2|3>
	 */

	qparm->idx = XNL_QIDX_INVALID;

	while (i < argc) {
#ifdef ERR_DEBUG
		if ((f_arg_required & (1 << QPARAM_ERR_NO)) &&
				(!strcmp(argv[i], "help"))) {
			uint32_t j;

			fprintf(stdout, "q err idx <N> <num_errs> [list of <err <[1|0]>>] dir <[h2c|c2h|bi]>\n");
			fprintf(stdout, "Supported errors:\n");
			for (j = 0; j < qdma_errs; j++)
				fprintf(stdout, "\t%s\n", qdma_err_str[j]);

			return argc;
		}
#endif
		if (!strcmp(argv[i], "idx")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->idx = v1;
			f_arg_set |= 1 << QPARM_IDX;
			qparm->num_q = 1;
#ifdef ERR_DEBUG
			if (f_arg_required & (1 << QPARAM_ERR_NO)) {
				unsigned char err_no;

				get_next_arg(argc, argv, &i);

				err_no = get_err_num(argv[i]);
				if (err_no >= qdma_errs) {
					fprintf(stderr,
						"unknown err %s.\n",
						argv[i]);
					return -EINVAL;
				}

				rv = next_arg_read_int(argc, argv, &i, &v1);
				if (rv < 0)
					return rv;

				qparm->err.en = v1 ? 1 : 0;
				qparm->err.err_no = err_no;
				printf("%s-%u: err_no/en: %u/%u\n", argv[i], qparm->idx, qparm->err.err_no, qparm->err.en);
				i++;
				f_arg_set |= 1 << QPARAM_ERR_NO;
			} else
				i++;
#else
			i++;
#endif

		} else if (!strcmp(argv[i], "list")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->idx = v1;
			f_arg_set |= 1 << QPARM_IDX;
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->num_q = v1;
			i++;

		} else if (!strcmp(argv[i], "mode")) {
			get_next_arg(argc, argv, (&i));

			if (!strcmp(argv[i], "mm")) {
				qparm->flags |= XNL_F_QMODE_MM;
			} else if (!strcmp(argv[i], "st")) {
				qparm->flags |= XNL_F_QMODE_ST;
			} else {
				warnx("unknown q mode %s.\n", argv[i]);
				return -EINVAL;
			}
			f_arg_set |= 1 << QPARM_MODE;
			i++;

		} else if (!strcmp(argv[i], "dir")) {
			get_next_arg(argc, argv, (&i));

			if (!strcmp(argv[i], "h2c")) {
				qparm->flags |= XNL_F_QDIR_H2C;
			} else if (!strcmp(argv[i], "c2h")) {
				qparm->flags |= XNL_F_QDIR_C2H;
			} else if (!strcmp(argv[i], "bi")) {
				qparm->flags |= XNL_F_QDIR_BOTH;
			} else if (!strcmp(argv[i], "cmpt")) {
				qparm->flags |= XNL_F_Q_CMPL;
			} else {
				warnx("unknown q dir %s.\n", argv[i]);
				return -EINVAL;
			}
			f_arg_set |= 1 << QPARM_DIR;
			i++;
		} else if (!strcmp(argv[i], "idx_bufsz")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->c2h_bufsz_idx = v1;

			f_arg_set |= 1 << QPARM_C2H_BUFSZ_IDX;
			i++;
		
		} else if (!strcmp(argv[i], "idx_ringsz")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->qrngsz_idx = v1;
			f_arg_set |= 1 << QPARM_RNGSZ_IDX;
			i++;
		} else if (!strcmp(argv[i], "idx_tmr")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->cmpt_tmr_idx = v1;
			f_arg_set |= 1 << QPARM_CMPT_TMR_IDX;
			i++;
		} else if (!strcmp(argv[i], "idx_cntr")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->cmpt_cntr_idx = v1;
			f_arg_set |= 1 << QPARM_CMPT_CNTR_IDX;
			i++;
		} else if (!strcmp(argv[i], "mm_chn")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			qparm->mm_channel = v1;
			f_arg_set |= 1 << QPARM_MM_CHANNEL;
			i++;
		} else if (!strcmp(argv[i], "cmpl_ovf_dis")) {
			qparm->flags |= XNL_F_CMPT_OVF_CHK_DIS;
			i++;
		} else if (!strcmp(argv[i], "trigmode")) {
			get_next_arg(argc, argv, (&i));

			if (!strcmp(argv[i], "every")) {
				v1 = 1;
			} else if (!strcmp(argv[i], "usr_cnt")) {
				v1 = 2;
			} else if (!strcmp(argv[i], "usr")) {
				v1 = 3;
			} else if (!strcmp(argv[i], "usr_tmr")) {
				v1=4;
			} else if (!strcmp(argv[i], "cntr_tmr")) {
				v1=5;
			} else if (!strcmp(argv[i], "dis")) {
				v1 = 0;
			} else {
				warnx("unknown q trigmode %s.\n", argv[i]);
				return -EINVAL;
			}

			qparm->cmpt_trig_mode = v1;
			f_arg_set |= 1 << QPARM_CMPT_TRIG_MODE;
			i++;
		} else if (!strcmp(argv[i], "desc")) {
			get_next_arg(argc, argv, &i);
			rv = read_range(argc, argv, i, &qparm->range_start,
					&qparm->range_end);
			if (rv < 0)
				return rv;
			i = rv;
			f_arg_set |= 1 << QPARM_DESC;

		} else if (!strcmp(argv[i], "cmpt")) {
		    get_next_arg(argc, argv, &i);
		    rv = read_range(argc, argv, i, &qparm->range_start,
			    &qparm->range_end);
		    if (rv < 0)
			return rv;
		    i = rv;
		    f_arg_set |= 1 << QPARM_CMPT;
		} else if (!strcmp(argv[i], "cmptsz")) {
		    get_next_arg(argc, argv, &i);
		    sscanf(argv[i], "%hhu", &qparm->cmpt_entry_size);
		    f_arg_set |= 1 << QPARM_CMPTSZ;
		    i++;
		} else if (!strcmp(argv[i], "sw_desc_sz")) {
		    get_next_arg(argc, argv, &i);
		    sscanf(argv[i], "%hhu", &qparm->sw_desc_sz);
		    f_arg_set |= 1 << QPARM_SW_DESC_SZ;
		    i++;
		} else if (!strcmp(argv[i], "pfetch_en")) {
			qparm->flags |= XNL_F_PFETCH_EN;
			i++;
		} else if (!strcmp(argv[i], "ping_pong_en")) {
			f_arg_set |= 1 << QPARM_PING_PONG_EN;
			i++;
		} else if (!strcmp(argv[i], "aperture_sz")) {
			rv = next_arg_read_int(argc, argv, &i, &v1);
			if (rv < 0)
				return rv;

			if(((v1 != 0) && ((v1 &(v1 - 1))))) {
				warnx("Error: Keyhole aperture should be a size of 2\n");
				return -EINVAL;
			}

			f_arg_set |= 1 << QPARM_KEYHOLE_EN;
			qparm->aperture_sz = v1;
			i++;
		} else if (!strcmp(argv[i], "pfetch_bypass_en")) {
			qparm->flags |= XNL_F_PFETCH_BYPASS_EN;
			i++;
		} else if (!strcmp(argv[i], "desc_bypass_en")) {
			qparm->flags |= XNL_F_DESC_BYPASS_EN;
			i++;
		} else if (!strcmp(argv[i], "c2h_cmpl_intr_en")) {
			qparm->flags |= XNL_F_C2H_CMPL_INTR_EN;
			i++;
		} else if (!strcmp(argv[i], "dis_cmpl_status")) {
			qparm->flags &= ~XNL_F_CMPL_STATUS_EN;
			i++;
		} else if (!strcmp(argv[i], "dis_cmpl_status_acc")) {
			qparm->flags &= ~XNL_F_CMPL_STATUS_ACC_EN;
			i++;
		} else if (!strcmp(argv[i], "dis_cmpl_status_pend_chk")) {
			qparm->flags &= ~XNL_F_CMPL_STATUS_PEND_CHK;
			i++;
		} else if (!strcmp(argv[i], "fetch_credit")) {
			get_next_arg(argc, argv, (&i));
			if (!strcmp(argv[i], "h2c")) {
				if (((qparm->flags & XNL_F_QDIR_H2C) == XNL_F_QDIR_H2C)
					|| ((qparm->flags & XNL_F_QDIR_BOTH) == XNL_F_QDIR_BOTH))
				v1 = Q_ENABLE_H2C_FETCH_CREDIT;
				else {
					warnx("Invalid Fetch credit option,%s",
					"Q direction mismatch, not H2C dir\n");
					return -EINVAL;
				}
			} else if (!strcmp(argv[i], "c2h")) {
				if (((qparm->flags & XNL_F_QDIR_C2H) == XNL_F_QDIR_C2H)
					|| ((qparm->flags & XNL_F_QDIR_BOTH) == XNL_F_QDIR_BOTH))
				v1 = Q_ENABLE_C2H_FETCH_CREDIT;
				else {
					warnx("Invalid Fetch credit option,%s",
					"Q direction mismatch, not C2H dir\n");
					return -EINVAL;
				}
			} else if (!strcmp(argv[i], "bi")) {
				if (((qparm->flags & XNL_F_QDIR_BOTH) == XNL_F_QDIR_BOTH))
					v1 = Q_ENABLE_H2C_C2H_FETCH_CREDIT;
				else {
					warnx("Invalid Fetch credit option, %s",
					"Q direction mismatch, not BI dir\n");
					return -EINVAL;
				}
			} else if (!strcmp(argv[i], "none")){
				v1 = Q_DISABLE_FETCH_CREDIT;
			} else {
				warnx("unknown fetch_credit option %s.\n", argv[i]);
				return -EINVAL;
			}
			qparm->fetch_credit = v1;
			i++;
		} else if (!strcmp(argv[i], "c2h_udd_en")) {
			qparm->flags |= XNL_F_CMPL_UDD_EN;
			i++;
		} else {
			warnx("unknown q parameter %s.\n", argv[i]);
			return -EINVAL;
		}
	}
	if ((f_arg_required & (1 << QPARM_RNGSZ_IDX)) &&
			!(f_arg_set & (1 << QPARM_RNGSZ_IDX))) {
		warnx("Info: Default ring size set to 2048");
		qparm->qrngsz_idx = 9;
		f_arg_set |= 1 << QPARM_RNGSZ_IDX;
	}
	/* check for any missing mandatory parameters */
	mask = f_arg_set & f_arg_required;
	if (mask != f_arg_required) {
		int i;
		unsigned int bit_mask = 1;

		for (i = 0; i < QPARM_MAX; i++, bit_mask <<= 1) {
			if (!(bit_mask & f_arg_required))
				continue;
			warnx("missing q parameter %s.\n", qparm_type_str[i]);
			return -EINVAL;
		}
	}


	if (!(f_arg_set & 1 << QPARM_DIR) && !(qparm->flags & XNL_F_Q_CMPL)) {
		/* default to H2C */
		warnx("Warn: Default dir set to \'h2c\'");
		f_arg_set |= 1 << QPARM_DIR;
		qparm->flags |=  XNL_F_QDIR_H2C;
	}

	qparm->sflags = f_arg_set;

	return argc;
}

static int parse_q_cmd(int argc, char *argv[], int i, struct xcmd_info *xcmd)
{
	struct xcmd_q_parm *qparm = &xcmd->req.qparm;
	int rv;
	int args_valid;

	/*
	 * q list
	 * q add idx <N> mode <mm|st> [dir <h2c|c2h|bi>] [cdev <0|1>] [cmptsz <0|1|2|3>]
	 * q start idx <N> dir <h2c|c2h|bi>
	 * q stop idx <N> dir <h2c|c2h|bi>
	 * q del idx <N> dir <h2c|c2h|bi>
	 * q dump idx <N> dir <h2c|c2h|bi>
	 * q dump idx <N> dir <h2c|c2h|bi> desc <x> <y>
	 * q dump idx <N> dir <h2c|c2h|bi> cmpt <x> <y>
	 * q pkt idx <N>
	 */

	if (!strcmp(argv[i], "list")) {
		xcmd->op = XNL_CMD_Q_LIST;
		return ++i;
	} else if (!strcmp(argv[i], "add")) {
		unsigned int mask;

		xcmd->op = XNL_CMD_Q_ADD;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
		/* error checking of parameter values */
		if (qparm->sflags & (1 << QPARM_MODE)) {
			mask = XNL_F_QMODE_MM | XNL_F_QMODE_ST;
			if ((qparm->flags & mask) == mask) {
				warnx("mode mm/st cannot be combined.\n");
				return -EINVAL;
			}
		} else {
			/* default to MM */
			warnx("Warn: Default mode set to \'mm\'");
			qparm->sflags |= 1 << QPARM_MODE;
			qparm->flags |=  XNL_F_QMODE_MM;
		}

	} else if (!strcmp(argv[i], "start")) {
		xcmd->op = XNL_CMD_Q_START;
		get_next_arg(argc, argv, &i);
		qparm->fetch_credit = Q_ENABLE_C2H_FETCH_CREDIT;
		qparm->flags |= (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
				XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
				XNL_F_FETCH_CREDIT);
		rv = read_qparm(argc, argv, i, qparm, ((1 << QPARM_IDX) |
				(1 << QPARM_RNGSZ_IDX)));
		if ((qparm->flags & (XNL_F_QDIR_C2H | XNL_F_QMODE_ST)) ==
				(XNL_F_QDIR_C2H | XNL_F_QMODE_ST)) {
			if (!(qparm->sflags & (1 << QPARM_CMPTSZ))) {
					/* default to 8B */
					qparm->cmpt_entry_size = XNL_ST_C2H_CMPT_DESC_SIZE_8B;
					qparm->sflags |=
						(1 << QPARM_CMPTSZ);
			}
		}
	} else if (!strcmp(argv[i], "stop")) {
		xcmd->op = XNL_CMD_Q_STOP;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
	} else if (!strcmp(argv[i], "del")) {
		xcmd->op = XNL_CMD_Q_DEL;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
	} else if (!strcmp(argv[i], "dump")) {
		xcmd->op = XNL_CMD_Q_DUMP;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
	} else if (!strcmp(argv[i], "pkt")) {
		xcmd->op = XNL_CMD_Q_RX_PKT;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
	} else if (!strcmp(argv[i], "cmpt_read")) {
		xcmd->op = XNL_CMD_Q_CMPT_READ;
		qparm->flags |= XNL_F_Q_CMPL;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, (1 << QPARM_IDX));
#ifdef ERR_DEBUG
	} else if (!strcmp(argv[i], "err")) {
		xcmd->op = XNL_CMD_Q_ERR_INDUCE;
		get_next_arg(argc, argv, &i);
		rv = read_qparm(argc, argv, i, qparm, ((1 << QPARM_IDX) |
		                (1 << QPARAM_ERR_NO)));
#endif
	} else {
		printf("Error: Unknown q command\n");
		return -EINVAL;
	}
	
	if (rv < 0)
		return rv;
	i = rv;

	if (xcmd->op == XNL_CMD_Q_DUMP) {
		unsigned int mask = (1 << QPARM_DESC) | (1 << QPARM_CMPT);

		if ((qparm->sflags & mask) == mask) {
			warnx("dump cmpt/desc cannot be combined.\n");
			return -EINVAL;
		}
		if ((qparm->sflags & (1 << QPARM_DESC)))
			xcmd->op = XNL_CMD_Q_DESC;
		else if ((qparm->sflags & (1 << QPARM_CMPT)))
			xcmd->op = XNL_CMD_Q_CMPT;
	}

	args_valid = validate_qcmd(xcmd->op, qparm);

	return (args_valid == 0) ? i : args_valid;
}

static int parse_dev_cmd(int argc, char *argv[], int i, struct xcmd_info *xcmd)
{
	if (!strcmp(argv[i], "list")) {
		xcmd->op = XNL_CMD_DEV_LIST;
		i++;
	}

	return i;
}

static int parse_stat_cmd(int argc, char *argv[], int i, struct xcmd_info *xcmd)
{

	xcmd->op = XNL_CMD_DEV_STAT;
	if (i >= argc)
		return i;
	if (!strcmp(argv[i], "clear")) {
		xcmd->op = XNL_CMD_DEV_STAT_CLEAR;
		i++;
	}
	return i;
}

static int parse_intr_cmd(int argc, char *argv[], int i, struct xcmd_info *xcmd)
{
	struct xcmd_intr	*intrcmd = &xcmd->req.intr;
	int rv;

	/*
	 * intr dump vector <N>
	 */

	memset(intrcmd, 0, sizeof(struct xcmd_intr));
	if (!strcmp(argv[i], "dump")) {
		xcmd->op = XNL_CMD_INTR_RING_DUMP;

		get_next_arg(argc, argv, &i);
		if (!strcmp(argv[i], "vector")) {
			rv = next_arg_read_int(argc, argv, &i, &intrcmd->vector);
			if (rv < 0)
				return rv;
		}
		rv = next_arg_read_int(argc, argv, &i, &intrcmd->start_idx);
		if (rv < 0) {
			intrcmd->start_idx = 0;
			intrcmd->end_idx = QDMA_MAX_INT_RING_ENTRIES - 1;
			goto func_ret;
		}
		rv = next_arg_read_int(argc, argv, &i, &intrcmd->end_idx);
		if (rv < 0)
			intrcmd->end_idx = QDMA_MAX_INT_RING_ENTRIES - 1;
	}
func_ret:
	i++;
	return i;
}

int parse_cmd(int argc, char *argv[], struct xcmd_info *xcmd)
{
	char *ifname;
	int i;
	int rv;

	memset(xcmd, 0, sizeof(struct xcmd_info));

	progname = argv[0];

	if (argc == 1) 
		usage(stderr);

	if (argc == 2) {
		if (!strcmp(argv[1], "?") || !strcmp(argv[1], "-h") ||
		    !strcmp(argv[1], "help") || !strcmp(argv[1], "--help"))
			usage(stdout);

		if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
			printf("%s version %s\n", PROGNAME, VERSION);
			printf("%s\n", COPYRIGHT);
			exit(0);
		}
	}

	if (!strcmp(argv[1], "dev")) {
		rv = parse_dev_cmd(argc, argv, 2, xcmd);
		goto done;
	}

	/* which dma fpga */
	ifname = argv[1];
	rv = parse_ifname(ifname, xcmd);
	if (rv < 0)
		return rv;

	if (argc == 2) {
		rv = 2;
		xcmd->op = XNL_CMD_DEV_INFO;
		goto done;
	}

	i = 3;
	if (!strcmp(argv[2], "reg")) {
		rv = parse_reg_cmd(argc, argv, i, xcmd);
	} else if (!strcmp(argv[2], "stat")) {
		rv = parse_stat_cmd(argc, argv, i, xcmd);
	} else if (!strcmp(argv[2], "q")) {
		rv = parse_q_cmd(argc, argv, i, xcmd);
	} else if (!strcmp(argv[2], "intring")){
		rv = parse_intr_cmd(argc, argv, i, xcmd);
	} else if (!strcmp(argv[2], "cap")) {
		rv = 3;
		xcmd->op = XNL_CMD_DEV_CAP;
	}  else if (!strcmp(argv[2], "global_csr")) {
		rv = 3;
		xcmd->op = XNL_CMD_GLOBAL_CSR;
	}
	else if (!strcmp(argv[2], "info")) { /* not exposed. only for debug */
		rv = 3;
		xcmd->op = XNL_CMD_DEV_INFO;
	} else {
		warnx("bad parameter \"%s\".\n", argv[2]);
		return -EINVAL;
	}

done:
	if (rv < 0)
		return rv;
	i = rv;
	
	if (i < argc) {
		warnx("unexpected parameter \"%s\".\n", argv[i]);
		return -EINVAL;
	}
	return 0;
}
