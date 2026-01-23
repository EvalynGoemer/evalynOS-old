#pragma once
#include <stdint.h>

enum {
    CPUID_GET_MAX_STANDARD = 0x00000000,
    CPUID_GET_FEATURES     = 0x00000001,
    CPUID_GET_FEATURES_EXT = 0x00000007,
    CPUID_GET_FREQ_INFO1   = 0x00000015,
    CPUID_GET_FREQ_INFO2   = 0x00000016,
    CPUID_GET_MAX_EXTENDED = 0x80000000,
    CPUID_GET_CAPABILITES  = 0x80000007,

    CPUID_x2APIC           = 21,
    CPUID_ADJUST_TSC       = 1,  // EBX
    CPUID_INVARIANT_TSC    = 8,  // EDX
    CPUID_LAPIC_TSC        = 24, // ECX
    CPUID_HYPERVISOR       = 31, // ECX
    CPUID_SMEP             = 7,  // EBX
    CPUID_SMAP             = 20, // EBX

    CPUID_EAX              = 'a',
    CPUID_EBX              = 'b',
    CPUID_ECX              = 'c',
    CPUID_EDX              = 'd',

    CPUID_NO_SUBLEAF       = 0,
};

struct cpuid_regs {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};

extern struct cpuid_regs cpuid(uint32_t leaf, uint32_t subleaf);
extern int cpuid_extended_supported(uint32_t leaf);
extern int cpuid_standard_supported(uint32_t leaf);
extern int cpu_feature_bit(uint32_t leaf, uint32_t subleaf, char reg, int bit);
extern char* get_cpu_vendor();
extern char* get_hypervisor_id();
extern char* get_cpu_name();
