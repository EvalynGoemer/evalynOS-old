#pragma once

#include <stdint.h>
void setup_pmm();
void *allocate_page();
void free_page(void *page);

typedef struct page {
    uint16_t ref_count;
    uint8_t clean:    1;
    uint8_t used:     1;
    uint8_t cow:      1;
} page_t;

typedef struct pmm_child_table {
    uint64_t num_pages;
    uint64_t start;
    uint64_t end;
    page_t pages[];
} pmm_child_table_t;

typedef struct pmm_parent_table {
    uint32_t num_child_tables;
    pmm_child_table_t* child_tables[];
} pmm_parent_table_t;

typedef struct pmm_freelist_node {
    struct pmm_freelist_node* next;
    uint64_t start;
    uint64_t end;
    uint8_t bootstrap: 1;
} pmm_freelist_node_t;




