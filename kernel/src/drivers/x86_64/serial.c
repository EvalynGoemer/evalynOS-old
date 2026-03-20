#include "drivers/timer.h"
#include <drivers/x86_64/serial.h>
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/ports.h>
#include <filesystem/filesystem.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

uint16_t serial_port = 0x3F8;
bool serial_enabled = false;
bool serial_works = false;

volatile uint8_t serial_buffer_index;
volatile char serial_buffer[256] = {'\0'};

int serialDeviceRead(__attribute__((unused)) char* path, char* return_data, int read_length) {
    for (int i = 0; i < read_length; i++) {
        return_data[i] = serial_buffer[serial_buffer_index];
        serial_buffer[serial_buffer_index] = '\0';
        serial_buffer_index--;
    }
    return 1;
}

int serialDeviceWrite(__attribute__((unused)) char* path, __attribute__((unused)) char* write_data, __attribute__((unused)) int write_length) {
    serial_send_string(write_data, write_length);
    return write_length;
}

static inline void serial_set_dlab(uint16_t port, bool setting) {
    uint8_t lcr = inbd(port + SERIAL_LINE_CONF);
    if (setting)
        outbd(port + SERIAL_LINE_CONF, lcr | SERIAL_DLAB_BIT);
    else
        outbd(port + SERIAL_LINE_CONF, lcr & ~SERIAL_DLAB_BIT);
}

static inline void serial_set_divisor(uint16_t port, uint16_t divsor) {
    serial_set_dlab(port, true);
    outbd(port + SERIAL_DLAB_DIV_LO, divsor & 0xff);
    outbd(port + SERIAL_DLAB_DIV_HI, (divsor >> 8) & 0xff);
    serial_set_dlab(port, false);
}

static inline void serial_set_interrupts(uint16_t port, uint8_t setting) {
    serial_set_dlab(port, false);
    outbd(port + SERIAL_INTR_CONF, setting);
}

static inline void serial_set_mcr(uint16_t port, uint8_t setting) {
    outbd(port + SERIAL_MODEM_CONF, setting);
}

static inline void serial_set_lcr(uint16_t port, uint8_t lcr) {
    outbd(port + SERIAL_LINE_CONF, lcr);
}

static inline void serial_set_fifo(uint16_t port, uint8_t fifo) {
    outbd(port + SERIAL_FIFO_CONF, fifo);
}

/// @warning: clobbers serial port config
bool serial_test(uint16_t port) {
    serial_set_divisor(port, SERIAL_115200_BAUD);
    serial_set_lcr(port, SERIAL_LCR_8BIT | SERIAL_LCR_1STOP | SERIAL_LCR_PARITY_NONE);
    serial_set_fifo(port, SERIAL_FIFO_TX_FLUSH | SERIAL_FIFO_RX_FLUSH);
    serial_set_mcr(port, SERIAL_MCR_TX_ENABLE | SERIAL_MCR_RX_ENABLE | SERIAL_MCR_LOOP_ENABLE);
    serial_set_dlab(port, false);
    for (int i = 0; i < SERIAL_TEST_RETRIES; i++) {
        outbd(port + SERIAL_TX_BUFF, SERIAL_TEST_MAGIC);
        timer_blocking_sleep_ms(1);
        if (inbd(port + SERIAL_RX_BUFF) == SERIAL_TEST_MAGIC)
            return true;
    }
    return false;
}

void setup_serial() {
    if (!serial_enabled) {
        return;
    }

    serial_set_interrupts(serial_port, false);
    if(!serial_test(serial_port)) {
        outbd(serial_port + SERIAL_SCRATCH_REG, SERIAL_TEST_MAGIC);
        if (inbd(serial_port + SERIAL_SCRATCH_REG) == SERIAL_TEST_MAGIC) {
            printf("\x1b[93mSERIAL: Serial on I/O port 0x%x exists but failed self test; Continuing anyways\x1b[0m\n", serial_port);
        }
        printf("SERIAL: Failed to init; Do you lack a serial port at I/O port 0x%x?\n", serial_port);
        serial_works = false;
        return;
    }

    serial_set_divisor(serial_port, SERIAL_115200_BAUD);
    serial_set_lcr(serial_port, SERIAL_LCR_8BIT | SERIAL_LCR_1STOP | SERIAL_LCR_PARITY_NONE);
    serial_set_fifo(serial_port, SERIAL_FIFO_ENABLE | SERIAL_FIFO_THRESH_1b | SERIAL_FIFO_TX_FLUSH | SERIAL_FIFO_RX_FLUSH);
    serial_set_mcr(serial_port, SERIAL_MCR_TX_ENABLE | SERIAL_MCR_RX_ENABLE | SERIAL_MCR_IRQ_ENABLE);
    serial_set_dlab(serial_port, false);
    serial_set_interrupts(serial_port, true);

    serial_works = true;
    unmask_irq(4);
    printf("SERIAL: Setup serial on I/O port 0x%x\n", serial_port);

    struct file* file = malloc(sizeof(struct file));
    strcpy(file->path, "/dev/term/stty");
    file->read = serialDeviceRead;
    file->write = serialDeviceWrite;
    register_file(file);
}

[[clang::overloadable]]
int serial_send_string(char *string, int write_length) {
    while (!serial_transmit_empty());

    int i = 0;
    while (string[i] != '\0' && (i < write_length)) {
        char c = string[i];
        // qemu serial terminal and maybe others expect CRLF and not LF while kernel uses LF so convert
        if (c == '\n')
            serial_send('\r');
        serial_send(c);
        i++;
    }

    return i;
}

[[clang::overloadable]]
void serial_send_string(char *string) {
    serial_send_string(string, strlen(string));
}
