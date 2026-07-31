#!/usr/bin/env python3
"""
=============================================================================
 Falkon-OS Build Driver (build.py)
 Orchestrates compilation, native C ISO creation, and QEMU execution.
 Keeps the repository root 100% pristine by placing all outputs in out/
 and build caches in build/. Zero automatic downloads.
=============================================================================
"""

import sys
import os
import time
import json
import hashlib
import platform
import subprocess
import shutil
import argparse
from datetime import datetime
from pathlib import Path

# ─────────────────────────────────────────────────────────────────────────────
# Pristine Directory Hierarchy
# ─────────────────────────────────────────────────────────────────────────────

ROOT_DIR        = Path(__file__).parent.resolve()
SRC_DIR         = ROOT_DIR / "src"
TOOLS_DIR       = ROOT_DIR / "tools"
BUILD_DIR       = ROOT_DIR / "build"
OUT_DIR         = ROOT_DIR / "out"

# Host ISO Builder C Tool
ISO_BUILDER_SRC = TOOLS_DIR / "iso_builder.c"
ISO_BUILDER_EXE = BUILD_DIR / ("iso_builder.exe" if platform.system().lower() == "windows" else "iso_builder")

# Output Artifacts (placed in out/ to keep root clean)
PRIMARY_ISO     = OUT_DIR / "FalkonOS.iso"
REPORT_FILE     = OUT_DIR / "build_report.json"

# ─────────────────────────────────────────────────────────────────────────────
# ANSI Structured Colored Logging
# ─────────────────────────────────────────────────────────────────────────────

class Color:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    RED     = "\033[1;31m"
    GREEN   = "\033[1;32m"
    YELLOW  = "\033[1;33m"
    CYAN    = "\033[1;36m"
    MAGENTA = "\033[1;35m"

def log_info(msg):    print(f"{Color.CYAN}[INFO]{Color.RESET}    {msg}")
def log_success(msg): print(f"{Color.GREEN}[SUCCESS]{Color.RESET} {msg}")
def log_warning(msg): print(f"{Color.YELLOW}[WARNING]{Color.RESET} {msg}")
def log_error(msg):   print(f"{Color.RED}[ERROR]{Color.RESET}   {msg}")
def log_stage(step, total, msg):
    print(f"{Color.MAGENTA}[{step}/{total}]{Color.RESET} {Color.BOLD}{msg}{Color.RESET}")

def print_banner():
    print(f"{Color.CYAN}{Color.BOLD}" + "=" * 65)
    print("         Falkon-OS Build & Toolchain Driver          ")
    print("=" * 65 + f"{Color.RESET}")

# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────

def get_cpu_threads():
    try: return os.cpu_count() or 4
    except Exception: return 4

def calculate_sha256(filepath):
    if not os.path.exists(filepath): return None
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536): h.update(chunk)
    return h.hexdigest()

def ensure_iso_builder():
    """Compiles tools/iso_builder.c into build/iso_builder executable."""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    if ISO_BUILDER_EXE.exists():
        if ISO_BUILDER_EXE.stat().st_mtime >= ISO_BUILDER_SRC.stat().st_mtime:
            return str(ISO_BUILDER_EXE)

    log_info("Compiling native C ISO builder (tools/iso_builder.c)...")
    cc = shutil.which("gcc") or shutil.which("clang") or shutil.which("cl")
    if not cc:
        log_error("Host C compiler not found to compile tools/iso_builder.c")
        sys.exit(1)

    res = subprocess.run([cc, str(ISO_BUILDER_SRC), "-o", str(ISO_BUILDER_EXE)], capture_output=True, text=True)
    if res.returncode != 0:
        log_error(f"Failed to compile iso_builder.c:\n{res.stderr or res.stdout}")
        sys.exit(1)

    log_success(f"Native C ISO Builder ready -> {ISO_BUILDER_EXE}")
    return str(ISO_BUILDER_EXE)

# ─────────────────────────────────────────────────────────────────────────────
# Build Profiles
# ─────────────────────────────────────────────────────────────────────────────

PROFILES = {
    "dev":     {"cmake_type": "Debug",   "flags": "-O0 -g -DDEV_MODE"},
    "release": {"cmake_type": "Release", "flags": "-O2 -DNDEBUG -DRELEASE_MODE"}
}

# ─────────────────────────────────────────────────────────────────────────────
# Core Build Routine
# ─────────────────────────────────────────────────────────────────────────────

def build_kernel(profile="dev", do_save=False):
    t_start = time.time()
    prof_cfg = PROFILES.get(profile, PROFILES["dev"])
    os_tag = "win" if platform.system().lower() == "windows" else "linux"
    threads = get_cpu_threads()

    log_info(f"Target OS: {os_tag} | Profile: {profile.upper()} | Threads: -j{threads}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    # 1. Configure CMake
    t0 = time.time()
    log_stage(1, 5, "Configuring CMake Build Engine...")
    if not shutil.which("cmake"):
        log_error("CMake is required to build Falkon-OS.")
        log_info("Run 'python build.py doctor' to view setup instructions.")
        sys.exit(1)

    cmake_cmd = ["cmake", "-B", str(BUILD_DIR)]
    if shutil.which("ninja"):
        cmake_cmd += ["-G", "Ninja"]
    cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={prof_cfg['cmake_type']}")

    res = subprocess.run(cmake_cmd, cwd=ROOT_DIR, capture_output=True, text=True)
    if res.returncode != 0:
        log_error(f"CMake Configuration Failed:\n{res.stderr or res.stdout}")
        sys.exit(1)
    t_config = time.time() - t0

    # 2. Compile Kernel
    t0 = time.time()
    log_stage(2, 5, f"Compiling Kernel Codebase (-j{threads})...")
    build_cmd = ["cmake", "--build", str(BUILD_DIR), "--config", prof_cfg['cmake_type'], "--parallel", str(threads)]
    res = subprocess.run(build_cmd, cwd=ROOT_DIR)
    if res.returncode != 0:
        log_error("Compilation Failed.")
        sys.exit(1)
    t_compile = time.time() - t0

    kern_bin = BUILD_DIR / "FalkonOS.bin"
    boot_bin = BUILD_DIR / "bootsector.bin"

    # Assemble Stage 1 bootloader if missing
    if not boot_bin.exists():
        nasm_bin = shutil.which("nasm") or r"C:\Program Files\NASM\nasm.exe"
        boot_asm = SRC_DIR / "arch" / "x86_64" / "boot" / "bootsector.asm"
        if os.path.exists(nasm_bin) and boot_asm.exists():
            log_info("Assembling Stage 1 bootloader via NASM...")
            subprocess.run([nasm_bin, "-f", "bin", str(boot_asm), "-o", str(boot_bin)])

    if not kern_bin.exists():
        log_error("FalkonOS.bin not found after build.")
        sys.exit(1)

    # 3. Native C ISO Generation
    t0 = time.time()
    log_stage(3, 5, "Generating Bootable ISO via Native C ISO Builder...")
    iso_builder_exe = ensure_iso_builder()
    target_iso = BUILD_DIR / "FalkonOS.iso"

    res = subprocess.run([iso_builder_exe, str(target_iso), str(boot_bin if boot_bin.exists() else kern_bin), str(kern_bin)])
    if res.returncode != 0 or not target_iso.exists():
        log_error("Native ISO Generation Failed.")
        sys.exit(1)
    t_iso = time.time() - t0

    # 4. Stage Output to out/FalkonOS.iso
    log_stage(4, 5, "Staging Output ISO to out/FalkonOS.iso...")
    shutil.copy2(target_iso, PRIMARY_ISO)
    log_success(f"Output ISO staged -> {PRIMARY_ISO}")

    if do_save:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        saved_dir = OUT_DIR / f"saved_{timestamp}"
        saved_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target_iso, saved_dir / "FalkonOS.iso")
        log_success(f"Snapshot Saved -> {saved_dir / 'FalkonOS.iso'}")

    # 5. Write Report to out/build_report.json
    t_total = time.time() - t_start
    log_stage(5, 5, "Writing Build Report to out/build_report.json...")
    generate_report(target_iso, kern_bin, profile, t_config, t_compile, t_iso, t_total)

    print(f"\n{Color.BOLD}{Color.GREEN}=== Falkon-OS Build Summary ==={Color.RESET}")
    print(f"  Primary Output : {PRIMARY_ISO}")
    print(f"  Total Duration : {t_total:.2f} sec")
    print(f"{Color.GREEN}==============================={Color.RESET}\n")

def generate_report(target_iso, kern_bin, profile, t_cfg, t_cmp, t_iso, t_tot):
    report = {
        "timestamp": datetime.now().isoformat(),
        "profile": profile,
        "kernel_bytes": kern_bin.stat().st_size if kern_bin.exists() else 0,
        "iso_bytes": target_iso.stat().st_size if target_iso.exists() else 0,
        "iso_sha256": calculate_sha256(PRIMARY_ISO),
        "timing_sec": {
            "configure": round(t_cfg, 3),
            "compile": round(t_cmp, 3),
            "iso_build": round(t_iso, 3),
            "total": round(t_tot, 3)
        }
    }
    try:
        with open(REPORT_FILE, "w") as f:
            json.dump(report, f, indent=4)
    except Exception: pass

# ─────────────────────────────────────────────────────────────────────────────
# QEMU Launcher
# ─────────────────────────────────────────────────────────────────────────────

def run_qemu(mode="normal"):
    iso_path = PRIMARY_ISO if PRIMARY_ISO.exists() else (BUILD_DIR / "FalkonOS.iso")
    bin_path = BUILD_DIR / "FalkonOS.bin"

    if not iso_path.exists() and not bin_path.exists():
        log_warning("Build artifacts missing in out/. Triggering automated build...")
        build_kernel("dev")
        iso_path = PRIMARY_ISO

    qemu_bin = shutil.which("qemu-system-x86_64") or r"C:\Program Files\qemu\qemu-system-x86_64.exe"
    if not os.path.exists(qemu_bin) and not shutil.which("qemu-system-x86_64"):
        log_error("QEMU emulator not found in PATH.")
        log_info("Run 'python build.py doctor' to view setup instructions.")
        sys.exit(1)

    cmd = [qemu_bin, "-m", "512M", "-vga", "std"]

    if mode == "debug":
        log_info("QEMU Debug Mode: Listening on GDB tcp::1234")
        cmd.extend(["-s", "-S"])

    if mode in ["serial", "debug"]:
        cmd.extend(["-serial", "stdio"])

    if iso_path.exists():
        cmd.extend(["-cdrom", str(iso_path)])
        log_info(f"Booting CD-ROM -> {iso_path}")
    else:
        cmd.extend(["-kernel", str(bin_path)])
        log_info(f"Booting Kernel Binary -> {bin_path}")

    subprocess.run(cmd)

# ─────────────────────────────────────────────────────────────────────────────
# Combined Verification & Installation Instructions (check / setup / doctor)
# Zero automatic downloads — displays copy-paste commands for missing tools
# ─────────────────────────────────────────────────────────────────────────────

def run_check_setup():
    print_banner()
    log_info("Verifying Build Environment & Dependencies...")

    required_tools = [
        ("C Compiler", "gcc"),
        ("NASM Assembler", "nasm"),
        ("CMake Engine", "cmake"),
        ("Ninja Generator", "ninja"),
        ("QEMU Emulator", "qemu-system-x86_64"),
        ("Git VCS", "git")
    ]

    missing = []
    for name, tool in required_tools:
        p = shutil.which(tool) or (r"C:\Program Files\qemu\qemu-system-x86_64.exe" if tool.startswith("qemu") else None)
        if p:
            log_success(f"Found {name:<20} -> {p}")
        else:
            log_warning(f"Missing {name:<19} ({tool})")
            missing.append(tool)

    if PRIMARY_ISO.exists():
        log_success(f"Primary Output ISO verified -> {PRIMARY_ISO} ({PRIMARY_ISO.stat().st_size} bytes)")
    else:
        log_info("No output ISO found in out/. Run 'python build.py build'")

    print()
    if missing:
        log_error(f"Missing required tool(s): {', '.join(missing)}")
        print(f"\n{Color.BOLD}{Color.YELLOW}=== Installation Instructions for Missing Tools ==={Color.RESET}\n")

        os_sys = platform.system().lower()
        if "windows" in os_sys:
            print(f"{Color.CYAN}[Option 1: Windows Winget (Copy & Paste in PowerShell)]{Color.RESET}")
            winget_pkgs = []
            if "nasm" in missing: winget_pkgs.append("NASM.NASM")
            if "cmake" in missing: winget_pkgs.append("Kitware.CMake")
            if "ninja" in missing: winget_pkgs.append("Ninja-build.Ninja")
            if "qemu-system-x86_64" in missing: winget_pkgs.append("SoftwareFreedomConservancy.QEMU")
            if winget_pkgs:
                print(f"  winget install {' '.join(winget_pkgs)}\n")

            print(f"{Color.CYAN}[Option 2: MSYS2 UCRT64 (Copy & Paste in MSYS2 Terminal)]{Color.RESET}")
            msys_pkgs = []
            if "gcc" in missing: msys_pkgs.append("mingw-w64-ucrt-x86_64-gcc")
            if "nasm" in missing: msys_pkgs.append("mingw-w64-ucrt-x86_64-nasm")
            if "cmake" in missing: msys_pkgs.append("mingw-w64-ucrt-x86_64-cmake")
            if "ninja" in missing: msys_pkgs.append("mingw-w64-ucrt-x86_64-ninja")
            if msys_pkgs:
                print(f"  pacman -S --needed {' '.join(msys_pkgs)}\n")
        else:
            print(f"{Color.CYAN}[Ubuntu / Debian]:{Color.RESET}  sudo apt update && sudo apt install -y build-essential nasm cmake ninja-build qemu-system-x86")
            print(f"{Color.CYAN}[Arch Linux]:{Color.RESET}      sudo pacman -S --needed base-devel nasm cmake ninja qemu-system-x86")
            print(f"{Color.CYAN}[Fedora]:{Color.RESET}          sudo dnf install -y gcc nasm cmake ninja-build qemu-system-x86")
            print(f"{Color.CYAN}[macOS]:{Color.RESET}           brew install nasm cmake ninja qemu\n")

        print(f"{Color.YELLOW}===================================================={Color.RESET}\n")
        return False
    else:
        log_success("Environment check passed! All build dependencies are installed.")
        return True

# ─────────────────────────────────────────────────────────────────────────────
# Entry Point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Falkon-OS Build Driver")
    subparsers = parser.add_subparsers(dest="subcommand")

    p_build = subparsers.add_parser("build")
    p_build.add_argument("profile", nargs="?", default="dev", choices=["dev", "release"])
    p_build.add_argument("-save", "--save", action="store_true")

    p_run = subparsers.add_parser("run")
    p_run.add_argument("mode", nargs="?", default="normal", choices=["normal", "debug", "serial"])

    subparsers.add_parser("clean")
    subparsers.add_parser("doctor")
    subparsers.add_parser("check")
    subparsers.add_parser("setup")

    args_raw = [a.lower() for a in sys.argv[1:]]

    if not args_raw:
        print_banner()
        parser.print_help()
        sys.exit(0)

    # Combined -check / -setup / doctor entry point
    if any(k in args_raw for k in ["-setup", "setup", "-check", "check", "doctor", "-doctor"]):
        run_check_setup()
        sys.exit(0)

    if "-clean" in args_raw or "clean" in args_raw:
        if OUT_DIR.exists(): shutil.rmtree(OUT_DIR, ignore_errors=True)
        if BUILD_DIR.exists(): shutil.rmtree(BUILD_DIR, ignore_errors=True)
        log_success("Cleaned build/ and out/ directories.")
        sys.exit(0)

    if "-build" in args_raw or "build" in args_raw:
        do_save = "-save" in args_raw or "--save" in args_raw or "save" in args_raw
        prof = "release" if "release" in args_raw else "dev"
        build_kernel(prof, do_save)
        if "-run" not in args_raw and "run" not in args_raw:
            sys.exit(0)

    if "-run" in args_raw or "run" in args_raw:
        mode = "debug" if "debug" in args_raw else ("serial" if "serial" in args_raw else "normal")
        run_qemu(mode)
        sys.exit(0)

    parsed = parser.parse_args()
    if parsed.subcommand == "build": build_kernel(parsed.profile, parsed.save)
    elif parsed.subcommand == "run": run_qemu(parsed.mode)

if __name__ == "__main__":
    main()
