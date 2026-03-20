/*
 * dbgstub (x86-64) - a gdb remote protocol server
 * Made for evalynOS --- Mostly Portable
 *
 * Warning; This only works with lldb for this kernel
 * With LLDB use `process connect --plugin gdb-remote serial:///dev/ttyUSB0?baud=115200&parity=no&parity-check=no`
 * to be able to properly connect over serial. Change device if needed.
 *
 * References Used
 * - https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html
 * - https://github.com/OBOS-dev/obos/tree/master/src/oboskrnl/arch/x86_64/gdbstub
 * - https://archive.ph/EIqip (Crappy medium article)
 * - https://lldb.llvm.org/use/map.html
 * - https://web.archive.org/web/20211101120042/https://www.moritz.systems/blog/lldb-serial-port-communication-support/
 *
 * TODO List:
 * - Refactor to make supporting more packets easier
 * - Support more gdb remote protocol packets
 * - Support software breakpoints
 * - Debugging of other CPU cores
 * - Debugging of kernel threads
 * - Add more generic arch abstractions
 */

#include "dbgstub.h"
#include "drivers/x86_64/drX.h"
#include <utils/panic.h>
#include <drivers/x86_64/serial.h>
#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/rflags.h>
#include <drivers/x86_64/irq.h>
#include <utils/safe_user_funcs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <interupts/interupts.h>

#define DBGSTUB_PACKET_BUFFER_SIZE 4096

char* dbgstub_send_buffer;
char* dbgstub_packet_send_buffer;
char* dbgstub_packet_recv_buffer;
uint32_t dbgstub_packet_buffer_index = 0;
struct interrupt_frame* last_frame;

bool dbgstub_middle_of_packet = false;
bool dbgstub_end_of_packet = false;
uint8_t dbgstub_end_of_packet_count = 0;
volatile _Atomic bool dbgstub_paused = true;
volatile _Atomic bool dbgstub_continue = false;

void dbgstub_init() {
    dbgstub_packet_recv_buffer = zalloc(DBGSTUB_PACKET_BUFFER_SIZE + 16);
    dbgstub_packet_send_buffer = zalloc(DBGSTUB_PACKET_BUFFER_SIZE + 16);
    dbgstub_send_buffer        = zalloc(DBGSTUB_PACKET_BUFFER_SIZE + 16);
    last_frame                 = zalloc(sizeof(struct interrupt_frame) );
}

void dbgstub_send(const char *str) {
    uint8_t checksum = 0;
    int i = 0;
    dbgstub_packet_send_buffer[i++] = '$';
    serial_send('$');
    while (*str) {
        uint8_t b = *str++;
        checksum += b;
        dbgstub_packet_send_buffer[i++] = b;
        serial_send(b);
    }
    serial_send('#');
    dbgstub_packet_send_buffer[i++] = '#';

    const char hex[] = "0123456789abcdef";
    dbgstub_packet_send_buffer[i++] = hex[(checksum >> 4) & 0xF];
    serial_send(dbgstub_packet_send_buffer[i - 1]);
    dbgstub_packet_send_buffer[i++] = hex[checksum & 0xF];
    serial_send(dbgstub_packet_send_buffer[i - 1]);
    dbgstub_packet_send_buffer[i++] = '\0';
}

void dbgstub_rx(char c, struct interrupt_frame* frame) {
    if (dbgstub_packet_buffer_index == DBGSTUB_PACKET_BUFFER_SIZE)
        panic("DBGSTUB: Packet buffer overflow");
    switch (c) {
        case '\003':
            if (!dbgstub_middle_of_packet) {
                frame->flags |= 0x100;
                dbgstub_paused = true;
                dbgstub_continue = false;
            }
            return;
        case '+':
            if (!dbgstub_middle_of_packet)
                return;
            break;
        case '-':
            if (!dbgstub_middle_of_packet) {
                dbgstub_send(dbgstub_packet_send_buffer);
                return;
            }
            break;
        case '}':
            panic("DBGSTUB: unhandled escaped charater");
            // TODO: handle escaped charaters
            break;
        case '$':
            dbgstub_end_of_packet = false;
            dbgstub_middle_of_packet = true;
            dbgstub_packet_buffer_index = 0;
            break;
        case '%':
            if (dbgstub_middle_of_packet)
                break;
            dbgstub_end_of_packet = false;
            dbgstub_middle_of_packet = true;
            dbgstub_packet_buffer_index = 0;
            break;
        case '#':
            dbgstub_end_of_packet = true;
            dbgstub_middle_of_packet = false;
            dbgstub_end_of_packet_count = 0;
            break;
    }

    dbgstub_packet_recv_buffer[dbgstub_packet_buffer_index] = c;
    dbgstub_packet_buffer_index++;

    if (dbgstub_end_of_packet) {
        dbgstub_end_of_packet_count++;
    }

    if (dbgstub_end_of_packet_count == 3) {
        dbgstub_packet_recv_buffer[dbgstub_packet_buffer_index + 1] = '\0';
        dbgstub_end_of_packet_count = 0;
        dbgstub_packet_buffer_index = 0;
        dbgstub_middle_of_packet = false;
        dbgstub_end_of_packet = false;
        dbgstub_process_packet(frame);
        return;
    }
}

bool dbgstub_validate_packet() {
    uint8_t calculated_checksum = 0;
    if (dbgstub_packet_recv_buffer[0] != '$' && dbgstub_packet_recv_buffer[0] != '%') {
        serial_send('-');
        return false;
    }

    int i = 1;
    for (;dbgstub_packet_recv_buffer[i] != '#'; i++) {
        calculated_checksum += dbgstub_packet_recv_buffer[i];
    }

    char packet_checksum[3];
    packet_checksum[0] = dbgstub_packet_recv_buffer[i+1];
    packet_checksum[1] = dbgstub_packet_recv_buffer[i+2];
    packet_checksum[2] = '\0';
    uint64_t packet_checksum_n = strtoll(packet_checksum, 0, 16);

    if (calculated_checksum == packet_checksum_n) {
        serial_send('+');
        return true;
    }

    serial_send('-');
    return false;
}

void dbgstub_process_packet([[maybe_unused]] struct interrupt_frame* frame) {
    bool status = dbgstub_validate_packet();
    if (status == false)
        return;

    if (strncmp("$qSupported", dbgstub_packet_recv_buffer, 11) == 0) {
        dbgstub_send("PacketSize=4000;hwbreak+;swbreak-;multiprocess-;multi-wp-addr-");
        return;
    }

    if (strncmp("$qAttached", dbgstub_packet_recv_buffer, 10) == 0) {
        dbgstub_send("1");
        return;
    }

    if (strncmp("$?", dbgstub_packet_recv_buffer, 2) == 0) {
        dbgstub_send("S05");
        return;
    }

    if (strncmp("$g", dbgstub_packet_recv_buffer, 2) == 0) {
        char *ptr = dbgstub_send_buffer;
        #define PACK_REG(reg_val, width) do {                 \
        uint64_t v = (uint64_t)(reg_val);                     \
        if ((width) == 16)                                    \
            v = __builtin_bswap64(v);                         \
        else                                                  \
            v = (uint64_t)__builtin_bswap32((uint32_t)v);     \
        for (int i = (width) - 1; i >= 0; i--) {              \
            ptr[i] = "0123456789abcdef"[v & 0xf];             \
            v >>= 4;                                          \
        }                                                     \
        ptr += (width);                                       \
        } while(0)

        PACK_REG(last_frame->rax, 16); PACK_REG(last_frame->rbx, 16);
        PACK_REG(last_frame->rcx, 16); PACK_REG(last_frame->rdx, 16);
        PACK_REG(last_frame->rsi, 16); PACK_REG(last_frame->rdi, 16);
        PACK_REG(last_frame->rbp, 16); PACK_REG(last_frame->rsp, 16);
        PACK_REG(last_frame->r8,  16); PACK_REG(last_frame->r9,  16);
        PACK_REG(last_frame->r10, 16); PACK_REG(last_frame->r11, 16);
        PACK_REG(last_frame->r12, 16); PACK_REG(last_frame->r13, 16);
        PACK_REG(last_frame->r14, 16); PACK_REG(last_frame->r15, 16);
        PACK_REG(last_frame->ip,  16); PACK_REG(last_frame->flags, 8);
        PACK_REG(last_frame->cs,   8); PACK_REG(last_frame->ss,    8);
        #undef PACK_REG
        *ptr = '\0';

        dbgstub_send(dbgstub_send_buffer);
        return;
    }

    if (strncmp("$G", dbgstub_packet_recv_buffer, 2) == 0) {
        char *ptr = dbgstub_packet_recv_buffer + 2;
        char hex_tmp[17];

        #define UNPACK_REG(reg_store, width) do {                         \
        memcpy(hex_tmp, ptr, width);                                      \
        hex_tmp[width] = '\0';                                            \
        uint64_t val = strtoull(hex_tmp, NULL, 16);                       \
        if (width == 16)                                                  \
            (reg_store) = __builtin_bswap64(val);                         \
        else                                                              \
            (reg_store) = (uint64_t)__builtin_bswap32((uint32_t)val);     \
        ptr += width;                                                     \
        } while(0)

        UNPACK_REG(last_frame->rax, 16); UNPACK_REG(last_frame->rbx, 16);
        UNPACK_REG(last_frame->rcx, 16); UNPACK_REG(last_frame->rdx, 16);
        UNPACK_REG(last_frame->rsi, 16); UNPACK_REG(last_frame->rdi, 16);
        UNPACK_REG(last_frame->rbp, 16); UNPACK_REG(last_frame->rsp, 16);
        UNPACK_REG(last_frame->r8,  16); UNPACK_REG(last_frame->r9,  16);
        UNPACK_REG(last_frame->r10, 16); UNPACK_REG(last_frame->r11, 16);
        UNPACK_REG(last_frame->r12, 16); UNPACK_REG(last_frame->r13, 16);
        UNPACK_REG(last_frame->r14, 16); UNPACK_REG(last_frame->r15, 16);
        UNPACK_REG(last_frame->ip, 16);  UNPACK_REG(last_frame->flags, 8);
        UNPACK_REG(last_frame->cs, 8);   UNPACK_REG(last_frame->ss, 8);
        #undef UNPACK_REG

        dbgstub_send("OK");
        return;
    }

    if (strncmp("$m", dbgstub_packet_recv_buffer, 2) == 0) {
        char *args = dbgstub_packet_recv_buffer + 2;
        char *comma = strchr(args, ',');

        if (!comma) {
            dbgstub_send("E00");
            return;
        }

        *comma = '\0';

        unsigned long long addr =   strtoull(args, NULL, 16);
        unsigned long long length = strtoull(comma + 1, NULL, 16);

        if (length == 0) {
            dbgstub_send("E01");
            return;
        }

        for (unsigned long long k = 0; k < length; k++) {
            uint16_t byte = safe_read_byte((void *)(addr + k));
            if (byte == 0xFFFF) {
                dbgstub_send("E02");
                return;
            }

            const char hex[] = "0123456789abcdef";
            dbgstub_send_buffer[(k * 2) + 0] = hex[byte >>  4];
            dbgstub_send_buffer[(k * 2) + 1] = hex[byte & 0xf];
        }

        dbgstub_send_buffer[length * 2] = '\0';
        dbgstub_send(dbgstub_send_buffer);
        return;
    }

    if (strncmp("$M", dbgstub_packet_recv_buffer, 2) == 0) {
        char *args = dbgstub_packet_recv_buffer + 2;
        char *comma = strchr(args, ',');
        char *colon = strchr(args, ':');

        if (!comma || !colon || comma > colon) {
            dbgstub_send("E00");
            return;
        }

        *comma = '\0';
        *colon = '\0';

        unsigned long long addr   = strtoull(args, NULL, 16);
        unsigned long long length = strtoull(comma + 1, NULL, 16);

        char *data = colon + 1;

        if (length == 0 || strlen(data) < length * 2) {
            dbgstub_send("E01");
            return;
        }

        for (unsigned long long k = 0; k < length; k++) {
            char hex[3];
            hex[0] = data[k * 2];
            hex[1] = data[k * 2 + 1];
            hex[2] = '\0';

            uint8_t value = (uint8_t)strtoull(hex, NULL, 16);

            uint16_t res = safe_write_byte((void *)(addr + k), value);
            if (res == 0xFFFF) {
                dbgstub_send("E03");
                return;
            }
        }

        dbgstub_send("OK");
        return;
    }

    if (strncmp("$z1", dbgstub_packet_recv_buffer, 3) == 0) {
        char *addrString = dbgstub_packet_recv_buffer + 4;
        unsigned long long addr = strtoull(addrString, NULL, 16);
        if (drx_free_bp(addr))
            dbgstub_send("OK");
        else
            dbgstub_send("E.INVALID_BREAKPOINT");
        return;
    }

    if (strncmp("$Z1", dbgstub_packet_recv_buffer, 3) == 0) {
        char *addrString = dbgstub_packet_recv_buffer + 4;
        unsigned long long addr = strtoull(addrString, NULL, 16);
        if (drx_alloc_bp(addr))
            dbgstub_send("OK");
        else
            dbgstub_send("E.OUT_OF_HARDWARE_BREAKPOINTS");
        return;
    }

    if (strncmp("$c", dbgstub_packet_recv_buffer, 2) == 0) {
        // TODO: properly parse the addr field if given
        last_frame->flags &= ~0x100;
        dbgstub_paused = false;
        dbgstub_continue = true;
        return;
    }

    if (strncmp("$C", dbgstub_packet_recv_buffer, 2) == 0) {
        // TODO: properly parse the addr field if given
        last_frame->flags &= ~0x100;
        dbgstub_paused = false;
        dbgstub_continue = true;
        return;
    }

    if (strncmp("$s", dbgstub_packet_recv_buffer, 2) == 0) {
        // TODO: properly parse the addr field if given
        dbgstub_paused = true;
        dbgstub_continue = true;
        return;
    }

    dbgstub_send("");
}

struct interrupt_frame* dbgstub_exception(struct interrupt_frame* frame) {
    int irql = irql_raise(14);
    if (drx_cnc_bp()) {
        frame->flags |= 0x10000; // set RF
        frame->flags |= 0x100;   // set TF
        dbgstub_paused = true;
        dbgstub_continue = false;
    }

    memcpy(last_frame, frame, sizeof(struct interrupt_frame));
    dbgstub_send("S05");

    while (dbgstub_paused) {
        if (dbgstub_continue) {
            dbgstub_continue = false;
            break;
        }
        asm volatile ("pause");
    }

    irql_lower(irql);
    return last_frame;
}

bool dbgstub_should_preempt() {
    return !dbgstub_paused;
}
