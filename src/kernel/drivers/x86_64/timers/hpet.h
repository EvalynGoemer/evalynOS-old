#pragma once

#include <stdint.h>
#include <stdbool.h>
#define HPET_CAPABILITES 0x00
#define HPET_CONFIG      0x10
#define HPET_COUNTER     0xF0

extern uint64_t hpet_address;

extern bool setup_hpet();
