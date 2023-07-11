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

static buffer_stack_t buffer_pool_stack;
BUF_POINTER buffer_list[NUMBER_OF_BUFFER];

static buffer_stack_t* stack = NULL;

void relese_buffers(int count) {

    for(int id = 0; id < count; id++) {
        if(buffer_list[id] != NULL) {
            free(buffer_list[id]);
        }
    }
}

int isStackEmpty() {
    return (stack->top == -1);
}

int isStackFull() {
    return (stack->top == NUMBER_OF_POOL_BUFFER - 1);
}

int push(BUF_POINTER element) {
    pthread_mutex_lock(&stack->mutex);

    if (isStackFull(stack)) {
        debug_printf("Stack is full. Cannot push.\n");
        pthread_mutex_unlock(&stack->mutex);
        return -1;
    }

    stack->top++;
    stack->elements[stack->top] = element;

    pthread_mutex_unlock(&stack->mutex);

    return 0;
}

BUF_POINTER pop() {
    pthread_mutex_lock(&stack->mutex);

    if (isStackEmpty(stack)) {
        debug_printf("Stack is empty. Cannot pop.\n");
        pthread_mutex_unlock(&stack->mutex);
        return EMPTY_ELEMENT;
    }

    BUF_POINTER poppedElement = stack->elements[stack->top];
    stack->top--;

    pthread_mutex_unlock(&stack->mutex);

    return poppedElement;
}

int initialize_buffer_allocation() {

    char *buffer;

    stack = &buffer_pool_stack;

    stack->top = -1;
    pthread_mutex_init(&stack->mutex, NULL);

    for(int id = 0; id < NUMBER_OF_BUFFER; id++) {
        buffer = NULL;

        if(posix_memalign((void **)&buffer, BUFFER_ALIGNMENT /*alignment */, MAX_BUFFER_LENGTH + BUFFER_ALIGNMENT)) {
            fprintf(stderr, "OOM %u.\n", MAX_BUFFER_LENGTH);
            relese_buffers(id);
            pthread_mutex_destroy(&stack->mutex);
            return -1;
        }

        buffer_list[id] = buffer;
        if(push(buffer)) {
            relese_buffers(id + 1);
            pthread_mutex_destroy(&stack->mutex);
            return -1;
        }
    }

    printf("Successfully allocated buffers(%u)\n", NUMBER_OF_BUFFER);

    return 0;
}

void buffer_release() {

    relese_buffers(NUMBER_OF_BUFFER);
    pthread_mutex_destroy(&stack->mutex);

    printf("Successfully release buffers(%u)\n", NUMBER_OF_BUFFER);
}

