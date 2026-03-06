#include "drivers/x86_64/irq.h"
#include "drivers/x86_64/pic.h"
#include "utils/spinlock.h"
#include <stdbool.h>

#include <drivers/x86_64/ports.h>
#include <drivers/x86_64/ps2.h>
#include <drivers/keyboard.h>

#include <drivers/mouse.h>

volatile int ps2InteruptsTriggered = 0;

volatile int shiftPressed = 0;
volatile int extended = 0;

static const unsigned char asciiNoShift[256] = { [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',[0x0C]='-',[0x0D]='=',[0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',[0x1A]='[',[0x1B]=']',[0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',[0x27]=';',[0x28]='\'',[0x29]='`',[0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',[0x32]='m',[0x33]=',',[0x34]='.',[0x35]='/',[0x39]=' ',[0x1C] = '\n', [0x0E] = '\b'};

static const unsigned char asciiShift[256] = { [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',[0x07]='^',[0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',[0x0C]='_',[0x0D]='+',[0x10]='Q',[0x11]='W',[0x12]='E',[0x13]='R',[0x14]='T',[0x15]='Y',[0x16]='U',[0x17]='I',[0x18]='O',[0x19]='P',[0x1A]='{',[0x1B]='}',[0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',[0x24]='J',[0x25]='K',[0x26]='L',[0x27]=':',[0x28]='\"',[0x29]='~',[0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',[0x30]='B',[0x31]='N',[0x32]='M',[0x33]='<',[0x34]='>',[0x35]='?',[0x39]=' ',[0x1C] = '\n', [0x0E] = '\b'};

int mouse_ptr = 0;
unsigned char mouse_buffer[4] = {0,0,0,0};

void ps2_isr() {
    unsigned char status = inb(0x64);

    while(status & 1) {
        unsigned char scancode = inb(0x60);
        if(!(status & (1 << 5))) {
            ps2InteruptsTriggered++;

            switch (scancode) {
                case 0xE0: {
                    extended = true;
                    goto raw;
                }
                case 0x2A: {
                    shiftPressed = 1;
                    goto raw;
                    break;
                }
                case 0xAA: {
                    shiftPressed = 0;
                    goto raw;
                }
                default: {
                    break;
                }
            }

            if (scancode & 0x80) {
                goto raw;
            }

            if (shiftPressed) {
                keyboard_buffer[keyboard_buffer_index] = asciiShift[scancode];
                keyboard_buffer_index++;
            } else {
                keyboard_buffer[keyboard_buffer_index] = asciiNoShift[scancode];
                keyboard_buffer_index++;
            }

            if (extended == true) {
                extended = false;
            }

            raw: {}
            int lock1r = spinlock_lock(&ps2Kbd_buffer_lock);
            uint8_t next_head = ps2Kbd_buffer_head + 1;
            if (next_head != ps2Kbd_buffer_tail) {
                ps2Kbd_buffer[ps2Kbd_buffer_head] = scancode;
                ps2Kbd_buffer_head = next_head;
            }
            spinlock_unlock(&ps2Kbd_buffer_lock, lock1r);
        } else {
            // mouse
            mouse_buffer[mouse_ptr++] = scancode;
            if(mouse_ptr == 3) {
                mouse_ev_t m_ev = {0,0,0,0};
                m_ev.x = mouse_buffer[1] - (mouse_buffer[0] & 0x10 ? 0x100 : 0);
                m_ev.y = mouse_buffer[2] - (mouse_buffer[0] & 0x20 ? 0x100 : 0);
                m_ev.z = (mouse_buffer[3] & 0x7) * (mouse_buffer[3] & 0x8 ? -1 : 1);
                m_ev.buttons |= (mouse_buffer[0] & 1) ? MOUSE_LB : 0;
                m_ev.buttons |= (mouse_buffer[0] & 2) ? MOUSE_RB : 0;
                m_ev.buttons |= (mouse_buffer[0] & 4) ? MOUSE_MB : 0;
                m_ev.buttons |= (mouse_buffer[0] & 0x10) ? MOUSE_B4 : 0;
                m_ev.buttons |= (mouse_buffer[0] & 0x20) ? MOUSE_B5 : 0; 
                m_ev.y = -m_ev.y;
                handle_mouse_event(m_ev);
                mouse_ptr = 0;
            }
        }
        status = inb(0x64);
    }

    send_eoi();
    pic_send_eoi(1);
    pic_send_eoi(12);
}
