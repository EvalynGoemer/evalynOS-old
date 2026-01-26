#pragma once

#include <stdio.h> // IWYU pragma: keep
#include <utils/cmdline.h>

#define ALIGN_UP(x, align) ((((uint64_t) (x)) + ((align) - 1)) & ~((uint64_t) ((align) - 1)))
#define ALIGN_DOWN(x, align) (((uint64_t) (x)) & ~((uint64_t) ((align) - 1)))

#define LOG(fmt, ...) \
    do { \
        if (dbg_enabled) \
            printf(fmt "\n", ##__VA_ARGS__); \
    } while (0);

#define DBG_LOG(fmt, ...) \
    do { \
        if (dbg_enabled) \
            printf("DBG: " fmt "\n", ##__VA_ARGS__); \
    } while (0);
