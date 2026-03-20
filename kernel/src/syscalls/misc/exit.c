#include "utils/panic.h"
#include <scheduler/scheduler.h>
#include <syscalls/syscalls.h>

void sys_exit(struct syscall_frame* frame) {
    (void)frame;
    get_current_thread()->thread_state = THREAD_STATE_REAPING;
    schedule();
    panic("Thread in reaping process ran");
}
