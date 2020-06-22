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

#pragma once

#include <string>
#include <system_error>
#include <vector>
#include <sstream>

#include <Windows.h>
#include <winioctl.h>
#include <SetupAPI.h>
#include <INITGUID.H>

#define DEV_NAME_MAX_SZ                 12

struct device_details {
    UINT8 bus_no;
    UINT8 dev_no;
    UINT8 fun_no;
    std::string device_name;
    std::string device_path;
};

#pragma warning( disable : 4505)
static bool get_device(std::vector<device_details> dev_details, const char *dev_name, device_details &dev_info) {
    bool is_dev_found = false;

    for (auto i = 0; i < (int)dev_details.size(); i++) {
        if (dev_details[i].device_name == dev_name) {
            is_dev_found = true;
            dev_info = dev_details[i];
            break;
        }
    }
    return is_dev_found;
}

static std::vector<device_details> get_device_details(GUID guid) {
    auto device_info = SetupDiGetClassDevs((LPGUID)&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("GetDevices INVALID_HANDLE_VALUE");
    }

    SP_DEVINFO_DATA dev_info_data;
    dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    SP_DEVICE_INTERFACE_DATA device_interface = { 0 };
    device_interface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);


    std::vector<device_details> dev_details;
    device_details info;

    /* enumerate through devices */
    for (unsigned index = 0;
         SetupDiEnumDeviceInterfaces(device_info, NULL, &guid, index, &device_interface);
         ++index) {

        unsigned char *bus_no = NULL;
        unsigned char *dev_addr = NULL;
        DWORD lsize = 0;

        /* get required buffer size */
        unsigned long detailLength = 0;
        if (!SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, NULL, 0, &detailLength, NULL) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            throw std::runtime_error("SetupDiGetDeviceInterfaceDetail - get length failed");
        }

        /* allocate space for device interface detail */
        auto dev_detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(new char[detailLength]);
        dev_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        /* get device interface detail */
        if (!SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, dev_detail, detailLength, NULL, &dev_info_data)) {
            delete[] dev_detail;
            throw std::runtime_error("SetupDiGetDeviceInterfaceDetail - get detail failed");
        }

        /* get the required size for PCIe BUS NUMBER Parameter */
        if (!SetupDiGetDeviceRegistryProperty(device_info, &dev_info_data, SPDRP_BUSNUMBER, NULL, (PBYTE)bus_no, 0, &lsize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            delete[] dev_detail;
            throw std::runtime_error("SetupDiGetDeviceRegistryProperty - get length failed");
        }

        /* Retrieve the PCIe bus number */
        bus_no = (unsigned char*)malloc(lsize);
        if (!SetupDiGetDeviceRegistryProperty(device_info, &dev_info_data, SPDRP_BUSNUMBER, NULL, (PBYTE)bus_no, lsize, NULL)) {
            delete[] dev_detail;
            free(bus_no);
            throw std::runtime_error("SetupDiGetDeviceRegistryProperty - get Registry failed");
        }

        /* get the required size for PCIe Device Address Parameter */
        if (!SetupDiGetDeviceRegistryProperty(device_info, &dev_info_data, SPDRP_ADDRESS, NULL, (PBYTE)dev_addr, 0, &lsize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            delete[] dev_detail;
            free(bus_no);
            throw std::runtime_error("SetupDiGetDeviceRegistryProperty - get length failed");
        }

        /* Retrieve the PCIe Device Address */
        dev_addr = (unsigned char*)malloc(lsize);
        if (!SetupDiGetDeviceRegistryProperty(device_info, &dev_info_data, SPDRP_ADDRESS, NULL, (PBYTE)dev_addr, lsize, NULL)) {
            delete[] dev_detail;
            free(bus_no);
            free(dev_addr);
            throw std::runtime_error("SetupDiGetDeviceRegistryProperty - get Registry failed");
        }

        /* Prepare BDF format (0xBBDDF) */
        UINT8 dev_no = (*dev_addr & 0xF8) >> 3;
        UINT8 fun_no = (*dev_addr & 0x7);
        UINT32 bdf = 0x0;
        bdf = bdf | (*bus_no << 12);
        bdf = bdf | (dev_no << 4);
        bdf = bdf | (fun_no);

        /* Prepare the PCIe devices identifier strings */
        std::ostringstream outs;
        outs << "qdma";
        outs.setf (std::ios::hex, std::ios::basefield);
        outs.width(5); outs.fill('0');
        outs << bdf;

        /* Fill the details in info structure */
        info.bus_no = *bus_no;
        info.dev_no = dev_no;
        info.fun_no = fun_no;
        info.device_name = outs.str();
        info.device_path = dev_detail->DevicePath;

        /* Append it to the return vector */
        dev_details.emplace_back(info);

        free(bus_no);
        free(dev_addr);
        delete[] dev_detail;
    }

    SetupDiDestroyDeviceInfoList(device_info);

    return dev_details;
}


inline static std::string get_last_win_err_msg() {
    char tmp[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), tmp, 256, NULL);

    return{ tmp, 256 };
}

struct device_file {
    HANDLE h;
    device_file();
    device_file(const std::string& path, DWORD accessFlags);
    ~device_file();

    device_file(const device_file& rhs) = delete;
    device_file& operator=(const device_file& rhs) = delete;

    device_file(device_file && rhs);

    device_file& operator=(device_file&& rhs);

    void open(const std::string& path, DWORD accessFlags, DWORD fattribs = FILE_ATTRIBUTE_NORMAL);

    void close();

    template <typename T>
    void write(long address, const T value);

    template <typename T>
    T read(long address);

    void seek(long device_offset);
    size_t write(void* buffer, size_t size);
    size_t read(void* buffer, size_t size);

    size_t ioctl(int code, void* inData = nullptr, DWORD inSize = 0, void* outData = nullptr,
                 DWORD outSize = 0);
};

inline device_file::device_file() : h(INVALID_HANDLE_VALUE) {};

inline void device_file::open(const std::string& path, DWORD accessFlags, DWORD fattribs) {
    h = CreateFile(path.c_str(), accessFlags, 0, NULL, OPEN_EXISTING,
                   fattribs, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateFile control failed: " + get_last_win_err_msg());
    }
}

inline void device_file::close() {
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
}

inline device_file::device_file(const std::string& path, DWORD accessFlags) : h(INVALID_HANDLE_VALUE) {
    open(path, accessFlags);
}

inline device_file::~device_file() {
    close();
}

#if 0
inline device_file::device_file(device_file && rhs)
    : h(INVALID_HANDLE_VALUE) {
    *this = std::move(rhs);
}

inline device_file& device_file::operator=(device_file&& rhs) {
    if (this != &rhs) {
        h = rhs.h;
        rhs.h = INVALID_HANDLE_VALUE;
    }
    return *this;
};
#endif

inline void device_file::seek(long device_offset) {
    if (INVALID_SET_FILE_POINTER == SetFilePointer(h, device_offset, NULL, FILE_BEGIN)) {
        throw std::runtime_error("SetFilePointer failed: " + get_last_win_err_msg());
    }
}

inline size_t device_file::write(void* buffer, size_t size) {
    unsigned long num_bytes_written;
    if (!WriteFile(h, buffer, (DWORD)size, &num_bytes_written, NULL)) {
        throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
    }

    return num_bytes_written;
}

template <typename T>
inline void device_file::write(long addr, T t) {
    seek(addr);
    unsigned long num_bytes_written;
    if (!WriteFile(h, &t, sizeof(T), &num_bytes_written, NULL)) {
        throw std::runtime_error("Failed to write to device! " + get_last_win_err_msg());
    } else if (num_bytes_written != sizeof(T)) {
        throw std::runtime_error("Failed to write all bytes!");
    }
}

inline size_t device_file::read(void* buffer, size_t size) {
    unsigned long num_bytes_read;
    if (!ReadFile(h, buffer, (DWORD)size, &num_bytes_read, NULL)) {
        throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
    }
    return num_bytes_read;
}

template <typename T>
inline T device_file::read(long addr) {
    seek(addr);
    T buffer;
    unsigned long num_bytes_read;
    if (!ReadFile(h, &buffer, sizeof(T), &num_bytes_read, NULL)) {
        throw std::runtime_error("Failed to read from device! " + get_last_win_err_msg());
    } else if (num_bytes_read != sizeof(T)) {
        throw std::runtime_error("Failed to read all bytes!");
    }
    return buffer;
}

inline size_t device_file::ioctl(int code, void* inData, DWORD inSize, void* outData, DWORD outSize) {

    DWORD nb = 0;
    BOOL success = DeviceIoControl(h, code, inData, inSize, outData, outSize, &nb, NULL);
    if (!success) {
        throw std::runtime_error("ioctl failed!" + get_last_win_err_msg());
    }
    return nb;
}
