#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <stdarg.h>

#include "../xdma/cdev_sgdma.h"

#include "dma_utils.c"

/* ltoh: little endian to host */
/* htol: host to little endian */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x)       (x)
#define ltohs(x)       (x)
#define htoll(x)       (x)
#define htols(x)       (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x)     __bswap_32(x)
#define ltohs(x)     __bswap_16(x)
#define htoll(x)     __bswap_32(x)
#define htols(x)     __bswap_16(x)
#endif

//#define __DEBUG__
void debug_printf(const char *fmt, ...) {

#ifdef __DEBUG__
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

int xdma_api_dev_open(char *devname, int eop_flush, int *fd) {

    int fpga_fd;

    /*
     * use O_TRUNC to indicate to the driver to flush the data up based on
     * EOP (end-of-packet), streaming mode only
     */
    if (eop_flush) {
        fpga_fd = open(devname, O_RDWR | O_TRUNC);
    } else {
        fpga_fd = open(devname, O_RDWR);
    }

    if (fpga_fd < 0) {
        perror("Device open Failed");
        *fd = fpga_fd;
        return fpga_fd;
    }

    debug_printf("%s Device open successfull\n", devname);
    *fd = fpga_fd;

    return 0;
}

int xdma_api_dev_close(int fd) {

    if ( close(fd) ) {
        perror("Device Close Failed");
        return -1;
    }

    debug_printf("Device(fd: %d) close successfull\n", fd);

    return 0;
}

int xdma_api_rd_register(char *devname, unsigned long int address, 
                         char access_width, uint32_t *read_val) {

    int fd;
    int err = 0;
    void *map;
    off_t target = (off_t)address;
    off_t pgsz, target_aligned, offset;
    uint32_t read_result;

    /* check for target page alignment */
    pgsz = sysconf(_SC_PAGESIZE);
    offset = target & (pgsz - 1);
    target_aligned = target & (~(pgsz - 1));

    if ((fd = open(devname, O_RDWR | O_SYNC)) == -1) {
        debug_printf("character device %s opened failed: %s.\n",
            devname, strerror(errno));
        return -errno;
    }

    debug_printf("character device %s opened.\n", devname);

    map = mmap(NULL, offset + 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target_aligned);
    if (map == (void *)-1) {
        debug_printf("Memory 0x%lx mapped failed: %s.\n", target, strerror(errno));
        err = 1;
        goto close;
    }
    debug_printf("Memory 0x%lx mapped at address %p.\n", target_aligned, map);

    map += offset;
    switch (access_width) {
    case 'b':
        read_result = *((uint8_t *) map);
        debug_printf("Read 8-bits value at address 0x%lx (%p): 0x%02x\n",
                 target, map, (unsigned int)read_result);
        break;
    case 'h':
        read_result = *((uint16_t *) map);
        /* swap 16-bit endianess if host is not little-endian */
        read_result = ltohs(read_result);
        debug_printf("Read 16-bit value at address 0x%lx (%p): 0x%04x\n",
                 target, map, (unsigned int)read_result);
        break;
    case 'w':
        read_result = *((uint32_t *) map);
        /* swap 32-bit endianess if host is not little-endian */
        read_result = ltohl(read_result);
        debug_printf("Read 32-bit value at address 0x%lx (%p): 0x%08x\n",
                 target, map, (unsigned int)read_result);
        break;
    default:
        fprintf(stderr, "Illegal data type '%c'.\n", access_width);
        err = 1;
        goto unmap;
    }
    *read_val = read_result;

unmap:
    map -= offset;
    if (munmap(map, offset + 4) == -1) {
        debug_printf("Memory 0x%lx mapped failed: %s.\n", target, strerror(errno));
    }
close:
    close(fd);

    return err;
}

int xdma_api_wr_register(char *devname, unsigned long int address, 
                         char access_width, uint32_t writeval) {

    int fd;
    int err = 0;
    void *map;
    off_t target = (off_t)address;
    off_t pgsz, target_aligned, offset;

    /* check for target page alignment */
    pgsz = sysconf(_SC_PAGESIZE);
    offset = target & (pgsz - 1);
    target_aligned = target & (~(pgsz - 1));

    if ((fd = open(devname, O_RDWR | O_SYNC)) == -1) {
        debug_printf("character device %s opened failed: %s.\n",
            devname, strerror(errno));
        return -errno;
    }

    debug_printf("character device %s opened.\n", devname);

    map = mmap(NULL, offset + 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target_aligned);
    if (map == (void *)-1) {
        debug_printf("Memory 0x%lx mapped failed: %s.\n", target, strerror(errno));
        err = 1;
        goto close;
    }
    debug_printf("Memory 0x%lx mapped at address %p.\n", target_aligned, map);

    map += offset;
    switch (access_width) {
    case 'b':
        debug_printf("Write 8-bits value 0x%02x to 0x%lx (0x%p)\n",
               (unsigned int)writeval, target, map);
        *((uint8_t *) map) = writeval;
        break;
    case 'h':
        debug_printf("Write 16-bits value 0x%04x to 0x%lx (0x%p)\n",
               (unsigned int)writeval, target, map);
        /* swap 16-bit endianess if host is not little-endian */
        writeval = htols(writeval);
        *((uint16_t *) map) = writeval;
        break;
    case 'w':
        debug_printf("Write 32-bits value 0x%08x to 0x%lx (0x%p)\n",
               (unsigned int)writeval, target, map);
        /* swap 32-bit endianess if host is not little-endian */
        writeval = htoll(writeval);
        *((uint32_t *) map) = writeval;
        break;
    default:
        fprintf(stderr, "Illegal data type '%c'.\n",
            access_width);
        err = 1;
        goto unmap;
    }

unmap:
    map -= offset;
    if (munmap(map, offset + 4) == -1) {
        debug_printf("Memory 0x%lx mapped failed: %s.\n", target, strerror(errno));
    }
close:
    close(fd);

    return err;
}

struct xdma_performance_ioctl perf;

int xdma_api_ioctl_perf_start(char *devname, int size) {

    int fd;
    int rc = 0;
  
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        debug_printf("FAILURE: Could not open %s.\n", devname);
        debug_printf("         Make sure xdma device driver is loaded and ");
        debug_printf("and you have access rights (maybe use sudo?).\n");
        return -1;
    }

    perf.version = IOCTL_XDMA_PERF_V1;
    perf.transfer_size = size;
    rc = ioctl(fd, IOCTL_XDMA_PERF_START, &perf);
    if (rc == 0) {
        debug_printf("IOCTL_XDMA_PERF_START succesful.\n");
    } else {
        debug_printf("ioctl(..., IOCTL_XDMA_PERF_START) = %d\n", rc);
    }

    close(fd);

    return rc;
}

int xdma_api_ioctl_perf_get(char *devname, 
                            struct xdma_performance_ioctl *perf) {
    int fd;
    int rc = 0;
  
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        debug_printf("FAILURE: Could not open %s.\n", devname);
        debug_printf("         Make sure xdma device driver is loaded and ");
        debug_printf("and you have access rights (maybe use sudo?).\n");
        return -1;
    }

    rc = ioctl(fd, IOCTL_XDMA_PERF_GET, perf);
    if (rc == 0) {
      debug_printf("IOCTL_XDMA_PERF_GET succesful.\n");
    } else {
      debug_printf("ioctl(..., IOCTL_XDMA_PERF_GET) = %d\n", rc);
      return -1;
    }
    debug_printf("perf.transfer_size = %d\n", perf->transfer_size);
    debug_printf("perf.iterations = %d\n", perf->iterations);
    debug_printf("(data transferred = %lld bytes)\n", 
            (long long)perf->transfer_size * (long long)perf->iterations);
    debug_printf("perf.clock_cycle_count = %lld\n", (long long)perf->clock_cycle_count);
    debug_printf("perf.data_cycle_count = %lld\n", (long long)perf->data_cycle_count);
    if (perf->clock_cycle_count && perf->data_cycle_count) {
        debug_printf("(data duty cycle = %lld%%)\n", 
                (long long)perf->data_cycle_count * 100 / (long long)perf->clock_cycle_count);
    }

    close(fd);

    return 0;
}

int xdma_api_ioctl_perf_stop(char *devname, struct xdma_performance_ioctl *perf)
{
    int fd;
    int rc = 0;
  
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        debug_printf("FAILURE: Could not open %s.\n", devname);
        debug_printf("         Make sure xdma device driver is loaded and ");
        debug_printf("and you have access rights (maybe use sudo?).\n");
        return -1;
    }

    rc = ioctl(fd, IOCTL_XDMA_PERF_STOP, perf);
    if (rc == 0) {
      debug_printf("IOCTL_XDMA_PERF_STOP succesful.\n");
    } else {
      debug_printf("ioctl(..., IOCTL_XDMA_PERF_STOP) = %d\n", rc);
      return -1;
    }
    debug_printf("perf.transfer_size = %d bytes\n", perf->transfer_size);
    debug_printf("perf.iterations = %d\n", perf->iterations);
    debug_printf("(data transferred = %lld bytes)\n", 
                 (long long)perf->transfer_size * (long long)perf->iterations);
    debug_printf("perf.clock_cycle_count = %lld\n", (long long)perf->clock_cycle_count);
    debug_printf("perf.data_cycle_count = %lld\n", (long long)perf->data_cycle_count);
    if (perf->clock_cycle_count && perf->data_cycle_count) {
        debug_printf("(data duty cycle = %lld%%)\n", 
                 (long long)perf->data_cycle_count * 100 / (long long)perf->clock_cycle_count);
        debug_printf(" data rate ***** bytes length = %d, rate = %f \n", perf->transfer_size, 
                 (double)(long long)perf->data_cycle_count/(long long)perf->clock_cycle_count);
    }
    debug_printf("perf.pending_count = %lld\n", (long long)perf->pending_count);

    close(fd);

    return 0;
}

/* dma from device */
int xdma_api_read_to_buffer(char *devname, char *buffer, 
                            uint64_t size, uint64_t *bytes_rcv) {

    int fd;
    size_t bytes_done = 0;

    /*
     * use O_TRUNC to indicate to the driver to flush the data up based on
     * EOP (end-of-packet), streaming mode only
     */
    fd = open(devname, O_RDWR | O_TRUNC);

    if (fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n", devname, fd);
        perror("open device");
        return -EINVAL;
    }

    bytes_done = read_to_buffer(devname, fd, buffer, size, 0 /*addr*/);
    close(fd);

    if (bytes_done < 0) {
        *bytes_rcv = 0;
        return -1;
    }

    *bytes_rcv = bytes_done;

    return 0;
}

int xdma_api_read_to_buffer_with_fd(char *devname, int fd, char *buffer, 
                                    uint64_t size, uint64_t *bytes_rcv) {

    size_t bytes_done = 0;

    bytes_done = read_to_buffer(devname, fd, buffer, size, 0 /*addr*/);

    if (bytes_done < 0) {
        *bytes_rcv = 0;
        return -1;
    }

    *bytes_rcv = bytes_done;

    return 0;
}

/* dma to device */
int xdma_api_write_from_buffer(char *devname, char *buffer, 
                               uint64_t size, uint64_t *bytes_tr) {

    int fd;
    size_t bytes_done = 0;

    fd = open(devname, O_RDWR);

    if (fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n", devname, fd);
        perror("open device");
        return -EINVAL;
    }

    /* write buffer to AXI MM address using SGDMA */
    bytes_done = write_from_buffer(devname, fd, buffer, size, 0 /*addr */);
    close(fd);
    if (bytes_done < 0) {
        return -1;
    }

    *bytes_tr = bytes_done;
    if (bytes_done < size) {
        debug_printf("underflow %ld/%ld.\n", bytes_done, size);
        return -2;
    }

    return 0;
}

int xdma_api_write_from_buffer_with_fd(char *devname, int fd, char *buffer, 
                                       uint64_t size, uint64_t *bytes_tr) {

    size_t bytes_done = 0;

    /* write buffer to AXI MM address using SGDMA */
    bytes_done = write_from_buffer(devname, fd, buffer, size, 0 /*addr */);
    if (bytes_done < 0) {
        return -1;
    }

    *bytes_tr = bytes_done;
    if (bytes_done < size) {
        debug_printf("underflow %ld/%ld.\n", bytes_done, size);
        return -2;
    }

    return 0;
}

char * xdma_api_get_buffer(uint64_t size) {
    char *buffer = NULL;

    if(posix_memalign((void **)&buffer, 4096 /*alignment */ , size + 4096)) {
        fprintf(stderr, "OOM %lu.\n", size + 4096);
    }

    return buffer;
}

/* dma from device */
int xdma_api_ioctl_aperture_r()
{
    return 0;
}

/* dma to device */
int xdma_api_ioctl_aperture_w()
{
    return 0;
}
