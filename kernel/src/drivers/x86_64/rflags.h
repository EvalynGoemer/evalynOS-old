#pragma once

#include <stdbool.h>
#include <drivers/x86_64/crX.h>

static inline void rflags_set_ac() {
    if (cr4_smap_enabled) {
        __asm__ volatile ("stac" ::: "cc");
    }
}

static inline void rflags_clr_ac(void) {
    if (cr4_smap_enabled) {
        __asm__ volatile ("clac" ::: "cc");
    }
}

extern bool interrupts_enabled();
