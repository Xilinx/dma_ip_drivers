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

#include <array>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <numeric>

#include "device_file.hpp"
#include "qdma_driver_api.h"

#pragma comment(lib, "setupapi.lib")

using std::uint32_t;
using std::string;
using std::runtime_error;
using std::cout;
using namespace std;

#define ST_C2H_MAX_PACK_SIZE_CHUNK      4096
#define COMPL_WAIT_TIME                 100 /* Milli Seconds */
#define MAX_TIMEOUT_PERIOD              1000 /* Milli Seconds */
#define FILE_NORMAL_MODE                0
#define FILE_EX_MODE                    1

/* -----  common functions -----  */
static constexpr uint32_t bit(uint64_t n) {
    return (1ull << n);
}

static constexpr bool is_bit_set(uint64_t x, uint64_t n) {
    return (x & bit(n)) == bit(n);
}

static constexpr uint64_t create_mask(uint64_t n, uint64_t l) {
    return ((1ull << l) - 1ull) << n;
}

static constexpr uint32_t get_bits(uint32_t value, uint32_t bit_index, uint32_t len) {
    return ((value & create_mask(bit_index, len)) >> bit_index);
}
/* ----- ----- */

enum class devnode_sel {
    queue_mm,
    queue_st,
    none
};

enum class op_sel {
    read,
    write,
    interrupt,
    none
};

struct cli_cmd {
    op_sel op;
    UINT8 bus_no;
    UINT8 dev_no;
    UINT8 fun_no;
    char dev_name[DEV_NAME_MAX_SZ];
    devnode_sel node;
    BYTE *data;
    LONG addr;
    ULONG qid;
    int mode;
    DWORD size;
    string file;
    size_t alignment;
    BOOL binary;
};

template<typename T, size_t Alignment = 4096>
struct aligned_allocator {

    typedef T value_type;

    aligned_allocator() noexcept {}

    template<class U>
    aligned_allocator(const aligned_allocator<U, Alignment>&) noexcept {}

    template<class U>
    bool operator==(const aligned_allocator<U>&) const noexcept
    {
        return true;
    }
    template<class U>
    bool operator!=(const aligned_allocator<U>&) const noexcept
    {
        return false;
    }

    T* allocate(const std::size_t num) {
        if (num == 0) {
            return nullptr;
        }

        void* const ptr = _aligned_malloc(num * sizeof(T), Alignment);
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }
    void deallocate(T* const p, std::size_t) {
        if (p) {
            _aligned_free(p);
        }
    }

    template<typename T2>
    struct rebind {
        typedef aligned_allocator<T2, Alignment> other;
    };
};

template<typename T, size_t N = 4096>
using aligned_vector = std::vector<T, aligned_allocator<T, N>>;

template<typename Iter>
void fill_pattern(Iter begin, Iter end) {
    std::iota(begin, end, 0); // 0, 1, 2, 3, 4 ...
}

class data_generator {
public:
    using value_type = uint16_t;

    explicit data_generator(const std::string& dev_path)
    {
        user_bar.open(dev_path + "\\user", GENERIC_WRITE | GENERIC_READ);
    }

    ~data_generator()
    {
        user_bar.close();
    }

    bool check_h2c(const uint32_t qid)
    {
        const auto match_reg = user_bar.read<uint32_t>(0x10);
        const auto match_qid = get_bits(match_reg, 4, 28);      /* bits 4-31 = qid */
        return (match_qid == qid) && is_bit_set(match_reg, 0);  /* bit 0 = match */
    }

    void reset_h2c()
    {
        user_bar.write<uint32_t>(0xC, 0x1);
    }

    void reset_pkt_ctrl()
    {
        user_bar.write<uint32_t>(0x8, 0x0);
    }

    void set_queue(const uint32_t qid)
    {
        user_bar.write<uint32_t>(0x0, qid);
    }

    void configure_c2h(const uint32_t qid, const uint32_t packet_size, const uint32_t num_packets)
    {
        //printf("DGEN : PKT SIZE : %d, PKT CNT : %d\n", packet_size, num_packets);
        set_queue(qid);
        user_bar.write<uint32_t>(0x4, packet_size);
        user_bar.write<uint32_t>(0x20, num_packets);
    }

    void generate_packets()
    {
        user_bar.write<uint32_t>(0x8, 0x2);
    }

    BOOL read(void *buff, LONG addr, DWORD size)
    {
        if (INVALID_SET_FILE_POINTER == SetFilePointer(user_bar.h, static_cast<LONG>(addr), nullptr, FILE_BEGIN)) {
            return FALSE;
        }

        return ReadFile(user_bar.h, buff, size, &size, NULL);
    }

    BOOL write(void *buff, LONG addr, DWORD size)
    {
        if (INVALID_SET_FILE_POINTER == SetFilePointer(user_bar.h, static_cast<LONG>(addr), nullptr, FILE_BEGIN)) {
            return FALSE;
        }

        return WriteFile(user_bar.h, buff, size, &size, NULL);
    }

    BOOL generate_user_interrupt(UINT32 fun_id)
    {
        UINT32 val = 0x0;

        val = 0x00;
        user_bar.write<uint32_t>(0x98, val);

        user_bar.write<uint32_t>(0x9C, val);

        val = 0x0;
        val = fun_id << 12;
        val |= 0x1;
        if (fun_id == 0)
            val |= (1u << 4);
        user_bar.write<uint32_t>(0x94, val);

        return TRUE;
    }

private:
    device_file user_bar;
};

class qdma_device {
public:
    HANDLE cmplt_th_handle;
    HANDLE cmplt_io_handle;
    bool thread_run = false;
    bool thread_exit = false;
    bool xfer_done = false;
    HANDLE sem_xfer_done;
    ULONG xfer_size;
    ULONG rxd_size;

    void async_env_init(void);
    void async_env_exit(void);
    void poll_completion(void);

    explicit qdma_device(const char* device_path) : dgen(device_path)
    {
        using namespace std::string_literals;
        dev_path = device_path;
        control_bar_.open(dev_path + "\\control"s, GENERIC_READ | GENERIC_WRITE);
        mgmt.open(dev_path + "\\mgmt"s, GENERIC_READ | GENERIC_WRITE);
        get_qstats();
    }

    ~qdma_device()
    {
        control_bar_.close();
        mgmt.close();
    }

    void get_qstats(void)
    {
        struct qstat_out qstats_info = { 0 };
        try {
            mgmt.ioctl(IOCTL_QDMA_GET_QSTATS, NULL, 0, &qstats_info, sizeof(qstats_info));
            qbase = qstats_info.qbase;
            qmax = qstats_info.qmax;
        }
        catch (const std::exception& e) {
            cout << "IOCTL Failed for qstats " << e.what() << '\n';
        }
    }

    unsigned int get_qmax()
    {
        return qmax;
    }

    int qopen(ULONG qid, bool is_mm)
    {
        DWORD f_attribs = (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
        if (true == is_mm)
            queue_.open(dev_path + "\\queue_"s + to_string(qid), GENERIC_READ | GENERIC_WRITE, f_attribs);
        else
            queue_.open(dev_path + "\\st_"s + to_string(qid), GENERIC_READ | GENERIC_WRITE, f_attribs);

        async_env_init();
        return 0;
    }

    void qclose(void)
    {
        async_env_exit();
        queue_.close();
    }

    int qdma_read();
    int qdma_write();
    int qdma_interrupt(void);

private:
    const char* dev_path;
    device_file control_bar_;
    data_generator dgen;
    device_file queue_;
    device_file mgmt;
    unsigned int qbase;
    unsigned int qmax;
};

static BYTE* allocate_buffer(size_t size, size_t alignment);
static void print_bytes(size_t addr, BYTE* data, size_t length);
static int read_file_option(void);
static void print_bytes_binary(BYTE* data, size_t length);