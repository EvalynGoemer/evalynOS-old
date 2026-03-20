#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <obsd-tree.h>

/*
 * Kernel Virtual Address Map
 * HHDM   0xFFFF800000000000 - 0xFFFF900000000000 (16TB)
 * VALLOC 0xFFFFA00000000000 - 0xFFFFE00000000000 (64TB)
 * HEAP   0xFFFFF00000000000 - .................. (????)
 * KRNL   0xFFFFFFFF80000000 - 0xFFFFFFFFFFFFFFFF (02GB)
 */

#define VALLOC_RANGE_START 0xFFFFA00000000000
#define VALLOC_RANGE_END   0xFFFFE00000000000

typedef struct vmm_page_range {
    uint64_t vaddr;
    uint64_t size;
    uint8_t type;
    RB_ENTRY(vmm_page_range) node;
} vmm_page_range_t;

#define in_range(ra,rb,x) (((x) >= (ra)) && ((x) < (rb)))
inline static int vmm_cmp_range(const vmm_page_range_t* left, const vmm_page_range_t* right) {
    if (in_range(right->vaddr, right->vaddr + right->size, left->vaddr))
        return 0;
    else if (left->vaddr < right->vaddr)
        return -1;
    else if (left->vaddr > right->vaddr)
        return 1;
    else
        return 0;
}
#undef in_range

RB_HEAD(vmm_valloc_tree, vmm_page_range);
RB_PROTOTYPE(vmm_valloc_tree, vmm_page_range, node, vmm_cmp_range);
typedef struct vmm_valloc_tree vmm_valloc_tree_t;

typedef struct pagemap {
    uint64_t *top_level;
    vmm_valloc_tree_t free_ranges;
    vmm_valloc_tree_t used_ranges;
} pagemap_t;

extern void valloc_init();
[[clang::overloadable]] extern uint64_t valloc(uint64_t size);
[[clang::overloadable]] extern uint64_t valloc(uint64_t size, uint64_t fixed_addr);
[[clang::overloadable]] extern uint64_t valloc(pagemap_t* pagemap, uint64_t size);
[[clang::overloadable]] extern uint64_t valloc(pagemap_t* pagemap, uint64_t size, uint64_t fixed_addr);
[[clang::overloadable]] extern uint64_t valloc(pagemap_t* pagemap, uint64_t size, uint64_t fixed_addr, bool noreplace);
[[clang::overloadable]] extern uint64_t vfree(pagemap_t* pagemap, uint64_t vaddr, uint64_t size);
[[clang::overloadable]] extern uint64_t vfree(uint64_t vaddr, uint64_t size);
