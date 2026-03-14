#include "utils/spinlock.h"
#include "utils/cmdline.h"
#include <drivers/x86_64/apic/apic.h>
#include <drivers/timer.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <scheduler/scheduler.h>
#include <scheduler/workers/reaper.h>
#include <drivers/dbgstub/dbgstub.h>
#include <drivers/x86_64/fred/fred.h>
#include <scheduler/switch.h>
#include <filesystem/filesystem.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/gdt.h>
#include <drivers/x86_64/msr.h>
#include <utils/panic.h>

uint64_t STACK_SIZE = 65536;

void task_quit() {
    while (1) {

    }
}

spinlock_t scheduler_spinlock = {0};
struct thread* threads = NULL;
_Atomic int next_thread_id = 0;

void create_thread(void (*entry_point)(void*), pagemap_t *pagemap) {
    int lock1r = spinlock_lock(&scheduler_spinlock);
    bool had_threads = (threads != NULL);

    struct thread* new_thread = malloc(sizeof(struct thread));
    memset(new_thread, 0, sizeof(struct thread));

    // HACK: 0 is reserved as stdout and libc redirects to the syscall
    // TODO: init some FDs for things like stdio properly
    new_thread->next_fd = 11;

    // TODO: make this a dynamic array so it cant overflow :^)
    new_thread->fds = malloc(256 * sizeof(struct fd));
    memset(new_thread->fds, 0, 256 * sizeof(struct fd));

    new_thread->heap_pos = 0x00000000B0000000;

    new_thread->stack = malloc(STACK_SIZE);
    memset(new_thread->stack, 0, STACK_SIZE);
    new_thread->stack_top = (void *)(((uintptr_t)new_thread->stack + STACK_SIZE) & ~0xF3ULL);

    new_thread->threadId = next_thread_id;
    next_thread_id++;

    if (pagemap != NULL) {
        new_thread->pagemap = pagemap;
        new_thread->is_user_task = 1;
    } else {
        new_thread->pagemap = &kernel_pagemap;
        new_thread->is_user_task = 0;
    }

    uint64_t *stack = (uint64_t *)new_thread->stack_top;

    if (entry_point == NULL) {
        entry_point = task_quit;
    }
    *--stack = (uint64_t)task_quit;
    *--stack = (uint64_t)entry_point;

    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;

    new_thread->krsp = (uint64_t)stack;

    if (!had_threads) {
        threads = new_thread;
        new_thread->next_thread = new_thread;
    } else {
        new_thread->next_thread = threads->next_thread;
        threads->next_thread = new_thread;
    }
    spinlock_unlock(&scheduler_spinlock, lock1r);
}

void schedule() {
    if (dbgstub_enabled)
        if (!dbgstub_should_preempt())
            return;
    int lock1r = spinlock_lock(&scheduler_spinlock);
    // Make sure IRQL is set to zero when unlocking as this
    // can be called from an ISR which will not return and
    // lower the IRQL properly. This is safe because this can
    // only be called from contexts where IRQL should become
    // zero after. EG the timer IRQ or places where you
    // are not holding onto a spinlock
    lock1r = 0;

    struct thread *previous_thread = threads;
    threads = threads->next_thread;
    struct thread *current_thread = threads;

    while (threads->sleep_awake_time > timer_get_ms()) {
        threads = threads->next_thread;
        current_thread = threads;
    }

    while (current_thread->thread_state == THREAD_STATE_REAPING) {
        int lock2r = spinlock_lock(&reaper_spinlock);
        struct thread *to_reap = current_thread;
        previous_thread->next_thread = current_thread->next_thread;
        threads = current_thread->next_thread;
        current_thread = threads;
        to_reap->next_thread = NULL;

        if (threads_to_reap == NULL) {
            threads_to_reap = to_reap;
        } else {
            to_reap->next_thread = threads_to_reap;
            threads_to_reap = to_reap;
        }
        spinlock_unlock(&reaper_spinlock, lock2r);
    }

    if (current_thread == previous_thread) {
        spinlock_unlock(&scheduler_spinlock, lock1r);
        return;
    }

    vmm_switch_to(current_thread->pagemap);

    if (fred_enbled) {
        wrmsr(FRED_RSP0, (uint64_t)current_thread->stack_top);
    }

    tss.rsp0 = (uint64_t)current_thread->stack_top;

    if (previous_thread->is_user_task) {
        asm volatile("fxsave %0 "::"m"(previous_thread->fpu_state));
        previous_thread->fsbase = rdmsr(FSBAS);
    }

    if (current_thread->is_user_task) {
        asm volatile("fxrstor %0 "::"m"(current_thread->fpu_state));
        wrmsr(FSBAS, current_thread->fsbase);
    }

    wrmsr(UGSBAS, (uint64_t)current_thread);

    uint64_t* old_rsp = &previous_thread->krsp;
    uint64_t  new_rsp = current_thread->krsp;
    thread_switch(&scheduler_spinlock, lock1r, old_rsp, new_rsp);
}

struct thread *get_current_thread() {
    int lock1r = spinlock_lock(&scheduler_spinlock);
    struct thread* tmp = threads;
    spinlock_unlock(&scheduler_spinlock, lock1r);
    return tmp;
}
