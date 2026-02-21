#include "drivers/x86_64/ports.h"
#include "drivers/x86_64/cpuid.h"
#include <stdbool.h>
#include <utils/globals.h>
#include <drivers/x86_64/serial.h>

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <flanterm.h>

#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_ALT_FORM_FLAG 1
#include <nanoprintf.h>

#include <utils/spinlock.h>

[[gnu::aligned(64)]] spinlock_t stdio_spinlock = {0};

void internal_putc(int c, void *_) {
    char ch = (char)c;

    flanterm_write(ft_ctx, &ch, 1);

    if (cpu_feature_bit(1, 0, 'c', CPUID_HYPERVISOR)) {
        outb(0xE9, ch);
    }

    if(serial_works) {
        write_serial(&ch, 1);
    }
}

int printf(const char* fmt, ...) {
    bool lock1r = spinlock_lock(&stdio_spinlock);
    va_list args;
    va_start(args, fmt);
    int ret = npf_vpprintf(internal_putc, NULL, fmt, args);
    va_end(args);
    spinlock_unlock(&stdio_spinlock, lock1r);
    return ret;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    npf_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return 0;
}
