#include <syscalls/syscalls.h>
#include <drivers/x86_64/pcskpr.h>

void sys_pcspkr_play(struct syscall_frame* frame) {
    play_sound(frame->rbx);
    frame->rax = 0;
}

void sys_pcspkr_stop(struct syscall_frame* frame) {
    stop_sound();
    frame->rax = 0;
}
