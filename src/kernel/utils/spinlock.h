#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <drivers/x86_64/rflags.h>

typedef struct {
    uint32_t flag;
} spinlock_t;

static inline void spinlock_init(spinlock_t* spinlock) {
    __atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELAXED);
}

[[nodiscard]]
static inline bool spinlock_lock(spinlock_t* spinlock) {
    bool irqs = interrupts_enabled();
    asm volatile("cli" ::: "memory");
    while (true) {
        while (__atomic_load_n(&spinlock->flag, __ATOMIC_RELAXED))
            asm volatile("pause");
        if (!__atomic_exchange_n(&spinlock->flag, 1, __ATOMIC_ACQUIRE))
            break;
    }
    return irqs;
}

static inline void spinlock_unlock(spinlock_t* spinlock, bool irqs) {
    __atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
    if (irqs)
        asm volatile("sti" ::: "memory");
    else
        asm volatile("cli" ::: "memory");
}
