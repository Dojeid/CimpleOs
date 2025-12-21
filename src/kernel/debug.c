#include "debug.h"
#include "printf.h"
#include "vga.h"
#include "string.h"
#include "timer.h"
#include <stdarg.h>

static log_buffer_t kernel_log;

void debug_init() {
    memset(kernel_log.buffer, 0, LOG_BUFFER_SIZE);
    kernel_log.write_pos = 0;
    kernel_log.read_pos = 0;
    
    debug_log("[DEBUG] Debug system initialized");
}

void debug_log(const char* message) {
    // Write to ring buffer
    int len = strlen(message);
    for (int i = 0; i < len && i < LOG_BUFFER_SIZE - 1; i++) {
        kernel_log.buffer[kernel_log.write_pos] = message[i];
        kernel_log.write_pos = (kernel_log.write_pos + 1) % LOG_BUFFER_SIZE;
        
        // Overwrite oldest if full
        if (kernel_log.write_pos == kernel_log.read_pos) {
            kernel_log.read_pos = (kernel_log.read_pos + 1) % LOG_BUFFER_SIZE;
        }
    }
    
    // Newline
    kernel_log.buffer[kernel_log.write_pos] = '\n';
    kernel_log.write_pos = (kernel_log.write_pos + 1) % LOG_BUFFER_SIZE;
    
    // Also print to VGA
    vga_print(message);
    vga_print("\n");
}

void debug_logf(const char* fmt, ...) {
    char buffer[512];
    
    va_list args;
    va_start(args, fmt);
    // Use sprintf from printf.h
    vsprintf(buffer, fmt, args);
    va_end(args);
    
    debug_log(buffer);
}

void debug_dump_log() {
    printf("\n=== KERNEL LOG DUMP ===\n");
    
    int pos = kernel_log.read_pos;
    while (pos != kernel_log.write_pos) {
        if (kernel_log.buffer[pos] != 0) {
            printf("%c", kernel_log.buffer[pos]);
        }
        pos = (pos + 1) % LOG_BUFFER_SIZE;
    }
    
    printf("\n=== END LOG ===\n");
}

int debug_validate_pointer(void* ptr) {
    uint32_t addr = (uint32_t)ptr;
    
    // NULL check
    if (addr == 0) return 0;
    
    // Check if in valid kernel range (1MB - 256MB for now)
    if (addr < 0x100000 || addr > 0x10000000) return 0;
    
    return 1;
}

int debug_validate_string(const char* str) {
    if (!debug_validate_pointer((void*)str)) return 0;
    
    // Check if string is reasonable (max 4KB)
    for (int i = 0; i < 4096; i++) {
        if (str[i] == '\0') return 1;
    }
    
    return 0;  // No null terminator found
}

// Simple profiling
static struct {
    const char* name;
    uint32_t start_tick;
    uint32_t total_ticks;
    uint32_t call_count;
} profiles[32];

static int profile_count = 0;

void debug_profile_start(const char* name) {
    // Find or create profile entry
    int idx = -1;
    for (int i = 0; i < profile_count; i++) {
        if (strcmp(profiles[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1 && profile_count < 32) {
        idx = profile_count++;
        profiles[idx].name = name;
        profiles[idx].total_ticks = 0;
        profiles[idx].call_count = 0;
    }
    
    if (idx != -1) {
        profiles[idx].start_tick = timer_get_ticks();
    }
}

void debug_profile_end(const char* name) {
    uint32_t end_tick = timer_get_ticks();
    
    for (int i = 0; i < profile_count; i++) {
        if (strcmp(profiles[i].name, name) == 0) {
            profiles[i].total_ticks += (end_tick - profiles[i].start_tick);
            profiles[i].call_count++;
            return;
        }
    }
}

// Command to show profiles
void debug_show_profiles() {
    printf("\n=== PERFORMANCE PROFILES ===\n");
    for (int i = 0; i < profile_count; i++) {
        uint32_t avg = profiles[i].call_count > 0 ? 
                       profiles[i].total_ticks / profiles[i].call_count : 0;
        printf("%s: %u calls, %u total ticks, %u avg\n",
               profiles[i].name,
               profiles[i].call_count,
               profiles[i].total_ticks,
               avg);
    }
    printf("=== END PROFILES ===\n");
}
