/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#ifndef _Q_CONF_H_
#define _Q_CONF_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/atomic.h>

enum pci_dev_type {
	/** device type is VF */
	PCI_TYPE_VF,
	/** device type is PF */
	PCI_TYPE_PF,
	/** unknown device type */
	PCI_TYPE_INVAL
};

enum q_cfg_state {
	/** initial state no qmax cfg done */
	Q_INITIAL,
	/** configured state qmax cfg already done */
	Q_CONFIGURED,
	/** unknown configuration state */
	Q_INVALID
};

/* for free entry management */
struct free_entry {
	/** to connect to free_list */
	struct list_head list_head;
	/** next available qbase */
	u32 next_qbase;
	/** free qs available in this entry */
	u32 free;
	/** index of the entry */
	u32 index;
};

/** q configuration entry */
struct qconf_entry {
	/** to connect to list head of */
	struct list_head list_head;
	/** idx of the device */
	u32 idx;
	/** qbase for func_id */
	u32 qbase;
	/** qmax for func_id */
	u32 qmax;
	/** current configuration state */
	enum q_cfg_state cfg_state;
	/** device type PF/VF */
	enum pci_dev_type type;
	/** func_id of the device */
	u8 func_id;
};

/** for hodling the qconf_entry structure */
struct qconf_entry_head {
	/** for holding vf qconf_entry */
	struct list_head vf_list;
	/** for holding vf free_entry */
	struct list_head vf_free_list;
	/** for holding pf qconf_entry */
	struct list_head pf_list;
	/** for maximum qs for vf */
	u32 vf_qmax;
	/** for maximum qs for pf */
	u32 pf_qmax;
	/** for holding vf qconf_entry */
	u32 vf_qbase;
	/** number of vfs attached to all pfs */
	atomic_t vf_cnt;
	/** free count of qs which can be configured */
	u32 qcnt_cfgd_free;
	/** number of qs free for initial cfg devices */
	u32 qcnt_init_free;
	/** used by INITIAL state devices */
	u32 qcnt_init_used;
};

/*****************************************************************************/
/**
 * xdev_set_qmax() - sets the qmax value of VF device from virtfn/qdma/qmax
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 * @param[in]	qmax:		qmax value to set
 * @param[out]	qbase:		pointer to return qbase value
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_set_qmax(u32 xdev, u8 func_id, u32 qmax, u32 *qbase);

/*****************************************************************************/
/**
 * xdev_del_qconf - delete the q configuration entry of funcid
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_del_qconf(u32 xdev, u8 func_id);

/*****************************************************************************/
/**
 * xdev_create_qconf - add the q configuration entry of funcid during hello
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
struct qconf_entry *xdev_create_qconf(u32 xdev, u8 func_id);

/*****************************************************************************/
/**
 * xdev_destroy_qconf - destroys the qconf entry during bye
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_destroy_qconf(u32 xdev, u8 func_id);

/*****************************************************************************/
/**
 * xdev_qconf_init - initialize the q configuration structures
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int xdev_qconf_init(void);

/*****************************************************************************/
/**
 * xdev_qconf_cleanup - cleanup the q configuration
 *
 * @return
 *****************************************************************************/
void xdev_qconf_cleanup(void);

/*****************************************************************************/
/**
 * xdev_dump_freelist - dump the freelist
 *
 * @return
 *****************************************************************************/
void xdev_dump_freelist(void);

/*****************************************************************************/
/**
 * xdev_dump_qconf- dump the q configuration
 * @param[in]	xdev:	device type to be used PF/VF
 *
 * @return
 *****************************************************************************/
void xdev_dump_qconf(u32 xdev);

/*****************************************************************************/
/**
 * set_vf_qmax	- set the value for vf_qmax;
 *
 * @param[in]	qmax:	qmax value to set
 *
 * @return: last set qmax value before qmax.
 *****************************************************************************/
int set_vf_qmax(u32 qmax);

/*****************************************************************************/
/**
 * qconf_get_qmax	- get the current qmax value of VF/PF
 *
 * @param[in]	xdev:		device type to be used PF/VF
 * @param[in]	card_id:	for differntiation the qdma card
 *
 * @return	0: success
 * @return	<0: failure
 *****************************************************************************/
int qconf_get_qmax(u32 xdev, u32 card_id);

/*****************************************************************************/
/**
 * is_vfqmax_configurable	- checks if the vfqmax is configurable.
 *
 * @param[in]	xdev:		list type to be used PF/VF
 * @param[in]	func_id:	func id to set the qmax
 *
 * @return	0: if not configurable
 * @return	1: if configurable
 *****************************************************************************/
int is_vfqmax_configurable(void);

#endif
