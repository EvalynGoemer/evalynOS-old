#pragma once

#include <stdint.h>
#include <utils/panic.h>
#include <drivers/x86_64/crX.h>

extern void (*unmask_irq)(uint8_t);
extern void (*send_eoi)(uint8_t);
extern void setup_irqs();

#define set_irql(irql) write_cr8(irql)
#define get_irql() read_cr8()

#define IRQL_PASSIVE  0
#define IRQL_DISPATCH 2

[[nodiscard]]
static inline int irql_raise(int irql) {
    int old_irql = get_irql();
    if (irql < old_irql)
        panic("irql raise to lower irql");
    if (irql == old_irql)
        return old_irql;
    set_irql(irql);
    return old_irql;
}

static inline int irql_lower(int irql) {
    int old_irql = get_irql();
    if (irql > old_irql)
        panic("irql lower to higher irql");
    if (irql == old_irql)
        return old_irql;
    set_irql(irql);
    return old_irql;
}
