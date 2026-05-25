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
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>

#include "dma_common_utils.h"
#include "version.h"

/* Sets the operation argument by applying the mask. */
#define set_operation_arg(x, mask) ((x) |= (1u << (mask)))
/* Clears the operation argument by applying the mask. */
#define clear_operation_arg(x, mask) ((x) &= (~(1u << (mask))))

/**
 * @brief   Function pointer array for processing MDB5 commands.
 *
 * This array contains pointers to functions that process various commands
 * to perform MDB5 operations.
 */
static int32_t (*process_fn[CMD_MAX + 1])(void *arg) = {
	mdb5_dma_reg_read,           /* CMD_REG_READ */
	mdb5_dma_reg_write,          /* CMD_REG_WRITE */
	mdb5_dma_stats_read,         /* CMD_STATS_READ */
	mdb5_dma_transfer_mode,      /* CMD_SET_TRANSFER_MODE */
	mdb5_dma_transfer_mode,      /* CMD_GET_TRANSFER_MODE */
	mdb5_dma_aperture_sz,        /* CMD_SET_APERTURE_MODE */
	mdb5_dma_aperture_sz,        /* CMD_GET_APERTURE_MODE */
	NULL
};

static const char *progname;
static struct option long_options[] = {
	{"channel", required_argument, NULL, 'c'},
	{"dir", required_argument, NULL, 'd'},
	{"set_transfer_mode", required_argument, NULL, 'm'},
	{"get_transfer_mode", no_argument, NULL, 'M'},
	{"bdf", required_argument, NULL, 's'},
	{"reg_read", required_argument, NULL, 'r'},
	{"reg_write", required_argument, NULL, 'w'},
	{"set_aperture_sz", required_argument, NULL, 'a'},
	{"get_aperture_sz", no_argument, NULL, 'A'},
	{"stats", no_argument, NULL, 'S'},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

static char *print_op(enum operation op)
{
	switch (op) {
	case CMD_REG_READ:
		return "--reg_read";
	case CMD_REG_WRITE:
		return "--reg_write";
	case CMD_STATS:
		return "--stats";
	case CMD_SET_TRANSFER_MODE:
		return "--set_transfer_mode";
	case CMD_GET_TRANSFER_MODE:
		return "--get_transfer_mode";
	case CMD_SET_APERTURE_MODE:
		return "--set_aperture_sz";
	case CMD_GET_APERTURE_MODE:
		return "--get_aperture_sz";
	case CMD_MAX:
	default:
		mdb5_dma_err("Invalid operation: \'%c\'\n", op);
		exit(EXIT_FAILURE);
	}
}

static void print_usage(enum operation op)
{
	putchar('\n');
	switch (op) {
	case CMD_REG_READ:
		printf("Register read usage:\n"
		       "%s -r <offset> -s <BB:DD.F in Hex>\n",	progname);
		break;
	case CMD_REG_WRITE:
		printf("Register write usage:\n"
		       "%s -w <offset> <data> -s <BB:DD.F in Hex>\n", progname);
		break;
	case CMD_STATS:
		printf("Channel stats usage:\n"
		       "%s -S -c <channel_index> "
		       "-d <channel_dir: read/write>\n", progname);
		break;
	case CMD_SET_TRANSFER_MODE:
		printf("Set Scatter-Gather / Simple DMA transfer mode usage:\n"
		       "%s -m <sg/simple>"
		       "-c <channel_index> -d <channel dir: read/write>\n",
		       progname);
		break;
	case CMD_GET_TRANSFER_MODE:
		printf("Get transfer mode usage:\n"
		       "%s -M -c <channel_index> "
		       "-d <channel dir: read/write>\n", progname);
		break;
	case CMD_SET_APERTURE_MODE:
		printf("Set aperture mode usage:\n"
		       "%s -a <aperture size> "
		       "-c <channel_index> -d <channel dir: read/write>\n",
			progname);
		break;
	case CMD_GET_APERTURE_MODE:
		printf("Get aperture mode usage:\n"
		       "%s -A -c <channel_index> "
		       "-d <channel dir: read/write>\n", progname);
		break;
	case CMD_MAX:
	default:
		mdb5_dma_err("Invalid operation: \'%c\'\n", op);
		exit(EXIT_FAILURE);
	}
}

static void __attribute__ ((noreturn)) usage(FILE *fp)
{
	fprintf(fp, "\nUsage: %s [OPTIONS]\n", progname);
	fprintf(fp,
		"  -c (--channel) <channel_index>\n"
		"  -d (--dir) <read/write>\n"
		"  -m (--set_transfer_mode) <sg/simple>\n"
		"  -M (--get_transfer_mode)\n"
		"  -s (--bdf) <BB:DD.F in Hex>\n"
		"  -r (--reg_read) <offset>\n"
		"  -w (--reg_write) <offset> <data>\n"
		"  -S (--stats)\n"
		"  -a (--set_aperture_sz) <value>\n"
		"  -A (--get_aperture_sz)\n"
		"  -v (--version)\n"
		"  -h (--help)\n");

	print_usage(CMD_REG_READ);
	print_usage(CMD_REG_WRITE);
	print_usage(CMD_STATS);
	print_usage(CMD_SET_TRANSFER_MODE);
	print_usage(CMD_GET_TRANSFER_MODE);
	print_usage(CMD_SET_APERTURE_MODE);
	print_usage(CMD_GET_APERTURE_MODE);
	exit(fp == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**
 * @brief   Sets the new operation if no previous operation is set.
 *
 * This function checks if a previous operation is already set. If not,
 * it sets the new operation. If a previous operation is already set,
 * it logs an error and returns false.
 *
 * @param   cur Pointer to the current operation.
 *
 * @param   new The new operation to be set.
 *
 * @return  true if the new operation is set successfully,
 *          false if a previous operation is already set.
 */
static inline bool set_operation(enum operation *cur, enum operation new)
{
	if (*cur != CMD_MAX) {
		mdb5_dma_err("Multiple operations selected \"%s\" and \"%s\"\n",
			     print_op(*cur), print_op(new));
		return false;
	}
	*cur = new;

	return true;
}

/**
 * @brief   Parses command-line options for MDB5 operations.
 *
 * This function processes the command-line arguments to set the appropriate
 * MDB5 operation and its arguments. It validates the input parameters and
 * sets the operation mode and arguments accordingly. If invalid parameters
 * are detected, it reports errors and provides usage information.
 *
 * @param   argc The number of command-line arguments.
 *
 * @param   argv The array of command-line arguments.
 *
 * @param   h Pointer to the @c mdb5_dma_common structure to store the parsed
 *          parameters.
 *
 * @return  0 on success, -EINVAL on invalid parameters.
 */
static int parse_options(int argc, char *argv[], struct mdb5_dma_common *h)
{
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	struct mdb5_dma_reg *reg = &h->reg;
	struct mdb5_dma_channel *chan = h->chan;
	int ret = 0;

	h->op = CMD_MAX;
	h->op_arg = 0;
	progname = argv[0];

	if (argc == 1)
		usage(stdout);

	while (true) {
		int option_index = -1;
		int c = getopt_long(argc, argv, "c:d:m:s:r:w:a:AMShv",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		/* Control Operations */
		case 'w':
			if (!set_operation(&h->op, CMD_REG_WRITE))
				goto usage_exit;

			if (optind >= argc || (*argv[optind] == '-')) {
				mdb5_dma_err("Missing/Invalid data for register "
					     "write operation: \"%s\"\n",
					     argv[optind]);
				print_usage(CMD_REG_WRITE);
				ret = -EINVAL;
				goto error;
			}

			reg->offset = getopt_integer(optarg);
			reg->data = getopt_integer(argv[optind]);
			optind++; // Skip the next argument
			break;
		case 'r':
			if (!set_operation(&h->op, CMD_REG_READ))
				goto usage_exit;

			reg->offset = getopt_integer(optarg);
			break;
		case 'm':
			if (!set_operation(&h->op, CMD_SET_TRANSFER_MODE))
				goto usage_exit;

			if (!strcmp(optarg, "sg")) {
				hioc->cmode.mode = MDB5_MODE_SG;
			} else if (!strcmp(optarg, "simple")) {
				hioc->cmode.mode = MDB5_MODE_SIMPLE;
			} else {
				mdb5_dma_err("Invalid transfer mode: "
					     "\"%s\"\n", optarg);
				print_usage(CMD_SET_TRANSFER_MODE);
				ret = -EINVAL;
				goto error;
			}
			h->set_mode = true;
			break;
		case 'M':
			if (!set_operation(&h->op, CMD_GET_TRANSFER_MODE))
				goto usage_exit;
			h->set_mode = false;
			break;
		case 'a':
			if (!set_operation(&h->op, CMD_SET_APERTURE_MODE))
				goto usage_exit;

			hioc->caperture.aperture = getopt_integer(optarg);
			if (is_negative((int)hioc->caperture.aperture) ||
			    (!(int)hioc->caperture.aperture)) {
				mdb5_dma_err("Invalid aperture size: %s\n", optarg);
				ret = -EINVAL;
				goto error;
			}
			h->set_aperture = true;
			break;
		case 'A':
			if (!set_operation(&h->op, CMD_GET_APERTURE_MODE))
				goto usage_exit;
			h->set_aperture = false;
			break;
		case 'S':
			if (!set_operation(&h->op, CMD_STATS))
				goto usage_exit;
			break;
		/* Control Operations Arguments*/
		case 'c':
			set_operation_arg(h->op_arg, ARG_CHAN_IDX);
			chan->ch_id = getopt_integer(optarg);
			if (is_negative((int8_t)chan->ch_id)) {
				mdb5_dma_err("Invalid channel index: %s\n",
					     optarg);
				ret = -EINVAL;
				goto error;
			}
			if (chan->ch_id >= MDB5_MAX_RD_CHAN ||
			    chan->ch_id >= MDB5_MAX_WR_CHAN) {
				mdb5_dma_err("Channel index: %s is not in range "
					     "RD: 0-%d, WR: 0-%d)\n",
					     optarg, MDB5_MAX_RD_CHAN - 1,
					     MDB5_MAX_WR_CHAN - 1);
				ret = -EINVAL;
				goto error;
			}
			break;
		case 'd':
			set_operation_arg(h->op_arg, ARG_CHAN_DIR);
			if (!strncmp(optarg, "write", 5)) {
				chan->dir = MDB5_CHAN_DIR_TO_DEV;
			} else if (!strncmp(optarg, "read", 4)) {
				chan->dir = MDB5_CHAN_DIR_FROM_DEV;
			} else {
				mdb5_dma_err("Invalid direction: \"%s\", expected "
					     "read/write\n", optarg);
				ret = -EINVAL;
				goto error;
			}
			break;
		case 's':
			set_operation_arg(h->op_arg, ARG_PCI_BDF);
			memcpy(reg->pci_bdf, optarg, sizeof(reg->pci_bdf));
			break;
		case 'v':
			printf("%s version %s\n", PROGNAME, VERSION);
			printf("%s\n", COPYRIGHT);
			exit(EXIT_SUCCESS);
		case 'h':
			usage(stdout);
		case 0:
			/* long option */
		case '?':
		default:
			usage(stderr);
		}
	}

	if (h->op == CMD_MAX)
		goto usage_exit;

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s, ", argv[optind++]);
		printf("ignoring..\n");
	}

error:
	return ret;
usage_exit:
	usage(stderr);
}

/**
 * @brief   Validates the input parameters for MDB5 operations.
 *
 * Ensures that the parameters are set correctly for the specified
 * operation and mode, and adjusts or reports errors as needed. This is
 * achieved using the @c set_operation_arg for specific @c enum operation.
 * Both are provided by user through commandline.
 * (e.g., dma-ctl -m sg -i 0 -c 0 -d write).
 *
 * @param   h Pointer to the mdb5_dma_common structure containing the parameters
 *          to validate.
 *
 * @return  0 on success,
 *          -EINVAL on invalid parameters or if required arguments are not
 *          provided for specific operation
 *
 * The function performs the following checks:
 * - Validates if required arguments are provided for each operation.
 * - Prepares node names for control and channel structures if needed.
 * - Checks if transfer mode is configured to Scatter Gather DMA mode before
 *   configuring the aperture size
 * - Logs information for aperture mode.
 */
static int validate_input_params(struct mdb5_dma_common *h)
{
	struct mdb5_dma_channel *chan = h->chan;
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	uint32_t op_arg = 0;
	enum operation op = h->op;
	int ret = 0;

	switch (op) {
	case CMD_REG_READ:
	case CMD_REG_WRITE:
		set_operation_arg(op_arg, ARG_PCI_BDF);
		break;
	case CMD_STATS:
	case CMD_SET_TRANSFER_MODE:
	case CMD_GET_TRANSFER_MODE:
	case CMD_SET_APERTURE_MODE:
	case CMD_GET_APERTURE_MODE:
		set_operation_arg(op_arg, ARG_CHAN_IDX);
		set_operation_arg(op_arg, ARG_CHAN_DIR);
		break;
	case CMD_MAX:
	default:
		mdb5_dma_err("Invalid operation: \'%c\'\n", op);
		ret = -EINVAL;
		goto error;
	}

	if (h->op_arg != op_arg) {
		mdb5_dma_err("Invalid arguments (0x%x!=0x%x) for operation \"%s\"\n",
			     h->op_arg, op_arg, print_op(op));
		print_usage(op);
		ret = -EINVAL;
		goto error;
	}

	if (op > CMD_REG_WRITE) {
		prepare_node_name(h->ctrl_name, "ctrl", 0);
		if (!is_file_available(h->ctrl_name)) {
			ret = -ENOENT;
			goto error;
		}

		prepare_node_name(chan->name, get_chan_dir(chan->dir),
				  chan->ch_id);
		if (!is_file_available(chan->name)) {
			ret = -ENOENT;
			goto error;
		}
	}

	if (op == CMD_SET_APERTURE_MODE || op == CMD_GET_APERTURE_MODE) {
		hioc->h = h;
		memcpy(&hioc->cmode.name, &chan->name, MDB5_NODE_CHAN_SZ);

		/*
		 * Aperture mode is applicable to SG transfer mode only.
		 * Make sure transfer mode SG is configured on channel
		 * before setting aperture size.
		 */
		h->set_mode = false;
		mdb5_dma_transfer_mode(hioc);

		if (hioc->cmode.mode != MDB5_MODE_SG) {
			mdb5_dma_err("Aperture size cannot be %s.\n"
				     "Make sure transfer mode on channel %s "
				     "is configured to \"sg\"\n",
				     h->set_aperture ? "configured" : "retrieved",
				     chan->name);
			ret = -EINVAL;
			goto error;
		}
	}

error:
	return ret;
}

/**
 * @brief   Processes a command based on the operation in mdb5_dma_common.
 *
 * Handles operations like register read/write, dumping channel stats,
 * setting transfer mode, and aperture size for a specific channel.
 * Prepares the appropriate argument based on the command and calls
 * the corresponding processing function.
 *
 * @param   h is a pointer to the mdb5_dma_common instance.
 *
 * @return  It is set accordingly as corresponding process function's return,
 *	    -EOPNOTSUPP if unsupported.
 */
static int process_cmd(struct mdb5_dma_common *h)
{
	struct mdb5_dma_ioctl *hioc = &h->hioc;
	struct mdb5_dma_reg *reg = &h->reg;
	struct mdb5_dma_channel *chan = h->chan;
	void *arg;

	switch (h->op) {
	case CMD_REG_READ:
	case CMD_REG_WRITE:
		arg = (void *)reg;
		break;
	case CMD_STATS:
		hioc->h = h;
		memcpy(&hioc->cstats.name, &chan->name, MDB5_NODE_CHAN_SZ);
		arg = (void *)hioc;
		break;
	case CMD_SET_TRANSFER_MODE:
	case CMD_GET_TRANSFER_MODE:
		hioc->h = h;
		memcpy(&hioc->cmode.name, &chan->name, MDB5_NODE_CHAN_SZ);
		arg = (void *)hioc;
		break;
	case CMD_SET_APERTURE_MODE:
	case CMD_GET_APERTURE_MODE:
		hioc->h = h;
		memcpy(&hioc->caperture.name, &chan->name, MDB5_NODE_CHAN_SZ);
		arg = (void *)hioc;
		break;
	case CMD_MAX:
	default:
		mdb5_dma_err("No function requested\n");
		usage(stderr);
	}

	if (process_fn[h->op])
		return process_fn[h->op](arg);

	return -EOPNOTSUPP;
}

int main(int argc, char *argv[])
{
	int ret;
	struct mdb5_dma_common h;
	struct mdb5_dma_channel chan;

	memset(&h, 0, sizeof(struct mdb5_dma_common));
	memset(&chan, 0, sizeof(struct mdb5_dma_channel));

	h.chan = &chan;
	ret = parse_options(argc, argv, &h);
	if (ret < 0)
		return ret;

	ret = validate_input_params(&h);
	if (ret < 0)
		return ret;

	ret = process_cmd(&h);
	if (ret < 0)
		return ret;

	return 0;
}
