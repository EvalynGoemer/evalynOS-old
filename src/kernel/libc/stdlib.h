#pragma once

#include <stddef.h>

extern void* malloc(size_t size);
extern void *zalloc(size_t size);
extern void free (void *ptr);

extern long long int strtoll(const char* str, char** _, int base);
