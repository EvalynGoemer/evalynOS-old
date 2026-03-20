#include <drivers/timer.h>
#include <drivers/x86_64/timers/tsc.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/cpuid.h>

uint64_t tsc_frequency = 0;
bool tsc_good = false;

bool setup_tsc() {
    if (cpuid_extended_supported(CPUID_GET_CAPABILITES)) {
        if(!cpu_feature_bit(CPUID_GET_CAPABILITES, CPUID_NO_SUBLEAF, CPUID_EDX, CPUID_INVARIANT_TSC)) {
            printf("TSC: TSC Unusable for timing: TSC is variant\n");
            tsc_good = false;
            return tsc_good;
        }
    }

    if (cpuid_standard_supported(CPUID_GET_FREQ_INFO1)) {
        struct cpuid_regs frequency_info = cpuid(CPUID_GET_FREQ_INFO1, CPUID_NO_SUBLEAF);
        if (frequency_info.eax && frequency_info.ebx && frequency_info.ecx) {
            tsc_frequency = ((uint64_t)frequency_info.ecx * ((uint64_t)frequency_info.ebx / (uint64_t)frequency_info.eax));
            tsc_good = true;
            goto end;
        }
    }

    if (currentTimerSource == TIMER_SOURCE_NONE) {
        printf("TSC: TSC Unusable for timing: No timer source to calibrate\n");
        tsc_good = false;
        return tsc_good;
    }

    uint64_t total_tsc_delta = 0;
    const int iterations = 3;
    for (int i = 0; i < iterations; i++) {
        uint64_t start = read_tsc();
        timer_blocking_sleep_ms(50);
        uint64_t end = read_tsc();
        total_tsc_delta += (end - start);
    }
    tsc_frequency = (total_tsc_delta * 20) / iterations;
    tsc_good = true;

    end:
    if (tsc_frequency < (uint64_t)10e6) {
        printf("TSC: TSC Unusable for timing: Calibration showed invalid frequency of %ldMHz\n", (tsc_frequency / (uint64_t)1e6));
        tsc_good = false;
        return tsc_good;
    }

    if (tsc_good) {
        currentTimerSource = TIMER_SOURCE_TSC;
        printf("TSC: Usable TSC found with frequency of %ldMHz\n", (tsc_frequency / (uint64_t)1e6));
    }

    return tsc_good;
}
