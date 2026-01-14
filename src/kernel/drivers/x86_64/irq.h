#pragma once

#include <stdint.h>

extern void (*unmask_irq)(uint8_t);
extern void (*send_eoi)();
extern void setup_irqs();
