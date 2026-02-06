#include <memory/vmm.h>
#include <memory/vma.h>
#include <utils/macros.h>
#include <stdlib.h>

RB_GENERATE(vmm_valloc_tree, vmm_page_range, node, vmm_cmp_range);

void valloc_init() {
    vmm_page_range_t *free_range = malloc(sizeof(vmm_page_range_t));
    free_range->vaddr = VALLOC_RANGE_START;
    free_range->size  = VALLOC_RANGE_END - VALLOC_RANGE_START;
    RB_INSERT(vmm_valloc_tree, &kernel_pagemap.free_ranges, free_range);
    printf("VMA: Virtual Memory Allocator Setup\n");
}

[[clang::overloadable]] uint64_t valloc(pagemap_t* pagemap, uint64_t size) {
    if (size == 0) {
        return 0;
    }

    size = ALIGN_UP(size, PAGE_SIZE);

    if (RB_EMPTY(&pagemap->free_ranges)) {
        return 0;
    }

    vmm_page_range_t *range = NULL;

    RB_FOREACH(range, vmm_valloc_tree, &pagemap->free_ranges) {
        if (range->size >= size) {
            break;
        }
    }

    if (!range || range->size < size) {
        return 0;
    }

    RB_REMOVE(vmm_valloc_tree, &pagemap->free_ranges, range);

    if (range->size == size) {
        RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, range);
        return range->vaddr;
    }

    vmm_page_range_t *split_range = malloc(sizeof(vmm_page_range_t));

    split_range->vaddr = range->vaddr;
    split_range->size  = size;

    range->vaddr += size;
    range->size  -= size;

    RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, range);
    RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, split_range);

    return split_range->vaddr;
}

[[clang::overloadable]] uint64_t valloc(pagemap_t* pagemap, uint64_t size, uint64_t fixed_addr, bool noreplace) {
    if (!IS_ALIGNED(fixed_addr, PAGE_SIZE)) {
        return 0;
    }
    size = ALIGN_UP(size, PAGE_SIZE);

    bool allocated_region = false;
    vmm_page_range_t usearch = {.vaddr = fixed_addr, .size = 0};
    vmm_page_range_t* uresult = RB_FIND(vmm_valloc_tree, &pagemap->used_ranges, &usearch);
    if (uresult)
        allocated_region = true;
    if (noreplace && allocated_region)
        return 0;
    if (uresult && uresult->size >= size)
        return fixed_addr;
    if (uresult && uresult->size < size) {
        fixed_addr += uresult->size;
        size -= uresult->size;
    }

    vmm_page_range_t fsearch = {.vaddr = fixed_addr, .size = 0};
    vmm_page_range_t* fresult = RB_FIND(vmm_valloc_tree, &pagemap->free_ranges, &fsearch);
    if (!fresult) {
        return 0;
    }

    RB_REMOVE(vmm_valloc_tree, &pagemap->free_ranges, fresult);

    if (fixed_addr == fresult->vaddr) {
        if (size == fresult->size) {
            RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, fresult);
            return fresult->vaddr;
        }
        vmm_page_range_t *split_range = malloc(sizeof(vmm_page_range_t));
        split_range->vaddr = fresult->vaddr;
        split_range->size  = size;
        fresult->vaddr += size;
        fresult->size  -= size;
        RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, fresult);
        RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, split_range);
        return split_range->vaddr;
    }

    uint64_t pre_size  = fixed_addr - fresult->vaddr;
    uint64_t post_size = fresult->size - pre_size - size;
    vmm_page_range_t* alloc_range = malloc(sizeof(vmm_page_range_t));
    alloc_range->vaddr = fixed_addr;
    alloc_range->size  = size;
    RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, alloc_range);
    if (pre_size > 0) {
        fresult->size = pre_size;
        RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, fresult);
    } else {
        free(fresult);
    }

    if (post_size > 0) {
        vmm_page_range_t* post_range = malloc(sizeof(vmm_page_range_t));
        post_range->vaddr = fixed_addr + size;
        post_range->size  = post_size;
        RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, post_range);
    }

    return fixed_addr;
}

[[clang::overloadable]] uint64_t valloc(pagemap_t* pagemap, uint64_t size, uint64_t fixed_addr) {
    return valloc(pagemap, size, fixed_addr, false);
}

uint64_t vfree(pagemap_t* pagemap, uint64_t vaddr, uint64_t size) {
    if (!IS_ALIGNED(vaddr, PAGE_SIZE)) {
        return false;
    }
    size = ALIGN_UP(size, PAGE_SIZE);

    vmm_page_range_t usearch = {.vaddr = vaddr, .size = 0};
    vmm_page_range_t* uresult = RB_FIND(vmm_valloc_tree, &pagemap->used_ranges, &usearch);
    if (!uresult)
        return false;
    // TODO: support vfree() calls that cross regions
    if ((uresult->vaddr + uresult->size) < (vaddr + size))
        return false;

    RB_REMOVE(vmm_valloc_tree, &pagemap->used_ranges, uresult);
    if (uresult->vaddr == vaddr) {
        if (uresult->size == size) {
            RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, uresult);
            return 1;
        }
        vmm_page_range_t *split_range = malloc(sizeof(vmm_page_range_t));
        split_range->vaddr = uresult->vaddr;
        split_range->size  = size;
        uresult->vaddr += size;
        uresult->size  -= size;
        RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, uresult);
        RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, split_range);
        return 2;
    }

    uint64_t pre_size  = vaddr - uresult->vaddr;
    uint64_t post_size = uresult->size - pre_size - size;
    vmm_page_range_t* alloc_range = malloc(sizeof(vmm_page_range_t));
    alloc_range->vaddr = vaddr;
    alloc_range->size  = size;
    RB_INSERT(vmm_valloc_tree, &pagemap->free_ranges, alloc_range);
    if (pre_size > 0) {
        uresult->size = pre_size;
        RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, uresult);
    } else {
        free(uresult);
    }

    if (post_size > 0) {
        vmm_page_range_t* post_range = malloc(sizeof(vmm_page_range_t));
        post_range->vaddr = vaddr + size;
        post_range->size  = post_size;
        RB_INSERT(vmm_valloc_tree, &pagemap->used_ranges, post_range);
    }

    return 3;
}
