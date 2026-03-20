#pragma once

#include <stdbool.h>
#include <stdint.h>

extern uint16_t acpi_timer_address_type;
extern uint64_t acpi_timer_address;

extern bool setup_acpi_timer();
