#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool verify_elf_64(void* file);
extern uint64_t load_elf(void* file);
