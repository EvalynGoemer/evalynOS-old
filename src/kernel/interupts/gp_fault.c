#include <interupts/interupts.h>
#include <utils/panic.h>

void gp_fault_isr(struct interrupt_frame* frame) {
    panic_interrupt_frame("General Protection Fault", frame);
}
