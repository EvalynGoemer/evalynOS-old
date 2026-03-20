#include <syscalls/syscalls.h>
#include <drivers/x86_64/cpuid.h>
#include <utils/globals.h>
#include <memory/vmm.h>
#include <scheduler/scheduler.h>

// TODO: when VFS is complete replace with with an MMAP and an IOCTL

void sys_fb_map(struct syscall_frame* frame) {
    uint32_t total_bytes = framebuffer->pitch * framebuffer->height;
    uint16_t pages = (total_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t virt_addr = 0x00000000A0000000;
    uint64_t phys_addr = ((uint64_t)framebuffer->address - hhdm_request.response->offset);
    for (uint16_t i = 0; i < pages; i++) {
        if (!cpu_feature_bit(1, 0, 'c', CPUID_HYPERVISOR)) {
            // enable WC on real hardware only
            vmm_map_page(get_current_thread()->pagemap, virt_addr, phys_addr, PTE_PRESENT | PTE_USER | PTE_WRITABLE | PTE_PCD | PTE_PAT);
        } else {
            vmm_map_page(get_current_thread()->pagemap, virt_addr, phys_addr, PTE_PRESENT | PTE_USER | PTE_WRITABLE);
        }
        virt_addr += 0x1000;
        phys_addr += 0x1000;
    }
    frame->rax = 0;
}

void sys_fb_get_pitch(struct syscall_frame* frame) {
    frame->rax = framebuffer->pitch;
}
