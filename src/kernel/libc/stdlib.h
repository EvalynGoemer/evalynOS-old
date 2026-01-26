#pragma once

#include <stddef.h>

void* malloc(size_t size);
void free (void *ptr);

extern long long int strtoll(const char* str, char** _, int base);
