/*
 * EvalynOS FRED (Flexible Return and Event Delivery) Implementation
 *   \/________________
 * /     _____________)
 * /     /     /   \ |
 * \/\/\/     (O) (O)|
 *  |           ------,
 *  |  _       ______/
 *  | (_      /   \  \
 *  |        /  ___\_ \
 *  |        \      / /
 * __|_________\______/
 * \______________\./__\
 * /     .       | \  |
 * \    /_\   .  |  \ |\
 * |`\       /_\ |   \| \
 *
 * This is built using only hopes and dreams
 * Only tested on intel SIMICS emulator
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct fred_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t error, ip, cs, flags, rsp, ss;
    uint64_t fred_event_data;
    uint64_t fred_reserved;
} fred_frame_t;

extern bool fred_enbled;

extern bool setup_fred();

extern void fred_switch_to_user(uint64_t start_addr, uint64_t stack_top);

extern void fred_ring3_entry(fred_frame_t* frame);
extern void fred_ring0_entry(fred_frame_t* frame);
extern void fred_ring3_entry_asm_stub();
