#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>
#include <drivers/timer.h>

void sys_sleep_ms(struct syscall_frame* frame) {
    get_current_thread()->sleep_awake_time = timer_get_ms() + frame->rbx;
    schedule();
}
