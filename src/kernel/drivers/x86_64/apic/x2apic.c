#include <drivers/x86_64/apic/apic.h>
#include <drivers/x86_64/apic/x2apic.h>
#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/cpuid.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/timers/tsc.h>
#include <scheduler/scheduler.h>
#include <drivers/timer.h>
#include <stdint.h>
#include <stdio.h>

volatile uint64_t x2apic_timer_ms = 0;

void x2apic_send_eoi() {
    wrmsr(x2APIC_EOI, 0);
}

uint32_t x2apic_get_id() {
    return rdmsr(x2APIC_ID);
}

uint64_t x2apic_get_ms() {
    return x2apic_timer_ms;
}

void x2apic_tsc_deadline_isr() {
    x2apic_timer_ms++;

    wrmsr(TSC_DEADLINE, read_tsc() + (tsc_frequency / 1000));
    wrmsr(x2APIC_EOI, 0);

    if (shouldSchedule /*&& ((x2apic_timer_ms % 10) == 0)*/) {
        schedule();
    }
}

void x2apic_periodic_isr() {
    x2apic_timer_ms++;

    wrmsr(x2APIC_EOI, 0);

    if (shouldSchedule /*&& ((x2apic_timer_ms % 10) == 0)*/) {
        schedule();
    }
}

void setup_x2apic() {
    /* Replace APIC Functions for x2APIC */
    send_eoi = x2apic_send_eoi;
    timer_get_ms = x2apic_get_ms;
    apic_get_id = x2apic_get_id;

    /* x2APIC Setup */
    uint64_t apic_base = rdmsr(APIC_BASE);
    int apic_enabled = (apic_base >> 11) & 1;
    int x2apic_enabled = (apic_base >> 10) & 1;

    if (!apic_enabled) {
        apic_base |= (1ULL << 11);
        wrmsr(APIC_BASE, apic_base);
    }

    if (!x2apic_enabled) {
        apic_base |= (1ULL << 10);
        wrmsr(APIC_BASE, apic_base);
    }

    uint64_t apic_svr = rdmsr(x2APIC_SVR);
    apic_svr |= 0x1FF; // enable lapic & enable spurious vector on vector 0xFF
    wrmsr(x2APIC_SVR, apic_svr);

    /* x2APIC Timer Setup */

    if (cpuid_standard_supported(CPUID_GET_FEATURES) && tsc_good) {
        if(cpu_feature_bit(CPUID_GET_FEATURES, CPUID_NO_SUBLEAF, CPUID_ECX, CPUID_LAPIC_TSC)) {
            printf("x2APIC: Setup using TSC Deadline Mode\n");
            apic_timer_isr = x2apic_tsc_deadline_isr;
            timer_get_ms = x2apic_get_ms;
            wrmsr(x2APIC_TIMER, 0x30 | APIC_MODE_TSC_DEADLINE);
            __sync_synchronize();
            wrmsr(TSC_DEADLINE, read_tsc() + (tsc_frequency / 1000));
            return;
        }
    }

    printf("x2APIC: Setup using Periodic Mode\n");
    wrmsr(x2APIC_TIMER_DIVIDE, 0x3);
    wrmsr(x2APIC_TIMER_ICOUNT, 0xFFFFFFFF);
    timer_blocking_sleep_ms(10);
    wrmsr(x2APIC_TIMER, APIC_MODE_MASKED);
    uint32_t ticksIn10ms = 0xFFFFFFFF - rdmsr(x2APIC_TIMER_CCOUNT);
    wrmsr(x2APIC_TIMER_DIVIDE, 0x3);
    wrmsr(x2APIC_TIMER, 0x30 | APIC_MODE_PERIODIC);
    wrmsr(x2APIC_TIMER_ICOUNT, ticksIn10ms / 10);

    timer_get_ms = x2apic_get_ms;
    apic_timer_isr = x2apic_periodic_isr;
}

