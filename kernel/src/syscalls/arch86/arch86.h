#include <syscalls/syscalls.h>

extern void sys_setfsbase(struct syscall_frame* frame);
extern void sys_pcspkr_play(struct syscall_frame* frame);
extern void sys_pcspkr_stop(struct syscall_frame* frame);
extern void sys_ps2_get_scancode(struct syscall_frame* frame);
