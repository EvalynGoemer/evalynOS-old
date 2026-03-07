#include <drivers/x86_64/cpuid.h>
#include <drivers/x86_64/serial.h>
#include <utils/globals.h>
#include <utils/cmdline.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

bool dbg_enabled = false;
bool dbgstub_enabled = false;

void setup_cmdline() {
    char *cmdline = executable_cmdline_request.response->cmdline;
    char *arg;

    if (!cmdline) {
        return;
    }

    while (*cmdline == ' ') {
        cmdline++;
    }

    while (*cmdline) {
        arg = cmdline;

        while (*cmdline && *cmdline != ' ') {
            cmdline++;
        }

        if (*cmdline) {
            *cmdline = '\0';
            cmdline++;
        }

        while (*cmdline == ' ') {
            cmdline++;
        }

        for(int i = 0; arg[i]; i++){
            arg[i] = tolower(arg[i]);
        }
        if (strcmp("serial=hv", arg) == 0) {
            if (cpu_feature_bit(CPUID_GET_FEATURES, CPUID_NO_SUBLEAF, CPUID_ECX, CPUID_HYPERVISOR)) {
                serial_enabled = true;
            }
            continue;
        }
        if (strcmp("serial=on", arg) == 0) {
            serial_enabled = true;
            continue;
        }
        if (strncmp("serial=", arg, 7) == 0) {
            arg += 7;
            serial_enabled = true;
            serial_port = strtoll(arg, NULL, 0);
            continue;
        }

        if (strcmp("dbg=on", arg) == 0) {
            dbg_enabled = true;
            continue;
        }
        if (strcmp("debug=on", arg) == 0) {
            dbg_enabled = true;
            continue;
        }

        if (strcmp("dbg=stub", arg) == 0) {
            dbg_enabled = true;
            dbgstub_enabled = true;
            continue;
        }
        if (strcmp("debug=stub", arg) == 0) {
            dbgstub_enabled = true;
            continue;
        }
    }
}
