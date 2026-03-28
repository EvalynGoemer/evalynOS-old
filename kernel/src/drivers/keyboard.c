#include <filesystem/filesystem.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils/spinlock.h>

spinlock_t keyboard_buffer_lock = {0};
volatile uint8_t keyboard_buffer_write = 0;
volatile uint8_t keyboard_buffer_read  = 0;
volatile char keyboard_buffer[256];

void kbdDevicePush(char c) {
    int lock1r = spinlock_lock(&keyboard_buffer_lock);
    uint8_t next = keyboard_buffer_write + 1;
    if (next == keyboard_buffer_read) {
        spinlock_unlock(&keyboard_buffer_lock, lock1r);
        return;
    }

    keyboard_buffer[keyboard_buffer_write] = c;
    keyboard_buffer_write = next;
    spinlock_unlock(&keyboard_buffer_lock, lock1r);
}

int kbdDeviceRead(__attribute__((unused)) char* path, char* return_data, int read_length) {
    int bytes_read = 0;

    int lock1r = spinlock_lock(&keyboard_buffer_lock);
    while (bytes_read < read_length) {
        if (keyboard_buffer_read == keyboard_buffer_write) {
            break;
        }
        return_data[bytes_read] = keyboard_buffer[keyboard_buffer_read];
        keyboard_buffer_read++;
        bytes_read++;
    }

    spinlock_unlock(&keyboard_buffer_lock, lock1r);
    return bytes_read;
}

int kbdDeviceWrite(__attribute__((unused)) char* path, __attribute__((unused)) char* write_data, __attribute__((unused)) int write_length) {
    return -1;
}

void setup_keyboard() {
    struct file* file = malloc(sizeof(struct file));
    strcpy(file->path, "/dev/kbd");
    file->read = kbdDeviceRead;
    file->write = kbdDeviceWrite;
    register_file(file);

    printf("KEYBOARD: Keyboard Glob Device Setup\n");
}
