#include "drivers/x86_64/apic/apic.h"
#include "stdbool.h"
#include "stddef.h"
#include <drivers/x86_64/apic/ioapic.h>
#include <stdint.h>
#include <stdio.h>
#include <utils/mmio.h>
#include <drivers/x86_64/irq.h>

struct ioapic_t* ioapic_head = NULL;
struct ioapic_t* ioapic_tail = NULL;

struct ioapic_irq_overide_t* ioapic_irq_overide_head = NULL;
struct ioapic_irq_overide_t* ioapic_irq_overide_tail = NULL;

#define IOAPIC_DEST_SHIFT 56

#define IOAPIC_REG_VER         0x01
#define IOAPIC_REG_REDTBL_BASE 0x10

uint32_t ioapic_read(ioapic_t* ioapic, uint32_t reg) {
    mmio_write_32(ioapic->address, reg);
    return mmio_read_32(ioapic->address + 16);
}

void ioapic_write(ioapic_t* ioapic, uint32_t reg, uint32_t value) {
    mmio_write_32(ioapic->address, reg);
    mmio_write_32(ioapic->address + 16, value);
}

uint8_t ioapic_max_entries(ioapic_t* ioapic) {
    uint32_t version = ioapic_read(ioapic, IOAPIC_REG_VER);
    return (uint8_t)((version >> 16) & 0xFF);
}

void ioapic_set_entry(ioapic_t* ioapic, uint8_t index, uint64_t data) {
    ioapic_write(ioapic, IOAPIC_REG_REDTBL_BASE + index * 2, (uint32_t) data);
    ioapic_write(ioapic, IOAPIC_REG_REDTBL_BASE + index * 2 + 1, (uint32_t) (data >> 32));
}

void ioapic_map_irq(ioapic_t* ioapic, uint8_t irq, uint8_t vector, uint8_t destination) {
    uint64_t redirect = vector;
    redirect |= ((uint64_t) destination << IOAPIC_DEST_SHIFT);
    ioapic_set_entry(ioapic, irq, redirect);
}

void ioapic_unmask_irq(uint8_t irq) {
    uint8_t actual_irq = irq;
    struct ioapic_irq_overide_t* current_overide = ioapic_irq_overide_head;

    while (current_overide != NULL) {
        if (current_overide->irq == irq) {
            actual_irq = current_overide->gsi;
            break;
        }
        current_overide = current_overide->next;
    }

    struct ioapic_t* current_ioapic = ioapic_head;
    while (current_ioapic != NULL) {
        uint8_t max_entries = ioapic_max_entries(current_ioapic);
        if (actual_irq >= current_ioapic->gsib &&
            actual_irq < (current_ioapic->gsib + max_entries)) {
            break;
        }
        current_ioapic = current_ioapic->next;
    }

    if (current_ioapic == NULL) {
        printf("IOAPIC: Failed to find IOAPIC to map IRQ %d\n", irq);
        return;
    }


    ioapic_map_irq(current_ioapic, actual_irq, irq + 0x20, apic_get_id());
}

bool setup_ioapic() {
    if (ioapic_head == NULL) {
        printf("IOAPIC: Your computer has no IOAPIC???????\n");
        return false;
    }

    unmask_irq = ioapic_unmask_irq;

    printf("IOAPIC: IOAPIC Setup\n");

    return true;
}
