#pragma once

#include <stdint.h>
#include <interupts/interupts.h>

extern int fault_handle_safe_funcs(struct interrupt_frame* frame);

extern int copy_to_user(void *udst, const void *ksrc, uint64_t len);
extern int copy_from_user(void *kdst, const void *usrc, uint64_t len);
extern int strlen_user(char *string);

extern uint16_t safe_read_byte(const void *addr);
extern uint16_t safe_write_byte(void *addr, uint8_t value);

void *safe_memcpy_asm(void *dst, const void *src, uint64_t len);
extern char safe_memcpy_asm_start;
extern char safe_memcpy_asm_end;
extern char safe_memcpy_asm_fail;

uint64_t safe_strlen_asm(const char *string);
extern char safe_strlen_asm_start;
extern char safe_strlen_asm_end;
extern char safe_strlen_asm_fail;
