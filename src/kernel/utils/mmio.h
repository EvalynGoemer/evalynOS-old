#pragma once

#include <stdint.h>
#include <stdio.h>

static inline void mmio_write_offset_64(uint64_t base_addr, uint64_t offset, uint64_t value) {
    volatile uint64_t *addr = (volatile uint64_t *)(base_addr + offset);
    *addr = value;
}

static inline uint64_t mmio_read_offset_64(uint64_t base_addr, uint64_t offset) {
    volatile uint64_t *addr = (volatile uint64_t *)(base_addr + offset);
    return *addr;
}

static inline void mmio_write_offset_32(uint64_t base_addr, uint64_t offset, uint32_t value) {
    volatile uint32_t *addr = (volatile uint32_t *)(base_addr + offset);
    *addr = value;
}

static inline uint32_t mmio_read_offset_32(uint64_t base_addr, uint64_t offset) {
    volatile uint32_t *addr = (volatile uint32_t *)(base_addr + offset);
    return *addr;
}

static inline void mmio_write_offset_8(uint64_t base_addr, uint64_t offset, uint8_t value) {
    volatile uint8_t *addr = (volatile uint8_t *)(base_addr + offset);
    *addr = value;
}

static inline uint8_t mmio_read_offset_8(uint64_t base_addr, uint64_t offset) {
    volatile uint8_t *addr = (volatile uint8_t *)(base_addr + offset);
    return *addr;
}

static inline void mmio_write_64(uint64_t addr, uint64_t value) {
    *(volatile uint64_t *)addr = value;
}

static inline uint64_t mmio_read_64(uint64_t addr) {
    return *(volatile uint64_t *)addr;
}

static inline void mmio_write_32(uint64_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read_32(uint64_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write_8(uint64_t addr, uint8_t value) {
    *(volatile uint8_t *)addr = value;
}

static inline uint8_t mmio_read_8(uint64_t addr) {
    return *(volatile uint8_t *)addr;
}
