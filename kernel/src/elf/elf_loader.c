#include <elf/elf.h>
#include <elf/elf_structs.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <scheduler/scheduler.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "elf.h"
#include "memory/vma.h"

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

struct elf_info load_elf(void *file, pagemap_t *pagemap) {
    if (!verify_elf_64(file)) {
        printf("ELF: Failed to load ELF file; VALIDATION FAILED\n");
        return (struct elf_info){0};
    }
    struct elf_header_64 *header = (struct elf_header_64*)file;
    struct elf_program_header_64 *prog_headers = (struct elf_program_header_64*)((uint8_t*)file + header->program_header_table);

    uint64_t load_base = 0;
    for (size_t i = 0; i < header->program_header_entries; i++) {
        struct elf_program_header_64 *ph = &prog_headers[i];
        if (ph->type == ELF_PROG_PT_LOAD_TYPE) {
            load_base = ph->virt_addr - ph->offset;
            break;
        }
    }

    for (size_t i = 0; i < header->program_header_entries; i++) {
        struct elf_program_header_64 *ph = &prog_headers[i];

        if (ph->type != ELF_PROG_PT_LOAD_TYPE) continue;

        uint64_t start = ph->virt_addr;
        uint64_t end = start + ph->mem_size;
        uint64_t aligned_start = start & ~(PAGE_SIZE - 1);
        uint64_t aligned_end = ALIGN_UP(end, PAGE_SIZE);
        uint64_t pagesToMap = (aligned_end - aligned_start) / PAGE_SIZE;

        uint64_t pte_flags = PTE_PRESENT | PTE_USER;
        if (ph->flags & ELF_PROG_WRITE) pte_flags |= PTE_WRITABLE;
        if (!(ph->flags & ELF_PROG_EXEC_FLAG)) pte_flags |= PTE_NX;

        uint64_t currentVirtAddr;
        if (ph->alignment > 1) {
            currentVirtAddr = ph->virt_addr & ~(ph->alignment - 1);
        } else {
            currentVirtAddr = ph->virt_addr;
        }

        uint64_t vaddr_offset = ph->virt_addr - currentVirtAddr;
        valloc(pagemap, ph->mem_size + vaddr_offset, currentVirtAddr);

        uint64_t fileRemaining = ph->file_size;
        uint8_t *src = (uint8_t *)file + ph->offset;

        for (uint64_t p = 0; p < pagesToMap; p++) {
            uint64_t ppage = (uint64_t)allocate_page();
            uint64_t vpage = ppage + hhdm_request.response->offset;
            vmm_map_page(pagemap, currentVirtAddr, ppage, pte_flags);

            uint64_t offset = (p == 0) ? vaddr_offset : 0;
            uint64_t space = PAGE_SIZE - offset;
            uint64_t toCopy = fileRemaining < space ? fileRemaining : space;
            if (offset) memset((void*)vpage, 0, offset);
            if (toCopy) memcpy((void*)(vpage + offset), src, toCopy);
            if (space > toCopy) memset((void*)(vpage + offset + toCopy), 0, space - toCopy);

            src += toCopy;
            fileRemaining -= toCopy;

            currentVirtAddr += PAGE_SIZE;
        }
    }

    return (struct elf_info){.entry_point = header->entry_point,
                             .phdr = load_base + header->program_header_table,
                             .phnum = header->program_header_entries,
                             .phentsize = header->program_header_size};
}
