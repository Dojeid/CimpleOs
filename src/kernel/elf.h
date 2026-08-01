#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

#define ELF_MAGIC 0x464C457F // \x7fELF

// ELF Identification Indices
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6

#define ELFCLASS32    1
#define ELFCLASS64    2

#define ELFDATA2LSB   1 // 2's complement, little endian
#define ELFDATA2MSB   2 // 2's complement, big endian

#define ET_EXEC       2 // Executable file
#define ET_DYN        3 // Shared object file

#define PT_LOAD       1 // Loadable program segment
#define PF_X          1 // Execute
#define PF_W          2 // Write
#define PF_R          4 // Read

// 64-bit ELF Header
typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

// 64-bit Program Header
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

// 32-bit ELF Header
typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

// 32-bit Program Header
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

int elf_validate(const uint8_t* buffer, size_t size);
int elf_load_and_run(const char* filepath, char* const argv[], char* const envp[]);

#endif // ELF_H
