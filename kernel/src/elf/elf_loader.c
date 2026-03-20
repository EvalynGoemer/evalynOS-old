#include "memory/vma.h"
#include <stdint.h>
#include <stdio.h>

#include <elf/elf.h>
#include <elf/elf_structs.h>

#include <memory/vmm.h>
#include <memory/pmm.h>
#include <scheduler/scheduler.h>
#include <string.h>

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

uint64_t load_elf(void* file, pagemap_t* pagemap) {
    if (!verify_elf_64(file)) {
        printf("ELF: Failed to load ELF file; VALIDATION FAILED\n");
        return 0;
    }

    struct elf_header_64 *header = (struct elf_header_64 *)file;
    struct elf_program_header_64 *prog_headers = (struct elf_program_header_64 *)((uint8_t *)file + header->program_header_table);

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

        valloc(pagemap, ph->mem_size, currentVirtAddr);

        uint64_t fileRemaining = ph->file_size;
        uint64_t memRemaining  = ph->mem_size;
        uint8_t* src = (uint8_t*)file + ph->offset;

        for (uint64_t p = 0; p < pagesToMap; p++) {
            uint64_t ppage = (uint64_t)allocate_page();
            uint64_t vpage =  ppage + hhdm_request.response->offset;
            vmm_map_page(pagemap, currentVirtAddr, ppage, pte_flags);

            uint64_t toCopy = fileRemaining > PAGE_SIZE ? PAGE_SIZE : fileRemaining;
            if (toCopy)
                memcpy((void*)vpage, src, toCopy);

            uint64_t toZero = PAGE_SIZE - toCopy;
            if (toZero && memRemaining > toCopy)
                memset((void*)vpage + toCopy, 0, toZero);

            src += toCopy;
            fileRemaining -= toCopy;
            memRemaining  -= PAGE_SIZE;
            currentVirtAddr += PAGE_SIZE;
        }
    }

    return header->entry_point;
}
