#!/usr/bin/env python3
"""
Falkon-OS Unified Build & VirtualBox Management System
======================================================
This single cross-platform script handles dependency verification, 
kernel compilation, ISO creation, VirtualBox VM setup & launch, and QEMU execution.

Usage:
  python build.py          -> Build FalkonOS.iso
  python build.py --vbox   -> Build ISO and run in VirtualBox (Recommended)
  python build.py --qemu   -> Build ISO and run in QEMU emulator
  python build.py --clean  -> Remove build artifacts
  python build.py --check  -> Check required toolchain tools
"""

import os
import sys
import shutil
import platform
import subprocess
import argparse

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
ISO_DIR = os.path.join(PROJECT_ROOT, "isodir")
SRC_DIR = os.path.join(PROJECT_ROOT, "src")
ISO_PATH = os.path.join(PROJECT_ROOT, "FalkonOS.iso")
KERNEL_BIN = os.path.join(BUILD_DIR, "FalkonOS.bin")

REQUIRED_TOOLS = ["nasm", "gcc", "ld", "grub-mkrescue"]

def log_step(msg):
    print(f"\n\033[1;34m[Falkon-OS]\033[0m \033[1m{msg}\033[0m")

def log_success(msg):
    print(f"\033[1;32m[✓]\033[0m {msg}")

def log_warning(msg):
    print(f"\033[1;33m[!]\033[0m {msg}")

def log_error(msg):
    print(f"\033[1;31m[ERROR]\033[0m {msg}")

def check_dependencies():
    """Validates that required compiler tools are installed and in PATH."""
    log_step("Verifying Build Dependencies...")
    missing = []
    
    for tool in REQUIRED_TOOLS:
        if shutil.which(tool) is None:
            missing.append(tool)
            
    if missing:
        log_error(f"Missing required build tools: {', '.join(missing)}")
        print("\nPlease install missing packages for your operating system:")
        print("  Windows: Install WSL2 (Ubuntu) or MSYS2 + NASM + GCC + GRUB")
        print("  Ubuntu/Debian: sudo apt update && sudo apt install -y build-essential nasm grub-pc-bin grub-common xorriso mtools")
        print("  Fedora:        sudo dnf install gcc nasm grub2-tools-extra xorriso mtools")
        print("  Arch Linux:    sudo pacman -S base-devel nasm grub xorriso mtools")
        return False
        
    log_success("All core tools found (gcc, nasm, ld, grub-mkrescue).")
    return True

def find_vboxmanage():
    """Finds VBoxManage executable location on Windows, Linux, or macOS."""
    if shutil.which("VBoxManage"):
        return "VBoxManage"
    if shutil.which("vboxmanage"):
        return "vboxmanage"
        
    # Standard Windows install path check
    if platform.system() == "Windows":
        standard_path = r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
        if os.path.exists(standard_path):
            return f'"{standard_path}"'
            
    return None

def clean():
    """Cleans build directories and generated ISO files."""
    log_step("Cleaning Build Artifacts...")
    for item in [BUILD_DIR, ISO_DIR, ISO_PATH, KERNEL_BIN]:
        if os.path.isdir(item):
            shutil.rmtree(item, ignore_errors=True)
            log_success(f"Removed directory: {os.path.basename(item)}")
        elif os.path.isfile(item):
            try:
                os.remove(item)
                log_success(f"Removed file: {os.path.basename(item)}")
            except OSError:
                pass
    log_success("Clean complete.")

def build_iso():
    """Runs make/compilation logic to build FalkonOS.iso."""
    if not check_dependencies():
        sys.exit(1)
        
    log_step("Compiling Falkon-OS Kernel & Creating ISO...")
    
    # Run make tool
    make_cmd = "make" if platform.system() != "Windows" else "make"
    if shutil.which("make") is None and shutil.which("mingw32-make"):
        make_cmd = "mingw32-make"
        
    try:
        res = subprocess.run([make_cmd, "all"], cwd=PROJECT_ROOT)
        if res.returncode != 0:
            log_error("Build failed during make execution.")
            sys.exit(1)
    except Exception as e:
        log_error(f"Failed to execute build tool '{make_cmd}': {e}")
        sys.exit(1)
        
    if os.path.exists(ISO_PATH):
        size_mb = os.path.getsize(ISO_PATH) / (1024 * 1024)
        log_success(f"FalkonOS.iso successfully generated! Size: {size_mb:.2f} MB")
    else:
        log_error("FalkonOS.iso was not created.")
        sys.exit(1)

def run_vbox():
    """Automates creation and launching of VirtualBox VM for Falkon-OS."""
    if not os.path.exists(ISO_PATH):
        log_warning("FalkonOS.iso not found! Compiling ISO first...")
        build_iso()
        
    vbox = find_vboxmanage()
    if not vbox:
        log_error("VirtualBox (VBoxManage) not found!")
        print("Please install Oracle VM VirtualBox from https://www.virtualbox.org/")
        sys.exit(1)
        
    vm_name = "FalkonOS"
    log_step(f"Setting up VirtualBox VM '{vm_name}'...")
    
    # Helper run function
    def run_vbox_cmd(args_str):
        cmd = f"{vbox} {args_str}"
        subprocess.run(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
    # Check if existing VM exists and remove it safely
    vms = subprocess.check_output(f"{vbox} list vms", shell=True).decode("utf-8", errors="ignore")
    if f'"{vm_name}"' in vms:
        log_step(f"Removing previous VM instance '{vm_name}'...")
        run_vbox_cmd(f'controlvm "{vm_name}" poweroff')
        run_vbox_cmd(f'unregistervm "{vm_name}" --delete')
        
    log_step("Creating VirtualBox VM instance (Linux 64-bit)...")
    run_vbox_cmd(f'createvm --name "{vm_name}" --ostype "Linux26_64" --register')
    
    log_step("Configuring VM parameters (512MB RAM, VMSVGA Graphics, PS/2 Mouse/Keyboard)...")
    run_vbox_cmd(f'modifyvm "{vm_name}" --memory 512 --vram 16 --graphicscontroller vmsvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2')
    
    log_step("Mounting FalkonOS.iso to IDE Optical Drive...")
    run_vbox_cmd(f'storagectl "{vm_name}" --name "IDE Controller" --add ide')
    run_vbox_cmd(f'storageattach "{vm_name}" --name "IDE Controller" --port 0 --device 0 --type dvddrive --medium "{ISO_PATH}"')
    
    log_step(f"Launching Falkon-OS in VirtualBox...")
    run_vbox_cmd(f'startvm "{vm_name}"')
    log_success("Falkon-OS VM started in VirtualBox successfully!")

def run_qemu():
    """Launches Falkon-OS in QEMU system emulator."""
    if not os.path.exists(ISO_PATH):
        log_warning("FalkonOS.iso not found! Compiling ISO first...")
        build_iso()
        
    qemu_cmd = shutil.which("qemu-system-x86_64")
    if not qemu_cmd:
        log_error("qemu-system-x86_64 executable not found in PATH.")
        sys.exit(1)
        
    log_step("Launching QEMU Emulator...")
    subprocess.run([qemu_cmd, "-cdrom", ISO_PATH, "-m", "512M", "-vga", "std"])

def main():
    parser = argparse.ArgumentParser(
        description="Falkon-OS Unified Cross-Platform Build & VM Management Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  python build.py          # Compile ISO
  python build.py --vbox   # Build ISO & launch in VirtualBox (Recommended)
  python build.py --qemu   # Build ISO & launch in QEMU
  python build.py --clean  # Clean build files
        """
    )
    parser.add_argument("--vbox", action="store_true", help="Build ISO and launch VirtualBox VM")
    parser.add_argument("--qemu", action="store_true", help="Build ISO and launch QEMU emulator")
    parser.add_argument("--clean", action="store_true", help="Clean build directories and ISO")
    parser.add_argument("--check", action="store_true", help="Verify build tool dependencies")
    
    args = parser.parse_args()
    
    if args.clean:
        clean()
    elif args.check:
        check_dependencies()
    elif args.vbox:
        build_iso()
        run_vbox()
    elif args.qemu:
        build_iso()
        run_qemu()
    else:
        build_iso()

if __name__ == "__main__":
    main()
