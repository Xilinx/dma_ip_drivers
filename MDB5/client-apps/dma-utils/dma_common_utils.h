/*
 * This file is part of the MDB5 userspace application
 * to enable the user to execute the MDB5 functionality
 *
 * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#ifndef _DMA_COMMON_UTILS_H_
#define _DMA_COMMON_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MDB5_NODE_CTRL_NAME_FMT		"/dev/mdb5_ctrl"
#define MDB5_NODE_CHAN_NAME_FMT		"/dev/mdb5_%s%02d"
#define MDB5_NODE_CTRL_SZ		(18)
#define MDB5_NODE_CHAN_SZ		(20)
#define MDB5_MAX_RD_CHAN		(8)
#define MDB5_MAX_WR_CHAN		(8)
#define MDB5_NAME_SZ			(32)
#define FILE_NAME_SZ			(128)
#define ALIGN_4K			(4096)
#define MDB5_MAX_PKT_SZ 0x100000000

#define PCI_BAR0			(0)
#define PCI_DOMAIN			(0)
#define REGISTER_SIZE			(4)

/* Provide signed value x */
#define is_negative(x) (((x) < 0) ? true : false)
#define SET_BIT(num, bit) ((num) |= (1 << (bit)))
#define IS_BIT_SET(num, bit) (((num) & (1 << (bit))) != 0)

/* Define log levels */
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4

/* Set the current log level */
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

/* Define macros for logging */
#define mdb5_dma_dbg(fmt, ...) \
	do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG) \
	    fprintf(stdout, "DEBUG: " fmt, ##__VA_ARGS__); } while (0)

#define mdb5_dma_info(fmt, ...) \
	do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO) \
	    fprintf(stdout, "INFO: " fmt, ##__VA_ARGS__); } while (0)

#define mdb5_dma_warn(fmt, ...) \
	do { if (CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN) \
	    fprintf(stderr, "WARN: " fmt, ##__VA_ARGS__); } while (0)

#define mdb5_dma_err(fmt, ...) \
	    fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)

enum mdb5_dma_io_type {
	MDB5_IO_SYNC = 1,
	MDB5_IO_ASYNC
};

enum mdb5_dma_chan_dir {
	MDB5_CHAN_DIR_TO_DEV = 1,
	MDB5_CHAN_DIR_FROM_DEV
};

enum mdb5_dma_chan_mode {
	MDB5_CHAN_MODE_MM = 1
};

enum mdb5_dma_transfer_mode {
	MDB5_MODE_SG = 0x100,
	MDB5_MODE_SIMPLE = 0x300,
};

/**
 * @enum operation
 * @brief Enumeration of MDB5 operations.
 *
 * This enumeration defines the various operations that can be performed from
 * user space.
 */
enum operation {
	CMD_REG_READ = 0,
	CMD_REG_WRITE,
	CMD_STATS,
	CMD_SET_TRANSFER_MODE,
	CMD_GET_TRANSFER_MODE,
	CMD_SET_APERTURE_MODE,
	CMD_GET_APERTURE_MODE,
	CMD_MAX
};

/**
 * @enum operation_arg
 * @brief Enumeration of MDB5 operation arguments.
 *
 * This enumeration defines the various arguments that can be used with
 * @c enum operation.
 */
enum operation_arg {
	ARG_CHAN_IDX = 0,
	ARG_PCI_BDF,
	ARG_CHAN_DIR,
	ARG_MAX
};

/**
 * This structure contains various statistics related to a MDB5 read/write
 * channels. It includes information about the number of packets received,
 * successfully processed, failed, the number of interrupts received, and
 * the total data length transferred.
 */
struct mdb5_dma_channel_stats {
	uint64_t req_rcvd;
	uint64_t req_success;
	uint64_t req_failed;
	uint64_t intr_rcvd;
	uint64_t read_data_processed;
	uint64_t write_data_processed;
	uint64_t total_data_processed;
};

/**
 * This structure contains information about MDB5 read/write channels.
 * It includes the device memory address, I/O type, Channel's name, index,
 * mode, direction and statistics. Additionally, it contains specific parameters
 * like aperture size for Scatter Gather (SG) DMA mode.
 */
struct mdb5_dma_channel {
	struct mdb5_dma_channel_stats stats;
	uint64_t address; /* Device memory address */
	uint32_t aperture_sz; /* For SG Mode */
	uint8_t ch_id; /* Channel Index */
	char name[MDB5_NAME_SZ]; /* Name of the PF/VF channel node */
	enum mdb5_dma_chan_dir dir;
	enum mdb5_dma_transfer_mode mode;
};

/**
 * This structure contains the offset, data, and PCI Bus, Device, Function
 * in BB:DD.F format
 */
struct mdb5_dma_reg {
	uint32_t offset;
	uint32_t data;
	char pci_bdf[8];
};

/**
 * This structure contains control mode information such as configuring channel
 * to transfer mode Scatter Gather or Simple DMA mode.
 */
struct ctrl_mode {
	char name[MDB5_NAME_SZ];
	enum mdb5_dma_transfer_mode mode;
};

/**
 * This structure contains aperture information. It includes control device node
 * name and aperture size.
 */
struct ctrl_aperture {
	char name[MDB5_NAME_SZ];
	uint32_t  aperture;
};

/**
 * This structure contains channel statistics information.
 * It includes physical address to be provided to kernel for filling statistics
 * @c mdb5_dma_channel_stats
 */
struct ctrl_stats {
	char name[MDB5_NAME_SZ];
	uint64_t  stat_addr;
};

struct mdb5_dma_common;

struct mdb5_dma_ioctl {
	struct ctrl_mode cmode;
	struct ctrl_aperture caperture;
	struct ctrl_stats cstats;
	struct mdb5_dma_common *h;
};

#define MDB5_CTRL_CMD_MAGIC          'm'

#define IOCTL_MDB5_SET_APERTURE_SIZE	\
_IOW(MDB5_CTRL_CMD_MAGIC, 1, struct ctrl_aperture)

#define IOCTL_MDB5_STATS	\
_IOR(MDB5_CTRL_CMD_MAGIC, 2, struct ctrl_stats)

#define IOCTL_MDB5_SET_TRANSFER_MODE	\
_IOW(MDB5_CTRL_CMD_MAGIC, 3, struct ctrl_mode)

#define IOCTL_MDB5_GET_TRANSFER_MODE	\
_IOR(MDB5_CTRL_CMD_MAGIC, 4, struct ctrl_mode)

#define IOCTL_MDB5_GET_APERTURE_SIZE	\
_IOR(MDB5_CTRL_CMD_MAGIC, 5, struct ctrl_aperture)

int mdb5_dma_reg_read(void *);
int mdb5_dma_reg_write(void *);
int mdb5_dma_stats_read(void *);
int mdb5_dma_transfer_mode(void *);
int mdb5_dma_aperture_sz(void *);

/**
 * This structure contains various configuration parameters and settings
 * for MDB5 operations. It includes information about the operation type,
 * channel mode, I/O type, channel direction, and other relevant details
 * required for MDB5 operations.
 * It is commonly used by all MDB5 user space utilities.
 */
struct mdb5_dma_common {
	struct mdb5_dma_ioctl hioc;
	struct mdb5_dma_reg reg;
	struct mdb5_dma_channel *chan;
	uint64_t address;
	size_t pkt_sz;
	uint32_t num_rd_chan;
	uint32_t num_wr_chan;
	uint32_t num_pkts;
	uint32_t op_arg; /* enum operation_arg */
	char ctrl_name[MDB5_NAME_SZ]; /* Name of the PF/VF control node */
	char cfg_name[64]; /* Configuration name for dma-xfer and dma-perf */
	enum operation op;
	enum mdb5_dma_chan_mode chan_mode;
	enum mdb5_dma_io_type io_type;
	bool set_mode;
	bool set_aperture;
	bool dump_en;
};

/* Buffer and MDB5 operations */
ssize_t read_to_buffer(char *, int, char *, size_t, size_t);
ssize_t write_from_buffer(char *, int, char *, size_t, size_t);
int mdb5_dmautils_sync_xfer(char *, enum mdb5_dma_chan_dir, void *, size_t, size_t);
int mdb5_dmautils_async_xfer(char *, enum mdb5_dma_chan_dir,
			 void *, size_t, size_t);
int reg_write_mmap(struct mdb5_dma_reg *);
int reg_read_mmap(struct mdb5_dma_reg *);
void dump_throughput_result(uint64_t, float);

/* MDB5 property and file validation functions */
char *get_io_type(enum mdb5_dma_io_type);
char *get_chan_dir(enum mdb5_dma_chan_dir);
char *get_chan_mode(enum mdb5_dma_chan_mode);
char *get_transfer_mode(enum mdb5_dma_transfer_mode);
bool is_file_available(char *);

/* String and utility functions */
char *strip_blanks(char *, unsigned long *);
char *strip_comments(char *);
uint32_t copy_value(char *, char *, uint32_t);
uint64_t getopt_integer(char *);
void timespec_sub(struct timespec *, struct timespec *);
int arg_read_int(char *s, uint32_t *);
int arg_read_int_ull(char *s, uint64_t *);
void prepare_node_name(char *, char *, uint32_t);
int validate_pkt_size(size_t pkt_sz);
const char *mdb5_dma_strerror(int error_code);

#endif /* _DMA_COMMON_UTILS_H_ */
