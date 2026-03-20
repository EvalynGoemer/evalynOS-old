#include <stdint.h>
#include <utils/panic.h>

uint64_t __stack_chk_guard = 0xdeafbeef69420bad;

__attribute__((noreturn))
void __stack_chk_fail(void) {
    panic("SSP: Stack check failed");
}
