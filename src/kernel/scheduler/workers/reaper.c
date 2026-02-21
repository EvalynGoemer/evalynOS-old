#include <scheduler/scheduler.h>
#include <utils/spinlock.h>
#include <stdlib.h>

spinlock_t reaper_spinlock = {0};
struct thread* threads_to_reap = NULL;

struct thread* get_next_thread_to_reap() {
    bool lock1r = spinlock_lock(&reaper_spinlock);
    if (threads_to_reap == NULL) {
        spinlock_unlock(&reaper_spinlock, lock1r);
        return NULL;
    }

    struct thread* thread = threads_to_reap;
    threads_to_reap = threads_to_reap->next_thread;
    thread->next_thread = NULL;

    spinlock_unlock(&reaper_spinlock, lock1r);
    return thread;
}

void reaper_thread() {
    while (1) {
        struct thread* thread = get_next_thread_to_reap();
        if (thread) {
            delete_pagemap(thread->pagemap);
            free(thread->fds);
            free(thread->stack);
            free(thread);
        } else {
            schedule();
        }
    }
}
