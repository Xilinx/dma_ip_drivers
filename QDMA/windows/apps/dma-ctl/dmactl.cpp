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

#include <windows.h>
#include <winioctl.h>
#include <iostream>
#include <algorithm>

#include "device_file.hpp"
#include "qdma_driver_api.h"
#include "version.h"

#define MAX_VALID_GLBL_IDX      15
#define MAX_VALID_CMPT_SZ       3
#define MAX_VALID_INTR_RING_VEC 8
#define MAX_VALID_BAR_NUM       5
#define MAX_CMPT_DESC_SZ        64
#define MAX_INTR_RING_ENTRY_SZ  32
#define MAX_DUMP_BUFF_SZ        64 * 1024
#define MAX_REG_INFO_SZ         1024

#pragma comment(lib, "setupapi.lib")

using std::uint32_t;
using std::string;
using std::runtime_error;
using std::cout;
using namespace std;

const char *desc_engine_mode[] = {
    "Internal and Bypass mode",
    "Bypass only mode"
    "Inernal only mode",
};


static void help(void);

static bool open_device(const char *dev_name, device_file& device)
{
    device_details dev_info;

    if (dev_name == NULL) {
        cout << "Null Parameter provided for dev_name\n";
        return false;
    }

    auto dev_details = get_device_details(GUID_DEVINTERFACE_QDMA);

    if (!dev_details.size()) {
        cout << "No QDMA Devices found\n";
        return false;
    }

    auto res = get_device(dev_details, dev_name, dev_info);
    if (res == false) {
        printf("Device name requested to open is not valid\n");
        return false;
    }

    const auto& dev_path = dev_info.device_path;
    device.open(dev_path + "\\mgmt", GENERIC_READ | GENERIC_WRITE);

    return true;
}

bool sort_fun(device_details d1, device_details d2)
{
    UINT32 bdf_1 = 0x0, bdf_2 = 0x0;

    bdf_1 = (d1.bus_no << 12) | (d1.dev_no << 4) | d1.fun_no;
    bdf_2 = (d2.bus_no << 12) | (d2.dev_no << 4) | d2.fun_no;
    return (bdf_1 < bdf_2);
}

static int get_qrange(const char *dev_name, unsigned int& qmax, unsigned int& qbase)
{
    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 1;

        struct qstat_out qstats_info = { 0 };
        device.ioctl(IOCTL_QDMA_GET_QSTATS, NULL, 0, &qstats_info, sizeof(qstats_info));
        qmax = qstats_info.qmax;
        qbase = qstats_info.qbase;
    }
    catch (const std::exception& e) {
        cout << "Failed to get qmax from ioctl : " << e.what() << '\n';
    }

    return 0;
}

static int handle_dev_cmds(const int argc, char* argv[])
{
    auto i = 0;

    if (0 == argc) {
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "list") == 0) {
            auto device_details = get_device_details(GUID_DEVINTERFACE_QDMA);
            UINT32 prev_bus = 0x000000;

            if (!device_details.size()) {
                printf("\nNo QDMA Devices present in the system\n\n");
                break;
            }

            std::sort(device_details.begin(), device_details.end(), sort_fun);

            for (auto idx = 0; idx < (int)device_details.size(); idx++) {
                unsigned int qmax = 0, qbase = 0;
                int ret = get_qrange(device_details[idx].device_name.c_str(), qmax, qbase);
                if (ret)
                    return 1;

                auto bus_no = device_details[idx].bus_no;
                auto dev_no = device_details[idx].dev_no;
                auto fun_no = device_details[idx].fun_no;
                auto end_q = qbase + qmax - 1;

                if (prev_bus != bus_no) {
                    printf("\n");
                    prev_bus = bus_no;
                }

                printf("%s\t0000:%02X:%02X:%01X\tmaxQP: %d, %d~%d\n", device_details[idx].device_name.c_str(), bus_no, dev_no, fun_no, qmax, qbase, end_q);
            }
            printf("\n");
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
        ++i;
    }

    return 0;
}
static int handle_csr_cmds(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    device_file device;

    auto ret = open_device(dev_name, device);
    if (ret == false)
        return 0;

    while (i < argc) {
        if (strcmp(argv[i], "dump") == 0) {
            ioctl_cmd cmd = {};
            DWORD ioctl_code = IOCTL_QDMA_CSR_DUMP;

            cmd.csr.out = (struct csr_conf_out *)new struct csr_conf_out;

            try {
                device.ioctl(ioctl_code, NULL, 0, cmd.csr.out, sizeof(struct csr_conf_out));
                printf("Global CSR :\n");

                printf("Index     Ring size           Timer count         Threshold count     Buffer size\n");
                for (auto ind = 0; ind < QDMA_CSR_SZ; ++ind)
                    printf("%-10d%-20u%-20u%-20u%-20u\n",
                        ind,
                        cmd.csr.out->ring_sz[ind],
                        cmd.csr.out->c2h_timer_cnt[ind],
                        cmd.csr.out->c2h_th_cnt[ind],
                        cmd.csr.out->c2h_buff_sz[ind]);

                delete cmd.csr.out;
            }
            catch (const std::exception& e) {
                delete cmd.csr.out;
                cout << "Failed to dump CSR from ioctl : " << e.what() << '\n';
            }
        }
        else {
            return 1;
        }

        ++i;
    }

    return 0;
}

static int handle_devinfo(const char *dev_name)
{
    device_file device;

    auto ret = open_device(dev_name, device);
    if (ret == false)
        return 0;

    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_DEVINFO;

    cmd.dev_info.out = (struct device_info_out *)new struct device_info_out;

    try {
        device.ioctl(ioctl_code, NULL, 0, cmd.dev_info.out, sizeof(struct device_info_out));

        printf("=============Hardware Version=================\n");
        printf("%-35s : %s\n", "RTL Version", cmd.dev_info.out->ver_info.qdma_rtl_version_str);
        printf("%-35s : %s\n", "Vivado ReleaseID", cmd.dev_info.out->ver_info.qdma_vivado_release_id_str);
        printf("%-35s : %s\n", "Device Type", cmd.dev_info.out->ver_info.qdma_device_type_str);
        if (strstr(cmd.dev_info.out->ver_info.qdma_device_type_str, "Versal IP") != NULL)
            printf("%-35s : %s\n", "Versal IP Type", cmd.dev_info.out->ver_info.qdma_versal_ip_type_str);
        printf("\n");

        printf("=============Software Version=================\n");
        printf("%-35s : %s\n", "qdma driver version", cmd.dev_info.out->ver_info.qdma_sw_version);
        printf("\n");

        printf("=============Hardware Capabilities============\n");

        printf("%-35s : %d\n", "Number of PFs supported", cmd.dev_info.out->num_pfs);
        printf("%-35s : %d\n", "Total number of queues supported", cmd.dev_info.out->num_qs);
        printf("%-35s : %d\n", "MM channels", cmd.dev_info.out->num_mm_channels);
        printf("%-35s : %s\n", "FLR Present", cmd.dev_info.out->flr_present ? "yes" : "no");
        printf("%-35s : %s\n", "ST enabled", cmd.dev_info.out->st_en ? "yes" : "no");
        printf("%-35s : %s\n", "MM enabled", cmd.dev_info.out->mm_en ? "yes" : "no");
        printf("%-35s : %s\n", "Mailbox enabled", cmd.dev_info.out->mailbox_en ? "yes" : "no");
        printf("%-35s : %s\n", "MM completion enabled", cmd.dev_info.out->mm_cmpl_en ? "yes" : "no");
        printf("%-35s : %s\n", "Debug Mode enabled", cmd.dev_info.out->debug_mode ? "yes" : "no");
        if (cmd.dev_info.out->desc_eng_mode < sizeof(desc_engine_mode) / sizeof(desc_engine_mode[0])) {
            printf("%-35s : %s\n", "Desc Engine Mode", desc_engine_mode[cmd.dev_info.out->desc_eng_mode]);
        }
        else {
            printf("%-35s : %s\n", "Desc Engine Mode", "Invalid");
        }

        printf("\n");

        delete cmd.dev_info.out;
    }
    catch (const std::exception& e) {
        delete cmd.dev_info.out;
        cout << "Failed to get devinfo from ioctl : " << e.what() << '\n';
    }
    return 0;
}

static int handle_qmax(const char *dev_name, const int argc, char* argv[])
{
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_SET_QMAX;

    if (argc > 1) {
        cout << "Only one argument is required" << endl;
        return 1;
    }

    cmd.qmax_info.in.qmax = (unsigned short)strtoul(argv[0], NULL, 0);

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, &cmd.qmax_info.in, sizeof(cmd.qmax_info.in), NULL, 0);
        cout << "Set qmax successfull" << endl;
    }
    catch (const std::exception& e) {
        cout << "Failed to set qmax.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_qstats(const char *dev_name)
{
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_GET_QSTATS;
    cmd.qstats_info.out = new struct qstat_out;

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, NULL, 0, cmd.qstats_info.out, sizeof(struct qstat_out));

        printf("Device             : %s\n", dev_name);
        printf("Maximum queues     : %u\n", cmd.qstats_info.out->qmax);
        printf("Active H2C queues  : %u\n", cmd.qstats_info.out->active_h2c_queues);
        printf("Active C2H queues  : %u\n", cmd.qstats_info.out->active_c2h_queues);
        printf("Active CMPT queues : %u\n", cmd.qstats_info.out->active_cmpt_queues);
    }
    catch (const std::exception& e) {
        cout << "Failed to set qmax.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_add_queue(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_ADD;
    bool mode, qid, h2c_ring_sz, c2h_ring_sz, c2h_timer, c2h_th, c2h_buff_sz;
    bool cmplsz, trigmode;
    mode = qid = h2c_ring_sz = c2h_ring_sz = c2h_timer = c2h_th = c2h_buff_sz = false;
    cmplsz = trigmode = false;

    while (i < argc) {
        if (strcmp(argv[i], "mode") == 0) {
            ++i;

            if (strcmp(argv[i], "mm") == 0) {
                cmd.q_conf.in.is_st = 0;
            }
            else if (strcmp(argv[i], "st") == 0) {
                cmd.q_conf.in.is_st = 1;
            }
            else {
                cout << "Unknown command " << argv[i] << endl;
                return 1;
            }
            mode = true;
        }
        else if (strcmp(argv[i], "qid") == 0) {
            ++i;
            cmd.q_conf.in.qid = (unsigned short)strtoul(argv[i], NULL, 0);
            cout << "Adding queue ::" << cmd.q_conf.in.qid << endl;
            qid = true;
        }
        else if (strcmp(argv[i], "en_mm_cmpl") == 0) {
            cmd.q_conf.in.en_mm_cmpl = true;
        }
        else if (strcmp(argv[i], "idx_h2c_ringsz") == 0) {
            ++i;
            cmd.q_conf.in.h2c_ring_sz_index = (unsigned char)strtoul(argv[i], NULL, 0);
            if (MAX_VALID_GLBL_IDX < cmd.q_conf.in.h2c_ring_sz_index) {
                cout << "Invalid h2c ring size index : " << argv[i] << endl;
                return 1;
            }
            h2c_ring_sz = true;
        }
        else if (strcmp(argv[i], "idx_c2h_ringsz") == 0) {
            ++i;
            cmd.q_conf.in.c2h_ring_sz_index = (unsigned char)strtoul(argv[i], NULL, 0);
            if (MAX_VALID_GLBL_IDX < cmd.q_conf.in.c2h_ring_sz_index) {
                cout << "Invalid c2h ring size index : " << argv[i] << endl;
                return 1;
            }
            c2h_ring_sz = true;
        }
        else if (strcmp(argv[i], "idx_c2h_timer") == 0) {
            ++i;
            cmd.q_conf.in.c2h_timer_cnt_index = (unsigned char)strtoul(argv[i], NULL, 0);
            if (MAX_VALID_GLBL_IDX < cmd.q_conf.in.c2h_timer_cnt_index) {
                cout << "Invalid c2h timer index : " << argv[i] << endl;
                return 1;
            }
            c2h_timer = true;
        }
        else if (strcmp(argv[i], "idx_c2h_th") == 0) {
            ++i;
            cmd.q_conf.in.c2h_th_cnt_index = (unsigned char)strtoul(argv[i], NULL, 0);
            if (MAX_VALID_GLBL_IDX < cmd.q_conf.in.c2h_th_cnt_index) {
                cout << "Invalid c2h threshold index : " << argv[i] << endl;
                return 1;
            }
            c2h_th = true;
        }
        else if (strcmp(argv[i], "idx_c2h_bufsz") == 0) {
            ++i;
            cmd.q_conf.in.c2h_buff_sz_index = (unsigned char)strtoul(argv[i], NULL, 0);
            if (MAX_VALID_GLBL_IDX < cmd.q_conf.in.c2h_buff_sz_index) {
                cout << "Invalid c2h buffer size index : " << argv[i] << endl;
                return 1;
            }
            c2h_buff_sz = true;
        }
        else if (strcmp(argv[i], "cmptsz") == 0) {
            ++i;
            unsigned char compl_sz = (unsigned char)strtoul(argv[i], NULL, 0);
            if (compl_sz == 0)
                cmd.q_conf.in.compl_sz = CMPT_DESC_SZ_8B;
            else if (compl_sz == 1)
                cmd.q_conf.in.compl_sz = CMPT_DESC_SZ_16B;
            else if (compl_sz == 2)
                cmd.q_conf.in.compl_sz = CMPT_DESC_SZ_32B;
            else if (compl_sz == 3)
                cmd.q_conf.in.compl_sz = CMPT_DESC_SZ_64B;
            else {
                cout << "Invalid c2h completion size : " << argv[i] << endl;
                return 1;
            }
            cmplsz = true;
        }
        else if (strcmp(argv[i], "trigmode") == 0) {
            ++i;

            if (strcmp(argv[i], "every") == 0) {
                cmd.q_conf.in.trig_mode = TRIG_MODE_EVERY;
            }
            else if (strcmp(argv[i], "usr_cnt") == 0) {
                cmd.q_conf.in.trig_mode = TRIG_MODE_USER_COUNT;
            }
            else if (strcmp(argv[i], "usr") == 0) {
                cmd.q_conf.in.trig_mode = TRIG_MODE_USER;
            }
            else if (strcmp(argv[i], "usr_tmr") == 0) {
                cmd.q_conf.in.trig_mode = TRIG_MODE_USER_TIMER;
            }
            else if (strcmp(argv[i], "usr_tmr_cnt") == 0) {
                cmd.q_conf.in.trig_mode = TRIG_MODE_USER_TIMER_COUNT;
            }
            else {
                cout << "Invalid trigger mode : " << argv[i] << endl;
                return 1;
            }

            trigmode = true;
        }
        else if (strcmp(argv[i], "sw_desc_sz") == 0) {
            ++i;
            cmd.q_conf.in.sw_desc_sz = (unsigned char)strtoul(argv[i], NULL, 0);
            if (cmd.q_conf.in.sw_desc_sz != 3) {
                cout << "sw_desc_sz must be 3\n";
                cout << "Only 64B descriptor(3) can be configured in bypass mode\n";
                return 1;
            }
        }
        else if (strcmp(argv[i], "desc_bypass_en") == 0) {
            cmd.q_conf.in.desc_bypass_en = true;
        }
        else if (strcmp(argv[i], "pfch_en") == 0) {
            cmd.q_conf.in.pfch_en = true;
        }
        else if (strcmp(argv[i], "pfch_bypass_en") == 0) {
            cmd.q_conf.in.pfch_bypass_en = true;
        }
        else if (strcmp(argv[i], "cmpl_ovf_dis") == 0) {
            cmd.q_conf.in.cmpl_ovf_dis = true;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    if (!mode || !qid || !h2c_ring_sz || !c2h_ring_sz) {
        cout << "Insufficient options provided\n";
        return 1;
    }

    if (!cmd.q_conf.in.is_st &&
        (c2h_buff_sz || cmd.q_conf.in.pfch_en
            || cmd.q_conf.in.pfch_bypass_en)) {
        cout << "Invalid arguments for MM mode\n";
        return 1;
    }

    if ((!cmd.q_conf.in.desc_bypass_en) && (cmd.q_conf.in.sw_desc_sz == 3)) {
        cout << "Invalid Parameter Combination : ";
        cout << "desc_bypass_en : " << cmd.q_conf.in.desc_bypass_en << ", sw_desc_sz : " << cmd.q_conf.in.sw_desc_sz << endl;
        cout << "64 Byte Descriptor supported only when descriptor bypass is enabled\n";
    }

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, &cmd.q_conf.in, sizeof(cmd.q_conf.in), NULL, 0);
        cout << "Added Queue " << cmd.q_conf.in.qid << " Successfully\n";
    }
    catch (const std::exception& e) {
        cout << "Failed to add queue.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_start_queue(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_START;

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            ++i;
            cmd.q_conf.in.qid = (unsigned short)strtoul(argv[i], NULL, 0);
            cout << "Starting queue :: " << cmd.q_conf.in.qid << endl;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, &cmd.q_conf.in, sizeof(cmd.q_conf.in), NULL, 0);
        cout << "Started Queue " << cmd.q_conf.in.qid << " Successfully\n";
    }
    catch (const std::exception& e) {
        cout << "Failed to start queue.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_stop_queue(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_STOP;

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            ++i;
            cmd.q_conf.in.qid = (unsigned short)strtoul(argv[i], NULL, 0);
            cout << "Stopping queue : " << cmd.q_conf.in.qid << endl;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, &cmd.q_conf.in, sizeof(cmd.q_conf.in), NULL, 0);
        cout << "Stopped Queue " << cmd.q_conf.in.qid << " Successfully\n";
    }
    catch (const std::exception& e) {
        cout << "Failed to stop queue.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_delete_queue(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_DELETE;

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            ++i;
            cmd.q_conf.in.qid = (unsigned short)strtoul(argv[i], NULL, 0);
            cout << "Deleting queue :: " << cmd.q_conf.in.qid << endl;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    try {
        device_file device;

        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        device.ioctl(ioctl_code, &cmd.q_conf.in, sizeof(cmd.q_conf.in), NULL, 0);
        cout << "Deleted Queue " << cmd.q_conf.in.qid << " Successfully\n";
    }
    catch (const std::exception& e) {
        cout << "Failed to delete queue.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_queue_state(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_DUMP_STATE;

    if (i == argc) {
        cout << "insufficient argument\n";
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            ++i;
            cmd.q_state.in.qid = static_cast<unsigned short>(strtoul(argv[i], NULL, 0));
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        cmd.q_state.out = (struct queue_state_out *)new struct queue_state_out;

        if (device.ioctl(ioctl_code, &cmd.q_state.in, sizeof(cmd.q_state.in),
                cmd.q_state.out, sizeof(struct queue_state_out))) {
            printf("%5s%20s\n", "QID", "STATE");
            printf("%5s%20s\n", "---", "-----");
            printf("%5u%20s\n", cmd.q_state.in.qid, cmd.q_state.out->state);
        }
        delete cmd.q_state.out;
    }
    catch (const std::exception& e) {
        delete cmd.q_state.out;
        cout << "Failed to get queue state.\n" << e.what() << '\n';
    }

    return 0;
}
static int handle_queue_dump(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    bool qid_valid = false, dir_valid = false, desc_valid = false, cmpt_valid = false;
    bool type_valid = false;
    bool ctx_valid = false;
    unsigned short qid = 0;
    enum queue_direction dir = QUEUE_DIR_H2C;
    enum ring_type type = ring_type::RING_TYPE_H2C;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = 0;

    if (i == argc) {
        cout << "insufficient argument\n";
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            if (argv[i + 1] == NULL) {
                cout << "Insufficient options provided\n";
                cout << "qid option needs as an argument : <qid no>\n";
                return 1;
            }

            ++i;
            qid = static_cast<unsigned short>(strtoul(argv[i], NULL, 0));
            qid_valid = true;
        }
        else if (strcmp(argv[i], "dir") == 0) {
            if (argv[i + 1] == NULL) {
                cout << "Insufficient options provided\n";
                cout << "dir option needs as an argument : h2c/c2h\n";
                return 1;
            }

            ++i;
            if (strcmp(argv[i], "h2c") == 0) {
                dir = QUEUE_DIR_H2C;
            }
            else if (strcmp(argv[i], "c2h") == 0) {
                dir = QUEUE_DIR_C2H;
            }
            else {
                cout << "Invalid direction : " << argv[i] << endl;
                return 1;
            }
            dir_valid = true;
        }
        else if (strcmp(argv[i], "type") == 0) {
            if (argv[i + 1] == NULL) {
                cout << "Insufficient options provided\n";
                cout << "type option needs as an argument : h2c/c2h/cmpt\n";
                return 1;
            }

            ++i;
            if (strcmp(argv[i], "h2c") == 0) {
                type = ring_type::RING_TYPE_H2C;
            }
            else if (strcmp(argv[i], "c2h") == 0) {
                type = ring_type::RING_TYPE_C2H;
            }
            else if (strcmp(argv[i], "cmpt") == 0) {
                type = ring_type::RING_TYPE_CMPT;
            }
            else {
                cout << "Invalid ring type : " << argv[i] << endl;
                return 1;
            }
            type_valid = true;
        }
        else if (strcmp(argv[i], "desc") == 0) {
            if ((argv[i + 1] == NULL) || (argv[i + 2] == NULL)) {
                cout << "Insufficient options provided\n";
                cout << "desc option needs two arguments : <start desc no> and <end desc no>\n";
                return 1;
            }

            ioctl_code = IOCTL_QDMA_QUEUE_DUMP_DESC;
            cmd.desc_info.in.desc_type = RING_DESC;

            ++i;
            cmd.desc_info.in.desc_start = strtoul(argv[i], NULL, 0);

            ++i;
            cmd.desc_info.in.desc_end = strtoul(argv[i], NULL, 0);

            if (cmd.desc_info.in.desc_end < cmd.desc_info.in.desc_start) {
                cout << "Insufficient range : ";
                cout << cmd.desc_info.in.desc_start << "~" << cmd.desc_info.in.desc_end << endl;
                return 1;
            }

            desc_valid = true;
        }
        else if (strcmp(argv[i], "cmpt") == 0) {
            if ((argv[i + 1] == NULL) || (argv[i + 2] == NULL)) {
                cout << "Insufficient options provided\n";
                cout << "desc option needs two arguments : <start desc no> and <end desc no>\n";
                return 1;
            }

            ioctl_code = IOCTL_QDMA_QUEUE_DUMP_DESC;
            cmd.desc_info.in.desc_type = CMPT_DESC;

            ++i;
            cmd.desc_info.in.desc_start = strtoul(argv[i], NULL, 0);

            ++i;
            cmd.desc_info.in.desc_end = strtoul(argv[i], NULL, 0);

            if (cmd.desc_info.in.desc_end < cmd.desc_info.in.desc_start) {
                cout << "Insufficient range : ";
                cout << cmd.desc_info.in.desc_start << "~" << cmd.desc_info.in.desc_end << endl;
                return 1;
            }

            cmpt_valid = true;
        }
        else if (strcmp(argv[i], "ctx") == 0) {
            ioctl_code = IOCTL_QDMA_QUEUE_DUMP_CTX;
            ctx_valid = true;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    if (!qid_valid || (!dir_valid && !cmpt_valid && !type_valid) ||
        (!desc_valid && !cmpt_valid && !ctx_valid) ||
        (type_valid ^ ctx_valid)) {
        cout << "Insufficient options provided\n";
        return 1;
    }

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        unsigned int size;
        unsigned int out_size;

        if (ctx_valid) {
            cmd.ctx_info.in.qid = qid;
            cmd.ctx_info.in.type = type;
            /* Allocate memory for the maximum possible context size */
            size = MAX_DUMP_BUFF_SZ;

            out_size = sizeof(struct ctx_dump_info_out) + size;
            cmd.ctx_info.out = (struct ctx_dump_info_out *)new char[out_size];
            cmd.ctx_info.out->ret_sz = 0;

            device.ioctl(ioctl_code, &cmd.ctx_info.in, sizeof(cmd.ctx_info.in),
                cmd.ctx_info.out, out_size);

            if (!cmd.ctx_info.out->ret_sz) {
                printf("Failed to dump the queue context\n");
            }
            else {
                char *addr = (char *)&cmd.ctx_info.out->pbuffer[0];
                for (i = 0; ((i < (int)cmd.ctx_info.out->ret_sz) && (addr[i] != '\0')); i++) {
                    printf("%c", addr[i]);
                }
            }
            delete[] cmd.ctx_info.out;
        }
        else {
            cmd.desc_info.in.qid = qid;
            cmd.desc_info.in.dir = dir;
            /* Allocate memory for the maximum possible combination of desc size */
            size = (cmd.desc_info.in.desc_end - cmd.desc_info.in.desc_start + 1) * 64;

            out_size = sizeof(struct desc_dump_info_out) + size;
            cmd.desc_info.out = (struct desc_dump_info_out *)new unsigned char[out_size];
            cmd.desc_info.out->desc_sz = 0;
            cmd.desc_info.out->data_sz = 0;

            device.ioctl(ioctl_code, &cmd.desc_info.in, sizeof(cmd.desc_info.in),
                cmd.desc_info.out, out_size);

            if (!&cmd.desc_info.out->desc_sz || !cmd.desc_info.out->data_sz) {
                printf("Failed to dump the queue descriptors\n");
            }
            else {
                auto desc_index = cmd.desc_info.in.desc_start;
                auto desc_cnt = cmd.desc_info.out->data_sz / cmd.desc_info.out->desc_sz;
                auto n = cmd.desc_info.out->desc_sz / 4;
                UINT32 *addr = (UINT32 *)&cmd.desc_info.out->pbuffer[0];

                for (i = 0; i < (int)desc_cnt; i++) {
                    printf("%5d : ", desc_index++);
                    for (UINT16 j = 0; j < n; j++) {
                        printf("%08X ", addr[(i * n) + j]);
                    }
                    printf("\n");
                }
            }


            delete[] cmd.desc_info.out;
        }
    }
    catch (const std::exception& e) {
        if (ctx_valid)
            delete[] cmd.ctx_info.out;
        else
            delete[] cmd.desc_info.out;

        cout << "Failed to dump queue descriptors.\n" << e.what() << '\n';
    }

    return 0;

}

static int handle_queue_cmpt_read(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    bool qid_valid = false;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = 0x0;

    if (i == argc) {
        cout << "insufficient argument\n";
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "qid") == 0) {
            if (argv[i + 1] == NULL) {
                cout << "Insufficient options provided\n";
                cout << "qid option needs as an argument : <qid no>\n";
                return 1;
            }

            ++i;
            cmd.cmpt_info.in.qid = static_cast<unsigned short>(strtoul(argv[i], NULL, 0));
            qid_valid = true;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }

        ++i;
    }

    if (!qid_valid) {
        cout << "Insufficient options provided: qid option is must to execute cmpt_read command\n";
        return 1;
    }

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        unsigned int size = 10 * MAX_CMPT_DESC_SZ;
        ioctl_code = IOCTL_QDMA_QUEUE_CMPT_READ;

        unsigned int out_size = sizeof(struct cmpt_data_info_out) + size;
        cmd.cmpt_info.out = (struct cmpt_data_info_out *)new unsigned char[out_size];
        bool is_data_avail = false;
        UINT32 cmpt_data_idx = 0;
        do {
            cmd.cmpt_info.out->ret_len = 0;
            cmd.cmpt_info.out->cmpt_desc_sz = 0;

            memset(&cmd.cmpt_info.out->pbuffer[0], 0xDEADBEEF, size);

            device.ioctl(ioctl_code, &cmd.cmpt_info.in, sizeof(cmd.cmpt_info.in),
                cmd.cmpt_info.out, out_size);

            if (!cmd.cmpt_info.out->cmpt_desc_sz || !cmd.cmpt_info.out->ret_len) {
                if (is_data_avail == false)
                    printf("Completion Data not available\n");
            }
            else {
                is_data_avail = true;
                auto n = cmd.cmpt_info.out->cmpt_desc_sz / 4;
                auto desc_cnt = cmd.cmpt_info.out->ret_len / cmd.cmpt_info.out->cmpt_desc_sz;
                UINT32 *addr = (UINT32 *)&cmd.cmpt_info.out->pbuffer[0];
                for (i = 0; i < (int)desc_cnt; i++) {
                    printf("%5d : ", cmpt_data_idx++);
                    for (auto j = 0U; j < n; j++) {
                        printf("%08X ", addr[(i * n) + j]);
                    }
                    printf("\n");
                }
            }
        } while (cmd.cmpt_info.out->ret_len);

        delete[] cmd.cmpt_info.out;
    }
    catch (const std::exception& e) {
        delete[] cmd.cmpt_info.out;
        cout << "Failed to execute cmpt_read command.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_queue_listall(const char *dev_name)
{
    ioctl_cmd cmd = {};
    DWORD ioctl_code = IOCTL_QDMA_QUEUE_DUMP_STATE;

    try {
        device_file device;
        unsigned int qmax = 0, qbase = 0;
        bool ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        ret = get_qrange(dev_name, qmax, qbase);
        if (ret)
            return 1;

        cmd.q_state.out = (struct queue_state_out *)new struct queue_state_out;

        printf("%5s%20s\n", "QID", "STATE");
        printf("%5s%20s\n", "---", "-----");
        for (unsigned short i = 0; i < qmax; i++) {
            memset(cmd.q_state.out, 0, sizeof(struct queue_state_out));

            cmd.q_state.in.qid = i;

            if (device.ioctl(ioctl_code, &cmd.q_state.in, sizeof(cmd.q_state.in),
                cmd.q_state.out, sizeof(struct queue_state_out))) {
                printf("%5u%20s\n", cmd.q_state.in.qid, cmd.q_state.out->state);
            }
        }
        delete cmd.q_state.out;
    }
    catch (const std::exception& e) {
        delete cmd.q_state.out;
        cout << "Failed to get queue state.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_intring_dump(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;
    bool vector_valid = false;
    bool index_valid = false;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = 0x0;

    if (i == argc) {
        cout << "insufficient argument\n";
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "vector") == 0) {
            if ((argv[i + 1] == NULL) || (argv[i + 2] == NULL) || (argv[i + 3] == NULL)) {
                cout << "Insufficient options provided\n";
                cout << "vector option needs as three arguments : <vec id> <start idx> <end idx>\n";
                return 1;
            }

            ++i;
            cmd.int_ring_info.in.vec_id = strtoul(argv[i], NULL, 0);
            if (MAX_VALID_INTR_RING_VEC < cmd.int_ring_info.in.vec_id) {
                cout << "Invalid intr vector id : " << argv[i] << endl;
                break;
            }
            vector_valid = true;

            ++i;
            cmd.int_ring_info.in.start_idx = strtoul(argv[i], NULL, 0);
            ++i;
            cmd.int_ring_info.in.end_idx = strtoul(argv[i], NULL, 0);
            if (cmd.int_ring_info.in.start_idx > cmd.int_ring_info.in.end_idx) {
                cout << "Start index must be less than or equal to end index" << endl;
                break;
            }
            index_valid = true;
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
        ++i;
    }

    if (!vector_valid || !index_valid) {
        cout << "Insufficient options provided\n";
        return 1;
    }

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        unsigned int size =
                (cmd.int_ring_info.in.end_idx - cmd.int_ring_info.in.start_idx + 1) * MAX_INTR_RING_ENTRY_SZ;

        unsigned int out_size = sizeof (struct intring_info_out) + size;

        cmd.int_ring_info.out = (struct intring_info_out *)new unsigned char[out_size];

        cmd.int_ring_info.out->ret_len = 0;
        cmd.int_ring_info.out->ring_entry_sz = 0;

        ioctl_code = IOCTL_QDMA_INTRING_DUMP;

        device.ioctl(ioctl_code, &cmd.int_ring_info.in, sizeof(cmd.int_ring_info.in),
            cmd.int_ring_info.out, out_size);

        if (!cmd.int_ring_info.out->ring_entry_sz || !cmd.int_ring_info.out->ret_len) {
            printf("Failed to dump the intr ring\n");
        }
        else {
            auto start_idx = cmd.int_ring_info.in.start_idx;
            auto ring_entry_cnt = cmd.int_ring_info.out->ret_len / cmd.int_ring_info.out->ring_entry_sz;
            auto n = cmd.int_ring_info.out->ring_entry_sz / 4;
            UINT32 *addr = (UINT32 *)&cmd.int_ring_info.out->pbuffer[0];
            for (i = 0; i < (int)ring_entry_cnt; i++) {
                printf("%5d : ", start_idx++);
                for (UINT16 j = 0; j < n; j++) {
                    printf("%08X ", addr[(i * n) + j]);
                }
                printf("\n");
            }
        }
        delete[] cmd.int_ring_info.out;
    }
    catch (const std::exception& e) {
        delete[] cmd.int_ring_info.out;
        cout << "Failed to execute intring dump command.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_reg_dump(const char *dev_name, const int argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    ioctl_cmd cmd = {};
    DWORD ioctl_code = 0x0;

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        unsigned int size = MAX_DUMP_BUFF_SZ;

        unsigned int out_size = sizeof(struct regdump_info_out) + size;
        cmd.reg_dump_info.out = (struct regdump_info_out *)new char[out_size];
        cmd.reg_dump_info.out->ret_len = 0;

        ioctl_code = IOCTL_QDMA_REG_DUMP;

        device.ioctl(ioctl_code, NULL, 0, cmd.reg_dump_info.out, out_size);

        if (!cmd.reg_dump_info.out->ret_len) {
            printf("Failed to dump the registers\n");
        }
        else {
            char *addr = (char *)&cmd.reg_dump_info.out->pbuffer[0];
            for (auto i = 0; ((i < (int)cmd.reg_dump_info.out->ret_len) && (addr[i] != '\0')); i++) {
                printf("%c", addr[i]);
            }
        }
        delete[] cmd.reg_dump_info.out;
    }
    catch (const std::exception& e) {
        delete[] cmd.reg_dump_info.out;
        cout << "Failed to execute reg dump command.\n" << e.what() << '\n';
    }

    return 0;
}


static int handle_reg_info(const char* dev_name, const int argc, char* argv[])
{
    UNREFERENCED_PARAMETER(dev_name);
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    auto i = 0;
    bool bar_valid = false;
    ioctl_cmd cmd = {};
    DWORD ioctl_code = 0x0;

    if (i == argc) {
        cout << "insufficient arguments\n";
        return 1;
    }

    while (i < argc) {
        if (strcmp(argv[i], "bar") == 0) {
            if ((argv[i + 1] == NULL) || (argv[i + 2] == NULL)) {
                cout << "Insufficient options provided\n";
                cout << "bar option needs atleast two arguments : <bar_num> <address> [num_regs <M>]\n";
                return 1;
            }

            ++i;
            cmd.reg_info.in.bar_no = strtoul(argv[i], NULL, 0);
            if ((MAX_VALID_BAR_NUM < cmd.reg_info.in.bar_no) && 
                ((cmd.reg_info.in.bar_no % 2) != 0)) {
                cout << "Invalid BAR number provided : " << argv[i] << endl;
                break;
            }
            cmd.reg_info.in.bar_no = cmd.reg_info.in.bar_no / 2;
            bar_valid = true;

            ++i;
            cmd.reg_info.in.address = strtoul(argv[i], NULL, 0);

            ++i;
            if ((argv[i] != NULL) && (strcmp(argv[i], "num_regs")) == 0) {
                cmd.reg_info.in.reg_cnt = strtoul(argv[i + 1], NULL, 0);
                ++i;
            }
            else {
                cmd.reg_info.in.reg_cnt = 1;
            }
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
        ++i;
    }

    if (!bar_valid) {
        return 1;
    }

    try {
        device_file device;
        auto ret = open_device(dev_name, device);
        if (ret == false)
            return 0;

        unsigned int size = (cmd.reg_info.in.reg_cnt * MAX_REG_INFO_SZ);
        unsigned int out_size = sizeof(struct reg_info_out) + size;
        cmd.reg_info.out = (struct reg_info_out *)new char[out_size];
        cmd.reg_info.out->ret_len = 0;

        ioctl_code = IOCTL_QDMA_REG_INFO;

        device.ioctl(ioctl_code, &cmd.reg_info.in, sizeof(cmd.reg_info.in),
            cmd.reg_info.out, out_size);

        if (!cmd.reg_info.out->ret_len) {
            printf("Failed to dump the registers\n");
        }
        else {
            char* data = (char*)&cmd.reg_info.out->pbuffer[0];
            for (i = 0; ((i < (int)cmd.reg_info.out->ret_len) &&
                (data[i] != '\0')); i++) {
                printf("%c", data[i]);
            }
        }
        delete[] cmd.reg_info.out;
    }
    catch (const std::exception& e) {
        delete[] cmd.reg_info.out;
        cout << "Failed to execute reg info command.\n" << e.what() << '\n';
    }

    return 0;
}

static int handle_queue_cmds(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;

    while (i < argc) {
        if (strcmp(argv[i], "add") == 0) {
            ++i;
            return handle_add_queue(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "start") == 0) {
            ++i;
            return handle_start_queue(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "stop") == 0) {
            ++i;
            return handle_stop_queue(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "delete") == 0) {
            ++i;
            return handle_delete_queue(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "state") == 0) {
            ++i;
            return handle_queue_state(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "dump") == 0) {
            ++i;
            return handle_queue_dump(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "cmpt_read") == 0) {
            ++i;
            return handle_queue_cmpt_read(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "listall") == 0) {
            ++i;
            return handle_queue_listall(dev_name);
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
    }

    return 0;
}

static int handle_intring_cmds(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;

    while (i < argc) {
        if (strcmp(argv[i], "dump") == 0) {
            ++i;
            return handle_intring_dump(dev_name, argc - i, argv + i);
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
    }
    return 0;
}

static int handle_reg_cmds(const char *dev_name, const int argc, char* argv[])
{
    auto i = 0;

    while (i < argc) {
        if (strcmp(argv[i], "dump") == 0) {
            ++i;
            return handle_reg_dump(dev_name, argc - i, argv + i);
        }
        else if (strcmp(argv[i], "info") == 0) {
            ++i;
            return handle_reg_info(dev_name, argc - i, argv + i);
        }
        else {
            cout << "Unknown command " << argv[i] << endl;
            return 1;
        }
    }
    return 0;
}

static int process_cli(const int argc, char* argv[])
{
    auto i = 1;
    char dev_name[DEV_NAME_MAX_SZ];

    if (strcmp(argv[1], "dev") == 0) {
        ++i;
        return handle_dev_cmds(argc - i, argv + i);
    }
    else if (strncmp(argv[1], "qdma", 4) == 0) {
        strncpy_s(dev_name, DEV_NAME_MAX_SZ, argv[1], _TRUNCATE);
        ++i;
    }
    else if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "/?") == 0)) {
        help();
        return 0;
    }
    else if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "-V") == 0)) {
        printf("%s version %s\n", PROGNAME, VERSION);
        printf("%s\n", COPYRIGHT);
        return 0;
    }

    if (strcmp(argv[i], "csr") == 0) {
        ++i;
        return handle_csr_cmds(dev_name, argc - i, argv + i);
    }
    else if (strcmp(argv[i], "devinfo") == 0) {
        ++i;
        return handle_devinfo(dev_name);
    }
    else if (strcmp(argv[i], "qmax") == 0) {
        ++i;
        return handle_qmax(dev_name, argc - i, argv + i);
    }
    else if (strcmp(argv[i], "qstats") == 0) {
        ++i;
        return handle_qstats(dev_name);
    }
    else if (strcmp(argv[i], "queue") == 0) {
        ++i;
        return handle_queue_cmds(dev_name, argc - i, argv + i);
    }
    else if (strcmp(argv[i], "intring") == 0) {
        ++i;
        return handle_intring_cmds(dev_name, argc - i, argv + i);
    }
    else if (strcmp(argv[i], "reg") == 0) {
        ++i;
        return handle_reg_cmds(dev_name, argc - i, argv + i);
    }
    else {
        cout << "Unknown command " << argv[i] << endl;
        return 1;
    }
}

static void help(void)
{
    cout << "usage: dma-ctl [dev | qdma<N>] [operation]\n";
    cout << "       dma-ctl -h - Prints this help\n";
    cout << "       dma-ctl -v - Prints the version information\n";
    cout << "\n";
    cout << "dev [operation]\t: system wide FPGA operations\n";
    cout << "     list\t: list all qdma functions\n";
    cout << "\n";
    cout << "qdma<N> [operation]\t: per QDMA FPGA operations\n";
    cout << "    <N>\t\t: N is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No\n";
    cout << "    csr <dump>\n";
    cout << "         dump\t: Dump QDMA CSR Information\n";
    cout << "    \n";
    cout << "    devinfo\t: lists the Hardware and Software version and capabilities\n";
    cout << "    qmax <N>\t: Assign maximum number of queues for a device\n";
    cout << "    qstats\t: Dump number of available and free queues\n";
    cout << "    \n";
    cout << "    queue <add|start|stop|delete|listall>\n";
    cout << "           add mode <mm|st> qid <N> [en_mm_cmpl] idx_h2c_ringsz <0:15> idx_c2h_ringsz <0:15>\n";
    cout << "               [idx_c2h_timer <0:15>] [idx_c2h_th <0:15>] [idx_c2h_bufsz <0:15>] [cmptsz <0|1|2|3>] - add a queue\n";
    cout << "               [trigmode <every|usr_cnt|usr|usr_tmr|usr_tmr_cnt>] [sw_desc_sz <3>] [desc_bypass_en] [pfch_en] [pfch_bypass_en]\n";
    cout << "               [cmpl_ovf_dis]\n";
    cout << "           start qid <N> - start a single queue\n" ;
    cout << "           stop qid <N> - stop a single queue\n";
    cout << "           delete qid <N> - delete a queue\n";
    cout << "           state qid <N> - print the state of the queue\n";
    cout << "           dump qid <N> dir <h2c|c2h> desc <start> <end> - dump desc ring entries <start> to <end>\n";
    cout << "           dump qid <N> cmpt <start> <end> - dump completion ring entries <start> to <end>\n";
    cout << "           dump qid <N> ctx type <h2c|c2h|cmpt> - dump context information of <qid>\n";
    cout << "           cmpt_read qid <N> - Read the completion data\n";
    cout << "    intring dump vector <N> <start_idx> <end_idx> - interrupt ring dump for vector number <N>\n";
    cout << "                                                    for intrrupt entries :<start_idx> --- <end_idx>\n";
    cout << "    reg dump - register dump\n";
    cout << "    reg info bar <N> addr [num_regs <M>] - dump detailed fields information of a register\n";
}

int __cdecl main(const int argc, char* argv[])
{
    try {
        if (argc > 1) {
            /* Pasre command line options */
            if (process_cli(argc, argv)) {
                help();
            }
        }
        else {
            help();
        }
    }
    catch (const std::exception& e) {
        cout << "Error: " << e.what() << '\n';
    }
}
