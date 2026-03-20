#include <stdint.h>
#include <stdio.h>
#include <drivers/x86_64/cpuid.h>

void setup_cr0() {
    __asm__ volatile (
        "mov %%cr0, %%rax\n"
        "btr $2, %%rax\n"    // clear EM
        "btr $3, %%rax\n"    // clear TS
        "bts $1, %%rax\n"    // set   MP
        "bts $5, %%rax\n"    // set   NE
        "mov %%rax, %%cr0"
        :
        :
        : "rax", "memory", "cc"
    );
    printf("CRX: CR0 Setup\n");
}

uint8_t cr4_smap_enabled = 0;

void setup_cr4() {
    __asm__ volatile (
        "mov %%cr4, %%rax\n"
        "bts $4, %%rax\n"    // set PSE
        "bts $9, %%rax\n"    // set OSFXSR
        "bts $10, %%rax\n"   // set OSXMMEXCPT
        "mov %%rax, %%cr4"
        :
        :
        : "rax", "memory", "cc"
    );

    if (cpuid_standard_supported(CPUID_GET_FEATURES_EXT)) {
        if (cpu_feature_bit(CPUID_GET_FEATURES_EXT, CPUID_NO_SUBLEAF, CPUID_EBX, CPUID_SMEP)) {
            __asm__ volatile (
                "mov %%cr4, %%rax\n"
                "bts $20,   %%rax\n" // Set SMEP
                "mov %%rax, %%cr4"
                :
                :
                : "rax", "memory"
            );
            printf("CRX: SMEP Enabled\n");
        } else {
            printf("CRX: SMEP could not be enabled\n");
        }

        if (cpu_feature_bit(CPUID_GET_FEATURES_EXT, CPUID_NO_SUBLEAF, CPUID_EBX, CPUID_SMAP)) {
            __asm__ volatile (
                "mov %%cr4, %%rax\n"
                "bts $21,   %%rax\n" // Set SMAP
                "mov %%rax, %%cr4"
                :
                :
                : "rax", "memory"
            );
            cr4_smap_enabled = 1;
            printf("CRX: SMAP Enabled\n");
        } else {
            cr4_smap_enabled = 0;
            printf("CRX: SMAP could not be enabled\n");
        }
    } else {
        printf("CRX: SMEP or SMAP could not be enabled\n");
    }

    printf("CRX: CR4 Setup\n");
}

void setup_control_registers() {
    setup_cr0();
    setup_cr4();
}
