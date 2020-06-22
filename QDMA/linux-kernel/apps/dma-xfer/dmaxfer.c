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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdbool.h>
#include <linux/types.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include "version.h"
#include "dmautils.h"
#include "qdma_nl.h"
#include "dmaxfer.h"

#define QDMA_Q_NAME_LEN     100
#define QDMA_ST_MAX_PKT_SIZE 0x7000  
#define QDMA_RW_MAX_SIZE	0x7ffff000
#define QDMA_GLBL_MAX_ENTRIES  (16)

static struct queue_info *q_info;
static int q_count;

enum qdma_q_dir {
	QDMA_Q_DIR_H2C,
	QDMA_Q_DIR_C2H,
	QDMA_Q_DIR_BIDI
};

enum qdma_q_mode {
	QDMA_Q_MODE_MM,
	QDMA_Q_MODE_ST
};

struct queue_info {
	char *q_name;
	int qid;
	int pf;
	enum qdmautils_io_dir dir;
};

enum qdma_q_mode mode;
enum qdma_q_dir dir;
static char cfg_name[64];
static unsigned int pkt_sz;
static unsigned int pci_bus;
static unsigned int pci_dev;
static int fun_id = -1;
static int is_vf;
static unsigned int q_start;
static unsigned int num_q;
static unsigned int idx_rngsz;
static unsigned int idx_tmr;
static unsigned int idx_cnt;
static unsigned int pfetch_en;
static unsigned int cmptsz;
static char input_file[128];
static char output_file[128];
static int io_type;
static char trigmode_str[10];
static unsigned char trig_mode;

static struct option const long_opts[] = {
	{"config", required_argument, NULL, 'c'},
	{0, 0, 0, 0}
};

static void prep_reg_dump(void);

static void usage(const char *name)
{
	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

	fprintf(stdout, "  -%c (--%s) config file that has configration for IO\n",
			long_opts[0].val, long_opts[0].name);
	fprintf(stdout, "  -v (--version), to print version name\n");
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

static int arg_read_int(char *s, uint32_t *v)
{
	char *p = NULL;


	*v = strtoul(s, &p, 0);
	if (*p && (*p != '\n') && !isblank(*p)) {
		printf("Error:something not right%s %s %s",
				s, p, isblank(*p)? "true": "false");
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
	int ret;

	memset(str, '\0', slen + 1);
	strncpy(str, s + 1, slen - trail_blanks - 2);
	str[slen] = '\0';

	elem = strtok(str, " ,");/* space or comma separated */
	while (elem != NULL) {
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

static ssize_t read_to_buffer(char *fname, int fd, char *buffer,
		uint64_t size, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0;
	char *buf = buffer;
	off_t offset = base;

	do { /* Support zero byte transfer */
		uint64_t bytes = size - count;

		if (bytes > QDMA_RW_MAX_SIZE)
			bytes = QDMA_RW_MAX_SIZE;

		if (offset) {
			rc = lseek(fd, offset, SEEK_SET);
			if (rc < 0) {
				printf("Error: %s, seek off 0x%lx failed %zd\n",
						fname, offset, rc);
				return -EIO;
			}
			if (rc != offset) {
				printf("Error: %s, seek off 0x%lx != 0x%lx\n",
						fname, rc, offset);
				return -EIO;
			}
		}

		/* read data from file into memory buffer */
		rc = read(fd, buf, bytes);
		if (rc < 0) {
			printf("Failed to Read %s, read off 0x%lx + 0x%lx failed %zd\n",
					fname, offset, bytes, rc);
			return -EIO;
		}
		if (rc != bytes) {
			printf("Failed to read %lx bytes from file %s, curr read:%lx\n",
					bytes, fname, rc);
			return -EIO;
		}

		count += bytes;
		buf += bytes;
		offset += bytes;

	} while (count < size);

	if (count != size) {
		printf("Failed to read %lx bytes from %s 0x%lx != 0x%lx.\n",
				size, fname, count, size);
		return -EIO;
	}

	return count;
}

static ssize_t write_from_buffer(char *fname, int fd, char *buffer,
		uint64_t size, uint64_t base)
{
	ssize_t rc;
	uint64_t count = 0;
	char *buf = buffer;
	off_t offset = base;

	do { /* Support zero byte transfer */
		uint64_t bytes = size - count;

		if (bytes > QDMA_RW_MAX_SIZE)
			bytes = QDMA_RW_MAX_SIZE;

		if (offset) {
			rc = lseek(fd, offset, SEEK_SET);
			if (rc < 0) {
				printf("Error: %s, seek off 0x%lx failed %zd\n",
						fname, offset, rc);
				return -EIO;
			}
			if (rc != offset) {
				printf("Error: %s, seek off 0x%lx != 0x%lx\n",
						fname, rc, offset);
				return -EIO;
			}
		}

		/* write data to file from memory buffer */
		rc = write(fd, buf, bytes);
		if (rc < 0) {
			printf("Failed to Read %s, read off 0x%lx + 0x%lx failed %zd\n",
					fname, offset, bytes, rc);
			return -EIO;
		}
		if (rc != bytes) {
			printf("Failed to read %lx bytes from file %s, curr read:%lx\n",
					bytes, fname, rc);
			return -EIO;
		}

		count += bytes;
		buf += bytes;
		offset += bytes;

	} while (count < size);

	if (count != size) {
		printf("Failed to read %lx bytes from %s 0x%lx != 0x%lx\n",
				size, fname, count, size);
		return -EIO;
	}

	return count;
}

static int parse_config_file(const char *cfg_fname)
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
	char rng_sz_path[200] = {'\0'};
	int rng_sz_fd, ret = 0;
	int input_file_provided = 0;
	int output_file_provided = 0;
	struct stat st;

	fp = fopen(cfg_fname, "r");
	if (fp == NULL) {
		printf("Failed to open Config File [%s]\n", cfg_fname);
		return -EINVAL;
	}

	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		linenum++;
		linebuf = strip_comments(linebuf);
		if (linebuf == NULL)
			continue;
		realbuf = strip_blanks(linebuf, &numblanks);
		linelen -= numblanks;
		if (0 == linelen)
			continue;
		config = strtok(realbuf, "=");
		value = strtok(NULL, "=");
		if (!strncmp(config, "mode", 4)) {
			if (!strncmp(value, "mm", 2))
				mode = QDMA_Q_MODE_MM;
			else if(!strncmp(value, "st", 2))
				mode = QDMA_Q_MODE_ST;
			else {
				printf("Error: Unknown mode\n");
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "dir", 3)) {
			if (!strncmp(value, "h2c", 3))
				dir = QDMA_Q_DIR_H2C;
			else if(!strncmp(value, "c2h", 3))
				dir = QDMA_Q_DIR_C2H;
			else if(!strncmp(value, "bi", 2))
				dir = QDMA_Q_DIR_BIDI; 
			else if(!strncmp(value, "cmpt", 4)) {
				printf("Error: cmpt type queue validation is not supported\n");
				goto prase_cleanup;
			} else {
				printf("Error: Unknown direction\n");
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "name", 3)) {
			copy_value(value, cfg_name, 64);
		} else if (!strncmp(config, "function", 8)) {
			if (arg_read_int(value, &fun_id)) {
				printf("Error: Invalid function:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "is_vf", 5)) {
			if (arg_read_int(value, &is_vf)) {
				printf("Error: Invalid is_vf param:%s\n", value);
				goto prase_cleanup;
			}
			if (is_vf > 1) {
				printf("Error: is_vf value is %d, expected 0 or 1\n",
						is_vf);
				goto prase_cleanup;
			}
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
		}  else if (!strncmp(config, "trig_mode", 9)) {
			copy_value(value, trigmode_str, 10);
		}  else if (!strncmp(config, "pkt_sz", 6)) {
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
		} else if (!strncmp(config, "inputfile", 7)) {
			copy_value(value, input_file, 128);
			input_file_provided = 1;
		} else if (!strncmp(config, "outputfile", 7)) {
			copy_value(value, output_file, 128);
			output_file_provided = 1;
		} else if (!strncmp(config, "io_type", 6)) {
			if (!strncmp(value, "io_sync", 6))
				io_type = 0;
			else if (!strncmp(value, "io_async", 6))
				io_type = 1;
			else {
				printf("Error: Unknown io_type\n");
				goto prase_cleanup;
			}
		}
	}
	fclose(fp);

	if (!pci_bus && !pci_dev) {
		printf("Error: PCI bus information not provided\n");
		return -EINVAL;	
	}

	if (fun_id < 0) {
		printf("Error: Valid function required\n");
		return -EINVAL;
	}

	if (fun_id <= 3 && is_vf) {
		printf("Error: invalid is_vf and fun_id values."
				"Fun_id for vf must be higer than 3\n");
		return -EINVAL;
	}

	if (mode == QDMA_Q_MODE_ST && pkt_sz > QDMA_ST_MAX_PKT_SIZE) {
		printf("Error: Pkt size [%u] larger than supported size [%d]\n",
				pkt_sz, QDMA_ST_MAX_PKT_SIZE);
		return -EINVAL;
	}

	if ((dir == QDMA_Q_DIR_H2C) || (dir == QDMA_Q_DIR_BIDI)) {
		if (!input_file_provided) {
			printf("Error: Input File required for Host to Card transfers\n");
			return -EINVAL;
		}

		ret = stat(input_file, &st);
		if (ret < 0) {
			printf("Error: Failed to read input file [%s] length\n",
					input_file);
			return ret;
		}

		if (pkt_sz > st.st_size) {
			printf("Error: File [%s] length is lesser than pkt_sz %u\n",
					input_file, pkt_sz);
			return -EINVAL;
		}
	}

	if (((dir == QDMA_Q_DIR_C2H) || (dir == QDMA_Q_DIR_BIDI)) &&
			!output_file_provided) {
		printf("Error: Data output file was not provided\n");
		return -EINVAL;
	}

	if (!strcmp(trigmode_str, "every"))
		trig_mode = 1;
	else if (!strcmp(trigmode_str, "usr_cnt"))
		trig_mode = 2;
	else if (!strcmp(trigmode_str, "usr"))
		trig_mode = 3;
	else if (!strcmp(trigmode_str, "usr_tmr"))
		trig_mode=4;
	else if (!strcmp(trigmode_str, "cntr_tmr"))
		trig_mode=5;
	else if (!strcmp(trigmode_str, "dis"))
		trig_mode = 0;
	else {
		printf("Error: unknown q trigmode %s.\n", trigmode_str);
		return -EINVAL;	
	}

	return 0;

prase_cleanup:
	fclose(fp);
	return -EINVAL;
}

static inline void qdma_q_prep_name(struct queue_info *q_info, int qid, int pf)
{
	q_info->q_name = calloc(QDMA_Q_NAME_LEN, 1);
	snprintf(q_info->q_name, QDMA_Q_NAME_LEN, "/dev/qdma%s%05x-%s-%d",
			(is_vf) ? "vf" : "",
			(pci_bus << 12) | (pci_dev << 4) | pf,
			(mode == QDMA_Q_MODE_MM) ? "MM" : "ST",
			qid);
}

static int qdma_validate_qrange(void)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	xcmd.op = XNL_CMD_DEV_INFO;
	xcmd.vf = is_vf; 
	xcmd.if_bdf = (pci_bus << 12) | (pci_dev << 4) | fun_id;

	/* Get dev info from qdma driver */
	ret = qdma_dev_info(&xcmd);
	if (ret < 0) {
		printf("Failed to read qmax for PF: %d\n", fun_id);
		return ret;
	}

	if (!xcmd.resp.dev_info.qmax) {
		printf("Error: invalid qmax assigned to function :%d qmax :%u\n",
				fun_id, xcmd.resp.dev_info.qmax);
		return -EINVAL;
	}

	if (xcmd.resp.dev_info.qmax <  num_q) {
		printf("Error: Q Range is beyond QMAX %u "
				"Funtion: %x Q start :%u Q Range End :%u\n",
				xcmd.resp.dev_info.qmax, fun_id, q_start, q_start + num_q);
		return -EINVAL;
	}

	return 0;
}

static int qdma_prepare_q_stop(struct xcmd_info *xcmd,
		enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd)
		return -EINVAL;

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_STOP;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;	

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;	


	return 0;
}

static int qdma_prepare_q_start(struct xcmd_info *xcmd,
		enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_q_parm *qparm;


	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}
	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_START;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}

	qparm->qrngsz_idx = idx_rngsz; 

	if ((dir == QDMA_Q_DIR_C2H) && (mode == QDMA_Q_MODE_ST)) {
		if (cmptsz)
			qparm->cmpt_entry_size = cmptsz;
		else
			qparm->cmpt_entry_size = XNL_ST_C2H_CMPT_DESC_SIZE_8B;
		qparm->cmpt_tmr_idx = idx_tmr; 
		qparm->cmpt_cntr_idx = idx_cnt; 
		qparm->cmpt_trig_mode = trig_mode;
		if (pfetch_en)
			qparm->flags |= XNL_F_PFETCH_EN;
	}

	qparm->flags |= (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
			XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
			XNL_F_FETCH_CREDIT);

	return 0;
}

static int qdma_prepare_q_del(struct xcmd_info *xcmd,
		enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_DEL;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}

	return 0;
}

static int qdma_prepare_q_add(struct xcmd_info *xcmd,
		enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_ADD;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}
	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}
	qparm->sflags = qparm->flags;

	return 0;
}

static int qdma_destroy_queue(enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_stop(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;	

	ret = qdma_q_stop(&xcmd);
	if (ret < 0)
		printf("Q_STOP failed, ret :%d\n", ret);

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	qdma_prepare_q_del(&xcmd, dir, qid, pf);
	ret = qdma_q_del(&xcmd);
	if (ret < 0)
		printf("Q_DEL failed, ret :%d\n", ret);

	return ret;
}

static int qdma_create_queue(enum qdmautils_io_dir dir,
		int qid, int pf)
{
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_add(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;

	ret = qdma_q_add(&xcmd);
	if (ret < 0) {
		printf("Q_ADD failed, ret :%d\n", ret);
		return ret;
	}

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_start(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;

	ret = qdma_q_start(&xcmd);
	if (ret < 0) {
		printf("Q_START failed, ret :%d\n", ret);
		qdma_prepare_q_del(&xcmd, dir, qid, pf);
		qdma_q_del(&xcmd);
	}

	return ret;
}

static int qdma_prepare_queue(struct queue_info *q_info,
		enum qdmautils_io_dir dir, int qid, int pf)
{
	int ret;

	if (!q_info) {
		printf("Error: Invalid queue info\n");
		return -EINVAL;
	}

	qdma_q_prep_name(q_info, qid, pf);
	q_info->dir = dir;
	ret = qdma_create_queue(q_info->dir, qid, pf);
	if (ret < 0) {
		printf("Q creation Failed PF:%d QID:%d\n",
				pf, qid);
		return ret;
	}
	q_info->qid = qid;
	q_info->pf = pf;

	return ret;
}

static int qdma_register_write(unsigned int pf, int bar,
		unsigned long reg, unsigned long value)
{
	struct xcmd_info xcmd;
	struct xcmd_reg *regcmd;
	int ret;
	regcmd = &xcmd.req.reg;
	xcmd.op = XNL_CMD_REG_WRT;
	xcmd.vf = is_vf; 
	xcmd.if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
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

static void qdma_queues_cleanup(struct queue_info *q_info, int q_count)
{
	unsigned int q_index;

	if (!q_info || q_count < 0)
		return;

	for (q_index = 0; q_index < q_count; q_index++) {
		qdma_destroy_queue(q_info[q_index].dir,
				q_info[q_index].qid,
				q_info[q_index].pf);
		free(q_info[q_index].q_name);
	}
}

static int qdma_setup_queues(struct queue_info **pq_info)
{
	struct queue_info *q_info;
	unsigned int qid;
	unsigned int q_count;
	unsigned int q_index;
	int ret;

	if (!pq_info) {
		printf("Error: Invalid queue info\n");
		return -EINVAL;
	}

	if (dir == QDMA_Q_DIR_BIDI)
		q_count = num_q * 2;
	else	
		q_count = num_q;

	*pq_info = q_info = (struct queue_info *)calloc(q_count, sizeof(struct queue_info));
	if (!q_info) {
		printf("Error: OOM\n");
		return -ENOMEM;
	}

	q_index = 0;
	for (qid = 0; qid < num_q; qid++) {
		if ((dir == QDMA_Q_DIR_BIDI) ||
				(dir == QDMA_Q_DIR_H2C)) {
			ret = qdma_prepare_queue(q_info + q_index,
					DMAXFER_IO_WRITE,
					qid + q_start,
					fun_id);
			if (ret < 0)
				break;
			q_index++;
		}
		if ((dir == QDMA_Q_DIR_BIDI) ||
				(dir == QDMA_Q_DIR_C2H)) {
			ret = qdma_prepare_queue(q_info + q_index,
					DMAXFER_IO_READ,
					qid + q_start,
					fun_id);
			if (ret < 0)
				break;
			q_index++;
		}
	}
	if (ret < 0) {
		qdma_queues_cleanup(q_info, q_index);
		return ret;
	}

	return q_count; 
}


static void qdma_env_cleanup()
{
	qdma_queues_cleanup(q_info, q_count);

	if (q_info)
		free(q_info);
	q_info = NULL;
	q_count = 0;
}

static int qdma_trigger_data_generator(struct queue_info *q_info)
{
	struct xcmd_info xcmd;
	unsigned char user_bar;
	unsigned int qbase;
	int ret;

	if (!q_info) {
		printf("Error: Invalid queue info\n");
		return -EINVAL;
	}

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	xcmd.op = XNL_CMD_DEV_INFO;
	xcmd.vf = is_vf; 
	xcmd.if_bdf = (pci_bus << 12) | (pci_dev << 4) | q_info->pf;

	ret = qdma_dev_info(&xcmd);
	if (ret < 0) {
		printf("Failed to read qmax for PF: %d\n", q_info->pf);
		return ret;
	}

	user_bar = xcmd.resp.dev_info.user_bar;
	qbase = xcmd.resp.dev_info.qbase;

	/* Disable DMA Bypass */
	ret = qdma_register_write(q_info->pf, user_bar, 0x90, 0);
	if (ret < 0) {
		printf("Failed to program HWQID PF :%d QID :%d\n",
				q_info->pf, q_info->qid);
		return ret;
	}

	/* Program HW QID */
	ret = qdma_register_write(q_info->pf, user_bar, 0x0,
			q_info->qid + qbase);
	if (ret < 0) {
		printf("Failed to program HWQID PF :%d QID :%d\n",
				q_info->pf, q_info->qid);
	}

	/* Set the transfer size to data generator */
	ret = qdma_register_write(q_info->pf, user_bar, 0x04,
			pkt_sz);
	if (ret < 0) {
		printf("Failed to set transfer size PF :%d QID :%d\n",
				q_info->pf, q_info->qid);
	}

	/* num packets to generate, setting to 1 */
	ret = qdma_register_write(q_info->pf, user_bar, 0x20,
			1);
	if (ret < 0) {
		printf("Failed to set transfer size PF :%d QID :%d\n",
				q_info->pf, q_info->qid);
	}

	/* trigger data generator */ 
	ret = qdma_register_write(q_info->pf, user_bar, 0x08,
			2);
	if (ret < 0) {
		printf("Failed to set transfer size PF :%d QID :%d\n",
				q_info->pf, q_info->qid);
	}

	return 0;
}

static int qdmautils_read(struct queue_info *q_info,
		char *output_file, int io_type)
{
	int outfile_fd = -1;
	char *buffer = NULL;
	char *allocated = NULL;
	unsigned int size;
	unsigned int offset;
	int ret;

	if (!q_info || !input_file) {
		printf("Error: Invalid input params\n");
		return -EINVAL;
	}

	size = pkt_sz;

	outfile_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC);
	if (outfile_fd < 0) {
		printf("Error: unable to open/create output file %s, ret :%d\n",
				output_file, outfile_fd);
		perror("open/create output file");
		return outfile_fd; 
	}

	offset = 0;
	posix_memalign((void **)&allocated, 4096 /*alignment */ , size + 4096);
	if (!allocated) {
		printf("Error: OOM %u.\n", size + 4096);
		ret = -ENOMEM;
		goto out;
	}
	buffer = allocated + offset;

	if (io_type == 0) {
		ret = qdmautils_sync_xfer(q_info->q_name,
				q_info->dir, buffer, size);
		if (ret < 0)
			printf("Error: QDMA SYNC transfer Failed, ret :%d\n", ret);
		else
			printf("PF :%d Queue :%d C2H Sync transfer success\n", q_info->pf, q_info->qid); 
	} else {
		ret = qdmautils_async_xfer(q_info->q_name,
				q_info->dir, buffer, size);
		if (ret < 0)
			printf("Error: QDMA ASYNC transfer Failed, ret :%d\n", ret);
		else
			printf("PF :%d Queue :%d C2H ASync transfer success\n", q_info->pf, q_info->qid); 
	}
	if (ret < 0)
		goto out;

	ret = write_from_buffer(output_file, outfile_fd, buffer, size, offset);
	if (ret < 0)
		printf("Error: Write from buffer to %s failed\n", output_file);
out:
	free(allocated);
	close(outfile_fd);

	return ret;
}

static int qdmautils_write(struct queue_info *q_info,
		char *input_file, int io_type)
{
	int infile_fd = -1;
	int outfile_fd = -1;
	char *buffer = NULL;
	char *allocated = NULL;
	unsigned int size;
	unsigned int offset;
	int ret;
	enum qdmautils_io_dir dir;

	if (!q_info || !input_file) {
		printf("Error: Invalid input params\n");
		return -EINVAL;
	}

	size = pkt_sz;

	infile_fd = open(input_file, O_RDONLY | O_NONBLOCK);
	if (infile_fd < 0) {
		printf("Error: unable to open input file %s, ret :%d\n",
				input_file, infile_fd);
		return infile_fd;
	}

	offset = 0;
	posix_memalign((void **)&allocated, 4096 /*alignment */ , size + 4096);
	if (!allocated) {
		printf("Error: OOM %u.\n", size + 4096);
		ret = -ENOMEM;
		goto out;
	}

	buffer = allocated + offset;
	ret = read_to_buffer(input_file, infile_fd, buffer, size, 0);
	if (ret < 0)
		goto out;

	if (io_type == 0) {
		ret = qdmautils_sync_xfer(q_info->q_name,
				q_info->dir, buffer, size);
		if (ret < 0)
			printf("Error: QDMA SYNC transfer Failed, ret :%d\n", ret);
		else
			printf("PF :%d Queue :%d H2C Sync transfer success\n", q_info->pf, q_info->qid); 
	} else {
		ret = qdmautils_async_xfer(q_info->q_name,
				q_info->dir, buffer, size);
		if (ret < 0)
			printf("Error: QDMA ASYNC transfer Failed, ret :%d\n", ret);
		else
			printf("PF :%d Queue :%d H2C Async transfer success\n", q_info->pf, q_info->qid); 
	}

out:
	free(allocated);
	close(infile_fd);

	return ret;
}

static int qdmautils_xfer(struct queue_info *q_info,
		unsigned int count, int io_type)
{
	int ret;
	int i;

	if (!q_info || count == 0) {
		printf("Error: Invalid input params\n");
		return -EINVAL;
	}

	for (i = 0; i < count; i++) {
		if (q_info[i].dir == DMAXFER_IO_WRITE) {
			/* Transfer DATA from inputfile to Device */
			ret = qdmautils_write(q_info + i, input_file, io_type);
			if (ret < 0)
				printf("qdmautils_write failed, ret :%d\n", ret);
		} else {
			if (mode == QDMA_Q_MODE_ST) {
				/* Generate ST - C2H Data before trying to read from Card */
				ret = qdma_trigger_data_generator(q_info + i);
				if (ret < 0) {
					printf("Failed to trigger data generator\n");
					return ret;
				}
			}
			/* Reads data from Device and writes into output file */
			ret = qdmautils_read(q_info + i, output_file, io_type);
			if (ret < 0)
				printf("qdmautils_read failed, ret :%d\n", ret);
		}

		if (ret < 0)
			break;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	char *cfg_fname;
	int cmd_opt;
	int ret;

	if (argc == 2) {
		if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
			printf("%s version %s\n", PROGNAME, VERSION);
			printf("%s\n", COPYRIGHT);
			return 0;	
		}
	}

	cfg_fname = NULL;
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

	if (cfg_fname == NULL) {
		printf("Config file required.\n");
		usage(argv[0]);
		return -EINVAL;
	}

	ret = parse_config_file(cfg_fname);
	if (ret < 0) {
		printf("Config File has invalid parameters\n");
		return ret;
	}

	ret = qdma_validate_qrange();
	if (ret < 0)
		return ret;

	q_count = 0;
	/* Addition and Starting of queues handled here */
	q_count = qdma_setup_queues(&q_info);
	if (q_count < 0) {
		printf("qdma_setup_queues failed, ret:%d\n", q_count);
		return q_count; 
	}

	/* queues has to be deleted upon termination */
	atexit(qdma_env_cleanup);
	/* Perform DMA transfers on each Queue */
	ret = qdmautils_xfer(q_info, q_count, io_type);
	if (ret < 0)
		printf("Qdmautils Transfer Failed, ret :%d\n", ret);

	return ret;
}
