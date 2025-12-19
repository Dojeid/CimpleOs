#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

// Panic with detailed error information
void kernel_panic(const char* message, uint32_t error_code);

// Exception names
extern const char* exception_messages[32];

// Assert macro
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            kernel_panic("Assertion failed: " #condition " at " __FILE__ ":" __LINE__, 0); \
        } \
    } while(0)

#endif
