#include <stdint.h>

#include <interupts/pit.h>
#include <string.h>
#include <syscalls/syscalls.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <drivers/x86_64/pit.h>
#include <drivers/x86_64/pcskpr.h>
#include <drivers/x86_64/msr.h>
#include <drivers/x86_64/cpuid.h>
#include <scheduler/scheduler.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

void init_syscall() {
    // enable syscall instruction
    uint64_t efer = rdmsr(EFER);
    efer |= (1 << 0);
    wrmsr(EFER, efer);

    uint64_t star = ((uint64_t)(0x18 | 3) << 48) | ((uint64_t)0x08 << 32);
    wrmsr(STAR, star);

    wrmsr(LSTAR, (uint64_t)syscall_handler);
    wrmsr(SFMASK, ~0x2);
}

void execute_syscall(struct syscall_frame* frame) {
    switch (frame->rax) {
        // get thread id
        case 0:
            frame->rax = get_current_thread()->threadId;
            break;
        // HACK: THIS IS REALLY UNSAFE
        // print (set to panic for debugging)
        case 1:
            printf("%s", (char*)frame->rbx);
            frame->rax = 0;
            break;
        // write to FSBASE
        case 2:
            if (frame->rbx <= 0x00007FFFFFFFFFFF) {
                wrmsr(FSBAS, frame->rbx);
                frame->rax = 0;
            } else {
                frame->rax = -1;
            }
            break;
        // WARNING: INCOMPLETE
        // MMAP
        case 3:
            asm volatile ("nop");
            frame->rax = get_current_thread()->heap_pos;
            uint32_t heapPages = ALIGN_UP(frame->rbx, PAGE_SIZE) / PAGE_SIZE;
            for (uint32_t i = 0; i < heapPages; i++) {
                    vmm_map_page(get_current_thread()->pagemap, get_current_thread()->heap_pos, (uint64_t)allocate_page(), PTE_PRESENT | PTE_USER | PTE_WRITABLE | PTE_NX);
                    get_current_thread()->heap_pos += PAGE_SIZE;
            }
            break;
        // open
        case 4:
            asm volatile ("nop");
            int size = tarfsGetFize((char*)frame->rbx);

            printf("opening file %s of size %x\n", (char*)frame->rbx, size);

            if (size == -1) {
                frame->rax = -1;
                break;
            }

            struct fd* ofile = malloc(sizeof(struct fd));
            ofile->file_data = malloc(size);
            fs_read((char*)frame->rbx, ofile->file_data, size);
            ofile->seek_pos = 0;
            ofile->file_name = malloc(strlen((char*)frame->rbx));
            strcpy(ofile->file_name, (char*)frame->rbx);
            get_current_thread()->fds[get_current_thread()->next_fd] = *ofile;
            frame->rax = get_current_thread()->next_fd++;
            break;
        // read
        case 5:
            asm volatile ("nop");

            struct fd rfile = get_current_thread()->fds[frame->rbx];

            if(rfile.file_name == NULL) {
                frame->rax = -1;
                break;
            }

            uint64_t rsize = tarfsGetFize(rfile.file_name);

            if (rfile.file_data == NULL) {
                frame->rax = -1;
                break;
            }

            if (rfile.seek_pos >= rsize) {
                frame->rax = 0;
                break;
            }

            size_t max_bytes = rsize - rfile.seek_pos;
            size_t bytes = frame->rdx > max_bytes ? max_bytes : frame->rdx;

            memcpy((void*)frame->rsi, rfile.file_data + rfile.seek_pos, bytes);
            rfile.seek_pos += bytes;
            get_current_thread()->fds[frame->rbx] = rfile;

            // printf("read %lx bytes from %s at offset %lx\n", bytes, rfile.file_name, rfile.seek_pos - bytes);

            frame->rax = bytes;
            break;
        // seek
        case 6:
            asm volatile ("nop");

            long offset = frame->rdx;
            int whence = frame->rsi;

            struct fd sfile = get_current_thread()->fds[frame->rbx];

            if(sfile.file_name == NULL) {
                frame->rax = -1;
                break;
            }

            int ssize = tarfsGetFize(sfile.file_name);

            long new_pos;

            if (whence == 0) {
                new_pos = offset;
            } else if (whence == 1) {
                new_pos = sfile.seek_pos + offset;
            } else if (whence == 2) {
                new_pos = ssize + offset;
            } else {
                frame->rax = -1;
                break;
            }

            if (new_pos < 0)
                new_pos = 0;
        if (new_pos > ssize)
            new_pos = ssize;

        sfile.seek_pos = new_pos;

        get_current_thread()->fds[frame->rbx] = sfile;

        // printf("fd: %lx file name: %s seeking to: %lx\n", frame->rbx, sfile.file_name, new_pos);

        frame->rax = new_pos;
        break;
        // sleep ms
        case 7:
            get_current_thread()->sleep_awake_time = pitInteruptsTriggered + frame->rbx;
            schedule();
            break;
        // play sound
        case 10:
            play_sound(frame->rbx);
            frame->rax = 0;
            break;
        // stop sound
        case 11:
            stop_sound();
            frame->rax = 0;
            break;
        // get pit cycles
        case 20:
            setup_pit(frame->rbx);
            frame->rax = 0;
            break;
        // reset pit cycles
        case 21:
            pitInteruptsTriggered = 0;
            frame->rax = 0;
            break;
        // get pit cycles
        case 22:
            frame->rax = pitInteruptsTriggered;
            break;
        // map framebuffer to 0x00000000A0000000 as write combining
        case 30:
            asm volatile ("nop");
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
            break;
        // get framebuffer pitch
        case 31:
            frame->rax = framebuffer->pitch;
            break;
        // get last ps/2 scancode
        case 40:
            asm volatile ("nop");
            char keyPressed[1] = {'\0'};
            int read = fs_read("/dev/ps2/kbd", keyPressed, 1);
            if (read == 0) {
                frame->rax = 0;
            } else {
                frame->rax = keyPressed[0];
            }
            break;
        default:
            frame->rax = -1;
            break;
    }
}
