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

#include "dma_rw.hpp"
#include "version.h"

cli_cmd cmd;

int qdma_device::qdma_read(void )
{
    cmd.data = allocate_buffer(cmd.size, cmd.alignment);
    if (!cmd.data) {
        throw std::runtime_error("Error allocating memory" + get_last_win_err_msg());
    }

    switch (cmd.node) {
    case devnode_sel::control:
        if (INVALID_SET_FILE_POINTER == SetFilePointer(control_bar_.h, static_cast<LONG>(cmd.addr), nullptr, FILE_BEGIN)) {
            throw runtime_error("SetFilePointer failed: " + std::to_string(GetLastError()));
        }
        if (!ReadFile(control_bar_.h, cmd.data, cmd.size, &cmd.size, NULL)) {
            throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }
        print_bytes(cmd.addr, cmd.data, cmd.size);
        break;

    case devnode_sel::user:
        if (false == dgen.read(cmd.data, cmd.addr, cmd.size)) {
            throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }
        print_bytes(cmd.addr, cmd.data, cmd.size);
        break;

    case devnode_sel::queue_mm:
        qopen(cmd.qid, true);

        if (INVALID_SET_FILE_POINTER == SetFilePointer(queue_.h, static_cast<LONG>(cmd.addr), nullptr, FILE_BEGIN)) {
            throw runtime_error("SetFilePointer failed: " + std::to_string(GetLastError()));
        }
        if (!ReadFile(queue_.h, cmd.data, cmd.size, &cmd.size, NULL)) {
            throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }

        if (TRUE == cmd.binary)
            print_bytes_binary(cmd.data, cmd.size);
        else
            print_bytes(cmd.addr, cmd.data, cmd.size);

        qclose();

        break;

    case devnode_sel::queue_st:
        {
            DWORD packet_size;
            DWORD packet_cnt;
            DWORD remain_size = cmd.size;
            DWORD rxd_size = 0;
            DWORD buf_idx = 0;
            bool  pkt_generate = true;
            bool  last_pkt_generate = true;

            qopen(cmd.qid, false);
            do {
                if (remain_size >= ST_C2H_MAX_PACK_SIZE_CHUNK) {
                    packet_cnt = remain_size / ST_C2H_MAX_PACK_SIZE_CHUNK;
                    packet_size = ST_C2H_MAX_PACK_SIZE_CHUNK;
                }
                else {
                    if (last_pkt_generate) {
                        pkt_generate = true;
                        last_pkt_generate = false;
                    }
                    packet_cnt = 1;
                    packet_size = remain_size;
                }

                if (cmd.size == 0x0) {
                    cout << "Initiating Zero Byte Read\n";
                }

                if (pkt_generate) {
                    dgen.reset_pkt_ctrl();
                    dgen.configure_c2h(cmd.qid + qbase, packet_size, packet_cnt);
                    dgen.generate_packets();
                    pkt_generate = false;
                }

                if (!ReadFile(queue_.h, &cmd.data[buf_idx], (packet_size * packet_cnt), &rxd_size, NULL)) {
                    throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
                }

                buf_idx = buf_idx + rxd_size;
                remain_size = remain_size - rxd_size;

                if (cmd.size == 0x0) {
                    cout << "ZERO Byte Read Successful\n";
                }
            } while (remain_size);
            qclose();

            if (TRUE == cmd.binary)
                print_bytes_binary(cmd.data, cmd.size);
            else
                print_bytes(cmd.addr, cmd.data, cmd.size);
        }

        break;

    default:
        return 1;
    }

    if (cmd.data) _aligned_free(cmd.data);
    return 0;
}

int qdma_device::qdma_write(void )
{
    switch (cmd.node) {
    case devnode_sel::control:
        if (INVALID_SET_FILE_POINTER == SetFilePointer(control_bar_.h, static_cast<LONG>(cmd.addr), nullptr, FILE_BEGIN)) {
            throw runtime_error("SetFilePointer failed: " + std::to_string(GetLastError()));
        }
        if (!WriteFile(control_bar_.h, cmd.data, cmd.size, &cmd.size, NULL)) {
            throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }
        break;

    case devnode_sel::user:
        if (false == dgen.write(cmd.data, cmd.addr, cmd.size)) {
            throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }
        break;

    case devnode_sel::queue_mm:
        qopen(cmd.qid, true);

        if (cmd.data == nullptr && false == cmd.file.empty()) {
            read_file_option();
        }

        if (INVALID_SET_FILE_POINTER == SetFilePointer(queue_.h, static_cast<LONG>(cmd.addr), nullptr, FILE_BEGIN)) {
            throw runtime_error("SetFilePointer failed: " + std::to_string(GetLastError()));
        }
        if (!WriteFile(queue_.h, cmd.data, cmd.size, &cmd.size, NULL)) {
            throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
        }

        qclose();

        break;

    case devnode_sel::queue_st:
    {
        qopen(cmd.qid, false);
        aligned_vector<uint16_t> wr_buffer(cmd.size);
        iota(wr_buffer.begin(), wr_buffer.end(), static_cast<uint16_t>(0)); /* 0, 1, 2, 3, 4 ... */
        dgen.reset_h2c();

        if (cmd.size == 0x0) {
            cout <<"Initiating Zero Byte Write\n";
        }

        if (!WriteFile(queue_.h, wr_buffer.data(), cmd.size, &cmd.size, NULL)) {
            throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
        }

        if (true == dgen.check_h2c(cmd.qid + qbase))
            cout << "Data generator returned SUCCESS for received data" << endl;
        else
            throw std::runtime_error("Data generator reported error for received data!");

        if (cmd.size == 0x0) {
            cout << "ZERO Byte Write Successful\n";
        }

        qclose();
        break;
    }
    default:
        return 1;
    }

    if (cmd.data) _aligned_free(cmd.data);
    return 0;
}

int qdma_device::qdma_interrupt(void)
{
    switch (cmd.node) {
    case devnode_sel::user:
        if (false == dgen.generate_user_interrupt(cmd.fun_no)) {
            throw std::runtime_error("Failed to generate user interrupt from device! " + get_last_win_err_msg());
        }
        break;

    default:
        throw std::runtime_error("Can not generate user interrupt for wrong target! " + get_last_win_err_msg());
    }

    return 0;
}

static int is_printable(char c)
{
    /* anything below ASCII code 32 is non-printing, 127 is DELETE */
    if (c < 32 || c == 127) {
        return FALSE;
    }
    return TRUE;
}

static void print_bytes_binary(BYTE* data, size_t length)
{
    FILE* output = stdout;

    if (false == cmd.file.empty()) {
        fopen_s(&output, cmd.file.data(), "wb");
    }

    fwrite(data, sizeof(BYTE), length, output);

    if (false == cmd.file.empty()) {
        fclose(output);
    }
}

static void print_bytes(size_t addr, BYTE* data, size_t length)
{
    FILE* output = stdout;

    if (false == cmd.file.empty()) {
        fopen_s(&output, cmd.file.data(), "wb");
    }

    /* formatted output */
    for (int row = 0; row < (int)(length / 16 + ((length % 16) ? 1 : 0));
        row++) {

        /* Print address */
        fprintf(output, "0x%04zX:  ", addr + row * 16);

        /* Print bytes */
        int column;
        for (column = 0; column < (int)min(16, length - (row * 16));
            column++) {
            fprintf(output, "%02x ", data[(row * 16) + column]);
        }
        for (; column < 16; column++) {
            fprintf(output, "   ");
        }

        /* Print gutter */
        fprintf(output, "    ");

        /* Print characters */
        for (column = 0; column < (int)min(16, length - (row * 16));
            column++) {
            fprintf(output, "%c", is_printable(data[(row * 16) + column]) ?
                (data[(row * 16) + column]) : '.');
        }
        for (; column < 16; column++) {
            fprintf(output, " ");
        }
        fprintf(output, "\n");
    }

    if (false == cmd.file.empty()) {
        fclose(output);
    }
}

static int read_file_option(void)
{
    FILE* inputFile;
    if (fopen_s(&inputFile, cmd.file.data(), "rb") != 0) {
        fprintf(stderr, "Could not open file <%s>\n", cmd.file.data());
        return 1;
    }

    /* determine file size */
    if (cmd.size == 0) {
        fseek(inputFile, 0, SEEK_END);
        fpos_t fpos;
        fgetpos(inputFile, &fpos);
        fseek(inputFile, 0, SEEK_SET);
        cmd.size = (DWORD)fpos;
    }

    cmd.data = allocate_buffer(cmd.size, cmd.alignment);
    if (!cmd.data) {
        fprintf(stderr, "Error allocating %ld bytes of memory, error code: %ld\n", cmd.size, GetLastError());
        return 1;
    }
    cmd.size = (DWORD)fread(cmd.data, 1, cmd.size, inputFile);

    fclose(inputFile);
    return 0;
}

static void help(void)
{
    cout << "dma-rw usage:\n";
    cout << "dma-rw -v  : prints the version information\n\n";
    cout << "dma-rw qdma<N> <DEVNODE> <read|write> <ADDR> [OPTIONS] [DATA]\n\n";
    cout << "- qdma<N>   : unique qdma device name (<N> is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No)\n";
    cout << "- DEVNODE   : One of: control | user | queue_mm_ | queue_st_*\n";
    cout << "              where the * is a numeric wildcard (0 - N) for queue.\n";
    cout << "- ADDR      : The target offset address of the read/write operation.\n";
    cout << "              Applicable only for control, user, queue_mm device nodes.\n";
    cout << "              Can be in hex or decimal.\n";
    cout << "- OPTIONS   : \n";
    cout << "              -a set alignment requirement for host-side buffer (default: PAGE_SIZE)\n";
    cout << "              -b open file as binary\n";
    cout << "              -f use contents of file as input or write output into file.\n";
    cout << "              -l length of data to read/write (default: 4 bytes or whole file if '-f' flag is used)\n";
    cout << "- DATA      : Space separated bytes (big endian) in decimal or hex, \n";
    cout << "              e.g.: 17 34 51 68\n";
    cout << "              or:   0x11 0x22 0x33 0x44\n";
}

static void init_cmd(void)
{
    cmd.addr = 0x0;
    cmd.alignment = 0;
    cmd.binary = false;
    cmd.node = devnode_sel::none;
    cmd.op = op_sel::none;
    cmd.bus_no = 0;
    cmd.dev_no = 0;
    cmd.fun_no = 0;
    cmd.dev_name[0] = '\0';
    cmd.qid = 0;
    cmd.size = 4;
    cmd.data = nullptr;
}

static BYTE* allocate_buffer(size_t size, size_t alignment)
{
    if (size == 0) {
        size = 4;
    }

    if (alignment == 0) {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        alignment = sys_info.dwPageSize;
    }

    return (BYTE*)_aligned_malloc(size, alignment);
}

int parse_command_line(const int argc, char* argv[])
{
    /*
     * dma-rw pf <pf_number> <device> <read|write> <address> [OPTIONS] [DATA]
     * 0       1      2          3          4           5        6...     n...
     */
    auto argidx = 1;

    if (argc < 2) {
        return 1;
    }

    if ((strcmp(argv[argidx], "-v") == 0) || (strcmp(argv[argidx], "-V") == 0)) {
        printf("%s version %s\n", PROGNAME, VERSION);
        printf("%s\n", COPYRIGHT);
        exit(0);
    }

    init_cmd();

    if (argc < 5) {
        return 1;
    }

    if (strncmp(argv[argidx], "qdma", 4)) {
        return 1;
    }

    strncpy_s(cmd.dev_name, DEV_NAME_MAX_SZ, argv[argidx], _TRUNCATE);
    ++argidx;

    if (strcmp(argv[argidx], "control") == 0) {
        cmd.node = devnode_sel::control;
    }
    else if (strcmp(argv[argidx], "user") == 0) {
        cmd.node = devnode_sel::user;

        if (strcmp(argv[argidx + 1], "interrupt") == 0) {
            cmd.op = op_sel::interrupt;
            return 0;
        }
    }
    else if (std::regex_match(argv[argidx], std::regex("queue_mm_[0-9]+"))) {
        string qid_str{ argv[argidx] };
        auto it = std::find(qid_str.begin(), qid_str.end(), '_');
        it = it+4;
        cout << *it << endl;
        cmd.qid = std::stoul(string{ it, qid_str.end() });
        cmd.node = devnode_sel::queue_mm;
    }
    else if (std::regex_match(argv[argidx], std::regex("queue_st_[0-9]+"))) {
        string qid_str{ argv[argidx] };
        auto it = std::find(qid_str.begin(), qid_str.end(), '_');
        it = it + 4;
        cmd.qid = std::stoul(string{ it, qid_str.end() });
        cmd.node = devnode_sel::queue_st;
    }
    else {
        return 1;
    }
    ++argidx;

    if (strcmp(argv[argidx], "read") == 0) {
        cmd.op = op_sel::read;
    }
    else if (strcmp(argv[argidx], "write") == 0) {
        cmd.op = op_sel::write;
    }
    else
        return 1;
    ++argidx;

    if (cmd.node == devnode_sel::queue_mm || cmd.node == devnode_sel::control || cmd.node == devnode_sel::user) {
        cmd.addr = strtoul(argv[argidx], NULL, 0);
        ++argidx;
    }
    else if (cmd.node == devnode_sel::queue_st) {
        if (argv[argidx][0] != '-') {
            return 1;
        }
    }

    while ((argidx < argc) && ((argv[argidx][0] == '-') || (argv[argidx][0] == '/'))) {
        switch (argv[argidx][1]) {
        case 'l':
            argidx++;
            cmd.size = strtoul(argv[argidx], NULL, 0);
            argidx++;
            break;
        case 'f':
            argidx++;
            cmd.file = _strdup(argv[argidx]);
            argidx++;
            break;
        case 'a':
            argidx++;
            cmd.alignment = strtoul(argv[argidx], NULL, 0);
            argidx++;
            break;
        case 'b':
            cmd.binary = TRUE;
            argidx++;
            break;
        case '?':
        case 'h':
        default:
            return 1;
        }
    }

    if (argidx != argc) {
        if (cmd.op == op_sel::write &&
            cmd.node != devnode_sel::queue_st) {
            cmd.size = argc - argidx;
            cmd.data = allocate_buffer(cmd.size, cmd.alignment);
            if (cmd.data == nullptr) {
                cout << "Could not allocate memory for data" << endl;
                exit(1);
            }

            for (int byteidx = 0; argidx < argc; argidx++, byteidx++) {
                cmd.data[byteidx] = (char)strtoul(argv[argidx], NULL, 0);
            }
        }
        else
            return 0;
    }

    return 0;
}

int __cdecl main(const int argc, char* argv[])
{
    try {
        if (parse_command_line(argc, argv)) {
            help();
            if (cmd.data) _aligned_free(cmd.data);
            exit(1);
        }

        auto dev_details = get_device_details(GUID_DEVINTERFACE_QDMA);
        cout << "Found " << dev_details.size() << " QDMA devices\n";

        device_details dev_info;
        auto res = get_device(dev_details, cmd.dev_name, dev_info);
        if (res == false) {
            printf("Device name is not valid\n");
            if (cmd.data) _aligned_free(cmd.data);
            return 1;
        }

        cmd.bus_no = dev_info.bus_no;
        cmd.dev_no = dev_info.dev_no;
        cmd.fun_no = dev_info.fun_no;
        qdma_device qdev(dev_info.device_path.c_str());

        if (cmd.qid >= qdev.get_qmax()) {
            if (cmd.data) _aligned_free(cmd.data);
            cout << "Invalid qid provided." << endl;
            return 1;
        }

        if (cmd.op == op_sel::read) {
            qdev.qdma_read();
        }
        else if (cmd.op == op_sel::write) {
            qdev.qdma_write();
        }
        else if (cmd.op == op_sel::interrupt) {
            qdev.qdma_interrupt();
        }
    }
    catch (const std::exception& e) {
        if (cmd.data) _aligned_free(cmd.data);
        cout << "Error: " << e.what() << '\n';
    }
}
