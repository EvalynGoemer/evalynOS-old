#include "drivers/x86_64/irq.h"
#include <drivers/x86_64/pic.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/serial.h>
#include <drivers/keyboard.h>

void serial_isr() {
    while (serial_data_ready()) {
        char c = serial_read();
        if (c != '\0') {
            serial_buffer[serial_buffer_index] = c;
            serial_buffer_index++;

            keyboard_buffer[keyboard_buffer_index] = c;
            keyboard_buffer_index++;
        }
    }

    pic_send_eoi(4);
    send_eoi();
}
