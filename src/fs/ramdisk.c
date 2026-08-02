#include "fs/vfs.h"
#include "lib/string.h"

// Sample system configuration files
static const char* sys_config = 
    "[Falkon-OS Kernel Config]\n"
    "Version=1.0.0-Enterprise\n"
    "Arch=x86_64\n"
    "Paging=4-Level-PML4\n"
    "Scheduler=Preemptive-RoundRobin\n"
    "GUI=Bochs-VBE-LFB\n";

static const char* demo_script = 
    "#!/bin/sh\n"
    "echo Executing Falkon-OS startup script...\n"
    "sysinfo\n"
    "meminfo\n"
    "echo Startup script complete.\n";

static const char* mp4_sample =
    "[Falkon-OS Media Stream Payload]\n"
    "Container: MP4 (ISOBMFF)\n"
    "Video: H.264 High Profile @ 60 FPS\n"
    "Audio: AAC Stereo 48kHz\n";

// 64-bit ELF binary image header sample (/bin/vlc)
static const uint8_t vlc_elf_binary[] = {
    0x7F, 'E', 'L', 'F', 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Minimal Ring 3 code
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
        "Falkon-OS features FFmpeg Powered Media Player (libavcodec & libavformat),\n"
        "Dynamic Display Resolution Switching, System Call ABI,\n"
        "and Ring 3 User-Mode Execution.\n\n"
        "Enjoy exploring your new operating system!\n";

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
}
