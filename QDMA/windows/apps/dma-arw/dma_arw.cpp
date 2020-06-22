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

#include "dma_arw.hpp"
#include "version.h"

cli_cmd cmd;

void CALLBACK CompletionRoutine(
    DWORD errorcode,
    DWORD bytes_transfered,
    LPOVERLAPPED overlapped
)
{
    qdma_device *qdev = (qdma_device *)overlapped->hEvent;

    if (errorcode) {
        qdev->xfer_done = false;
        ReleaseSemaphore(qdev->sem_xfer_done, 1, NULL);
        std::cerr << "queue transfer failed with error : " << get_last_win_err_msg() << std::endl;
        return;
    }

    qdev->rxd_size = bytes_transfered;
    if (qdev->xfer_size >= bytes_transfered) {
        qdev->xfer_done = true;
        ReleaseSemaphore(qdev->sem_xfer_done, 1, NULL);
    }

    return;
}

BOOL WINAPI xfer_completion(PVOID arg)
{
    UNREFERENCED_PARAMETER(arg);
    bool ret;
    constexpr unsigned int wait_size = 50;
    LPOVERLAPPED_ENTRY wait_ov_entry = (LPOVERLAPPED_ENTRY)malloc(wait_size * sizeof(OVERLAPPED));
    ULONG no_entries_cmpltd = 0;
    ULONG wait_time = COMPL_WAIT_TIME; /* In msec */
    ULONG total_bytes_xfered = 0;
    qdma_device *qdev = (qdma_device *)arg;


    try {
        qdev->thread_run = true;
        while (true) {
            ret = GetQueuedCompletionStatusEx(
                qdev->cmplt_io_handle, &wait_ov_entry[0],
                wait_size, &no_entries_cmpltd,
                wait_time, FALSE);

            if (ret == true) {
                qdev->rxd_size = 0;
                for (ULONG index = 0; index < no_entries_cmpltd; index++) {
                    total_bytes_xfered += wait_ov_entry[index].dwNumberOfBytesTransferred;
                    qdev->rxd_size += wait_ov_entry[index].dwNumberOfBytesTransferred;
                }
                if (total_bytes_xfered >= qdev->xfer_size) {
                    qdev->xfer_done = true;
                    ReleaseSemaphore(qdev->sem_xfer_done, 1, NULL);
                }
            }
            if (qdev->thread_exit == true)
                break;
        }
        free(wait_ov_entry);
    }
    catch (const std::exception& e) {
        free(wait_ov_entry);
        std::cout << "Error: " << e.what() << '\n';
    }

    return true;
}

void qdma_device::async_env_init(void)
{
    sem_xfer_done = CreateSemaphore(NULL, 0, 1, NULL);
    if (sem_xfer_done == NULL) {
        throw std::runtime_error("CreateSemaphore error: " + get_last_win_err_msg());
    }

    if (cmd.mode == FILE_NORMAL_MODE) {
        cmplt_io_handle = CreateIoCompletionPort(queue_.h, NULL, 1, 0);
        if (NULL == cmplt_io_handle) {
            throw std::runtime_error("FAILED TO CREATE COMPLETION HANDLE : " + get_last_win_err_msg());
        }

        thread_run = false;
        thread_exit = false;
        xfer_done = false;

        cmplt_th_handle =
            CreateThread(NULL,
                10 * 1024 * 1024, // stack size
                (LPTHREAD_START_ROUTINE)xfer_completion,
                (void *)this,
                0,
                NULL);

        if (NULL == cmplt_th_handle) {
            CloseHandle(sem_xfer_done);
            throw std::runtime_error("Error CreateThread Failed : " + get_last_win_err_msg());
        }

        while (thread_run == false);
    }
}

void qdma_device::async_env_exit(void)
{
    if (cmd.mode == FILE_NORMAL_MODE) {
        thread_exit = true;
        Sleep(2 * COMPL_WAIT_TIME);
        WaitForSingleObject(cmplt_th_handle, INFINITE);
        CloseHandle(cmplt_th_handle);
    }
    CloseHandle(sem_xfer_done);
}

void qdma_device::poll_completion(void)
{
    int timeout = 0;

    while (WAIT_OBJECT_0 != WaitForSingleObject(sem_xfer_done, 1)) {
        timeout++;
        SleepEx(9, TRUE);
        if (timeout > MAX_TIMEOUT_PERIOD)
            break;
    }

    if (cmd.mode == FILE_NORMAL_MODE) {
        thread_exit = true;
        Sleep(2 * COMPL_WAIT_TIME);
    }
}

int qdma_device::qdma_read(void)
{
    bool ret;
    OVERLAPPED overlapped;

    cmd.data = allocate_buffer(cmd.size, cmd.alignment);
    if (!cmd.data) {
        throw std::runtime_error("Error allocating memory" + get_last_win_err_msg());
    }

    switch (cmd.node) {
    case devnode_sel::queue_mm:
        qopen(cmd.qid, true);
        xfer_size = cmd.size;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.OffsetHigh = 0x0;
        overlapped.Offset = (DWORD)cmd.addr;

        if (cmd.mode == FILE_NORMAL_MODE) {
            ret = ReadFile(queue_.h, cmd.data, xfer_size, NULL, &overlapped);
        }
        else {
            overlapped.hEvent = this;
            ret = ReadFileEx(queue_.h, cmd.data, xfer_size, &overlapped, CompletionRoutine);
        }

        if (ret == false) {
            auto error = GetLastError();
            if (error != ERROR_IO_PENDING)
                throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
        }

        /* no-op...just waiting for async completion */
        poll_completion();

        if (xfer_done) {
            printf("Asynchronous Read completed : \n");
            printf("------------------------------\n");
            if (TRUE == cmd.binary)
                print_bytes_binary(cmd.data, cmd.size);
            else
                print_bytes(cmd.addr, cmd.data, cmd.size);
        }
        else {
            printf("Failed to Read : Async Timeout Occurred\n");
        }
        qclose();
        break;

    case devnode_sel::queue_st:
        {
            DWORD packet_size;
            DWORD packet_cnt;
            DWORD remain_size = cmd.size;
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

                xfer_size = packet_size * packet_cnt;
                xfer_done = false;

                memset(&overlapped, 0, sizeof(overlapped));
                overlapped.OffsetHigh = 0x0;
                overlapped.Offset = (DWORD)cmd.addr;

                //printf("buf_idx : %d, len : %d\n", buf_idx, xfer_size);

                if (cmd.mode == FILE_NORMAL_MODE) {
                    ret = ReadFile(queue_.h, &cmd.data[buf_idx], xfer_size, NULL, &overlapped);
                }
                else {
                    overlapped.hEvent = this;
                    ret = ReadFileEx(queue_.h, &cmd.data[buf_idx], xfer_size, &overlapped, CompletionRoutine);
                }

                if (ret == false) {
                    auto error = GetLastError();
                    if (error != ERROR_IO_PENDING)
                        throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
                }

                poll_completion();
                if (!xfer_done)
                    break;
                buf_idx = buf_idx + rxd_size;
                remain_size = remain_size - rxd_size;

                if (cmd.size == 0x0) {
                    cout << "ZERO Byte Read Successful\n";
                }
            } while (remain_size);

            if (xfer_done) {
                printf("Asynchronous Read completed : \n");
                printf("------------------------------\n");
                if (TRUE == cmd.binary)
                    print_bytes_binary(cmd.data, cmd.size);
                else
                    print_bytes(cmd.addr, cmd.data, cmd.size);
            }
            else {
                printf("Failed to Read : Async Timeout Occurred\n");
            }
            qclose();
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
    bool ret;
    OVERLAPPED overlapped;

    switch (cmd.node) {
    case devnode_sel::queue_mm:
        qopen(cmd.qid, true);

        if (cmd.data == nullptr && false == cmd.file.empty()) {
            read_file_option();
        }

        xfer_size = cmd.size;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.OffsetHigh = 0x0;
        overlapped.Offset = (DWORD)cmd.addr;

        if (cmd.mode == FILE_NORMAL_MODE) {
            ret = WriteFile(queue_.h, cmd.data, xfer_size, NULL, &overlapped);
        }
        else {
            overlapped.hEvent = this;
            ret = WriteFileEx(queue_.h, cmd.data, xfer_size, &overlapped, CompletionRoutine);
        }

        if (ret == false) {
            auto error = GetLastError();
            if (error != ERROR_IO_PENDING)
                throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
        }

        poll_completion();

        if (!xfer_done) {
            printf("Failed to Write : Async Timeout Occurred\n");
        }
        else {
            printf("Asynchronous Write completed : \n");
            printf("------------------------------\n");
        }

        qclose();

        break;

    case devnode_sel::queue_st:
    {
        qopen(cmd.qid, false);
        aligned_vector<uint16_t> wr_buffer(cmd.size);
        iota(wr_buffer.begin(), wr_buffer.end(), static_cast<uint16_t>(0)); /* 0, 1, 2, 3, 4 ... */
        dgen.reset_h2c();

        if (cmd.size == 0x0)
            cout <<"Initiating Zero Byte Write\n";

        xfer_size = cmd.size;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.OffsetHigh = 0x0;
        overlapped.Offset = (DWORD)cmd.addr;

        if (cmd.mode == FILE_NORMAL_MODE) {
            ret = WriteFile(queue_.h, wr_buffer.data(), xfer_size, NULL, &overlapped);
        }
        else {
            overlapped.hEvent = this;
            ret = WriteFileEx(queue_.h, wr_buffer.data(), xfer_size, &overlapped, CompletionRoutine);
        }

        if (ret == false) {
            auto error = GetLastError();
            if (error != ERROR_IO_PENDING)
                throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
        }

        poll_completion();

        if (xfer_done) {
            printf("Asynchronous Write completed : \n");
            printf("------------------------------\n");
            if (true == dgen.check_h2c(cmd.qid + qbase))
                cout << "Data generator returned SUCCESS for received data" << endl;
            else
                throw std::runtime_error("Data generator reported error for received data!");

            if (cmd.size == 0x0)
                cout << "ZERO Byte Write Successful\n";
        }
        else {
            printf("Failed to Write : Async Timeout Occurred\n");
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
    cout << "dma-arw usage:\n";
    cout << "dma-arw -v  : prints the version information\n\n";
    cout << "dma-arw qdma<N> mode <0 | 1> <DEVNODE> <read|write> <ADDR> [OPTIONS] [DATA]\n\n";
    cout << "- qdma<N>   : unique qdma device name (<N> is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No)\n";
    cout << "- mode      : 0 : this mode uses ReadFile and WriteFile async implementation\n";
    cout << "            : 1 : this mode uses ReadFileEx and WriteFileEx async implementation\n";
    cout << "- DEVNODE   : One of: queue_mm_ | queue_st_*\n";
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
     * qdma_rw pf <pf_number> <device> <read|write> <address> [OPTIONS] [DATA]
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

    if (strncmp(argv[argidx], "mode", 4)) {
        return 1;
    }
    ++argidx;

    cmd.mode = std::stoul(argv[argidx]);
    ++argidx;

    if (std::regex_match(argv[argidx], std::regex("queue_mm_[0-9]+"))) {
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

    if (cmd.node == devnode_sel::queue_mm) {
        cmd.addr = strtoul(argv[argidx], NULL, 0);
        ++argidx;
    }
    else if (cmd.node == devnode_sel::queue_st) {
        if (argv[argidx][0] != '-') {
            printf("\nErr: ST mode doesn't need the target address option\n\n");
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
        if (!dev_details.size()) {
            std::cerr << "No QDMA Devices found!" << std::endl;
            return 1;
        }

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
    }
    catch (const std::exception& e) {
        if (cmd.data) _aligned_free(cmd.data);
        cout << "Error: " << e.what() << '\n';
    }
}
