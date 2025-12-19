#ifndef PRINTF_H
#define PRINTF_H

// Formatted printing to screen
void printf(const char* fmt, ...);
void sprintf(char* buf, const char* fmt, ...);

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
