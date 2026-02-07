#include <scheduler/scheduler.h>
#include <stdlib.h>

struct thread* threads_to_reap = NULL;

struct thread* get_next_thread_to_reap() {
    if (threads_to_reap == NULL) {
        return NULL;
    }

    struct thread* thread = threads_to_reap;
    threads_to_reap = threads_to_reap->next_thread;
    thread->next_thread = NULL;

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
