#include <interupts/interupts.h>
#include <drivers/x86_64/ports.h>
#include <scheduler/scheduler.h>
#include <stdint.h>

volatile uint64_t pitInteruptsTriggered = 0;
volatile int shouldSchedule = 0;

void pit_isr() {
    pitInteruptsTriggered++;

    if (shouldSchedule && ((pitInteruptsTriggered % 10) == 0)) {
        schedule();
    }

    outb(0x20,0x20);
}
