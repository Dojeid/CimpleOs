#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String length
size_t strlen(const char* str);

// String compare
int strcmp(const char* s1, const char* s2);

// String copy
char* strcpy(char* dest, const char* src);

// Integer to ASCII
void itoa(int value, char* str, int base);

// Memory operations
void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t n);

#endif
