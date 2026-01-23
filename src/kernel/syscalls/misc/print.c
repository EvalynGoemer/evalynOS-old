#include <stdlib.h>
#include <syscalls/syscalls.h>
#include <utils/safe_user_funcs.h>
#include <drivers/x86_64/rflags.h>
#include <stdio.h>

void sys_print(struct syscall_frame* frame) {
    int stringLength = strlen_user((char*)frame->rbx);
    if (stringLength != -1) {
        char* kstring = malloc(stringLength + 1);
        if (copy_from_user(kstring, (char*)frame->rbx, stringLength + 1) == 0) {
            printf("%s", kstring);
        } else {
            // TODO: send SIGSEGV or quit process
            free(kstring);
        }
        free(kstring);
    } else {
        // TODO: send SIGSEGV or quit process
    }
    frame->rax = 0;
}
