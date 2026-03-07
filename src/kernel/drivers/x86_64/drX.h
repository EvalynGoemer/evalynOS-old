#pragma once
#include <stdint.h>
#include <stdbool.h>

static inline void drx_setbp(int reg, uint64_t value) {
    switch (reg) {
        case 0: asm volatile("mov %0, %%dr0" : : "r"(value)); break;
        case 1: asm volatile("mov %0, %%dr1" : : "r"(value)); break;
        case 2: asm volatile("mov %0, %%dr2" : : "r"(value)); break;
        case 3: asm volatile("mov %0, %%dr3" : : "r"(value)); break;
    }
}

static inline void drx_enablebp(int reg) {
    uint64_t dr7;
    asm volatile("mov %%dr7, %0" : "=r"(dr7));
    dr7 |= (1ULL << (reg * 2));
    asm volatile("mov %0, %%dr7" : : "r"(dr7));
}

static inline void drx_disablebp(int reg) {
    uint64_t dr7;
    asm volatile("mov %%dr7, %0" : "=r"(dr7));
    dr7 &= ~(3ULL << (reg * 2));
    asm volatile("mov %0, %%dr7" : : "r"(dr7));
}


static inline bool drx_cnc_bp(void) {
    uint64_t dr6;
    asm volatile("mov %%dr6, %0" : "=r"(dr6));
    bool triggered = dr6 & 0xF;
    asm volatile("mov %%dr6, %0" : : "r"(dr6 & ~0xF));
    return triggered;
}

extern bool drx_alloc_bp(uint64_t addr);
extern bool drx_free_bp(uint64_t addr);
