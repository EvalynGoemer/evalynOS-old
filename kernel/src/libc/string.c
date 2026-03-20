#include <stdint.h>
#include <stddef.h>

void* memcpy(void* dst, const void* src, size_t n) {
    void* tmp = dst;
    asm volatile( "rep movsb" : "+D"(dst), "+S"(src), "+c"(n) : : "memory" );
    return tmp;
}

void* memset(void* dst, int c, size_t n) {
    void* tmp = dst;
    asm volatile("rep stosb" : "+D"(dst), "+c"(n) : "a"(c) : "memory");
    return tmp;
}

// The following two functions were taken from https://codeberg.org/Limine/limine-c-template/raw/commit/c8bc5a2b93397a19272a19a6004b0eeb1e90d982/kernel/src/main.c
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, unsigned long n) {
    unsigned long i = 0;

    while (i < n && s1[i] && (s1[i] == s2[i])) {
        i++;
    }

    if (i == n) {
        return 0;
    }

    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

// Taken from https://stackoverflow.com/a/20190538
char *strcpy(char *strDest, const char *strSrc) {
    char *temp = strDest;
    while ((*strDest++ = *strSrc++) != '\0');
    return temp;
}


size_t strlen(const char *str) {
    const char *s = str;
    while (*s) {
        s++;
    }
    return s - str;
}

char *strchr(const char *s, char c) {
    while (*s != '\0') {
        if (*s == c)
            return (char*)s;
        s++;
    }

    if (c == '\0')
        return (char*)s;

    return NULL;
}
