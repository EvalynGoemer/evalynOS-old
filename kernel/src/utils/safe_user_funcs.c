#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>
#include <drivers/x86_64/rflags.h>
#include <interupts/interupts.h>
#include <utils/safe_user_funcs.h>

#define USER_TOP    0x0000800000000000ULL
#define KERNEL_BASE 0xFFFF800000000000ULL

static inline bool is_kernel_range(const void *p, uint64_t len) {
    uint64_t a = (uint64_t) p;
    bool wrap = len > ~a;
    return !wrap && (a & (a+len) & KERNEL_BASE) == KERNEL_BASE;
}

#define is_user_range(ptr, len) ({ !is_kernel_range(ptr, len); })

int copy_to_user(void *udst, const void *ksrc, uint64_t len) {
    if (!is_user_range(udst, len) || !is_kernel_range(ksrc, len))
        return -1;

    rflags_set_ac();
    if (safe_memcpy_asm(udst, ksrc, len) == (void*)-1) {
        rflags_clr_ac();
        return -1;
    }

    rflags_clr_ac();
    return 0;
}

int copy_from_user(void *kdst, const void *usrc, uint64_t len) {
    if (!is_kernel_range(kdst, len) || !is_user_range(usrc, len))
        return -1;

    rflags_set_ac();
    if (safe_memcpy_asm(kdst, usrc, len) == (void*)-1) {
        rflags_clr_ac();
        return -1;
    }
    rflags_clr_ac();

    return 0;
}

int strlen_user(char *string) {
    int len;

    if (!is_user_range(string, 4096))
        return -1;

    rflags_set_ac();
    len = (int)safe_strlen_asm(string);
    rflags_clr_ac();

    if (len < 0)
        return -1;

    return len;
}

uint16_t safe_read_byte(const void *addr) {
    uint8_t value = 0;

    void* ret = safe_memcpy_asm(&value, addr, 1);

    if (ret == (void*)-1)
        return 0xFFFF;

    return (uint8_t)value;
}

uint16_t safe_write_byte(void *addr, uint8_t value) {
    void* ret = safe_memcpy_asm(addr, &value, 1);

    if (ret == (void*)-1)
        return 0xFFFF;

    return 0;
}

int fault_handle_safe_funcs(struct interrupt_frame* frame) {
    if (frame->ip >= (uint64_t)&safe_memcpy_asm_start &&
        frame->ip <  (uint64_t)&safe_memcpy_asm_end) {
        frame->ip =  (uint64_t)&safe_memcpy_asm_fail;
        return 0;
    }

    if (frame->ip >= (uint64_t)&safe_strlen_asm_start &&
        frame->ip <  (uint64_t)&safe_strlen_asm_end) {
        frame->ip =  (uint64_t)&safe_strlen_asm_fail;
        return 0;
    }

    return -1;
}
