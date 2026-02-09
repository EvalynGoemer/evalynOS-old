#pragma once

#include <stdint.h>
#include <utils/spinlock.h>

extern void setup_ps2();
extern spinlock_t ps2Kbd_buffer_lock;
extern volatile uint8_t ps2Kbd_buffer_head;
extern volatile uint8_t ps2Kbd_buffer_tail;
extern volatile char ps2Kbd_buffer[256];
