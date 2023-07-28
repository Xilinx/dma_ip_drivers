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
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "xdma_common.h"
#include "platform_config.h"
#include "buffer_handler.h"

#include "../libxdma/api_xdma.h"

static CircularQueue_t g_queue;
static CircularQueue_t* queue = NULL;

stats_t rx_stats;

extern int rx_thread_run;

void initialize_queue(CircularQueue_t* p_queue) {
    queue = p_queue;

    p_queue->front = 0;
    p_queue->rear = -1;
    p_queue->count = 0;
    pthread_mutex_init(&p_queue->mutex, NULL);
}

int isQueueEmpty() {
    return (queue->count == 0);
}

int isQueueFull() {
    return (queue->count == NUMBER_OF_QUEUE);
}

int getQueueCount() {
    return queue->count;
}

void xbuffer_enqueue(QueueElement element) {
    pthread_mutex_lock(&queue->mutex);

    if (isQueueFull(queue)) {
        debug_printf("Queue is full. Cannot xbuffer_enqueue.\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    queue->rear = (queue->rear + 1) % NUMBER_OF_QUEUE;
    queue->elements[queue->rear] = element;
    queue->count++;

    pthread_mutex_unlock(&queue->mutex);
}

QueueElement xbuffer_dequeue() {
    pthread_mutex_lock(&queue->mutex);

    if (isQueueEmpty(queue)) {
        debug_printf("Queue is empty. Cannot xbuffer_dequeue.\n");
        pthread_mutex_unlock(&queue->mutex);
        return EMPTY_ELEMENT;
    }

    QueueElement dequeuedElement = queue->elements[queue->front];
    queue->front = (queue->front + 1) % NUMBER_OF_QUEUE;
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return dequeuedElement;
}

void initialize_statistics(stats_t* p_stats) {

#if 1
    memset(p_stats, 0, sizeof(stats_t));
#else
    p_stats->readPackets = 0;
    p_stats->writePackets = 0;
    p_stats->rxPackets = 0;
    p_stats->rxBytes = 0;
    p_stats->rxErrors = 0;
    p_stats->rxDrops = 0;
    p_stats->rxPps = 0;
    p_stats->rxBps = 0;
    p_stats->txPackets = 0;
    p_stats->txFiltered = 0;
    p_stats->txBytes = 0;
    p_stats->txPps = 0;
    p_stats->txBps = 0;
#endif
}

#define BUFFER_SIZE 16

void print_hex_ascii(int addr, const unsigned char *buffer, size_t length) {

    size_t i, j;

    printf("%7d: ", addr);
    for (i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
        if (i == 7)
            printf(" ");
    }
    if (length < BUFFER_SIZE) {
        for (j = 0; j < (BUFFER_SIZE - length); j++) {
            printf("   ");
            if (j == 7)
                printf(" ");
        }
    }
    printf(" ");
    for (i = 0; i < length; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            printf("%c", buffer[i]);
        } else {
            printf(".");
        }
        if (i == 7)
            printf(" ");
    }
    printf("\n");
}

void packet_dump(BUF_POINTER buffer, int length) {

    int total = 0, len;
    int address = 0;

    while(total < length) {
        if(length >= BUFFER_SIZE) {
            len = BUFFER_SIZE;
        } else {
            len = length;
        }
        print_hex_ascii(address, (const unsigned char *)&buffer[total], len);
        total += len;
        address += BUFFER_SIZE;
    }
}

void receiver_in_normal_mode(char* devname, int fd, uint64_t size) {

    BUF_POINTER buffer;
    int bytes_rcv;

    set_register(REG_TSN_CONTROL, 1);
    while (rx_thread_run) {
        buffer = buffer_pool_alloc();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not buffer_pool_alloc.\n");
            continue;
        }

        bytes_rcv = 0;
        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) 
        {
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }
        if(bytes_rcv > MAX_BUFFER_LENGTH) {
            continue;
        }

        rx_stats.rxPackets++;
        rx_stats.rxBytes = rx_stats.rxBytes + bytes_rcv;

        // printf("  rx_stats.rxPackets: %16lld \n    rx_stats.rxBytes: %16lld\n", rx_stats.rxPackets, rx_stats.rxBytes);
        // packet_dump(buffer, bytes_rcv);

        xbuffer_enqueue((QueueElement)buffer);
    }
    set_register(REG_TSN_CONTROL, 0);
}

void receiver_in_loopback_mode(char* devname, int fd, char *fn, uint64_t size) {

    BUF_POINTER buffer;
    BUF_POINTER data;
    int bytes_rcv;
    int infile_fd = -1;
    ssize_t rc;

    infile_fd = open(fn, O_RDONLY);
    if (infile_fd < 0) {
        printf("Unable to open input file %s, %d.\n", fn, infile_fd);
        return;
    }

    data = (QueueElement)xdma_api_get_buffer(size);
    if(data == NULL) {
        close(infile_fd);
        return;
    }

    rc = read_to_buffer(fn, infile_fd, data, size, 0);
    if (rc < 0 || rc < size) {
        close(infile_fd);
        free(data);
        return;
    }
    close(infile_fd);

    while (rx_thread_run) {
        buffer = buffer_pool_alloc();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not buffer_pool_alloc.\n");
            continue;
        }

        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) {
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }

        if(size != bytes_rcv) {
            debug_printf("FAILURE: size(%ld) and bytes_rcv(%ld) are different.\n", 
                         size, bytes_rcv);
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }

        if(memcmp((const void *)data, (const void *)buffer, size)) {
            debug_printf("FAILURE: data(%p) and buffer(%p) are different.\n", 
                         data, buffer);
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }

        rx_stats.rxPackets++;
        rx_stats.rxBytes += bytes_rcv;

//        printf("  rx_stats.rxPackets: %16lld \n    rx_stats.rxBytes: %16lld\n", rx_stats.rxPackets, rx_stats.rxBytes);

        buffer_pool_free(buffer);
    }
}

void receiver_in_performance_mode(char* devname, int fd, char *fn, uint64_t size) {

    BUF_POINTER buffer;
    int bytes_rcv;

    set_register(REG_TSN_CONTROL, 1);
    while (rx_thread_run) {
        buffer = buffer_pool_alloc();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not buffer_pool_alloc.\n");
            continue;
        }

        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) {
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }

        if(size != bytes_rcv) {
            debug_printf("FAILURE: size(%ld) and bytes_rcv(%ld) are different.\n", 
                         size, bytes_rcv);
            if(buffer_pool_free(buffer)) {
                debug_printf("FAILURE: Could not buffer_pool_free.\n");
            }
            continue;
        }

        rx_stats.rxPackets++;
        rx_stats.rxBytes += bytes_rcv;

        buffer_pool_free(buffer);
    }
    set_register(REG_TSN_CONTROL, 0);
}

void* receiver_thread(void* arg) {

    rx_thread_arg_t* p_arg = (rx_thread_arg_t*)arg;
    int fd = 0;

    printf(">>> %s(devname: %s, fn: %s, mode: %d, size: %d)\n", 
                __func__, p_arg->devname, p_arg->fn,
                p_arg->mode, p_arg->size);

    if(xdma_api_dev_open(p_arg->devname, 1 /* eop_flush */, &fd)) {
        printf("FAILURE: Could not open %s. Make sure xdma device driver is loaded and you have access rights (maybe use sudo?).\n", p_arg->devname);
        printf("<<< %s\n", __func__);
        return NULL;
    }

    initialize_queue(&g_queue);
    initialize_statistics(&rx_stats);

    switch(p_arg->mode) {
    case RUN_MODE_TSN:
    case RUN_MODE_NORMAL:
        receiver_in_normal_mode(p_arg->devname, fd, p_arg->size);
    break;
    case RUN_MODE_LOOPBACK:
        receiver_in_loopback_mode(p_arg->devname, fd, p_arg->fn, p_arg->size);
    break;
    case RUN_MODE_PERFORMANCE:
        receiver_in_performance_mode(p_arg->devname, fd, p_arg->fn, p_arg->size);
    break;
    default:
        printf("%s - Unknown mode(%d)\n", __func__, p_arg->mode);
    break;
    }

    pthread_mutex_destroy(&g_queue.mutex);

    xdma_api_dev_close(fd);
    printf("<<< %s\n", __func__);
    return NULL;
}

