#include <drivers/x86_64/pic.h>
#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/serial.h>
#include <drivers/dbgstub/dbgstub.h>
#include <drivers/keyboard.h>
#include <utils/cmdline.h>

void serial_isr(struct interrupt_frame* frame) {
    while (serial_data_ready()) {
        char c = serial_read();
        if (dbgstub_enabled) {
            dbgstub_rx(c, frame);
        } else if (c != '\0') {
            serial_buffer[serial_buffer_index] = c;
            serial_buffer_index++;

            kbdDevicePush(c);
        }
    }
}
