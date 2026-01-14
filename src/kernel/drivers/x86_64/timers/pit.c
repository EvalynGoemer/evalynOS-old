#include "drivers/timer.h"
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/ports.h>
#include <scheduler/scheduler.h>
#include <stdint.h>
#include <stdbool.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define PIT_CONTROL_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_FREQUENCY 1193182

static inline void io_wait() {
    outb(0x80, 0);
}

int pitFrequency;

uint16_t read_pit_count() {
    uint16_t count = 0;
    outb(0x43, 0b0000000);
    io_wait();
    count  = inb(0x40);
    io_wait();
    count |= inb(0x40) << 8;
    io_wait();
    return count;
}

void pit_sleep_ms(uint64_t ms) {
    uint32_t wanted_ticks = (ms * PIT_FREQUENCY + 999) / 1000;
    uint32_t divisor = (wanted_ticks > 0xFFFF) ? 0xFFFF : wanted_ticks;

    if (divisor < 100) divisor = 100;

    outb(PIT_CONTROL_PORT, 0x34);
    io_wait();
    outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF);

    uint32_t elapsed_ticks = 0;
    uint16_t current_count = 0;

    uint16_t prev_count = read_pit_count();

    while (elapsed_ticks < wanted_ticks) {
        current_count = read_pit_count();

        if (current_count > prev_count) {
            elapsed_ticks += (prev_count) + (divisor - current_count);
        } else {
            elapsed_ticks += (prev_count - current_count);
        }

        prev_count = current_count;
    }
}

void setup_pit() {
    timer_blocking_sleep_ms = pit_sleep_ms;
    currentTimerSource = TIMER_SOURCE_PIT;
}
