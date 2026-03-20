#pragma once

#include <stdio.h> // IWYU pragma: keep
#include <utils/cmdline.h>

#define ALIGN_UP(x, align) ((((uint64_t) (x)) + ((align) - 1)) & ~((uint64_t) ((align) - 1)))
#define ALIGN_DOWN(x, align) (((uint64_t) (x)) & ~((uint64_t) ((align) - 1)))
#define IS_ALIGNED(x, align) (((uint64_t)(x) & ((align) - 1)) == 0)

#define LOG(fmt, ...) \
    do { \
            printf(fmt "\n", ##__VA_ARGS__); \
    } while (0);

#define DBG_LOG(fmt, ...) \
    do { \
        if (dbg_enabled) \
            printf("\x1b[93mDBG: " fmt "\x1b[0m\n", ##__VA_ARGS__); \
    } while (0);
