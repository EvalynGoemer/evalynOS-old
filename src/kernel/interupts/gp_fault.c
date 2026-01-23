#include <interupts/interupts.h>
#include <utils/safe_user_funcs.h>
#include <utils/panic.h>

void gp_fault_isr(struct interrupt_frame* frame) {
    if (fault_handle_safe_funcs(frame) == 0) {
        return;
    }

    panic_interrupt_frame("General Protection Fault", frame);
}
