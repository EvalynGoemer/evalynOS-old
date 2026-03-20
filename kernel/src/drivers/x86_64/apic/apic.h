#pragma once

#include <stdint.h>

#define APIC_MODE_TSC_DEADLINE  0x40000
#define APIC_MODE_PERIODIC      0x20000
#define APIC_MODE_MASKED        0x10000

extern uint16_t setup_apic();
extern uint32_t (*apic_get_id)();
extern void (*apic_timer_isr)();
