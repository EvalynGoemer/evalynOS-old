#include <stdlib.h>
#include <syscalls/syscalls.h>
#include <utils/safe_user_funcs.h>
#include <drivers/x86_64/rflags.h>
#include <stdio.h>

void sys_print(struct syscall_frame* frame) {
    char* user_ptr = (char*)frame->rbx;
    int len = strlen_user(user_ptr);

    if (len < 0)
        goto fail;

    char* kstring = malloc(len + 1);
    if (!kstring)
        goto fail;

    if (copy_from_user(kstring, user_ptr, len + 1) != 0) {
        free(kstring);
        goto fail;
    }

    kstring[len] = '\0';
    printf("%s", kstring);

    free(kstring);
    frame->rax = 0;
    return;

    fail:
    frame->rax = -1;
}
