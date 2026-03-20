#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <drivers/x86_64/ports.h>

/* Serial Port Registers */
#define SERIAL_RX_BUFF     0 // read  ; DLAB = 0
#define SERIAL_TX_BUFF     0 // write ; DLAB = 0
#define SERIAL_INTR_CONF   1 // both  ; DLAB = 0
#define SERIAL_DLAB_DIV_LO 0 // both  ; DLAB = 1
#define SERIAL_DLAB_DIV_HI 1 // both  ; DLAB = 1
#define SERIAL_INTR_INFO   2 // read
#define SERIAL_FIFO_CONF   2 // write
#define SERIAL_LINE_CONF   3 // both
#define SERIAL_MODEM_CONF  4 // both
#define SERIAL_LINE_INFO   5 // read
#define SERIAL_MODEM_INFO  6 // read
#define SERIAL_SCRATCH_REG 7 // both

/* FIFO Config */
#define SERIAL_FIFO_THRESH_1b  0x00 // bit 6 & 7 unset
#define SERIAL_FIFO_THRESH_4b  0x40 // bit 6 set
#define SERIAL_FIFO_THRESH_8b  0x80 // bit 7 set
#define SERIAL_FIFO_THRESH_14b 0xC0 // bit 6 & 7 set
#define SERIAL_FIFO_ENABLE     0x01 // bit 0 set
#define SERIAL_FIFO_RX_FLUSH   0x02 // bit 1 set
#define SERIAL_FIFO_TX_FLUSH   0x04 // bit 2 set

/* Line Control Register */
#define SERIAL_LCR_8BIT        0x03 // bit 0 & 1 set
#define SERIAL_LCR_7BIT        0x01 // bit 0 set
#define SERIAL_LCR_6BIT        0x02 // bit 1 set
#define SERIAL_LCR_5BIT        0x00 // bit 0 & 1 unset
#define SERIAL_LCR_1STOP       0x00 // bit 2 unset
#define SERIAL_LCR_2STOP       0x00 // bit 2 set
#define SERIAL_LCR_PARITY_NONE 0x00 // bit 3 & 4 & 5 unset
#define SERIAL_LCR_PARITY_ODD  0x08 // bit 3 set
#define SERIAL_LCR_PARITY_EVEN 0x18 // bit 3 & 4 set
#define SERIAL_LCR_PARITY_MARK 0x28 // bit 5 & 3 set
#define SERIAL_LCR_PARITY_SPCE 0x38 // bit 3 & 4 & 5 set

/* Modem Control Register*/
#define SERIAL_MCR_TX_ENABLE   0x01 // bit 0 set (DTR)
#define SERIAL_MCR_RX_ENABLE   0x02 // bit 1 set (RTS)
#define SERIAL_MCR_IRQ_ENABLE  0x08 // bit 3 set
#define SERIAL_MCR_LOOP_ENABLE 0x10 // bit 4 set

/* Baud Rate Divisors */
#define SERIAL_115200_BAUD   1
#define SERIAL_57600_BAUD    2
#define SERIAL_38400_BAUD    3
#define SERIAL_19200_BAUD    6
#define SERIAL_9600_BAUD    12
#define SERIAL_4800_BAUD    24
#define SERIAL_2400_BAUD    48
#define SERIAL_1200_BAUD    96
#define SERIAL_600_BAUD    192
#define SERIAL_300_BAUD    384

/* Misc */
#define SERIAL_DLAB_BIT        0x80
#define SERIAL_DATA_READY_BIT  0x01
#define SERIAL_TX_EMPTY_BIT    0x20
#define SERIAL_TEST_MAGIC      0x69
#define SERIAL_TEST_RETRIES    5

extern uint16_t serial_port;
extern bool serial_enabled;
extern bool serial_works;

extern volatile uint8_t serial_buffer_index;
extern volatile char serial_buffer[256];

extern void setup_serial();

[[clang::overloadable]] extern int serial_send_string(char *string, int write_length);
[[clang::overloadable]] extern void serial_send_string(char *string);

[[gnu::always_inline]]
static inline int serial_transmit_empty() {
    return inbd(serial_port + SERIAL_LINE_INFO) & SERIAL_TX_EMPTY_BIT;
}

[[gnu::always_inline]]
static inline int serial_data_ready() {
    return inbd(serial_port + SERIAL_LINE_INFO) & SERIAL_DATA_READY_BIT;
}

[[gnu::always_inline]]
static inline void serial_send(char c) {
    while (!serial_transmit_empty());
    outbd(serial_port + SERIAL_RX_BUFF, c);
}

[[gnu::always_inline]]
static inline char serial_read() {
    return inbd(serial_port + SERIAL_RX_BUFF);
}

[[gnu::always_inline]]
static inline char serial_read_blocking() {
    while (!serial_data_ready());
    return serial_read();
}
