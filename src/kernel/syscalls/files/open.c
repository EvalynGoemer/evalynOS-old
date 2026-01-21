#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>

#include <stdlib.h>
#include <string.h>

// TODO: make work non tarFS filesystems
// TODO: make not hacky lmao
void sys_open(struct syscall_frame* frame) {
    int size = tarfsGetFize((char*)frame->rbx);

    if (size < 0) {
        frame->rax = -1;
        return;
    }

    struct fd* file = malloc(sizeof(struct fd));
    file->file_data = malloc(size);
    fs_read((char*)frame->rbx, file->file_data, size);
    file->seek_pos = 0;
    file->file_name = malloc(strlen((char*)frame->rbx));
    strcpy(file->file_name, (char*)frame->rbx);
    get_current_thread()->fds[get_current_thread()->next_fd] = *file;
    frame->rax = get_current_thread()->next_fd++;
}
