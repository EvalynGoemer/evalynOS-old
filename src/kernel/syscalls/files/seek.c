#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// TODO: make work non tarFS filesystems
// TODO: make not hacky lmao
void sys_seek(struct syscall_frame* frame) {
    long offset = frame->rdx;
    int whence = frame->rsi;

    struct fd file = get_current_thread()->fds[frame->rbx];

    if(file.file_name == NULL) {
        frame->rax = -1;
        return;
    }

    int size = tarfsGetFize(file.file_name);

    long new_pos;

    if (whence == SEEK_SET) {
        new_pos = offset;
    } else if (whence == SEEK_CUR) {
        new_pos = file.seek_pos + offset;
    } else if (whence == SEEK_END) {
        new_pos = size + offset;
    } else {
        frame->rax = -1;
        return;
    }

    if (new_pos < 0)
        new_pos = 0;
    if (new_pos > size)
        new_pos = size;

    file.seek_pos = new_pos;

    get_current_thread()->fds[frame->rbx] = file;
    frame->rax = new_pos;
}
