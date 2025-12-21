#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

// Formatted printing to screen
void printf(const char* fmt, ...);
void sprintf(char* buf, const char* fmt, ...);
void vsprintf(char* buf, const char* fmt, va_list args);  // Added for debug.c

// Logging levels
enum log_level {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

void log_printf(enum log_level level, const char* fmt, ...);

#endif
