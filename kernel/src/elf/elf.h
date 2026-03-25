#pragma once

#include <memory/vma.h>
#include <stdint.h>
#include <stdbool.h>

extern bool verify_elf_64(void* file);
struct elf_info {
    uint64_t entry_point;
    uint64_t phdr;
    uint64_t phentsize;
    uint64_t phnum;
};
extern struct elf_info load_elf(void* file, pagemap_t* pagemap);
