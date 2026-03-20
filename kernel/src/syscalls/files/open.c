#include "utils/safe_user_funcs.h"
#include <syscalls/syscalls.h>
#include <scheduler/scheduler.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>
#include <drivers/x86_64/rflags.h>

#include <stdlib.h>
#include <string.h>

// TODO: make work non tarFS filesystems
// TODO: make not hacky lmao
void sys_open(struct syscall_frame* frame) {
    int fileNameLength = strlen_user((char*)frame->rbx);
    if (fileNameLength  == -1) {
        frame->rax = -1;
        return;
    }

    char* fileName = malloc(fileNameLength + 1);
    if (copy_from_user(fileName, (char*)frame->rbx, fileNameLength + 1) != 0) {
        free(fileName);
        frame->rax = -1;
        return;
    }

    int size = tarfsGetFize(fileName);

    if (size < 0) {
        free(fileName);
        frame->rax = -1;
        return;
    }

    int fd = get_current_thread()->next_fd;

    if (fd >= 256) {
        free(fileName);
        frame->rax = -1;
        return;
    }

    struct fd* file = &get_current_thread()->fds[fd];

    file->file_data = malloc(size);
    if (!file->file_data) {
        free(fileName);
        frame->rax = -1;
        return;
    }

    if (fs_read(fileName, file->file_data, size) < 0) {
        free(file->file_data);
        free(fileName);
        frame->rax = -1;
        return;
    }

    file->seek_pos = 0;

    file->file_name = malloc(strlen(fileName) + 1);
    if (!file->file_name) {
        free(file->file_data);
        free(fileName);
        frame->rax = -1;
        return;
    }

    strcpy(file->file_name, fileName);

    frame->rax = fd;
    get_current_thread()->next_fd++;

    free(fileName);
}
