#include "memory/pmm.h"
#include "memory/vma.h"
#include "memory/vmm.h"
#include "utils/macros.h"
#include "utils/spinlock.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <memory/heap.h>
#include <stdbool.h>

typedef struct malloc_block {
    uint64_t size;
    [[gnu::unused]] uint64_t padding;
} malloc_block_t;

spinlock_t slab_memlock;

void* malloc(size_t size) {
    if (size <= 1024) {
        int r = spinlock_lock(&slab_memlock);
        malloc_block_t *block = kmalloc(size + 16);
        block->size = size;
        spinlock_unlock(&slab_memlock, r);
        return (void*)((uint64_t)block + 16);
    } else {
        malloc_block_t *block = (void*)valloc(size + 16);
        uint64_t aligned_size = ALIGN_UP(size + 16, PAGE_SIZE);
        for (uint64_t off = 0; off < aligned_size; off += PAGE_SIZE) {
            uintptr_t pa = (uintptr_t)allocate_page();
            vmm_map_page(&kernel_pagemap, (uintptr_t)block + off, pa, PTE_WRITABLE | PTE_NX);
        }
        block->size = size;
        return (void*)((uint64_t)block + 16);
    }
}

void free (void *ptr) {
    malloc_block_t *block = (void*)((uint64_t)ptr - 16);
    if (block->size <= 1024) {
        int r = spinlock_lock(&slab_memlock);
        kfree((void*)((uint64_t)ptr - 16));
        spinlock_unlock(&slab_memlock, r);
    } else {
        uint64_t size = block->size;
        uint64_t total_size = ALIGN_UP(size + 16, PAGE_SIZE);
        for (uint64_t off = 0; off < total_size; off += PAGE_SIZE) {
            uintptr_t vaddr = (uintptr_t)block + off;
            void *phys = vmm_get_phys(&kernel_pagemap, vaddr);
            vmm_unmap_page(&kernel_pagemap, vaddr);
            if (phys) free_page(phys);
        }
        vfree((uint64_t)block, size + 16);
    }
}

void *zalloc(size_t size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0)
        return malloc(0);
    if (nmemb > SIZE_MAX / size)
        return NULL;
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    memset(ptr, 0, total);
    return ptr;
}

long long int strtoll(const char* str, char** _, int base) {
    long long int value = 0;
    bool neg = false;
    char c;

    if (str[0] == '-') {
        neg = true;
        str += 1;
    } else if (str[0] == '+') {
        str += 1;
    }

    if (base == 0) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            base = 16;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'o')) {
            base = 8;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'b')) {
            base = 2;
            str += 2;
        } else {
            base = 10;
        }
    }

    while ((c = *str++)) {
        int digit;
        if      (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;

        if (digit >= base) break;
        value = value * base + digit;
    }

    if (!neg) return value;
    else return -value;
}

unsigned long long int strtoull(const char* str, char** _, int base) {
    unsigned long long int value = 0;
    char c;

    if (base == 0) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            base = 16;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'o')) {
            base = 8;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'b')) {
            base = 2;
            str += 2;
        } else {
            base = 10;
        }
    }

    while ((c = *str++)) {
        int digit;
        if      (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;

        if (digit >= base) break;
        value = value * base + digit;
    }

    return value;
}
