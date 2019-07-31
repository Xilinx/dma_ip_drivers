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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include "aioutility.h"

#include <libaio.h>

#define MSEC2NSEC 1000000

static void aio_completion_handler(io_context_t io_ctx, struct iocb *iocb, long res, long res2)
{
	struct aio_block *aioblock;
	struct aio_job *aiojob;
	struct io_jobs *iojob;

	aioblock = get_aio_block(iocb);
	if (NULL == aioblock) return;
	aiojob = aioblock->parent;
	iojob = aiojob->iojob;

	/* Did the request complete? */
	aiojob->mask_completed |= aioblock->mask;
	if (0 == res2) {
		ssize_t rc;
		struct timespec ts_end;
		long total_time = 0;
		float result;
		float avg_time = 0;

		if (0 <= res) {
			printf("%s iteration %d block %d completed successfully\n",
			   iojob->job_name, aioblock->jobitr, aioblock->block_num);
			if (aiojob->mask_expected == aiojob->mask_completed) {
				rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
				timespec_sub(&ts_end, &(aiojob->ts_start));
				total_time += ts_end.tv_nsec;
				result = ((float)(iojob->numbytes * iojob->numitr))*1000/total_time;
				printf("** %s Job, total time %ld nsec, for bytes = %u, BW = %fMB/s \n",
						iojob->job_name, total_time,
						(iojob->numbytes * iojob->numitr),
						result);
				close(aiojob->fd);
				verify_read(iojob);
			} else
				printf("Expected mask = %llx vs complete mask = %llx\n", aiojob->mask_expected, aiojob->mask_completed);
		} else
			printf("Error: %s job itr %d block %d completed with error\n",
			       iojob->job_name, aioblock->jobitr, aioblock->block_num);
	} else
		printf("Error: %s job itr %d block %d failed with error %ld\n",
		       iojob->job_name, aioblock->jobitr,
		       aioblock->block_num, res2);

	sem_post(&aioblock->lock); /* ready for destruction */
}

static bool create_naio_job(struct iocb **io_list, struct io_jobs *iojob)
{
	int fd;
	unsigned int i;
	struct aio_block *aioblock; /* one per block */
	struct aio_job *aiojob; /* one per job */
	ssize_t rc;
	unsigned int blk_idx = 0;
	unsigned int blk_num;

	fd = open(iojob->node, O_RDWR);
	if (0 > fd) {
		printf("Error: Opening %s node\n", iojob->node);
		return false;
	}

	aiojob = calloc(1, sizeof(struct aio_job));

	iojob->aiojob = aiojob;
	blk_num = get_io_job_num_blocks(iojob);
	aiojob->mask_completed = 0;
	aiojob->mask_expected = 0;
	aiojob->iojob = iojob;

	aiojob->aio_list = (void *)io_list;

	for (i = 0; i < iojob->numitr; i++) {
		unsigned int j;

		for (j = 0; j < blk_num; j++) {
			struct iocb *aio_entry;
			unsigned int buf_off = (i * blk_num * iojob->block_size) + (j * iojob->block_size);
			unsigned int io_numbytes = iojob->block_size;

			if (j == (blk_num - 1) &&
					(iojob->numbytes % iojob->block_size))
				io_numbytes = (iojob->numbytes % iojob->block_size);

			aioblock = calloc(1, sizeof(struct aio_block));
			aioblock->parent = aiojob;
			sem_init(&aioblock->lock, 0, 1); /* to track completion */
			aioblock->jobitr = i;
			aioblock->block_num = j;
			io_list[blk_idx] = calloc(1, sizeof(struct iocb));
			aio_entry = io_list[blk_idx];
			aioblock->aio_entry =  io_list[blk_idx];

			if (0 == strncmp(iojob->iotype, "read", 5)) {
				io_prep_pread(aio_entry,
				              fd,
				              iojob->buf + buf_off,
				              io_numbytes,
				              0);
			} else if (0 == strncmp(iojob->iotype, "write", 4)) {
				io_prep_pwrite(aio_entry,
				              fd,
					      iojob->buf + buf_off,
					      io_numbytes,
					      0);
			} else {
				printf("Error: Invalid IO operation specified for %s IO job\n",
				       iojob->job_name);
				goto ret_false;
			}

			io_set_callback(aio_entry, aio_completion_handler);

			aioblock->mask = (((__u64)1) << blk_idx);
			aiojob->mask_expected |= aioblock->mask;
			sem_wait(&aioblock->lock);
			enqueue_aio_block(aiojob, aioblock);
			blk_idx++;
		}
		/* since all IOs are submitted at once, start time shall almost be same */
		rc = clock_gettime(CLOCK_MONOTONIC, &(aiojob->ts_start));
	}
	enqueue_aio_task(aiojob);
	return true;
ret_false:
	if (aiojob) {
		aiojob->mask_completed = aiojob->mask_expected;
		cleanup_aiojob(aiojob, false);
	}

	return false;
}

void submit_naio_job(void *_iojob)
{
	struct io_jobs *iojob = (struct io_jobs *)_iojob;
	struct iocb **io_list;
	unsigned int numb_blks = get_io_job_num_blocks(iojob);
	unsigned int total_jobs = (iojob->numitr * numb_blks);
	struct io_event *events;
	io_context_t io_ctxt;
	int ret;
	struct timespec ts_cur;
	int i;

	/* setup IO job env */
	run_env_cmds(iojob->setup_tail);

	io_list = (struct iocb **)calloc(total_jobs,
	                                   sizeof(struct iocb *));

	events = calloc(total_jobs, sizeof(struct io_event));

	if (false == create_naio_job(io_list, iojob)) {
		printf("Error: Failed to create IO job %s\n", iojob->job_name);
		run_env_cmds(iojob->cleanup_tail);
		return;
	}

	memset(&io_ctxt, 0, sizeof(io_ctxt));
	if(io_queue_init(total_jobs, &io_ctxt)!=0){//init
	    printf("Error: io_setup error\n");
	    return;
	}
	ret = io_submit(io_ctxt, total_jobs, io_list);
	if(ret != total_jobs) {
	    printf("Error: io_submit error:%d\n", ret);
	    return;
	}

	ret = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
	for (i = 0; i < total_jobs; i++) {
		if (ts_cur.tv_nsec > (ts_cur.tv_nsec + (50 * MSEC2NSEC))) {
			ts_cur.tv_sec += 1;
		}
		ts_cur.tv_nsec += (50 * MSEC2NSEC);  /*50 msec/io*/
	}
	if (io_getevents(io_ctxt,
			   total_jobs,
			   total_jobs,
			   events,
			   &ts_cur) == total_jobs) {
		for (i = 0; i < total_jobs; i++) {
			((io_callback_t)(events[i].data))(io_ctxt,
					events[i].obj,
					events[i].res,
					events[i].res2);
		}
	} else {
		printf("AIO completion not received\n");
	}
	io_destroy(io_ctxt);
	cleanup_aio_jobs(true);
}

void cancel_naio_job(void *_aiojob)
{
	struct aio_job *aiojob = (struct aio_job *)_aiojob;
	int rc;
	struct iocb *iocb;
	struct aio_block *aioblock;

	aioblock = aiojob->aio_block_head;
	while (1) {
		if (NULL == aioblock) break;
		if (aioblock->mask != (aiojob->mask_expected &
				aioblock->mask)) {
			iocb = (struct iocb *)aioblock->aio_entry;

			rc = io_cancel((io_context_t)aiojob->naio_ctxt,
			               iocb,
			               (struct io_event *)(aiojob->events));
			if (0 != rc)
				printf("Warning: %s job block %d itr %d not cancelled. Will wait until cancelled\n",
				       aiojob->iojob->job_name,
				       aioblock->block_num, aioblock->jobitr);
			sem_post(&aioblock->lock); /* ready for destruction */
		}
		aioblock = aioblock->next;
	}
}

