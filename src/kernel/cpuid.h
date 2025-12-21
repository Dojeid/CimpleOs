#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>

// CPU vendor strings
#define CPUID_VENDOR_INTEL     "GenuineIntel"
#define CPUID_VENDOR_AMD       "AuthenticAMD"
#define CPUID_VENDOR_UNKNOWN   "Unknown"

// CPU feature flags (EDX from CPUID 0x1)
#define CPUID_FEAT_FPU      (1 << 0)   // Floating Point Unit
#define CPUID_FEAT_VME      (1 << 1)   // Virtual 8086 Mode
#define CPUID_FEAT_DE       (1 << 2)   // Debugging Extensions
#define CPUID_FEAT_PSE      (1 << 3)   // Page Size Extension
#define CPUID_FEAT_TSC      (1 << 4)   // Time Stamp Counter
#define CPUID_FEAT_MSR      (1 << 5)   // Model Specific Registers
#define CPUID_FEAT_PAE      (1 << 6)   // Physical Address Extension
#define CPUID_FEAT_MCE      (1 << 7)   // Machine Check Exception
#define CPUID_FEAT_CX8      (1 << 8)   // CMPXCHG8 instruction
#define CPUID_FEAT_APIC     (1 << 9)   // APIC on chip
#define CPUID_FEAT_SEP      (1 << 11)  // SYSENTER/SYSEXIT
#define CPUID_FEAT_MTRR     (1 << 12)  // Memory Type Range Registers
#define CPUID_FEAT_PGE      (1 << 13)  // Page Global Enable
#define CPUID_FEAT_MCA      (1 << 14)  // Machine Check Architecture
#define CPUID_FEAT_CMOV     (1 << 15)  // Conditional Move
#define CPUID_FEAT_PAT      (1 << 16)  // Page Attribute Table
#define CPUID_FEAT_PSE36    (1 << 17)  // 36-bit Page Size Extension
#define CPUID_FEAT_MMX      (1 << 23)  // MMX instructions
#define CPUID_FEAT_FXSR     (1 << 24)  // FXSAVE/FXRSTOR
#define CPUID_FEAT_SSE      (1 << 25)  // SSE instructions
#define CPUID_FEAT_SSE2     (1 << 26)  // SSE2 instructions

// CPU feature flags (ECX from CPUID 0x1)
#define CPUID_FEAT_ECX_SSE3    (1 << 0)   // SSE3 instructions
#define CPUID_FEAT_ECX_SSSE3   (1 << 9)   // SSSE3 instructions
#define CPUID_FEAT_ECX_SSE41   (1 << 19)  // SSE4.1 instructions
#define CPUID_FEAT_ECX_SSE42   (1 << 20)  // SSE4.2 instructions
#define CPUID_FEAT_ECX_AVX     (1 << 28)  // AVX instructions

// CPU information structure
typedef struct {
    char vendor[13];          // 12 chars + null
    char brand[49];           // 48 chars + null
    uint32_t stepping;
    uint32_t model;
    uint32_t family;
    uint32_t type;
    uint32_t features_edx;    // Feature flags from EDX
    uint32_t features_ecx;    // Feature flags from ECX
    uint32_t logical_cores;
    uint32_t physical_cores;
} cpu_info_t;

// Global CPU info
extern cpu_info_t cpu_info;

// Initialize CPU detection
void cpuid_init();

// Get CPU info
void cpuid_get_info(cpu_info_t* info);

// Check if CPUID is supported
int cpuid_is_supported();

// Print CPU features
void cpuid_print_features(uint32_t edx, uint32_t ecx);

#endif
