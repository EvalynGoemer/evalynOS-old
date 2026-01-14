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

    /*
     * The intel SDM https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
     * as of 11/11/26 states CPUID leaf 0x15 can give the TSC's frequency but on an intel laptop
     * with an Intel® Core™ i5-12450H this seemingly reports a nonsense value when computing
     * the TSC frequency as the manual states with TSC_frequency = ECX * EBX/EAX so this
     * method is disabled until futher info is gotten. Calibrating it normaly works fine anyways.
     *
     * See Volume 1, Section 21.3, Subsection CPUID.15H
     */

    // if (cpuid_standard_supported(CPUID_GET_FREQ_INFO1)) {
    //     struct cpuid_regs frequency_info = cpuid(CPUID_GET_FREQ_INFO1, CPUID_NO_SUBLEAF);
    //     if (frequency_info.eax && frequency_info.ebx && frequency_info.ecx) {
    //         tsc_frequency = (frequency_info.ecx * (frequency_info.ebx / frequency_info.eax));
    //         tsc_good = true;
    //         goto end;
    //     }
    // }

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
