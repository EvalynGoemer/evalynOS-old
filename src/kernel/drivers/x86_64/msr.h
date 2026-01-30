#pragma once
#include <stdint.h>

enum MSRs : uint32_t {
    // LAPIC
    APIC_BASE = 0x0000001B,

    // FRED (Flexible Return and Event Delivery)
    FRED_CONFIG  = 0x000001D4,
    FRED_RSP0    = 0x000001CC,
    FRED_RSP1    = 0x000001CD,
    FRED_RSP2    = 0x000001CE,
    FRED_RSP3    = 0x000001CF,
    FRED_STKLVLS = 0x000001D0,

    // x2APIC
    x2APIC_ID    = 0x00000802,
    x2APIC_EOI   = 0x0000080B,
    x2APIC_SVR   = 0x0000080F,

    // x2APIC Timer
    x2APIC_TIMER        = 0x00000832,
    x2APIC_TIMER_LINT0  = 0x00000835,
    x2APIC_TIMER_LINT1  = 0x00000836,
    x2APIC_TIMER_ICOUNT = 0x00000838,
    x2APIC_TIMER_CCOUNT = 0x00000839,
    x2APIC_TIMER_DIVIDE = 0x0000083E,

    // TSC
    TSC          = 0x00000010,
    TSC_DEADLINE = 0x000006E0,

    // Extended Feature Enable Register
    EFER   = 0xC0000080,

    // SYSCALL Related MSRs
    STAR   = 0xC0000081,
    LSTAR  = 0xC0000082,
    CSTAR  = 0xC0000083,
    SFMASK = 0xC0000084,

    // Segment Register
    FSBAS =  0xC0000100,  // FS Base
    UGSBAS = 0xC0000101, // User GS Base
    KGSBAS = 0xC0000102, // Kernel GS Base
};

extern uint64_t rdmsr(uint32_t msr) ;
extern void wrmsr(uint32_t msr, uint64_t value);
