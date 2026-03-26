#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils/panic.h>
#include <filesystem/filesystem.h>
#include <memory/debug.h>
#include <memory/pmm.h>
#include <drivers/x86_64/fred/fred.h>
#include <memory/vmm.h>
#include <drivers/x86_64/rflags.h>
#include <scheduler/scheduler.h>
#include <scheduler/switch.h>

#include <drivers/mouse.h>

#include <elf/elf.h>

#include "shell.h"
#include "drivers/timer.h"

void spawn_app_kthread(char* path) {
    void* elf_file = malloc(16 * 1024 * 1024);
    fs_read(path, elf_file, 16 * 1024 * 1024);
    struct elf_info info = load_elf(elf_file, get_current_thread()->pagemap);
    free(elf_file);
    free(path);

    size_t stack_size = 64 * 1024;
    uintptr_t stack_top = valloc(get_current_thread()->pagemap, stack_size);
    stack_top += stack_size;

    for (size_t offset = 0; offset < stack_size; offset += PAGE_SIZE) {
        uintptr_t va = stack_top - PAGE_SIZE - offset;
        uintptr_t pa = (uintptr_t)allocate_page();
        vmm_map_page(get_current_thread()->pagemap, va, pa, PTE_PRESENT | PTE_USER | PTE_WRITABLE);
    }

    uintptr_t sp = stack_top & ~0xF;
    uint64_t *stack = (uint64_t *)sp;
    rflags_set_ac();
    *--stack = 0; // alignment
    *--stack = 0; // AT_NULL value
    *--stack = 0; // AT_NULL type
    *--stack = info.entry_point;
    *--stack = 9; // AT_ENTRY
    *--stack = info.phdr;
    *--stack = 3; // AT_PHDR
    *--stack = info.phentsize;
    *--stack = 4; // AT_PHENT
    *--stack = info.phnum;
    *--stack = 5; // AT_PHNUM
    *--stack = 0; // envp
    *--stack = 0; // argv
    *--stack = 0; // argc
    rflags_clr_ac();
    stack_top = (uintptr_t)stack;

    if (info.entry_point != 0) {
        if (fred_enbled) {
            fred_switch_to_user(info.entry_point, stack_top);
        } else {
            switch_to_user(info.entry_point, stack_top);
        }
    }
}
__attribute__((noinline))
void smash_stack() {
    volatile char buf[16];
    for (int i = 0; i < 256; i++) {
        buf[i] = (char)i;
    }
}

char *to_upper(const char *s) {
    static char buf[256];
    char *p = buf;

    if (!s) return NULL;

    while (*s && (size_t)(p - buf) < sizeof(buf) - 1) {
        *p++ = (*s >= 'a' && *s <= 'z') ? *s - ('a' - 'A') : *s;
        s++;
    }
    *p = '\0';
    return buf;
}

volatile int sink;
void execute_commands(const char *cmd) {
    if (strcmp("TEST", to_upper(cmd)) == 0) {
        volatile int v;
        v = 10 ^ sink;
        v = v + 1;
        v = v * 2;
        v = v - 3;
        v = v ^ 0x55;
        v = v * v;
        v = v + 42;
        sink = v;
        printf("Test Command Executed\n");
        return;
    }
    if (strcmp("CREDITS", to_upper(cmd)) == 0) {
        char * data = malloc(16 * 1024);
        fs_read("/credits.txt", data, 16 * 1024);

        bool nextLineMessagePrinted = false;
        int lineCount = 0;
        char *ptr = data;
        char *lineStart = ptr;

        while (*ptr) {
            if (nextLineMessagePrinted) {
                printf("\x1b[2K\r"); // ansi for clear line and return to start of line
                nextLineMessagePrinted = false;
            }

            if (*ptr == '\n') {
                lineCount++;
                *ptr = '\0';
                printf("%s\n", lineStart);
                lineStart = ptr + 1;
            }

            if (lineCount == 20) {
                char keyPressed[1] = {'\0'};

                while (keyPressed[0] == '\0') {
                    if (!nextLineMessagePrinted) {
                        printf("Press any key to continute");
                        nextLineMessagePrinted = true;
                    }
                    schedule();
                    fs_read("/dev/kbd", keyPressed, 1);
                }

                lineCount = 0;
            }

            ptr++;
        }

        if (*lineStart) {
            printf("%s\n", lineStart);
        }

        free(data);
        return;
    }
    if (strcmp("MMAP", to_upper(cmd)) == 0) {
        printMemoryMap();
        return;
    }
    if (strcmp("BADAPPLE", to_upper(cmd)) == 0) {
        pagemap_t* pagemap = new_pagemap();
        char* path = strdup("/badapple.elf");
        create_thread(spawn_app_kthread, pagemap, (args_t){(uint64_t)path,0,0,0,0,0});
        printf("Started playing BAD APPLE in userspace\n");
        return;
    }
    if (strcmp("HELLO", to_upper(cmd)) == 0) {
        pagemap_t* pagemap = new_pagemap();
        char* path = strdup("/hello_world.elf");
        create_thread(spawn_app_kthread, pagemap, (args_t){(uint64_t)path,0,0,0,0,0});
        printf("Started playing HELLO in userspace\n");
        return;
    }
    if (strcmp("FIREWORKS", to_upper(cmd)) == 0) {
        printf("Started fireworks test with HELLO.elf {64k Iterations; 1ms delay}\n");
        for (int i = 0; i < 64 * 1024; i++) {
            if (i % 100 == 0)
                printf("FIREWORKS: Spawning #%d\n", i);
            uint64_t tsc = __rdtsc();
            int sleep_time = (tsc % 1) + 1;
            pagemap_t* pagemap = new_pagemap();
            char* path = strdup("/hello_world.elf");
            create_thread(spawn_app_kthread, pagemap, (args_t){(uint64_t)path,0,0,0,0,0});
            timer_blocking_sleep_ms(sleep_time);
            if (i % 10 == 0)
                schedule();
        }
        return;
    }
    if (strcmp("DOOM", to_upper(cmd)) == 0) {
        pagemap_t* pagemap = new_pagemap();
        char* path = strdup("/doomgeneric.elf");
        create_thread(spawn_app_kthread, pagemap, (args_t){(uint64_t)path,0,0,0,0,0});
        printf("Started playing DOOM in userspace\n");
        return;
    }
    if (strcmp("BASH", to_upper(cmd)) == 0) {
        pagemap_t* pagemap = new_pagemap();
        char* path = strdup("/bash.elf");
        create_thread(spawn_app_kthread, pagemap, (args_t){(uint64_t)path,0,0,0,0,0});
        printf("Started running BASH in userspace\n");
        return;
    }
    if ((strcmp("CLEAR", to_upper(cmd)) == 0) || (strcmp("CLS", to_upper(cmd)) == 0)) {
        printf("\x1b[2J\x1b[H"); // ansi for clear screen and go home
        return;
    }
    if (strcmp("SMASH", to_upper(cmd)) == 0) {
        smash_stack();
        return;
    }
    if (strcmp("PANIC", to_upper(cmd)) == 0) {
        panic("You asked for this lmao");
        return;
    }
    if (strcmp("FAULT", to_upper(cmd)) == 0) {
        volatile uint64_t *fault = (volatile uint64_t *)0xDEADBEEF;
        *fault = 0xDEADBEEF;
        return;
    }

    if (strcmp("MOUSE_TEST", to_upper(cmd)) == 0) {
        enable_mouse_test();
        return;
    }

    if (strcmp("", to_upper(cmd)) != 0) {
        printf("Invalid Command: %s\n", cmd);
    }
}

void start_shell() {
    char typingBuffer[64] = {'\0'};
    int typingBufferIndex = 0;

    bool newLineStarted = true;
    bool noKeysLeft = false;

    while (1) {
        if (noKeysLeft) {
            schedule();
        }

        if (newLineStarted) {
            printf("\x1b[2K\r"); // ansi for clear line and return to start of line
            printf("Kernel Shell> ");
            newLineStarted = false;
        }

        char keyPressed[1] = {'\0'};
        fs_read("/dev/kbd", keyPressed, 1);

        if (keyPressed[0] == '\0') {
            noKeysLeft = true;
        } else {
            noKeysLeft = false;
        }

        switch (keyPressed[0]) {
            case '\b':
                if ((typingBufferIndex - 1) > -1) {
                    typingBuffer[--typingBufferIndex] = '\0';
                    printf("\b \b");
                }
                break;
            case '\x7F': // serial port sends [BACKSPACE (\b)] as [DEL (\x7F)] so pretend it its [BACKSPACE (\b)]
                if ((typingBufferIndex - 1) > -1) {
                    typingBuffer[--typingBufferIndex] = '\0';
                    printf("\b \b");
                }
                break;
            case '\n':
                printf("\n");
                execute_commands(typingBuffer);

                typingBufferIndex = 0;
                memset(typingBuffer, '\0', 64);
                newLineStarted = true;
                break;
            case '\r': // serial port sends [ENTER (\n)] as [RETURN (\r)] so pretend it its [ENTER (\n)]
                printf("\n");
                execute_commands(typingBuffer);

                typingBufferIndex = 0;
                memset(typingBuffer, '\0', 64);
                newLineStarted = true;
                break;
            default:
                if (keyPressed[0] != '\0') {
                    if ((typingBufferIndex + 1) <= 63) {
                        typingBuffer[typingBufferIndex++] = keyPressed[0];
                        printf("%c", keyPressed[0]);
                    }
                }
                break;
        }
    }
}
