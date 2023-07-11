/*
* TSNv1 XDMA :
* -------------------------------------------------------------------------------
# Copyrights (c) 2023 TSN Lab. All rights reserved.
# Programmed by hounjoung@tsnlab.com
#
# Revision history
# 2023-xx-xx    hounjoung   create this file.
# $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "receiver_thread.h"

#include "../libxdma/api_xdma.h"

stats_t tx_stats;

extern int tx_thread_run;

void sender_in_normal_mode(char* devname, int fd, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;

    while (tx_thread_run) {
        buffer = NULL;
        buffer = dequeue();

        if(buffer == NULL) {
            continue;
        }

        if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                       size, &bytes_tr)) {
            continue;
        }

        tx_stats.txPackets++;
        tx_stats.txBytes += bytes_tr;

        push((BUF_POINTER)buffer);
    }
}

void sender_in_loopback_mode(char* devname, int fd, char *fn, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;
    int infile_fd = -1;
    ssize_t rc;

    infile_fd = open(fn, O_RDONLY);
    if (infile_fd < 0) {
        printf("Unable to open input file %s, %d.\n", fn, infile_fd);
        return;
    }

    while (tx_thread_run) {
        buffer = NULL;
        buffer = dequeue();

        if(buffer == NULL) {
            continue;
        }

        lseek(infile_fd, 0, SEEK_SET);
        rc = read_to_buffer(fn, infile_fd, buffer, size, 0);
        if (rc < 0 || rc < size) {
            printf("%s - Error, read_to_buffer: size - %ld, rc - %ld.\n", __func__, size, rc);
            close(infile_fd);
            return;
        }

        if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                       size, &bytes_tr)) {
            printf("%s - Error, xdma_api_write_from_buffer_with_fd.\n", __func__);
            continue;
        }

        tx_stats.txPackets++;
        tx_stats.txBytes += bytes_tr;

        push((BUF_POINTER)buffer);
    }
    close(infile_fd);
}

void sender_in_performance_mode(char* devname, int fd, char *fn, uint64_t size) {

    QueueElement buffer = NULL;
    uint64_t bytes_tr;
    int infile_fd = -1;
    ssize_t rc;

    infile_fd = open(fn, O_RDONLY);
    if (infile_fd < 0) {
        printf("Unable to open input file %s, %d.\n", fn, infile_fd);
        return;
    }

    buffer = (QueueElement)xdma_api_get_buffer(size);
    if(buffer == NULL) {
        close(infile_fd);
        return;
    }

    rc = read_to_buffer(fn, infile_fd, buffer, size, 0);
    if (rc < 0 || rc < size) {
        close(infile_fd);
        free(buffer);
        return;
    }

    while (tx_thread_run) {
        if(xdma_api_write_from_buffer_with_fd(devname, fd, buffer,
                                       size, &bytes_tr)) {
            continue;
        }

        tx_stats.txPackets++;
        tx_stats.txBytes += bytes_tr;
    }

    close(infile_fd);
    free(buffer);
}

void* sender_thread(void* arg) {

    tx_thread_arg_t* p_arg = (tx_thread_arg_t*)arg;
    int fd;

    printf(">>> %s(devname: %s, fn: %s, mode: %d, size: %d)\n", 
               __func__, p_arg->devname, p_arg->fn, 
               p_arg->mode, p_arg->size);

    if(xdma_api_dev_open(p_arg->devname, 0 /* eop_flush */, &fd)) {
        printf("FAILURE: Could not open %s. Make sure xdma device driver is loaded and you have access rights (maybe use sudo?).\n", p_arg->devname);
        printf("<<< %s\n", __func__);
        return NULL;
    }

    initialize_statistics(&tx_stats);

    switch(p_arg->mode) {
    case RUN_MODE_NORMAL:
        sender_in_normal_mode(p_arg->devname, fd, p_arg->size);
    break;
    case RUN_MODE_LOOPBACK:
        //sender_in_loopback_mode(p_arg->devname, fd, p_arg->fn, p_arg->size);
        sender_in_performance_mode(p_arg->devname, fd, p_arg->fn, p_arg->size);
    break;
    case RUN_MODE_PERFORMANCE:
        sender_in_performance_mode(p_arg->devname, fd, p_arg->fn, p_arg->size);
    break;
    default:
        printf("%s - Unknown mode(%d)\n", __func__, p_arg->mode);
    break;
    }

    close(fd);
    printf("<<< %s()\n", __func__);

    return NULL;
}

