#include "stdbool.h"
#include <interupts/interupts.h>
#include <utils/panic.h>
#include <stdio.h>

void page_fault_isr(struct interrupt_frame* frame) {
    uint64_t error = frame->error & 0x7f;
    bool p    = error   & 0x1;
    bool rw   = error   & 0x2;
    bool us   = error   & 0x4;
    bool rsvd = error   & 0x8;
    bool id   = error   & 0x10;

    if (rsvd) {
        panic_interrupt_frame("PF: Reserved bit set", frame);
    }

    const char* who    = us ? "User"  : "Kernel";
    const char* action = rw ? "write to" : "read from";
    const char* type = p    ? "read only" : "non present";

    if (!rw && id) {
        action = "execute";
    }

    if (id && p && !rw) {
        type = "no execute";
    }

    unsigned long cr2;
    asm volatile(
        "mov %%cr2, %0\n\t"
        : "=r"(cr2)
    );

    char msg[256];
    snprintf(msg, sizeof(msg), "PF: %s process tried to %s a %s page at 0x%016lx", who, action, type, cr2);
    panic_interrupt_frame(msg, frame);
}
