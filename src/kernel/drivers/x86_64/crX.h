#pragma once

#include <drivers/x86_64/cpuid.h>

extern uint8_t cr4_smap_enabled;

extern void setup_control_registers();
