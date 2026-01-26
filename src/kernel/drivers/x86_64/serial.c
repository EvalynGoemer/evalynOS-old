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

void setup_serial() {
    if (!serial_enabled) {
        return;
    }

    outb(serial_port + 3, 0x00); // disable DLAB to set interrupt register
    outb(serial_port + 1, 0x01); // enable interrupts
    outb(serial_port + 3, 0x80); // enable DLAB to change baud rate
    outb(serial_port + 0, 0x01); // set divsor to 1 for 115200 baud
    outb(serial_port + 1, 0x00); // high byte of previous
    outb(serial_port + 3, 0x03); // set 8N1 mode
    outb(serial_port + 2, 0x07); // set FIFO with 1 byte threshold
    outb(serial_port + 4, 0x0B); // enable the irqs

    outb(serial_port + 4, 0x1E); // enable loopback for testing
    outb(serial_port + 0, 0x69); // send test byte
    if(inb(serial_port + 0) != 0x69) {
        printf("SERIAL: Failed to init; Do you lack a serial port at I/O port 0x%x?\n", serial_port);
        serial_works = false;
        return;
    }
    outb(serial_port + 4, 0x0F); // disable loopback

    printf("SERIAL: Setup serial on I/O port 0x%x\n", serial_port);

    serial_works = true;

    unmask_irq(4);

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
        if (c == '\n') {
            outb(serial_port, '\r');
        }

        outb(serial_port, c);
        i++;
    }

    return i;
}
