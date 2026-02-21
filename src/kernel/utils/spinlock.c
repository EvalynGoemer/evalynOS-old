#include <stdbool.h>
#include <drivers/x86_64/rflags.h>
#include <utils/spinlock.h>

void spinlock_unlock_nil(spinlock_t* spinlock, bool irqs) {
    __atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
    if (irqs)
        asm volatile("sti" ::: "memory");
    else
        asm volatile("cli" ::: "memory");
}
