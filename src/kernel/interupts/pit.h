#pragma once

#include <stdint.h>

extern volatile uint64_t pitInteruptsTriggered;
extern volatile int shouldSchedule;

extern void pit_isr();
