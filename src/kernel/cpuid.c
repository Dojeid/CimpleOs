#include "cpuid.h"
#include "lib/printf.h"
#include "lib/string.h"

cpu_info_t cpu_info;

// Execute CPUID instruction
static inline void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code)
    );
}

// Check if CPUID is supported
int cpuid_is_supported() {
    uint64_t rax, rdx;
    
    // Try to flip ID bit (bit 21) in RFLAGS
    // In 64-bit mode: use pushfq/popfq and 64-bit registers
    asm volatile(
        "pushfq\n"
        "pop %%rax\n"
        "mov %%rax, %%rdx\n"
        "xor $0x200000, %%rax\n"
        "push %%rax\n"
        "popfq\n"
        "pushfq\n"
        "pop %%rax\n"
        "xor %%rdx, %%rax\n"
        "and $0x200000, %%rax\n"
        : "=a"(rax), "=d"(rdx)
        :
        : "cc"
    );
    
    return rax != 0;
}

// Get CPU vendor string
static void cpuid_get_vendor(char* vendor) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    // Vendor string is in EBX, EDX, ECX order
    *(uint32_t*)(vendor + 0) = ebx;
    *(uint32_t*)(vendor + 4) = edx;
    *(uint32_t*)(vendor + 8) = ecx;
    vendor[12] = '\0';
}

// Get CPU brand string
static void cpuid_get_brand(char* brand) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check if extended functions are supported
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) {
        strcpy(brand, "Unknown");
        return;
    }
    
    // Brand string is in 3 calls: 0x80000002, 0x80000003, 0x80000004
    cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)(brand + 0) = eax;
    *(uint32_t*)(brand + 4) = ebx;
    *(uint32_t*)(brand + 8) = ecx;
    *(uint32_t*)(brand + 12) = edx;
    
    cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)(brand + 16) = eax;
    *(uint32_t*)(brand + 20) = ebx;
    *(uint32_t*)(brand + 24) = ecx;
    *(uint32_t*)(brand + 28) = edx;
    
    cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)(brand + 32) = eax;
    *(uint32_t*)(brand + 36) = ebx;
    *(uint32_t*)(brand + 40) = ecx;
    *(uint32_t*)(brand + 44) = edx;
    
    brand[48] = '\0';
    
    // Trim leading spaces
    char* start = brand;
    while (*start == ' ') start++;
    if (start != brand) {
        int len = strlen(start);
        for (int i = 0; i <= len; i++) {
            brand[i] = start[i];
        }
    }
}

// Get CPU features
static void cpuid_get_features(uint32_t* feat_edx, uint32_t* feat_ecx) {
    uint32_t eax, ebx;
    cpuid(1, &eax, &ebx, feat_ecx, feat_edx);
}

// Get CPU signature (family, model, stepping)
static void cpuid_get_signature(uint32_t* family, uint32_t* model, uint32_t* stepping) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    *stepping = eax & 0xF;
    *model = (eax >> 4) & 0xF;
    *family = (eax >> 8) & 0xF;
    
    // Extended family and model
    if (*family == 0xF) {
        *family += (eax >> 20) & 0xFF;
    }
    if (*family == 0x6 || *family == 0xF) {
        *model += ((eax >> 16) & 0xF) << 4;
    }
}

// Initialize CPUID
void cpuid_init() {
    memset(&cpu_info, 0, sizeof(cpu_info_t));
    
    if (!cpuid_is_supported()) {
        strcpy(cpu_info.vendor, "NoCPUID");
        strcpy(cpu_info.brand, "CPUID not supported");
        return;
    }
    
    // Get vendor
    cpuid_get_vendor(cpu_info.vendor);
    
    // Get brand
    cpuid_get_brand(cpu_info.brand);
    
    // Get signature
    cpuid_get_signature(&cpu_info.family, &cpu_info.model, &cpu_info.stepping);
    
    // Get features
    cpuid_get_features(&cpu_info.features_edx, &cpu_info.features_ecx);
    
    // Try to get core count (simplified)
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    cpu_info.logical_cores = (ebx >> 16) & 0xFF;
    
    // For now, assume physical = logical (proper detection needs APIC)
    cpu_info.physical_cores = cpu_info.logical_cores;
}

// Get CPU info
void cpuid_get_info(cpu_info_t* info) {
    memcpy(info, &cpu_info, sizeof(cpu_info_t));
}

// Print CPU features
void cpuid_print_features(uint32_t edx, uint32_t ecx) {
    printf("  Features: ");
    
    if (edx & CPUID_FEAT_FPU)   printf("FPU ");
    if (edx & CPUID_FEAT_VME)   printf("VME ");
    if (edx & CPUID_FEAT_DE)    printf("DE ");
    if (edx & CPUID_FEAT_PSE)   printf("PSE ");
    if (edx & CPUID_FEAT_TSC)   printf("TSC ");
    if (edx & CPUID_FEAT_MSR)   printf("MSR ");
    if (edx & CPUID_FEAT_PAE)   printf("PAE ");
    if (edx & CPUID_FEAT_MCE)   printf("MCE ");
    if (edx & CPUID_FEAT_CX8)   printf("CX8 ");
    if (edx & CPUID_FEAT_APIC)  printf("APIC ");
    if (edx & CPUID_FEAT_SEP)   printf("SEP ");
    if (edx & CPUID_FEAT_MTRR)  printf("MTRR ");
    if (edx & CPUID_FEAT_PGE)   printf("PGE ");
    if (edx & CPUID_FEAT_MCA)   printf("MCA ");
    if (edx & CPUID_FEAT_CMOV)  printf("CMOV ");
    if (edx & CPUID_FEAT_MMX)   printf("MMX ");
    if (edx & CPUID_FEAT_SSE)   printf("SSE ");
    if (edx & CPUID_FEAT_SSE2)  printf("SSE2 ");
    
    if (ecx & CPUID_FEAT_ECX_SSE3)   printf("SSE3 ");
    if (ecx & CPUID_FEAT_ECX_SSSE3)  printf("SSSE3 ");
    if (ecx & CPUID_FEAT_ECX_SSE41)  printf("SSE4.1 ");
    if (ecx & CPUID_FEAT_ECX_SSE42)  printf("SSE4.2 ");
    if (ecx & CPUID_FEAT_ECX_AVX)    printf("AVX ");
    
    printf("\n");
}
