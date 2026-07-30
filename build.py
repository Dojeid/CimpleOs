#!/usr/bin/env python3
"""
Falkon-OS - Cross-Platform Build & Virtual Machine Driver (build.py)
Delegates build orchestration to CMake/Makefile while providing an intuitive, case-insensitive CLI.
Inspired by Falkon Compiler Toolchain Driver architecture.
"""

import sys
import os
import platform
import subprocess
import shutil
from pathlib import Path

def print_banner():
    print("=" * 60)
    print("        Falkon-OS 64-bit Operating System Build Driver       ")
    print("=" * 60)

def print_help():
    print("Usage: python build.py [-build [dev|release]] [-clean [dev|release|all]] [-run [vbox|qemu]] [-check]")
    print("\nOptions:")
    print("  -build [dev|release]  Compiles Falkon-OS kernel & generates bootable FalkonOS.iso.")
    print("                        Default mode is 'dev' if mode is omitted.")
    print("                        - dev     -> Debug build staged in falkon_[os]_dev")
    print("                        - release -> Optimized build staged in falkon_[os]_release")
    print("\n  -clean [dev|release|all] Removes build directories and ISO artifacts.")
    print("                        Default mode is 'dev' if mode is omitted.")
    print("                        - dev     -> Removes falkon_[os]_dev & build tree")
    print("                        - release -> Removes falkon_[os]_release & build_release tree")
    print("                        - all     -> Removes all build & output staging directories")
    print("\n  -run [vbox|qemu]      Builds ISO and launches in VirtualBox or QEMU emulator.")
    print("                        Default target is 'vbox' (Recommended) if mode is omitted.")
    print("                        - vbox    -> Auto-creates 64-bit VM in VirtualBox and boots ISO")
    print("                        - qemu    -> Runs qemu-system-x86_64 emulator")
    print("\n  -check                Validates toolchain dependencies (gcc, nasm, ld, grub-mkrescue, etc.)")
    print("\nExamples:")
    print("  python build.py -build")
    print("  python build.py -run vbox")
    print("  python build.py -run qemu")
    print("  python build.py -clean all")
    print("  python build.py -clean dev -build dev -run vbox")
    print("=" * 60)

def get_os_tag():
    system = platform.system().lower()
    if "windows" in system:
        return "win"
    elif "darwin" in system:
        return "mac"
    else:
        return "linux"

def parse_cli_args(args):
    """
    Case-insensitive CLI parser.
    Returns a dict with clean_mode, do_clean, build_mode, do_build, run_mode, do_run, do_check, show_help.
    """
    raw_tokens = [a.lower() for a in args[1:]]
    
    config = {
        "show_help": False,
        "do_check": False,
        "do_clean": False,
        "clean_mode": "dev",
        "do_build": False,
        "build_mode": "dev",
        "do_run": False,
        "run_mode": "vbox"
    }

    if not raw_tokens or any(h in raw_tokens for h in ["-h", "--h", "-help", "--help", "help", "?", "-?"]):
        config["show_help"] = True
        return config

    i = 0
    while i < len(raw_tokens):
        token = raw_tokens[i]
        
        if token in ["-check", "--check", "check"]:
            config["do_check"] = True
        elif token in ["-clean", "--clean", "clean"]:
            config["do_clean"] = True
            if i + 1 < len(raw_tokens) and raw_tokens[i + 1] in ["dev", "release", "all"]:
                config["clean_mode"] = raw_tokens[i + 1]
                i += 1
        elif token in ["-build", "--build", "build"]:
            config["do_build"] = True
            if i + 1 < len(raw_tokens) and raw_tokens[i + 1] in ["dev", "release"]:
                config["build_mode"] = raw_tokens[i + 1]
                i += 1
        elif token in ["-run", "--run", "run", "-vbox", "--vbox", "vbox", "-qemu", "--qemu", "qemu"]:
            config["do_run"] = True
            if token in ["vbox", "-vbox", "--vbox"]:
                config["run_mode"] = "vbox"
            elif token in ["qemu", "-qemu", "--qemu"]:
                config["run_mode"] = "qemu"
            elif i + 1 < len(raw_tokens) and raw_tokens[i + 1] in ["vbox", "qemu"]:
                config["run_mode"] = raw_tokens[i + 1]
                i += 1
        i += 1

    return config

def detect_detailed_os():
    """
    Detects detailed OS environment and returns OS name & specific install command.
    """
    system = platform.system().lower()
    
    if "linux" in system:
        # Check for WSL (Windows Subsystem for Linux)
        if os.path.exists("/proc/version"):
            try:
                with open("/proc/version", "r") as f:
                    if "microsoft" in f.read().lower():
                        return "WSL2 (Windows Subsystem for Linux - Ubuntu)", "sudo apt update && sudo apt install -y build-essential cmake nasm grub-pc-bin grub-common xorriso mtools virtualbox qemu-system-x86"
            except Exception:
                pass
        
        # Check Linux distro via /etc/os-release
        if os.path.exists("/etc/os-release"):
            try:
                with open("/etc/os-release", "r") as f:
                    content = f.read().lower()
                    if "ubuntu" in content or "debian" in content or "mint" in content:
                        return "Linux (Ubuntu / Debian / Mint)", "sudo apt update && sudo apt install -y build-essential cmake nasm grub-pc-bin grub-common xorriso mtools virtualbox qemu-system-x86"
                    elif "fedora" in content or "rhel" in content or "centos" in content:
                        return "Linux (Fedora / RHEL)", "sudo dnf install -y gcc cmake nasm grub2-tools-extra xorriso mtools virtualbox qemu-system-x86"
                    elif "arch" in content or "manjaro" in content:
                        return "Linux (Arch Linux / Manjaro)", "sudo pacman -S --needed base-devel cmake nasm grub xorriso mtools virtualbox qemu-desktop"
                    elif "alpine" in content:
                        return "Linux (Alpine Linux)", "sudo apk add build-base cmake nasm grub grub-efi xorriso mtools qemu-system-x86_64"
            except Exception:
                pass
                
        return "Generic Linux", "sudo apt update && sudo apt install -y build-essential cmake nasm grub-pc-bin grub-common xorriso mtools"
        
    elif "windows" in system:
        return "Windows (Native Command Prompt / PowerShell)", "Option A (WSL2 - Recommended):\n    Open PowerShell as Admin: wsl --install -d Ubuntu\n\n  Option B (Native Windows):\n    Install Chocolatey/MSYS2: choco install nasm virtualbox cmake -y"
        
    elif "darwin" in system:
        return "macOS", "brew install x86_64-elf-gcc cmake nasm i386-elf-grub xorriso mtools virtualbox"
        
    return "Unknown OS", "Install gcc, nasm, ld, grub-mkrescue, xorriso, mtools, and virtualbox."

def run_check():
    os_name, install_cmd = detect_detailed_os()
    print(f"\n[CHECK] Detected Operating System: \033[1;36m{os_name}\033[0m")
    print("[CHECK] Verifying Toolchain & Build Dependencies...")
    
    required_tools = ["gcc", "nasm", "ld", "grub-mkrescue"]
    missing = [tool for tool in required_tools if shutil.which(tool) is None]

    if missing:
        print(f"\n  \033[1;31m[!] ERROR: Missing required build tools: {', '.join(missing)}\033[0m")
        print(f"\n  To install all required tools on {os_name}, run:")
        print(f"  \033[1;32m{install_cmd}\033[0m\n")
        return False

    print("  + [✓] Core compilation tools found (gcc, nasm, ld, grub-mkrescue).")
    
    if shutil.which("cmake"):
        print("  + [✓] CMake build system detected.")
    else:
        print("  + [!] CMake not found (Using Makefile build engine).")

    if shutil.which("ninja"):
        print("  + [✓] Ninja build generator detected.")

    vbox_found = shutil.which("VBoxManage") or shutil.which("vboxmanage") or os.path.exists(r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe")
    if vbox_found:
        print("  + [✓] VirtualBox detected (Recommended VM launcher).")
    else:
        print("  + [!] VirtualBox not found. Install from https://www.virtualbox.org/")

    if shutil.which("qemu-system-x86_64"):
        print("  + [✓] QEMU detected.")

    print(f"\n[SUCCESS] Environment check passed for {os_name}!")
    return True

def run_clean(os_tag, mode):
    print(f"\n[CLEAN] Purging target: '{mode}'...")
    root_dir = Path(__file__).parent.resolve()
    
    dirs_to_remove = []
    files_to_remove = [root_dir / "FalkonOS.iso", root_dir / "FalkonOS.bin"]
    
    if mode == "dev":
        dirs_to_remove.extend([root_dir / f"falkon_{os_tag}_dev", root_dir / "build", root_dir / "isodir"])
    elif mode == "release":
        dirs_to_remove.extend([root_dir / f"falkon_{os_tag}_release", root_dir / "build_release", root_dir / "isodir"])
    elif mode == "all":
        dirs_to_remove.extend([
            root_dir / f"falkon_{os_tag}_dev", 
            root_dir / f"falkon_{os_tag}_release", 
            root_dir / "build", 
            root_dir / "build_release",
            root_dir / "isodir"
        ])

    for d in dirs_to_remove:
        if d.exists():
            print(f"  -> Removing directory: {d}")
            try:
                shutil.rmtree(d, ignore_errors=True)
            except Exception as e:
                print(f"     [!] Warning: Could not remove {d}: {e}")

    for f in files_to_remove:
        if f.exists():
            print(f"  -> Removing file: {f}")
            try:
                f.unlink()
            except Exception as e:
                print(f"     [!] Warning: Could not remove {f}: {e}")

    print("[CLEAN] Completed.")

def run_build(os_tag, mode):
    print(f"\n[BUILD] Compiling Falkon-OS kernel in '{mode.upper()}' mode for OS: {os_tag}...")
    root_dir = Path(__file__).parent.resolve()
    
    build_type = "Debug" if mode == "dev" else "Release"
    build_dir = root_dir / ("build" if mode == "dev" else "build_release")
    out_dir = root_dir / f"falkon_{os_tag}_{mode}"

    has_cmake = shutil.which("cmake") is not None

    if has_cmake:
        print("[BUILD] Using CMake Build Engine...")
        cmake_cmd = ["cmake", "-B", str(build_dir)]
        if shutil.which("ninja"):
            cmake_cmd.extend(["-G", "Ninja"])
        cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={build_type}")

        print(f"> {' '.join(cmake_cmd)}")
        res = subprocess.run(cmake_cmd, cwd=root_dir)
        if res.returncode != 0:
            print("[!] ERROR: CMake configuration failed.")
            sys.exit(1)

        build_cmd = ["cmake", "--build", str(build_dir), "--config", build_type]
        print(f"> {' '.join(build_cmd)}")
        res = subprocess.run(build_cmd, cwd=root_dir)
        if res.returncode != 0:
            print("[!] ERROR: Compilation failed.")
            sys.exit(1)
    else:
        print("[BUILD] Fallback to GNU Makefile...")
        make_cmd = "make"
        if shutil.which("mingw32-make"):
            make_cmd = "mingw32-make"
        res = subprocess.run([make_cmd, "all", f"MODE={mode}"], cwd=root_dir)
        if res.returncode != 0:
            print("[!] ERROR: Makefile build failed.")
            sys.exit(1)

    # Stage distribution output
    print(f"\n[STAGE] Packaging distribution in: {out_dir}")
    out_dir.mkdir(parents=True, exist_ok=True)

    iso_src = build_dir / "FalkonOS.iso"
    if not iso_src.exists():
        iso_src = root_dir / "FalkonOS.iso"

    if iso_src.exists():
        shutil.copy2(iso_src, out_dir / "FalkonOS.iso")
        if iso_src != root_dir / "FalkonOS.iso":
            shutil.copy2(iso_src, root_dir / "FalkonOS.iso")
        print(f"  + Staged bootable ISO: {out_dir / 'FalkonOS.iso'}")
    else:
        print("[!] ERROR: FalkonOS.iso not found after build.")
        sys.exit(1)

    print(f"\n[SUCCESS] Falkon-OS '{mode}' distribution successfully built -> {out_dir}")

def find_vboxmanage():
    if shutil.which("VBoxManage"): return "VBoxManage"
    if shutil.which("vboxmanage"): return "vboxmanage"
    if platform.system() == "Windows":
        path = r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
        if os.path.exists(path): return f'"{path}"'
    return None

def run_vm(os_tag, mode):
    root_dir = Path(__file__).parent.resolve()
    iso_path = root_dir / "FalkonOS.iso"

    if not iso_path.exists():
        print(f"\n[!] FalkonOS.iso not found! Triggering automatic build...")
        run_build(os_tag, "dev")

    if mode == "vbox":
        print(f"\n[RUN] Launching Falkon-OS in VirtualBox...")
        vbox = find_vboxmanage()
        if not vbox:
            print("[!] ERROR: VirtualBox (VBoxManage) not found in PATH or standard directory.")
            sys.exit(1)

        vm_name = "FalkonOS"
        def exec_vbox(cmd_str):
            subprocess.run(f"{vbox} {cmd_str}", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        vms = subprocess.check_output(f"{vbox} list vms", shell=True).decode("utf-8", errors="ignore")
        if f'"{vm_name}"' in vms:
            print(f"  -> Cleaning up existing VirtualBox VM '{vm_name}'...")
            exec_vbox(f'controlvm "{vm_name}" poweroff')
            exec_vbox(f'unregistervm "{vm_name}" --delete')

        print(f"  -> Creating 64-bit VirtualBox VM '{vm_name}'...")
        exec_vbox(f'createvm --name "{vm_name}" --ostype "Linux26_64" --register')
        exec_vbox(f'modifyvm "{vm_name}" --memory 512 --vram 16 --graphicscontroller vmsvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2')
        exec_vbox(f'storagectl "{vm_name}" --name "IDE Controller" --add ide')
        exec_vbox(f'storageattach "{vm_name}" --name "IDE Controller" --port 0 --device 0 --type dvddrive --medium "{iso_path}"')
        
        print(f"  -> Powering on VirtualBox VM...")
        exec_vbox(f'startvm "{vm_name}"')
        print("[SUCCESS] Falkon-OS running in VirtualBox!")

    elif mode == "qemu":
        print(f"\n[RUN] Launching Falkon-OS in QEMU system emulator...")
        qemu_bin = shutil.which("qemu-system-x86_64")
        if not qemu_bin:
            print("[!] ERROR: qemu-system-x86_64 not found in PATH.")
            sys.exit(1)
        subprocess.run([qemu_bin, "-cdrom", str(iso_path), "-m", "512M", "-vga", "std"])

def main():
    print_banner()
    os_tag = get_os_tag()
    config = parse_cli_args(sys.argv)

    if config["show_help"]:
        print_help()
        sys.exit(0)

    if config["do_check"]:
        run_check()

    # Rule: -clean ALWAYS executes BEFORE -build
    if config["do_clean"]:
        run_clean(os_tag, config["clean_mode"])

    if config["do_build"]:
        run_build(os_tag, config["build_mode"])

    # Rule: -run executes AFTER build
    if config["do_run"]:
        run_vm(os_tag, config["run_mode"])

if __name__ == "__main__":
    main()
