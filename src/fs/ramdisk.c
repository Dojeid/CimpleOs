#include "fs/ramdisk.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

// Pre-compiled 64-bit ELF Executable for /bin/vlc
static const uint8_t vlc_elf_binary[] = {
    // 1. ELF64 Header (56 bytes)
    0x7F, 'E', 'L', 'F', 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_ident
    0x02, 0x00, // e_type (ET_EXEC = 2)
    0x3E, 0x00, // e_machine (x86_64 = 0x3E)
    0x01, 0x00, 0x00, 0x00, // e_version (1)
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // e_entry (0x400000)
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_phoff (64)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_shoff (0)
    0x00, 0x00, 0x00, 0x00, // e_flags
    0x40, 0x00, // e_ehsize (64)
    0x38, 0x00, // e_phentsize (56)
    0x01, 0x00, // e_phnum (1)
    0x40, 0x00, // e_shentsize (64)
    0x00, 0x00, // e_shnum (0)
    0x00, 0x00, // e_shstrndx

    // 2. Program Header 0 (56 bytes): PT_LOAD segment
    0x01, 0x00, 0x00, 0x00, // p_type (PT_LOAD = 1)
    0x07, 0x00, 0x00, 0x00, // p_flags (R/W/X = 7)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_offset (0)
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // p_vaddr (0x400000)
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // p_paddr (0x400000)
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_filesz (128 bytes)
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_memsz (128 bytes)
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_align (4096)

    // 3. User Code Payload at 0x400000 (VLC Entry Point)
    0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1
    0xC3                          // ret
};

void ramdisk_init(void) {
    vfs_node_t* root = vfs_get_root();

    vfs_node_t* docs   = vfs_mkdir(root, "docs");
    vfs_node_t* sys    = vfs_mkdir(root, "sys");
    vfs_node_t* bin    = vfs_mkdir(root, "bin");
    vfs_node_t* videos = vfs_mkdir(root, "videos");

    const char* welcome_msg = 
        "Welcome to Falkon-OS v1.0 Enterprise!\n"
        "====================================\n"
        "Falkon-OS features ELF Executable Loading (Ring 3 User Mode),\n"
        "Dynamic Display Resolution Switching, System Call ABI,\n"
        "and VLC User-Mode Binary Execution.\n\n"
        "Enjoy exploring your new operating system!\n";

    const char* sys_config = 
        "OS_NAME=Falkon-OS\n"
        "VERSION=1.0.0\n"
        "ARCH=x86_64\n"
        "BOOTLOADER=Custom-ElTorito\n"
        "GUI_MODE=1024x768x32\n";

    const char* demo_script = 
        "echo Starting Falkon-OS System Services...\n"
        "vlc /videos/sample.mp4\n"
        "fetch\n"
        "meminfo\n"
        "ps\n";

    const char* mp4_sample = "FALKON_MP4_HEADER_VIDEO_STREAM_DATA_60FPS";

    if (docs) {
        vfs_create_file(docs, "welcome.txt", (const uint8_t*)welcome_msg, strlen(welcome_msg));
    }
    if (sys) {
        vfs_create_file(sys, "os_info.cfg", (const uint8_t*)sys_config, strlen(sys_config));
    }
    if (bin) {
        vfs_create_file(bin, "demo.sh", (const uint8_t*)demo_script, strlen(demo_script));
        vfs_create_file(bin, "vlc", vlc_elf_binary, sizeof(vlc_elf_binary));
    }
    if (videos) {
        vfs_create_file(videos, "sample.mp4", (const uint8_t*)mp4_sample, strlen(mp4_sample));
    }

    vga_print("[Ramdisk] In-memory system files pre-populated (/docs, /sys, /bin, /videos).\n");
}
