#include <syscalls/syscalls.h>

extern void sys_open(struct syscall_frame* frame);
extern void sys_read(struct syscall_frame* frame);
extern void sys_seek(struct syscall_frame* frame);
