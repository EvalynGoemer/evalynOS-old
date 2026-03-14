#include "utils/spinlock.h"
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <filesystem/filesystem.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/irq.h>
#include <drivers/timer.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64
#define PS2_TIMEOUT_RW   100000
#define PS2_TIMEOUT_MS   50

#define PS2_STATUS_INPUT_BUFFER_FULL 0x02
#define PS2_STATUS_OUTPUT_BUFFER_FULL 0x01

#define PS2_CONTROLLER_CMD_ENABLE_PORT_1  0xAE
#define PS2_CONTROLLER_CMD_ENABLE_PORT_2  0xA8
#define PS2_CONTROLLER_CMD_DISABLE_PORT_1 0xAD
#define PS2_CONTROLLER_CMD_DISABLE_PORT_2 0xA7
#define PS2_CONTROLLER_CMD_READ_CONFIG    0x20
#define PS2_CONTROLLER_CMD_WRITE_CONFIG   0x60
#define PS2_CONTROLLER_CMD_TALK_TO_PORT2  0xD4
#define PS2_CONTROLLER_CONF_BYTE_MASK     0x43

#define PS2_DEVICE_CMD_RESET         0xFF
#define PS2_DEVICE_CMD_RESET_ACK     0xFA
#define PS2_KBD_CMD_SET_SCANCODE_SET 0xF0
#define PS2_KDB_SCANCODE_SET_1       0x43
#define PS2_MOUSE_SET_DEFAULT_CONF   0xF6
#define PS2_MOUSE_ENABLE             0xF4
#define PS2_KBD_IRQ                  0x01
#define PS2_MOUSE_IRQ                0x0C

spinlock_t ps2Kbd_buffer_lock = {0};
volatile uint8_t ps2Kbd_buffer_head = 0;
volatile uint8_t ps2Kbd_buffer_tail = 0;
volatile char ps2Kbd_buffer[256] = {'\0'};

int ps2KbdDeviceRead(__attribute__((unused)) char* path, char* return_data, int read_length) {
    int bytes_read = 0;
    for (int i = 0; i < read_length; i++) {
        if (ps2Kbd_buffer_head == ps2Kbd_buffer_tail)
            break;
        return_data[i] = ps2Kbd_buffer[ps2Kbd_buffer_tail];
        ps2Kbd_buffer_tail++;
        bytes_read++;
    }
    return bytes_read;
}

int ps2KbdDeviceWrite(__attribute__((unused)) char* path, __attribute__((unused)) char* write_data, __attribute__((unused)) int write_length) {
    return -1;
}

static inline bool ps2_wait_write() {
    int t = PS2_TIMEOUT_RW;
    while(--t && (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER_FULL)) { io_wait(); }
    return t > 0;
}

static inline bool ps2_wait_read() {
    int t = PS2_TIMEOUT_RW;
    while(--t && !(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER_FULL)) { io_wait(); }
    return t > 0;
}

static inline void ps2_cmd(uint8_t cmd) {
    ps2_wait_write();
    outbd(PS2_COMMAND_PORT, cmd);
}

static inline void ps2_data(uint8_t data) {
    ps2_wait_write();
    outbd(PS2_DATA_PORT, data);
}

static inline uint8_t ps2_read() {
    ps2_wait_read();
    return inbd(PS2_DATA_PORT);
}

static inline void ps2_flush() {
    for (int i = 16; i != 0; i--)
        inbd(PS2_DATA_PORT);
}

void setup_ps2() {
    ps2_cmd(PS2_CONTROLLER_CMD_DISABLE_PORT_1);
    ps2_cmd(PS2_CONTROLLER_CMD_DISABLE_PORT_2);
    ps2_flush();

    ps2_cmd(PS2_CONTROLLER_CMD_READ_CONFIG);
    uint8_t config = ps2_read();
    config |= PS2_CONTROLLER_CONF_BYTE_MASK;
    ps2_cmd(PS2_CONTROLLER_CMD_WRITE_CONFIG);
    ps2_data(config);

    ps2_cmd(PS2_CONTROLLER_CMD_ENABLE_PORT_1);
    ps2_data(PS2_DEVICE_CMD_RESET);

    for (int i = PS2_TIMEOUT_MS; i != 0; i--) {
        uint8_t response = 0;
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER_FULL) {
            response = inb(PS2_DATA_PORT);
            if (response == PS2_DEVICE_CMD_RESET_ACK) break;
        }
        timer_blocking_sleep_ms(1);
    }

    ps2_data(PS2_KBD_CMD_SET_SCANCODE_SET);
    ps2_data(PS2_KDB_SCANCODE_SET_1);
    ps2_flush();

    ps2_cmd(PS2_CONTROLLER_CMD_ENABLE_PORT_2);
    ps2_cmd(PS2_CONTROLLER_CMD_TALK_TO_PORT2);
    ps2_data(PS2_DEVICE_CMD_RESET);

    for (int i = PS2_TIMEOUT_MS; i != 0; i--) {
        uint8_t response = 0;
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER_FULL) {
            response = inb(PS2_DATA_PORT);
            if (response == PS2_DEVICE_CMD_RESET_ACK) break;
        }
        timer_blocking_sleep_ms(1);
    }

    ps2_cmd(PS2_CONTROLLER_CMD_TALK_TO_PORT2);
    ps2_data(PS2_MOUSE_SET_DEFAULT_CONF);
    ps2_cmd(PS2_CONTROLLER_CMD_TALK_TO_PORT2);
    ps2_data(PS2_MOUSE_ENABLE);
    ps2_flush();

    unmask_irq(PS2_KBD_IRQ);
    unmask_irq(PS2_MOUSE_IRQ);

    struct file* file = malloc(sizeof(struct file));
    strcpy(file->path, "/dev/ps2/kbd");
    file->read = ps2KbdDeviceRead;
    file->write = ps2KbdDeviceWrite;
    register_file(file);

    printf("PS/2: PS/2 Keyboard Setup\n");
}
