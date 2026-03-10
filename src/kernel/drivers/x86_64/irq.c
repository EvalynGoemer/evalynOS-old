#include "drivers/x86_64/pic.h"
#include "drivers/x86_64/apic/ioapic.h"
#include "utils/panic.h"
#include <stdio.h>

void unmask_irq_stub(uint8_t _) { panic("IRQ tried to be unmasked before IRQ drivers setup"); }
void (*unmask_irq)(uint8_t) = unmask_irq_stub;
void send_eoi_stub(uint8_t _) { panic("EOI attempted to be sent before IRQ drivers setup"); }
void (*send_eoi)(uint8_t) = send_eoi_stub;

void setup_irqs() {
    asm volatile ("cli");
    setup_pic(0x20, 0x28);
    if (!setup_ioapic()) {
        printf("IRQ: IOAPIC Failed to setup; Falling back to legacy PIC\n");
    }

    asm volatile ("sti");
    printf("IRQ: IRQ System Setup\n");
}
