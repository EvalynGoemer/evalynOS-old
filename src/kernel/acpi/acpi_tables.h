#pragma once

#include <stdint.h>
#include <stdbool.h>

struct RSDP {
    char     signature[8];
    uint8_t  checksum;
    char     oemiID[6];
    uint8_t  revision;
    uint32_t rsdtAddress;
} __attribute__ ((packed));


struct XSDP {
    char signature[8];
    uint8_t  checksum;
    char     oemiID[6];
    uint8_t  revision;
    uint32_t rsdtAddress;
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t  extendedChecksum;
    uint8_t  reserved[3];
} __attribute__ ((packed));

struct ACPISDTHeader {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  Checksum;
    char     oemID[6];
    char     oemTableID[8];
    uint32_t oemRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} __attribute__ ((packed));

struct RSDT {
    struct ACPISDTHeader header;
    uint32_t pointerSDTs[] __attribute__((packed, aligned(4)));
} __attribute__((packed));

struct XSDT {
    struct ACPISDTHeader header;
    uint64_t pointerSDTs[] __attribute__((packed, aligned(4)));
} __attribute__((packed));

#define ACPI_ADDRESS_TYPE_MMIO 0
#define ACPI_ADDRESS_TYPE_PORT_IO 1

struct ACPI_addr {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed));

struct FADT {
    struct   ACPISDTHeader h;
    uint32_t firmwareCtrl;
    uint32_t fsdt;

    uint8_t  reserved;

    uint8_t  preferredPowerManagementProfile;
    uint16_t sciInterrupt;
    uint32_t smiCommandPort;
    uint8_t  acpiEnable;
    uint8_t  acpiDisable;
    uint8_t  s4viosRequest;
    uint8_t  pstateControl;
    uint32_t pm1aEventBlock;
    uint32_t pm1bEventBlock;
    uint32_t pm1aControlBlock;
    uint32_t pm1bControlBlock;
    uint32_t pm2ControlBlock;
    uint32_t pmTimerBlock;
    uint32_t gpe0Block;
    uint32_t gpe1Block;
    uint8_t  pm1EventLength;
    uint8_t  pm1ControlLength;
    uint8_t  pm2ControlLength;
    uint8_t  pmTimerLength;
    uint8_t  gpe0Length;
    uint8_t  gpe1Length;
    uint8_t  gpe1Base;
    uint8_t  cStateControl;
    uint16_t worstC2Latency;
    uint16_t worstC3Latency;
    uint16_t flushSize;
    uint16_t flushStride;
    uint8_t  dutyOffset;
    uint8_t  dutyWidth;
    uint8_t  dayAlarm;
    uint8_t  monthAlarm;
    uint8_t  century;

    uint16_t bootArchitectureFlags;

    uint8_t  reserved2;
    uint32_t flags;

    struct ACPI_addr resetReg;

    uint8_t  resetValue;
    uint8_t  reserved3[3];

    uint64_t                xFirmwareControl;
    uint64_t                xDsdt;

    struct ACPI_addr xPM1aEventBlock;
    struct ACPI_addr xPM1bEventBlock;
    struct ACPI_addr xPM1aControlBlock;
    struct ACPI_addr xPM1bControlBlock;
    struct ACPI_addr xPM2ControlBlock;
    struct ACPI_addr xPMTimerBlock;
    struct ACPI_addr xGPE0Block;
    struct ACPI_addr xGPE1Block;
} __attribute__((packed));


struct HPET {
    struct ACPISDTHeader header;
    uint8_t hardware_rev_id;
    uint8_t comparator_count:5;
    uint8_t counter_size:1;
    uint8_t reserved:1;
    uint8_t legacy_replacement:1;
    uint16_t pci_vendor_id;
    struct ACPI_addr address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

#define MADT_TYPE_LAPIC            0
#define MADT_TYPE_IOAPIC           1
#define MADT_TYPE_ISR_OVERRIDE     2
#define MADT_TYPE_IOAPIC_NMI       3
#define MADT_TYPE_LAPIC_NMI        4
#define MADT_TYPE_LAPIC_ADDR_OVR   5
#define MADT_TYPE_PLX2APIC         9

struct MADT {
    struct ACPISDTHeader header;
    uint32_t lapic_address;
    uint32_t flags;
} __attribute__ ((packed));

struct MADTEntryHeader {
    uint8_t type;
    uint8_t length;
} __attribute__ ((packed));

struct MADT_plapic {
    uint8_t  type;
    uint8_t  length;
    uint8_t  cpu_id;
    uint8_t  apic_id;
    uint32_t flags;
} __attribute__ ((packed));

struct MADT_ioapic {
    uint8_t  type;
    uint8_t  length;
    uint8_t  ioapic_id;
    uint8_t  reserved;
    uint32_t ioapic_address;
    uint32_t gsib;
} __attribute__ ((packed));

struct MADT_isr_override {
    uint8_t  type;
    uint8_t  length;
    uint8_t  bus_source;
    uint8_t  irq_source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__ ((packed));

struct MADT_ioapic_nmi {
    uint8_t  type;
    uint8_t  length;
    uint8_t  nmi_source;
    uint8_t  reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute__ ((packed));

struct MADT_lapic_nmi {
    uint8_t  type;
    uint8_t  length;
    uint8_t  cpu_id;
    uint16_t flags;
    uint8_t  lint;
} __attribute__ ((packed));

struct MADT_lapic_addr_override {
    uint8_t  type;
    uint8_t  length;
    uint16_t reserved;
    uint64_t lapic_addr;
} __attribute__ ((packed));

struct MADT_plx2apic {
    uint8_t  type;
    uint8_t  length;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__ ((packed));

extern bool acpi_verify_rsdp();
extern void acpi_parse_tables();
