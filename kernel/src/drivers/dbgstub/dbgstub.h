#pragma once
#include <interupts/interupts.h>
#include <stdbool.h>

extern void dbgstub_init();
extern void dbgstub_panic(char* msg);
extern void dbgstub_tx(char c);
extern void dbgstub_rx(char c, struct interrupt_frame* frame);
extern void dbgstub_process_packet(struct interrupt_frame* frame);
extern struct interrupt_frame* dbgstub_exception(struct interrupt_frame* frame);

extern bool dbgstub_should_preempt();
#define dbgstub_should_debug() !dbgstub_should_preempt()
