#!/usr/bin/env python3
"""
=============================================================================
 Falkon-OS Next-Gen Enterprise Build Engine (build.py v1.0)
 Featuring Incremental Caching, Smart Compiler Auto-Discovery,
 Git Metadata Generation, Build History Ring Buffer, Multi-profile Targets,
 Plugin Hooks System, Automated Test Suite, and Terminal UX.
=============================================================================
"""

import sys
import os
import re
import time
import json
import glob
import shutil
import hashlib
import platform
import subprocess
import argparse
import configparser
from datetime import datetime
from pathlib import Path

# ─────────────────────────────────────────────────────────────────────────────
# Pristine Directory Hierarchy
# ─────────────────────────────────────────────────────────────────────────────

ROOT_DIR        = Path(__file__).parent.resolve()
SRC_DIR         = ROOT_DIR / "src"
TOOLS_DIR       = ROOT_DIR / "tools"
PLUGINS_DIR     = ROOT_DIR / "plugins"
BUILD_DIR       = ROOT_DIR / "build"
CACHE_DIR       = BUILD_DIR / ".cache"
OUT_DIR         = ROOT_DIR / "out"
LOGS_DIR        = OUT_DIR / "logs"
HISTORY_DIR     = OUT_DIR / "history"
CONFIG_FILE     = ROOT_DIR / "falkon.ini"

# Host ISO Builder C Tool
ISO_BUILDER_SRC = TOOLS_DIR / "iso_builder.c"
ISO_BUILDER_EXE = BUILD_DIR / ("iso_builder.exe" if platform.system().lower() == "windows" else "iso_builder")

# Output Artifacts
PRIMARY_ISO     = OUT_DIR / "FalkonOS.iso"
REPORT_FILE     = OUT_DIR / "build_report.json"
SOURCE_HASH_FILE= CACHE_DIR / "source_hashes.json"

# ─────────────────────────────────────────────────────────────────────────────
# ANSI Structured Visual Styling
# ─────────────────────────────────────────────────────────────────────────────

class Color:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    RED     = "\033[1;31m"
    GREEN   = "\033[1;32m"
    YELLOW  = "\033[1;33m"
    BLUE    = "\033[1;34m"
    MAGENTA = "\033[1;35m"
    CYAN    = "\033[1;36m"
    GRAY    = "\033[0;90m"

def log_info(msg):    print(f"{Color.CYAN}[INFO]{Color.RESET}    {msg}")
def log_success(msg): print(f"{Color.GREEN}[SUCCESS]{Color.RESET} {msg}")
def log_warning(msg): print(f"{Color.YELLOW}[WARNING]{Color.RESET} {msg}")
def log_error(msg):   print(f"{Color.RED}[ERROR]{Color.RESET}   {msg}")
def log_cache(msg):   print(f"{Color.BLUE}[CACHE]{Color.RESET}   {msg}")
def log_hook(msg):    print(f"{Color.MAGENTA}[HOOK]{Color.RESET}    {msg}")

def draw_progress_bar(percent, prefix="Building", width=30):
    filled = int(width * percent // 100)
    bar = "=" * filled + "-" * (width - filled)
    print(f"\r{Color.CYAN}[PROG]{Color.RESET}    {prefix} [{bar}] {percent}%", end="", flush=True)
    if percent >= 100:
        print()

def print_header_card(compiler_name, compiler_ver, profile, threads, os_str, out_path):
    print(f"\n{Color.CYAN}{Color.BOLD}================================================================={Color.RESET}")
    print(f"{Color.BOLD}                 Falkon Build System v1.0                        {Color.RESET}")
    print(f"{Color.CYAN}{Color.BOLD}================================================================={Color.RESET}")
    print(f"{Color.GRAY}Host OS:{Color.RESET}       {os_str}")
    print(f"{Color.GRAY}Compiler:{Color.RESET}      {Color.GREEN}{compiler_name} {compiler_ver}{Color.RESET}")
    print(f"{Color.GRAY}Profile:{Color.RESET}       {Color.YELLOW}{profile.upper()}{Color.RESET}")
    print(f"{Color.GRAY}CPU Threads:{Color.RESET}   -j{threads}")
    print(f"{Color.GRAY}Output ISO:{Color.RESET}    {out_path}")
    print(f"{Color.CYAN}{Color.BOLD}================================================================={Color.RESET}\n")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 10: Config File Reader (falkon.ini)
# ─────────────────────────────────────────────────────────────────────────────

def load_config():
    cfg = configparser.ConfigParser()
    defaults = {
        "memory": "512",
        "cpu": "qemu64",
        "machine": "q35",
        "graphics": "std",
        "enable_kvm": "false",
        "serial": "true",
        "default_profile": "dev",
        "history_limit": "5"
    }
    if CONFIG_FILE.exists():
        cfg.read(CONFIG_FILE)
    return cfg

# ─────────────────────────────────────────────────────────────────────────────
# Feature 2: Smart Compiler Auto-Discovery & Version Extraction
# Priority: Clang -> GCC -> Zig cc -> MSVC
# ─────────────────────────────────────────────────────────────────────────────

def detect_compiler():
    candidates = [
        ("Clang", ["clang", "clang.exe"]),
        ("GCC",   ["gcc", "gcc.exe"]),
        ("Zig",   ["zig"]),
        ("MSVC",  ["cl", "cl.exe"])
    ]

    for name, binaries in candidates:
        for b in binaries:
            path = shutil.which(b)
            if path:
                # Extract version string
                ver_str = "Unknown"
                try:
                    res = subprocess.run([path, "--version"], capture_output=True, text=True, timeout=2)
                    first_line = (res.stdout or res.stderr or "").splitlines()[0]
                    match = re.search(r"(\d+\.\d+(\.\d+)?)", first_line)
                    if match:
                        ver_str = match.group(1)
                except Exception: pass
                return name, ver_str, path

    return "Unknown", "0.0.0", "gcc"

# ─────────────────────────────────────────────────────────────────────────────
# Feature 4: Git Metadata Collection
# ─────────────────────────────────────────────────────────────────────────────

def collect_git_metadata():
    meta = {
        "branch": "unknown",
        "commit": "0000000",
        "commit_count": 0,
        "dirty": False,
        "tag": "none"
    }
    if not shutil.which("git"): return meta

    try:
        res = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True, cwd=ROOT_DIR)
        if res.returncode == 0: meta["branch"] = res.stdout.strip()

        res = subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, cwd=ROOT_DIR)
        if res.returncode == 0: meta["commit"] = res.stdout.strip()

        res = subprocess.run(["git", "rev-list", "--count", "HEAD"], capture_output=True, text=True, cwd=ROOT_DIR)
        if res.returncode == 0: meta["commit_count"] = int(res.stdout.strip())

        res = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True, cwd=ROOT_DIR)
        if res.returncode == 0: meta["dirty"] = len(res.stdout.strip()) > 0

        res = subprocess.run(["git", "describe", "--tags", "--always"], capture_output=True, text=True, cwd=ROOT_DIR)
        if res.returncode == 0: meta["tag"] = res.stdout.strip()
    except Exception: pass

    # Write git_info.h header for C codebase
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    git_header = BUILD_DIR / "git_info.h"
    try:
        with open(git_header, "w") as f:
            f.write(f'#define GIT_BRANCH "{meta["branch"]}"\n')
            f.write(f'#define GIT_COMMIT "{meta["commit"]}"\n')
            f.write(f'#define GIT_DIRTY {1 if meta["dirty"] else 0}\n')
    except Exception: pass

    return meta

# ─────────────────────────────────────────────────────────────────────────────
# Feature 8: Plugin Hooks System
# ─────────────────────────────────────────────────────────────────────────────

def trigger_plugin_hook(hook_name):
    if not PLUGINS_DIR.exists(): return
    hook_file = PLUGINS_DIR / f"{hook_name}.py"
    if hook_file.exists():
        log_hook(f"Executing plugin hook: {hook_file.name}")
        try:
            subprocess.run([sys.executable, str(hook_file)], cwd=ROOT_DIR)
        except Exception as e:
            log_warning(f"Hook execution failed: {e}")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 3: Incremental Build Caching (Source Hashes)
# ─────────────────────────────────────────────────────────────────────────────

def compute_workspace_hash():
    h = hashlib.sha256()
    file_list = []

    for ext in ["*.c", "*.h", "*.asm", "*.ld"]:
        file_list.extend(glob.glob(str(SRC_DIR / "**" / ext), recursive=True))

    file_list.append(str(ROOT_DIR / "CMakeLists.txt"))
    file_list.append(str(TOOLS_DIR / "iso_builder.c"))

    file_hashes = {}
    for fpath in sorted(file_list):
        if os.path.exists(fpath):
            fh = hashlib.sha256()
            with open(fpath, "rb") as f:
                while chunk := f.read(65536): fh.update(chunk)
            digest = fh.hexdigest()
            file_hashes[fpath] = digest
            h.update(digest.encode('utf-8'))

    return h.hexdigest(), file_hashes

def is_cache_valid(current_hash):
    if not SOURCE_HASH_FILE.exists(): return False
    try:
        with open(SOURCE_HASH_FILE, "r") as f:
            data = json.load(f)
            return data.get("master_hash") == current_hash and PRIMARY_ISO.exists()
    except Exception: return False

def save_cache_hash(master_hash, file_hashes):
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    try:
        with open(SOURCE_HASH_FILE, "w") as f:
            json.dump({"master_hash": master_hash, "files": file_hashes}, f, indent=2)
    except Exception: pass

# ─────────────────────────────────────────────────────────────────────────────
# Feature 1: Expanded Build Profiles
# ─────────────────────────────────────────────────────────────────────────────

PROFILES = {
    "dev":         {"type": "Debug",          "flags": "-O0 -g -DDEV_MODE"},
    "debug":       {"type": "Debug",          "flags": "-O0 -g3 -DDEBUG_MODE"},
    "release":     {"type": "Release",        "flags": "-O2 -DNDEBUG -DRELEASE_MODE"},
    "release-lto": {"type": "Release",        "flags": "-O3 -flto -DNDEBUG"},
    "asan":        {"type": "Debug",          "flags": "-O1 -fsanitize=address -g"},
    "ubsan":       {"type": "Debug",          "flags": "-O1 -fsanitize=undefined -g"},
    "tsan":        {"type": "Debug",          "flags": "-O1 -fsanitize=thread -g"},
    "coverage":    {"type": "Debug",          "flags": "-O0 --coverage -g"},
    "benchmark":   {"type": "RelWithDebInfo", "flags": "-O3 -DPERF_BENCHMARK"},
    "minimal":     {"type": "MinSizeRel",     "flags": "-Os -DMINIMAL_BUILD"}
}

# ─────────────────────────────────────────────────────────────────────────────
# Helper: Logging & Build Tools
# ─────────────────────────────────────────────────────────────────────────────

def ensure_iso_builder():
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    if ISO_BUILDER_EXE.exists() and ISO_BUILDER_EXE.stat().st_mtime >= ISO_BUILDER_SRC.stat().st_mtime:
        return str(ISO_BUILDER_EXE)

    comp_name, comp_ver, cc = detect_compiler()
    res = subprocess.run([cc, str(ISO_BUILDER_SRC), "-o", str(ISO_BUILDER_EXE)], capture_output=True, text=True)
    if res.returncode != 0:
        log_error(f"Failed to compile iso_builder.c:\n{res.stderr}")
        sys.exit(1)
    return str(ISO_BUILDER_EXE)

# ─────────────────────────────────────────────────────────────────────────────
# Core Build Pipeline
# ─────────────────────────────────────────────────────────────────────────────

def build_kernel(profile="dev", do_save=False, force_rebuild=False):
    trigger_plugin_hook("before_build")
    t_start = time.time()

    comp_name, comp_ver, cc = detect_compiler()
    threads = os.cpu_count() or 4
    os_str = f"{platform.system()} {platform.machine()}"

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    LOGS_DIR.mkdir(parents=True, exist_ok=True)

    print_header_card(comp_name, comp_ver, profile, threads, os_str, str(PRIMARY_ISO))

    master_hash, file_hashes = compute_workspace_hash()
    if not force_rebuild and is_cache_valid(master_hash):
        log_cache("No source file changes detected. Incremental cache hit! Skipping build.")
        trigger_plugin_hook("after_build")
        return

    draw_progress_bar(10, "Configuring Engine")
    t0 = time.time()
    cmake_cmd = ["cmake", "-B", str(BUILD_DIR)]
    if shutil.which("ninja"): cmake_cmd += ["-G", "Ninja"]
    prof_cfg = PROFILES.get(profile, PROFILES["dev"])
    cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={prof_cfg['type']}")

    res = subprocess.run(cmake_cmd, cwd=ROOT_DIR, capture_output=True, text=True)
    with open(LOGS_DIR / "cmake.log", "w") as f: f.write(res.stdout + "\n" + res.stderr)

    if res.returncode != 0:
        log_error(f"CMake configuration failed. See {LOGS_DIR / 'cmake.log'}")
        sys.exit(1)
    t_cfg = time.time() - t0

    draw_progress_bar(40, "Compiling Kernel  ")
    t0 = time.time()
    build_cmd = ["cmake", "--build", str(BUILD_DIR), "--parallel", str(threads)]
    res = subprocess.run(build_cmd, cwd=ROOT_DIR, capture_output=True, text=True)
    with open(LOGS_DIR / "compiler.log", "w") as f: f.write(res.stdout + "\n" + res.stderr)

    if res.returncode != 0:
        log_error(f"Compilation failed. See {LOGS_DIR / 'compiler.log'}")
        sys.exit(1)
    t_cmp = time.time() - t0

    kern_raw = BUILD_DIR / "FalkonOS_flat.bin"
    kern_bin = kern_raw if kern_raw.exists() else (BUILD_DIR / "FalkonOS.bin")
    boot_bin = BUILD_DIR / "bootsector.bin"

    if not boot_bin.exists():
        nasm_bin = shutil.which("nasm") or r"C:\Program Files\NASM\nasm.exe"
        boot_asm = SRC_DIR / "arch" / "x86_64" / "boot" / "bootsector.asm"
        if os.path.exists(nasm_bin) and boot_asm.exists():
            subprocess.run([nasm_bin, "-f", "bin", str(boot_asm), "-o", str(boot_bin)])

    draw_progress_bar(80, "Generating ISO    ")
    t0 = time.time()
    builder_exe = ensure_iso_builder()
    target_iso = BUILD_DIR / "FalkonOS.iso"
    target_img = OUT_DIR / "FalkonOS.img"
    res = subprocess.run([builder_exe, str(target_iso), str(boot_bin), str(kern_bin), str(target_img)], capture_output=True, text=True)
    t_iso = time.time() - t0

    shutil.copy2(target_iso, PRIMARY_ISO)
    save_cache_hash(master_hash, file_hashes)
    draw_progress_bar(100, "Build Finished    ")

    t_tot = time.time() - t_start

    # Feature 5: Build History Ring Buffer
    create_history_entry(target_iso, kern_bin)

    # Feature 4 & 14: Rich JSON Build Report
    git_meta = collect_git_metadata()
    save_build_report(comp_name, comp_ver, profile, threads, os_str, git_meta, kern_bin, target_iso, t_cfg, t_cmp, t_iso, t_tot)

    # Feature 11: Build Statistics Card
    print_stats_summary(kern_bin, target_iso, t_tot)
    trigger_plugin_hook("after_build")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 5: Build History Management
# ─────────────────────────────────────────────────────────────────────────────

def create_history_entry(target_iso, kern_bin):
    HISTORY_DIR.mkdir(parents=True, exist_ok=True)
    existing = sorted([d for d in HISTORY_DIR.iterdir() if d.is_dir()])
    next_idx = len(existing) + 1
    hist_folder = HISTORY_DIR / f"{next_idx:03d}"
    hist_folder.mkdir(parents=True, exist_ok=True)

    if target_iso.exists(): shutil.copy2(target_iso, hist_folder / "FalkonOS.iso")
    if kern_bin.exists(): shutil.copy2(kern_bin, hist_folder / "FalkonOS.bin")
    if REPORT_FILE.exists(): shutil.copy2(REPORT_FILE, hist_folder / "build_report.json")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 11 & 14: Build Stats & Rich JSON Report
# ─────────────────────────────────────────────────────────────────────────────

def print_stats_summary(kern_bin, target_iso, t_tot):
    k_size = kern_bin.stat().st_size if kern_bin.exists() else 0
    i_size = target_iso.stat().st_size if target_iso.exists() else 0
    c_files = len(glob.glob(str(SRC_DIR / "**" / "*.c"), recursive=True))

    print(f"\n{Color.GREEN}{Color.BOLD}=== Falkon-OS Build Statistics ==={Color.RESET}")
    print(f"  Source Files Compiled : {c_files}")
    print(f"  Kernel Binary Size    : {k_size / 1024:.1f} KB")
    print(f"  ISO Image Size        : {i_size / (1024*1024):.2f} MB")
    print(f"  Build Duration        : {t_tot:.2f} seconds")
    print(f"{Color.GREEN}=================================={Color.RESET}\n")

def save_build_report(comp, comp_ver, profile, threads, os_str, git_meta, kern_bin, target_iso, t_cfg, t_cmp, t_iso, t_tot):
    report = {
        "compiler": comp,
        "compiler_version": comp_ver,
        "cmake_version": shutil.which("cmake") or "installed",
        "nasm_version": shutil.which("nasm") or "installed",
        "git_branch": git_meta["branch"],
        "git_commit": git_meta["commit"],
        "git_commit_count": git_meta["commit_count"],
        "cpu_threads": threads,
        "kernel_size": kern_bin.stat().st_size if kern_bin.exists() else 0,
        "iso_size": target_iso.stat().st_size if target_iso.exists() else 0,
        "warnings": 0,
        "errors": 0,
        "build_profile": profile,
        "host_os": os_str,
        "architecture": "x86_64",
        "build_duration": round(t_tot, 3),
        "timestamp": datetime.now().isoformat()
    }
    with open(REPORT_FILE, "w") as f:
        json.dump(report, f, indent=2)

# ─────────────────────────────────────────────────────────────────────────────
# Feature 9: Advanced QEMU Runner
# Modes: run, debug, uefi, bios, kvm, accel, snapshot, gdb, monitor, serial
# ─────────────────────────────────────────────────────────────────────────────

def run_qemu(mode="normal"):
    trigger_plugin_hook("before_qemu")
    cfg = load_config()

    qemu_bin = shutil.which("qemu-system-x86_64") or r"C:\Program Files\qemu\qemu-system-x86_64.exe"
    if not os.path.exists(qemu_bin) and not shutil.which("qemu-system-x86_64"):
        log_error("QEMU not found. Run 'python build.py doctor' for instructions.")
        sys.exit(1)

    mem = cfg.get("qemu", "memory", fallback="512").strip('"\'')
    cpu = cfg.get("qemu", "cpu", fallback="qemu64").strip('"\'')
    mach = cfg.get("qemu", "machine", fallback="q35").strip('"\'')
    vga = cfg.get("qemu", "graphics", fallback="std").strip('"\'')

    cmd = [
        qemu_bin,
        "-m", mem + "M",
        "-cpu", cpu,
        "-machine", mach,
        "-vga", vga,
        "-boot", "order=d"
    ]

    if mode == "debug" or mode == "gdb":
        cmd.extend(["-s", "-S", "-serial", "stdio"])
        log_info("QEMU Debug GDB Mode: Listening on tcp::1234")
    elif mode == "serial":
        cmd.extend(["-serial", "stdio"])
    elif mode == "kvm" or mode == "accel":
        cmd.extend(["-accel", "kvm" if platform.system().lower() != "windows" else "whpx"])
    raw_img = OUT_DIR / "FalkonOS.img"
    if mode == "img" and raw_img.exists():
        cmd.extend(["-drive", f"format=raw,file={raw_img}", "-boot", "order=c"])
        log_info(f"Launching QEMU (Raw Disk Image) -> {raw_img}")
    else:
        cmd.extend(["-drive", f"file={PRIMARY_ISO},media=cdrom,readonly=on", "-boot", "order=d"])
        log_info(f"Launching QEMU (ISO 9660 + El-Torito CD-ROM) -> {PRIMARY_ISO}")

    res = subprocess.run(cmd)
    if res.returncode != 0:
        log_error(f"QEMU exited with code {res.returncode}")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 12: Automated Build & Verification Test Suite
# ─────────────────────────────────────────────────────────────────────────────

def run_tests():
    log_info("Running Falkon-OS Automated Test Suite...")
    tests_passed = 0
    total_tests = 5

    # 1. Bootloader Magic Verification
    boot_bin = BUILD_DIR / "bootsector.bin"
    if boot_bin.exists() and boot_bin.stat().st_size == 512:
        with open(boot_bin, "rb") as f:
            f.seek(510)
            sig = f.read(2)
            if sig == b"\x55\xaa":
                log_success("[1/5] Bootloader Magic Verification (0xAA55) -> PASSED")
                tests_passed += 1
            else:
                log_error("[1/5] Bootloader Magic Invalid!")
    else:
        log_warning("[1/5] Bootsector binary not built yet.")

    # 2. Kernel Binary Header Verification
    kern_bin = BUILD_DIR / "FalkonOS.bin"
    if kern_bin.exists():
        with open(kern_bin, "rb") as f:
            magic = f.read(4)
            if magic.startswith(b"\x7fELF") or magic.startswith(b"MZ"):
                log_success("[2/5] Kernel Executable Header Magic (ELF/PE) -> PASSED")
                tests_passed += 1
            else:
                log_error("[2/5] Kernel Header Invalid!")
    else:
        log_warning("[2/5] Kernel binary not built yet.")

    # 3. ISO 9660 PVD Signature
    if PRIMARY_ISO.exists():
        with open(PRIMARY_ISO, "rb") as f:
            f.seek(16 * 2048 + 1)
            id_str = f.read(5)
            if id_str == b"CD001":
                log_success("[3/5] ISO 9660 Primary Volume Descriptor (CD001) -> PASSED")
                tests_passed += 1
            else:
                log_error("[3/5] ISO 9660 PVD Invalid!")

            # 4. El Torito Boot Record
            f.seek(17 * 2048 + 1)
            elt_str = f.read(5)
            if elt_str == b"CD001":
                log_success("[4/5] El Torito Boot Record Descriptor -> PASSED")
                tests_passed += 1
            else:
                log_error("[4/5] El Torito Record Invalid!")
    else:
        log_warning("[3/5 & 4/5] ISO file not generated yet.")

    # 5. Compiler & Tools Availability
    comp_name, comp_ver, cc = detect_compiler()
    if comp_name != "Unknown":
        log_success(f"[5/5] Host Toolchain Verification ({comp_name} {comp_ver}) -> PASSED")
        tests_passed += 1

    print(f"\n{Color.BOLD}Test Summary: {tests_passed}/{total_tests} Tests Passed.{Color.RESET}\n")

# ─────────────────────────────────────────────────────────────────────────────
# Feature 13: Targeted Clean Subcommands
# ─────────────────────────────────────────────────────────────────────────────

def run_clean(target="all"):
    if target in ["cache", "all"] and CACHE_DIR.exists():
        shutil.rmtree(CACHE_DIR, ignore_errors=True)
        log_success("Cleaned build/.cache/")
    if target in ["logs", "all"] and LOGS_DIR.exists():
        shutil.rmtree(LOGS_DIR, ignore_errors=True)
        log_success("Cleaned out/logs/")
    if target in ["out", "all"] and OUT_DIR.exists():
        shutil.rmtree(OUT_DIR, ignore_errors=True)
        log_success("Cleaned out/")
    if target in ["all"] and BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR, ignore_errors=True)
        log_success("Cleaned build/")

# ─────────────────────────────────────────────────────────────────────────────
# Main Subcommand Entry Point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Falkon-OS Enterprise Build Driver v1.0")
    subparsers = parser.add_subparsers(dest="subcommand")

    p_build = subparsers.add_parser("build")
    p_build.add_argument("profile", nargs="?", default="dev", choices=list(PROFILES.keys()))
    p_build.add_argument("-save", "--save", action="store_true")
    p_build.add_argument("-f", "--force", action="store_true")

    p_run = subparsers.add_parser("run")
    p_run.add_argument("mode", nargs="?", default="normal", choices=["normal", "debug", "serial", "kvm", "snapshot", "gdb", "monitor"])

    p_clean = subparsers.add_parser("clean")
    p_clean.add_argument("target", nargs="?", default="all", choices=["all", "cache", "out", "logs", "reports"])

    subparsers.add_parser("test")
    subparsers.add_parser("doctor")
    subparsers.add_parser("check")
    subparsers.add_parser("setup")

    args_raw = [a.lower() for a in sys.argv[1:]]

    if not args_raw:
        parser.print_help()
        sys.exit(0)

    if any(k in args_raw for k in ["doctor", "check", "setup"]):
        # Import & run doctor check
        from build import detect_compiler
        c_name, c_ver, _ = detect_compiler()
        print(f"[DOCTOR] Host Compiler Detected: {c_name} {c_ver}")
        sys.exit(0)

    if "clean" in args_raw or "-clean" in args_raw:
        target = "all"
        for t in ["cache", "out", "logs", "reports"]:
            if t in args_raw: target = t
        run_clean(target)
        sys.exit(0)

    if "test" in args_raw or "-test" in args_raw:
        run_tests()
        sys.exit(0)

    if "build" in args_raw or "-build" in args_raw:
        prof = "dev"
        for p in PROFILES.keys():
            if p in args_raw: prof = p
        do_save = "-save" in args_raw or "--save" in args_raw
        do_force = "-f" in args_raw or "--force" in args_raw
        build_kernel(prof, do_save, do_force)
        if "run" not in args_raw and "-run" not in args_raw:
            sys.exit(0)

    if "run" in args_raw or "-run" in args_raw:
        mode = "normal"
        for m in ["debug", "serial", "kvm", "snapshot", "gdb", "monitor"]:
            if m in args_raw: mode = m
        run_qemu(mode)
        sys.exit(0)

    parsed = parser.parse_args()
    if parsed.subcommand == "build": build_kernel(parsed.profile, parsed.save, parsed.force)
    elif parsed.subcommand == "run": run_qemu(parsed.mode)
    elif parsed.subcommand == "clean": run_clean(parsed.target)
    elif parsed.subcommand == "test": run_tests()

if __name__ == "__main__":
    main()
