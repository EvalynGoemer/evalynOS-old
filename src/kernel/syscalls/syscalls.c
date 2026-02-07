#include <stdio.h>
#include <stdint.h>

#include <syscalls/syscalls.h>
#include <syscalls/memory/memory.h>
#include <syscalls/arch86/arch86.h>
#include <syscalls/misc/misc.h>
#include <syscalls/files/files.h>

#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/rflags.h>

#define MAX_SYSCALLS 256
void (*syscalls[MAX_SYSCALLS])(struct syscall_frame* frame) = {NULL};

void init_syscall() {
    // enable syscall instruction
    uint64_t efer = rdmsr(EFER);
    efer |= (1 << 0);
    wrmsr(EFER, efer);

    uint64_t star = ((uint64_t)(0x18 | 3) << 48) | ((uint64_t)0x08 << 32);
    wrmsr(STAR, star);

    wrmsr(LSTAR, (uint64_t)syscall_handler);
    wrmsr(SFMASK, ~0x2);

    syscalls[SYSCALL_GET_THREAD_ID]    = sys_get_thread_id;
    syscalls[SYSCALL_SET_FS_BASE]      = sys_setfsbase;
    syscalls[SYSCALL_PRINT]            = sys_print;
    syscalls[SYSCALL_MMAP]             = sys_mmap;
    syscalls[SYSCALL_OPEN]             = sys_open;
    syscalls[SYSCALL_READ]             = sys_read;
    syscalls[SYSCALL_SEEK]             = sys_seek;
    syscalls[SYSCALL_SLEEP_MS]         = sys_sleep_ms;
    syscalls[SYSCALL_EXIT]             = sys_exit;
    syscalls[SYSCALL_GET_MS]           = sys_get_ms;
    syscalls[SYSCALL_PCSPKR_PLAY]      = sys_pcspkr_play;
    syscalls[SYSCALL_PCSPKR_STOP]      = sys_pcspkr_stop;
    syscalls[SYSCALL_FB_MAP]           = sys_fb_map;
    syscalls[SYSCALL_FB_GET_PITCH]     = sys_fb_get_pitch;
    syscalls[SYSCALL_PS2_GET_SCANCODE] = sys_ps2_get_scancode;

    printf("SYSCALL: Syscalls setup\n");
}

void execute_syscall(struct syscall_frame* frame) {
    rflags_clr_ac();

    uint64_t syscall = frame->rax;
    if (syscall < MAX_SYSCALLS && syscalls[syscall] != NULL) {
        syscalls[syscall](frame);
    } else {
        frame->rax = -1;
    }
}
