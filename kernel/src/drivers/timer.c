#include "drivers/timer.h"
#include "utils/panic.h"
#include <acpi/acpi_timer.h>
#include <drivers/x86_64/timers/hpet.h>
#include <drivers/x86_64/timers/pit.h>
#include <drivers/x86_64/timers/tsc.h>
#include <stdint.h>
#include <stdio.h>

uint8_t shouldSchedule = 0;
uint8_t currentTimerSource = TIMER_SOURCE_NONE;

void timer_blocking_sleep_ms_stub(uint64_t _) { panic("Timer services used before ready; timer_blocking_sleep_ms()"); }
void (*timer_blocking_sleep_ms)(uint64_t) = timer_blocking_sleep_ms_stub;
uint64_t timer_get_ms_stub() { panic("Timer services used before ready; timer_get_ms()"); }
uint64_t (*timer_get_ms)(void) = timer_get_ms_stub;

void setup_timer() {
    printf("TIMER: Using ACPI Timer to calibrate other timers\n");
    if (!setup_acpi_timer()) {
        printf("TIMER: ACPI Timer timer not found; Falling back to HPET\n");
        if (!setup_hpet()) {
            printf("TIMER: HPET timer not found; Falling back to PIT\n");
            setup_pit();
        }
    }

    setup_tsc();

    printf("TIMER: Timers setup\n");
}
