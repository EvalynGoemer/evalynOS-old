#include <stddef.h>
#include <string.h>
#include <memory/heap.h>
#include <stdbool.h>

void* malloc(size_t size) {
    return kmalloc(size);
}
void *zalloc(size_t size) {
    void *ptr = kmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void free (void *ptr) {
    kfree(ptr);
}

long long int strtoll(const char* str, char** _, int base) {
    long long int value = 0;
    bool neg = false;
    char c;

    if (str[0] == '-') {
        neg = true;
        str += 1;
    } else if (str[0] == '+') {
        str += 1;
    }

    if (base == 0) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            base = 16;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'o')) {
            base = 8;
            str += 2;
        } else if (str[0] == '0' && (str[1] == 'b')) {
            base = 2;
            str += 2;
        } else {
            base = 10;
        }
    }

    while ((c = *str++)) {
        int digit;
        if      (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;

        if (digit >= base) break;
        value = value * base + digit;
    }

    if (!neg) return value;
    else return -value;
}
