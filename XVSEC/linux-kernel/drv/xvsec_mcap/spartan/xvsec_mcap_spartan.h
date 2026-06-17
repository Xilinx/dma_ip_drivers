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

#ifndef __XVSEC_MCAP_SPARTAN_H__
#define __XVSEC_MCAP_SPARTAN_H__

/** @defgroup mcapv3_constants MCAP V3 Constants
 *  @{
 */

/** @brief Maximum file name length */
#define MAX_FLEN                          (300)
/** @brief Maximum loop count for polling operations */
#define XVSEC_MCAPV3_LOOP_COUNT           (1000000)
/** @brief Maximum poll count for MCAP V3 operations */
#define MAX_OP_POLL_CNT_V3                   (100000)
/** @brief DMA HWICAP bitstream flow buffer size in bytes */
#define DMA_HWICAP_BIT_FLOW_BUFFER_SIZE   (8192)
/** @brief Maximum retry count for End-Of-Startup (EOS) check */
#define EMCAPV3_EOS_RETRY_COUNT             (10)

/** @brief PDI file extension string */
#define MCAPV3_PDI_FILE                   (".pdi")

/** @} */ /* end of mcapv3_constants */

/** @defgroup mcapv3_reg_offsets MCAP V3 Spartan UltraScale+ Register Offsets
 *  @{
 */

/** @brief Extended Capability Header register offset */
#define XVSEC_MCAPV3_EXTENDED_HEADER      (0x0000)
/** @brief Vendor Specific Header register offset */
#define XVSEC_MCAPV3_VENDOR_HEADER        (0x0004)
/** @brief Reserved register offset */
#define XVSEC_MCAPV3_RESERVED             (0x0008)
/** @brief FPGA Bitstream Version register offset */
#define XVSEC_MCAPV3_FPGA_BIT_VER         (0x000c)
/** @brief Status register offset */
#define XVSEC_MCAPV3_STATUS_REG           (0x0010)
/** @brief Control register offset */
#define XVSEC_MCAPV3_CONTROL_REG          (0x0014)
/** @brief Write Data register offset */
#define XVSEC_MCAPV3_WRITE_DATA_REG       (0x0018)
/** @brief Configuration register offset */
#define XVSEC_MCAPV3_CONFIG_REG           (0x001c)

/** @} */ /* end of mcapv3_reg_offsets */

/** @defgroup mcapv3_ctrl_fields MCAP V3 Control Register Bit Fields
 *  @{
 */

/** @brief Control register - MCAP Enable bit (bit 0) */
#define XVSEC_MCAPV3_CTRL_ENABLE          (1 << 0)
/** @brief Control register - Read Enable bit (bit 1) */
#define XVSEC_MCAPV3_READ_ENABLE          (1 << 1)
/** @brief Control register - Reset bit (bit 4) */
#define XVSEC_MCAPV3_CTRL_RESET           (1 << 4)
/** @brief Control register - Module Reset bit (bit 5) */
#define XVSEC_MCAPV3_CTRL_MOD_RESET       (1 << 5)
/** @brief Control register - Request Access bit (bit 8) */
#define XVSEC_MCAPV3_CTRL_REQ_ACCESS      (1 << 8)
/** @brief Control register - Configuration Switch bit (bit 12) */
#define XVSEC_MCAPV3_CTRL_CFG_SWICTH      (1 << 12)
/** @brief Control register - Write Enable bit (bit 16) */
#define XVSEC_MCAPV3_CTRL_WR_ENABLE       (1 << 16)

/** @} */ /* end of mcapv3_ctrl_fields */

/** @defgroup mcapv3_cnfg_fields MCAP V3 Configuration Register Bit Fields
 *  @{
 */

/** @brief Configuration register - SBI Empty status bit (bit 20) */
#define XVSEC_MCAPV3_CNFG_SBI_EMPTY       (1 << 20)
/** @brief Configuration register - Spartan UltraScale+ Mode bit (bit 21) */
#define XVSEC_MCAPV3_CNFG_LMODE           (1 << 21)
/** @brief Configuration register - Reset status bit (bit 22) */
#define XVSEC_MCAPV3_CNFG_RESET		  (1 << 22)
/** @brief Configuration register - PMC Logic Error bit (bit 23) */
#define XVSEC_MCAPV3_CNFG_PMCL_ERR	  (1 << 23)
/** @brief Configuration register - MCAP Disable bit (bit 24) */
#define XVSEC_MCAPV3_CNFG_DISABLE	  (1 << 24)
/** @brief Configuration register - eFuse MCAP Disable bit (bit 25) */
#define XVSEC_MCAPV3_CNFG_EFUSE		  (1 << 25)

/** @} */ /* end of mcapv3_cnfg_fields */

/** @defgroup mcapv3_sts_fields MCAP V3 Status Register Bit Fields
 *  @{
 */

/** @brief Status register - Error bit (bit 0) */
#define XVSEC_MCAPV3_STATUS_ERR           (1 << 0)
/** @brief Status register - End-Of-Startup (EOS) bit (bit 1) */
#define XVSEC_MCAPV3_STATUS_EOS           (1 << 1)
/** @brief Status register - FIFO Overflow bit (bit 8) */
#define XVSEC_MCAPV3_STATUS_FIFO_OVFL     (1 << 8)
/** @brief Status register - FPGA Configuration Access bit (bit 24) */
#define XVSEC_MCAPV3_STATUS_ACCESS        (1 << 24)

/** @} */ /* end of mcapv3_sts_fields */

/**
 * @brief Check if the SBI (Slave Boot Interface) FIFO is empty.
 * @param[in] sts  The configuration register value to check.
 * @return true if SBI FIFO is empty, false otherwise.
 */
#define XVSEC_MCAPV3_IS_SBI_EMPTY(sts) \
	((sts & XVSEC_MCAPV3_CNFG_SBI_EMPTY) ? true : false)


/**
 * @brief Perform MCAP V3 reset on a Spartan UltraScale+ device.
 *
 * Asserts the MCAP reset bit and waits for the reset to complete
 * by polling the configuration register.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_reset(struct vsec_context *mcap_ctx);

/**
 * @brief Perform MCAP V3 module reset on a Spartan UltraScale+ device.
 *
 * Asserts the module reset bit in the MCAP control register.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_module_reset(struct vsec_context *mcap_ctx);

/**
 * @brief Perform a full MCAP V3 reset (MCAP + module) on a Spartan UltraScale+ device.
 *
 * Asserts both the MCAP reset and module reset bits, then waits
 * for the reset to complete.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_full_reset(struct vsec_context *mcap_ctx);

/**
 * @brief Read all MCAP V3 registers for a Spartan UltraScale+ device.
 *
 * Reads the complete set of MCAP V3 registers from the PCI
 * configuration space into the provided register union.
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Pointer to union to store the register values.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_get_regs(struct vsec_context *mcap_ctx,
        union mcap_regs *regs);

/**
 * @brief Read an MCAP V3 configuration address on a Spartan UltraScale+ device.
 *
 * Reads a byte, half-word, or word from the specified offset
 * within the MCAP VSEC configuration space.
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] data      Pointer to cfg_data union containing offset,
 *                           access type, and storage for read data.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_rd_cfg_addr(
        struct vsec_context *mcap_ctx, union cfg_data *data);

/**
 * @brief Write to an MCAP V3 configuration address on a Spartan UltraScale+ device.
 *
 * Writes a byte, half-word, or word to the specified offset
 * within the MCAP VSEC configuration space.
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] data      Pointer to cfg_data union containing offset,
 *                       access type, and data to write.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_wr_cfg_addr(
        struct vsec_context *mcap_ctx, union cfg_data *data);

/**
 * @brief Program a full bitstream via MCAP V3 on a Spartan UltraScale+ device.
 *
 * Performs the complete MCAP programming sequence including access
 * request, error checking, write enable, and optional partial clear
 * and bitstream file programming. Sets the CFG_SWITCH bit on success.
 *
 * @param[in]     mcap_ctx   Pointer to the VSEC context structure.
 * @param[in,out] bit_files  Pointer to bitstream_file_v3 union containing
 *                            file paths, transfer mode, and output status.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_program_bitstream_full(struct vsec_context *mcap_ctx,
        union bitstream_file_v3 *bit_files);

/**
 * @brief Program a raw bitstream via MCAP V3 on a Spartan UltraScale+ device.
 *
 * Programs the partial clear and/or bitstream files without performing
 * MCAP access request or control register setup.
 *
 * @param[in]     mcap_ctx   Pointer to the VSEC context structure.
 * @param[in,out] bit_files  Pointer to bitstream_file_v3 union containing
 *                            file paths, transfer mode, and output status.
 * @return 0 on success, negative error code on failure.
 */
int xvsec_mcapv3_program_bitstream_raw(struct vsec_context *mcap_ctx,
        union bitstream_file_v3 *bit_files);

/** @defgroup mcapv3_unsupported Unsupported Operations for Spartan UltraScale+ Devices
 *  @{
 */

/**
 * @brief Get MCAP data registers (unsupported on Spartan UltraScale+).
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Array of 4 uint32_t to store data register values.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_get_data_regs(struct vsec_context *mcap_ctx, uint32_t regs[4]);

/**
 * @brief Get FPGA configuration registers (unsupported on Spartan UltraScale+).
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] regs      Pointer to fpga_cfg_regs union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_get_fpga_regs(struct vsec_context *mcap_ctx,
        union fpga_cfg_regs *regs);

/**
 * @brief Read FPGA configuration register address (unsupported on Spartan UltraScale+).
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] cfg_reg   Pointer to fpga_cfg_reg union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_fpgav3_rd_cfg_addr(struct vsec_context *mcap_ctx,
        union fpga_cfg_reg *cfg_reg);

/**
 * @brief Write FPGA configuration register address (unsupported on Spartan UltraScale+).
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] cfg_reg   Pointer to fpga_cfg_reg union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_fpgav3_wr_cfg_addr(struct vsec_context *mcap_ctx,
        union fpga_cfg_reg *cfg_reg);

/**
 * @brief Read AXI register address (unsupported on Spartan UltraScale+).
 *
 * @param[in]     mcap_ctx  Pointer to the VSEC context structure.
 * @param[in,out] cfg_reg   Pointer to axi_reg_data union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_axi_rd_addr(struct vsec_context *mcap_ctx,
        union axi_reg_data *cfg_reg);

/**
 * @brief Write AXI register address (unsupported on Spartan UltraScale+).
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] cfg_reg   Pointer to axi_reg_data union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_axi_wr_addr(struct vsec_context *mcap_ctx,
        union axi_reg_data *cfg_reg);

/**
 * @brief Download file via AXI (unsupported on Spartan UltraScale+).
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] file      Pointer to file_download_upload union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_file_download(struct vsec_context *mcap_ctx,
        union file_download_upload *file);

/**
 * @brief Upload file via AXI (unsupported on Spartan UltraScale+).
 *
 * @param[in]  mcap_ctx  Pointer to the VSEC context structure.
 * @param[out] file      Pointer to file_download_upload union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_file_upload(struct vsec_context *mcap_ctx,
        union file_download_upload *file);

/**
 * @brief Set AXI cache attributes (unsupported on Spartan UltraScale+).
 *
 * @param[in] mcap_ctx  Pointer to the VSEC context structure.
 * @param[in] attr      Pointer to axi_cache_attr union.
 * @return -EPERM always (operation not supported on Spartan UltraScale+ devices).
 */
int xvsec_mcapv3_set_axi_cache_attr(struct vsec_context *mcap_ctx,
        union axi_cache_attr *attr);

/** @} */ /* end of mcapv3_unsupported */

#endif
