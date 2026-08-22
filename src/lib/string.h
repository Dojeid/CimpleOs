#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String length
size_t strlen(const char* str);

// String compare
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

// String copy & concatenation
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);

// Integer to ASCII & ASCII to Integer
void itoa(int value, char* str, int base);
int  atoi(const char* str);

// Memory operations
int   memcmp(const void* s1, const void* s2, size_t n);
void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
char* strtok(char* str, const char* delim);

#endif
