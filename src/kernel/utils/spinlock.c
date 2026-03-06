#include <stdbool.h>
#include <drivers/x86_64/rflags.h>
#include <utils/spinlock.h>

void spinlock_unlock_nil(spinlock_t* spinlock, int irqs) {
    __atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
    irql_lower(irqs);
}
