#ifndef __API_XDMA_H__
#define __API_XDMA_H__

#include <stdint.h>

#include "../xdma/cdev_sgdma.h"


void debug_printf(const char *fmt, ...);

int xdma_api_dev_open(char *devname, int eop_flush, int *fd);
int xdma_api_dev_close(int fd);

int xdma_api_rd_register(char *devname, unsigned long int address, 
                         char access_width, uint32_t *read_val);
int xdma_api_wr_register(char *devname, unsigned long int address, 
                         char access_width, uint32_t writeval);
int xdma_api_ioctl_perf_start(char *devname, int size);
int xdma_api_ioctl_perf_get(char *devname, 
                            struct xdma_performance_ioctl *perf);
int xdma_api_ioctl_perf_stop(char *devname, 
                             struct xdma_performance_ioctl *perf);

/* dma from device */
int xdma_api_read_to_buffer(char *devname, char *buffer, 
                            uint64_t size, uint64_t *bytes_rcv);
int xdma_api_read_to_buffer_with_fd(char *devname, int fd, char *buffer,
                                    uint64_t size, int *bytes_rcv);
/* dma to device */
int xdma_api_write_from_buffer(char *devname, char *buffer, 
                               uint64_t size, uint64_t *bytes_tr);

int xdma_api_write_from_buffer_with_fd(char *devname, int fd, 
                      char *buffer, uint64_t size, uint64_t *bytes_tr);
char * xdma_api_get_buffer(uint64_t size);

ssize_t read_to_buffer(char *fname, int fd, char *buffer, 
                       uint64_t size, uint64_t base);

#endif // __API_XDMA_H__
