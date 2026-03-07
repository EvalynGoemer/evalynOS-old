#include "drivers/x86_64/fred/fred.h"
#include "interupts/spurious.h"
#include <interupts/interupts.h>
#include <drivers/x86_64/timers/pit.h>
#include <drivers/x86_64/apic/apic.h>
#include <drivers/x86_64/irq.h>
#include <drivers/x86_64/rflags.h>
#include <drivers/timer.h>
#include <drivers/dbgstub/dbgstub.h>
#include <utils/cmdline.h>
#include <string.h>

void dispatch_interupt (struct interrupt_frame *frame) {
    if (frame->cs & 0x3) {
        rflags_clr_ac();
        if (!fred_enbled) {
            asm volatile ("swapgs");
        }
    }

    int new_irql = (frame->vector >> 4) & 0xf; int old_irql;
    if (new_irql >= IRQL_DISPATCH)
        old_irql = irql_raise(new_irql);

    send_eoi();
    asm volatile ("sti");

    switch (frame->vector) {
        case INTERRUPT_HANDLER_BREAKPOINT_TRAP:
        case INTERRUPT_HANDLER_DEBUG_TRAP:
            if (dbgstub_enabled) {
                struct interrupt_frame *nframe = dbgstub_exception(frame);
                memmove(frame, nframe, sizeof(struct interrupt_frame));
            }
            break;
        case INTERRUPT_HANDLER_DOUBLE_FAULT:
            double_fault_isr(frame);
            break;
        case INTERRUPT_HANDLER_GENERAL_PROTECTION_FAULT:
            gp_fault_isr(frame);
            break;
        case INTERRUPT_HANDLER_PAGE_FAULT:
            page_fault_isr(frame);
            break;
        case INTERRUPT_HANDLER_APIC_TIMER:
            apic_timer_isr();
        case INTERRUPT_HANDLER_PS2_MOUSE:
        case INTERRUPT_HANDLER_PS2:
            ps2_isr();
            break;
        case INTERRUPT_HANDLER_HIGH_PRIORITY_SERIAL:
            serial_isr(frame);
            break;
        case INTERRUPT_HANDLER_SERIAL:
            serial_isr(frame);
            break;
        case INTERRUPT_HANDLER_SPURIOUS_PIC_1:
        case INTERRUPT_HANDLER_SPURIOUS_PIC_2:
        case INTERRUPT_HANDLER_SPURIOUS_APIC:
            spurious_isr();
            break;
        default:
            generic_isr(frame);
            break;
    }

    asm volatile ("cli");

    if (new_irql >= IRQL_DISPATCH)
        irql_lower(old_irql);

    if (frame->cs & 0x3) {
        if (!fred_enbled) {
            asm volatile ("swapgs");
        }
    }
}
