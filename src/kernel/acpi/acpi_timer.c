#include <drivers/x86_64/ports.h>
#include <drivers/timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <acpi/acpi_tables.h>
#include <stdio.h>
#include <utils/mmio.h>

#define ACPI_TIMER_FREQUENCY 3579545

uint16_t acpi_timer_address_type = 0;
uint64_t acpi_timer_address = 0;

static inline void io_wait() {
    outb(0x80, 0);
}

void acpi_timer_mmio_blocking_sleep_ms(uint64_t ms) {
    if (acpi_timer_address == 0) {
        return;
    }

    uint64_t ticks_needed = (ACPI_TIMER_FREQUENCY * ms) / 1000;
    uint64_t ticks_elapsed = 0;
    uint32_t last_tick = mmio_read_32(acpi_timer_address) & (uint32_t)0xFFFFFF;

    while (ticks_elapsed < ticks_needed) {
        uint32_t current_tick = mmio_read_32(acpi_timer_address) & (uint32_t)0xFFFFFF;
        ticks_elapsed += (uint32_t)(current_tick - last_tick);
        last_tick = current_tick;
    }
}

void acpi_timer_port_io_blocking_sleep_ms(uint64_t ms) {
    if (acpi_timer_address == 0) {
        return;
    }

    uint64_t ticks_needed = (ACPI_TIMER_FREQUENCY * ms) / 1000;
    uint64_t ticks_elapsed = 0;
    uint32_t last_tick = inl(acpi_timer_address) & (uint32_t)0xFFFFFF;

    while (ticks_elapsed < ticks_needed) {
        uint32_t current_tick = inl(acpi_timer_address) & (uint32_t)0xFFFFFF;
        ticks_elapsed += (uint32_t)(current_tick - last_tick);
        last_tick = current_tick;
    }
}

bool setup_acpi_timer() {
    if (acpi_timer_address == 0) {
        return false;
    }

    if (acpi_timer_address_type == ACPI_ADDRESS_TYPE_MMIO) {
        if (mmio_read_32(acpi_timer_address) != 0) {
            currentTimerSource = TIMER_SOURCE_ACPI;
            timer_blocking_sleep_ms = acpi_timer_mmio_blocking_sleep_ms;
            return true;
        }
        printf("ACPI Timer: Timer read zero; Retrying\n");
        io_wait(); io_wait(); io_wait(); io_wait();
        if (mmio_read_32(acpi_timer_address) != 0) {
            currentTimerSource = TIMER_SOURCE_ACPI;
            timer_blocking_sleep_ms = acpi_timer_mmio_blocking_sleep_ms;
            return true;
        }
        printf("ACPI Timer: Failed to read\n");
        return false;
    } else if (acpi_timer_address_type == ACPI_ADDRESS_TYPE_PORT_IO) {
        if (inl(acpi_timer_address) != 0) {
            currentTimerSource = TIMER_SOURCE_ACPI;
            timer_blocking_sleep_ms = acpi_timer_port_io_blocking_sleep_ms;
            return true;
        }
        printf("ACPI Timer: Timer read zero; Retrying\n");
        io_wait(); io_wait(); io_wait(); io_wait();
        currentTimerSource = TIMER_SOURCE_ACPI;
        timer_blocking_sleep_ms = acpi_timer_port_io_blocking_sleep_ms;
        if (inl(acpi_timer_address) != 0) {
            return true;
        }
        printf("ACPI Timer: Failed to read\n");
        return false;
    }

    return true;
}
