#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint16_t serial_port;
extern bool serial_enabled;
extern bool serial_works;

extern volatile uint8_t serial_buffer_index;
extern volatile char serial_buffer[256];

extern void setup_serial();

extern int write_serial(char *string, int write_length);
extern char read_serial();
