#pragma once

#include <drivers/x86_64/cpuid.h>

extern uint8_t cr4_smap_enabled;

extern void setup_control_registers();

[[gnu::always_inline]]
static inline uint64_t read_cr8() {
    uint64_t value;
    asm volatile ("mov %%cr8, %0" : "=r"(value));
    return value;
}

[[gnu::always_inline]]
static inline void write_cr8(uint64_t value) {
    asm volatile ("mov %0, %%cr8" :: "r"(value));
}
