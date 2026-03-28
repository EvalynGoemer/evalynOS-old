#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>

#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <misc/font8x8_basic.h>

#include <utils/globals.h>
#include <utils/cmdline.h>
#include <utils/panic.h>
#include <drivers/x86_64/idt.h>
#include <drivers/x86_64/gdt.h>
#include <drivers/x86_64/fred/fred.h>
#include <drivers/x86_64/pic.h>
#include <drivers/x86_64/ps2.h>
#include <drivers/x86_64/crX.h>
#include <drivers/x86_64/timers/tsc.h>
#include <drivers/x86_64/timers/pit.h>
#include <drivers/x86_64/apic/apic.h>
#include <drivers/x86_64/apic/ioapic.h>
#include <drivers/x86_64/serial.h>
#include <drivers/x86_64/cpuid.h>
#include <drivers/x86_64/irq.h>
#include <drivers/tty.h>
#include <drivers/timer.h>
#include <drivers/keyboard.h>
#include <drivers/fb_renderer.h>
#include <interupts/interupts.h>
#include <syscalls/syscalls.h>
#include <utils/safe_user_funcs.h>
#include <utils/macros.h>
#include "drivers/dbgstub/dbgstub.h"
#include <utils/cpulocal.h>

#include <acpi/acpi.h>

#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>

#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>
#include <scheduler/scheduler.h>
#include <scheduler/workers/idle.h>
#include <scheduler/workers/reaper.h>
#include <scheduler/switch.h>
#include <apps/shell.h>

void kmain() {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        __asm__ __volatile__("hlt");

    }

    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        __asm__ __volatile__("hlt");
    }

    framebuffer = framebuffer_request.response->framebuffers[0];
    FB_WIDTH = framebuffer_request.response->framebuffers[0]->width;
    FB_HEIGHT = framebuffer_request.response->framebuffers[0]->height;

    ft_ctx = flanterm_fb_init(
        NULL, NULL,
        framebuffer_request.response->framebuffers[0]->address,
        framebuffer_request.response->framebuffers[0]->width,
        framebuffer_request.response->framebuffers[0]->height,
        framebuffer_request.response->framebuffers[0]->pitch,
        framebuffer_request.response->framebuffers[0]->red_mask_size,
        framebuffer_request.response->framebuffers[0]->red_mask_shift,
        framebuffer_request.response->framebuffers[0]->green_mask_size,
        framebuffer_request.response->framebuffers[0]->green_mask_shift,
        framebuffer_request.response->framebuffers[0]->blue_mask_size,
        framebuffer_request.response->framebuffers[0]->blue_mask_shift,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        8, 8, 1, 0, 0, 0
    );

    printf("\033c\033[2J\033[H");
    printf("KERNEL: Kernel Started\n");

    printf("KERNEL: CPU Vendor: %s\n", get_cpu_vendor());
    printf("KERNEL: CPU Name: %s\n", get_cpu_name());
    if (cpu_feature_bit(1, 0, 'c', CPUID_HYPERVISOR)) {
        printf("KERNEL: Hypervisor ID: %s\n", get_hypervisor_id());
    }

    setup_cmdline();

    setup_control_registers();
    setup_gdt();

    if (!setup_fred()) {
        setup_idt();
    }

    setup_pmm();
    setup_vmm();
    setup_heap();
    valloc_init();

    setup_acpi();
    setup_irqs();
    setup_timer();
    setup_apic();

    setup_tty();
    setup_serial();
    if (dbgstub_enabled) {
        serial_works = false;
        dbgstub_init();
        DBG_LOG("Awaiting Debugger")
        asm volatile("int %0" : : "i" (0x3));
    }
    setup_ps2();
    setup_keyboard();

    init_tarfs();

    char readBuf[512];
    int status = fs_read("/test.txt", readBuf, 512);
    if (status > 0) {
        printf("KERNEL: Testing tarfs by printing \"test.txt\" from initramfs: %s", readBuf);
    }

    init_syscall();

    printf("KERNEL: Press Enter to Start the Builtin Kernel Test CLI\n");
    int enterPressed = 0;
    while (!enterPressed) {
        char keyPressed[1];
        fs_read("/dev/kbd", keyPressed, 1);
        if (keyPressed[0] == '\n' || keyPressed[0] == '\r') {
            enterPressed = 1;
        }
        timer_blocking_sleep_ms(1);
    }

    create_thread(idle_thread, NULL, NO_ARGS);
    create_thread(reaper_thread, NULL, NO_ARGS);
    create_thread(start_shell, NULL, NO_ARGS);
    shouldSchedule = 1;

    while (1) {
        CPU_LOCAL_WRITE(irq_should_preempt, true);
        asm("hlt");
    }
}
