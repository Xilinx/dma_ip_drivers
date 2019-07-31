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

#define _XOPEN_SOURCE 500
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#include <sys/mman.h>
#include <sys/time.h>

#include "aioutility.h"


#define DEFAULT_BLOCK_SIZE 4096
#define SEC2NSEC           1000000000
#define SEC2USEC           1000000
#define DEFAULT_PAGE_SIZE  4096

//#define AIO_DEBUG
#define USE_LINUX_NATIVE_AIO 1

enum env_cmd_type {
	ENV_CMD_GLOBAL_SETUP,
	ENV_CMD_GLOBAL_CLEANUP,
	ENV_CMD_IO_SETUP,
	ENV_CMD_IO_CLEANUP,
};

sem_t prev_io;

static struct env_cmds *glb_setup_cmd_head = NULL;
static struct env_cmds *glb_setup_cmd_tail = NULL;
static struct env_cmds *glb_clean_cmd_head = NULL;
static struct env_cmds *glb_clean_cmd_tail = NULL;
static struct io_jobs *io_job_head = NULL;
static struct io_jobs *io_job_tail = NULL;
struct aio_block *aio_block_head = NULL;
struct aio_block *aio_block_tail = NULL;
struct aio_job *aio_task_head = NULL;
static unsigned int num_io_jobs = 0;
#define ENV_GLOBAL_CMD_HEAD(cmd_type) (ENV_CMD_GLOBAL_SETUP == cmd_type)? &glb_setup_cmd_head:&glb_clean_cmd_head
#define ENV_GLOBAL_CMD_TAIL(cmd_type) (ENV_CMD_GLOBAL_SETUP == cmd_type)? &glb_setup_cmd_tail:&glb_clean_cmd_tail
#define ENV_IO_CMD_HEAD(cmd_type) (ENV_CMD_IO_SETUP == cmd_type)? &(iojob->setup_head):&(iojob->cleanup_head)
#define ENV_IO_CMD_TAIL(cmd_type) (ENV_CMD_IO_SETUP == cmd_type)?  &(iojob->setup_tail):&(iojob->cleanup_tail)

static struct option const long_opts[] = {
	{"config", required_argument, NULL, 'c'},
	{0, 0, 0, 0}
};

static void usage(const char *name)
{
	int i = 0;
	fprintf(stdout, "%s\n\n", name);
	fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);

	fprintf(stdout, "  -%c (--%s) config file that has IO jobs to be executed\n",
		long_opts[i].val, long_opts[i].name);
	i++;
}

static int timespec_check(struct timespec *t)
{
	if ((t->tv_nsec < 0) || (t->tv_nsec >= SEC2NSEC))
		return -1;
	return 0;

}

void timespec_sub(struct timespec *t1, struct timespec *t2)
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

static void enqueue_global_env_cmd(enum env_cmd_type cmd_type,
                               struct env_cmds *gcmd)
{
	struct env_cmds **gcmd_head = ENV_GLOBAL_CMD_HEAD(cmd_type);
	struct env_cmds **gcmd_tail = ENV_GLOBAL_CMD_TAIL(cmd_type);

	gcmd->prev = NULL;
	if (NULL == *gcmd_head) {
		*gcmd_head = gcmd;
		(*gcmd_head)->next = NULL;
		*gcmd_tail = *gcmd_head;
	} else {
		gcmd->next = *gcmd_head;
		(*gcmd_head)->prev = gcmd;
		*gcmd_head = gcmd;
	}
}

static struct env_cmds * dequeue_global_env_cmd(enum env_cmd_type cmd_type)
{
	struct env_cmds **gcmd_head = ENV_GLOBAL_CMD_HEAD(cmd_type);
	struct env_cmds **gcmd_tail = ENV_GLOBAL_CMD_TAIL(cmd_type);
	struct env_cmds *gcmd = *gcmd_head;

	if (*gcmd_tail == *gcmd_head) {
		*gcmd_tail = NULL;
		*gcmd_head = NULL;
	} else {
		*gcmd_head = (*gcmd_head)->next;
		(*gcmd_head)->prev = NULL;
	}

	return gcmd;
}

static void enqueue_io_env_cmd(enum env_cmd_type cmd_type,
                               struct io_jobs *iojob,
                               struct env_cmds *gcmd)
{
	struct env_cmds **gcmd_head = ENV_IO_CMD_HEAD(cmd_type);
	struct env_cmds **gcmd_tail = ENV_IO_CMD_TAIL(cmd_type);

	gcmd->prev = NULL;
	if (NULL == *gcmd_head) {
		*gcmd_head = gcmd;
		(*gcmd_head)->next = NULL;
		*gcmd_tail = *gcmd_head;
	} else {
		gcmd->next = *gcmd_head;
		(*gcmd_head)->prev = gcmd;
		*gcmd_head = gcmd;
	}
}

static struct env_cmds * dequeue_io_env_cmd(enum env_cmd_type cmd_type,
                                            struct io_jobs *iojob)
{
	struct env_cmds **gcmd_head = ENV_IO_CMD_HEAD(cmd_type);
	struct env_cmds **gcmd_tail = ENV_IO_CMD_TAIL(cmd_type);
	struct env_cmds *gcmd = *gcmd_head;

	if (*gcmd_tail == *gcmd_head) {
		*gcmd_tail = NULL;
		*gcmd_head = NULL;
	} else {
		*gcmd_head = (*gcmd_head)->next;
		(*gcmd_head)->prev = NULL;
	}

	return gcmd;
}

void enqueue_io_job(struct io_jobs *iojob)
{
	iojob->prev = NULL;
	if (NULL == io_job_head) {
		io_job_head = iojob;
		io_job_head->next = NULL;
		io_job_tail = io_job_head;
	} else {
		iojob->next = io_job_head;
		io_job_head->prev = iojob;
		io_job_head = iojob;
	}
	num_io_jobs++;
}

static struct io_jobs * dequeue_io_job(void)
{
	struct io_jobs *iojob = io_job_head;

	if (io_job_tail == io_job_head) {
		io_job_tail = NULL;
		io_job_head = NULL;
	} else {
		io_job_head = io_job_head->next;
		io_job_head->prev = NULL;
	}

	return iojob;
}

void enqueue_aio_block(struct aio_job *aiojob, struct aio_block *aioblock)
{
	if (NULL == aiojob->aio_block_head) {
		aiojob->aio_block_head = aioblock;
		aiojob->aio_block_head->next = NULL;
		aiojob->aio_block_tail = aiojob->aio_block_head;
	} else {
		aioblock->next = aiojob->aio_block_head;
		aiojob->aio_block_head = aioblock;
	}
}

static struct aio_block * dequeue_aio_block(struct aio_job *aiojob)
{
	struct aio_block *aioblock = aiojob->aio_block_head;

	if (aiojob->aio_block_tail == aiojob->aio_block_head) {
		aiojob->aio_block_head = NULL;
		aiojob->aio_block_tail = NULL;
	} else
		aiojob->aio_block_head = aiojob->aio_block_head->next;

	return aioblock;
}

void enqueue_aio_task(struct aio_job *aiotask)
{
	if (NULL == aio_task_head) {
		aio_task_head = aiotask;
		aio_task_head->next = NULL;
	} else {
		aiotask->next = aio_task_head;
		aio_task_head = aiotask;
	}
}

struct aio_block * get_aio_block(void *_aioblock)
{
	struct aio_job *aiotask = aio_task_head;
	struct aio_block *aioblock;

	while (aiotask) {
		aioblock = aiotask->aio_block_head;
		while (aioblock) {
			if (aioblock->aio_entry == _aioblock)
				break;
			aioblock = aioblock->next;
		}
		aiotask = aiotask->next;
	}

	return aioblock;
}


static char * strip_blanks(char *word, long unsigned int *banlks)
{
	char *p = word;
	unsigned int i = 0;

	while (isblank(p[0])) {
		p++;
		i++;
	}
	*banlks = i;

	return p;
}

static bool get_io_param_attrs(char *linebuf, size_t linelen,
                               unsigned int *lhspos, unsigned int *rhspos)
{
	long unsigned int numblanks = 0;
	char *p = strip_blanks(linebuf, &numblanks);
	size_t i = 0;

	while (i < (linelen - numblanks)) {
		if ('=' != p[i])
			i++;
		else
			break;
	}
	if (i == (linelen - numblanks)) {
		printf("Error: assignment operator missing in below line:\n %s\n",
		       linebuf);
		return false;
	} else {
		*lhspos = numblanks;
		p = strip_blanks(p + i + 1,&numblanks);
		*rhspos = i + 1 + numblanks;
	}

	return true;
}

static void add_env_cmd(enum env_cmd_type cmd_type,
                                     char *linebuf, size_t linelen, void *iojob)
{
	struct env_cmds *gcmd = malloc(sizeof(struct env_cmds));
	size_t numblanks;
	char *sh_cmd = strip_blanks(linebuf, &numblanks);
	unsigned int sh_cmd_len = linelen - numblanks;

	gcmd->cmd = malloc(sh_cmd_len);
	memcpy(gcmd->cmd, sh_cmd, sh_cmd_len);


	if ((ENV_CMD_GLOBAL_SETUP == cmd_type) ||
			(ENV_CMD_GLOBAL_CLEANUP == cmd_type))
		enqueue_global_env_cmd(cmd_type, gcmd);
	else
		enqueue_io_env_cmd(cmd_type, (struct io_jobs *)iojob, gcmd);
}

static bool parse_global_cmd_info(FILE *fp,
                            unsigned int *linenum)
{
	char *linebuf = NULL;
	char *realbuf;
	size_t linelen = 0;
	size_t numread;
	size_t numblanks;
	fpos_t prev_pos;

	fgetpos(fp, &prev_pos);
	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		*linenum = *linenum + 1;
		realbuf = strip_blanks(linebuf, &numblanks);
		numread -= numblanks;
		if (0 == numread)
			continue;
		else if ((realbuf[0] == ';') || (realbuf[0] == '#'))
			continue;
		else if (realbuf[0] == '[') { /* new job - return */
			fsetpos(fp, &prev_pos);
			break;
		} else {
			if (0 == strncmp(&realbuf[0], "setup", 5)) { /* enqueue env setup job */
				add_env_cmd(ENV_CMD_GLOBAL_SETUP,
				            &realbuf[5],
				            numread - 5,
				            NULL);
			} else if (0 == strncmp(&realbuf[0], "clean", 5)) { /* enqueue env cleanup job */
				add_env_cmd(ENV_CMD_GLOBAL_CLEANUP,
					    &realbuf[5],
					    numread - 5,
					    NULL);
			} else {
				printf ("Error: unknown global command\n");
				return false;
			}
		}
		fgetpos(fp, &prev_pos);
	}

	return true;
}

static bool parse_io_job_attrs(struct io_jobs *iojob, char *linebuf, size_t numread)
{
	unsigned int lhspos = 0, rhspos = 0;
	char *lhs, *rhs;

	if (0 == strncmp(linebuf, "setup", 5)) { /* enqueue env setup job */
		add_env_cmd(ENV_CMD_IO_SETUP,
		            &linebuf[5],
		            numread - 5,
		            iojob);
	} else if (0 == strncmp(linebuf, "clean", 5)) { /* enqueue env cleanup job */
		add_env_cmd(ENV_CMD_IO_CLEANUP,
			    &linebuf[5],
			    numread - 5,
		            iojob);
	}else if (get_io_param_attrs(linebuf, numread, &lhspos, &rhspos)) {
		lhs = linebuf + lhspos;
		rhs = linebuf + rhspos;
		if (0 == strncmp(lhs, "ioengine", 8)) {
			if (0 == strncmp(rhs, "libaio", 6)) {
				iojob->io_type = IO_ENGINE_LIBAIO;
				iojob->commit_io = submit_naio_job;
				iojob->cancel_io = cancel_naio_job;
			}
			else if (0 == strncmp(rhs, "posixaio", 8)) {
				iojob->io_type = IO_ENGINE_POSIXAIO;
				iojob->commit_io = submit_paio_job;
				iojob->cancel_io = cancel_paio_job;
			}
			else {
				printf ("Error: Invalid IO engine specified for %s job\n",
				        iojob->job_name);
				return false;
			}
		} else if (0 == strncmp(lhs, "filename", 8)) {
			iojob->node = malloc(numread - rhspos);
			memcpy(iojob->node, rhs, numread - rhspos);
		} else if (0 == strncmp(lhs, "iotype", 6)) {
			iojob->iotype = malloc(numread - rhspos);
			memcpy(iojob->iotype, rhs, numread - rhspos);
		} else if ((0 == strncmp(lhs, "numwrite", 8)) ||
				(0 == strncmp(lhs, "numread", 7))) {
			sscanf(rhs, "%u", &iojob->numbytes);
		} else if (0 == strncmp(lhs, "infile", 6)) {
			int fbuf;
			size_t datsize = 0;
			struct stat infile_stat;
			ssize_t numrd = 0;
			char *wrfilename = malloc(numread - rhspos);

			memcpy(wrfilename, rhs, numread - rhspos);

			fbuf = open(wrfilename, O_RDONLY);

			if (0 > fbuf) {
				printf("Error: opening %s failed\n", wrfilename);
				return false;
			} else {
				if (0 > fstat(fbuf, &infile_stat)) {
					printf("Error: File stat did not work for %s\n", wrfilename);
					return false;
				}
				datsize = infile_stat.st_size;
				if (0 > lseek(fbuf, 0, SEEK_SET)) {
					printf("Error: Setting file descriptor to beginning for %s failed\n",
					       wrfilename);
					return false;
				}
				iojob->buf = malloc(datsize);

				while (datsize) {
					numrd = read(fbuf, iojob->buf, datsize);
					if (0 < numrd) {
						datsize -= numrd;
					} else
						break;
				}
				close(fbuf);
			}
			free(wrfilename);
		} else if (0 == strncmp(lhs, "block", 5)) {
			sscanf(rhs, "%u", &iojob->block_size);
		} else if (0 == strncmp(lhs, "numio", 5)) {
			sscanf(rhs, "%u", &iojob->numitr);
		} else if (0 == strncmp(lhs, "verify_file", 11)) {
			if (iojob->verify_read != READ_VERIFY_FILE) {
				printf("Error: verify method not matching with input for %s\n",
				       iojob->job_name);
				return false;
			}
			iojob->verify_file_name = malloc(numread - rhspos);
			memcpy(iojob->verify_file_name, rhs, numread - rhspos);
		} else if (0 == strncmp(lhs, "verify_pattern", 14)) {
			if (iojob->verify_read != READ_VERIFY_PATTERN) {
				printf("Error: verify method not matching with input for %s\n",
				       iojob->job_name);
				return false;
			}
			iojob->verify_pattern = malloc(numread - rhspos);
			memcpy(iojob->verify_pattern, rhs, numread - rhspos);
		} else if (0 == strncmp(lhs, "verify", 6)) {
			if (0 == strncmp(rhs, "file", 4)) {
				iojob->verify_read = READ_VERIFY_FILE;
			} else if (0 == strncmp(rhs, "pattern", 7)) {
				iojob->verify_read = READ_VERIFY_PATTERN;
			} else {
				printf("Error: unknown read verify type in %s\n",
				       linebuf);
				return false;
			}
		} else {
			printf("Error: Unknown IO option\n");
		}
	} else
		return false;

	return true;
}

static bool parse_io_job_info(struct io_jobs *iojob, FILE *fp,
                            unsigned int *linenum)
{
	char *linebuf = NULL;
	char *realbuf;
	size_t linelen = 0;
	size_t numread;
	size_t numblanks;
	fpos_t prev_pos;

	fgetpos(fp, &prev_pos);
	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		(*linenum)++;
		realbuf = strip_blanks(linebuf, &numblanks);
		numread -= numblanks;
		if (0 == numread)
			continue;
		else if ((realbuf[0] == ';') || (realbuf[0] == '#'))
			continue;
		else if (realbuf[0] == '[') { /* new job - return */
			fsetpos(fp, &prev_pos);
			break;
		} else {
			if (false == parse_io_job_attrs(iojob, linebuf, numread))
				return false;
		}
		fgetpos(fp, &prev_pos);
	}

	return true;
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

	fp = fopen(cfg_fname, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((numread = getline(&linebuf, &linelen, fp)) != -1) {
		numread--;
		linenum++;
		realbuf = strip_blanks(linebuf, &numblanks);
		linelen -= numblanks;
		if (0 == linelen)
			continue;
		else if ((';' == realbuf[0]) || ('#' == realbuf[0]))
			continue;
		else if (realbuf[0] == '[') { /* extract job name */
			unsigned char job_eidx = 1;
			char *jobname;

			while (']' != realbuf[job_eidx])
				job_eidx++;
			if (0 == strncmp(&realbuf[1], "global", job_eidx - 1)) {
				if (!parse_global_cmd_info(fp, &linenum)) {
					fclose(fp);
					exit(1);
				}
			} else { /* enqueue io job */
				struct io_jobs *iojob = calloc(1, sizeof(struct io_jobs));

				memset(iojob, 0 ,sizeof(struct io_jobs));
				iojob->job_name = malloc(job_eidx - 1);
				iojob->verify_read = READ_VERIFY_NONE;
				memcpy(iojob->job_name, &realbuf[1], job_eidx - 1);
				iojob->block_size = DEFAULT_BLOCK_SIZE;
				if (!parse_io_job_info(iojob, fp, &linenum)) {
					free(iojob->job_name);
					free(iojob);
					fclose(fp);
					exit(1);
				}
				if (0 == strncmp(iojob->iotype, "read", 4)) {
					if (0 == iojob->numbytes) {
						printf ("Error:Number of bytes to read not provided for %s\n",
						        iojob->job_name);
						exit(0);
					} else {
						unsigned int num_blocks = get_io_job_num_blocks(iojob);

						if (0 != posix_memalign((void **)&iojob->buf,
								DEFAULT_PAGE_SIZE /*alignment */ ,
						               (iojob->numitr *
						        	num_blocks *
						        	iojob->block_size) +
						        	DEFAULT_PAGE_SIZE)) {
							printf("Error: Memory could not be allocated\n");
							exit(0);
						}
					}
				}
				enqueue_io_job(iojob);
			}
		}
	}
	fclose(fp);
}

void run_env_cmds(struct env_cmds *gcmd)
{
	while (NULL != gcmd) {
		printf("Running cmd: %s\n", gcmd->cmd);
		system(gcmd->cmd);
		gcmd = gcmd->prev;
	}
}

void verify_read(struct io_jobs *iojob)
{
	char *expected;
	int rd_diff;
	unsigned int i, j;
	unsigned int total_jobs = get_io_job_num_blocks(iojob);

	if (iojob->verify_read == READ_VERIFY_FILE) {
		int fd;
		unsigned int datsize;
		struct stat infile_stat;
		ssize_t numrd = 0;
		int rc;

		fd = open(iojob->verify_file_name, O_RDONLY);
		if (0 > fstat(fd, &infile_stat)) {
			printf("Error: File stat did not work for %s\n", iojob->verify_file_name);
			return;
		}
		datsize = infile_stat.st_size;
		if (0 > lseek(fd, 0, SEEK_SET)) {
			printf("Error: Setting file descriptor to beginning for %s failed\n",
			       iojob->verify_file_name);
			return;
		}
		expected = malloc(datsize);
		while (datsize) {
			numrd = read(fd, expected, datsize);
			if (0 < numrd) {
				datsize -= numrd;
			} else
				break;
		}
		close(fd);

		for (i = 0; i < iojob->numitr; i++) {
			unsigned int buf_offset = (i * total_jobs * iojob->block_size);
			unsigned int io_numbytes = iojob->numbytes;

			rd_diff = memcmp(expected,
					 &iojob->buf[buf_offset],
					 io_numbytes);
			if (0 != rd_diff) {

				printf("Error: read verify failed for %s itr %d at %d\n",
					       iojob->job_name, i, rd_diff);

			} else
				printf("Read validated for %s -> itr %u successfully\n", iojob->job_name, i);
		}
		free(expected);
	} else if (iojob->verify_read == READ_VERIFY_PATTERN) {
		unsigned int pat_len = strlen(iojob->verify_pattern);
		unsigned int offset = 0;

		unsigned int buf_offset = (i * iojob->block_size);
		while (offset < iojob->numbytes) {
			rd_diff = memcmp(iojob->verify_pattern,
					 &iojob->buf[offset],
					 pat_len);
			if (0 != rd_diff)
				printf("Error: read verify failed for %s at %d\n",
					       iojob->job_name, offset + rd_diff);
			offset += pat_len;
		}
	}
}

unsigned int get_io_job_num_blocks(struct io_jobs *iojob)
{
	unsigned int num_blks;
	unsigned int extra_block = (iojob->numbytes % iojob->block_size)? 1 : 0;

	num_blks = (iojob->numbytes / iojob->block_size) + extra_block;

	return num_blks;
}


#ifdef AIO_DEBUG
static void dump_iojob(struct io_jobs *iojob)
{
	struct env_cmds *gcmd;

	printf("%s:\n", iojob->job_name);
	gcmd = iojob->setup_tail;
	while (NULL != gcmd) {
		printf("%s\n", gcmd->cmd);
		gcmd = gcmd->prev;
	}
	if (NULL == iojob->setup_tail)
		printf("No IO setup cmds listed\n");
	if (NULL == iojob->cleanup_tail)
		printf("No IO cleanup cmds listed\n");
	gcmd = iojob->cleanup_tail;
	while (NULL != gcmd) {
		printf("%s\n", gcmd->cmd);
		gcmd = gcmd->prev;
	}
	printf("working on %s:\n", iojob->node);
	printf("IO Operation: %s\n", iojob->iotype);
	printf("verification type: %d\n", iojob->verify_read);
	printf("verify_file_name: %s\n", iojob->verify_file_name);
	printf("verify_pattern: %s\n", iojob->verify_pattern);
	printf("numbytes: %d\n", iojob->numbytes);
	printf("block_size: %d\n", iojob->block_size);
	printf("numitr: %d\n", iojob->numitr);
}
#endif

static void run_io_job(unsigned int jobnum)
{
	struct io_jobs *iojob = io_job_tail;

	if (jobnum > num_io_jobs) {
		printf("Error: Invalid IO job number %d\n", jobnum);
		return;
	}

	if (0 == jobnum) {
		while (iojob) {
#ifdef AIO_DEBUG
			dump_iojob(iojob);
#endif
			iojob->commit_io(iojob);
			iojob = iojob->prev;
		}
	} else {
		while (--jobnum) {
			iojob = iojob->prev;
		}

#ifdef AIO_DEBUG
		dump_iojob(iojob);
#endif
		iojob->commit_io(iojob);
	}
}

static void cancel_outstanding_reqs(void)
{
	struct aio_block *aioblock;
	struct aio_job *aiojob = aio_task_head;
	struct aiocb *iocb;
	int rc;

	while (NULL != aiojob) {
		aiojob->iojob->cancel_io(aiojob);
		aiojob = aiojob->next;
	}
}

bool cleanup_aiojob(struct aio_job *aiojob, bool force)
{
	struct aio_block *aioblock;
	void *aio_list;
	struct timespec ts_cur;
	int rc;
	struct io_jobs *iojob = aiojob->iojob;

	aioblock = aiojob->aio_block_head;
	while (force) {
		if (NULL != aioblock) {
			if (aiojob->mask_completed & aioblock->mask)
				goto aioblock_next;
			rc = clock_gettime(CLOCK_MONOTONIC, &ts_cur);
			ts_cur.tv_nsec += SEC2USEC;
			printf("waiting for %s iter %d block %d to cancel\n",
			       iojob->job_name, aioblock->jobitr,
			       aioblock->block_num);
			sem_timedwait(&aioblock->lock, &ts_cur);
			sem_destroy(&aioblock->lock);
			aiojob->mask_completed |= aioblock->mask;
		} else
			break;
aioblock_next:
		aioblock = aioblock->next;
	}
	if (aiojob->mask_completed != aiojob->mask_expected) {
		if (force)
			printf("Invalid path in aio task cleanup\n");
		return false;
	}
	run_env_cmds(iojob->cleanup_tail);
	while (1) {
		aioblock = dequeue_aio_block(aiojob);
		if (NULL == aioblock) break;
		if (NULL != aioblock->aio_entry) {
			free(aioblock->aio_entry);
			aioblock->aio_entry = NULL;
		}
		free(aioblock);
		aioblock = NULL;
	}
	close(aiojob->fd);
	aio_list = aiojob->aio_list;
	aio_task_head = aio_task_head->next;
	aiojob->aio_list = NULL;
	free(aiojob);
	aiojob = NULL;
	free(aio_list);

	sem_post(&prev_io);

	return true;
}

void cleanup_aio_jobs(bool force)
{
	struct aio_job *aiojob = aio_task_head;

	while (NULL != aiojob) {
		if (false == cleanup_aiojob(aiojob, force))
			break; /* need to clean up sequentially in reverse */
		else
			aiojob = aio_task_head;
	}
}

static void cleanup(void)
{
	struct io_jobs *iojob;
	struct env_cmds *gcmd;

	cancel_outstanding_reqs();
	cleanup_aio_jobs(true);

	while (1) {
		iojob = dequeue_io_job();
		if (NULL == iojob) break;
		if (NULL != iojob->verify_file_name)
			free(iojob->verify_file_name);
		if (NULL != iojob->verify_pattern)
			free(iojob->verify_pattern);
		free(iojob->iotype);
		free(iojob->buf);
		free(iojob->node);
		free(iojob->job_name);
		while (iojob->cleanup_head) {
			gcmd = dequeue_io_env_cmd(ENV_CMD_IO_CLEANUP, iojob);
			free(gcmd->cmd);
			free(gcmd);
		}
		while (iojob->setup_head) {
			gcmd = dequeue_io_env_cmd(ENV_CMD_IO_SETUP, iojob);
			free(gcmd->cmd);
			free(gcmd);
		}
		free(iojob);
	}
	while (1) {
		gcmd = dequeue_global_env_cmd(ENV_CMD_GLOBAL_CLEANUP);
		if (NULL == gcmd) break;
		free(gcmd->cmd);
		free(gcmd);
	}
	while (1) {
		gcmd = dequeue_global_env_cmd(ENV_CMD_GLOBAL_SETUP);
		if (NULL == gcmd) break;
		free(gcmd->cmd);
		free(gcmd);
	}
}

int main(int argc, char *argv[])
{
	int cmd_opt;
	char *cfg_fname;
	unsigned int i;
	struct io_jobs *iojob;

	while ((cmd_opt = getopt_long(argc, argv, "vhxc:c:", long_opts,
			    NULL)) != -1) {
		switch (cmd_opt) {
		case 0:
			/* long option */
			break;
		case 'c':
			/* condig file name */
			cfg_fname = strdup(optarg);
			break;
		default:
			usage(argv[0]);
			exit(0);
			break;
		}
	}

	parse_config_file(cfg_fname);
	sem_init(&prev_io, 0, 1);
	atexit(cleanup);
#ifdef AIO_DEBUG
		{
			struct env_cmds *gcmd;

			gcmd = glb_setup_cmd_tail;
			while (NULL != gcmd) {
				printf("%s\n", gcmd->cmd);
				gcmd = gcmd->prev;
			}
			gcmd = glb_clean_cmd_tail;
			while (NULL != gcmd) {
				printf("%s\n", gcmd->cmd);
				gcmd = gcmd->prev;
			}
		}
#endif

	while (1) {
		unsigned int j = 0;

		printf("\nSelect the Job to run:\n");
		printf("0. exit\n");
		printf("1. global setup\n");
		printf("2. global cleanup\n");
		printf("3. All IO jobs\n");
		i = 4;
		iojob = io_job_tail;
		while (iojob) {
			printf("%d. %s\n", i, iojob->job_name);
			iojob = iojob->prev;
			i++;
		}
		scanf("%u", &j);
		sem_wait(&prev_io);
		if (0 == j) {
			exit(0);
		} else if (1 == j) {
			run_env_cmds(glb_setup_cmd_tail);
			sem_post(&prev_io);
		} else if (2 == j) {
			run_env_cmds(glb_clean_cmd_tail);
			sem_post(&prev_io);
		} else if (3 == j) {
			run_io_job(0);
		} else if (j < i) {
			printf("Running IO Job %d\n", j-3);
			run_io_job(j - 3);
		} else {
			printf ("Error: Invalid option\n");
		}
		sem_wait(&prev_io);
		exit(0);
	}

	return 0;
}

