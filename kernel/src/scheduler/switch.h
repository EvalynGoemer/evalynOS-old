#pragma once

#include <stdint.h>
#include <utils/spinlock.h>

extern void thread_switch(spinlock_t* spinlock, int irqs, uint64_t* old_sp, uint64_t new_sp);
extern void switch_to_user(uint64_t start_addr, uint64_t stack_top);
