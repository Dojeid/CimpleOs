#include "string.h"

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
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) {
        n--;
    }
    while (n--) {
        *dest++ = '\0';
    }
    return ret;
}

char* strcat(char* dest, const char* src) {
    char* rd = dest;
    while (*rd) rd++;
    while ((*rd++ = *src++));
    return dest;
}

void itoa(int value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);

    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';

    // Reverse
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

int atoi(const char* str) {
    if (!str) return 0;
    int res = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t' || *str == '\n') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') { str++; }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char *p1 = (const unsigned char*)s1;
    const unsigned char *p2 = (const unsigned char*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

void* memset(void* dest, int val, size_t count) {
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

void* memcpy(void* dest, const void* src, size_t n) {
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
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if ((char)c == '\0') return (char*)s;
    return (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack, *n = needle;
            while (*h && *n && *h == *n) { h++; n++; }
            if (!*n) return (char*)haystack;
        }
    }
    return 0;
}

static char* strtok_saved = 0;

char* strtok(char* str, const char* delim) {
    char* s = str ? str : strtok_saved;
    if (!s) return 0;

    while (*s && strchr(delim, *s)) s++;
    if (!*s) {
        strtok_saved = 0;
        return 0;
    }

    char* token_start = s;
    while (*s && !strchr(delim, *s)) s++;

    if (*s) {
        *s = 0;
        strtok_saved = s + 1;
    } else {
        strtok_saved = 0;
    }

    return token_start;
}
