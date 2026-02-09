#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <drivers/x86_64/rflags.h>

typedef struct {
    atomic_flag lock;
} spinlock_t;

static inline void spinlock_init(spinlock_t* spinlock) {
    atomic_flag_clear(&spinlock->lock);
}

[[clang::overloadable]] [[nodiscard]] static inline bool spinlock_lock(spinlock_t* spinlock) {
    bool irqs = interrupts_enabled();
    asm volatile("cli");
    while (atomic_flag_test_and_set_explicit(&spinlock->lock, memory_order_acquire))
        asm volatile("pause");
    return irqs;
}

[[clang::overloadable]] static inline void spinlock_unlock(spinlock_t* spinlock, bool irqs) {
    atomic_flag_clear_explicit(&spinlock->lock, memory_order_release);
    if (irqs)
        asm volatile("sti");
    else
        asm volatile("cli");
}
