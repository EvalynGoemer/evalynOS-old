#include "stdbool.h"
#include <acpi/acpi_tables.h>
#include <acpi/acpi_timer.h>
#include <stdint.h>
#include <stdio.h>
#include <utils/globals.h>
#include <utils/mmio.h>
#include <stdlib.h>
#include <drivers/x86_64/apic/ioapic.h>
#include <drivers/x86_64/timers/hpet.h>
#include <memory/vmm.h>
#include <string.h>

uint32_t ioapic_count = 0;

bool acpi_verify_rsdp() {
    if (!rsdp_request.response->address) {
        printf("ACPI: No RSDP Found\n");
        return false;
    }

    struct RSDP* rsdp = (struct RSDP*)rsdp_request.response->address;
    printf("ACPI: RSDP found at %p rev %d\n", rsdp, rsdp->revision);
    uint64_t checksumV1 = 0;
    uint8_t *bytes = (uint8_t *)rsdp;
    for (size_t i = 0; i < sizeof(struct RSDP); i++) {
        checksumV1 += bytes[i];
    }

    if ((uint8_t)checksumV1 != 0) {
        printf("ACPI: RSDP ChecksumV1 is bad\n");
    }

    if (rsdp->revision != 2) {
        return true;
    }

    uint64_t checksumV2 = 0;
    for (size_t i = 0; i < sizeof(struct XSDP); i++) {
        checksumV2 += bytes[i];
    }

    if ((uint8_t)checksumV2 != 0) {
        printf("ACPI: RSDP ChecksumV2 is bad\n");
    }

    return true;
}

void* acpi_find_sdt_rsdt(char* signature) {
    struct RSDP* rsdp = (struct RSDP*)rsdp_request.response->address;
    struct RSDT *rsdt = (struct RSDT *) (rsdp->rsdtAddress + hhdm_request.response->offset);
    int entries = (rsdt->header.length - sizeof(rsdt->header)) / 8;

    for (int i = 0; i < entries; i++) {
        struct ACPISDTHeader *header = (struct ACPISDTHeader *) (rsdt->pointerSDTs[i] + hhdm_request.response->offset);
        if (!strncmp(header->signature, signature, 4)) {
            return (void *) header;
        }
    }

    return NULL;
}

void* acpi_find_sdt_xsdt(char* signature) {
    struct XSDP* xsdp = (struct XSDP*)rsdp_request.response->address;
    struct XSDT *xsdt = (struct XSDT *) (xsdp->xsdtAddress + hhdm_request.response->offset);
    int entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

    for (int i = 0; i < entries; i++) {
        struct ACPISDTHeader *header = (struct ACPISDTHeader *) (xsdt->pointerSDTs[i] + hhdm_request.response->offset);
        if (!strncmp(header->signature, signature, 4)) {
            return (void *) header;
        }
    }

    return NULL;
}

void *acpi_find_sdt(char* signature) {
    struct RSDP* rsdp = (struct RSDP*)rsdp_request.response->address;
    if(rsdp->revision < 2) {
        return acpi_find_sdt_rsdt(signature);
    } else {
        return acpi_find_sdt_xsdt(signature);
    }
}

void acpi_parse_fadt() {
    struct FADT* fadt = acpi_find_sdt("FACP");

    if (fadt) {
        if (fadt->pmTimerLength != 4) {
            printf("ACPI: No ACPI Timer found???\n");
        }

        if (fadt->xPMTimerBlock.address != 0) {
            if (fadt->xPMTimerBlock.address_space_id == ACPI_ADDRESS_TYPE_MMIO) {
                acpi_timer_address = fadt->xPMTimerBlock.address + hhdm_request.response->offset;
                vmm_map_page(&kernel_pagemap, acpi_timer_address, fadt->xPMTimerBlock.address,
                             PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
                vmm_map_page(&kernel_pagemap, acpi_timer_address + PAGE_SIZE, fadt->xPMTimerBlock.address + PAGE_SIZE,
                             PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
                acpi_timer_address_type = ACPI_ADDRESS_TYPE_MMIO;
                printf("ACPI: Found ACPI Timer at %08lx\n", fadt->xPMTimerBlock.address);
            } else if (fadt->xPMTimerBlock.address_space_id == ACPI_ADDRESS_TYPE_PORT_IO) {
                acpi_timer_address = fadt->xPMTimerBlock.address;
                acpi_timer_address_type = ACPI_ADDRESS_TYPE_PORT_IO;
                printf("ACPI: Found ACPI Timer on I/O port 0x%lx\n", fadt->xPMTimerBlock.address);
            } else {
                printf("ACPI: ACPI Timer on bad address space type %d\n",fadt->xPMTimerBlock.address_space_id);
            }
        } else {
            acpi_timer_address = fadt->pmTimerBlock;
            acpi_timer_address_type = ACPI_ADDRESS_TYPE_PORT_IO;
            printf("ACPI: Found ACPI Timer on I/O port 0x%x\n", fadt->pmTimerBlock);
        }
    } else {
        printf("ACPI: No FADT Table found???\n");
    }
}

void acpi_parse_hpet() {
    struct HPET* hpet = acpi_find_sdt("HPET");

    if (hpet) {
        hpet_address = hpet->address.address + hhdm_request.response->offset;
        vmm_map_page(&kernel_pagemap, hpet_address, hpet->address.address,
                     PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
        vmm_map_page(&kernel_pagemap, hpet_address + PAGE_SIZE, hpet->address.address + PAGE_SIZE,
                     PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
        printf("ACPI: Found HPET at 0x%08lx\n", hpet->address.address);
    }
}

void acpi_parse_madt() {
    uint8_t* madt_base = acpi_find_sdt("APIC");

    if (!madt_base) {
        return;
    }

    struct MADT* madt = (struct MADT*)madt_base;
    uint8_t* ptr = madt_base + sizeof(struct MADT);
    uint8_t* end = madt_base + madt->header.length;

    while (ptr + sizeof(struct MADTEntryHeader) <= end) {
        struct MADTEntryHeader* entry = (struct MADTEntryHeader*)ptr;
        if (ptr + entry->length > end) {
            break;
        }

        switch (entry->type) {
            case MADT_TYPE_LAPIC: {
                struct MADT_plapic* lapic = (struct MADT_plapic*)ptr;
                (void)lapic;
                break;
            }
            case MADT_TYPE_IOAPIC: {
                struct MADT_ioapic* ioapic = (struct MADT_ioapic*)ptr;
                uint64_t ioapic_address = ioapic->ioapic_address + hhdm_request.response->offset;
                vmm_map_page(&kernel_pagemap, ioapic_address, ioapic->ioapic_address,
                             PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);
                vmm_map_page(&kernel_pagemap, ioapic_address + PAGE_SIZE,
                             ioapic->ioapic_address + PAGE_SIZE, PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT);

                struct ioapic_t* ioapic_struct = malloc(sizeof(struct ioapic_t));
                ioapic_struct->address = ioapic_address;
                ioapic_struct->gsib = ioapic->gsib;
                ioapic_struct->next = NULL;

                if (ioapic_head == NULL) {
                    ioapic_head = ioapic_struct;
                    ioapic_tail = ioapic_struct;
                } else {
                    ioapic_tail->next = ioapic_struct;
                    ioapic_tail = ioapic_struct;
                }

                ioapic_count++;

                printf("ACPI: Found IOAPIC at 0x%08x\n", ioapic->ioapic_address);
                break;
            }
            case MADT_TYPE_ISR_OVERRIDE: {
                struct MADT_isr_override* isr_overide = (struct MADT_isr_override*)ptr;
                if (isr_overide->irq_source != isr_overide->gsi) {
                    struct ioapic_irq_overide_t* ioapic_irq_overide = malloc(sizeof(struct ioapic_irq_overide_t));
                    ioapic_irq_overide->irq = isr_overide->irq_source;
                    ioapic_irq_overide->gsi = isr_overide->gsi;
                    ioapic_irq_overide->next = NULL;

                    if (ioapic_irq_overide_head == NULL) {
                        ioapic_irq_overide_head = ioapic_irq_overide;
                        ioapic_irq_overide_tail = ioapic_irq_overide;
                    } else {
                        ioapic_irq_overide_tail->next = ioapic_irq_overide;
                        ioapic_irq_overide_tail = ioapic_irq_overide;
                    }

                    printf("ACPI: Found IRQ Overide for irq %u to gsi %u\n", isr_overide->irq_source, isr_overide->gsi);
                }
                break;
            }
            case MADT_TYPE_IOAPIC_NMI: {
                struct MADT_ioapic_nmi* nmi = (struct MADT_ioapic_nmi*)ptr;
                (void)nmi;
                break;
            }
            case MADT_TYPE_LAPIC_NMI: {
                struct MADT_lapic_nmi* nmi = (struct MADT_lapic_nmi*)ptr;
                (void)nmi;
                break;
            }
            case MADT_TYPE_LAPIC_ADDR_OVR: {
                struct MADT_lapic_addr_override* addr = (struct MADT_lapic_addr_override*)ptr;
                (void)addr;
                break;
            }
            case MADT_TYPE_PLX2APIC: {
                struct MADT_plx2apic* x2apic = (struct MADT_plx2apic*)ptr;
                (void)x2apic;
                break;
            }
            default:
                printf("ACPI: Unknown MADT entry: %d (Len: %d)\n", entry->type, entry->length);
                break;
        }

        ptr += entry->length;
    }
}

void acpi_parse_tables() {
    acpi_parse_fadt();
    acpi_parse_madt();
    acpi_parse_hpet();

    printf("ACPI: Tables Parsed\n");
}
