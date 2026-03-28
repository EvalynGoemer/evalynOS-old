#pragma once
#include <stdint.h>

#define SYSCALL_GET_THREAD_ID    0
#define SYSCALL_PRINT            1
#define SYSCALL_SET_FS_BASE      2
#define SYSCALL_MMAP             3
#define SYSCALL_OPEN             4
#define SYSCALL_READ             5
#define SYSCALL_SEEK             6
#define SYSCALL_SLEEP_MS         7
#define SYSCALL_EXIT             8
#define SYSCALL_STDIN            9
#define SYSCALL_PCSPKR_PLAY      10
#define SYSCALL_PCSPKR_STOP      11
#define SYSCALL_GET_MS           22
#define SYSCALL_FB_MAP           30
#define SYSCALL_FB_GET_PITCH     31
#define SYSCALL_PS2_GET_SCANCODE 40

typedef struct syscall_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
} syscall_frame_t;

extern void init_syscall();
extern void syscall_handler();
extern void execute_syscall(struct syscall_frame* frame);
