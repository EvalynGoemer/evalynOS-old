#include "memory/vma.h"
#include "memory/vmm.h"
#include "utils/mmio.h"
#include <drivers/x86_64/apic/apic.h>
#include <drivers/x86_64/apic/x2apic.h>
#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/cpuid.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/timers/tsc.h>
#include <scheduler/scheduler.h>
#include <drivers/timer.h>
#include <utils/macros.h>
#include <stdint.h>
#include <stdio.h>

#define APIC_REGISTER_ID     0x020
#define APIC_REGISTER_EOI    0x0B0
#define APIC_REGISTER_SVR    0x0F0
#define APIC_REGISTER_TIMER  0x320
#define APIC_REGISTER_DIVIDE 0x3E0
#define APIC_REGISTER_ICOUNT 0x380
#define APIC_REGISTER_CCOUNT 0x390

volatile uint64_t xapic_timer_ms = 0;
volatile uint64_t apic_address = 0;

uint32_t (*apic_get_id)() = NULL;
void (*apic_timer_isr)() = NULL;

uint64_t xapic_get_ms() {
    return xapic_timer_ms;
}

uint32_t xapic_get_id() {
    return mmio_read_offset_32(apic_address, APIC_REGISTER_ID) & ~0xFFFFFF;
}

void xapic_send_eoi() {
    mmio_write_offset_32(apic_address, APIC_REGISTER_EOI, 0);
}

void xapic_tsc_deadline_isr() {
    xapic_timer_ms++;

    wrmsr(TSC_DEADLINE, read_tsc() + (tsc_frequency / 1000));
    mmio_write_offset_32(apic_address, APIC_REGISTER_EOI, 0);

    if (shouldSchedule /*&& ((xapic_timer_ms % 10) == 0)*/) {
        schedule();
    }
}

void xapic_periodic_isr() {
    xapic_timer_ms++;

    mmio_write_offset_32(apic_address, APIC_REGISTER_EOI, 0);

    if (shouldSchedule /*&& ((xapic_timer_ms % 10) == 0)*/) {
        schedule();
    }
}

uint16_t setup_apic() {
    int x2apic_supported = cpu_feature_bit(CPUID_GET_FEATURES, CPUID_NO_SUBLEAF, CPUID_ECX, CPUID_x2APIC);
    if (x2apic_supported) {
        setup_x2apic();
        return 2;
    }

    apic_get_id = xapic_get_id;
    send_eoi = xapic_send_eoi;

    uint64_t apic_base = rdmsr(APIC_BASE);
    int apic_enabled = (apic_base >> 11) & 1;

    if (!apic_enabled) {
        apic_base |= (1ULL << 11);
        wrmsr(APIC_BASE, apic_base);
    }

    uint64_t apic_phys_address = ALIGN_DOWN(rdmsr(APIC_BASE), PAGE_SIZE);
    apic_address = valloc(PAGE_SIZE * 2);
    vmm_map_page(&kernel_pagemap, apic_address, apic_phys_address,
                 PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
    vmm_map_page(&kernel_pagemap, apic_address + PAGE_SIZE, apic_phys_address,
                 PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);

    uint64_t apic_svr = mmio_read_offset_32(apic_address, APIC_REGISTER_SVR);
    apic_svr |= 0x1FF;
    mmio_write_offset_32(apic_address, APIC_REGISTER_SVR, apic_svr);

    if (cpuid_standard_supported(CPUID_GET_FEATURES) && tsc_good) {
        if(cpu_feature_bit(CPUID_GET_FEATURES, CPUID_NO_SUBLEAF, CPUID_ECX, CPUID_LAPIC_TSC)) {
            printf("xAPIC: Setup using TSC Deadline Mode\n");
            apic_timer_isr = xapic_tsc_deadline_isr;
            timer_get_ms = xapic_get_ms;
            mmio_write_offset_32(apic_address, APIC_REGISTER_TIMER, 0x20 | APIC_MODE_TSC_DEADLINE);
            __sync_synchronize();
            wrmsr(TSC_DEADLINE, read_tsc() + (tsc_frequency / 1000));
            return 0;
        }
    }

    timer_get_ms = xapic_get_ms;
    apic_timer_isr = xapic_periodic_isr;

    printf("xAPIC: Setup using Periodic Mode\n");
    mmio_write_offset_32(apic_address, APIC_REGISTER_DIVIDE, 0x3);
    mmio_write_offset_32(apic_address, APIC_REGISTER_ICOUNT, 0xFFFFFFFF);
    timer_blocking_sleep_ms(10);
    mmio_write_offset_32(apic_address, APIC_REGISTER_TIMER, APIC_MODE_MASKED);
    uint32_t ticksIn10ms = 0xFFFFFFFF - mmio_read_offset_32(apic_address, APIC_REGISTER_CCOUNT);
    mmio_write_offset_32(apic_address, APIC_REGISTER_DIVIDE, 0x3);
    mmio_write_offset_32(apic_address, APIC_REGISTER_TIMER, 0x20 | APIC_MODE_PERIODIC);
    mmio_write_offset_32(apic_address, APIC_REGISTER_ICOUNT, ticksIn10ms / 10);

    return 0;
}
