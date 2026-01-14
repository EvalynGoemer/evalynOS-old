#pragma once
#include <stdint.h>
#include <stdbool.h>

#define TIMER_SOURCE_NONE 0
#define TIMER_SOURCE_PIT  1
#define TIMER_SOURCE_HPET 2
#define TIMER_SOURCE_TSC  3
#define TIMER_SOURCE_ACPI  4

extern uint8_t shouldSchedule;
extern uint8_t currentTimerSource;
extern void setup_timer();

extern void (*timer_blocking_sleep_ms)(uint64_t);
extern uint64_t (*timer_get_ms)(void);
