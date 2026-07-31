#include "fs/ramdisk.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

void ramdisk_init(void) {
    vfs_node_t* root = vfs_get_root();

    vfs_node_t* docs = vfs_mkdir(root, "docs");
    vfs_node_t* sys  = vfs_mkdir(root, "sys");
    vfs_node_t* bin  = vfs_mkdir(root, "bin");

    const char* welcome_msg = 
        "Welcome to Falkon-OS v1.0 Enterprise!\n"
        "====================================\n"
        "Falkon-OS features a custom 64-bit Long Mode Kernel,\n"
        "Virtual File System (VFS), Desktop Window Manager,\n"
        "Process Scheduler, and System Call Subsystem.\n\n"
        "Enjoy exploring your new operating system!\n";

    const char* sys_config = 
        "OS_NAME=Falkon-OS\n"
        "VERSION=1.0.0\n"
        "ARCH=x86_64\n"
        "BOOTLOADER=Custom-ElTorito\n"
        "GUI_MODE=1024x768x32\n";

    const char* demo_script = 
        "echo Starting Falkon-OS System Services...\n"
        "fetch\n"
        "meminfo\n"
        "ps\n";

    if (docs) {
        vfs_create_file(docs, "welcome.txt", (const uint8_t*)welcome_msg, strlen(welcome_msg));
    }
    if (sys) {
        vfs_create_file(sys, "os_info.cfg", (const uint8_t*)sys_config, strlen(sys_config));
    }
    if (bin) {
        vfs_create_file(bin, "demo.sh", (const uint8_t*)demo_script, strlen(demo_script));
    }

    vga_print("[Ramdisk] In-memory system files pre-populated (/docs, /sys, /bin).\n");
}
