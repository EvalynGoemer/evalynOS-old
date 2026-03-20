#include <syscalls/syscalls.h>

extern void sys_fb_map(struct syscall_frame* frame);
extern void sys_fb_get_pitch(struct syscall_frame* frame);
extern void sys_mmap(struct syscall_frame* frame);
