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

#include "qdma_platform_env.h"
#include "qdma_config.h"
#include "qdma_exports.h"
#include "qdma_reg_ext.h"

namespace xlnx {
/* Forward declaration */
class qdma_device;
struct queue_pair;

typedef void (*poll_fn) (void *);

struct poll_op {
    poll_fn fn;
    void *arg;
};

struct qdma_thread {
    bool terminate;
    ULONG id;
    HANDLE th_handle;
    KSEMAPHORE semaphore;
    volatile LONG sem_count = 0;
    void *th_object;
    UINT32 weight;
    WDFSPINLOCK lock;
    /* Per thread DPC object to wake up the thread for completion processing */
    KDPC dpc;
    LIST_ENTRY poll_ops_head;
};

struct poll_operation_entry {
    poll_op op;
    qdma_thread *thread;
    LIST_ENTRY list_entry;
};

struct err_thread {
    bool terminate;
    HANDLE th_handle;
    void *th_object;
    qdma_device *device = nullptr;
};

struct thread_manager {
    ULONG active_processors = 0;
    ULONG active_threads = 0;
    WDFSPINLOCK lock = nullptr;

    qdma_thread *threads = nullptr;
    err_thread *err_th_para = nullptr;

    ULONG get_active_proc_cnt(void);
    NTSTATUS create_sys_threads(queue_op_mode mode);
    void terminate_sys_threads(void);
    void init_dpc(qdma_thread *thread);
    NTSTATUS create_err_poll_thread(qdma_device *device);
    void terminate_err_poll_thread(void);

    poll_operation_entry *register_poll_function(poll_op& ops);
    void unregister_poll_function(poll_operation_entry *poll_entry);
};

void wakeup_thread(qdma_thread *thread);

} /* namespace xlnx */


