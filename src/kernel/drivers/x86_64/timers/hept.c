#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <utils/bits.h>
#include <utils/mmio.h>
#include <drivers/timer.h>
#include <drivers/x86_64/timers/hpet.h>

uint64_t hpet_address = 0;
uint64_t hpet_frequency = 0;

void hpet_blocking_sleep_ms(uint64_t ms) {
    if (hpet_address == 0 || hpet_frequency == 0) {
        return;
    }

    uint64_t start = mmio_read_offset_64(hpet_address, HPET_COUNTER);
    uint64_t ticks_needed = (hpet_frequency * ms) / 1000;

    while ((mmio_read_offset_64(hpet_address, HPET_COUNTER) - start) < ticks_needed);
}

bool setup_hpet() {
    if (hpet_address == 0) {
        return false;
    }

    uint64_t hpet_caps = mmio_read_offset_64(hpet_address, HPET_CAPABILITES);
    uint64_t tick_period = hpet_caps >> 32;
    hpet_frequency = 1000000000000000 / tick_period;

    printf("HPET: Detected Frequency: %ldMHz\n", (hpet_frequency / (uint64_t)1e6));

    mmio_write_offset_64(hpet_address, HPET_CONFIG, 0b01);

    currentTimerSource = TIMER_SOURCE_HPET;
    timer_blocking_sleep_ms = hpet_blocking_sleep_ms;

    return true;
}
