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

#define _XOPEN_SOURCE 600
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <aio.h>
#include "aioutility.h"


static void aio_completion_handler( sigval_t sigval )
{
	struct aiocb *req;
	struct io_jobs *iojob;
	struct aio_block *aioblock;
	struct aio_job *aiojob;
	ssize_t ret;
	int req_err;

	aioblock = (struct aio_block *)(sigval.sival_ptr);
	if (NULL == aioblock) return;
	req = (struct aiocb *)aioblock->aio_entry;
	if (NULL == aioblock->aio_entry) return;
	aiojob = aioblock->parent;
	iojob = aiojob->iojob;

	/* Did the request complete? */
	req_err = aio_error( req );
	aiojob->mask_completed |= aioblock->mask;
	if (0 == req_err) {
		ssize_t rc;
		struct timespec ts_end;
		long total_time = 0;
		float result;
		float avg_time = 0;

		ret = aio_return( req );

		if (0 <= ret) {
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
				verify_read(iojob);
			} else
				printf("Expected mask = %llx vs complete mask = %llx\n", aiojob->mask_expected, aiojob->mask_completed);
		} else
			printf("Error: %s job itr %d block %d completed with error\n",
			       iojob->job_name, aioblock->jobitr, aioblock->block_num);
	} else
		printf("Error: %s job itr %d block %d failed with error %d\n",
		       iojob->job_name, aioblock->jobitr,
		       aioblock->block_num, req_err);

	sem_post(&aioblock->lock); /* ready for destruction */
	cleanup_aio_jobs(false);
}

static bool create_paio_job(struct aiocb **aio_list, struct io_jobs *iojob)
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

	aiojob->aio_list = (void *)aio_list;

	for (i = 0; i < iojob->numitr; i++) {
		unsigned int j;

		for (j = 0; j < blk_num; j++) {
			struct aiocb *aio_entry;
            unsigned int buf_off = (i * blk_num * iojob->block_size) + (j * iojob->block_size);
            unsigned int io_numbytes = iojob->block_size;

			aioblock = calloc(1, sizeof(struct aio_block));
			aioblock->parent = aiojob;
			sem_init(&aioblock->lock, 0, 1); /* to track completion */
			aioblock->jobitr = i;
			aioblock->block_num = j;
			aio_list[blk_idx] = calloc(1, sizeof(struct aiocb));
			aio_entry = aio_list[blk_idx];

			aioblock->aio_entry = aio_list[blk_idx];
			/* Set up the AIO request */
			aio_entry->aio_fildes = fd;
			aio_entry->aio_buf = iojob->buf + buf_off;
			aio_entry->aio_nbytes = io_numbytes;

			/* Link the AIO request with a thread callback */
			aio_entry->aio_sigevent.sigev_notify = SIGEV_THREAD;
			aio_entry->aio_sigevent._sigev_un._sigev_thread._function = aio_completion_handler;
			aio_entry->aio_sigevent._sigev_un._sigev_thread._attribute = NULL;
			aio_entry->aio_sigevent.sigev_value.sival_ptr = aioblock;

			if (0 == strncmp(iojob->iotype, "write", 5)) {
				aio_entry->aio_lio_opcode = LIO_WRITE;
			} else if (0 == strncmp(iojob->iotype, "read", 4)) {
				aio_entry->aio_lio_opcode = LIO_READ;
			} else {
				printf("Error: Invalid IO operation specified for %s IO job\n",
				       iojob->job_name);
				goto ret_false;
			}
			sem_wait(&aioblock->lock);
			aioblock->mask = (((__u64)1) << blk_idx);
			aiojob->mask_expected |= aioblock->mask;
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

void submit_paio_job(void *_iojob)
{
	struct io_jobs *iojob = (struct io_jobs *)_iojob;
	struct aiocb **aio_list;
	unsigned int numb_blks = get_io_job_num_blocks(iojob);
	unsigned int total_jobs = (iojob->numitr * numb_blks);

	printf("setting up environment for %s\n", iojob->job_name);
	/* setup IO job env */
	run_env_cmds(iojob->setup_tail);

	aio_list = (struct aiocb **)calloc(total_jobs,
	                                   sizeof(struct aiocb *));
	if (false == create_paio_job(aio_list, iojob)) {
		printf("\nError: Could not create %s IO job. Skipping.\n", iojob->job_name);
		run_env_cmds(iojob->cleanup_tail);
	}
	else {
		int s = 0;
		int i;

		/* submit all jobs at once */
		s = lio_listio(LIO_WAIT, aio_list, total_jobs, NULL);
		if (0 <= s)
			printf("\nSuccessfully submitted %s IO job.\n", iojob->job_name);
		else {
			printf("\nError: Could not submit %s IO job.\n",
			       iojob->job_name);
			for (i = 0; i < total_jobs; i++)
				printf("    block %d - error %d.\n",
				       i, aio_error(aio_list[i]));
			free(aio_list);
			aio_list = NULL;
		}
	}
}

void cancel_paio_job(void *_aiojob)
{
	struct aio_job *aiojob = (struct aio_job *)_aiojob;
	int rc;
	struct aiocb *iocb;
	struct aio_block *aioblock;

	aioblock = aiojob->aio_block_head;
	while (1) {
		if (NULL == aioblock) break;
		if (aioblock->mask !=
				(aiojob->mask_expected &
						aioblock->mask)) {
			iocb = (struct aiocb *)aioblock->aio_entry;

			rc = aio_cancel(aiojob->fd, iocb);
			if (AIO_NOTCANCELED == rc)
				printf("Warning: %s job block %d itr %d not cancelled. Will wait until cancelled\n",
				       aiojob->iojob->job_name,
				       aioblock->block_num, aioblock->jobitr);
		}
		aioblock = aioblock->next;
	}
}
