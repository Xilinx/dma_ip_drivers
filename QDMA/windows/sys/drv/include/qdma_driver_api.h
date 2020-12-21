/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef QDMA_DRIVER_API_H_
#define QDMA_DRIVER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <basetsd.h>
#include <pshpack8.h>

/**
 * Define an Interface Guid so that app can find the device and talk to it.
 */
#include <guiddef.h>

typedef int BOOL;

#ifdef DEFINE_GUID
DEFINE_GUID (GUID_DEVINTERFACE_QDMA,
    0x80f90fa8,0x0cea,0x43c5,0x81,0xce,0x4b,0xf9,0x4a,0xa1,0x4c,0x72);
/** {80f90fa8-0cea-43c5-81ce-4bf94aa14c72} */
#endif

/** Interface File Names exposed by driver */
#define QDMA_FILE_USER              L"\\user"
#define QDMA_FILE_CONTROL           L"\\control"
#define QDMA_FILE_BYPASS            L"\\bypass"
#define QDMA_FILE_DMA_QUEUE         L"\\queue_"
#define QDMA_FILE_DMA_ST_QUEUE      L"\\st_"
#define QDMA_FILE_DMA_MGMT          L"\\mgmt"

#define QDMA_CSR_SZ                 16U
#define QDMA_MAX_UDD_DATA_LEN       64U
#define QDMA_VERSION_INFO_LENGTH    32U

/** Queue Direction */
enum queue_direction {
    QUEUE_DIR_H2C = 0,
    QUEUE_DIR_C2H
};

/** Descriptor Type */
enum descriptor_type {
    RING_DESC = 0,
    CMPT_DESC
};

/** Completion descriptor size */
enum cmpt_desc_sz {
    CMPT_DESC_SZ_8B = 0,
    CMPT_DESC_SZ_16B,
    CMPT_DESC_SZ_32B,
    CMPT_DESC_SZ_64B,
    CMPT_DESC_SZ_MAX
};

/** Trigger mode (to be used for ST Queues) */
enum trig_mode {
    TRIG_MODE_DISABLE = 0,
    TRIG_MODE_EVERY,
    TRIG_MODE_USER_COUNT,
    TRIG_MODE_USER,
    TRIG_MODE_USER_TIMER,
    TRIG_MODE_USER_TIMER_COUNT,
    TRIG_MODE_MAX
};

enum ring_type {
    RING_TYPE_H2C = 0,
    RING_TYPE_C2H,
    RING_TYPE_CMPT
};

/** Device IOCTL Commands */
enum commands {
    CMD_CSR_DUMP,
    CMD_DEVINFO,
    CMD_QUEUE_ADD,
    CMD_QUEUE_START,
    CMD_QUEUE_STOP,
    CMD_QUEUE_DELETE,
    CMD_QUEUE_DUMP_STATE,
    CMD_QUEUE_READ_UDD,
    CMD_QUEUE_DUMP_DESC,
    CMD_QUEUE_DUMP_CTX,
    CMD_QUEUE_CMPT_READ,
    CMD_INTRING_DUMP,
    CMD_REG_DUMP,
    CMD_QUEUE_NO_COPY,
    CMD_SET_QMAX,
    CMD_GET_QSTATS,
    CMD_REG_INFO,
    CMD_OP_MAX
};

/** Structure which holds the CSR registers information
 *  IOCTL Command :
 *          IOCTL_QDMA_CSR_DUMP
 */
struct csr_conf_out{
    UINT32 ring_sz[QDMA_CSR_SZ];
    UINT32 c2h_timer_cnt[QDMA_CSR_SZ];
    UINT32 c2h_th_cnt[QDMA_CSR_SZ];
    UINT32 c2h_buff_sz[QDMA_CSR_SZ];
    UINT32 wb_interval;
};

/** Structure which holds the Queue configuration parameters
 *  Valid IOCTL Commands:
 *          IOCTL_QDMA_QUEUE_ADD
 *          IOCTL_QDMA_QUEUE_START
 *          IOCTL_QDMA_QUEUE_STOP
 *          IOCTL_QDMA_QUEUE_DELETE
 */
struct queue_conf_in {
    UINT16				qid;
    BOOL                is_st;
    UINT8				h2c_ring_sz_index;
    UINT8				c2h_ring_sz_index;
    UINT8				c2h_buff_sz_index;
    UINT8				c2h_th_cnt_index;
    UINT8				c2h_timer_cnt_index;
    enum cmpt_desc_sz   compl_sz;
    enum trig_mode      trig_mode;
    UINT8				sw_desc_sz;
    BOOL                desc_bypass_en;
    BOOL                pfch_en;
    BOOL                pfch_bypass_en;
    BOOL                cmpl_ovf_dis;
    BOOL                en_mm_cmpl;
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_STATE
 */
struct queue_state_in {
    UINT16 qid;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_STATE
 */
struct queue_state_out {
    char state[20];
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_DEVINFO
*/
struct device_info_out {
    struct version_info {
        char qdma_rtl_version_str[QDMA_VERSION_INFO_LENGTH];
        char qdma_vivado_release_id_str[QDMA_VERSION_INFO_LENGTH];
        char qdma_device_type_str[QDMA_VERSION_INFO_LENGTH];
        char qdma_versal_ip_type_str[QDMA_VERSION_INFO_LENGTH];
        char qdma_sw_version[QDMA_VERSION_INFO_LENGTH];
    }ver_info;
    UINT32	num_pfs;
    UINT32	num_qs;
    BOOL	flr_present;
    BOOL	st_en;
    BOOL	mm_en;
    BOOL	mm_cmpl_en;
    BOOL	mailbox_en;
    BOOL	debug_mode;
    UINT8	desc_eng_mode;
    UINT32	num_mm_channels;
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_READ_UDD
*/
struct cmpt_udd_info_in {
    UINT16 qid;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_READ_UDD
*/
struct cmpt_udd_info_out {
    UINT64 length;
    UINT8  buffer[QDMA_MAX_UDD_DATA_LEN];
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_DESC
*/
struct desc_dump_info_in {
    UINT16			        qid;
    enum queue_direction    dir;
    enum descriptor_type    desc_type;
    UINT32                  desc_start;
    UINT32                  desc_end;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_DESC
*/
struct desc_dump_info_out {
    size_t    desc_sz;
    size_t    data_sz;
    UINT8     pbuffer[1];
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_CTX
*/
struct ctx_dump_info_in {
    UINT16          qid;
    enum ring_type  type;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_DUMP_CTX
*/
struct ctx_dump_info_out {
    size_t  ret_sz;
    char    pbuffer[1];
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_CMPT_READ
*/
struct cmpt_data_info_in {
    UINT16  qid;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_QUEUE_CMPT_READ
*/
struct cmpt_data_info_out {
    size_t  ret_len;
    size_t  cmpt_desc_sz;
    UINT8 pbuffer[1];
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_INTRING_DUMP
 */
struct intring_info_in {
    UINT32  vec_id;
    UINT32  start_idx;
    UINT32  end_idx;
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_SET_QMAX
 */
struct qmax_in {
    UINT32  qmax;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_INTRING_DUMP
 */
struct intring_info_out {
    size_t  ret_len;
    size_t  ring_entry_sz;
    UINT8   pbuffer[1];
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_REG_DUMP
 */
struct regdump_info_out {
    size_t  ret_len;
    char   pbuffer[1];
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_GET_QSTATS
 */
struct qstat_out {
    UINT32 qbase;
    UINT32 qmax;
    UINT32 active_h2c_queues;
    UINT32 active_c2h_queues;
    UINT32 active_cmpt_queues;
};

/** Structure to be passed as input parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_REG_INFO
 */
struct reg_info_in {
    UINT32  bar_no;
    UINT32  address;
    UINT32  reg_cnt;
};

/** Structure to be passed as output parameter for
 *  IOCTL Command :
 *          IOCTL_QDMA_REG_INFO
 */
struct reg_info_out {
    size_t  ret_len;
    char    pbuffer[1];
};

struct csr_conf_data {
    struct csr_conf_out *out;
};

struct queue_conf {
    struct queue_conf_in in;
};

struct queue_active_state {
    struct queue_state_in   in;
    struct queue_state_out  *out;
};

struct device_info {
    struct device_info_out  *out;
};

struct cmpt_udd_info {
    struct cmpt_udd_info_in     in;
    struct cmpt_udd_info_out    *out;
};

struct desc_dump_info {
    struct desc_dump_info_in    in;
    struct desc_dump_info_out   *out;
};

struct ctx_dump_info {
    struct ctx_dump_info_in     in;
    struct ctx_dump_info_out    *out;
};

struct cmpt_data_info {
    struct cmpt_data_info_in    in;
    struct cmpt_data_info_out   *out;
};

struct intring_info {
    struct intring_info_in      in;
    struct intring_info_out     *out;
};

struct regdump_info {
    struct regdump_info_out     *out;
};

struct qmax_info {
    struct qmax_in              in;
};

struct qstats_info {
    struct qstat_out            *out;
};

struct reg_info {
    struct reg_info_in      in;
    struct reg_info_out* out;
};

/** Union that consolidates parameters for all ioctl commands */
union ioctl_cmd {
    struct csr_conf_data        csr;
    struct queue_conf           q_conf;
    struct queue_active_state   q_state;
    struct device_info          dev_info;
    struct cmpt_udd_info        udd_info;
    struct desc_dump_info       desc_info;
    struct ctx_dump_info        ctx_info;
    struct cmpt_data_info       cmpt_info;
    struct intring_info         int_ring_info;
    struct regdump_info         reg_dump_info;
    struct qmax_info            qmax_info;
    struct qstats_info          qstats_info;
    struct reg_info             reg_info;
};

#define QDMA_IOCTL(index) CTL_CODE(FILE_DEVICE_UNKNOWN, index, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_QDMA_CSR_DUMP             QDMA_IOCTL(CMD_CSR_DUMP)
#define IOCTL_QDMA_DEVINFO              QDMA_IOCTL(CMD_DEVINFO)
#define IOCTL_QDMA_QUEUE_ADD            QDMA_IOCTL(CMD_QUEUE_ADD)
#define IOCTL_QDMA_QUEUE_START          QDMA_IOCTL(CMD_QUEUE_START)
#define IOCTL_QDMA_QUEUE_STOP           QDMA_IOCTL(CMD_QUEUE_STOP)
#define IOCTL_QDMA_QUEUE_DELETE         QDMA_IOCTL(CMD_QUEUE_DELETE)
#define IOCTL_QDMA_QUEUE_DUMP_STATE     QDMA_IOCTL(CMD_QUEUE_DUMP_STATE)
#define IOCTL_QDMA_QUEUE_READ_UDD       QDMA_IOCTL(CMD_QUEUE_READ_UDD)
#define IOCTL_QDMA_QUEUE_DUMP_DESC      QDMA_IOCTL(CMD_QUEUE_DUMP_DESC)
#define IOCTL_QDMA_QUEUE_DUMP_CTX       QDMA_IOCTL(CMD_QUEUE_DUMP_CTX)
#define IOCTL_QDMA_QUEUE_CMPT_READ      QDMA_IOCTL(CMD_QUEUE_CMPT_READ)
#define IOCTL_QDMA_INTRING_DUMP         QDMA_IOCTL(CMD_INTRING_DUMP)
#define IOCTL_QDMA_REG_DUMP             QDMA_IOCTL(CMD_REG_DUMP)
#define IOCTL_QDMA_QUEUE_NO_COPY        QDMA_IOCTL(CMD_QUEUE_NO_COPY)
#define IOCTL_QDMA_SET_QMAX             QDMA_IOCTL(CMD_SET_QMAX)
#define IOCTL_QDMA_GET_QSTATS           QDMA_IOCTL(CMD_GET_QSTATS)
#define IOCTL_QDMA_REG_INFO             QDMA_IOCTL(CMD_REG_INFO)

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif /* QDMA_DRIVER_API_H_ */
