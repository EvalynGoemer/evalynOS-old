/*
 * EvalynOS FRED (Flexible Return and Event Delivery) Implementation
 *   \/________________
 * /     _____________)
 * /     /     /   \ |
 * \/\/\/     (O) (O)|
 *  |           ------,
 *  |  _       ______/
 *  | (_      /   \  \
 *  |        /  ___\_ \
 *  |        \      / /
 * __|_________\______/
 * \______________\./__\
 * /     .       | \  |
 * \    /_\   .  |  \ |\
 * |`\       /_\ |   \| \
 *
 * This is built using only hopes and dreams
 * Only tested on intel SIMICS emulator
 */

#include <stdint.h>
#include <stdbool.h>
#include <interupts/interupts.h>
#include <stdio.h>
#include <syscalls/syscalls.h>
#include <drivers/x86_64/fred/fred.h>
#include <drivers/x86_64/cpuid.h>
#include <drivers/x86_64/msr.h>
#include <utils/macros.h>

bool fred_enbled = false;

__attribute__ ((aligned (64))) uint8_t fred_kernel_stack[4096];
__attribute__ ((aligned (64))) uint8_t fred_df_stack[4096];
__attribute__ ((aligned (64))) uint8_t fred_nmi_stack[4096];

bool setup_fred() {
    if (cpuid(CPUID_GET_FEATURES_EXT, CPUID_NO_SUBLEAF).eax <= 1) {
        if (!cpu_feature_bit(CPUID_GET_FEATURES_EXT, CPUID_SUBLEAF_1, CPUID_EAX, CPUID_FRED)) {
            return false;
        }
    } else {
        return false;
    }

    __asm__ volatile (
        "mov %%cr4, %%rax\n"
        "bts $32, %%rax\n"    // set FRED
        "mov %%rax, %%cr4"
        :
        :
        : "rax", "memory"
    );

    uint64_t star = ((uint64_t)(0x18 | 3) << 48) | ((uint64_t)0x08 << 32);
    wrmsr(STAR, star);
    wrmsr(FRED_CONFIG, (uint64_t)fred_ring3_entry_asm_stub);
    wrmsr(FRED_RSP0, (uint64_t)&fred_kernel_stack + sizeof(fred_kernel_stack));
    wrmsr(FRED_RSP2, (uint64_t)&fred_df_stack     + sizeof(fred_df_stack));
    wrmsr(FRED_RSP3, (uint64_t)&fred_nmi_stack    + sizeof(fred_nmi_stack));
    wrmsr(FRED_STKLVLS, 0);

    printf("FRED: FRED Setup\n");

    fred_enbled = true;
    return true;
}

void fred_ring3_entry(fred_frame_t* frame) {
    uint8_t vector = (frame->ss >> 32) & 0xFF;
    uint8_t type   = (frame->ss >> 48) & 0xF;

    if (type == 7) {
        syscall_frame_t sysFrame;
        sysFrame.rax = frame->rax;
        sysFrame.rbx = frame->rbx;
        sysFrame.rcx = frame->rcx;
        sysFrame.rdx = frame->rdx;
        sysFrame.rsi = frame->rsi;
        sysFrame.rdi = frame->rdi;
        sysFrame.rbp = frame->rbp;
        sysFrame.r8  = frame->r8;
        sysFrame.r9  = frame->r9;
        sysFrame.r10 = frame->r10;
        sysFrame.r11 = frame->r11;
        sysFrame.r12 = frame->r12;
        sysFrame.r13 = frame->r13;
        sysFrame.r14 = frame->r14;
        sysFrame.r15 = frame->r15;

        execute_syscall(&sysFrame);

        frame->rax = sysFrame.rax;
        frame->rbx = sysFrame.rbx;
        frame->rcx = sysFrame.rcx;
        frame->rdx = sysFrame.rdx;
        frame->rsi = sysFrame.rsi;
        frame->rdi = sysFrame.rdi;
        frame->rbp = sysFrame.rbp;
        frame->r8  = sysFrame.r8;
        frame->r9  = sysFrame.r9;
        frame->r10 = sysFrame.r10;
        frame->r11 = sysFrame.r11;
        frame->r12 = sysFrame.r12;
        frame->r13 = sysFrame.r13;
        frame->r14 = sysFrame.r14;
        frame->r15 = sysFrame.r15;
    } else {
        interrupt_frame_t iFrame;
        iFrame.vector = vector;
        iFrame.rax = frame->rax;
        iFrame.rbx = frame->rbx;
        iFrame.rcx = frame->rcx;
        iFrame.rdx = frame->rdx;
        iFrame.rsi = frame->rsi;
        iFrame.rdi = frame->rdi;
        iFrame.rbp = frame->rbp;
        iFrame.rsp = frame->rsp;
        iFrame.r8  = frame->r8;
        iFrame.r9  = frame->r9;
        iFrame.r10 = frame->r10;
        iFrame.r11 = frame->r11;
        iFrame.r12 = frame->r12;
        iFrame.r13 = frame->r13;
        iFrame.r14 = frame->r14;
        iFrame.r15 = frame->r15;

        iFrame.ip    = frame->ip;
        iFrame.cs    = frame->cs;
        iFrame.flags = frame->flags;
        iFrame.rsp   = frame->rsp;
        iFrame.ss    = frame->ss;
        iFrame.error = frame->error;

        dispatch_interupt(&iFrame);

        frame->rax = iFrame.rax;
        frame->rbx = iFrame.rbx;
        frame->rcx = iFrame.rcx;
        frame->rdx = iFrame.rdx;
        frame->rsi = iFrame.rsi;
        frame->rdi = iFrame.rdi;
        frame->rbp = iFrame.rbp;
        frame->rsp = iFrame.rsp;
        frame->r8  = iFrame.r8;
        frame->r9  = iFrame.r9;
        frame->r10 = iFrame.r10;
        frame->r11 = iFrame.r11;
        frame->r12 = iFrame.r12;
        frame->r13 = iFrame.r13;
        frame->r14 = iFrame.r14;
        frame->r15 = iFrame.r15;

        frame->ip    = iFrame.ip;
        frame->cs    = iFrame.cs;
        frame->flags = iFrame.flags;
        frame->rsp   = iFrame.rsp;
        frame->ss    = iFrame.ss;
        frame->error = iFrame.error;
    }
}

void fred_ring0_entry(fred_frame_t* frame) {
    uint8_t vector = (frame->ss >> 32) & 0xFF;

    interrupt_frame_t iFrame;
    iFrame.vector = vector;
    iFrame.rax = frame->rax;
    iFrame.rbx = frame->rbx;
    iFrame.rcx = frame->rcx;
    iFrame.rdx = frame->rdx;
    iFrame.rsi = frame->rsi;
    iFrame.rdi = frame->rdi;
    iFrame.rbp = frame->rbp;
    iFrame.rsp = frame->rsp;
    iFrame.r8  = frame->r8;
    iFrame.r9  = frame->r9;
    iFrame.r10 = frame->r10;
    iFrame.r11 = frame->r11;
    iFrame.r12 = frame->r12;
    iFrame.r13 = frame->r13;
    iFrame.r14 = frame->r14;
    iFrame.r15 = frame->r15;

    iFrame.ip    = frame->ip;
    iFrame.cs    = frame->cs;
    iFrame.flags = frame->flags;
    iFrame.rsp   = frame->rsp;
    iFrame.ss    = frame->ss;
    iFrame.error = frame->error;

    dispatch_interupt(&iFrame);

    frame->rax = iFrame.rax;
    frame->rbx = iFrame.rbx;
    frame->rcx = iFrame.rcx;
    frame->rdx = iFrame.rdx;
    frame->rsi = iFrame.rsi;
    frame->rdi = iFrame.rdi;
    frame->rbp = iFrame.rbp;
    frame->rsp = iFrame.rsp;
    frame->r8  = iFrame.r8;
    frame->r9  = iFrame.r9;
    frame->r10 = iFrame.r10;
    frame->r11 = iFrame.r11;
    frame->r12 = iFrame.r12;
    frame->r13 = iFrame.r13;
    frame->r14 = iFrame.r14;
    frame->r15 = iFrame.r15;

    frame->ip    = iFrame.ip;
    frame->cs    = iFrame.cs;
    frame->flags = iFrame.flags;
    frame->rsp   = iFrame.rsp;
    frame->ss    = iFrame.ss;
    frame->error = iFrame.error;
}
