#include <syscalls/syscalls.h>
#include <filesystem/filesystem.h>

void sys_ps2_get_scancode(struct syscall_frame* frame) {
    char keyPressed[1] = {'\0'};
    int read = fs_read("/dev/ps2/kbd", keyPressed, 1);
    if (read == 0) {
        frame->rax = 0;
    } else {
        frame->rax = keyPressed[0];
    }
}
