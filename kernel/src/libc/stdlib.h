#pragma once

#include <stddef.h>

extern void* malloc(size_t size);
extern void *zalloc(size_t size);
extern void *calloc(size_t nmemb, size_t size);
extern void free (void *ptr);

extern long long int strtoll(const char* str, char** _, int base);
extern unsigned long long int strtoull(const char* str, char** _, int base);

