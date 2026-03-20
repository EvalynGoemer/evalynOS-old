#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils/globals.h>
#include <utils/panic.h>
#include <utils/macros.h>
#include <utils/spinlock.h>
#include <limine.h>
#include <memory/pmm.h>

// TODO
// - Turn into buddy allocator

spinlock_t pmm_spinlock = {0};

#define PAGE_SIZE 4096

uint64_t hhdmOffset = 0;
struct limine_memmap_response* memmap = NULL;

pmm_freelist_node_t* freelist_head = NULL;

uint64_t parent_table_size;
pmm_parent_table_t* parent_table = NULL;

uint64_t total_pages = 0;
uint64_t used_pages = 0;

page_t* get_page_info(void* phys_addr) {
    for (uint32_t i = 0; parent_table->num_child_tables > i; i++) {
        if ((uint64_t)phys_addr >= parent_table->child_tables[i]->start && (uint64_t)phys_addr <= parent_table->child_tables[i]->end) {
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

static void pmm_fill(size_t pages) {
    static uint32_t fill_t = 0;
    static uint64_t fill_p = 0;
    size_t pages_added = 0;
    while (fill_t < parent_table->num_child_tables && pages_added < pages) {
        pmm_child_table_t* child = parent_table->child_tables[fill_t];
        while (fill_p < child->num_pages && pages_added < pages) {
            uintptr_t page_addr = child->start + PAGE_SIZE * fill_p;
            page_t* info = &child->pages[fill_p];
            if (info->state == PAGE_UNINIT) {
                info->state = PAGE_FREE;
                pmm_freelist_node_t* node = (pmm_freelist_node_t*)(page_addr + hhdmOffset);
                node->next = freelist_head;
                freelist_head = node;
                pages_added++;
            }
            fill_p++;
        }

        if (fill_p >= child->num_pages) {
            fill_t++;
            fill_p = 0;
        }
    }
}

void setup_pmm() {
    hhdmOffset = hhdm_request.response->offset;
    memmap = memmap_request.response;

    parent_table_size = sizeof(pmm_parent_table_t);
    uint64_t num_usable_regions = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap->entries[i]->length > PAGE_SIZE * 16) {
            num_usable_regions++;
            parent_table_size += sizeof(pmm_child_table_t*);
        }
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap->entries[i]->length > PAGE_SIZE * 16) {
            parent_table = (pmm_parent_table_t*)(ALIGN_UP(memmap->entries[i]->base, PAGE_SIZE) + hhdmOffset);
            memset(parent_table, 0, parent_table_size);
            parent_table->num_child_tables = num_usable_regions;
            break;
        }
    }

    if (parent_table == NULL) panic("PMM: could not find space for parent table");

    uint64_t j = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE || entry->length <= PAGE_SIZE * 16) continue;

        uint64_t num_pages = entry->length / PAGE_SIZE;
        uint64_t child_table_size = sizeof(pmm_child_table_t) + sizeof(page_t) * num_pages;

        pmm_child_table_t* child;
        if (ALIGN_UP(entry->base, PAGE_SIZE) + hhdmOffset == (uintptr_t)parent_table) {
            child = (pmm_child_table_t*)((uintptr_t)parent_table + parent_table_size);
        } else {
            child = (pmm_child_table_t*)(ALIGN_UP(entry->base, PAGE_SIZE) + hhdmOffset);
        }

        memset(child, 0, child_table_size);

        child->start = ALIGN_UP((uint64_t)child - hhdmOffset + child_table_size, PAGE_SIZE);
        child->end = ALIGN_DOWN(entry->base + entry->length, PAGE_SIZE);

        if (child->end <= child->start) {
            parent_table->num_child_tables--;
            continue;
        }

        child->num_pages = (child->end - child->start) / PAGE_SIZE;
        total_pages += child->num_pages;
        parent_table->child_tables[j] = child;
        j++;
    }

    pmm_fill(512);

    printf("PMM: %lu MiB usable memory detected\n", (total_pages * PAGE_SIZE) >> 20);
    printf("PMM: Physical Memory Manager Setup\n");
}

void* allocate_page() {
    int lock1r = spinlock_lock(&pmm_spinlock);

    if (freelist_head == NULL) pmm_fill(512);
    if (freelist_head == NULL) panic("PMM: out of memory");

    pmm_freelist_node_t* node = freelist_head;
    freelist_head = node->next;
    void* phys = (void*)((uint64_t)node - hhdmOffset);

    page_t* info = get_page_info(phys);
    if (info == NULL) panic("PMM: attempted to allocate page not in PFNdb");
    if (info->state == PAGE_USED) panic("PMM: attempted to allocate used page");

    info->state = PAGE_USED;
    used_pages++;

    memset(node, 0, PAGE_SIZE);

    spinlock_unlock(&pmm_spinlock, lock1r);
    return phys;
}

void free_page(void* phys) {
    int lock1r = spinlock_lock(&pmm_spinlock);
    page_t* info = get_page_info(phys);
    if (info == NULL) {
        spinlock_unlock(&pmm_spinlock, lock1r);
        return;
    }

    if (info->state == PAGE_UNINIT) panic("PMM: attempted to free an uninit page");
    if (info->state == PAGE_FREE) panic("PMM: attempted to double free");

    info->state = PAGE_FREE;
    used_pages--;

    pmm_freelist_node_t* node = (pmm_freelist_node_t*)((uint64_t)phys + hhdmOffset);
    node->next = freelist_head;
    freelist_head = node;

    spinlock_unlock(&pmm_spinlock, lock1r);
}
