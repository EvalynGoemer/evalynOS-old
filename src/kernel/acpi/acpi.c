#include <stdbool.h>
#include <acpi/acpi_tables.h>
#include <stdio.h>

bool setup_acpi() {
    bool status;
    status = acpi_verify_rsdp();

    if(status == false) {
        printf("ACPI: Unable to setup ACPI\n");
        return false;
    }

    acpi_parse_tables();

    return true;
}
