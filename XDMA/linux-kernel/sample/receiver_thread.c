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

void enqueue(QueueElement element) {
    pthread_mutex_lock(&queue->mutex);

    if (isQueueFull(queue)) {
        debug_printf("Queue is full. Cannot enqueue.\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    queue->rear = (queue->rear + 1) % NUMBER_OF_QUEUE;
    queue->elements[queue->rear] = element;
    queue->count++;

    pthread_mutex_unlock(&queue->mutex);
}

QueueElement dequeue() {
    pthread_mutex_lock(&queue->mutex);

    if (isQueueEmpty(queue)) {
        debug_printf("Queue is empty. Cannot dequeue.\n");
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


void receiver_in_normal_mode(char* devname, int fd, uint64_t size) {

    BUF_POINTER buffer;
    uint64_t bytes_rcv;

    while (rx_thread_run) {
        buffer = pop();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not pop.\n");
            continue;
        }

        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) {
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }
        rx_stats.rxPackets++;
        rx_stats.rxBytes += bytes_rcv;

        enqueue((QueueElement)buffer);
    }
}

void receiver_in_loopback_mode(char* devname, int fd, char *fn, uint64_t size) {

    BUF_POINTER buffer;
    BUF_POINTER data;
    uint64_t bytes_rcv;
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
        buffer = pop();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not pop.\n");
            continue;
        }

        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) {
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }

        if(size != bytes_rcv) {
            debug_printf("FAILURE: size(%ld) and bytes_rcv(%ld) are different.\n", 
                         size, bytes_rcv);
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }

        if(memcmp((const void *)data, (const void *)buffer, size)) {
            debug_printf("FAILURE: data(%p) and buffer(%p) are different.\n", 
                         data, buffer);
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }

        rx_stats.rxPackets++;
        rx_stats.rxBytes += bytes_rcv;

        push(buffer);
    }
}

void receiver_in_performance_mode(char* devname, int fd, char *fn, uint64_t size) {

    BUF_POINTER buffer;
    uint64_t bytes_rcv;

    while (rx_thread_run) {
        buffer = pop();
        if(buffer == NULL) {
            debug_printf("FAILURE: Could not pop.\n");
            continue;
        }

        if(xdma_api_read_to_buffer_with_fd(devname, fd, buffer, 
                                           size, &bytes_rcv)) {
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }

        if(size != bytes_rcv) {
            debug_printf("FAILURE: size(%ld) and bytes_rcv(%ld) are different.\n", 
                         size, bytes_rcv);
            if(push(buffer)) {
                debug_printf("FAILURE: Could not push.\n");
            }
            continue;
        }

        rx_stats.rxPackets++;
        rx_stats.rxBytes += bytes_rcv;

        push(buffer);
    }
}

void* receiver_thread(void* arg) {

    rx_thread_arg_t* p_arg = (rx_thread_arg_t*)arg;
    int fd;

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

