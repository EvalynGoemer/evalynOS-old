#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <utils/globals.h>

#define PAGE_SIZE       0x1000
#define LARGE_PAGE_SIZE 0x200000
#define JUMBO_PAGE_SIZE 0x40000000

#define PTE_MASK 0x000ffffffffff000

#define PTE_PRESENT   0x1
#define PTE_WRITABLE  0x2
#define PTE_USER      0x4
#define PTE_PWT       0x8
#define PTE_PCD       0x10
#define PTE_ACCESSED  0x20
#define PTE_DIRTY     0x40
#define PTE_PS        0x80  // only on PML2/3 entries
#define PTE_PAT       0x80  // only on PML1 entries
#define PTE_GLOBAL    0x100
#define PTE_PERM      0x200 // OS spesific; Do not free this entry;
#define PTE_COW       0x400 // OS spesific; Copy on write page; Look at pfndb for more details on page;
#define PTE_AVL       0x800 // OS spesific; Unused OS Spesific bit;
#define PTE_NX        0x8000000000000000

typedef struct pagemap {
    uint64_t *top_level;
} pagemap_t;

extern pagemap_t kernel_pagemap;

extern void setup_vmm();
extern void vmm_switch_to(pagemap_t *pagemap);
extern void vmm_map_page(pagemap_t *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);
extern pagemap_t *new_pagemap();
