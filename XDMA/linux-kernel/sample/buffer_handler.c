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
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>

#include "xdma_common.h"
#include "libxdma/api_xdma.h"

/* Stack operation for Rx & Tx buffer management */

static buffer_stack_t xdma_buffer_pool_stack;
static reserved_buffer_stack_t reserved_buffer_stack;
BUF_POINTER buffer_list[NUMBER_OF_BUFFER+NUMBER_OF_RESERVED_BUFFER];

static buffer_stack_t* stack = NULL;
static reserved_buffer_stack_t* reserved_stack = NULL;
static BUF_POINTER RESERVED_BUFFER_BASE = NULL;

void relese_buffers(int count) {

    for(int id = 0; id < count; id++) {
        if(buffer_list[id] != NULL) {
            free(buffer_list[id]);
        }
    }
}

int isReservedStackEmpty() {
    return (reserved_stack->top == -1);
}

int isReservedStackFull() {
    return (reserved_stack->top == NUMBER_OF_RESERVED_BUFFER - 1);
}

BUF_POINTER get_reserved_tx_buffer() {

    pthread_mutex_lock(&reserved_stack->mutex);

    if (isReservedStackEmpty()) {
//        debug_printf("Stack is empty. Cannot reserved_buffer_pool_alloc.\n");
        pthread_mutex_unlock(&reserved_stack->mutex);
        return EMPTY_ELEMENT;
    }

    BUF_POINTER poppedElement = reserved_stack->elements[reserved_stack->top];
    reserved_stack->top--;

    pthread_mutex_unlock(&reserved_stack->mutex);

    return poppedElement;
}

int isStackEmpty() {
    return (stack->top == -1);
}

int isStackFull() {
    return (stack->top == NUMBER_OF_POOL_BUFFER - 1);
}

int buffer_pool_free(BUF_POINTER element) {
    if (element >= RESERVED_BUFFER_BASE ) {
        pthread_mutex_lock(&reserved_stack->mutex);

        if (isReservedStackFull()) {
            debug_printf("Stack is full. Cannot buffer_pool_free.\n");
            pthread_mutex_unlock(&reserved_stack->mutex);
            return -1;
        }

        reserved_stack->top++;
        reserved_stack->elements[reserved_stack->top] = element;

        pthread_mutex_unlock(&reserved_stack->mutex);
    }
    else {
        pthread_mutex_lock(&stack->mutex);

        if (isStackFull()) {
            debug_printf("Stack is full. Cannot buffer_pool_free.\n");
            pthread_mutex_unlock(&stack->mutex);
            return -1;
        }

        stack->top++;
        stack->elements[stack->top] = element;

        pthread_mutex_unlock(&stack->mutex);
    }

    return 0;
}

BUF_POINTER buffer_pool_alloc() {
    pthread_mutex_lock(&stack->mutex);

    if (isStackEmpty()) {
        debug_printf("Stack is empty. Cannot buffer_pool_alloc.\n");
        pthread_mutex_unlock(&stack->mutex);
        return EMPTY_ELEMENT;
    }

    BUF_POINTER poppedElement = stack->elements[stack->top];
    stack->top--;

    pthread_mutex_unlock(&stack->mutex);

    return poppedElement;
}

void quickSort(BUF_POINTER arr[], int left, int right) {

    int i = left, j = right;
    BUF_POINTER pivot = arr[(left + right) / 2];

    while (i <= j) {
        while (arr[i] < pivot) {
            i++;
        }
        while (arr[j] > pivot) {
            j--;
        }
        if (i <= j) {
            BUF_POINTER temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
            i++;
            j--;
        }
    }

    if (left < j) {
        quickSort(arr, left, j);
    }
    if (i < right) {
        quickSort(arr, i, right);
    }
}

int initialize_buffer_allocation() {

    char *buffer;
    int id;

    stack = &xdma_buffer_pool_stack;
    reserved_stack = &reserved_buffer_stack;

    stack->top = -1;
    pthread_mutex_init(&stack->mutex, NULL);

    reserved_stack->top = -1;
    pthread_mutex_init(&reserved_stack->mutex, NULL);

    for(id = 0; id < (NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER); id++) {
        buffer = NULL;

        if(posix_memalign((void **)&buffer, BUFFER_ALIGNMENT /*alignment */, MAX_BUFFER_LENGTH + BUFFER_ALIGNMENT)) {
            fprintf(stderr, "OOM %u.\n", MAX_BUFFER_LENGTH);
            relese_buffers(id);
            pthread_mutex_destroy(&stack->mutex);
            pthread_mutex_destroy(&reserved_stack->mutex);
            return -1;
        }
        buffer_list[id] = buffer;
    }

    for (id = 0; id < (NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER); id++) {
        debug_printf("%p\n", buffer_list[id]);
    }
    debug_printf("\n");

    quickSort(buffer_list, 0, (NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER - 1));

    for (id = 0; id < (NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER); id++) {
        debug_printf("%p\n", buffer_list[id]);
    }
    debug_printf("\n");

    RESERVED_BUFFER_BASE = buffer_list[NUMBER_OF_BUFFER];

    for(id = 0; id < (NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER); id++) {
        if(buffer_pool_free(buffer_list[id])) {
            relese_buffers(id + 1);
            pthread_mutex_destroy(&stack->mutex);
            pthread_mutex_destroy(&reserved_stack->mutex);
            return -1;
        }
    }

    printf("      stack->elements[%4d]: %p\n", 0, stack->elements[0]);
    printf("      stack->elements[%4d]: %p\n", NUMBER_OF_BUFFER-1, stack->elements[NUMBER_OF_BUFFER-1]);
    printf("       RESERVED_BUFFER_BASE: %p\n", RESERVED_BUFFER_BASE);
    printf("reserved_stack->elements[%d]: %p\n", 0, reserved_stack->elements[0]);
    printf("reserved_stack->elements[%d]: %p\n", NUMBER_OF_RESERVED_BUFFER-1, reserved_stack->elements[NUMBER_OF_RESERVED_BUFFER-1]);

    printf("Successfully allocated buffers(%u)\n", NUMBER_OF_BUFFER+NUMBER_OF_RESERVED_BUFFER);

    return 0;
}

void buffer_release() {

    relese_buffers(NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER);
    pthread_mutex_destroy(&stack->mutex);
    pthread_mutex_destroy(&reserved_stack->mutex);

    printf("Successfully release buffers(%u)\n", NUMBER_OF_BUFFER + NUMBER_OF_RESERVED_BUFFER);
}

