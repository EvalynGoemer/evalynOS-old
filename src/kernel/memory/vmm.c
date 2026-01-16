#include "elf/elf.h"
#include "elf/elf_structs.h"
#include "limine.h"
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/x86_64/cpuid.h>
#include <utils/globals.h>
#include <utils/panic.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <stdlib.h>

#define ALIGN_UP(x, align) ((((uintptr_t) (x)) + ((align) - 1)) & ~((uintptr_t) ((align) - 1)))
#define ALIGN_DOWN(x, align) (((uintptr_t) (x)) & ~((uintptr_t) ((align) - 1)))

pagemap_t kernel_pagemap = {0};

void vmm_switch_to(pagemap_t *pagemap) {
    uintptr_t cr3 = (uintptr_t)pagemap->top_level - hhdm_request.response->offset;
    asm volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
}

void vmm_map_page(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
    uint16_t pml1i = (virt_addr >> 12) & 0x1ff;
    uint16_t pml2i = (virt_addr >> 21) & 0x1ff;
    uint16_t pml3i = (virt_addr >> 30) & 0x1ff;
    uint16_t pml4i = (virt_addr >> 39) & 0x1ff;

    if (!(pagemap->top_level[pml4i] & PTE_PRESENT)) {
        void* new_page = allocate_page();
        pagemap->top_level[pml4i] = (uint64_t)new_page & PTE_MASK;
        pagemap->top_level[pml4i] |= PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t* pml3v = (uint64_t*)((pagemap->top_level[pml4i] & PTE_MASK) + hhdm_request.response->offset);

    if (!(pml3v[pml3i] & PTE_PRESENT)) {
        void* new_page = allocate_page();
        pml3v[pml3i] = (uint64_t)new_page & PTE_MASK;
        pml3v[pml3i] |= PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t* pml2v = (uint64_t*)((pml3v[pml3i] & PTE_MASK) + hhdm_request.response->offset);

    if (!(pml2v[pml2i] & PTE_PRESENT)) {
        void* new_page = allocate_page();
        pml2v[pml2i] = (uint64_t)new_page & PTE_MASK;
        pml2v[pml2i] |= PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t* pml1v = (uint64_t*)((pml2v[pml2i] & PTE_MASK) + hhdm_request.response->offset);

    pml1v[pml1i] = phys_addr & PTE_MASK;
    pml1v[pml1i] |= PTE_PRESENT | flags;
}

void vmm_map_pages_continuous(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t page_count, uint64_t flags) {
    for(uint64_t i = 0; i < page_count; i++) {
        vmm_map_page(pagemap, virt_addr + (i * PAGE_SIZE), phys_addr + (i * PAGE_SIZE), flags);
    }
}

void setup_vmm() {
    uint64_t pml4_phys = (uint64_t)allocate_page();
    uint64_t* pml4_virt = (void*)(pml4_phys + hhdm_request.response->offset);

    for (int i = 256; i < 511; i++) {
        pml4_virt[i] = (uint64_t)allocate_page();
        pml4_virt[i] |= PTE_PRESENT | PTE_WRITABLE | PTE_PERM;
    }

    kernel_pagemap.top_level = (void*)(pml4_phys + hhdm_request.response->offset);

    struct limine_executable_address_response *kaddr = executable_address_request.response;
    struct limine_executable_file_response *kexec = executable_file_request.response;

    if(!verify_elf_64(kexec->executable_file->address)) {
        panic("VMM: Kernel elf file was invalid");
    }

    struct elf_header_64 *header = (struct elf_header_64 *)kexec->executable_file->address;
    struct elf_program_header_64 *prog_headers = (struct elf_program_header_64 *)((uint8_t *)kexec->executable_file->address + header->program_header_table);
    for (size_t i = 0; i < header->program_header_entries; i++) {
        struct elf_program_header_64 *ph = &prog_headers[i];
        if (ph->type != ELF_PROG_PT_LOAD_TYPE) continue;
        uint64_t flags = PTE_PRESENT | PTE_GLOBAL;
        if (ph->flags & ELF_PROG_WRITE) flags |= PTE_WRITABLE;
        if (!(ph->flags & ELF_PROG_EXEC_FLAG)) flags |= PTE_NX;

        vmm_map_pages_continuous(&kernel_pagemap, ph->virt_addr,
                                 kaddr->physical_base + ph->virt_addr - 0xffffffff80000000,
                                 (ALIGN_UP(ph->mem_size, PAGE_SIZE) / PAGE_SIZE), flags);
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    bool hypervisor = cpu_feature_bit(1, 0, 'c', CPUID_HYPERVISOR);
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uint64_t map_base   = ALIGN_DOWN(entry->base, PAGE_SIZE);
        uint64_t page_count = ALIGN_UP(entry->length, PAGE_SIZE) / 4096;
        uint64_t flags      = PTE_PRESENT | PTE_WRITABLE | PTE_NX;
        if (!hypervisor && entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            flags |= PTE_PCD | PTE_PAT;
        }

        vmm_map_pages_continuous(&kernel_pagemap, map_base + hhdm_request.response->offset, map_base, page_count, flags);
    }

    vmm_switch_to(&kernel_pagemap);

    printf("VMM: Virtual Memory Manager Setup\n");
}

pagemap_t *new_pagemap() {
    void *pml4_phys = allocate_page();
    if (pml4_phys == NULL) {
        panic("Failed to allocate new PML4 table page\n");
    }
    uint64_t *pml4_virt = (uint64_t *)((uintptr_t)pml4_phys + hhdm_request.response->offset);
    memcpy(pml4_virt, kernel_pagemap.top_level, PAGE_SIZE);

    memset(pml4_virt, 0, PAGE_SIZE / 2);

    pagemap_t *new_pagemap = malloc(sizeof(pagemap_t));
    new_pagemap->top_level = pml4_virt;
    return new_pagemap;
}
