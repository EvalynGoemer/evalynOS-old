#pragma once
#include "interupts/interupts.h"

__attribute__((noreturn))
extern void panic(char* message);

__attribute__((noreturn))
extern void panic_interrupt_frame(char* message, struct interrupt_frame* frame);

extern int panic_count;
