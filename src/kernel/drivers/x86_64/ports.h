#pragma once
#include <stdint.h>

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ( "inb %w1, %b0"
    : "=a"(ret)
    : "Nd"(port)
    : "memory");
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "dN"(port)
    );
    return value;
}
