#include <syscalls/syscalls.h>

extern void sys_print(struct syscall_frame* frame);
extern void sys_get_thread_id(struct syscall_frame* frame);
extern void sys_get_ms(struct syscall_frame* frame);
extern void sys_sleep_ms(struct syscall_frame* frame);
extern void sys_exit(struct syscall_frame* frame);
