/*
 * This file is part of the XVSEC driver for Linux
 *
 * Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "xvsec_util.h"
#include "xvsec_drv.h"
#include "xvsec_mcap.h"
#include "xvsec_drv_int.h"
#include "xvsec_mcap_spartan.h"

/** @brief Forward declaration: Program a PDI file via MCAP V3. */
static int xvsec_mcapv3_program(struct vsec_context *mcap_ctx, char *fname, enum data_transfer_mode tr_mode);
/** @brief Forward declaration: Write PDI data to the MCAP write data register. */
static int xvsec_write_pdi(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size, enum data_transfer_mode tr_mode);

/**
 * @brief Process a user-provided file for MCAP V3 programming.
 *
 * Copies the file name from user space, validates it as a PDI file,
 * and initiates the MCAP V3 programming sequence.
 *
 * @param[in] mcap_ctx      Pointer to the VSEC context structure.
 * @param[in] user_file_ptr User-space pointer to the file name string.
 * @param[in] tr_mode       Data transfer mode (fast or slow).
 * @param[in] file_type     Descriptive string for error logging.
 * @return 0 on success, negative error code on failure.
 */
static int xvsec_mcapv3_process_file(struct vsec_context *mcap_ctx,
				   char __user *user_file_ptr,
				   enum data_transfer_mode tr_mode,
				   const char *file_type)
{
	int ret;
	char bitfile[MAX_FLEN];
	uint16_t len;

	len = strnlen_user(user_file_ptr, MAX_FLEN);
	if (len > MAX_FLEN) {
		pr_err("File Name too long\n");
		return -(EINVAL);
	}

	ret = strncpy_from_user(bitfile, user_file_ptr, len);
	if (ret < 0) {
		pr_err("File Name Copy Failed\n");
		return ret;
	}

	/** At present only PDI file format is implemented */
	if (xvsec_util_find_file_type(bitfile, MCAPV3_PDI_FILE) < 0) {
		pr_err("Only PDI files supported for a while\n");
		return -(EINVAL);
	}

	ret = xvsec_mcapv3_program(mcap_ctx, bitfile, tr_mode);
	if (ret < 0) {
		pr_err("[xvsec_mcap] : xvsec_mcapv3_program failed for %s with err : %d\n",
		       file_type, ret);
	}

	return ret;
}

/**
 * @brief Toggle the read enable bit in the MCAP V3 control register.
 *
 * Performs a read-modify-write sequence to pulse the READ_ENABLE bit
 * (and optionally CTRL_ENABLE) high then low. This toggle is required
 * before reading certain MCAP registers (e.g., CONFIG_REG).
 *
 * @param[in] mcap_ctx       Pointer to the VSEC context structure.
 * @param[in] wr_mode_active If true, only toggle READ_ENABLE
 *                            (CTRL_ENABLE is already asserted by the
 *                            write/programming path).
 *                            If false, toggle both CTRL_ENABLE and
 *                            READ_ENABLE (standalone read path).
 * @return 0 on success, negative PCI error code on failure.
 */

static int mcap_read_enable_toggle(struct vsec_context *mcap_ctx,
                   bool wr_mode_active)
{
    int ret;
    struct pci_dev *pdev = mcap_ctx->pdev;
   uint16_t ctrl_offset;
    uint32_t ctrl_data;
    uint32_t toggle_bits;

    ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;

    /* Determine which bits to toggle */
    if (wr_mode_active)
        toggle_bits = XVSEC_MCAPV3_READ_ENABLE;
    else
        toggle_bits = XVSEC_MCAPV3_CTRL_ENABLE |
                  XVSEC_MCAPV3_READ_ENABLE;

    /* Step 1: Read current control register value */
    ret = pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
    if (ret != 0) {
        pr_err("%s: PCI read failed (ret=%d)\n", __func__, ret);
        return ret;
    }

    /* Step 2: Assert the toggle bits */
    ctrl_data |= toggle_bits;
    ret = pci_write_config_dword(pdev, ctrl_offset, ctrl_data);
    if (ret != 0) {
        pr_err("%s: PCI write (set) failed (ret=%d)\n", __func__, ret);
        return ret;
    }
    udelay(2);

    /* Step 3: De-assert the toggle bits */
    ctrl_data &= ~toggle_bits;
    ret = pci_write_config_dword(pdev, ctrl_offset, ctrl_data);
    if (ret != 0) {
        pr_err("%s: PCI write (clear) failed (ret=%d)\n", __func__, ret);
        return ret;
    }
    udelay(2);

    return 0;
}

/**
 * @brief Check if MCAP V3 is enabled on the Spartan UltraScale+ device.
 *
 * Reads the configuration register and checks the eFuse disable
 * and MCAP disable bits to determine if MCAP operations are allowed.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 if MCAP is enabled, -EPERM if offset is invalid,
 *         -EBUSY if MCAP is disabled.
 */
static int xvsec_mcapv3_is_enabled(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint32_t cnfg_data = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;

	if (mcap_ctx->vsec_offset == INVALID_OFFSET)
		return -(EPERM);

	ret = mcap_read_enable_toggle(mcap_ctx, 0);
	if (ret < 0)
		return ret;
	pci_read_config_dword(pdev, mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG, &cnfg_data);

	if ((cnfg_data & XVSEC_MCAPV3_CNFG_EFUSE) != 0x0) {
		pr_err("Efuse MCAP Disabled can't perform any MCAP operations (cnfg_data=0x%08X)\n", cnfg_data);
		ret = -(EBUSY);
		goto err;
	}
	if ((cnfg_data & XVSEC_MCAPV3_CNFG_DISABLE)  != 0x0) {
		pr_err("MCAP Disabled can't perform any MCAP operations (cnfg_data=0x%08X)\n", cnfg_data);
		ret = -(EBUSY);
		goto err;
	}

err:
	return ret;
}

/**
 * @brief Request FPGA configuration access via MCAP V3.
 *
 * Checks the status register for existing access and, if needed,
 * sets the enable and request-access bits in the control register.
 * Polls until access is granted or a timeout occurs.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] restore   Pointer to store the original control register
 *                        value for later restoration.
 * @return 0 on success, -EPERM if offset is invalid, -EBUSY on timeout.
 */
static int xvsec_mcapv3_req_access(struct vsec_context *mcap_ctx,
	uint32_t *restore)
{
	int ret = 0;
	int delay = XVSEC_MCAPV3_LOOP_COUNT;
	uint16_t ctrl_offset, sts_offset;
	uint32_t ctrl_data, sts_data;
	struct pci_dev *pdev = mcap_ctx->pdev;

	if (mcap_ctx->vsec_offset == INVALID_OFFSET)
		return -(EPERM);

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;
	sts_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_STATUS_REG;

	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);
	*restore = ctrl_data;

	pci_read_config_dword(pdev, sts_offset, &sts_data);

	if ((sts_data & XVSEC_MCAPV3_STATUS_ACCESS) != 0x0) {

		ctrl_data = ctrl_data |
			(XVSEC_MCAPV3_CTRL_ENABLE | XVSEC_MCAPV3_CTRL_REQ_ACCESS);

		pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

		for (; delay > 0; --delay) {
			pci_read_config_dword(pdev, sts_offset, &sts_data);
			if (!(sts_data & XVSEC_MCAPV3_STATUS_ACCESS))
				break;
		}

		if (delay == 0) {
			pr_err("Unable to get the FPGA CFG Access\n");
			ctrl_data &= ~(XVSEC_MCAPV3_CTRL_ENABLE |
					XVSEC_MCAPV3_CTRL_REQ_ACCESS);
			pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

			ret = -(EBUSY);
		}
	}

	return ret;
}

/**
 * @brief Perform MCAP V3 reset on a Spartan UltraScale+ device.
 *
 * Acquires FPGA configuration access, asserts the MCAP reset bit,
 * and polls the configuration register until the reset completes.
 * Restores the original control register value afterwards.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	int delay = XVSEC_MCAPV3_LOOP_COUNT;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	/* Acquire the Access */
	ret = xvsec_mcapv3_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Reset */
	ctrl_data = ctrl_data | XVSEC_MCAPV3_CTRL_RESET |
		XVSEC_MCAPV3_CTRL_ENABLE | XVSEC_MCAPV3_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data & XVSEC_MCAPV3_CTRL_RESET) == 0x0) {
		pr_err("MCAP reset bit not asserted in control register\n");
		ret = -(EBUSY);
		goto err;
	}

	status_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG;
	do {
		ret = mcap_read_enable_toggle(mcap_ctx, 0);
		if (ret < 0) {
			pr_err("MCAP reset: read-enable toggle failed (ret=%d)\n", ret);
			goto err;
		}
		pci_read_config_dword(pdev, status_offset, &status_data);
		if ((status_data & XVSEC_MCAPV3_CNFG_RESET) == 0x0)
			break;
		delay = delay - 1;
	} while (delay != 0);

	if (delay == 0) {
		pr_err("MCAP reset did not complete\n");
		ret = -(EBUSY);
	}

err:
	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

/**
 * @brief Perform MCAP V3 module reset on a Spartan UltraScale+ device.
 *
 * Acquires FPGA configuration access and asserts the module reset bit
 * in the MCAP control register. Verifies the reset was applied by
 * reading back the control register.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, -EBUSY if module reset was not asserted,
 *         negative error code on other failures.
 */
int xvsec_mcapv3_module_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint16_t ctrl_offset;
	uint32_t ctrl_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	ret = xvsec_mcapv3_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Module Reset */
	ctrl_data = ctrl_data | XVSEC_MCAPV3_CTRL_MOD_RESET |
		XVSEC_MCAPV3_CTRL_ENABLE | XVSEC_MCAPV3_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data & XVSEC_MCAPV3_CTRL_MOD_RESET) == 0x0)
		ret = -(EBUSY);

	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

/**
 * @brief Perform a full MCAP V3 reset (MCAP + module) on a Spartan UltraScale+ device.
 *
 * Acquires FPGA configuration access, asserts both the MCAP reset and
 * module reset bits, and polls the configuration register until the
 * reset sequence completes. Restores the original control register value.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, -EBUSY if reset did not complete or was not
 *         asserted, negative error code on other failures.
 */
int xvsec_mcapv3_full_reset(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	int delay = XVSEC_MCAPV3_LOOP_COUNT;
	uint16_t ctrl_offset, status_offset;
	uint32_t ctrl_data, status_data, read_data, restore;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	/* Acquire the Access */
	ret = xvsec_mcapv3_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	/* Asserting the Full Reset */
	ctrl_data = ctrl_data |
		XVSEC_MCAPV3_CTRL_RESET | XVSEC_MCAPV3_CTRL_MOD_RESET |
		XVSEC_MCAPV3_CTRL_ENABLE | XVSEC_MCAPV3_CTRL_REQ_ACCESS;
	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);

	/* Read Back */
	pci_read_config_dword(pdev, ctrl_offset, &read_data);
	if ((read_data &
		(XVSEC_MCAPV3_CTRL_RESET | XVSEC_MCAPV3_CTRL_MOD_RESET)) == 0x0) {
		pr_err("MCAP full reset bits not asserted in control register\n");
		ret = -(EBUSY);
		goto err;
	}

	status_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG;
	do {
		ret = mcap_read_enable_toggle(mcap_ctx, 0);
		if (ret < 0) {
			pr_err("MCAP full reset: read-enable toggle failed (ret=%d)\n", ret);
			goto err;
		}
		pci_read_config_dword(pdev, status_offset, &status_data);
		if ((status_data & XVSEC_MCAPV3_CNFG_RESET) == 0x0)
			break;
		delay = delay - 1;
	} while (delay != 0);

	if (delay == 0) {
		pr_err("MCAP reset did not complete\n");
		ret = -(EBUSY);
	}

err:
	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}

/**
 * @brief Get MCAP data registers (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Array of 4 uint32_t (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_get_data_regs(struct vsec_context *mcap_ctx, uint32_t regs[4])
{
	pr_err("get_data_regs is not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}


/**
 * @brief Read all MCAP V3 registers for a Spartan UltraScale+ device.
 *
 * Iterates through the MCAP V3 register space and reads each
 * 32-bit register from PCI configuration space. Performs a
 * read-enable toggle before reading the configuration register.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Pointer to mcap_regs union to store the
 *                        register values. v3.valid is set to 1 on success.
 * @return 0 on success, -EPERM if offset is invalid,
 *         negative error code on other failures.
 */
int xvsec_mcapv3_get_regs(
	struct vsec_context *mcap_ctx, union mcap_regs *regs)
{
	int ret = 0;
	uint16_t mcap_offset, index;
	uint32_t *ptr;
	struct pci_dev *pdev = mcap_ctx->pdev;

	regs->v3.valid = 0;

	mcap_offset = mcap_ctx->vsec_offset;
	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	if (mcap_offset == INVALID_OFFSET)
		return -(EPERM);

	ptr = &regs->v3.ext_cap_header;
	for (index = 0; index <= (XVSEC_MCAPV3_CONFIG_REG / 4); index++) {

		if (mcap_offset == mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG) {
			ret = mcap_read_enable_toggle(mcap_ctx, 0);
			if (ret < 0)
				return ret;
		}

		pci_read_config_dword(pdev, mcap_offset, &ptr[index]);
		mcap_offset += 4;
	}

	regs->v3.valid = 1;

	return 0;
}

/**
 * @brief Get FPGA configuration registers (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Pointer to fpga_cfg_regs union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_get_fpga_regs(
	struct vsec_context *mcap_ctx, union fpga_cfg_regs *regs)
{
	pr_err("FPGA read config register not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Program a full bitstream via MCAP V3 on a Spartan UltraScale+ device.
 *
 * Performs the complete MCAP programming sequence:
 * 1. Validates input files and transfer mode
 * 2. Checks MCAP is enabled
 * 3. Acquires FPGA configuration access
 * 4. Checks for existing configuration engine errors
 * 5. Enables MCAP write mode and waits for Spartan UltraScale+ mode readiness
 * 6. Programs partial clear file (if provided)
 * 7. Programs bitstream file (if provided)
 * 8. Sets CFG_SWITCH bit on success
 *
 * @param[in]     mcap_ctx   Pointer to the VSEC context structure.
 * @param[in,out] bit_files  Pointer to bitstream_file_v3 union containing
 *                            partial_clr_file, bitstream_file, tr_mode,
 *                            and output status field.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_program_bitstream_full(
	struct vsec_context *mcap_ctx, union bitstream_file_v3 *bit_files)
{
	int ret = 0;
	int delay = XVSEC_MCAPV3_LOOP_COUNT;
	uint16_t ctrl_offset;
	uint32_t ctrl_data, restore;
	uint16_t cnfg_offset;
	uint32_t cnfg_data;
	enum data_transfer_mode tr_mode;
	struct pci_dev *pdev = mcap_ctx->pdev;

	bit_files->v3.status = MCAP_BITSTREAM_PROGRAM_FAILURE;

	if ((bit_files->v3.partial_clr_file == NULL) &&
			(bit_files->v3.bitstream_file == NULL)) {
		pr_err("Both Bit files are NULL\n");
		return -(EINVAL);
	}

	if (bit_files->v3.tr_mode > DATA_TRANSFER_MODE_SLOW) {
		pr_err("%s: Invalid Transfer mode : %d\n", __func__, bit_files->v3.tr_mode);
		return -(EINVAL);
	}

	tr_mode = bit_files->v3.tr_mode;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	/* Acquire the Access */
	ret = xvsec_mcapv3_req_access(mcap_ctx, &restore);
	if (ret < 0)
		return ret;

	cnfg_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG;
	ret = mcap_read_enable_toggle(mcap_ctx, 1);
	if (ret < 0) {
		pr_err("MCAP program: read-enable toggle failed before error check (ret=%d)\n", ret);
		goto CLEANUP;
	}
	pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);
	if ((cnfg_data & XVSEC_MCAPV3_CNFG_PMCL_ERR ) != 0x0) {
		pr_err("MCAP Config Engine Error\n");
		xvsec_mcapv3_full_reset(mcap_ctx);
		ret = -(EIO);
		goto CLEANUP;
	}

	ctrl_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONTROL_REG;
	pci_read_config_dword(pdev, ctrl_offset, &ctrl_data);

	ctrl_data = ctrl_data | XVSEC_MCAPV3_CTRL_WR_ENABLE |
			XVSEC_MCAPV3_CTRL_ENABLE | XVSEC_MCAPV3_CTRL_REQ_ACCESS;

	ctrl_data = ctrl_data &
			~(XVSEC_MCAPV3_CTRL_RESET | XVSEC_MCAPV3_CTRL_MOD_RESET |
				XVSEC_MCAPV3_CTRL_CFG_SWICTH);

	pci_write_config_dword(pdev, ctrl_offset, ctrl_data);
	ret = mcap_read_enable_toggle(mcap_ctx, 1);
	if (ret < 0) {
		pr_err("MCAP program: read-enable toggle failed after write enable (ret=%d)\n", ret);
		goto CLEANUP;
	}
	pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);

	for (; delay > 0; --delay) {
		if (cnfg_data & XVSEC_MCAPV3_CNFG_LMODE)
			break;
		ret = mcap_read_enable_toggle(mcap_ctx, 1);
		if (ret < 0) {
			pr_err("MCAP program: read-enable toggle failed during Spartan UltraScale+ mode poll (ret=%d)\n", ret);
			goto CLEANUP;
		}
		pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);
	}

	if (delay == 0) {
		pr_err("MCAP: Config Engine Not Ready\n");
		goto CLEANUP;
	}

	/* Process partial clear file if provided */
	if (bit_files->v3.partial_clr_file != NULL) {
		ret = xvsec_mcapv3_process_file(mcap_ctx,
				bit_files->v3.partial_clr_file,
				tr_mode, "Clear File");
		if (ret < 0)
			goto CLEANUP;
	}

	/* Process bitstream file if provided */
	if (bit_files->v3.bitstream_file != NULL) {
		ret = xvsec_mcapv3_process_file(mcap_ctx,
				bit_files->v3.bitstream_file,
				tr_mode, "Bit File");
		if (ret < 0)
			goto CLEANUP;
	}

	/* Set CFG_SWITCH bit if bitstream file was processed */
	if (bit_files->v3.bitstream_file != NULL)
		restore = restore | XVSEC_MCAPV3_CTRL_CFG_SWICTH;

	if (ret == 0)
		bit_files->v3.status = MCAP_BITSTREAM_PROGRAM_SUCCESS;
CLEANUP:
	pci_write_config_dword(pdev, ctrl_offset, restore);

	return ret;
}


/**
 * @brief Program a raw bitstream via MCAP V3 on a Spartan UltraScale+ device.
 *
 * Programs the partial clear and/or bitstream files in raw mode
 * without performing MCAP access request, enable checks, or
 * control register setup.
 *
 * @param[in]     mcap_ctx   Pointer to the VSEC context structure.
 * @param[in,out] bit_files  Pointer to bitstream_file_v3 union containing
 *                            file paths, transfer mode, and output status.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_program_bitstream_raw(
	struct vsec_context *mcap_ctx, union bitstream_file_v3 *bit_files)
{
	int ret = 0;
	enum data_transfer_mode tr_mode;

	bit_files->v3.status = MCAP_BITSTREAM_PROGRAM_FAILURE;

	if ((bit_files->v3.partial_clr_file == NULL) &&
			(bit_files->v3.bitstream_file == NULL)) {
		pr_err("Both Bit files are NULL\n");
		return -(EINVAL);
	}

	if (bit_files->v3.tr_mode > DATA_TRANSFER_MODE_SLOW) {
		pr_err("%s: Invalid Transfer mode : %d\n", __func__, bit_files->v3.tr_mode);
		return -(EINVAL);
	}

	tr_mode = bit_files->v3.tr_mode;

	/* Process partial clear file if provided */
	if (bit_files->v3.partial_clr_file != NULL) {
		ret = xvsec_mcapv3_process_file(mcap_ctx,
				bit_files->v3.partial_clr_file,
				tr_mode, "Clear File");
		if (ret < 0)
			goto CLEANUP;
	}

	/* Process bitstream file if provided */
	if (bit_files->v3.bitstream_file != NULL) {
		ret = xvsec_mcapv3_process_file(mcap_ctx,
				bit_files->v3.bitstream_file,
				tr_mode, "Bit File");
		if (ret < 0)
			goto CLEANUP;
	}

	if (ret == 0)
		bit_files->v3.status = MCAP_BITSTREAM_PROGRAM_SUCCESS;

CLEANUP:
	return ret;
}

/**
 * @brief Read an MCAP V3 configuration address on a Spartan UltraScale+ device.
 *
 * Reads a byte ('b'), half-word ('h'), or word ('w') from the
 * specified offset within the MCAP VSEC configuration space.
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] data      Pointer to cfg_data union with offset,
 *                           access type, and storage for read data.
 * @return 0 on success, -EINVAL for invalid access type,
 *         negative error code on other failures.
 */
int xvsec_mcapv3_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	/*
	 * The CONFIG register requires a read-enable toggle to latch
	 * fresh hardware values before reading. Without this, the read
	 * returns stale data.
	 */
	if (data->v3.offset == XVSEC_MCAPV3_CONFIG_REG) {
		ret = mcap_read_enable_toggle(mcap_ctx, 0);
		if (ret < 0) {
			pr_err("%s: read-enable toggle failed for config reg (ret=%d)\n",
			       __func__, ret);
			return ret;
		}
	}

	address = mcap_ctx->vsec_offset + data->v3.offset;
	switch (data->v3.access) {
	case 'b':
		ret = pci_read_config_byte(pdev, address, &byte_data);
		if (ret == 0)
			data->v3.data = byte_data;
		break;
	case 'h':
		ret = pci_read_config_word(pdev, address, &short_data);
		if (ret == 0)
			data->v3.data = short_data;
		break;
	case 'w':
		ret = pci_read_config_dword(pdev, address, &word_data);
		if (ret == 0)
			data->v3.data = word_data;
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

/**
 * @brief Write to an MCAP V3 configuration address on a Spartan UltraScale+ device.
 *
 * Writes a byte ('b'), half-word ('h'), or word ('w') to the
 * specified offset within the MCAP VSEC configuration space.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] data      Pointer to cfg_data union with offset,
 *                       access type, and data to write.
 * @return 0 on success, -EINVAL for invalid access type,
 *         negative error code on other failures.
 */
int xvsec_mcapv3_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union cfg_data *data)
{
	int ret = 0;
	uint8_t byte_data;
	uint16_t short_data;
	uint32_t word_data;
	uint32_t address;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = xvsec_mcapv3_is_enabled(mcap_ctx);
	if (ret < 0)
		return ret;

	address = mcap_ctx->vsec_offset + data->v3.offset;

	switch (data->v3.access) {
	case 'b':
		byte_data = (uint8_t)data->v3.data;
		ret = pci_write_config_byte(pdev, address, byte_data);
		break;
	case 'h':
		short_data = (uint16_t)data->v3.data;
		ret = pci_write_config_word(pdev, address, short_data);
		break;
	case 'w':
		word_data = (uint32_t)data->v3.data;
		ret = pci_write_config_dword(pdev, address, word_data);
		break;
	default:
		ret = -(EINVAL);
		break;
	}

	return ret;
}

/**
 * @brief Read FPGA configuration register (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] cfg_reg   Pointer to fpga_cfg_reg union (unused).
 * @return -EPERM always.
 */
int xvsec_fpgav3_rd_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg)
{
	pr_err("FPGA read config register not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Write FPGA configuration register (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] cfg_reg   Pointer to fpga_cfg_reg union (unused).
 * @return -EPERM always.
 */
int xvsec_fpgav3_wr_cfg_addr(struct vsec_context *mcap_ctx,
	union fpga_cfg_reg *cfg_reg)
{
	pr_err("FPGA write config register not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Read AXI register address (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] cfg_reg   Pointer to axi_reg_data union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_axi_rd_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg)
{
	pr_err("AXI Read operation not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Write AXI register address (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] cfg_reg   Pointer to axi_reg_data union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_axi_wr_addr(struct vsec_context *mcap_ctx,
	union axi_reg_data *cfg_reg)
{
	pr_err("AXI Write operation not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Download file via AXI (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] file      Pointer to file_download_upload union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_file_download(struct vsec_context *mcap_ctx,
	union file_download_upload *file)
{
	pr_err("AXI File Download not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Upload file via AXI (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] file      Pointer to file_download_upload union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_file_upload(struct vsec_context *mcap_ctx,
	union file_download_upload *file)
{
	pr_err("AXI File upload not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}

/**
 * @brief Set AXI cache attributes (unsupported on Spartan UltraScale+).
 *
 * This operation is not supported for Spartan UltraScale+ devices.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] attr      Pointer to axi_cache_attr union (unused).
 * @return -EPERM always.
 */
int xvsec_mcapv3_set_axi_cache_attr(
	struct vsec_context *mcap_ctx, union axi_cache_attr *attr)
{
	pr_err("AXI attributes settings not supported for Spartan UltraScale+ devices\n");
	return -(EPERM);
}


/**
 * @brief Check for MCAP V3 programming completion.
 *
 * Polls the status register for the End-Of-Startup (EOS) bit to assert,
 * indicating that MCAP programming has completed. Performs read-enable
 * toggles during the polling loop.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] ret       Pointer to store the final status register value.
 * @return 0 on success (EOS asserted), -EIO on timeout.
 */
static int check_for_completion(struct vsec_context *mcap_ctx, uint32_t *ret)
{
	int rv = 0;
	unsigned long retry_count = 0;
	uint16_t sts_offset;
	uint32_t sts_data = 0;

	struct pci_dev *pdev = mcap_ctx->pdev;

	sts_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_STATUS_REG;
	pci_read_config_dword(pdev, sts_offset, &sts_data);

	while ((sts_data & XVSEC_MCAPV3_STATUS_EOS) == 0x0) {
		pci_read_config_dword(pdev, sts_offset, &sts_data);
		retry_count++;
		if (retry_count > EMCAPV3_EOS_RETRY_COUNT) {
			pr_err("Error: The MCAP EOS bit did not assert after");
			pr_err(" programming the specified programming file\n");
			pr_err("Status Reg : 0x%X\n", sts_data);
			rv = -EIO;
			break;
		}
		msleep(10);
	}
	*ret = sts_data;

	return rv;
}

/**
 * @brief Program a PDI file to the FPGA via MCAP V3.
 *
 * Opens the specified file, determines its size, writes the PDI
 * data to the MCAP write data register, checks for completion,
 * and verifies no errors occurred during programming.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] fname     Path to the PDI file to program.
 * @param[in] tr_mode   Data transfer mode (fast or slow).
 * @return 0 on success, negative error code on failure.
 */
static int xvsec_mcapv3_program(struct vsec_context *mcap_ctx, char *fname, enum data_transfer_mode tr_mode)
{
	int ret = 0;
	loff_t file_size;
	struct file *filep;
	uint32_t sts_data;
	uint16_t cnfg_offset;
	uint32_t cnfg_data;
	struct pci_dev *pdev = mcap_ctx->pdev;

	pr_info("file name : %s\n", fname);

	filep = xvsec_util_fopen(fname, O_RDONLY, 0);
	if (filep == NULL)
		return -(ENOENT);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	ret = xvsec_util_get_file_size(fname, &file_size);
	if (ret < 0)
		goto CLEANUP;
#else
	file_size = xvsec_util_get_file_size(filep);
#endif
	pr_info("file_size = %lld & %d\n", file_size, tr_mode);

	if (file_size <= 0) {
		ret = -(EINVAL);
		goto CLEANUP;
	}

	ret = xvsec_write_pdi(mcap_ctx, filep, file_size, tr_mode);
	if (ret < 0)
		goto CLEANUP;

	ret = check_for_completion(mcap_ctx, &sts_data);
	if ((ret != 0) ||
		((sts_data & XVSEC_MCAPV3_STATUS_ERR) != 0x0) ||
		((sts_data & XVSEC_MCAPV3_STATUS_FIFO_OVFL) != 0x0)) {
		pr_err("MCAP Error\n");
		xvsec_mcapv3_full_reset(mcap_ctx);
		ret = -(EIO);
		goto CLEANUP;
	}

	ret = mcap_read_enable_toggle(mcap_ctx, 1);
	if (ret < 0) {
		pr_err("MCAP program: read-enable toggle failed after completion (ret=%d)\n", ret);
		goto CLEANUP;
	}

	cnfg_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG;
	pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);
	if ((cnfg_data & XVSEC_MCAPV3_CNFG_PMCL_ERR ) != 0x0) {
		pr_err("MCAP Config Engine Error\n");
		xvsec_mcapv3_full_reset(mcap_ctx);
		ret = -(EIO);
	}

CLEANUP:
	xvsec_util_fclose(filep);
	return ret;
}

/**
 * @brief Wait for the SBI (Slave Boot Interface) FIFO to become empty.
 *
 * Polls the configuration register for the SBI_EMPTY bit with a
 * read-enable toggle on each iteration. Times out after
 * MAX_OP_POLL_CNT_V3 iterations.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, -ETIME on timeout.
 */
static int xvsec_mcapv3_wait_for_sbi_empty(struct vsec_context *mcap_ctx)
{
	int ret = 0;
	uint32_t wait_cnt = 0;
	uint16_t cnfg_offset;
	uint32_t cnfg_data;
	bool is_sbi_empty;
	struct pci_dev *pdev = mcap_ctx->pdev;

	ret = mcap_read_enable_toggle(mcap_ctx, 1);
	if (ret < 0)
	     return ret;

	cnfg_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_CONFIG_REG;
	pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);

	while (((is_sbi_empty = XVSEC_MCAPV3_IS_SBI_EMPTY(cnfg_data)) != true)
				&& (wait_cnt++ < MAX_OP_POLL_CNT_V3)) {
		udelay(1);
		ret = mcap_read_enable_toggle(mcap_ctx, 1);
		if (ret < 0)
		     return ret;
		pci_read_config_dword(pdev, cnfg_offset, &cnfg_data);
	}

	if (is_sbi_empty != true) {
		pr_err("%s: Timeout SBI Empty\n", __func__);
		ret = -(ETIME);
	}

	return ret;
}

/**
 * @brief Write PDI data to the MCAP V3 write data register.
 *
 * Reads the PDI file in chunks and writes each 32-bit word to the
 * MCAP write data register via PCI configuration space. In slow
 * transfer mode, waits for the SBI FIFO to drain after each chunk.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] filep     File pointer to the opened PDI file.
 * @param[in] size      Total size of the PDI file in bytes.
 * @param[in] tr_mode   Data transfer mode (DATA_TRANSFER_MODE_SLOW
 *                       waits for SBI empty between chunks).
 * @return 0 on success, -ENOMEM on allocation failure,
 *         negative error code on other failures.
 */
static int xvsec_write_pdi(struct vsec_context *mcap_ctx,
	struct file *filep, loff_t size, enum data_transfer_mode tr_mode)
{
	int err = 0;
	uint32_t loop;
	uint64_t offset = 0x0;
	uint32_t *buf;
	uint16_t chunk = 0;
	uint16_t wr_offset;
	loff_t remain_size = 0;
	struct pci_dev *pdev = mcap_ctx->pdev;

	buf = kmalloc(DMA_HWICAP_BIT_FLOW_BUFFER_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -(ENOMEM);

	err = xvsec_mcapv3_wait_for_sbi_empty(mcap_ctx);
	if (err != 0) {
		pr_err("MCAP Config FIFO Not Empty\n");
		goto CLEANUP;
	}

	memset(buf, 0, DMA_HWICAP_BIT_FLOW_BUFFER_SIZE);
	remain_size = size;
	wr_offset = mcap_ctx->vsec_offset + XVSEC_MCAPV3_WRITE_DATA_REG;
	while (remain_size != 0) {
		chunk = (remain_size > DMA_HWICAP_BIT_FLOW_BUFFER_SIZE) ?
			DMA_HWICAP_BIT_FLOW_BUFFER_SIZE : remain_size;


		err = xvsec_util_fread(filep,
			offset, (uint8_t *)&buf[0], chunk);
		if (err < 0)
			goto CLEANUP;

		for (loop = 0; loop < (chunk / 4); loop++) {

			// Write the processed data
			pci_write_config_dword(pdev, wr_offset, (uint32_t)buf[loop]);

			/* FROM SDAccel Code:
			 * This delay resolves the MIG calibration issues
			 * we have been seeing with Tandem Stage 2 Loading
			 */
			udelay(1);
		}

		if (tr_mode == DATA_TRANSFER_MODE_SLOW) {

			err = xvsec_mcapv3_wait_for_sbi_empty(mcap_ctx);
			if (err != 0) {
				pr_err("MCAP Config FIFO Not Empty\n");
				goto CLEANUP;
			}
		}

		offset = offset + chunk;
		remain_size = remain_size - chunk;
	}
CLEANUP:
	kfree(buf);
	return err;
}
