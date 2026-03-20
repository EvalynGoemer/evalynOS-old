#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct ioapic_t {
    uint64_t address;
    uint32_t gsib;
    struct ioapic_t* next;
} ioapic_t;

typedef struct ioapic_irq_overide_t {
    uint8_t irq;
    uint8_t gsi;
    struct ioapic_irq_overide_t* next;
} ioapic_irq_overide_t;

extern struct ioapic_t* ioapic_head;
extern struct ioapic_t* ioapic_tail;

extern struct ioapic_irq_overide_t* ioapic_irq_overide_head;
extern struct ioapic_irq_overide_t* ioapic_irq_overide_tail;

extern void ioapic_map_irq(ioapic_t* ioapic, uint8_t irq, uint8_t vector, uint8_t destination);
extern bool setup_ioapic();
