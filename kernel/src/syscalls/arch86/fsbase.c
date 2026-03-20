#include <syscalls/syscalls.h>
#include <drivers/x86_64/msr.h>

void sys_setfsbase(struct syscall_frame* frame) {
    if (frame->rbx <= 0x00007FFFFFFFFFFF) {
        wrmsr(FSBAS, frame->rbx);
        frame->rax = 0;
    } else {
        frame->rax = -1;
    }
}
