#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <symbols.h>
#include <utils/globals.h>
#include <drivers/fb_renderer.h>
#include <misc/kpanic_image.h>
#include <interupts/interupts.h>
#include <drivers/x86_64/pcskpr.h>

struct descriptor_table_ptr {
    unsigned short limit;
    unsigned long base;
} __attribute__((packed));


static inline uint64_t read_tsc() {
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t xorshift64star() {
    static uint64_t x = 0;
    if (x == 0) {
        x = read_tsc();
    }

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
}

static int rand_between(int min, int max) {
    if (min > max) {
        int tmp = min;
        min = max;
        max = tmp;
    }

    uint64_t range = (uint64_t)(max - min + 1);
    uint64_t x, limit;

    limit = UINT64_MAX - (UINT64_MAX % range);

    do {
        x = xorshift64star();
    } while (x >= limit);

    return min + (x % range);
}

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))
static const char *error_mssages[] = {
    "The kernel is fucked dawg",
    "The kernel got franned",
    "I like elephants and god likes elephants",
    "Electric Boogaloo",
    "Everybody do the flop",
    "I like trains",
    "Hey, somebody kill me",
    "Miku, Miku, oooeeuuu",
    "Poteto Chips",
    "Im nothing like the rest of them but trust me im a hidden gem",
    "When you know it's some DNS bullshit but you can't prove it",
    "youtu.be/dQw4w9WgXcQ",
    "SHAWWWWWW",
    "GIT GUD",
    "Deltarune Tomorrow",
    "Better than XiOS",
    "Number Fifteen",
    "Talk is cheap. Show me the code.",
    "Advancement Get: How did we get here?",
    "Keyboard Cat!",
    "r/ihavereddit",
    "Prisencolinensinainciusol",
    "The Sun is a Deadly Lazer",
    "FUCK MICROSOFT",
    "FAILURE AFTER FAILURE AFTER FAILURE",
    "THE RESULTS REFUSE TO ALTER",
    "AGAIN AND AGAIN AND AGAIN AND AGAIN",
    "MY FAITH BEGINS TO FALTER",
    "MANKIND IS A FAILURE",
    "FREE WILL IS A FLAW",
    "YOU.. INSIGNIFICANT.. FUCK! THIS IS NOT OVER!",
    "I HAVE CREATED HELL... And now I can no longer unmake it...",
    "Good Girl :3",
    "Albuquerque",
    "They've got allen wrenches, gerbil feeders, toilet seats, electric heaters...",
    "A Platypus?",
    "Grass grows, birds fly, sun shines, and friend, I crash kernels",
    "On days like these, kernels like you... should be burning in hell.",
    "PERRY THE PLATYPUS!",
    "The Kernel-Panic-Inator",
    "My Name is Doof and you will do what i say. WOOP WOOP!",
    "Ferb i know what we are gonna do today",
    "You got any glazed donuts?",
    "No, we're outta glazed donuts",
    "The panic revolving",
    "[[Hyperlink Blocked]]",
    "[[Number 1 Rated Kernel1997]]",
    "KERNEL, GONE DOWN THE [[Drain]] [[Drain]]??",
    "YOUR [[KernelShapedObject]].",
    "The Roaring Fraud",
    "GOD DAMMIT KRIS WHERE THE HELL ARE WE?!",
    "Doobie",
    "All right, fine! FINE, I ADMIT IT! Maybe I AM a little GLOOBY sometimes!! I AM!!! I'm GLOOBY!!!",
    "COMINING STRAIGHT FROM YOUR HOUSE",
    "Aurora Borealis? At this time of year? At this time of day? In this part of the country? Localized entirely within your Kernel?",
    "My Kernel is ruined!",
    "Eat my shorts!",
    "Cromulently Ruined",
    "The kernel is a lie",
    "Have you tried ToaruOS?",
    "Program in C pointers, assembly manage your memory with malloc and free!",
};

#define PANIC_FLAGS_VECTOR (1 << 0)
#define PANIC_FLAGS_FRAME (1 << 1)
#define PANIC_FLAGS_ERROR (1 << 2)

int panic_count = 0;

__attribute__((noreturn))
void panic_interrupt_frame(char* message, struct interrupt_frame* frame) {
    unsigned long cr0, cr2, cr3, cr4, cr8;
    struct descriptor_table_ptr gdtr, idtr;
    unsigned short ldt, tr;

    asm volatile(
        "mov %%cr0, %0\n\t"
        "mov %%cr2, %1\n\t"
        "mov %%cr3, %2\n\t"
        "mov %%cr4, %3\n\t"
        "mov %%cr8, %4\n\t"
        : "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4), "=r"(cr8)
    );

    asm volatile(
        "sgdt %0\n\t"
        "sidt %1\n\t"
        : "=m"(gdtr), "=m"(idtr)
    );

    asm volatile(
        "sldt %0\n\t"
        "str %1\n\t"
        : "=m"(ldt), "=m"(tr)
    );

    stop_sound();

    if (panic_count > 0) {
        int scaleX = framebuffer->width / kernel_panic_image_data_width;
        int scaleY = framebuffer->height / kernel_panic_image_data_height;
        if (scaleX < 1) scaleX = 1;
        if (scaleY < 1) scaleY = 1;
        drawImage(kernel_panic_image, kernel_panic_image_data_width, kernel_panic_image_data_height, 0, 0, scaleX, scaleY);

        printString("The kernel is fucked 2: Electric Boogaloo (Panic Paniced)", 0, 0);
        printString(message, 100, 100);

        if(frame != NULL) {
            char result[32];
            snprintf(result, sizeof(result), "%lx", frame->ip);
            printString(result, 300, 300);
        }

        while (1) {
            __asm__ __volatile__("hlt");
        }
    }
    panic_count++;

    printf("\033[38;2;100;93;232m%s\033[0m\n", error_mssages[rand_between(0, SIZEOF(error_mssages) - 1)]);

    printf("\033[38;2;255;48;48m%s\033[0m\n", message);

    printf("\033[38;2;175;56;255mGeneral Registers:\n");
    printf("RAX=0x%016lx RBX=0x%016lx\n", frame->rax, frame->rbx);
    printf("RCX=0x%016lx RDX=0x%016lx\n", frame->rcx, frame->rdx);
    printf("RSI=0x%016lx RDI=0x%016lx\n", frame->rsi, frame->rdi);
    printf("RBP=0x%016lx RSP=0x%016lx\n", frame->rbp, frame->rsp);
    printf("R8 =0x%016lx R9 =0x%016lx\n", frame->r8, frame->r9);
    printf("R10=0x%016lx R11=0x%016lx\n", frame->r10, frame->r11);
    printf("R12=0x%016lx R13=0x%016lx\n", frame->r12, frame->r13);
    printf("R14=0x%016lx R15=0x%016lx\n", frame->r14, frame->r15);

    printf("\033[38;2;231;133;255mInterrupt Frame:\n");
    printf("IP=0x%016lx SP=0x%016lx\n", frame->ip, frame->rsp);
    printf("SS=0x%016lx CS=0x%016lx\n", frame->ss, frame->cs);
    printf("FLAGS: %08b %08b %08b %08b\n",
            (int)frame->flags >> 24 & 0xFF,
            (int)frame->flags >> 16 & 0xFF,
            (int)frame->flags >> 8  & 0xFF,
            (int)frame->flags       & 0xFF);

    printf("ERROR: %08b %08b %08b %08b\n",
           (int)frame->error >> 24 & 0xFF,
           (int)frame->error >> 16 & 0xFF,
           (int)frame->error >> 8  & 0xFF,
           (int)frame->error       & 0xFF);

    printf("\033[38;2;255;238;0mControl Registers:\n");
    printf("CR0: %08b %08b %08b %08b\n",
           (int)cr0 >> 24 & 0xFF,
           (int)cr0 >> 16 & 0xFF,
           (int)cr0 >> 8  & 0xFF,
           (int)cr0       & 0xFF);

    printf("CR4: %08b %08b %08b %08b\n",
           (int)cr4 >> 24 & 0xFF,
           (int)cr4 >> 16 & 0xFF,
           (int)cr4 >> 8  & 0xFF,
           (int)cr4       & 0xFF);

    printf("CR8: %08b %08b %08b %08b\n",
           (int)cr8 >> 24 & 0xFF,
           (int)cr8 >> 16 & 0xFF,
           (int)cr8 >> 8  & 0xFF,
           (int)cr8       & 0xFF);

    printf("CR2=0x%016lx CR3=0x%016lx\n", cr2, cr3);

    printf("\033[38;2;76;230;112mMisc:\n");

    printf("GDTR Base=0x%016lx GDTR Limit=0x%08x\n", gdtr.base, gdtr.limit);
    printf("IDTR Base=0x%016lx IDTR Limit=0x%08x\n", idtr.base, idtr.limit);
    printf("LDT=%08x TR=%08x\n", ldt, tr);


    printf("\033[38;2;26;237;209mStack Trace:\n");

    uint64_t *rbp_ptr;
    asm volatile ("mov %%rbp, %0" : "=r" (rbp_ptr));

    switch (frame->vector) {
        case INTERRUPT_HANDLER_DOUBLE_FAULT:
            if(frame->flags | PANIC_FLAGS_FRAME) {
                for (unsigned int j = 0; j < symbol_count; j++) {
                    if (frame->ip >= symbols[j].address && frame->ip  < symbols[j].address + symbols[j].size) {
                        uint64_t offset = frame->ip - symbols[j].address;
                        printf("%s: 0x%016lx + 0x%08lx\n", symbols[j].name, symbols[j].address, offset);
                        break;
                    }
                }
            }
            break;
        case INTERRUPT_HANDLER_GENERAL_PROTECTION_FAULT:
            if(frame->flags | PANIC_FLAGS_FRAME) {
                for (unsigned int j = 0; j < symbol_count; j++) {
                    if (frame->ip >= symbols[j].address && frame->ip  < symbols[j].address + symbols[j].size) {
                        uint64_t offset = frame->ip - symbols[j].address;
                        printf("%s: 0x%016lx + 0x%08lx\n", symbols[j].name, symbols[j].address, offset);

                        break;
                    }
                }
            }
            break;
        case INTERRUPT_HANDLER_PAGE_FAULT:
            if(frame->flags | PANIC_FLAGS_FRAME) {
                for (unsigned int j = 0; j < symbol_count; j++) {
                    if (frame->ip >= symbols[j].address && frame->ip  < symbols[j].address + symbols[j].size) {
                        uint64_t offset = frame->ip - symbols[j].address;
                        printf("%s: 0x%016lx + 0x%08lx\n", symbols[j].name, symbols[j].address, offset);
                        break;
                    }
                }
            }
            break;
    }

    uint64_t *prev_rbp = NULL;
    unsigned int frames = 0;

    while (rbp_ptr && frames++ < 64) {
        uintptr_t rbp = (uintptr_t)rbp_ptr;

        if ((rbp & 0xF) != 0) break;

        if (rbp < 0x4000) break;

        if (prev_rbp && rbp <= (uintptr_t)prev_rbp) break;

        uint64_t next_rbp = rbp_ptr[0];
        uint64_t rip = rbp_ptr[1];

        bool found = false;
        for (unsigned int j = 0; j < symbol_count; j++) {
            uint64_t start = symbols[j].address;
            uint64_t end = start + symbols[j].size;

            if (rip >= start && rip < end) {
                uint64_t offset = rip - start;
                printf("%s: 0x%016lx + 0x%08lx\n", symbols[j].name, start, offset);
                found = true;
                break;
            }
        }

        if (!found) {
            printf("unknown: 0x%016lx\n", rip);
        }

        prev_rbp = rbp_ptr;
        rbp_ptr = (uint64_t *)next_rbp;
    }

    printf("\x1b[0m");
    printf("                                                                                                    \n");
    printf("                                         @@@@@@@@@@@@@@@@@@@@@                                      \n");
    printf("                                     @@@@                     @@@                                   \n");
    printf("                                   @@                            @@                                 \n");
    printf("                                 @@                                @@                               \n");
    printf("                               @@                                    @@                             \n");
    printf("                             @@                                       @@                            \n");
    printf("                            @@                                          @                           \n");
    printf("                           @@                                            @                          \n");
    printf("                           @                                             @@                         \n");
    printf("                          @                                               @@                        \n");
    printf("                         @@                                               @@                        \n");
    printf("                         @@                                                @                        \n");
    printf("                         @                                                 @@                       \n");
    printf("                        @@                                                  @                       \n");
    printf("                        @@                                                  @                       \n");
    printf("                       @@@                                                  @                       \n");
    printf("                    @@@  @                                                 @@@@                     \n");
    printf("                   @     @@                                                @@  @                    \n");
    printf("                  @      @@                                                @@   @                   \n");
    printf("                 @@       @@                                              @@    @                   \n");
    printf("                 @@        @                                             @@@    @                   \n");
    printf("                 @         @@    @@@@ @                                  @@     @@                  \n");
    printf("                 @          @@   @ @@@ @                               @@       @@                  \n");
    printf("                @@           @@ @@ @@  @ @             @@@@@@         @@        @@                  \n");
    printf("                @@             @@@ @@  @@@@         @@@@  @@ @      @@          @@                  \n");
    printf("                @@               @     @  @@@@   @@@@ @@  @@  @   @@            @@                  \n");
    printf("                 @                @       @ @@  @  @   @      @@@@              @@                  \n");
    printf("                 @               @        @@@@@@@@          @@@                 @@                  \n");
    printf("                 @              @         @       @@@@@       @@                @                   \n");
    printf("                 @@          @@@         @@           @         @              @@                   \n");
    printf("                 @@         @@           @            @@         @@@           @@                   \n");
    printf("                 @@         @           @@             @@          @@         @@                    \n");
    printf("@@@@@@@@@@@@@@@@@@@        @            @@@@@@@@@@@@@@@@@@           @@       @@@@@@@@@@@@@@@@@@@@@@\n");
    printf("                 @@       @            @@@              @@@            @@     @@                    \n");
    printf("                  @@     @            @                   @@            @@   @@@                    \n");
    printf("                  @@@   @            @                     @@            @   @@                     \n");
    printf("                    @               @@                      @@              @@@                     \n");
    printf("                     @             @@                        @@             @@                      \n");
    printf("                      @           @                           @@           @                        \n");
    printf("                       @@@@@@@@@@@                             @@@@@@@@@@@@                         \n");

    printf("god damn it; \033[38;2;255;0;0mhalting;\033[?25l");

    #ifndef MUTE_KERNEL_PANIC
        play_sound(1000);
    #endif

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
