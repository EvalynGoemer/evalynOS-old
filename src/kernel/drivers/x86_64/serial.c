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
    write_serial(write_data, write_length);
    return write_length;
}

void serial_set_divisor(uint16_t port, uint16_t divsor) {
    uint8_t lcr = inbd(port + 3);          // save lcr state
    outbd(port + 3, lcr | 0x80);           // enable dlab
    outbd(port + 0, divsor & 0xff);        // set lo-byte
    outbd(port + 1, (divsor >> 8) & 0xff); // set hi-byte
    outbd(port + 3, lcr);                  // restore lcr state
}

void serial_set_interrupts(uint16_t port, uint8_t setting) {
    uint8_t lcr = inbd(port + 3);          // save lcr state
    outbd(port + 3, lcr & ~0x80);          // disable dlab
    outbd(port + 1, setting);              // disable interrupts
    outbd(port + 3, lcr);                  // restore lcr state
}

void serial_set_dlab(uint16_t port, bool setting) {
    uint8_t lcr = inbd(port + 3);          // get lcr state
    if (setting)
        outbd(port + 3, lcr | 0x80);       // set dlab
    else
        outbd(port + 3, lcr & ~0x80);      // disable dlab
}

void serial_set_mcr(uint16_t port, uint8_t setting) {
    outbd(port + 4, setting);              // set mcr state
}

void serial_set_mode(uint16_t port, uint8_t settingA, uint8_t settingB) {
    uint8_t dlab = inbd(port + 3) & 0x80;  // get dlab state
    outbd(port + 3, settingA | dlab);      // set settingA
    outbd(port + 2, settingB);             // set settingB
}

bool serial_test(uint16_t port) {
    uint8_t lcr = inbd(port + 3);          // save lcr state
    uint8_t mcr = inbd(port + 4);          // save mcr state
    outbd(port + 3, lcr & ~0x80);          // disable dlab
    outbd(port + 4, 0x1E);                 // enable loopback for testing
    for (int i = 0; i < 5; i++) {
        outbd(port + 0, 0x69);             // send test byte
        if (inbd(port + 0) == 0x69) {
            outbd(port + 4, mcr);          // restore mcr state
            outbd(port + 3, lcr);          // restore lcr state
            return true;
        }
    }
    return false;
}

void setup_serial() {
    if (!serial_enabled) {
        return;
    }

    serial_set_interrupts(serial_port, 0);
    serial_set_divisor(serial_port, 1);       // 115200 baud
    serial_set_mode(serial_port, 0x03, 0x07); // 8N1

    if(!serial_test(serial_port)) {
        printf("SERIAL: Failed to init; Do you lack a serial port at I/O port 0x%x?\n", serial_port);
        serial_works = false;
        return;
    }
    serial_set_mcr(serial_port, 0x0B);
    serial_set_dlab(serial_port, false);
    serial_set_interrupts(serial_port, 1);

    serial_works = true;
    unmask_irq(4);
    printf("SERIAL: Setup serial on I/O port 0x%x\n", serial_port);

    struct file* file = malloc(sizeof(struct file));
    strcpy(file->path, "/dev/term/stty");
    file->read = serialDeviceRead;
    file->write = serialDeviceWrite;
    register_file(file);
}

int serial_received() {
    return inb(serial_port + 5) & 1;
}

char read_serial() {
    while (serial_received() == 0);
    return inb(serial_port);
}

int is_transmit_empty() {
    return inb(serial_port + 5) & 0x20;
}

int write_serial(char *string, int write_length) {
    while (is_transmit_empty() == 0);

    int i = 0;

    while (string[i] != '\0' && (i < write_length)) {
        char c = string[i];

        // qemu serial terminal and maybe others expect CRLF and not LF while kernel uses LF so convert
        if (c == '\n')
            outb(serial_port, '\r');

        outb(serial_port, c);
        i++;
    }

    return i;
}
