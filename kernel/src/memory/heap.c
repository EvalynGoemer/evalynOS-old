// Rayan Margham (Developer of Nyaux)
// Slab allocator implementation
// 2026 MIT license
// Written for evalynOS

#include <memory/pmm.h>
#include <memory/vmm.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils/globals.h>
#include <utils/panic.h>
#include <utils/spinlock.h>

struct slab_object {
    struct slab_object *next;
};
struct slab_header {
    struct slab_header *other_slabs;
    size_t obj_size;
    size_t obj_am;
    struct slab_object *objects;
};
struct slab_cache {
    size_t size;
    struct slab_header *slabs;
};

struct slab_header *new_slab(size_t obj_size) {
    struct slab_header *new_slab = allocate_page() + hhdm_request.response->offset;
    new_slab->obj_size = obj_size;
    size_t obj_am = (PAGE_SIZE - sizeof(struct slab_header)) / obj_size;
    new_slab->obj_am = obj_am;
    struct slab_object *prev_obj = ((void *)new_slab + sizeof(struct slab_header));
    for (size_t i = 1; i < obj_am; i++) {
        struct slab_object *new_ob = (((void *)new_slab + sizeof(struct slab_header)) + (i * obj_size));
        new_ob->next = prev_obj;
        prev_obj = new_ob;
    }
    new_slab->objects = prev_obj;
    return new_slab;
}
void *allocate_with_slab(struct slab_header *slab) {
    while (true) {
        if (slab->objects != NULL) {
            if (slab->objects->next != NULL) {
                struct slab_object *our_obj = slab->objects;
                slab->objects = our_obj->next;
                slab->obj_am -= 1;
                return (void *)our_obj;
            } else {
                slab->obj_am -= 1;
                struct slab_object *our_obj = slab->objects;
                slab->objects = NULL;
                return (void *)our_obj;
            }
        } else {
            if (slab->other_slabs != NULL) {
                slab = slab->other_slabs;
            } else {
                struct slab_header *old_slab = slab;
                struct slab_header *new_sl = new_slab(old_slab->obj_size);
                old_slab->other_slabs = new_sl;
                slab = new_sl;
            }
        }
    }
}
struct slab_cache init_slabcache(size_t obj_si) {
    struct slab_header *new_sl = new_slab(obj_si);
    struct slab_cache new = {.size = obj_si, .slabs = new_sl};
    return new;
}
static struct slab_cache slab_caches[6] = {0};
void setup_heap(void) {
    slab_caches[0] = init_slabcache(32);
    slab_caches[1] = init_slabcache(64);
    slab_caches[2] = init_slabcache(128);
    slab_caches[3] = init_slabcache(256);
    slab_caches[4] = init_slabcache(512);
    slab_caches[5] = init_slabcache(1024);

    printf("HEAP: Heap Setup\n");
}

void *kmalloc(size_t size) {
    for (size_t i = 0; i < 6; i++) {
        if (size <= slab_caches[i].size) {
            return allocate_with_slab(slab_caches[i].slabs);
        }
    }
    panic("no mem");
    return NULL;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    struct slab_header *hea = (struct slab_header *)((uint64_t)ptr & ~0xFFF);
    memset(ptr, 0, hea->obj_size);
    if (hea->objects == NULL) {
        hea->objects = (struct slab_object *)ptr;
        hea->obj_am += 1;
    } else {
        ((struct slab_object *)ptr)->next = hea->objects;
        hea->objects = (struct slab_object *)ptr;
        hea->obj_am += 1;
    }
}
