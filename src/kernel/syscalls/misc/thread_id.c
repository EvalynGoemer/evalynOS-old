#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>

void sys_get_thread_id(struct syscall_frame* frame) {
    frame->rax = get_current_thread()->threadId;
}
