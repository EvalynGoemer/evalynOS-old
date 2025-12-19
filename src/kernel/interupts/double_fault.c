#include <interupts/interupts.h>
#include <utils/panic.h>

void double_fault_isr(struct interrupt_frame* frame) {
    panic_interrupt_frame("Double Fault", frame);
}
