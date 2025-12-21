#include "printf.h"
#include "vga.h"
#include "string.h"
#include <stdarg.h>
#include <stdint.h>

// Helper to print a number
static void print_num(char* buf, int* pos, unsigned int num, int base, int uppercase) {
    char digits_lower[] = "0123456789abcdef";
    char digits_upper[] = "0123456789ABCDEF";
    char* digits = uppercase ? digits_upper : digits_lower;
    
    char temp[32];
    int i = 0;
    
    if (num == 0) {
        temp[i++] = '0';
    } else {
        while (num > 0) {
            temp[i++] = digits[num % base];
            num /= base;
        }
    }
    
    // Reverse
    while (i > 0) {
        buf[(*pos)++] = temp[--i];
    }
}

// Core formatting function
static void do_printf(char* output, const char* fmt, va_list args) {
    int pos = 0;
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    if (num < 0) {
                        output[pos++] = '-';
                        num = -num;
                    }
                    print_num(output, &pos, num, 10, 0);
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_num(output, &pos, num, 10, 0);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_num(output, &pos, num, 16, 0);
                    break;
                }
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_num(output, &pos, num, 16, 1);
                    break;
                }
                case 'p': {
                    output[pos++] = '0';
                    output[pos++] = 'x';
                    void* ptr = va_arg(args, void*);
                    print_num(output, &pos, (unsigned int)ptr, 16, 0);
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    if (!str) str = "(null)";
                    while (*str) {
                        output[pos++] = *str++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    output[pos++] = c;
                    break;
                }
                case '%': {
                    output[pos++] = '%';
                    break;
                }
                default:
                    output[pos++] = '%';
                    output[pos++] = *fmt;
                    break;
            }
        } else {
            output[pos++] = *fmt;
        }
        fmt++;
    }
    
    output[pos] = '\0';
}

void printf(const char* fmt, ...) {
    char buffer[512];  // Reduced from 1024 - saves stack space
    va_list args;
    va_start(args, fmt);
    do_printf(buffer, fmt, args);
    va_end(args);
    
    vga_print(buffer);
}

void sprintf(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    do_printf(buf, fmt, args);
    va_end(args);
}

void vsprintf(char* buf, const char* fmt, va_list args) {
    do_printf(buf, fmt, args);
}

// Logging with levels
void log_printf(enum log_level level, const char* fmt, ...) {
    char buffer[1024];
    char final[1200];
    
    const char* level_str[] = {
        "[DEBUG] ",
        "[INFO]  ",
        "[WARN]  ",
        "[ERROR] ",
        "[FATAL] "
    };
    
    va_list args;
    va_start(args, fmt);
    do_printf(buffer, fmt, args);
    va_end(args);
    
    // Combine level + message
    strcpy(final, level_str[level]);
    int len = strlen(final);
    strcpy(final + len, buffer);
    
    vga_print(final);
}
