#include "lib/string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* orig = dest;
    while ((*dest++ = *src++));
    return orig;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* orig = dest;
    while (n > 0 && *src) {
        *dest++ = *src++;
        n--;
    }
    while (n > 0) {
        *dest++ = '\0';
        n--;
    }
    return orig;
}

char* strcat(char* dest, const char* src) {
    char* orig = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return orig;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* orig = dest;
    while (*dest) dest++;
    while (n-- && *src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return orig;
}

__attribute__((weak)) void* memset(void* dest, int val, size_t count) {
    uint8_t v8 = (uint8_t)val;
    uint64_t v64 = ((uint64_t)v8 << 56) | ((uint64_t)v8 << 48) |
                   ((uint64_t)v8 << 40) | ((uint64_t)v8 << 32) |
                   ((uint64_t)v8 << 24) | ((uint64_t)v8 << 16) |
                   ((uint64_t)v8 << 8)  | (uint64_t)v8;

    uint64_t* d64 = (uint64_t*)dest;
    size_t qwords = count / 8;
    size_t remainder = count % 8;

    while (qwords--) {
        *d64++ = v64;
    }

    unsigned char* d8 = (unsigned char*)d64;
    while (remainder--) {
        *d8++ = v8;
    }

    return dest;
}

__attribute__((weak)) void* memcpy(void* dest, const void* src, size_t n) {
    uint64_t* d64 = (uint64_t*)dest;
    const uint64_t* s64 = (const uint64_t*)src;
    size_t qwords = n / 8;
    size_t remainder = n % 8;

    while (qwords--) {
        *d64++ = *s64++;
    }

    unsigned char* d8 = (unsigned char*)d64;
    const unsigned char* s8 = (const unsigned char*)s64;
    while (remainder--) {
        *d8++ = *s8++;
    }

    return dest;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s++) return 0;
    }
    return (char*)s;
}

char* strrchr(const char* s, int c) {
    const char* last = 0;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char*)last;
}

__attribute__((weak)) char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char* h = haystack;
            const char* n = needle;
            while (*h && *n && *h == *n) {
                h++;
                n++;
            }
            if (!*n) return (char*)haystack;
        }
    }
    return 0;
}

__attribute__((weak)) int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

__attribute__((weak)) void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}
