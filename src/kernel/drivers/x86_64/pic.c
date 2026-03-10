#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/irq.h>
#include <stdio.h>

#define PIC1          0x20
#define PIC2          0xA0
#define PIC1_COMMAND  PIC1
#define PIC1_DATA     (PIC1+1)
#define PIC2_COMMAND  PIC2
#define PIC2_DATA     (PIC2+1)

#define ICW1_ICW4       0x01
#define ICW1_SINGLE     0x02
#define ICW1_INTERVAL4  0x04
#define ICW1_LEVEL      0x08
#define ICW1_INIT       0x10

#define ICW4_8086       0x01
#define ICW4_AUTO       0x02
#define ICW4_BUF_SLAVE  0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM       0x10

#define CASCADE_IRQ 2

void pic_send_eoi(uint8_t irq) {
    if(irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }

    outb(PIC1_COMMAND, 0x20);
}

void pic_unmask_irq(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

void setup_pic(int offset1, int offset2) {
    outbd(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outbd(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outbd(PIC1_DATA, offset1);
    outbd(PIC2_DATA, offset2);
    outbd(PIC1_DATA, 1 << CASCADE_IRQ);
    outbd(PIC2_DATA, 2);
    outbd(PIC1_DATA, ICW4_8086);
    outbd(PIC2_DATA, ICW4_8086);
    outbd(PIC1_DATA, 0xFF);
    outbd(PIC2_DATA, 0xFF);

    unmask_irq = pic_unmask_irq;
    send_eoi = pic_send_eoi;

    printf("PIC: PIC Setup\n");
}
