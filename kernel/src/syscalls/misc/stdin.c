#include <syscalls/syscalls.h>
#include <filesystem/filesystem.h>
#include <scheduler/scheduler.h>
#include <drivers/timer.h>
#include <stdio.h>
#include <utils/safe_user_funcs.h>
#include <drivers/keyboard.h>

#define LINE_BUF_SIZE 256
static char line_buffer[LINE_BUF_SIZE];
static int  line_len = 0;

void sys_do_hacky_stdin(struct syscall_frame *frame) {
    void *ubuf = (void *)frame->rbx;
    uint64_t count = frame->rdx;

    for (;;) {
        char keyPressed = '\0';
        fs_read("/dev/kbd", &keyPressed, 1);

        if (keyPressed == '\0') {
            get_current_thread()->sleep_awake_time = timer_get_ms() + 1;
            schedule();
            continue;
        }

        unsigned char c = (unsigned char)keyPressed;
        if (c == '\r') c = '\n';

        if (c == '\n') {
            printf("\n");
            line_buffer[line_len++] = '\n';
            break;
        }

        if (c == '\b' || c == 0x7F) {
            if (line_len > 0) {
                line_len--;
                printf("\b \b");
            }
            continue;
        }

        if (c < 0x20 || c > 0x7E)
            continue;

        printf("%c", c);
        line_buffer[line_len++] = (char)c;

        if (line_len >= LINE_BUF_SIZE - 1) {
            line_buffer[line_len++] = '\n';
            printf("\n");
            break;
        }
    }

    uint64_t to_copy = (uint64_t)line_len < count ? (uint64_t)line_len : count;
    copy_to_user(ubuf, line_buffer, to_copy);
    frame->rax = to_copy;
    line_len = 0;
}
