#include <stdint.h>
#include <drivers/x86_64/drX.h>

uint64_t hbreakpoints[4] = {0};

bool drx_alloc_bp(uint64_t addr) {
    if (addr == 0) return false;
    for (int i = 0; i < 4; i++) {
        if (hbreakpoints[i] == addr) return false;
        if (hbreakpoints[i] == 0) {
            hbreakpoints[i] = addr;
            drx_setbp(i, addr);
            drx_enablebp(i);
            return true;
        }
    }
    return false;
}

bool drx_free_bp(uint64_t addr) {
    if (addr == 0) return false;
    for (int i = 0; i < 4; i++) {
        if (hbreakpoints[i] == addr) {
            hbreakpoints[i] = 0;
            drx_setbp(i, 0);
            drx_enablebp(i);
            return true;
        }
    }
    return false;
}
