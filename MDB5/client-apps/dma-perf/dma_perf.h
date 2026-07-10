/*
 * This file is part of the MDB5 userspace application
 * to enable the user to execute the MDB5 functionality
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */

#ifndef _DMA_PERF_H_
#define _DMA_PERF_H_

#include <unistd.h>
#include <semaphore.h>
#include <libaio.h>

#define SEC2NSEC           1000000000
#define SEC2USEC           1000000
#define DEFAULT_PAGE_SIZE  4096
#define PAGE_SHIFT         12

#define DATA_VALIDATION 0
#define MSEC2NSEC 1000000
#define CMPL_STATUS_ACC_CMD_LEN 200
#define PCI_DUMP_CMD_LEN 100
#define THREADS_SET_CPU_AFFINITY 0
#define DEBUG


struct sync_timer {
	pthread_mutex_t mutex;
	struct timespec start_time;
	uint32_t ready_count;
	uint32_t total_processes;
};

struct list_head {
	struct list_head *next;
	uint32_t max_events;
	uint32_t completed_events;
	io_context_t ctxt;
};

struct mdb5_dma_perf;

struct io_info {
	struct mdb5_dma_perf *p;
	pthread_t evt_id;
	sem_t llock;
	struct list_head *head;
	struct list_head *tail;
	uint64_t offset;
	uint64_t pkt_sz;
#ifdef DEBUG
	uint64_t total_nodes;
	uint64_t freed_nodes;
#endif
	uint32_t pkt_burst;
	uint32_t aperture_sz;
	uint32_t pf;
	uint32_t ch_started;
	uint32_t ch_wait_for_stop;
	uint32_t ch_id;
	uint32_t thread_id;
	uint32_t num_req_submitted;
	uint32_t num_req_completed;
	uint32_t num_req_completed_in_time;
	uint32_t pfetch_en;
	uint32_t idx_tmr;
	uint32_t idx_cnt;
	int pid;
	int fd;
#if THREADS_SET_CPU_AFFINITY
	int cpu;
#endif
	int exit_check_count;
	uint8_t ch_ctrl;
	char ch_name[20];
	enum mdb5_dma_chan_mode chan_mode;
	enum mdb5_dma_chan_dir dir;
};

struct mdb5_dma_perf {
	struct sync_timer* timer;
	struct mdb5_dma_common *h;
	struct io_info *info;
	uint32_t ch_start;
	uint32_t idx_tmr;
	uint32_t idx_cnt;
	uint32_t pfetch_en;
	uint32_t no_memcpy;
	uint32_t tsecs;
	uint32_t force_exit;
	uint32_t num_thrds;
	uint32_t num_thrds_per_chan;
	uint32_t marker_en;
#if THREADS_SET_CPU_AFFINITY
	uint32_t num_processors;
#endif
	uint32_t *io_exit;
	int shmid;
	int ch_lst_stop_mid;
	int io_exit_id;
	int base_pid;
	int input_file_provided;
	int output_file_provided;
	int *child_pid_lst;
#if DATA_VALIDATION
	uint16_t valid_data[2 * 1024];
#endif
	uint8_t pci_bus;
	uint8_t pci_dev;
	uint8_t pf_start;
	uint8_t num_pf;
	uint8_t *ch_lst_stop;
	char trigmode[10];
	char pci_dump[PCI_DUMP_CMD_LEN];
};

#endif /* _DMA_PERF_H_ */
