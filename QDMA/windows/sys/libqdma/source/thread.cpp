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

#include "thread.h"
#include "qdma.h"
#include "trace.h"

#ifdef ENABLE_WPP_TRACING
#include "thread.tmh"
#endif

using namespace xlnx;
KDEFERRED_ROUTINE queueDataDpc;

VOID qdma_poll_thread(
    PVOID context)
{
    qdma_thread *th = (qdma_thread *)context;
    KAFFINITY affinity = (KAFFINITY)1 << th->id;
    KeSetSystemAffinityThread(affinity);

    TraceVerbose(TRACE_THREAD, "Thread active on CPU core : %lu", th->id);

    while (1) {

        KeWaitForSingleObject(&th->semaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        InterlockedDecrement(&th->sem_count);

        if (th->terminate) {
            TraceVerbose(TRACE_THREAD, "Terminating thread on CPU core: %lu", th->id);
            PsTerminateSystemThread(STATUS_SUCCESS);
        }

        WdfSpinLockAcquire(th->lock);

        PLIST_ENTRY entry;
        PLIST_ENTRY temp;
        LIST_FOR_EACH_ENTRY_SAFE(&th->poll_ops_head, temp, entry) {
            poll_operation_entry *poll_op = CONTAINING_RECORD(entry, poll_operation_entry, list_entry);
            poll_op->op.fn(poll_op->op.arg);
        }

        WdfSpinLockRelease(th->lock);
    }
}

ULONG thread_manager::get_active_proc_cnt(void)
{
    return KeQueryActiveProcessorCount(NULL);
}

_Use_decl_annotations_
VOID
queueDataDpc(
    PKDPC dpc,
    PVOID context,
    PVOID arg1,
    PVOID arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(arg2);

    qdma_thread* thread = static_cast<qdma_thread *>(arg1);
    wakeup_thread(thread);
}

void thread_manager::init_dpc(qdma_thread *thread) {
    KeInitializeDpc(&thread->dpc, queueDataDpc, thread);
}

NTSTATUS thread_manager::create_sys_threads(queue_op_mode mode)
{
    ULONG i = 0;
    NTSTATUS status = STATUS_SUCCESS;

    active_processors = get_active_proc_cnt();

    threads = static_cast<qdma_thread *>(qdma_calloc(active_processors, sizeof(qdma_thread)));
    if (nullptr == threads) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < active_processors; ++i) {
        /* THREAD DATA INITIALIZATION */
        threads[i].weight = 0;
        threads[i].id = i;
        threads[i].terminate = false;
        INIT_LIST_HEAD(&threads[i].poll_ops_head);

        KeInitializeSemaphore(&threads[i].semaphore, 0, MAXLONG);

        status = PsCreateSystemThread(&threads[i].th_handle,
                                      (ACCESS_MASK)0,
                                      NULL,
                                      (HANDLE)0,
                                      NULL,
                                      qdma_poll_thread,
                                      &threads[i]);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_THREAD, "Failed to create thread on cpu core %d - %!STATUS!", i, status);
            break;
        }

        ObReferenceObjectByHandle(threads[i].th_handle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  KernelMode,
                                  &threads[i].th_object,
                                  NULL);

        ZwClose(threads[i].th_handle);

        status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &threads[i].lock);
        if (!NT_SUCCESS(status)) {
            TraceError(TRACE_THREAD, "Failed to create thread spinlock for CPU core %d - %!STATUS!", i, status);
            active_threads = i + 1;
            goto ErrExit;
        }

        /* Per Thread DPC creation */
        if (mode != queue_op_mode::POLL_MODE) {
            init_dpc(&threads[i]);
        }
    }

    active_threads = i;

    if ((ULONG)0 == active_threads) {
        /* If no thread is active, then return failure. */
        goto ErrExit;
    }

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &lock);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_THREAD, "Failed to create thread manager lock %!STATUS!", status);
        goto ErrExit;
    }

    TraceVerbose(TRACE_THREAD, "Active threads %lu", active_threads);

    return status;

ErrExit:
    terminate_sys_threads();
    return status;
}

void thread_manager::terminate_sys_threads(void)
{
    if (nullptr == threads)
        return;

    for (ULONG i = 0; i < active_threads; ++i) {
        threads[i].terminate = true;
        KeReleaseSemaphore(&threads[i].semaphore, 0, 1, FALSE);
    }

    for (ULONG i = 0; i < active_threads; ++i) {
        if (threads[i].lock)
            WdfObjectDelete(threads[i].lock);

        KeWaitForSingleObject(threads[i].th_object,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ObDereferenceObject(threads[i].th_object);
    }

    if (lock) {
        WdfObjectDelete(lock);
        lock = nullptr;
    }

    qdma_memfree(threads);
    threads = nullptr;
}

void err_poll_thread(PVOID context)
{
    err_thread *ctx = (err_thread *)context;
    LARGE_INTEGER  Interval;
    Interval.QuadPart = WDF_REL_TIMEOUT_IN_MS(1000);

    while (1) {
        ctx->device->hw.qdma_hw_error_process(ctx->device);

        if (ctx->terminate) {
            TraceVerbose(TRACE_THREAD, "ctx->terminate");
            PsTerminateSystemThread(STATUS_SUCCESS);
        }

        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }
}

NTSTATUS thread_manager::create_err_poll_thread(qdma_device *device)
{
    err_th_para = static_cast<err_thread *>(qdma_calloc(1, sizeof(err_thread)));
    if (nullptr == err_th_para) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* THREAD DATA INITIALIZATION */
    err_th_para->terminate = false;
    err_th_para->device = device;

    auto status = PsCreateSystemThread(&err_th_para->th_handle,
                                       (ACCESS_MASK)0,
                                       NULL,
                                       (HANDLE)0,
                                       NULL,
                                       err_poll_thread,
                                       err_th_para);
    if (!NT_SUCCESS(status)) {
        TraceError(TRACE_THREAD, "Failed to create Error handling thread - %!STATUS!", status);
        qdma_memfree(err_th_para);
        err_th_para = nullptr;
        return status;
    }

    ObReferenceObjectByHandle(err_th_para->th_handle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &err_th_para->th_object,
        NULL);

    ZwClose(err_th_para->th_handle);

    TraceVerbose(TRACE_THREAD, "Error handling Thread Active");

    return STATUS_SUCCESS;
}

void thread_manager::terminate_err_poll_thread(void)
{
    TraceVerbose(TRACE_THREAD, "terminating error handling thread");

    if (err_th_para == nullptr)
        return;

    err_th_para->terminate = true;

    KeWaitForSingleObject(err_th_para->th_object,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ObDereferenceObject(err_th_para->th_object);

    qdma_memfree(err_th_para);
    err_th_para = nullptr;
}

poll_operation_entry *thread_manager::register_poll_function(poll_op& ops)
{
    UINT32 weight;
    UINT32 sel_weight_id;

    WdfSpinLockAcquire(lock);

    weight = threads[0].weight;
    sel_weight_id = 0;

    for (ULONG i = 1; i < active_threads; ++i) {
        if (weight > threads[i].weight) {
            weight = threads[i].weight;
            sel_weight_id = i;
        }
    }

    threads[sel_weight_id].weight++;
    WdfSpinLockRelease(lock);

    poll_operation_entry *poll_op_entry = static_cast<poll_operation_entry *>
                                          (qdma_calloc(1, sizeof(poll_operation_entry)));
    if (nullptr == poll_op_entry) {
        return nullptr;
    }

    poll_op_entry->op.fn = ops.fn;
    poll_op_entry->op.arg = ops.arg;
    poll_op_entry->thread = &threads[sel_weight_id];

    /* To Avoid Visual studio static analysis reported warning */
    WDFSPINLOCK th_lock = threads[sel_weight_id].lock;
    WdfSpinLockAcquire(th_lock);
    LIST_ADD_TAIL(&threads[sel_weight_id].poll_ops_head, &poll_op_entry->list_entry);
    WdfSpinLockRelease(th_lock);

    TraceVerbose(TRACE_THREAD, "Poll function registered for thread : %u New Weight : %u, Poll Handle : %p",
        sel_weight_id, threads[sel_weight_id].weight, poll_op_entry);

    return poll_op_entry;
}

void thread_manager::unregister_poll_function(poll_operation_entry *poll_entry)
{
    qdma_thread *thread = poll_entry->thread;

    WdfSpinLockAcquire(lock);
    thread->weight--;
    WdfSpinLockRelease(lock);

    WdfSpinLockAcquire(thread->lock);
    PLIST_ENTRY entry = &(poll_entry->list_entry);
    LIST_DEL_NODE(entry);
    WdfSpinLockRelease(thread->lock);

    qdma_memfree(poll_entry);
}

void xlnx::wakeup_thread(qdma_thread* thread)
{
    if (thread->sem_count <= 10) {
        InterlockedIncrement(&thread->sem_count);
        KeReleaseSemaphore(&thread->semaphore, 0, 1, FALSE);
    }
}
