#include <interupts/interupts.h>
#include <utils/cpulocal.h>

void idle_thread() {
    while (1) {
        CPU_LOCAL_WRITE(irq_should_preempt, true);
        asm("hlt");
    }
}
