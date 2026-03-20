#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint64_t tsc_frequency;
extern bool tsc_good;

extern bool setup_tsc();

static inline uint64_t read_tsc() {
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
