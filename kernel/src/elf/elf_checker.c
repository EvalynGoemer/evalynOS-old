#include <stdbool.h>

#include <elf/elf_structs.h>
#include <stdio.h>

bool verify_elf_magic(struct elf_header_64 *header) {
    return header->magic[0] == 0x7F &&
           header->magic[1] == 'E' &&
           header->magic[2] == 'L' &&
           header->magic[3] == 'F';
}

bool verify_elf_64(void* file) {
    struct elf_header_64 *header = (struct elf_header_64 *)file;
    if (verify_elf_magic(header)) {
        // printf("ELF: Magic Value Good\n");
    } else {
        printf("ELF: BAD ELF FILE\n");
        return false;
    }

    if (header->header_version != ELF_HEADER_VER) {
        printf("ELF: BAD ELF FILE\n");
        return false;
    }

    if (header->elf_version != ELF_VER) {
        printf("ELF: BAD ELF FILE\n");
        return false;
    }

    if (header->bits == ELF_64_BIT) {
        // printf("ELF: 64 Bit Elf File\n");
    } else if (header->bits == ELF_32_BIT) {
        printf("ELF: 32 Bit Elf File; Not Supported\n");
    } else {
        printf("ELF: BAD ELF FILE\n");
        return false;
    }

    if (header->endianness == ELF_LITTLE_ENDIAN) {
        // printf("ELF: Little Endian Elf File\n");
    } else if (header->endianness == ELF_BIG_ENDIAN) {
        printf("ELF: Big Endian Elf File; Not Supported\n");
        return false;
    } else {
        printf("ELF: BAD ELF FILE\n");
        return false;
    }

    if (header->arch == ELF_X86_64_ARCH) {
        // printf("ELF: x86-64 Detected\n");
    } else {
        printf("ELF: Unkown Arch Detected; Not Supported\n");
        return false;
    }

    if (header->abi == ELF_SYSV_ABI) {
        // printf("ELF: SYSV Detected\n");
    } else {
        printf("ELF: Unkown Arch Detected; Not Supported\n");
        return false;
    }

    if (header->type == ELF_EXECUTABLE_TYPE) {
        // printf("ELF: Executable Detected\n");
    } else {
        printf("ELF: Unkown Type Detected; Not Supported\n");
        return false;
    }

    return true;
}
