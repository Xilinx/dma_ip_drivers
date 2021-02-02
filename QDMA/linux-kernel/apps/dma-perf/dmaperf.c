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

#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <linux/types.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <stddef.h>
#include <errno.h>
#include <error.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include </usr/include/pthread.h>
#include <libaio.h>
#include <sys/sysinfo.h>
#include "dmautils.h"
#include "qdma_nl.h"

#define SEC2NSEC           1000000000
#define SEC2USEC           1000000
#define DEFAULT_PAGE_SIZE  4096
#define PAGE_SHIFT         12

#define DATA_VALIDATION 0

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

#define MSEC2NSEC 1000000
#define CMPL_STATUS_ACC_CMD_LEN 200
#define PCI_DUMP_CMD_LEN 100

#define QDMA_UL_SEND_MARKER_PACKET     (1 << 5)
#define QDMA_UL_IMM_DUMP_C2H_DATA      (1 << 17)
#define QDMA_UL_STOP_C2H_TRANSFER      (1 << 18)
#define QDMA_UL_DROP_ENABLE            (1 << 19)
#define QDMA_UL_IMM_DUMP_CMPT_FIFO     (1 << 20)
#define QDMA_UL_STOP_CMPT_TRANSFER     (1 << 21)

#define QDMA_GLBL_MAX_ENTRIES  (16)
//#define DEBUG

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

#define THREADS_SET_CPU_AFFINITY 0

struct io_info {
	unsigned int num_req_submitted;
	unsigned int num_req_completed;
	unsigned int num_req_completed_in_time;
	int exit_check_count;
	struct list_head *head;
	struct list_head *tail;
	sem_t llock;
	int pid;
	pthread_t evt_id;
	char q_name[20];
	char trig_mode[10];
	unsigned char q_ctrl;
	unsigned int q_added;
	unsigned int q_started;
	unsigned int q_wait_for_stop;
	int fd;
	unsigned int pf;
	unsigned int qid;
	enum q_mode mode;
	enum q_dir dir;
	unsigned int idx_tmr;
	unsigned int idx_cnt;
	unsigned int idx_rngsz;
	unsigned int pfetch_en;
	unsigned int pkt_burst;
	unsigned int pkt_sz;
	unsigned int cmptsz;
	unsigned int stm_mode;
	unsigned int pipe_gl_max;
	unsigned int pipe_flow_id;
	unsigned int pipe_slr_id;
	unsigned int pipe_tdest;
	unsigned int mm_chnl;
	int keyhole_en;
	unsigned int aperture_sz;
	unsigned int offset;
#ifdef DEBUG
	unsigned long long total_nodes;
	unsigned long long freed_nodes;
#endif
	unsigned int thread_id;
#if THREADS_SET_CPU_AFFINITY
	int cpu;
#endif
};

struct list_head {
	struct list_head *next;
	unsigned int max_events;
	unsigned int completed_events;
	io_context_t ctxt;
};

#define container_of(ptr, type, member) ({                      \
        const struct iocb *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

static unsigned int *io_exit = 0;
int io_exit_id;
static unsigned int mm_chnl = 0;
static unsigned int force_exit = 0;
static unsigned int num_q = 0;
static unsigned int pkt_sz = 0;
static unsigned int num_pkts;
static int keyhole_en = 0;
static unsigned int aperture_sz = 0;
static unsigned int offset = 0;
static unsigned int tsecs = 0;
struct io_info *info = NULL;
static char cfg_name[20];
static unsigned int pci_bus = 0;
static unsigned int pci_dev = 0;
static unsigned int vf_perf = 0;
static char *dmactl_dev_prefix_str;
char *pf_dmactl_prefix_str = "qdma";
char *vf_dmactl_prefix_str = "qdmavf";
unsigned int num_thrds = 0;
unsigned int num_thrds_per_q = 1;
int shmid;
int base_pid;
enum q_mode mode;
enum q_dir dir;
unsigned int num_pf = 0;
unsigned int pf_start = 0;
unsigned int q_start = 0;
unsigned int idx_rngsz = 0;
unsigned int idx_tmr = 0;
unsigned int idx_cnt = 0;
unsigned int pfetch_en = 0;
unsigned int cmptsz = 0;
unsigned int no_memcpy = 1;
unsigned int stm_mode = 0;
unsigned int *pipe_gl_max_lst = NULL;
unsigned int *pipe_flow_id_lst = NULL;
unsigned int *pipe_slr_id_lst = NULL;
unsigned int *pipe_tdest_lst = NULL;
char trigmode[10];
char pci_dump[PCI_DUMP_CMD_LEN];
unsigned int dump_en = 0;
unsigned int marker_en = 1;
static struct timespec g_ts_start;
static unsigned char *q_lst_stop = NULL;
int q_lst_stop_mid;
int *child_pid_lst = NULL;
unsigned int glbl_rng_sz[QDMA_GLBL_MAX_ENTRIES];
#if THREADS_SET_CPU_AFFINITY
unsigned int num_processors = 1;
#endif
#if DATA_VALIDATION
unsigned short valid_data[2*1024];
#endif

static void clear_events(struct io_info *_info, struct list_head *node);
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
	if (stm_mode) {
		printf("pipe_gl_max = %u\n", _info->pipe_gl_max);
		printf("pipe_flow_id = %u\n", _info->pipe_flow_id);
		printf("pipe_slr_id = %u\n", _info->pipe_slr_id);
		printf("pipe_tdest = %u\n", _info->pipe_tdest);
	}
#if THREADS_SET_CPU_AFFINITY
	printf("cpu = %u\n", _info->cpu);
#endif
}

struct dma_meminfo {
	void *memptr;
	unsigned int num_blks;
};

#define USE_MEMPOOL

struct mempool_handle {
	void *mempool;
	unsigned int mempool_blkidx;
	unsigned int mempool_blksz;
	unsigned int total_memblks;
	struct dma_meminfo *mempool_info;
#ifdef DEBUG
	unsigned int id;
	unsigned int loop;
#endif
};

static struct mempool_handle ctxhandle;
static struct mempool_handle iocbhandle;
static struct mempool_handle datahandle;

static void mempool_create(struct mempool_handle *mpool, unsigned int entry_size, unsigned int max_entries)
{
#ifdef USE_MEMPOOL
	if (posix_memalign((void **)&mpool->mempool, DEFAULT_PAGE_SIZE,
			   max_entries * (entry_size + sizeof(struct dma_meminfo)))) {
		printf("OOM\n");
		exit(1);
	}
	mpool->mempool_info = (struct dma_meminfo *)(((char *)mpool->mempool) + (max_entries * entry_size));
#endif
	mpool->mempool_blksz = entry_size;
	mpool->total_memblks = max_entries;
	mpool->mempool_blkidx = 0;
}

static void mempool_free(struct mempool_handle *mpool)
{
#ifdef USE_MEMPOOL
	free(mpool->mempool);
	mpool->mempool = NULL;
#endif
}

static void *dma_memalloc(struct mempool_handle *mpool, unsigned int num_blks)
{
	unsigned int _mempool_blkidx = mpool->mempool_blkidx;
	unsigned int tmp_blkidx = _mempool_blkidx;
	unsigned int max_blkcnt = tmp_blkidx + num_blks;
	unsigned int i, avail = 0;
	void *memptr = NULL;
	char *mempool = mpool->mempool;
	struct dma_meminfo *_mempool_info = mpool->mempool_info;
	unsigned int _total_memblks = mpool->total_memblks;

#ifdef USE_MEMPOOL
	if (max_blkcnt > _total_memblks) {
		tmp_blkidx = 0;
		max_blkcnt = num_blks;
	}
	for (i = tmp_blkidx; (i < _total_memblks) && (i < max_blkcnt); i++) {
		if (_mempool_info[i].memptr) { /* occupied blks ahead */
			i += _mempool_info[i].num_blks;
			max_blkcnt = i + num_blks;
			avail = 0;
			tmp_blkidx = i;
		} else
			avail++;
		if (max_blkcnt > _total_memblks) { /* reached the end of mempool. circle through*/
			if (num_blks > _mempool_blkidx) return NULL; /* Continuous num_blks not available */
			i = 0;
			avail = 0;
			max_blkcnt = num_blks;
			tmp_blkidx = 0;
		}
	}
	if (avail < num_blks) { /* no required available blocks */
		return NULL;
	}

	memptr = &(mempool[tmp_blkidx * mpool->mempool_blksz]);
	_mempool_info[tmp_blkidx].memptr = memptr;
	_mempool_info[tmp_blkidx].num_blks = num_blks;
	mpool->mempool_blkidx = tmp_blkidx + num_blks;
#else
	memptr = calloc(num_blks, mpool->mempool_blksz);
#endif

	return memptr;
}

static void dma_free(struct mempool_handle *mpool, void *memptr)
{
#ifdef USE_MEMPOOL
	struct dma_meminfo *_meminfo = mpool->mempool_info;
	unsigned int idx;

	if (!memptr) return;

	idx = (memptr - mpool->mempool)/mpool->mempool_blksz;
#ifdef DEBUG
	if (idx >= mpool->total_memblks) {
		printf("Asserting: %u:Invalid memory index %u acquired\n", mpool->id, idx);
		while(1);
	}
#endif

	_meminfo[idx].num_blks = 0;
	_meminfo[idx].memptr = NULL;
#else
	free(memptr);
#endif
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

static void create_thread_info(void)
{
	unsigned int base = 0;
	unsigned int dir_factor = 1;
	unsigned int q_ctrl = 1;
	unsigned int i, j, k;
	struct io_info *_info;
	int last_fd = -1;
	unsigned char is_new_fd = 1;
#if THREADS_SET_CPU_AFFINITY
	int h2c_cpu = 0;
	int max_h2c_cpu = (num_processors / 2);
	int c2h_cpu = max_h2c_cpu;
	int max_c2h_cpu = (num_processors / 2) + ((num_processors % 2) ? 1 : 0);
#endif

	if (dir == Q_DIR_BI)
		dir_factor = 2;
	if ((shmid = shmget(IPC_PRIVATE, num_thrds * sizeof(struct io_info), IPC_CREAT | 0666)) < 0)
	{
		perror("smget returned -1\n");
		error(-1, errno, " ");
		exit(-1);
	}
	if ((q_lst_stop_mid = shmget(IPC_PRIVATE, dir_factor * num_q * num_pf, IPC_CREAT | 0666)) < 0)
	{
		perror("smget returned -1\n");
		error(-1, errno, " ");
		exit(-1);
	}
	if ((q_lst_stop = (unsigned char *) shmat(q_lst_stop_mid, NULL, 0)) == (unsigned char *) -1) {
		perror("Process shmat returned NULL\n");
		error(-1, errno, " ");
		exit(1);
	}
	memset(q_lst_stop, 0 , num_q);
	if (shmdt(q_lst_stop) == -1) {
		perror("shmdt returned -1\n");
	        error(-1, errno, " ");
	}
	if ((_info = (struct io_info *) shmat(shmid, NULL, 0)) == (struct io_info *) -1) {
		perror("Process shmat returned NULL\n");
		error(-1, errno, " ");
	}

	if ((io_exit_id = shmget(IPC_PRIVATE,sizeof(unsigned int), IPC_CREAT | 0666)) < 0)
	{
		perror("smget returned -1\n");
		error(-1, errno, " ");
		exit(-1);
	}

	if ((io_exit = (unsigned int *) shmat(io_exit_id, NULL, 0)) == (unsigned int *) -1) {
		perror("Process shmat returned NULL\n");
		error(-1, errno, " ");
	}
	prep_pci_dump();
	if ((mode == Q_MODE_ST) && (dir != Q_DIR_H2C)) {
		qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x50, cmptsz);
		qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x90, pkt_sz);
		usleep(1000);
	}

	base = 0;
	for (k = 0; k < num_pf; k++) {
		for (i = 0 ; i < num_q; i++) {
			q_ctrl = 1;
#if THREADS_SET_CPU_AFFINITY
			if (h2c_cpu >= max_h2c_cpu)
				h2c_cpu = 0;
			if (c2h_cpu >= (max_h2c_cpu + max_h2c_cpu))
				c2h_cpu = max_h2c_cpu;
#endif
			for (j = 0; j < num_thrds_per_q; j++) {
				is_new_fd = 1;
				if ((dir == Q_DIR_H2C) || (dir == Q_DIR_BI)) {
					snprintf(_info[base].q_name, 20, "%s%02x%02x%01x-%s-%u",
					dmactl_dev_prefix_str, pci_bus, pci_dev,
					pf_start+k, (mode == Q_MODE_MM) ? "MM" : "ST", q_start + i);
					_info[base].dir = Q_DIR_H2C;
					_info[base].mode = mode;
					_info[base].idx_rngsz = idx_rngsz;
					_info[base].pf = (pci_bus << 12) | (pci_dev << 4) | (pf_start + k);
					_info[base].qid = q_start + i;
					_info[base].q_ctrl = q_ctrl;
					_info[base].fd = last_fd;
					_info[base].pkt_burst = num_pkts;
					_info[base].mm_chnl = mm_chnl;
					_info[base].pkt_sz = pkt_sz;
					if ((_info[base].mode == Q_MODE_ST) &&
							(stm_mode)) {
						_info[base].stm_mode = stm_mode;
						_info[base].pipe_gl_max = pipe_gl_max_lst[(k*num_q) + i];
						_info[base].pipe_flow_id = pipe_flow_id_lst[(k*num_q) + i];
						_info[base].pipe_slr_id = pipe_slr_id_lst[(k*num_q) + i];
						_info[base].pipe_tdest = pipe_tdest_lst[(k*num_q) + i];
					}

					if (_info[base].mode == Q_MODE_MM &&
							keyhole_en) {
						_info[base].aperture_sz = aperture_sz;
					}
					_info[base].offset = offset;
#if THREADS_SET_CPU_AFFINITY
					_info[base].cpu = h2c_cpu;
#endif
					sem_init(&_info[base].llock, 0, 1);
					if (q_ctrl != 0) {
						last_fd = setup_thrd_env(&_info[base], is_new_fd);
					}
					_info[base].thread_id = base;
//					dump_thrd_info(&_info[base]);
					_info[base].exit_check_count = 0;
					base++;
					is_new_fd = 0;
				}
				if (dir != Q_DIR_H2C)
				{
					snprintf(_info[base].q_name, 20, "%s%02x%02x%01x-%s-%u",
					dmactl_dev_prefix_str, pci_bus, pci_dev, pf_start+k, (mode == Q_MODE_MM) ? "MM" : "ST", q_start + i);
					_info[base].exit_check_count = 0;
					_info[base].dir = Q_DIR_C2H;
					_info[base].mode = mode;
					_info[base].idx_rngsz = idx_rngsz;
					_info[base].pf = (pci_bus << 12) | (pci_dev << 4) | (pf_start + k);
					_info[base].qid = q_start + i;
					_info[base].q_ctrl = q_ctrl;
					_info[base].pkt_burst = num_pkts;
					_info[base].mm_chnl = mm_chnl;
					_info[base].pkt_sz = pkt_sz;
#if THREADS_SET_CPU_AFFINITY
					_info[base].cpu = c2h_cpu;
#endif
					if (_info[base].mode == Q_MODE_ST) {
						_info[base].pfetch_en = pfetch_en;
						_info[base].idx_cnt = idx_cnt;
						_info[base].idx_tmr = idx_tmr;
						_info[base].cmptsz = cmptsz;
						strncpy(_info[base].trig_mode, trigmode, 10);
						if (stm_mode) {
							_info[base].stm_mode = stm_mode;
							_info[base].pipe_gl_max = pipe_gl_max_lst[(k*num_q) + i];
							_info[base].pipe_flow_id = pipe_flow_id_lst[(k*num_q) + i];
							_info[base].pipe_slr_id = pipe_slr_id_lst[(k*num_q) + i];
							_info[base].pipe_tdest = pipe_tdest_lst[(k*num_q) + i];
						}
					}
					sem_init(&_info[base].llock, 0, 1);
					_info[base].fd = last_fd;
					if (q_ctrl != 0) {
						last_fd = setup_thrd_env(&_info[base], is_new_fd);
					}
					_info[base].thread_id = base;
//					dump_thrd_info(&_info[base]);
					base++;
				}
				q_ctrl = 0;
			}
#if THREADS_SET_CPU_AFFINITY
			h2c_cpu++;
			c2h_cpu++;
#endif
		}
	}
	if ((mode == Q_MODE_ST) && (dir != Q_DIR_H2C)) {
		if (!stm_mode) {
			qdma_register_write(vf_perf, (pci_bus << 12) | (pci_dev << 4) | pf_start, 2, 0x08,
					QDMA_UL_IMM_DUMP_C2H_DATA | QDMA_UL_IMM_DUMP_CMPT_FIFO/* | QDMA_UL_DROP_ENABLE*/);
			usleep(1000);
		}
	}
	if (shmdt(_info) == -1) {
		perror("shmdt returned -1\n");
	        error(-1, errno, " ");
	}
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
	char rng_sz_path[200] = {'\0'};
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
		if (!strncmp(config, "mode", 4)) {
			if (!strncmp(value, "mm", 2))
				mode = Q_MODE_MM;
			else if(!strncmp(value, "st", 2))
				mode = Q_MODE_ST;
			else {
				printf("Error: Unkown mode");
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "dir", 3)) {
			if (!strncmp(value, "h2c", 3))
				dir = Q_DIR_H2C;
			else if(!strncmp(value, "c2h", 3))
				dir = Q_DIR_C2H;
			else if(!strncmp(value, "bi", 2))
				dir = Q_DIR_BI;
			else {
				printf("Error: Unkown dir");
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "name", 3)) {
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
		} else if (!strncmp(config, "num_threads", 11)) {
			if (arg_read_int(value, &num_thrds_per_q)) {
				printf("Error: Invalid num_threads:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "pkt_sz", 6)) {
			if (arg_read_int(value, &pkt_sz)) {
				printf("Error: Invalid pkt_sz:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "mm_chnl", 7)) {
			if (arg_read_int(value, &mm_chnl)) {
				printf("Error: Invalid mm_chnl:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "num_pkt", 7)) {
			if (arg_read_int(value, &num_pkts)) {
				printf("Error: Invalid num_pkt:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "aperture_sz", 11)) {
			if (arg_read_int(value, &aperture_sz)) {
				printf("Error: Invalid aperture size:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "offset", 6)) {
			if (arg_read_int(value, &offset)) {
				printf("Error: Invalid aperture offset:%s\n", value);
				goto prase_cleanup;
			}
		}else if (!strncmp(config, "keyhole_en", 7)) {

			if (arg_read_int(value, &keyhole_en)) {
				printf("Error: Invalid keyhole option:%s\n", value);
				goto prase_cleanup;
			}
		}
		else if (!strncmp(config, "no_memcpy", 9)) {
			if (arg_read_int(value, &no_memcpy)) {
				printf("Error: Invalid no_memcpy:%s\n", value);
				goto prase_cleanup;
			}
		} else if (!strncmp(config, "stm_mode", 8)) {
			if (arg_read_int(value, &stm_mode)) {
				printf("Error: Invalid stm_mode:%s\n", value);
				goto prase_cleanup;
			}
	        } else if (!strncmp(config, "pipe_gl_max_lst", 15)) {
	            int arr_len = get_array_len(value);
	            int i;

	            if ((arr_len < 0) || (arr_len != (int)(num_q*num_pf))) {
	                printf("ERROR: Invalid number of entries in pipe_gl_max_lst - %s\n", value);
	                exit(1);
	            }
	            if (arr_len) {
			    pipe_gl_max_lst = (unsigned int *)calloc(arr_len, sizeof(unsigned int));
			    int ret = arg_read_int_array(value, pipe_gl_max_lst, arr_len);
			    if (ret <= 0) {
				printf("Error: Invalid pipe_gl_max_lst:%s\n", value);
				exit(1);
			    }
	            }
	        } else if (!strncmp(config, "pipe_flow_id_lst", 16)) {
	            int arr_len = get_array_len(value);
	            int i;

	            if ((arr_len < 0) || (arr_len != (int)(num_q*num_pf))) {
	                printf("ERROR: Invalid number of entries in pipe_flow_id_lst - %s\n", value);
	                exit(1);
	            }
	            if (arr_len) {
			    pipe_flow_id_lst = (unsigned int *)calloc(arr_len, sizeof(unsigned int));
			    int ret = arg_read_int_array(value, pipe_flow_id_lst, arr_len);
			    if (ret <= 0) {
				printf("Error: Invalid pipe_flow_id_lst:%s\n", value);
				exit(1);
			    }
	            }
	        } else if (!strncmp(config, "pipe_slr_id_lst", 15)) {
	            int arr_len = get_array_len(value);
	            int i;

	            if ((arr_len < 0) || (arr_len != (int)(num_q*num_pf))) {
	                printf("ERROR: Invalid number of entries in pipe_slr_id_lst - %s\n", value);
	                exit(1);
	            }
	            if (arr_len) {
			    pipe_slr_id_lst = (unsigned int *)calloc(arr_len, sizeof(unsigned int));
			    int ret = arg_read_int_array(value, pipe_slr_id_lst, arr_len);
			    if (ret <= 0) {
				printf("Error: Invalid pipe_slr_id_lst:%s\n", value);
				exit(1);
			    }
	            }
	        } else if (!strncmp(config, "pipe_tdest_lst", 15)) {
	            int arr_len = get_array_len(value);
	            int i;

	            if ((arr_len < 0) || (arr_len != (int)(num_q*num_pf))) {
	                printf("ERROR: Invalid number of entries in pipe_tdest_lst - %s\n", value);
	                exit(1);
	            }
	            if (arr_len) {
			    pipe_tdest_lst = (unsigned int *)calloc(arr_len, sizeof(unsigned int));
			    int ret = arg_read_int_array(value, pipe_tdest_lst, arr_len);
			    if (ret <= 0) {
				printf("Error: Invalid pipe_tdest_lst:%s\n", value);
				exit(1);
			    }
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

		} else if (!strncmp(config, "marker_en", 9)) {
			char *p;

			marker_en = strtoul(value, &p, 16);
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
	if (stm_mode && (!pipe_gl_max_lst || !pipe_flow_id_lst ||
			!pipe_slr_id_lst || !pipe_tdest_lst)) {
		printf("Error: STM Attributes missing\n");
		exit(1);
	}

	if (!pci_bus && !pci_dev) {
		printf("Error: PCI bus information not provided\n");
		exit(1);
	}

	snprintf(rng_sz_path, 200,"dma-ctl %s%05x global_csr | grep \"Global Ring\"| cut -d \":\" -f 2 > glbl_rng_sz",
			 dmactl_dev_prefix_str, (pci_bus << 12) | (pci_dev << 4) | pf_start);
	system(rng_sz_path);
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

	if (dir == Q_DIR_BI)
		dir_factor = 2;
	num_thrds = num_pf * num_q * dir_factor * num_thrds_per_q;
	create_thread_info();
	if (stm_mode) {
		free(pipe_tdest_lst);
		free(pipe_slr_id_lst);
		free(pipe_flow_id_lst);
		free(pipe_gl_max_lst);
	}
	return;

prase_cleanup:
	fclose(fp);
}

#define MAX_AIO_EVENTS 65536

static void list_add_tail(struct io_info *_info, struct list_head *node)
{
    sem_wait(&_info->llock);
    if (_info->head == NULL) {
        _info->head = node;
        _info->tail = node;
    } else {
        _info->tail->next = node;
        _info->tail = node;
    }
#ifdef DEBUG
    _info->total_nodes++;
#endif
    sem_post( &_info->llock);
}

static void list_add_head(struct io_info *_info, struct list_head *node)
{
    sem_wait(&_info->llock);
    node->next = _info->head;
    if (_info->head == NULL) {
        _info->tail = node;
    }
    _info->head = node;
    sem_post( &_info->llock);
}

static struct list_head *list_pop(struct io_info *_info)
{
	struct list_head *node = NULL;

	sem_wait(&_info->llock);
	node = _info->head;
	if (_info->head == _info->tail)
		_info->tail = NULL;

	if (node)
		_info->head = node->next;

	sem_post(&_info->llock);

	return node;
}

static void list_free(struct io_info *_info)
{
	struct list_head *node = NULL;

	sem_wait(&_info->llock);
#ifdef DEBUG
	printf("Need to free %llu nodes in thrd%u\n", _info->total_nodes - _info->freed_nodes, _info->thread_id);
#endif
	node = _info->head;

	while (node != NULL) {
		struct list_head *prev_node = NULL;

		clear_events(_info, node);
		io_destroy(node->ctxt);
		prev_node = node;
		node = node->next;
		dma_free(&ctxhandle, prev_node);
#ifdef DEBUG
		_info->freed_nodes++;
#endif
	}
	sem_post(&_info->llock);
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

static void clear_events(struct io_info *_info, struct list_head *node) {
	struct io_event *events = NULL;
	int num_events = 0;
	unsigned int j, bufcnt;
	struct timespec ts_cur = {1, 0};

	if (node->max_events <= node->completed_events)
		return;
#ifdef DEBUG
	printf("Thrd%u: Need to clear %u/%u events in node %p\n",
	       _info->thread_id, node->max_events - node->completed_events,
	       node->max_events, node);
#endif
	events = calloc(node->max_events - node->completed_events, sizeof(struct io_event));
	if (events == NULL) {
		printf("OOM\n");
		return;
	}
	do {
		num_events = io_getevents(node->ctxt, 1,
					  node->max_events - node->completed_events, events,
					  &ts_cur);
		for (j = 0; (num_events > 0) && (j < num_events); j++) {
			struct iocb *iocb = (struct iocb *)events[j].obj;
			struct iovec *iov;

			node->completed_events++;
			if (!iocb) {
				printf("Error: Invalid IOCB from events\n");
				continue;
			}

			iov = (struct iovec *)iocb->u.c.buf;

			for (bufcnt = 0; bufcnt < iocb->u.c.nbytes; bufcnt++)
				dma_free(&datahandle, iov[bufcnt].iov_base);
			dma_free(&iocbhandle, iocb);
		}
	} while ((num_events > 0) && (node->max_events > node->completed_events));

	free(events);
}

// this function returns 1 as long as long as traffic can run,
// returns 0 when it is safe to exit the thread
static int thread_exit_check(struct io_info *_info) {
	if((0 == *io_exit) && (0 == force_exit) ){
		return 1;
	} else {
		if ((_info->num_req_submitted != _info->num_req_completed) && _info->exit_check_count < 10000) {
			if(_info->exit_check_count == 0) {
				_info->num_req_completed_in_time = _info->num_req_completed;
			}
			_info->exit_check_count++;
			usleep(100);
			return 1;
		} else {
		    printf("Exit Check: tid =%u, req_sbmitted=%u req_completed=%u dir=%s, intime=%u loop_count=%d, \n",
			   _info->thread_id, _info->num_req_submitted,
			   _info->num_req_completed,_info->dir == Q_DIR_H2C ? "H2C": "C2H",
			   _info->num_req_completed_in_time,  _info->exit_check_count);
		    if(_info->exit_check_count != 0 ) {
			    _info->num_req_completed = _info->num_req_completed_in_time;
		    }
		    return 0;
		}

	}
}



static void *event_mon(void *argp)
{
	struct io_info *_info = (struct io_info *)argp;
	unsigned int j, bufcnt;
	struct io_event *events = NULL;
	int num_events = 0;
	struct timespec ts_cur = {0, 0};
#if DATA_VALIDATION
	unsigned short *rcv_data;
	unsigned int k;
#endif

	events = calloc(MAX_AIO_EVENTS, sizeof(struct io_event));
	if (events == NULL) {
		printf("OOM\n");
		exit(1);
	}

	while (thread_exit_check(_info)) {
		struct list_head *node = list_pop(_info);

		if (!node)
			continue;

		memset(events, 0, MAX_AIO_EVENTS * sizeof(struct io_event));
		do {
			num_events = io_getevents(node->ctxt, 1,
						  node->max_events - node->completed_events, events,
						  &ts_cur);
			for (j = 0; (num_events > 0) && (j < num_events); j++) {
				struct iocb *iocb = (struct iocb *)events[j].obj;
				struct iovec *iov = NULL;

				if (!iocb) {
					printf("Error: Invalid IOCB from events\n");
					continue;
				}
				_info->num_req_completed += events[j].res;

				iov = (struct iovec *)(iocb->u.c.buf);
				if (!iov) {
					printf("invalid buffer\n");
					continue;
				}
#if DATA_VALIDATION
				rcv_data = iov[0].iov_base;
				for (k = 0; k < (iov[0].iov_len/2) && events[j].res && !(events[j].res2); k += 8) {
					printf("%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n", k,
							rcv_data[k], rcv_data[k+1], rcv_data[k+2],
							rcv_data[k+3], rcv_data[k+4], rcv_data[k+5],
							rcv_data[k+6], rcv_data[k+7]);
				}
#endif
				for (bufcnt = 0; (bufcnt < iocb->u.c.nbytes) && iov; bufcnt++)
					dma_free(&datahandle, iov[bufcnt].iov_base);
				dma_free(&iocbhandle, iocb);
			}
			if (num_events > 0)
			    node->completed_events += num_events;
			if (node->completed_events >= node->max_events) {
				io_destroy(node->ctxt);
				dma_free(&ctxhandle, node);
				break;
			}
		} while(thread_exit_check(_info));
		if (node->completed_events < node->max_events)
		    list_add_head(_info, node);
	}
	free(events);
#ifdef DEBUG
	printf("Exiting evt_thrd: %u\n", _info->thread_id);
#endif

	return NULL;
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
	unsigned int f_arg_set = 0;


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
	f_arg_set |= 1 << QPARM_IDX;
	qparm->fetch_credit = Q_ENABLE_C2H_FETCH_CREDIT;

	if (info->mode == Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (info->mode == Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;
	f_arg_set |= 1 << QPARM_MODE;
	if (info->dir == Q_DIR_H2C)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (info->dir == Q_DIR_C2H)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;

	f_arg_set |= 1 << QPARM_DIR;
	qparm->qrngsz_idx = info->idx_rngsz;
	f_arg_set |= 1 << QPARM_RNGSZ_IDX;
	if ((info->dir == Q_DIR_C2H) && (info->mode == Q_MODE_ST)) {
		if (cmptsz)
			qparm->cmpt_entry_size = info->cmptsz;
		else
			qparm->cmpt_entry_size = XNL_ST_C2H_CMPT_DESC_SIZE_8B;
		f_arg_set |= 1 << QPARM_CMPTSZ;
		qparm->cmpt_tmr_idx = info->idx_tmr;
		f_arg_set |= 1 << QPARM_CMPT_TMR_IDX;
		qparm->cmpt_cntr_idx = info->idx_cnt;
		f_arg_set |= 1 << QPARM_CMPT_CNTR_IDX;

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
		f_arg_set |= 1 << QPARM_CMPT_TRIG_MODE;
		if (pfetch_en)
			qparm->flags |= XNL_F_PFETCH_EN;
	}

	if (info->dir == Q_MODE_MM) {
		qparm->mm_channel = info->mm_chnl;
		f_arg_set |= 1 <<QPARM_MM_CHANNEL;
	}


	if ((info->dir == Q_DIR_H2C) && (info->mode == Q_MODE_MM)) {
		if (keyhole_en) {
			qparm->aperture_sz = info->aperture_sz;
			f_arg_set |= 1 << QPARM_KEYHOLE_EN;
		}
	}

	qparm->flags |= (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
			XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
			XNL_F_FETCH_CREDIT);

	qparm->sflags = f_arg_set;
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

static void io_proc_cleanup(struct io_info *_info)
{
	unsigned int i;
	unsigned int q_offset;
	unsigned int dir_factor = (dir == Q_DIR_BI) ? 1 : 2;
	unsigned int q_lst_idx_base;
	unsigned char is_q_stop = (_info->q_ctrl && _info->q_started);
	int reg_value = 0;

	*io_exit = 1;
	pthread_join(_info->evt_id, NULL);

	q_offset = (_info->dir == Q_DIR_H2C) ? 0 : num_q;
	if (dir != Q_DIR_BI)
		q_offset = 0;
	q_lst_idx_base = ((((_info->pf & 0x0000F) - pf_start) * num_q * dir_factor)
			  + q_offset);
	_info->q_wait_for_stop = 1;

	while (is_q_stop) {
		unsigned int j = 0;
		for (i = 0; i < num_thrds; i++) {
			if ((info[i].pf != _info->pf) || (info[i].qid != _info->qid)
			    || (info[i].dir != _info->dir))
				continue;
			if (info[i].q_wait_for_stop)
				j++;
			else
				break;
		}
		if (j != num_thrds_per_q)
			sched_yield();
		else
			break;
	}

	if ((mode == Q_MODE_ST) && (dir != Q_DIR_H2C)) {
		do {
			for (i = 0; i < num_thrds; i++) {
				if (!info[i].q_wait_for_stop)
					break;
			}
			if (i == num_thrds)
				break;
		} while (1);

		if (!stm_mode && _info->pid == base_pid) {
			qdma_register_write(vf_perf, _info->pf, 2, 0x08,
					    QDMA_UL_STOP_C2H_TRANSFER | QDMA_UL_STOP_CMPT_TRANSFER |
					    QDMA_UL_IMM_DUMP_CMPT_FIFO | QDMA_UL_IMM_DUMP_C2H_DATA);

			if (marker_en) {
				qdma_register_write(vf_perf, _info->pf, 2, 0x08,
						    QDMA_UL_IMM_DUMP_C2H_DATA | QDMA_UL_IMM_DUMP_CMPT_FIFO
						    | QDMA_UL_SEND_MARKER_PACKET);

				usleep(1000);

				unsigned retry = 50;
				while (retry) {
					qdma_register_read(vf_perf, _info->pf, 2, 0x18, &reg_value);

					usleep(500);
					if (reg_value == 0x1)
						break;
					else
						printf("Didnt received the c2h marker completion, retry count = %u\n",
						       (50 - (retry - 1)));

					usleep(500);
					retry--;
				}
			}
		}
	}

	if (is_q_stop) {
		int s;
		if (dump_en) {
			s = qdma_dump_queue(vf_perf, _info);
			if (s < 0) {
				printf("Failed: queue_dump\nerrcode = %d\n", s);
			}
		}

		if (_info->q_started) {
			s = qdma_stop_queue(vf_perf, _info);
			if (s < 0) {
				printf("Failed: queue_stop\nerrcode = %d\n", s);
			}
		}
		_info->q_started = 0;

		q_lst_stop[q_lst_idx_base + _info->qid - q_start] = 1;
	}

	while (!(q_lst_stop[q_lst_idx_base + _info->qid - q_start])) {
		sched_yield();
	}

	if (shmdt(q_lst_stop) == -1) {
		perror("shmdt returned -1\n");
		error(-1, errno, " ");
	}

	list_free(_info);

	mempool_free(&iocbhandle);
	mempool_free(&ctxhandle);
	mempool_free(&datahandle);
}

static void *io_thread(void *argp)
{
	struct io_info *_info = (struct io_info *)argp;
	int ret;
	int s;
	unsigned int max_io = MAX_AIO_EVENTS;
	unsigned int cnt = 0;
	pthread_attr_t attr;
	unsigned int io_sz = _info->pkt_sz;
	unsigned int burst_cnt = _info->pkt_burst;
	unsigned int num_desc;
	unsigned int max_reqs;
	struct iocb *io_list[1];

	if ((_info->mode == Q_MODE_ST) && (_info->dir == Q_DIR_C2H)) {
		io_sz = _info->pkt_burst * _info->pkt_sz;
		burst_cnt = 1;
	}
	num_desc = (io_sz + DEFAULT_PAGE_SIZE - 1) >> PAGE_SHIFT;
	max_reqs = glbl_rng_sz[idx_rngsz];
	mempool_create(&datahandle, num_desc*DEFAULT_PAGE_SIZE,  max_reqs + (burst_cnt * num_desc));
	mempool_create(&ctxhandle, sizeof(struct list_head), max_reqs);
	mempool_create(&iocbhandle, sizeof(struct iocb) + (burst_cnt * sizeof(struct iovec)), max_reqs + (burst_cnt * num_desc));
#ifdef DEBUG
	ctxhandle.id = 1;
	datahandle.id = 0;
	iocbhandle.id = 2;
#endif
	s = pthread_attr_init(&attr);
	if (s != 0)
		printf("pthread_attr_init failed\n");
	if (pthread_create(&_info->evt_id, &attr, event_mon, _info))
		exit(1);

	do {
		struct list_head *node = NULL;
		struct timespec ts_cur;

		if (tsecs) {
			if (clock_gettime(CLOCK_MONOTONIC, &ts_cur) != 0)
				break;
			timespec_sub(&ts_cur, &g_ts_start);
			if (ts_cur.tv_sec >= tsecs)
				break;
		}
		node = dma_memalloc(&ctxhandle, 1);
		if (!node) {
			continue;
		}
		ret = io_queue_init(max_io, &node->ctxt);
		if (ret != 0) {
			printf("Error: io_setup error %d on %u\n", ret, _info->thread_id);
			dma_free(&ctxhandle, node);
			sched_yield();
			continue;
		}
		cnt = 0;
		node->max_events = max_io;
		list_add_tail(_info, node);
		do {
			struct iovec *iov = NULL;
			unsigned int iovcnt;
			if (tsecs) {
				ret = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
				timespec_sub(&ts_cur, &g_ts_start);
				if (ts_cur.tv_sec >= tsecs) {
					node->max_events = cnt;
					break;
				}
			}

			if (((_info->num_req_submitted - _info->num_req_completed) *
					num_desc) > max_reqs) {
				sched_yield();
				continue;
			}

			io_list[0] = dma_memalloc(&iocbhandle, 1);
			if (io_list[0] == NULL) {
				if (cnt) {
					node->max_events = cnt;
					break;
				}
				else {
					sched_yield();
					continue;
				}
			}
			iov = (struct iovec *)(io_list[0] + 1);
			for (iovcnt = 0; iovcnt < burst_cnt; iovcnt++) {
				iov[iovcnt].iov_base = dma_memalloc(&datahandle, 1);
				if (iov[iovcnt].iov_base == NULL)
					break;
				iov[iovcnt].iov_len = io_sz;
			}
			if (iovcnt == 0) {
				dma_free(&iocbhandle, io_list[0]);
				continue;
			}
			if (_info->dir == Q_DIR_H2C) {
				io_prep_pwritev(io_list[0],
					       _info->fd,
					       iov,
					       iovcnt,
						  offset);
			} else {
				io_prep_preadv(io_list[0],
					       _info->fd,
					       iov,
					       iovcnt,
						  offset);
			}

			ret = io_submit(node->ctxt, 1, io_list);
			if(ret != 1) {
				printf("Error: io_submit error:%d on %s for %u\n", ret, _info->q_name, cnt);
				for (; iovcnt > 0; iovcnt--)
					dma_free(&datahandle, iov[iovcnt].iov_base);
				dma_free(&iocbhandle, io_list[0]);
				node->max_events = cnt;
				break;
			} else {
				cnt++;
				_info->num_req_submitted += iovcnt;
			}
		} while (tsecs && !force_exit && (cnt < max_io));
	} while (tsecs && !force_exit);

	io_proc_cleanup(_info);

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

	s = ioctl(_info->fd, 0, &no_memcpy);
	if (s != 0) {
		printf("failed to set non memcpy\n");
		exit(1);
	}

	return _info->fd;
}

static void dump_result(unsigned long long total_io_sz)
{
	unsigned long long gig_div = ((unsigned long long)tsecs * 1000000000);
	unsigned long long meg_div = ((unsigned long long)tsecs * 1000000);
	unsigned long long kil_div = ((unsigned long long)tsecs * 1000);
	unsigned long long byt_div = ((unsigned long long)tsecs);

	if ((total_io_sz/gig_div)) {
		printf("BW = %f GB/sec\n", ((double)total_io_sz/gig_div));
	} else if ((total_io_sz/meg_div)) {
		printf("BW = %f MB/sec\n", ((double)total_io_sz/meg_div));
	} else if ((total_io_sz/kil_div)) {
		printf("BW = %f KB/sec\n", ((double)total_io_sz/kil_div));
	} else
		printf("BW = %f Bytes/sec\n", ((double)total_io_sz/byt_div));
}

static int is_valid_fd(int fd)
{
    return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

static void cleanup(void)
{
	int i;
	unsigned long long total_num_h2c_ios = 0;
	unsigned long long total_num_c2h_ios = 0;
	unsigned long long total_io_sz = 0;
	int s;

	if ((*io_exit == 0)) {
		printf("force exit: cleaning up\n");
		force_exit = 1;
		if (getpid() != base_pid) {
			if (shmdt(info) == -1){
				perror("shmdt returned -1\n");
				error(-1, errno, " ");
			}
			if ((shmdt(q_lst_stop) == -1)) {
				perror("shmdt returned -1\n");
				error(-1, errno, " ");
			}
		} else
			for (i = 1; i < num_thrds; i++)
				wait(NULL);
	}/* else
		printf("normal exit: cleaning up\n");*/

	if (getpid() != base_pid) return;

	if (child_pid_lst != NULL)
		free(child_pid_lst);
	if (shmctl(q_lst_stop_mid, IPC_RMID, NULL) == -1) {
		perror("shmctl returned -1\n");
	        error(-1, errno, " ");
	}
	if (info == NULL) return;

	if (dump_en) {
		s = system(pci_dump);
		if (s != 0) {
		    printf("Failed: %s\nerrcode = %d\n", pci_dump, s);
		}

		if(!vf_perf) {
			s = qdma_registers_dump(vf_perf, &info[0]);
			if (s < 0) {
				printf("qdma_registers_dump failed. Errcode = %d\n", s);
			}
		}
	}

	for (i = 0; i < num_thrds; i++) {
		if (info[i].q_ctrl && info[i].q_started) {
			if (dump_en) {
				s = qdma_dump_queue(vf_perf, &info[i]);
				if (s < 0) {
					printf("Failed: queue_dump\nerrcode = %d\n", s);
				}
			}

			s = qdma_stop_queue(vf_perf, &info[i]);
			if (s < 0) {
				printf("Failed: queue_stop\nerrcode = %d\n", s);
			}
			info[i].q_started = 0;
		}
		if ((info[i].q_ctrl != 0) && (info[i].fd > 0) && is_valid_fd(info[i].fd))
			close(info[i].fd);
		if (info[i].q_ctrl && info[i].q_added) {
			s = qdma_del_queue(vf_perf, &info[i]);
			if (s < 0) {
				printf("Failed: queue_del\nerrcode = %d\n", s);
			}
			info[i].q_added = 0;
		}
	}


	if(marker_en) {
		if (info[0].mode == Q_MODE_ST && info[0].dir == Q_DIR_C2H) {
			qdma_register_write(vf_perf, info[0].pf, 2, 0x08,
					QDMA_UL_IMM_DUMP_C2H_DATA | QDMA_UL_IMM_DUMP_CMPT_FIFO);
		}
	}

	/* accumulate the statistics */
	for (i = 0; i < num_thrds; i++) {
		if (info[i].dir == Q_DIR_H2C)
			total_num_h2c_ios += info[i].num_req_completed;
		else {
			if (info[i].mode == Q_MODE_ST)
				info[i].num_req_completed *= info[i].pkt_burst;
			total_num_c2h_ios += info[i].num_req_completed;
		}
	}
	if (shmdt(info) == -1){
		perror("shmdt returned -1\n");
		error(-1, errno, " ");
	}
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("shmctl returned -1\n");
	        error(-1, errno, " ");
	}
	if (shmdt(io_exit) == -1){
		perror("shmdt returned -1\n");
		error(-1, errno, " ");
	}
	if (shmctl(io_exit_id, IPC_RMID, NULL) == -1) {
		perror("shmctl returned -1\n");
	        error(-1, errno, " ");
	}
	if (tsecs == 0) tsecs = 1;
	if (0 != total_num_h2c_ios) {
		total_io_sz = (total_num_h2c_ios*pkt_sz);
		printf("WRITE: total pps = %llu", total_num_h2c_ios/tsecs);
		printf(" ");
		dump_result(total_io_sz);
	}
	if (0 != total_num_c2h_ios) {
		total_io_sz = (total_num_c2h_ios*pkt_sz);
		printf("READ: total pps = %llu", total_num_c2h_ios/tsecs);
		printf(" ");
		dump_result(total_io_sz);
	}
	if ((0 == total_num_h2c_ios) && (0 == total_num_c2h_ios))
		printf("No IOs happened\n");
}

int main(int argc, char *argv[])
{
	int cmd_opt;
	char *cfg_fname = NULL;
	unsigned int i;
	unsigned int aio_max_nr = 0xFFFFFFFF;
	char aio_max_nr_cmd[100] = {'\0'};
#if THREADS_SET_CPU_AFFINITY
	cpu_set_t set;
#endif

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

#if THREADS_SET_CPU_AFFINITY
	num_processors = get_nprocs_conf();
	CPU_ZERO(&set);
#endif
#if DATA_VALIDATION
	for (i = 0; i < 2*1024; i++)
		valid_data[i] = i;
#endif
	parse_config_file(cfg_fname);
	atexit(cleanup);

	snprintf(aio_max_nr_cmd, 100, "echo %u > /proc/sys/fs/aio-max-nr", aio_max_nr);
	system(aio_max_nr_cmd);

	printf("dmautils(%u) threads\n", num_thrds);
	child_pid_lst = calloc(num_thrds, sizeof(int));
	base_pid = getpid();
	child_pid_lst[0] = base_pid;
	for (i = 1; i < num_thrds; i++) {
		if (getpid() == base_pid)
			child_pid_lst[i] = fork();
		else
			break;
	}
	if ((info = (struct io_info *) shmat(shmid, NULL, 0)) == (struct io_info *) -1) {
		perror("Process shmat returned NULL\n");
		error(-1, errno, " ");
		exit(1);
	}
	if ((q_lst_stop = (unsigned char *) shmat(q_lst_stop_mid, NULL, 0)) == (unsigned char *) -1) {
		perror("Process shmat returned NULL\n");
		error(-1, errno, " ");
		exit(1);
	}

	clock_gettime(CLOCK_MONOTONIC, &g_ts_start);
	if (getpid() == base_pid) {
		io_thread(&info[0]);
#if THREADS_SET_CPU_AFFINITY
		for (j = 0; j < num_processors; j++) {
			if (j != info[0].cpu)
				CPU_SET(j, &set);
		}
		if (sched_setaffinity(base_pid, sizeof(set), &set) == -1)
			printf("setaffinity for thrd%u failed\n", info[i].thread_id);
#endif
	        for(i = 1; i < num_thrds; i++) {
	            waitpid(child_pid_lst[i], NULL, 0);
	        }
	        free(child_pid_lst);
	        child_pid_lst = NULL;
	} else {
		info[i].pid = getpid();
#if THREADS_SET_CPU_AFFINITY
		for (j = 0; j < num_processors; j++) {
			if (j != info[i].cpu)
				CPU_SET(j, &set);
		}
		if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
			printf("setaffinity for thrd%u failed\n", info[i].thread_id);
#endif
		io_thread(&info[i - 1]);
		if ((shmdt(info) == -1)) {
			perror("shmdt returned -1\n");
			error(-1, errno, " ");
		}
	}

	*io_exit = 1;
	return 0;
}

