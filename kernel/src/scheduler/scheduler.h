#pragma once
#include <stdint.h>
#include <memory/vmm.h>

#define THREAD_STATE_RUNNING 0
#define THREAD_STATE_REAPING 1
#define THREAD_STATE_ZOMBIE  2

struct fd {
    char* file_name;
    void* file_data;
    uint64_t seek_pos;
};

struct thread {
    uint64_t threadId;

    void* stack;
    void* stack_top;
    uint64_t krsp;
    uint64_t ursp;

    pagemap_t* pagemap;

    uint64_t heap_pos;

    int next_fd;
    struct fd* fds;

    int is_user_task;
    uint64_t sleep_awake_time;

    uint64_t thread_state;
    struct thread* next_thread;

    uint64_t fsbase;
    char fpu_state[512] __attribute__((aligned(16)));
};

typedef struct args {
    uint64_t arg1, arg2, arg3, arg4, arg5, arg6;
} args_t;

#define NO_ARGS (args_t){0}

void create_thread(void* entry_point, pagemap_t *pagemap, args_t args);
extern void schedule();

struct thread *get_current_thread();
