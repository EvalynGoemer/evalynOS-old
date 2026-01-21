#include <syscalls/syscalls.h>
#include <stdio.h>

// TODO: make safer lmao
void sys_print(struct syscall_frame* frame) {
    printf("%s", (char*)frame->rbx);
    frame->rax = 0;
}
