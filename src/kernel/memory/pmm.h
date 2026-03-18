#pragma once

#include <stdint.h>

typedef enum {
    PAGE_UNINIT = 0,
    PAGE_FREE   = 1,
    PAGE_USED   = 2,
} page_state_t;

typedef struct {
    page_state_t state;
} page_t;

typedef struct pmm_freelist_node {
    struct pmm_freelist_node* next;
} pmm_freelist_node_t;

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t num_pages;
    page_t pages[];
} pmm_child_table_t;

typedef struct {
    uint32_t num_child_tables;
    pmm_child_table_t* child_tables[];
} pmm_parent_table_t;

void  setup_pmm(void);
void* allocate_page(void);
void  free_page(void* phys);

page_t* get_page_info(void* phys_addr);
void*   get_phys_addr_from_page_info(page_t* page);
