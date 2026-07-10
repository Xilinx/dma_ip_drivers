/*
 * This file is part of the MDB5 userspace application
 * to enable the user to execute the MDB5 functionality
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>
#include <ctype.h>

#include <linux/types.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include "version.h"
#include "dma_common_utils.h"
#include "dma_perf.h"

struct mdb5_dma_perf p;
struct mdb5_dma_common h;

#if THREADS_SET_CPU_AFFINITY
unsigned int num_processors = 1;
#endif

static const struct option long_opts[] = {
	{"config", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

static void usage(const char *name)
{
	int i = 0;

	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

	fprintf(stdout,
		"  -%c (--%s) config file that has configuration for IO\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,	"  -%c (--%s) print usage help and exit\n",
		long_opts[i].val, long_opts[i].name);
	i++;
	fprintf(stdout,	"  -%c (--%s) to print version name\n",
		long_opts[i].val, long_opts[i].name);
}

static int parse_options(int argc, char *argv[], char **cfg_fname)
{
	int ret = 0, cmd_opt = 0;

	if (argc == 1) {
		usage(argv[0]);
		ret = -EINVAL;
		goto error;
	}

	while ((cmd_opt = getopt_long(argc, argv, "vhxc:", long_opts,
				      NULL)) != -1) {
		switch (cmd_opt) {
		case 'c':
			/* config file name */
			*cfg_fname = strdup(optarg);
			break;
		case 'v':
			printf("%s version %s\n", PROGNAME, VERSION);
			printf("%s\n", COPYRIGHT);
			exit(EXIT_SUCCESS);
		case 'h':
		case 0:
			/* long option */
		default:
			usage(argv[0]);
			exit(0);
		}
	}

	if (!cfg_fname || *cfg_fname == NULL) {
		mdb5_dma_err("Config file required.\n");
		usage(argv[0]);
		ret = -EINVAL;
		goto error;
	}

error:
	return ret;
}

static int parse_config_file(char *cfg_fname, struct mdb5_dma_perf *p)
{
	struct mdb5_dma_common *h = p->h;
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	size_t numread, numblanks, linelen = 0;
	uint32_t linenum = 0;
	char *realbuf, *linebuf = NULL;
	char *config, *value;
	FILE *fp;

	fp = fopen(cfg_fname, "r");
	if (!fp) {
		mdb5_dma_err("Failed to open Config File [%s]: %s\n",
			 cfg_fname, mdb5_dma_strerror(errno));
		return -errno;
	}

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
		if (!strncmp(config, "name", 4)) {
			copy_value(value, h->cfg_name, 64);
		} else if (!strncmp(config, "chan_mode", 9)) {
			if (!strncmp(value, "mm", 2)) {
				h->chan_mode = MDB5_CHAN_MODE_MM;
			} else {
				mdb5_dma_err("Invalid channel mode: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "ch_start_idx", 12)) {
			uint32_t ch_start = 0;

			if (arg_read_int(value, &ch_start) || is_negative((int8_t)ch_start)) {
				mdb5_dma_err("Invalid channel start index: %s\n", value);
				goto parse_cleanup;
			}
			p->ch_start = (uint8_t)ch_start;
		} else if (!strncmp(config, "pci_bus", 7)) {
			uint32_t pci_bus = 0;

			if (arg_read_int(value, &pci_bus) ||
			    is_negative((int8_t)pci_bus)) {
				mdb5_dma_err("Invalid PCI bus: \"%s\\n", value);
				goto parse_cleanup;
			}
			p->pci_bus = (uint8_t)pci_bus;
		} else if (!strncmp(config, "pci_dev", 7)) {
			uint32_t pci_dev = 0;

			if (arg_read_int(value, &pci_dev) ||
			    is_negative((int8_t)pci_dev)) {
				mdb5_dma_err("Invalid PCI device idx: \"%s\\n",
					 value);
				goto parse_cleanup;
			}
			p->pci_dev = (uint8_t)pci_dev;
		} else if (!strncmp(config, "address", 7)) {
			if (arg_read_int_ull(value, &h->address)) {
				mdb5_dma_err("Invalid address: %s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_rd_channels", 15)) {
			if (arg_read_int(value, &h->num_rd_chan) ||
			    is_negative((int8_t)h->num_rd_chan)) {
				mdb5_dma_err("Invalid num of read channels:"
					 "%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_wr_channels", 15)) {
			if (arg_read_int(value, &h->num_wr_chan) ||
			    is_negative((int8_t)h->num_wr_chan)) {
				mdb5_dma_err("Invalid num of write channels:"
					 "%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "runtime", 7)) {
			if (arg_read_int(value, &p->tsecs)) {
				mdb5_dma_err("Invalid tsecs:%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_process", 11)) {
			if (arg_read_int(value, &p->num_thrds_per_chan)) {
				mdb5_dma_err("Invalid num_process:%s\n",
					 value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "num_pkt", 7)) {
			if (arg_read_int(value, &h->num_pkts) ||
			    is_negative((int)h->num_pkts)) {
				mdb5_dma_err("Invalid num_pkt:%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "pkt_sz", 6)) {
			if (arg_read_int_ull(value, &h->pkt_sz) ||
			    is_negative((int64_t)h->pkt_sz)) {
				mdb5_dma_err("Invalid pkt_sz:%s\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "aperture_sz", 11)) {
			if (arg_read_int(value, &hioc->caperture.aperture) ||
			    is_negative((int)hioc->caperture.aperture)) {
				mdb5_dma_err("Invalid aperture size %s "
					 "for SG DMA mode\n", value);
				goto parse_cleanup;
			}
		} else if (!strncmp(config, "dump_en", 7)) {
			if (!strncmp(value, "true", 4)) {
				h->dump_en = true;
			} else if (!strncmp(value, "false", 5)) {
				h->dump_en = false;
			} else {
				mdb5_dma_err("Invalid dump_en:%s\n", value);
				goto parse_cleanup;
			}
		}
	}
	if (linebuf)
		free(linebuf);

	fclose(fp);
	return 0;

parse_cleanup:
	if (linebuf)
		free(linebuf);

	fclose(fp);
	return -EINVAL;
}

static int validate_input_params(struct mdb5_dma_perf *p)
{
	struct mdb5_dma_common *h = p->h;
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	uint32_t num_rd_chan = h->num_rd_chan;
	uint32_t num_wr_chan = h->num_wr_chan;
	int ret = 0;

	if (!p->pci_bus && !p->pci_dev) {
		mdb5_dma_err("PCI bus information not provided\n");
		ret = -EINVAL;
		goto error;
	}

	if (num_wr_chan > MDB5_MAX_WR_CHAN ||
	    num_rd_chan > MDB5_MAX_RD_CHAN) {
		mdb5_dma_err("Input read or write channels exceeded %d or "
			"%d\n", MDB5_MAX_RD_CHAN, MDB5_MAX_WR_CHAN);
		ret = -EINVAL;
		goto error;
	}

	if (hioc->caperture.aperture > 0 && hioc->cmode.mode != MDB5_MODE_SG) {
		mdb5_dma_err("Aperture size=%d for Scatter Gather mode only, "
			"ignoring for other transfer modes\n",
			hioc->caperture.aperture);
		hioc->caperture.aperture = 0;
	}

error:
	return ret;
}

#define USE_MEMPOOL

struct pool_block {
    void* data;
    struct pool_block *next;
};

struct memory_pool {
    void *buffer;
    uint32_t block_size;
    uint32_t total_blocks;
    struct pool_block *blocks;
    struct pool_block *free_head;
    pthread_spinlock_t lock;
#ifdef DEBUG
    uint32_t alloc_count;
	uint32_t free_count;
#endif
};

static struct memory_pool ctxhandle;
static struct memory_pool iocbhandle;
static struct memory_pool datahandle;

static int memory_pool_create(struct memory_pool *pool, uint32_t block_size, uint32_t capacity)
{
#ifdef USE_MEMPOOL
    const size_t total_size = (size_t)capacity * block_size;
    uint32_t i;

    if (posix_memalign((void **)&pool->buffer, DEFAULT_PAGE_SIZE, total_size)) {
        mdb5_dma_err("%s : Out of memory\n", __func__);
        return -ENOMEM;
    }
    memset(pool->buffer, 0, total_size);

    pool->blocks = calloc(capacity, sizeof(struct pool_block));
    if (!pool->blocks) {
        mdb5_dma_err("Failed to allocate blocks: %s\n", mdb5_dma_strerror(errno));
        free(pool->buffer);
        return -ENOMEM;
    }

    pool->free_head = &pool->blocks[0];

    for (i = 0; i < capacity; i++) {
        pool->blocks[i].data = (char *)pool->buffer + (i * block_size);

        if (i < capacity - 1) {
            pool->blocks[i].next = &pool->blocks[i + 1];
        } else {
            pool->blocks[i].next = NULL;
        }
    }

    if (pthread_spin_init(&pool->lock, PTHREAD_PROCESS_PRIVATE) != 0) {
        mdb5_dma_err("Failed to init spinlock: %s\n", mdb5_dma_strerror(errno));
        free(pool->blocks);
        free(pool->buffer);
        return -errno;
    }

#ifdef DEBUG
    pool->alloc_count = 0;
	pool->free_count = 0;
#endif
	pool->block_size = block_size;
	pool->total_blocks = capacity;
#endif

	return 0;
}

#if DATA_VALIDATION
static inline void mdb5_buffer_fill_pattern(void *buffer, size_t size, uint32_t pattern)
{
	if (!buffer)
		return;

	uint32_t *data = (uint32_t *)buffer;
	for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
		data[i] = pattern;
	}
}
#endif

static void memory_pool_free(struct memory_pool *pool)
{
#ifdef USE_MEMPOOL
    if (pool->buffer) {
#ifdef DEBUG
        int32_t leaked = (int32_t)pool->alloc_count - (int32_t)pool->free_count;
        mdb5_dma_dbg("%s : allocs=%u, frees=%u, leaked=%d\n",
              __func__, pool->alloc_count, pool->free_count, leaked);

        if (leaked != 0) {
            mdb5_dma_warn("Pool has %d leaked blocks!\n", leaked);
        }
#endif
        pthread_spin_destroy(&pool->lock);
        free(pool->blocks);
        free(pool->buffer);
        pool->buffer = NULL;
        pool->blocks = NULL;
        pool->free_head = NULL;
    }
#endif
}

static void *memory_pool_alloc_block(struct memory_pool *pool)
{
    void *data = NULL;
    struct pool_block *block;

#ifdef USE_MEMPOOL

    pthread_spin_lock(&pool->lock);

    if (pool->free_head == NULL) {
        pthread_spin_unlock(&pool->lock);
        return NULL;
    }

    block = pool->free_head;
    pool->free_head = block->next;
    data = block->data;
	memset(data, 0, pool->block_size);

#ifdef DEBUG
    pool->alloc_count++;
#endif

#else
	data = calloc(1, pool->block_size);
#endif

	pthread_spin_unlock(&pool->lock);

	return data;
}

static void memory_pool_free_block(struct memory_pool *pool, void *buffer)
{
#ifdef USE_MEMPOOL
    uint32_t index;
    struct pool_block *block;

    if (!buffer)
        return;

    index = ((char *)buffer - (char *)pool->buffer) / pool->block_size;

#ifdef DEBUG
    if (index >= pool->total_blocks) {
        mdb5_dma_err("%s : free invalid index %u\n", __func__, index);
        return;
    }
#endif

    block = &pool->blocks[index];

    pthread_spin_lock(&pool->lock);

    block->next = pool->free_head;
    pool->free_head = block;

#ifdef DEBUG
    pool->free_count++;
#endif

    pthread_spin_unlock(&pool->lock);

#else
    free(buffer);
#endif
}

static struct sync_timer *sync_timer_init(uint32_t num_processes)
{
	pthread_mutexattr_t mutex_attr;
	struct sync_timer *timer;

	timer = mmap(NULL, sizeof(struct sync_timer), PROT_READ | PROT_WRITE,
		    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (timer == MAP_FAILED) {
		mdb5_dma_err("%s on %s\n", mdb5_dma_strerror(errno), __func__);
		return NULL;
	}

	memset(timer, 0, sizeof(struct sync_timer));
	timer->total_processes = num_processes;

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

	if (pthread_mutex_init(&timer->mutex, &mutex_attr) != 0) {
		mdb5_dma_err("%s on %s\n", mdb5_dma_strerror(errno), __func__);
		pthread_mutexattr_destroy(&mutex_attr);
		munmap(timer, sizeof(struct sync_timer));
		return NULL;
	}
	pthread_mutexattr_destroy(&mutex_attr);

	return timer;
}

static void sync_timer_wait_and_start(struct sync_timer *timer)
{
	struct timespec ts_now, ts_start, ts_cur;
	uint32_t timeout_seconds;

	__sync_fetch_and_add(&timer->ready_count, 1);

	timeout_seconds = 10 + timer->total_processes;

	clock_gettime(CLOCK_MONOTONIC, &ts_start);
	while (timer->ready_count < timer->total_processes) {
		clock_gettime(CLOCK_MONOTONIC, &ts_cur);
		if ((ts_cur.tv_sec - ts_start.tv_sec) >= timeout_seconds) {
			mdb5_dma_err("Timeout (%us) waiting for processes to sync: %u/%u ready\n",
				     timeout_seconds, timer->ready_count, timer->total_processes);
			exit(EXIT_FAILURE);
		}
		usleep(100);
	}

	pthread_mutex_lock(&timer->mutex);
	if (timer->start_time.tv_sec == 0 && timer->start_time.tv_nsec == 0) {
		clock_gettime(CLOCK_MONOTONIC, &ts_now);
		timer->start_time = ts_now;
		printf("dma transfer initiated\n");
	}
	pthread_mutex_unlock(&timer->mutex);
}

static inline bool sync_timer_check_elapsed(struct sync_timer *timer, uint32_t tsecs)
{
	struct timespec ts_cur, ts_start;

	if (tsecs == 0) {
		return false;
	}

	pthread_mutex_lock(&timer->mutex);
	ts_start = timer->start_time;
	pthread_mutex_unlock(&timer->mutex);

	if (clock_gettime(CLOCK_MONOTONIC, &ts_cur) != 0) {
		return true;
	}

	timespec_sub(&ts_cur, &ts_start);

	return (ts_cur.tv_sec >= tsecs);
}

static inline void sync_timer_destroy(struct sync_timer *timer)
{
	if (timer) {
		pthread_mutex_destroy(&timer->mutex);
		munmap(timer, sizeof(struct sync_timer));
	}
}

static void dump_thrd_info(struct io_info *_info)
{
	/* Config file contents dump */
	printf("Channel=%s\n", _info->ch_name);
	printf("Channel mode=%s\n", get_chan_mode(_info->chan_mode));
	printf("Channel dir=%s\n", get_chan_dir(_info->dir));
	printf("PF=0x%x\n", _info->pf);
	printf("fd=%d\n", _info->fd);
	printf("Channel ctrl=%u\n", _info->ch_ctrl);
	printf("Channel Started=%s\n", _info->ch_started ? "true" : "false");
	printf("Offset=0x%lx\n", _info->offset);
#if THREADS_SET_CPU_AFFINITY
	printf("cpu=%u\n", _info->cpu);
#endif
}

static void prep_pci_dump(struct mdb5_dma_perf *p)
{
	memset(p->pci_dump, '\0', PCI_DUMP_CMD_LEN);
	snprintf(p->pci_dump, PCI_DUMP_CMD_LEN, "lspci -s %02x:%02x.%01x -vvv",
		 p->pci_bus, p->pci_dev, p->pf_start);
}

static int setup_thrd_env(struct io_info *_info, uint8_t is_new_fd)
{
	if (is_new_fd) {
		_info->fd = open(_info->ch_name, O_RDWR);
		if (_info->fd < 0) {
			mdb5_dma_err("Cannot find %s: %s\n", _info->ch_name,
				     mdb5_dma_strerror(errno));
			return _info->fd;  /* Return error instead of exit */
		}
	}

	_info->ch_started++;

	return _info->fd;
}

static void clear_events(struct io_info *_info, struct list_head *node)
{
	struct io_event *events = NULL;
	int num_events = 0;
	uint32_t j, bufcnt;
	struct timespec ts_cur = { 1, 0 };

	if (node->max_events <= node->completed_events)
		return;
#ifdef DEBUG
	mdb5_dma_dbg("Thrd%u: Need to clear %u/%u events in node %p\n",
		 _info->thread_id,
		 node->max_events - node->completed_events,
		 node->max_events, node);
#endif
	events =
	    calloc(node->max_events - node->completed_events,
		   sizeof(struct io_event));
	if (!events) {
		mdb5_dma_err("%s: OOM: %s\n", __func__, mdb5_dma_strerror(errno));
		return;
	}
	do {
		num_events = io_getevents(node->ctxt, 1,
					  node->max_events -
					  node->completed_events, events,
					  &ts_cur);
		for (j = 0; (num_events > 0) && (j < num_events); j++) {
			struct iocb *iocb = (struct iocb *)events[j].obj;
			struct iovec *iov;

			node->completed_events++;

			if (!iocb) {
				mdb5_dma_err("%s: Invalid IOCB from events\n", __func__);
				continue;
			}
			iov = (struct iovec *)iocb->u.c.buf;

			for (bufcnt = 0; bufcnt < iocb->u.c.nbytes; bufcnt++)
				memory_pool_free_block(&datahandle, iov[bufcnt].iov_base);

			memory_pool_free_block(&iocbhandle, iocb);
		}
	} while ((num_events > 0) &&
		 (node->max_events > node->completed_events));

	free(events);
}

static void create_thread_info(struct mdb5_dma_perf *p)
{
	struct mdb5_dma_common *h = p->h;
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	struct io_info *_info = NULL;
	uint32_t num_rd_chan = h->num_rd_chan;
	uint32_t num_wr_chan = h->num_wr_chan;
	uint32_t num_chan = num_rd_chan + num_wr_chan;
	uint32_t ch_ctrl = 1, wr_cnt = 0, rd_cnt = 0;
	uint32_t base = 0;
	uint32_t i, j, k;
	int last_fd = -1;
#if THREADS_SET_CPU_AFFINITY
	int h2c_cpu = 0;
	int max_h2c_cpu = (num_processors / 2);
	int c2h_cpu = max_h2c_cpu;
	int max_c2h_cpu = (num_processors / 2) + ((num_processors % 2) ? 1 : 0);
#endif
	uint8_t is_new_fd = 1;
	enum mdb5_dma_chan_dir dir;

	p->num_thrds = p->num_pf * num_chan * p->num_thrds_per_chan;

	p->shmid = shmget(IPC_PRIVATE, p->num_thrds * sizeof(struct io_info),
			  IPC_CREAT | 0666);
	if (p->shmid < 0) {
		mdb5_dma_err("shmget failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	p->ch_lst_stop_mid = shmget(IPC_PRIVATE, num_chan * p->num_pf,
				    IPC_CREAT | 0666);
	if (p->ch_lst_stop_mid < 0) {
		mdb5_dma_err("shmget failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	p->ch_lst_stop = (uint8_t *)shmat(p->ch_lst_stop_mid, NULL, 0);
	if (p->ch_lst_stop == (uint8_t *)-1) {
		mdb5_dma_err("shmat failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	memset(p->ch_lst_stop, 0, num_chan);
	if (shmdt(p->ch_lst_stop) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	_info = (struct io_info *)shmat(p->shmid, NULL, 0);
	if (_info == (struct io_info *)-1) {
		mdb5_dma_err("shmat failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	p->io_exit_id = shmget(IPC_PRIVATE, sizeof(uint32_t), IPC_CREAT | 0666);
	if (p->io_exit_id < 0) {
		mdb5_dma_err("shmget failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	p->io_exit = (uint32_t *)shmat(p->io_exit_id, NULL, 0);
	if (p->io_exit == (uint32_t *)-1) {
		mdb5_dma_err("shmat failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	p->timer = sync_timer_init(p->num_thrds);
	if (!p->timer) {
		exit(-1);
	}

	if (CURRENT_LOG_LEVEL == LOG_LEVEL_DEBUG)
		prep_pci_dump(p);

	uint64_t write_offset = h->address;
	uint64_t read_offset = h->address + (num_wr_chan * h->pkt_sz * h->num_pkts * p->num_thrds_per_chan);

	for (k = 0; k < p->num_pf; k++) {
		for (i = 0; i < num_chan; i++) {
			ch_ctrl = 1;
			dir = 0;

			if (wr_cnt < num_wr_chan)
				dir = MDB5_CHAN_DIR_TO_DEV;
			else if (rd_cnt < num_rd_chan)
				dir = MDB5_CHAN_DIR_FROM_DEV;

#if THREADS_SET_CPU_AFFINITY
			if (h2c_cpu >= max_h2c_cpu)
				h2c_cpu = 0;
			if (c2h_cpu >= (max_h2c_cpu + max_h2c_cpu))
				c2h_cpu = max_h2c_cpu;
#endif
			for (j = 0; j < p->num_thrds_per_chan; j++) {
				is_new_fd = 1;
				if (dir == MDB5_CHAN_DIR_TO_DEV) {
					_info[base].ch_id = p->ch_start + i;
					prepare_node_name(_info[base].ch_name, "write",
							  _info[base].ch_id);
					if (!is_file_available(_info[base].ch_name)) {
						mdb5_dma_err("Invalid channel index: %d\n", _info[base].ch_id);
						exit(1);
					}
					_info[base].dir = dir;
					_info[base].chan_mode = h->chan_mode;
					_info[base].pf =
					    (p->pci_bus << 12) | (p->pci_dev << 4) |
					    (p->pf_start + k);
					_info[base].ch_ctrl = ch_ctrl;
					_info[base].fd = last_fd;
					_info[base].pkt_burst = h->num_pkts;
					_info[base].pkt_sz = h->pkt_sz;

					if (_info[base].chan_mode == MDB5_CHAN_MODE_MM &&
					    hioc->caperture.aperture > 0) {
						_info[base].aperture_sz =
						hioc->caperture.aperture;
					}
					_info[base].offset = write_offset;
					write_offset += h->pkt_sz * h->num_pkts;
#if THREADS_SET_CPU_AFFINITY
					_info[base].cpu = h2c_cpu;
#endif
					sem_init(&_info[base].llock, 0, 1);
					if (ch_ctrl != 0) {
						last_fd =
						    setup_thrd_env(&_info[base],
								   is_new_fd);
						if (last_fd < 0) {
							mdb5_dma_err("Failed to setup thread environment for channel %d\n", _info[base].ch_id);
							goto thread_cleanup;
						}
					}
					_info[base].thread_id = base;
					dump_thrd_info(&_info[base]);
					_info[base].exit_check_count = 0;
					base++;
					is_new_fd = 0;
				} else if (dir == MDB5_CHAN_DIR_FROM_DEV) {
					_info[base].ch_id = p->ch_start + i;
					prepare_node_name(_info[base].ch_name,"read",
							  _info[base].ch_id - num_wr_chan);
					if (!is_file_available(_info[base].ch_name)) {
						mdb5_dma_err("Invalid channel index: %d\n", _info[base].ch_id);
						exit(1);
					}
					_info[base].exit_check_count = 0;
					_info[base].dir = dir;
					_info[base].chan_mode = h->chan_mode;
					_info[base].pf =
					    (p->pci_bus << 12) | (p->pci_dev << 4) |
					    (p->pf_start + k);
					_info[base].ch_ctrl = ch_ctrl;
					_info[base].pkt_burst = h->num_pkts;
					_info[base].offset = read_offset;
					read_offset += h->pkt_sz * h->num_pkts;

					_info[base].pkt_sz = h->pkt_sz;
#if THREADS_SET_CPU_AFFINITY
					_info[base].cpu = c2h_cpu;
#endif
					sem_init(&_info[base].llock, 0, 1);
					_info[base].fd = last_fd;
					if (ch_ctrl != 0) {
						last_fd =
						    setup_thrd_env(&_info[base],
								   is_new_fd);
						if (last_fd < 0) {
							mdb5_dma_err("Failed to setup thread environment for channel %d\n", _info[base].ch_id);
							goto thread_cleanup;
						}
					}
					_info[base].thread_id = base;
					dump_thrd_info(&_info[base]);
					base++;
				}
				ch_ctrl = 0;
			}
#if THREADS_SET_CPU_AFFINITY
			h2c_cpu++;
			c2h_cpu++;
#endif
			if (dir == MDB5_CHAN_DIR_TO_DEV)
				wr_cnt++;
			else if (dir == MDB5_CHAN_DIR_FROM_DEV)
				rd_cnt++;
		}
	}
	if (shmdt(_info) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	return;

thread_cleanup:
	/* Clean up any opened file descriptors */
	for (uint32_t cleanup_idx = 0; cleanup_idx < base; cleanup_idx++) {
		if (_info[cleanup_idx].fd > 0) {
			close(_info[cleanup_idx].fd);
			_info[cleanup_idx].fd = -1;
		}
	}
	if (shmdt(_info) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	exit(EXIT_FAILURE);
}

#define MAX_AIO_EVENTS 65536

static void list_add_tail(struct io_info *_info, struct list_head *node)
{
	sem_wait(&_info->llock);
	node->next = NULL;
	if (!_info->head) {
		_info->head = node;
		_info->tail = node;
	} else {
		_info->tail->next = node;
		_info->tail = node;
	}
#ifdef DEBUG
	_info->total_nodes++;
#endif
	sem_post(&_info->llock);
}

static void list_add_head(struct io_info *_info, struct list_head *node)
{
	sem_wait(&_info->llock);
	node->next = _info->head;
	if (!_info->head)
		_info->tail = node;
	_info->head = node;
	sem_post(&_info->llock);
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
	mdb5_dma_dbg("Need to free %lu nodes in thrd%u\n",
		 _info->total_nodes - _info->freed_nodes, _info->thread_id);
#endif
	node = _info->head;

	while (node) {
		struct list_head *prev_node = NULL;

		// Wait for all submitted IOs to complete before cleaning up
		// This prevents leaking buffers that are still in flight
		while (node->max_events > node->completed_events) {
			clear_events(_info, node);
			if (node->max_events > node->completed_events) {
				usleep(1000);  // Sleep 1ms and retry
			}
		}

		clear_events(_info, node);
		io_destroy(node->ctxt);
		prev_node = node;
		node = node->next;
		memory_pool_free_block(&ctxhandle, prev_node);
#ifdef DEBUG
		_info->freed_nodes++;
#endif
	}
	sem_post(&_info->llock);
}

// this function returns 1 as long as long as traffic can run,
// returns 0 when it is safe to exit the thread
static int thread_exit_check(struct io_info *_info)
{
	struct mdb5_dma_perf *p = _info->p;

	if ((0 == *p->io_exit) && (0 == p->force_exit))
		return 1;

	if ((_info->num_req_submitted != _info->num_req_completed) &&
	    _info->exit_check_count < 10000) {
		if (_info->exit_check_count == 0) {
			_info->num_req_completed_in_time =
			    _info->num_req_completed;
		}
		_info->exit_check_count++;
		usleep(100);
		return 1;
	} else {
		mdb5_dma_info("Exit Check: tid =%u, req_sbmitted=%u, "
			  "req_completed=%u, dir=%s, intime=%u, "
			  "loop_count=%d\n",
			  _info->thread_id, _info->num_req_submitted,
			  _info->num_req_completed,
			  _info->dir == MDB5_CHAN_DIR_TO_DEV ? "TO-DEVICE" :
					"FROM-DEVICE",
			  _info->num_req_completed_in_time,
			  _info->exit_check_count);
		if (_info->exit_check_count != 0) {
			_info->num_req_completed =
			    _info->num_req_completed_in_time;
		}
		return 0;
	}
}

static void *event_mon(void *argp)
{
	struct io_info *_info = (struct io_info *)argp;
	uint32_t j, bufcnt;
	struct io_event *events = NULL;
	int num_events = 0;
	struct timespec ts_cur = { 0, 0 };
#if DATA_VALIDATION
	uint32_t k;
	uint16_t *rcv_data;
#endif

	events = calloc(MAX_AIO_EVENTS, sizeof(struct io_event));
	if (!events) {
		mdb5_dma_err("%s: OOM: %s\n", __func__, mdb5_dma_strerror(errno));
		return NULL;
	}

	while (thread_exit_check(_info)) {
		struct list_head *node = list_pop(_info);

		if (!node)
			continue;

		memset(events, 0, MAX_AIO_EVENTS * sizeof(struct io_event));
		do {
			num_events = io_getevents(node->ctxt, 1,
						  node->max_events -
						  node->completed_events,
						  events, &ts_cur);
			for (j = 0; (num_events > 0) && (j < num_events); j++) {
				struct iocb *iocb =
				    (struct iocb *)events[j].obj;
				struct iovec *iov = NULL;

				if (!iocb) {
					mdb5_dma_err("%s: Invalid IOCB from "
						 "events\n", __func__);
					continue;
				}
				_info->num_req_completed += events[j].res;

				iov = (struct iovec *)(iocb->u.c.buf);
				if (!iov) {
					mdb5_dma_err("%s: Invalid buffer\n", __func__);
					continue;
				}
#if DATA_VALIDATION
				rcv_data = iov[0].iov_base;
				for (k = 0;
				     k < (iov[0].iov_len / 2) &&
				     events[j].res && !(events[j].res2);
				     k += 8) {
					mdb5_dma_dbg("%04x: %04x %04x %04x %04x "
						 "%04x %04x %04x %04x\n",
					     k, rcv_data[k], rcv_data[k + 1],
					     rcv_data[k + 2], rcv_data[k + 3],
					     rcv_data[k + 4], rcv_data[k + 5],
					     rcv_data[k + 6], rcv_data[k + 7]);
				}
#endif
				for (bufcnt = 0;
				     (bufcnt < iocb->u.c.nbytes) && iov;
				     bufcnt++)
					memory_pool_free_block(&datahandle,
						 iov[bufcnt].iov_base);
				memory_pool_free_block(&iocbhandle, iocb);
			}
			if (num_events > 0)
				node->completed_events += num_events;
			if (node->completed_events >= node->max_events) {
				io_destroy(node->ctxt);
				memory_pool_free_block(&ctxhandle, node);
				node = NULL;
				break;
			}
		} while (thread_exit_check(_info));
		if (node && node->completed_events < node->max_events)
			list_add_head(_info, node);
	}
	free(events);
#ifdef DEBUG
	mdb5_dma_dbg("Exiting evt_thrd: %u\n", _info->thread_id);
#endif

	return NULL;
}

static void io_proc_cleanup(struct io_info *_info, struct mdb5_dma_perf *p)
{
	struct mdb5_dma_common *h = p->h;
	/**TODO: For tracking _info from info structure**/
	struct io_info *info = p->info;
	uint32_t num_chan = h->num_rd_chan + h->num_wr_chan, i;
	uint32_t ch_lst_idx_base;
	uint8_t is_ch_stop = (_info->ch_ctrl && _info->ch_started);

	*p->io_exit = 1;
	pthread_join(_info->evt_id, NULL);
	ch_lst_idx_base = ((((_info->pf & 0x0000F) - p->pf_start) * num_chan)
			   /* TODO: + ch_offset */);

	_info->ch_wait_for_stop = 1;

	while (is_ch_stop) {
		uint32_t j = 0;

		for (i = 0; i < p->num_thrds; i++) {
			if ((info[i].pf != _info->pf) ||
			    (info[i].ch_id != _info->ch_id) ||
			    (info[i].dir != _info->dir))
				continue;
			if (info[i].ch_wait_for_stop)
				j++;
			else
				break;
		}
		if (j != p->num_thrds_per_chan)
			sched_yield();
		else
			break;
	}

	if (is_ch_stop) {
		_info->ch_started = 0;
		p->ch_lst_stop[ch_lst_idx_base + _info->ch_id - p->ch_start] = 1;
	}

	while (!(p->ch_lst_stop[ch_lst_idx_base + _info->ch_id - p->ch_start]))
		sched_yield();

	if (shmdt(p->ch_lst_stop) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	list_free(_info);

	memory_pool_free(&iocbhandle);
	memory_pool_free(&ctxhandle);
	memory_pool_free(&datahandle);
}

static void *io_thread(void *argp)
{
	pthread_attr_t attr;
	struct io_info *_info = (struct io_info *)argp;
	struct mdb5_dma_perf *p = _info->p;
	struct iocb *io_list[1];
	uint32_t burst_cnt = _info->pkt_burst;
	uint32_t io_sz = _info->pkt_sz;
	uint32_t max_io = MAX_AIO_EVENTS;
	uint32_t cnt = 0, num_desc = 0;
	uint32_t max_reqs = 256;
	uint32_t tsecs = p->tsecs;
	int ret, s;

	num_desc = (io_sz + DEFAULT_PAGE_SIZE - 1) >> PAGE_SHIFT;
	ret = memory_pool_create(&datahandle, num_desc * DEFAULT_PAGE_SIZE,
		       max_reqs + (burst_cnt * num_desc));
	if (ret < 0) {
		mdb5_dma_err("Failed to create data memory pool\n");
		return NULL;
	}

	ret = memory_pool_create(&ctxhandle, sizeof(struct list_head), max_reqs);
	if (ret < 0) {
		mdb5_dma_err("Failed to create context memory pool\n");
		memory_pool_free(&datahandle);
		return NULL;
	}
	ret = memory_pool_create(&iocbhandle,
		       sizeof(struct iocb) + (burst_cnt * sizeof(struct iovec)),
		       max_reqs + (burst_cnt * num_desc));
	if (ret < 0) {
		mdb5_dma_err("Failed to create iocb memory pool\n");
		memory_pool_free(&datahandle);
		memory_pool_free(&ctxhandle);
		return NULL;
	}
	s = pthread_attr_init(&attr);
	if (s != 0)
		mdb5_dma_err("pid=%d:%s: pthread_attr_init failed\n",
			 getpid(), __func__);
	if (pthread_create(&_info->evt_id, &attr, event_mon, _info)) {
		mdb5_dma_err("Failed to create event monitoring thread\n");
		memory_pool_free(&datahandle);
		memory_pool_free(&ctxhandle);
		memory_pool_free(&iocbhandle);
		return NULL;
	}

	sync_timer_wait_and_start(p->timer);

	do {
		struct list_head *node = NULL;

		if (sync_timer_check_elapsed(p->timer, tsecs))
			break;
		node = memory_pool_alloc_block(&ctxhandle);
		if (!node)
			continue;

		ret = io_queue_init(max_io, &node->ctxt);
		if (ret != 0) {
			mdb5_dma_err("io_setup error %d on %u\n", ret,
				 _info->thread_id);
			memory_pool_free_block(&ctxhandle, node);
			sched_yield();
			continue;
		}
		cnt = 0;
		node->max_events = max_io;
		list_add_tail(_info, node);
		do {
			struct iovec *iov = NULL;
			uint32_t iovcnt;

			if (sync_timer_check_elapsed(p->timer, tsecs)) {
				node->max_events = cnt;
				break;
			}

			if (((_info->num_req_submitted -
			      _info->num_req_completed) * num_desc) >
			    max_reqs) {
				sched_yield();
				continue;
			}
			mdb5_dma_dbg("%s: sub=%d, comp=%d, num_desc=%d, max_reqs=%d\n",
				 __func__, _info->num_req_submitted,
				 _info->num_req_completed, num_desc,
				 max_reqs);

			io_list[0] = memory_pool_alloc_block(&iocbhandle);
			if (!io_list[0]) {
				if (cnt) {
					node->max_events = cnt;
					break;
				} else {
					sched_yield();
					continue;
				}
			}
			iov = (struct iovec *)(io_list[0] + 1);
			for (iovcnt = 0; iovcnt < burst_cnt; iovcnt++) {
				iov[iovcnt].iov_base =
				    memory_pool_alloc_block(&datahandle);
				if (!iov[iovcnt].iov_base)
					break;
				iov[iovcnt].iov_len = io_sz;
#if DATA_VALIDATION
				mdb5_buffer_fill_pattern(iov[iovcnt].iov_base, io_sz, 0xDEADDEAD);
#endif
			}
			if (iovcnt == 0) {
				memory_pool_free_block(&iocbhandle, io_list[0]);
				continue;
			}
			if (_info->dir == MDB5_CHAN_DIR_TO_DEV) {
				io_prep_pwritev(io_list[0],
						_info->fd,
						iov, iovcnt, _info->offset);
			} else if (_info->dir == MDB5_CHAN_DIR_FROM_DEV) {
				io_prep_preadv(io_list[0],
					       _info->fd,
					       iov, iovcnt, _info->offset);
			}

			ret = io_submit(node->ctxt, 1, io_list);
			if (ret != 1) {
				mdb5_dma_err("pid=%d:%s:io_submit error:%d on "
					 "%s for %u\n",
					 getpid(), __func__, ret,
					 _info->ch_name, cnt);
				// Free the allocated buffers on error (fix: use correct index)
				while (iovcnt > 0) {
					iovcnt--;
					memory_pool_free_block(&datahandle, iov[iovcnt].iov_base);
				}
				memory_pool_free_block(&iocbhandle, io_list[0]);
				node->max_events = cnt;
				break;
			} else {
				cnt++;
				_info->num_req_submitted += iovcnt;
			}
		} while (tsecs && !p->force_exit && (cnt < max_io));
	} while (tsecs && !p->force_exit);

	io_proc_cleanup(_info, p);

	return NULL;
}

static void dump_result(uint64_t total_io_sz, uint32_t tsecs)
{
	uint64_t gig_div = ((uint64_t)tsecs * 1000000000);
	uint64_t meg_div = ((uint64_t)tsecs * 1000000);
	uint64_t kil_div = ((uint64_t)tsecs * 1000);
	uint64_t byt_div = ((uint64_t)tsecs);

	if ((total_io_sz / gig_div))
		mdb5_dma_info("BW = %f GB/sec\n", ((double)total_io_sz / gig_div));
	else if ((total_io_sz / meg_div))
		mdb5_dma_info("BW = %f MB/sec\n", ((double)total_io_sz / meg_div));
	else if ((total_io_sz / kil_div))
		mdb5_dma_info("BW = %f KB/sec\n", ((double)total_io_sz / kil_div));
	else
		mdb5_dma_info("BW = %f Bytes/sec\n",
			  ((double)total_io_sz / byt_div));
}

static int is_valid_fd(int fd)
{
	return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

/**TODO: Only one***/
static void cleanup(int status, void *argp)
{
	struct mdb5_dma_perf *p = (struct mdb5_dma_perf *)argp;
	struct mdb5_dma_common *h = p->h;
	struct io_info *info = p->info;
	uint64_t total_num_h2c_ios = 0;
	uint64_t total_num_c2h_ios = 0;
	uint64_t total_io_sz = 0;
	int i, s;
	bool dump_en = h->dump_en;

	if ((*p->io_exit == 0)) {
		mdb5_dma_info("force exit: cleaning up\n");
		p->force_exit = 1;
		if (getpid() != p->base_pid) {
			if (shmdt(info) == -1) {
				mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
				exit(EXIT_FAILURE);
			}
			if ((shmdt(p->ch_lst_stop) == -1)) {
				mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
				exit(EXIT_FAILURE);
			}
		} else {
			for (i = 1; i < p->num_thrds; i++)
				wait(NULL);
		}
	}

	if (getpid() != p->base_pid)
		return;

	if (p->child_pid_lst)
		free(p->child_pid_lst);

	sync_timer_destroy(p->timer);

	if (shmctl(p->ch_lst_stop_mid, IPC_RMID, NULL) == -1) {
		mdb5_dma_err("shmctl failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!info)
		return;

	if (dump_en) {
		s = system(p->pci_dump);
		if (s != 0)
			mdb5_dma_err("%s\nerrcode = %d\n", p->pci_dump, s);
	}

	for (i = 0; i < p->num_thrds; i++) {
		if (info[i].ch_ctrl && info[i].ch_started)
			info[i].ch_started = 0;
		if ((info[i].ch_ctrl != 0) && (info[i].fd > 0) &&
		    is_valid_fd(info[i].fd))
			close(info[i].fd);
	}

	/* accumulate the statistics */
	for (i = 0; i < p->num_thrds; i++) {
		if (info[i].dir == MDB5_CHAN_DIR_TO_DEV)
			total_num_h2c_ios += info[i].num_req_completed;
		else
			total_num_c2h_ios += info[i].num_req_completed;
	}
	if (shmdt(info) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (shmctl(p->shmid, IPC_RMID, NULL) == -1) {
		mdb5_dma_err("shmctl failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (shmdt(p->io_exit) == -1) {
		mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (shmctl(p->io_exit_id, IPC_RMID, NULL) == -1) {
		mdb5_dma_err("shmctl failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (p->tsecs == 0)
		p->tsecs = 1;
	if (0 != total_num_h2c_ios) {
		total_io_sz = (total_num_h2c_ios * h->pkt_sz);
		mdb5_dma_info("WRITE: total pps = %lu",
			  total_num_h2c_ios / p->tsecs);
		printf(" ");
		dump_result(total_io_sz, p->tsecs);
	}
	if (0 != total_num_c2h_ios) {
		total_io_sz = (total_num_c2h_ios * h->pkt_sz);
		mdb5_dma_info("READ: total pps = %lu",
			  total_num_c2h_ios / p->tsecs);
		printf(" ");
		dump_result(total_io_sz, p->tsecs);
	}

	if ((0 == total_num_h2c_ios) && (0 == total_num_c2h_ios)) {
		mdb5_dma_err("No IOs happened\n");
	}
}

int main(int argc, char *argv[])
{
	struct io_info *info = NULL;
	uint32_t i, max_chan = MDB5_MAX_RD_CHAN + MDB5_MAX_WR_CHAN;
	uint32_t aio_max_nr = 0xFFFFFFFF;
	int ret = 0;
	char *cfg_fname = NULL;
	char aio_max_nr_cmd[100] = { '\0' };
	struct mdb5_dma_channel chan[max_chan];
#if THREADS_SET_CPU_AFFINITY
	cpu_set_t set;
	uint32_t j;
#endif
	pid_t cpid;

	memset(&p, 0, sizeof(struct mdb5_dma_perf));
	memset(&h, 0, sizeof(struct mdb5_dma_common));
	memset(chan, 0, max_chan * sizeof(struct mdb5_dma_channel));

	h.chan = chan;
	p.h = &h;
	p.pf_start = 0;
	p.num_pf = 1;

	ret = parse_options(argc, argv, &cfg_fname);
	if (ret < 0)
		return ret;

#if THREADS_SET_CPU_AFFINITY
	num_processors = get_nprocs_conf();
	CPU_ZERO(&set);
#endif
#if DATA_VALIDATION
	for (i = 0; i < 2 * 1024; i++)
		p.valid_data[i] = i;
#endif
	ret = parse_config_file(cfg_fname, &p);
	if (ret < 0) {
		mdb5_dma_err("Config File:%s has invalid parameters\n", cfg_fname);
		goto cleanup_cfg;
	}

	ret = validate_input_params(&p);
	if (ret < 0) {
		mdb5_dma_err("Invalid Input parameters in Config File:%s\n",
			 cfg_fname);
		goto cleanup_cfg;
	}

	create_thread_info(&p);
//	atexit(cleanup);
	/**TODO: struct info variable not defined as of now not correct**/
	/**TODO: For detach of shm, called on every exit**/
	on_exit(cleanup, &p);

	snprintf(aio_max_nr_cmd, 100, "echo %u > /proc/sys/fs/aio-max-nr",
		 aio_max_nr);
	system(aio_max_nr_cmd);

	mdb5_dma_info("dmaperf(%u) processes\n", p.num_thrds);
	p.child_pid_lst = calloc(p.num_thrds, sizeof(int));
	p.base_pid = getpid();
	p.child_pid_lst[0] = p.base_pid;

	for (i = 1; i < p.num_thrds; i++) {
		if (p.base_pid == getpid()) {
			cpid = fork();
			if (!cpid) {
				break;
			}
			p.child_pid_lst[i] = cpid;
		}
	}

	/*TODO: what if we attach this before fork?*/
	info = (struct io_info *)shmat(p.shmid, NULL, 0);
	if (info == (struct io_info *)-1) {
		mdb5_dma_err("shmat failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}
	p.info = info;

	p.ch_lst_stop = (uint8_t *)shmat(p.ch_lst_stop_mid, NULL, 0);
	if (p.ch_lst_stop == (uint8_t *)-1) {
		mdb5_dma_err("shmat failed: %s\n", mdb5_dma_strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (getpid() == p.base_pid) {
		info[0].p = &p;
		io_thread(&info[0]);
#if THREADS_SET_CPU_AFFINITY
		for (j = 0; j < num_processors; j++) {
			if (j != info[0].cpu)
				CPU_SET(j, &set);
		}
		if (sched_setaffinity(p.base_pid, sizeof(set), &set) == -1)
			mdb5_dma_err("pid=%d: setaffinity for thrd%u failed\n",
				 p.base_pid, info[i].thread_id);
#endif
		for (i = 1; i < p.num_thrds; i++)
			waitpid(p.child_pid_lst[i], NULL, 0);
		free(p.child_pid_lst);
		p.child_pid_lst = NULL;
	} else {
		info[i].pid = getpid();
#if THREADS_SET_CPU_AFFINITY
		for (j = 0; j < num_processors; j++) {
			if (j != info[i].cpu)
				CPU_SET(j, &set);
		}
		if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
			mdb5_dma_err("pid=%d: setaffinity for thrd%u failed\n",
				 info[i].pid, info[i].thread_id);
#endif
		info[i].p = &p;
		io_thread(&info[i]);
		if ((shmdt(info) == -1)) {
			mdb5_dma_err("shmdt failed: %s\n", mdb5_dma_strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	*p.io_exit = 1;
	goto cleanup_cfg;

cleanup_cfg:
	if (cfg_fname) {
		free(cfg_fname);
		cfg_fname = NULL;
	}
	return ret;
}
