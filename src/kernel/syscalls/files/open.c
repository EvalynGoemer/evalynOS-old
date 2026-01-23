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

    struct fd* file = malloc(sizeof(struct fd));
    file->file_data = malloc(size);
    fs_read(fileName, file->file_data, size);
    file->seek_pos = 0;
    file->file_name = malloc(strlen(fileName));
    strcpy(file->file_name, fileName);
    get_current_thread()->fds[get_current_thread()->next_fd] = *file;
    frame->rax = get_current_thread()->next_fd++;

    free(fileName);
}
