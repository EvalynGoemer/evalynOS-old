#include "memory/pmm.h"
#include <stdlib.h>
#include "limine.h"
#include "stdbool.h"
#include "stddef.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils/panic.h>
#include <utils/globals.h>
#include <utils/spinlock.h>

// TODO
// - Turn into buddy allocator

spinlock_t pmm_spinlock = {0};

#define PAGE_SIZE 4096
#define ALIGN_UP(x, align) ((((uintptr_t) (x)) + ((align) - 1)) & ~((uintptr_t) ((align) - 1)))
#define ALIGN_DOWN(x, align) (((uintptr_t) (x)) & ~((uintptr_t) ((align) - 1)))

uint64_t hhdmOffset = 0;
struct limine_memmap_response* memmap = NULL;

pmm_freelist_node_t* freelist_head = NULL;

uint64_t parent_table_size;
pmm_parent_table_t* parent_table = NULL;

uint64_t total_pages = 0;
uint64_t used_pages = 0;

page_t* get_page_info(void* phys_addr) {
    for (uint32_t i = 0; parent_table->num_child_tables > i; i++) {
        if ((uint64_t)phys_addr >= parent_table->child_tables[i]->start &&
            (uint64_t)phys_addr <= parent_table->child_tables[i]->end) {
                uint64_t index = (ALIGN_DOWN(phys_addr, PAGE_SIZE) - parent_table->child_tables[i]->start) / PAGE_SIZE;

                if (index >= parent_table->child_tables[i]->num_pages) {
                    printf("PMM: bad index : %lu\n", index);
                    panic("PMM: attempted to get info on out of bounds page that was meant to be in range");
                }

                return &parent_table->child_tables[i]->pages[index];
        }
    }
    return NULL;
}

void* get_phys_addr_from_page_info(page_t* page) {
    for (uint32_t i = 0; i < parent_table->num_child_tables; i++) {
        pmm_child_table_t* sub = parent_table->child_tables[i];

        if (page >= &sub->pages[0] && page < &sub->pages[sub->num_pages]) {
            uint64_t index = page - &sub->pages[0];
            return (void*)(sub->start + (index * PAGE_SIZE));
        }
    }
    return NULL;
}

uint16_t balloc_current_page = 0;
void*    balloc_current_page_ptr = 0;
uint16_t balloc_current_byte = 0;
void* bootstrap_balloc(uint16_t size) {
    if (size > PAGE_SIZE) {
        panic("PMM: Bootstrap allocator allocation too large");
    }

    if (balloc_current_byte + size > PAGE_SIZE) {
        balloc_current_page++;
        balloc_current_byte = 0;
    }

    if (balloc_current_page >= parent_table->child_tables[0]->num_pages) {
        panic("PMM: Out of memory [bootstrap]");
    }

    parent_table->child_tables[0]->pages[balloc_current_page].used = true;
    uint8_t* page_ptr = (uint8_t*)get_phys_addr_from_page_info(&parent_table->child_tables[0]->pages[balloc_current_page]);

    void* ptr = page_ptr + balloc_current_byte;
    balloc_current_byte += size;
    return ptr + hhdmOffset;
}

void setup_pmm() {
    hhdmOffset = hhdm_request.response->offset;
    memmap = memmap_request.response;

    parent_table_size = sizeof(pmm_parent_table_t);
    uint64_t num_usable_regions = 0;
    for (uint64_t i = 0; memmap->entry_count > i; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap->entries[i]->length > (PAGE_SIZE * 16)) {
            num_usable_regions++;
            parent_table_size += sizeof(pmm_child_table_t*);
        }
    }

    for (uint64_t i = 0; memmap->entry_count > i; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap->entries[i]->length > (PAGE_SIZE * 16)) {
            parent_table = (void*)ALIGN_UP(memmap->entries[i]->base, PAGE_SIZE) + hhdmOffset;
            memset(parent_table, 0, parent_table_size);
            parent_table->num_child_tables = num_usable_regions;
            break;
        }
    }

    if (parent_table == NULL) {
        panic("PMM: Could not find place for parent table");
    }

    uint64_t j = 0;
    for (uint64_t i = 0; memmap->entry_count > i; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap->entries[i]->length > (PAGE_SIZE * 16)) {
            uint64_t num_pages = (memmap->entries[i]->length / PAGE_SIZE) + 1;
            total_pages += num_pages;

            uint64_t child_table_size = sizeof(pmm_child_table_t) + sizeof(page_t) * num_pages;
            pmm_child_table_t* child_table;
            if ((void*)ALIGN_UP(memmap->entries[i]->base, PAGE_SIZE) + hhdmOffset == parent_table) {
                child_table = (void*)ALIGN_UP(memmap->entries[i]->base, PAGE_SIZE) + hhdmOffset + parent_table_size;
            } else {
                child_table = (void*)ALIGN_UP(memmap->entries[i]->base, PAGE_SIZE) + hhdmOffset;
            }

            memset(child_table, 0, child_table_size);

            child_table->start = ALIGN_UP((uint64_t)child_table - hhdmOffset + child_table_size + 1, PAGE_SIZE);
            child_table->end = ALIGN_DOWN(memmap->entries[i]->base + memmap->entries[i]->length, PAGE_SIZE);
            child_table->num_pages = (child_table->end - child_table->start) / PAGE_SIZE;

            parent_table->child_tables[j] = (void*)child_table;
            j++;
        }
    }

    freelist_head = bootstrap_balloc(sizeof(pmm_freelist_node_t));
    freelist_head->start = parent_table->child_tables[0]->start + (balloc_current_page + 1 * PAGE_SIZE);
    freelist_head->end = parent_table->child_tables[0]->end;
    freelist_head->next = NULL;
    freelist_head->bootstrap = true;
    pmm_freelist_node_t* current_node = freelist_head;
    for (uint32_t i = 1; parent_table->num_child_tables > i; i++) {
        current_node->next = bootstrap_balloc(sizeof(pmm_freelist_node_t));
        current_node->next->start = parent_table->child_tables[i]->start;
        current_node->next->end = parent_table->child_tables[i]->end;
        current_node->next->bootstrap = true;
        current_node = current_node->next;
    }

    used_pages += balloc_current_page;
    printf("PMM: Found %lu MiB of usable memory\n", ((total_pages - used_pages) * PAGE_SIZE) >> 20);
    printf("PMM: Physical Memory Manager Setup\n");
}

void *allocate_page() {
    if (!freelist_head)
        panic("PMM: Out of memory");

    if (used_pages >= total_pages) {
        panic("PMM: Out of memory");
    }

    int lock1r = spinlock_lock(&pmm_spinlock);

    used_pages++;

    pmm_freelist_node_t *node = freelist_head;
    uint64_t phys_addr = node->start;

    page_t* page_info = get_page_info((void*)phys_addr);
    if (page_info->used) {
        panic("PMM: Attempted to allocate used page");
    }
    page_info->used = true;

    if (page_info->clean == false) {
        memset((void*)(uintptr_t)(phys_addr + hhdmOffset), 0, PAGE_SIZE);
    }

    node->start += PAGE_SIZE;
    if (node->start == node->end) {
        if (node->bootstrap == false) {
            pmm_freelist_node_t *old_node = node;
            freelist_head = node->next;
            free(old_node);
        } else {
            freelist_head = node->next;
        }
    }

    spinlock_unlock(&pmm_spinlock, lock1r);

    return (void*)(uintptr_t)phys_addr;
}

void free_page(void *page) {
    uint64_t phys_addr = (uint64_t)page;
    page_t *info = get_page_info((void *)phys_addr);

    if (!info)
        return;

    if (!info->used)
        panic("PMM: Double free");

    int lock1r = spinlock_lock(&pmm_spinlock);

    if (info->ref_count > 1) {
        info->ref_count--;
        spinlock_unlock(&pmm_spinlock, lock1r);
        return;
    }

    used_pages--;

    info->used = false;
    info->clean = false;
    info->ref_count = 0;

    if (freelist_head) {
        if (phys_addr + PAGE_SIZE == freelist_head->start) {
            freelist_head->start = phys_addr;
            spinlock_unlock(&pmm_spinlock, lock1r);
            return;
        }

        if (freelist_head->end == phys_addr) {
            freelist_head->end += PAGE_SIZE;
            spinlock_unlock(&pmm_spinlock, lock1r);
            return;
        }
    }

    pmm_freelist_node_t *node = malloc(sizeof(pmm_freelist_node_t));
    node->start = phys_addr;
    node->end = phys_addr + PAGE_SIZE;
    node->next = freelist_head;
    freelist_head = node;
    spinlock_unlock(&pmm_spinlock, lock1r);
}
