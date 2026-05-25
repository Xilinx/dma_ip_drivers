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
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <libaio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "dma_common_utils.h"

/*
 * man 2 write:
 * On Linux, write() (and similar system calls) will transfer at most
 *	0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
 *	actually transferred.  (This is true on both 32-bit and 64-bit
 *	systems.)
 *
 * Reference: https://man7.org/linux/man-pages/man2/write.2.html
 */

#define RW_MAX_SIZE	(0x7ffff000)
#define GB_DIV		(1000000000)
#define MB_DIV		(1000000)
#define KB_DIV		(1000)

char *get_io_type(enum mdb5_dma_io_type io_type)
{
	switch (io_type) {
	case MDB5_IO_SYNC:
		return "sync";
	case MDB5_IO_ASYNC:
		return "async";
	default:
		return NULL;
	}
}

char *get_chan_dir(enum mdb5_dma_chan_dir dir)
{
	switch (dir) {
	case MDB5_CHAN_DIR_FROM_DEV:
		return "read";
	case MDB5_CHAN_DIR_TO_DEV:
		return "write";
	default:
		return NULL;
	}
}

char *get_chan_mode(enum mdb5_dma_chan_mode chan_mode)
{
	switch (chan_mode) {
	case MDB5_CHAN_MODE_MM:
		return "mm";
	default:
		return NULL;
	}
}

char *get_transfer_mode(enum mdb5_dma_transfer_mode mode)
{
	switch (mode) {
	case MDB5_MODE_SG:
		return "Scatter Gather DMA";
	case MDB5_MODE_SIMPLE:
		return "Simple DMA";
	default:
		return NULL;
	}
}

void dump_throughput_result(uint64_t size, float result)
{
	if (((long long)(result) / GB_DIV))
		mdb5_dma_info("Average BW = %f GB/sec\n",
			  ((double)result / GB_DIV));
	else if (((long long)(result) / MB_DIV))
		mdb5_dma_info("Average BW = %f MB/sec\n",
			  ((double)result / MB_DIV));
	else if (((long long)(result) / KB_DIV))
		mdb5_dma_info("Average BW = %f KB/sec\n",
			  ((double)result / KB_DIV));
	else
		mdb5_dma_info("Average BW = %f Bytes/sec\n", ((double)result));
}

uint64_t getopt_integer(char *optarg)
{
	int rc;
	uint64_t value;

	rc = sscanf(optarg, "0x%lx", &value);
	if (rc <= 0)
		sscanf(optarg, "%lu", &value);

	return value;
}

bool is_file_available(char *pathname)
{
	struct stat stat_buf;

	memset(&stat_buf, 0, sizeof(stat));
	if (stat(pathname, &stat_buf) != 0) {
		mdb5_dma_err("Failed to stat file '%s': %s\n",
			     pathname, mdb5_dma_strerror(errno));
		return false;
	}

	return true;
}

/**
 * @brief Prepares the name of an MDB5 node.
 *
 * This function constructs the name of an MDB5 node based on the PCI device
 * number, format string, virtual function flag, and channel number.
 *
 * @param   s Pointer to the buffer where the node name will be stored.
 *
 * @param   fmt Format string indicating the type of node
 *          (e.g., "write", "read", "ctrl").
 *
 * @param   ch_num Channel number.
 *
 * @return
 *          None.
 */
void prepare_node_name(char *s, char *fmt, uint32_t ch_num)
{
	if (!strncmp(fmt, "write", 5))
		snprintf(s, MDB5_NODE_CHAN_SZ,
			 MDB5_NODE_CHAN_NAME_FMT,
			 fmt, ch_num);
	else if (!strncmp(fmt, "read", 4))
		snprintf(s, MDB5_NODE_CHAN_SZ,
			 MDB5_NODE_CHAN_NAME_FMT,
			 fmt, ch_num);
	else if (!strncmp(fmt, "ctrl", 4))
		snprintf(s, MDB5_NODE_CTRL_SZ,
			 MDB5_NODE_CTRL_NAME_FMT);
	
}

/*
 * Register I/O through mmap of BAR0.
 */

/* /sys/bus/pci/devices/<DOMAIN>:<bus>:<dev>.<func>/resource<bar#> */
#define get_syspath_bar_mmap(s, domain, bdf, bar_num) \
	snprintf(s, sizeof(s), \
		"/sys/bus/pci/devices/%04x:%s/resource%u", \
		domain, bdf, bar_num)

static uint32_t *mmap_bar(char *fname, size_t len, int prot)
{
	int fd;
	uint32_t *bar = NULL;

	fd = open(fname, (prot & PROT_WRITE) ? O_RDWR : O_RDONLY);
	if (fd < 0) {
		mdb5_dma_err("%s: Failed to open file '%s': %s\n", __func__,
			     fname, mdb5_dma_strerror(errno));
		return NULL;
	}

	bar = mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	if (!bar) {
		mdb5_dma_err("%s: Memory mapping failed for file descriptor "
			     "%d with error: %s\n", __func__,
			     fd, mdb5_dma_strerror(errno));
		goto error;
	}

error:
	close(fd);
	return bar == MAP_FAILED ? NULL : bar;
}

/**
 * @brief   Reads from an @c bar_num register offset using memory-mapped I/O.
 *
 * This function reads data from @c bar_num register offset specified by the
 * given @c mdb5_dma_reg structure.
 *
 * @param   reg Pointer to the `mdb5_dma_reg` structure containing the register
 * information.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int reg_read_mmap(struct mdb5_dma_reg *reg)
{
	uint32_t offset = reg->offset;
	uint32_t *bar;
	uint8_t bar_num = PCI_BAR0;
	char fname[256];
	char *bdf = reg->pci_bdf;
	int ret = 0;

	get_syspath_bar_mmap(fname, PCI_DOMAIN, bdf, bar_num);

	if (!is_file_available(fname)) {
		ret = -ENOENT;
		goto error;
	}

	bar = mmap_bar(fname, offset + REGISTER_SIZE, PROT_READ);
	if (!bar) {
		ret = -1;
		goto error;
	}

	reg->data = bar[offset / REGISTER_SIZE];
	munmap(bar, offset + REGISTER_SIZE);

error:
	return ret;
}

/**
 * @brief   Writes data to @c bar_num register offset using memory-mapped I/O.
 *
 * This function writes data to @c bar_num register offset specified by the
 * given @c mdb5_dma_reg structure.
 *
 * @param   reg Pointer to the `mdb5_dma_reg` structure containing the register
 * information.
 *
 * @return  0 on success, or a negative error code on failure.
 */
int reg_write_mmap(struct mdb5_dma_reg *reg)
{
	uint32_t offset = reg->offset;
	uint32_t *bar;
	uint8_t bar_num = PCI_BAR0;
	char fname[256];
	char *bdf = reg->pci_bdf;
	int ret = 0;

	get_syspath_bar_mmap(fname, PCI_DOMAIN, bdf, bar_num);

	if (!is_file_available(fname)) {
		ret = -ENOENT;
		goto error;
	}

	bar = mmap_bar(fname, offset + REGISTER_SIZE, PROT_WRITE);
	if (!bar) {
		ret = -1;
		goto error;
	}

	bar[offset / REGISTER_SIZE] = reg->data;
	munmap(bar, offset + REGISTER_SIZE);

error:
	return ret;
}

/**
 * @brief   Reads data from a file into a buffer.
 *
 * This function reads a specified amount of data from a file, starting at a
 * given base offset, into a provided buffer.
 *
 * @param   fname Name of the file to read from.
 *
 * @param   fd File descriptor of the file.
 *
 * @param   buffer Pointer to the buffer where data will be stored.
 *
 * @param   size Number of bytes to read.
 *
 * @param   base Base offset in the file to start reading from.
 *
 * @return  ssize_t Number of bytes read on success, or a negative error code
 *          on failure.
 */
ssize_t read_to_buffer(char *fname, int fd, char *buffer, size_t size,
		       size_t base)
{
	ssize_t rc;
	size_t count = 0;
	char *buf = buffer;
	off_t offset = (off_t) base;
	int ret;

	do {
		size_t bytes = size - count;

		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;

		/* read data from file at offset into memory buffer */
		rc = pread(fd, buf, bytes, offset);
		if (rc < 0) {
			mdb5_dma_err("%s, read off 0x%lx + 0x%lx failed: %s\n",
				     fname, offset, bytes, mdb5_dma_strerror(errno));
			ret = -errno;
			goto error;
		}
		if (rc != bytes) {
			mdb5_dma_err("%s, read off 0x%lx, 0x%lx != 0x%lx\n",
				     fname, count, rc, bytes);
			ret = -EIO;
			goto error;
		}

		count += bytes;
		buf += bytes;
		offset += bytes;
	} while (count < size);

	if (count != size) {
		mdb5_dma_err("%s, read failed 0x%lx != 0x%lx\n",
			     fname, count, size);
		ret = -EIO;
		goto error;
	}

	return count;
error:
	return ret;
}

/**
 * @brief   Writes data from a buffer to a file.
 *
 * This function writes a specified amount of data from a provided buffer to
 * a file, starting at a given base offset.
 *
 * @param   fname Name of the file to write to.
 *
 * @param   fd File descriptor of the file.
 *
 * @param   buffer Pointer to the buffer containing the data to be written.
 *
 * @param   size Number of bytes to write.
 *
 * @param   base Base offset in the file to start writing to.
 *
 * @return  ssize_t Number of bytes written on success, or a negative error
 *          code on failure.
 */
ssize_t write_from_buffer(char *fname, int fd, char *buffer, size_t size,
			  size_t base)
{
	ssize_t rc;
	size_t count = 0;
	char *buf = buffer;
	off_t offset = (off_t) base;
	int ret;

	do {
		size_t bytes = size - count;

		if (bytes > RW_MAX_SIZE)
			bytes = RW_MAX_SIZE;

		/* write data to file at offset from memory buffer */
		rc = pwrite(fd, buf, bytes, offset);
		if (rc < 0) {
			mdb5_dma_err("%s, write off 0x%lx, 0x%lx failed: %s\n",
				     fname, offset, bytes, mdb5_dma_strerror(errno));
			ret = -errno;
			goto error;
		}
		if (rc != bytes) {
			mdb5_dma_err("%s, write off 0x%lx, 0x%lx != 0x%lx\n",
				     fname, offset, rc, bytes);
			ret = -EIO;
			goto error;
		}

		count += bytes;
		buf += bytes;
		offset += bytes;
	} while (count < size);

	if (count != size) {
		mdb5_dma_err("%s, write failed 0x%lx != 0x%lx\n",
			     fname, count, size);
		ret = -EIO;
		goto error;
	}

	return count;
error:
	return ret;
}

/**
 * @brief   Submits an asynchronous I/O operation for MDB5.
 *
 * This function submits an asynchronous read or write operation for MDB5 using
 * the specified file descriptor, buffer, size, and offset.
 *
 * @param   fd File descriptor of the file.
 *
 * @param   buffer Pointer to the buffer containing the data to be read or
 *          written.
 *
 * @param   size Number of bytes to read or write.
 *
 * @param   offset Offset in the file to start the operation.
 *
 * @param   enum chan_dir indicating whether the operation is a
 *          write/MDB5_CHAN_DIR_TO_DEV or read/MDB5_CHAN_DIR_FROM_DEV.
 *
 * @return  ssize_t Number of bytes read or written on success, or a negative
 *          error code on failure.
 */
static ssize_t mdb5_dma_asynciosubmit(int fd, char *buffer, size_t size,
				      size_t offset,
				      enum mdb5_dma_chan_dir chan_dir)
{
	io_context_t aioctx;
	struct iocb *iocb = NULL;
	struct iocb *iocbp;
	struct iovec *iov;
	struct io_event event;
	struct timespec ts_cur = { 1, 0 };
	unsigned int num_events;
	int max_events = 1;
	ssize_t ret;

	memset(&aioctx, 0, sizeof(aioctx));
	ret = io_queue_init(max_events, &aioctx);
	if (ret != 0) {
		mdb5_dma_err("%s: io_setup error: %s\n", __func__, mdb5_dma_strerror(errno));
		return ret;
	}

	iocb = (struct iocb *)calloc(sizeof(struct iocb), 1);
	if (!iocb) {
		mdb5_dma_err("%s: OOM: %s\n", __func__, mdb5_dma_strerror(errno));
		return -ENOMEM;
	}

	if (chan_dir == MDB5_CHAN_DIR_TO_DEV)
		io_prep_pwrite(iocb, fd, buffer, size, offset);
	else if (chan_dir == MDB5_CHAN_DIR_FROM_DEV)
		io_prep_pread(iocb, fd, buffer, size, offset);

	ret = io_submit(aioctx, max_events, &iocb);
	if (ret != 1) {
		mdb5_dma_err("%s: io_submit failed: %s\n",
			 __func__, mdb5_dma_strerror(errno));
		ret = -errno;
		goto cleanup;
	}

	num_events = io_getevents(aioctx, 1, max_events, &event, &ts_cur);
	if (num_events != 1) {
		mdb5_dma_err("%s: io_getevents timed out\n", __func__);
		ret = -EIO;
		goto cleanup;
	}

	iocbp = (struct iocb *)event.obj;
	if (!iocbp) {
		mdb5_dma_err("%s: Invalid IOCB from events\n", __func__);
		ret = -EIO;
		goto cleanup;
	}

	iov = (struct iovec *)(iocbp->u.c.buf);
	if (!iov) {
		mdb5_dma_err("%s: Invalid buffer\n", __func__);
		ret = -EIO;
		goto cleanup;
	}

	ret = (ssize_t) size;
cleanup:
	if (iocb)
		free(iocb);
	io_queue_release(aioctx);
	return ret;
}

/**
 * @brief   Submits an I/O operation for MDB5.
 *
 * This function submits a read or write operation for MDB5 using the specified
 * file name, buffer, size, and base offset. It supports both synchronous and
 * asynchronous I/O types.
 *
 * @param   fname Name of the file to read from or write to.
 *
 * @param   enum chan_dir indicating whether the operation is a
 *          write/MDB5_CHAN_DIR_TO_DEV or read/MDB5_CHAN_DIR_FROM_DEV.
 *
 * @param   io_type Type of I/O operation (MDB5_IO_SYNC or MDB5_IO_ASYNC).
 *
 * @param   buffer Pointer to the buffer containing the data to be read or
 *          written.
 *
 * @param   size Number of bytes to read or write.
 *
 * @param   base Base offset in the file to start the operation.
 *
 * @return  ssize_t Number of bytes read or written on success, or a negative
 *          error code on failure.
 */
static ssize_t mdb5_dma_iosubmit(char *fname, enum mdb5_dma_chan_dir chan_dir,
			         enum mdb5_dma_io_type io_type, char *buffer,
			         size_t size, size_t base)
{
	ssize_t count = 0;
	int fd;

	if (!fname || !buffer || size == 0) {
		mdb5_dma_err("%s: Invalid arguments\n", __func__);
		return -EINVAL;
	}

	fd = open(fname, O_RDWR);
	if (fd < 0)
		return fd;

	if (io_type == MDB5_IO_SYNC) {
		if (chan_dir == MDB5_CHAN_DIR_TO_DEV)
			count = write_from_buffer(fname, fd,
						  buffer, size, base);
		else if (chan_dir == MDB5_CHAN_DIR_FROM_DEV)
			count = read_to_buffer(fname, fd, buffer, size, base);
	} else if (io_type == MDB5_IO_ASYNC) {
		count = mdb5_dma_asynciosubmit(fd, buffer, size, base, chan_dir);
	}

	close(fd);

	return count;
}

int mdb5_dmautils_sync_xfer(char *filename, enum mdb5_dma_chan_dir chan_dir,
			    void *buf, size_t xfer_len, size_t base)
{
	return mdb5_dma_iosubmit(filename, chan_dir, MDB5_IO_SYNC, buf,
			         xfer_len, base);
}

int mdb5_dmautils_async_xfer(char *filename, enum mdb5_dma_chan_dir chan_dir,
			     void *buf, size_t xfer_len, size_t base)
{
	return mdb5_dma_iosubmit(filename, chan_dir, MDB5_IO_ASYNC, buf,
			         xfer_len, base);
}

static int timespec_check(struct timespec *t)
{
	if (t->tv_nsec < 0 || t->tv_nsec >= 1000000000)
		return -1;
	return 0;
}

/**
 * Subtract timespec t2 from t1
 *
 * Both t1 and t2 must already be normalized
 * i.e. 0 <= nsec < 1000000000
 */
void timespec_sub(struct timespec *t1, struct timespec *t2)
{
	if (timespec_check(t1) < 0) {
		mdb5_dma_err("%s: Invalid time #1: %lld.%.9ld.\n",
			     __func__, (long long)t1->tv_sec, t1->tv_nsec);
		return;
	}
	if (timespec_check(t2) < 0) {
		mdb5_dma_err("%s: Invalid time #2: %lld.%.9ld.\n",
			     __func__, (long long)t2->tv_sec, t2->tv_nsec);
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

char *strip_blanks(char *word, unsigned long *blanks)
{
	char *p = word;
	uint32_t i = 0;

	while (isblank(p[0])) {
		p++;
		i++;
	}
	if (blanks)
		*blanks = i;

	return p;
}

uint32_t copy_value(char *src, char *dst, uint32_t max_len)
{
	char *p = src;
	uint32_t i = 0;

	while (max_len && !isspace(p[0])) {
		dst[i] = p[0];
		p++;
		i++;
		max_len--;
	}

	return i;
}

char *strip_comments(char *word)
{
	size_t numblanks;
	char *p = strip_blanks(word, &numblanks);

	if (p[0] == '#')
		goto skip_processing;
	else
		p = strtok(word, "#");

	return p;

skip_processing:
	return NULL;
}

int arg_read_int(char *s, uint32_t *v)
{
	char *p = NULL;

	*v = strtoull(s, &p, 0);
	if (*p && (*p != '\n') && !isblank(*p)) {
		mdb5_dma_err("%s: something not right%s %s %s", __func__, s, p,
			     isblank(*p) ? "true" : "false");
		return -EINVAL;
	}
	return 0;
}

int arg_read_int_ull(char *s, uint64_t *v)
{
	char *p = NULL;

	*v = strtoull(s, &p, 0);
	if (*p && (*p != '\n') && !isblank(*p)) {
		mdb5_dma_err("%s: something not right%s %s %s", __func__, s, p,
			     isblank(*p) ? "true" : "false");
		return -EINVAL;
	}
	return 0;
}

int validate_pkt_size(size_t pkt_sz)
{
	if (pkt_sz == 0 || pkt_sz > MDB5_MAX_PKT_SZ) {
		mdb5_dma_err("allowed range is %u - %lu, given size is %lu\n", 1, MDB5_MAX_PKT_SZ, pkt_sz);
		return -EINVAL;
	}

    return 0;
}

const char *mdb5_dma_strerror(int error_code)
{

    error_code = (error_code < 0) ? -error_code : error_code;

    switch (error_code) {
    case ETIMEDOUT:
        return "DMA transfer timed out - device not responding or busy";
    case EBUSY:
        return "DMA channel is busy or in use";
    case EIO:
        return "DMA I/O error - hardware failure or misconfiguration";
    case ENOMEM:
        return "Insufficient memory for DMA operation";
    case EINVAL:
        return "Invalid DMA parameters or configuration";
    case ENODEV:
        return "DMA device not found or not available";
    case EACCES:
        return "Permission denied - check device access rights";
    case EAGAIN:
        return "DMA resource temporarily unavailable - retry later";
    case EFAULT:
        return "Invalid memory address for DMA transfer";
    case ENOENT:
        return "DMA device or channel does not exist";
    case EPERM:
        return "Operation not permitted on DMA device";
	case ENOSPC:
		return "Insufficient buffer space for DMA operation";
	case ERANGE:
		return "DMA transfer size out of valid range";
    default:
        return strerror(error_code);
    }
}
