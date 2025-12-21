#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

// Kernel log buffer (ring buffer)
#define LOG_BUFFER_SIZE 4096

typedef struct {
    char buffer[LOG_BUFFER_SIZE];
    int write_pos;
    int read_pos;
} log_buffer_t;

// Initialize debug system
void debug_init();

// Kernel logging (goes to ring buffer + VGA)
void debug_log(const char* message);
void debug_logf(const char* fmt, ...);

// Dump log buffer
void debug_dump_log();

// Breakpoint (INT 3)
#define BREAKPOINT() asm volatile("int $3")

// Debug assertions
#ifndef NDEBUG
#define DEBUG_ASSERT(cond) \
    do { if (!(cond)) { \
        debug_logf("ASSERTION FAILED: %s at %s:%d", #cond, __FILE__, __LINE__); \
        BREAKPOINT(); \
    } } while(0)
#else
#define DEBUG_ASSERT(cond) do {} while(0)
#endif

// Memory validation
int debug_validate_pointer(void* ptr);
int debug_validate_string(const char* str);

// Performance profiling
void debug_profile_start(const char* name);
void debug_profile_end(const char* name);

#endif
