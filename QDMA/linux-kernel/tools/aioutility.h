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

#ifndef __AIOUTILITY_H__
#define __AIOUTILITY_H__

#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <linux/types.h>

enum read_verify_type {
	READ_VERIFY_NONE,
	READ_VERIFY_FILE,
	READ_VERIFY_PATTERN
};

enum io_engine {
	IO_ENGINE_LIBAIO,
	IO_ENGINE_POSIXAIO
};

struct env_cmds {
	char *cmd; /* commands to be run  */
	struct env_cmds *next;
	struct env_cmds *prev;
};

typedef void (*submit_aio_job)(void *);
typedef void (*cancel_aio_job)(void *);

struct io_jobs {
	char *job_name;
	char *node;
	char *iotype;
	char *buf;
	char *verify_file_name;
	char *verify_pattern;
	unsigned int numbytes;
	unsigned int block_size;
	unsigned int numitr;
	enum read_verify_type verify_read;
	enum io_engine io_type;
	void *aiojob;
	struct env_cmds *setup_head;
	struct env_cmds *setup_tail;
	struct env_cmds *cleanup_head;
	struct env_cmds *cleanup_tail;
	submit_aio_job commit_io;
	cancel_aio_job cancel_io;
	struct io_jobs *next;
	struct io_jobs *prev;
};

struct aio_block {
	sem_t lock;
	__u64 mask;
	void *aio_entry;
	unsigned int jobitr;
	unsigned int num_bytes;
	unsigned int block_num;
	unsigned int num_blocks_completed;
	struct aio_job *parent;
	void *next;
};

struct aio_job {
	struct io_jobs *iojob;
	int fd;
	__u64 mask_expected;
	__u64 mask_completed;
	struct timespec ts_start;
	void *aio_list;
	void *naio_ctxt;
	void *events;
	struct aio_block *aio_block_head;
	struct aio_block *aio_block_tail;
	struct aio_job *next;
};

void submit_naio_job(void *iojob);
void submit_paio_job(void *iojob);
void cancel_naio_job(void *aiojob);
void cancel_paio_job(void *aiojob);

unsigned int get_io_job_num_blocks(struct io_jobs *iojob);
void verify_read(struct io_jobs *iojob);
bool cleanup_aiojob(struct aio_job *aiojob, bool force);
void cleanup_aio_jobs(bool);
void enqueue_io_job(struct io_jobs *iojob);
void enqueue_aio_block(struct aio_job *aiojob, struct aio_block *aioblock);
void enqueue_aio_task(struct aio_job *aiotask);
struct aio_block * get_aio_block(void *_aioblock);

void run_env_cmds(struct env_cmds *gcmd);

void timespec_sub(struct timespec *t1, struct timespec *t2);

#endif
