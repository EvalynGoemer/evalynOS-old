#include "utils/safe_user_funcs.h"
#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>

#include <stdlib.h>
#include <string.h>

// TODO: make work non tarFS filesystems
// TODO: make not hacky lmao
void sys_read(struct syscall_frame* frame) {
    struct fd file = get_current_thread()->fds[frame->rbx];

    if(file.file_name == NULL) {
        frame->rax = -1;
        return;
    }

    uint64_t size = tarfsGetFize(file.file_name);

    if (file.file_data == NULL) {
        frame->rax = -1;
        return;
    }

    if (file.seek_pos >= size) {
        frame->rax = 0;
        return;
    }

    size_t max_bytes = size - file.seek_pos;
    size_t bytes = frame->rdx > max_bytes ? max_bytes : frame->rdx;

    copy_to_user((void*)frame->rsi, file.file_data + file.seek_pos, bytes);
    file.seek_pos += bytes;
    get_current_thread()->fds[frame->rbx] = file;
    frame->rax = bytes;
}
