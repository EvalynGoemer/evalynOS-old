#include "memory/vma.h"
#include <stdint.h>
#include <syscalls/syscalls.h>
#include <utils/globals.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <scheduler/scheduler.h>

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

// TODO: make complete
// currently only bump allocates a "heap" for annon pages; this will not work for much
void sys_mmap(struct syscall_frame* frame) {
    uint64_t allocPos = valloc(get_current_thread()->pagemap, frame->rbx);
    frame->rax = allocPos;
    uint32_t heapPages = ALIGN_UP(frame->rbx, PAGE_SIZE) / PAGE_SIZE;
    for (uint32_t i = 0; i < heapPages; i++) {
        vmm_map_page(get_current_thread()->pagemap, allocPos, (uint64_t)allocate_page(), PTE_PRESENT | PTE_USER | PTE_WRITABLE | PTE_NX);
        allocPos += PAGE_SIZE;
    }
}
