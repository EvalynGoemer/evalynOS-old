#include "utils/panic.h"
#include <scheduler/scheduler.h>
#include <syscalls/syscalls.h>
#include <stdio.h>

void sys_exit(struct syscall_frame* frame) {
    (void)frame;
    printf("Process %ld is exiting\n", get_current_thread()->threadId);
    get_current_thread()->thread_state = THREAD_STATE_REAPING;
    schedule();
    panic("Thread in reaping process ran");
}
