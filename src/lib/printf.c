#include "printf.h"
#include "drivers/video/vga.h"
#include "gui/terminal.h"
#include "string.h"
#include <stdarg.h>
#include <stdint.h>

// Helper to print a 64-bit number
static void print_num64(char* buf, int* pos, uint64_t num, int base, int uppercase) {
    char digits_lower[] = "0123456789abcdef";
    char digits_upper[] = "0123456789ABCDEF";
    char* digits = uppercase ? digits_upper : digits_lower;
    
    char temp[64];
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

// Core formatting function (64-bit compatible)
static void do_printf(char* output, const char* fmt, va_list args) {
    int pos = 0;
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            
            // Parse flags/width: '-' (left-align), '0' (zero-pad), digits (width)
            int is_long = 0;
            int left = 0, zero = 0, width = 0;
            for (;;) {
                if (*fmt == '-') { left = 1; fmt++; }
                else if (*fmt == '0') { zero = 1; fmt++; }
                else if (*fmt >= '1' && *fmt <= '9') {
                    width = width * 10 + (*fmt - '0');
                    fmt++;
                } else {
                    break;
                }
            }
            
            // Check for 'l' modifier (e.g. %lx, %lX, %ld)
            if (*fmt == 'l') {
                is_long = 1;
                fmt++;
            }

            int field_start = pos;
            switch (*fmt) {
                case 'd':
                case 'i': {
                    if (is_long) {
                        int64_t num = va_arg(args, int64_t);
                        if (num < 0) {
                            output[pos++] = '-';
                            num = -num;
                        }
                        print_num64(output, &pos, (uint64_t)num, 10, 0);
                    } else {
                        int num = va_arg(args, int);
                        if (num < 0) {
                            output[pos++] = '-';
                            num = -num;
                        }
                        print_num64(output, &pos, (uint64_t)num, 10, 0);
                    }
                    break;
                }
                case 'u': {
                    uint64_t num = is_long ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num64(output, &pos, num, 10, 0);
                    break;
                }
                case 'x': {
                    uint64_t num = is_long ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num64(output, &pos, num, 16, 0);
                    break;
                }
                case 'X': {
                    uint64_t num = is_long ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num64(output, &pos, num, 16, 1);
                    break;
                }
                case 'p': {
                    output[pos++] = '0';
                    output[pos++] = 'x';
                    uintptr_t ptr = (uintptr_t)va_arg(args, void*);
                    print_num64(output, &pos, (uint64_t)ptr, 16, 1);
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
            
            // Apply width padding (left-align appends spaces, else pad at the front)
            int field_len = pos - field_start;
            if (width > field_len) {
                int pad_count = width - field_len;
                if (left) {
                    for (int i = 0; i < pad_count; i++) {
                        output[pos++] = ' ';
                    }
                } else {
                    char padc = zero ? '0' : ' ';
                    for (int i = pos - 1; i >= field_start; i--) {
                        output[i + pad_count] = output[i];
                    }
                    for (int i = 0; i < pad_count; i++) {
                        output[field_start + i] = padc;
                    }
                    pos += pad_count;
                }
            }
        } else {
            output[pos++] = *fmt;
        }
        fmt++;
    }
    
    output[pos] = '\0';
}

// Route formatted output to the active (or default) GUI terminal when
// available, falling back to VGA text mode during early boot.
static void print_output(const char* buffer) {
    extern terminal_instance_t* active_terminal;
    terminal_instance_t* dest = active_terminal ? active_terminal : terminal_get_state();
    if (dest) {
        terminal_instance_print(dest, buffer);
    } else {
        vga_print(buffer);
    }
}

void printf(const char* fmt, ...) {
    char buffer[512];  // Reduced from 1024 - saves stack space
    va_list args;
    va_start(args, fmt);
    do_printf(buffer, fmt, args);
    va_end(args);
    
    print_output(buffer);
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
    
    print_output(final);
}
