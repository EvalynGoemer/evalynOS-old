#include <syscalls/syscalls.h>
#include <drivers/timer.h>

void sys_get_ms(struct syscall_frame* frame) {
    frame->rax = timer_get_ms();
}
