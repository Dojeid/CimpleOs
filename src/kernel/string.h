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

// Integer to ASCII (decimal)
void itoa(int value, char* str, int base);

// Memory set
void* memset(void* dest, int val, size_t count);

#endif
