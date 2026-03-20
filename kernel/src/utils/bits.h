#pragma once
#include <stdint.h>

static inline uint64_t extract_bits(uint64_t number, uint8_t start, uint8_t end) {
    uint8_t width = end - start + 1;
    uint64_t mask = (1ULL << width) - 1;
    return (number >> start) & mask;
}

static inline uint64_t insert_bits(uint64_t target, uint64_t bits, uint8_t start, uint8_t end) {
    uint8_t width = end - start + 1;
    uint64_t mask = ((1ULL << width) - 1) << start;
    target &= ~mask;
    target |= (bits << start) & mask;
    return target;
}

static inline uint64_t insert_bit(uint64_t target, uint8_t bit, uint8_t start, uint8_t end) {
    uint8_t width = end - start + 1;
    uint64_t mask = ((1ULL << width) - 1) << start;
    if (bit)
        target |= mask;
    else
        target &= ~mask;
    return target;
}
