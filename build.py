#!/usr/bin/env python3
"""
Falkon-OS - Cross-Platform Build & Virtual Machine Driver (build.py)
Delegates build orchestration to CMake/Ninja while providing an intuitive CLI.
"""

import sys
import os
import platform
import subprocess
import shutil
import zipfile
import urllib.request
from datetime import datetime
from pathlib import Path

ROOT_DIR = Path(__file__).parent.resolve()
TOOLS_DIR = ROOT_DIR / "tools"
GRUB_TOOLS_DIR = TOOLS_DIR / "grub"
GRUB_WIN_ZIP = TOOLS_DIR / "grub-win.zip"
GRUB_WIN_URL = "https://ftp.gnu.org/gnu/grub/grub-2.06-for-windows.zip"

CROSS_DIR = TOOLS_DIR / "cross"
CROSS_WIN_ZIP = TOOLS_DIR / "x86_64-elf-tools-windows.zip"
CROSS_WIN_URL = "https://github.com/lordmilko/i686-elf-tools/releases/download/15.2.0/x86_64-elf-tools-windows.zip"

# ─────────────────────────────────────────────────────────────────────────────
# Path helpers
# ─────────────────────────────────────────────────────────────────────────────

def to_msys_path(path):
    p = Path(path).resolve().as_posix()
    if len(p) >= 2 and p[1] == ":":
        drive = p[0].lower()
        return f"/{drive}" + p[2:]
    return p

def find_msys2_bash():
    """Scan common drives for an MSYS2 bash.exe."""
    bash_in_path = shutil.which("bash")
    if bash_in_path and "msys" in bash_in_path.lower():
        return bash_in_path
    drives = ["C:", "D:", "E:", "F:"]
    subdirs = [
        r"msys64\usr\bin\bash.exe",
        r"Apps\msys64\usr\bin\bash.exe",
        r"Program Files\msys64\usr\bin\bash.exe",
        r"tools\msys64\usr\bin\bash.exe",
    ]
    for d in drives:
        for sub in subdirs:
            full = Path(d) / sub
            if full.exists():
                return str(full)
    return None

def find_grub_mkrescue():
    """
    Find grub-mkrescue in order of preference:
    1. System PATH (Linux / WSL)
    2. Local tools/grub/ extracted from GNU Windows zip
    3. MSYS2 (if installed)
    """
    sys_gmr = shutil.which("grub-mkrescue")
    if sys_gmr:
        return sys_gmr

    # Check our bundled copy
    local_gmr = GRUB_TOOLS_DIR / "grub-mkrescue.exe"
    if local_gmr.exists():
        return str(local_gmr)

    # Check MSYS2 ucrt64 / mingw64 bins
    msys_bash = find_msys2_bash()
    if msys_bash:
        msys_root = Path(msys_bash).parent.parent
        for sub in [r"usr\bin\grub-mkrescue", r"ucrt64\bin\grub-mkrescue", r"mingw64\bin\grub-mkrescue"]:
            p = msys_root / sub
            if p.exists():
                return str(p)

    return None

def find_cross_gcc():
    """Find x86_64-elf-gcc cross-compiler (bundled or system)."""
    # Bundled (tools/cross/bin/)
    for name in ["x86_64-elf-gcc.exe", "x86_64-elf-gcc"]:
        p = CROSS_DIR / "bin" / name
        if p.exists():
            return str(p)
    # System PATH
    sys_cc = shutil.which("x86_64-elf-gcc")
    if sys_cc:
        return sys_cc
    return None

def fix_windows_path():
    if platform.system().lower() != "windows":
        return
    possible_paths = [
        r"C:\Program Files\NASM",
        r"C:\Program Files (x86)\NASM",
        r"C:\ProgramData\chocolatey\bin",
        os.path.expanduser(r"~\AppData\Local\bin\NASM"),
        os.path.expanduser(r"~\AppData\Local\Programs\NASM"),
        str(GRUB_TOOLS_DIR),
        str(CROSS_DIR / "bin"),
    ]
    msys_bash = find_msys2_bash()
    if msys_bash:
        msys_root = Path(msys_bash).parent.parent
        possible_paths.extend([
            str(msys_root / "usr" / "bin"),
            str(msys_root / "ucrt64" / "bin"),
            str(msys_root / "mingw64" / "bin"),
        ])
    current_path = os.environ.get("PATH", "")
    for p in possible_paths:
        if os.path.exists(p) and p.lower() not in current_path.lower():
            os.environ["PATH"] = p + os.path.pathsep + os.environ["PATH"]

fix_windows_path()

# ─────────────────────────────────────────────────────────────────────────────
# GRUB bundled-tool bootstrap
# ─────────────────────────────────────────────────────────────────────────────

def ensure_grub_tools():
    """
    Download grub-2.06-for-windows.zip from GNU FTP and extract grub-mkrescue
    + required i386-pc modules into tools/grub/ if not already present.
    Returns path to grub-mkrescue.exe or None.
    """
    local_gmr = GRUB_TOOLS_DIR / "grub-mkrescue.exe"
    if local_gmr.exists():
        return str(local_gmr)

    if not GRUB_WIN_ZIP.exists():
        print(f"  -> Downloading GRUB 2.06 for Windows from GNU FTP (~12 MB)...")
        TOOLS_DIR.mkdir(parents=True, exist_ok=True)
        try:
            req = urllib.request.Request(GRUB_WIN_URL, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=120) as resp, open(GRUB_WIN_ZIP, "wb") as out:
                total = int(resp.headers.get("Content-Length", 0))
                downloaded = 0
                while True:
                    chunk = resp.read(65536)
                    if not chunk:
                        break
                    out.write(chunk)
                    downloaded += len(chunk)
                    if total:
                        pct = downloaded * 100 // total
                        print(f"\r     Progress: {pct}%", end="", flush=True)
            print()
        except Exception as e:
            print(f"  [!] Download failed: {e}")
            return None

    print(f"  -> Extracting GRUB tools to {GRUB_TOOLS_DIR}...")
    GRUB_TOOLS_DIR.mkdir(parents=True, exist_ok=True)
    try:
        with zipfile.ZipFile(GRUB_WIN_ZIP, "r") as zf:
            for member in zf.namelist():
                # Extract grub-mkrescue.exe, grub-bios-setup.exe and the i386-pc module directory
                basename = member.split("/")[-1]
                if member.endswith("grub-mkrescue.exe") or member.endswith("grub-bios-setup.exe"):
                    zf.extract(member, GRUB_TOOLS_DIR / "_raw")
                    src = GRUB_TOOLS_DIR / "_raw" / member
                    dst = GRUB_TOOLS_DIR / basename
                    shutil.copy2(src, dst)
                elif "/i386-pc/" in member and not member.endswith("/"):
                    rel = "/".join(member.split("/")[1:])   # strip top-level dir
                    dst = GRUB_TOOLS_DIR / rel
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    data = zf.read(member)
                    dst.write_bytes(data)
                elif "/locale/" in member or "/themes/" in member:
                    pass  # skip bloat
                elif not member.endswith("/"):
                    rel = "/".join(member.split("/")[1:])
                    dst = GRUB_TOOLS_DIR / rel
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    data = zf.read(member)
                    dst.write_bytes(data)
        # Clean temp dir
        raw = GRUB_TOOLS_DIR / "_raw"
        if raw.exists():
            shutil.rmtree(raw, ignore_errors=True)
        if local_gmr.exists():
            print(f"  [OK] grub-mkrescue extracted -> {local_gmr}")
            fix_windows_path()
            return str(local_gmr)
    except Exception as e:
        print(f"  [!] Extraction failed: {e}")
    return None

def ensure_cross_compiler():
    """
    Download and extract x86_64-elf-gcc cross-compiler for Windows.
    Extracts to tools/cross/. Returns path to x86_64-elf-gcc.exe or None.
    """
    local_gcc = CROSS_DIR / "bin" / "x86_64-elf-gcc.exe"
    if local_gcc.exists():
        return str(local_gcc)

    if not CROSS_WIN_ZIP.exists():
        print(f"  -> Downloading x86_64-elf cross-compiler (~30 MB)...")
        TOOLS_DIR.mkdir(parents=True, exist_ok=True)
        try:
            req = urllib.request.Request(CROSS_WIN_URL, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=300) as resp, open(CROSS_WIN_ZIP, "wb") as out:
                total = int(resp.headers.get("Content-Length", 0))
                downloaded = 0
                while True:
                    chunk = resp.read(65536)
                    if not chunk:
                        break
                    out.write(chunk)
                    downloaded += len(chunk)
                    if total:
                        pct = downloaded * 100 // total
                        print(f"\r     Progress: {pct}%", end="", flush=True)
            print()
        except Exception as e:
            print(f"  [!] Cross-compiler download failed: {e}")
            return None

    print(f"  -> Extracting cross-compiler to {CROSS_DIR}...")
    CROSS_DIR.mkdir(parents=True, exist_ok=True)
    try:
        with zipfile.ZipFile(CROSS_WIN_ZIP, "r") as zf:
            # The zip may have files at root or inside a subdirectory
            members = zf.namelist()
            # Detect if there's a top-level folder
            top_dirs = set(m.split("/")[0] for m in members if "/" in m)
            has_subdir = len(top_dirs) == 1
            top = list(top_dirs)[0] if has_subdir else ""

            for member in members:
                if member.endswith("/"):
                    continue
                if has_subdir and member.startswith(top + "/"):
                    rel = member[len(top) + 1:]
                else:
                    rel = member
                if not rel:
                    continue
                dst = CROSS_DIR / rel
                dst.parent.mkdir(parents=True, exist_ok=True)
                data = zf.read(member)
                dst.write_bytes(data)

        if local_gcc.exists():
            print(f"  [OK] x86_64-elf-gcc extracted -> {local_gcc}")
            fix_windows_path()
            return str(local_gcc)
        else:
            # Check alternative layout
            for candidate in CROSS_DIR.rglob("x86_64-elf-gcc.exe"):
                print(f"  [OK] x86_64-elf-gcc found at -> {candidate}")
                return str(candidate)
    except Exception as e:
        print(f"  [!] Cross-compiler extraction failed: {e}")
    return None

# ─────────────────────────────────────────────────────────────────────────────
# CLI & helpers
# ─────────────────────────────────────────────────────────────────────────────


def print_banner():
    print("=" * 60)
    print("        Falkon-OS 64-bit Operating System Build Driver       ")
    print("=" * 60)

def print_help():
    print("Usage: python build.py [-setup] [-build [dev|release] [-save]] [-clean [dev|release|all]] [-run [vbox|qemu]] [-check]")
    print("\nOptions:")
    print("  -setup                1-Click automated zero-friction environment setup")
    print("  -build [dev|release]  Compiles Falkon-OS kernel & generates bootable FalkonOS.iso.")
    print("                        Rotates and preserves last 3 builds in falkon_[os]_[mode]_1/2/3.")
    print("                        - dev     -> Debug build (default)")
    print("                        - release -> Optimized build")
    print("                        -save     -> Permanently saves a timestamped snapshot")
    print("\n  -clean [dev|release|all] Removes rotated build directories and ISO artifacts.")
    print("                        (Note: permanently saved -save snapshots are preserved)")
    print("\n  -run [vbox|qemu]      Builds ISO and launches in VirtualBox or QEMU emulator.")
    print("\n  -check                Validates toolchain dependencies")
    print("\nExamples:")
    print("  python build.py -build")
    print("  python build.py -build -save")
    print("  python build.py -build release -save")
    print("  python build.py -run vbox")
    print("  python build.py -run qemu")
    print("  python build.py -clean all")
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
    raw_tokens = [a.lower() for a in args[1:]]
    config = {
        "show_help": False,
        "do_check": False,
        "do_clean": False,
        "clean_mode": "dev",
        "do_build": False,
        "build_mode": "dev",
        "do_save": False,
        "do_run": False,
        "run_mode": "vbox",
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
        elif token in ["-save", "--save", "save"]:
            config["do_save"] = True
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

def detect_os_type():
    system = platform.system().lower()
    if "linux" in system:
        if os.path.exists("/proc/version"):
            try:
                with open("/proc/version", "r") as f:
                    if "microsoft" in f.read().lower():
                        return "wsl"
            except Exception:
                pass
        if os.path.exists("/etc/os-release"):
            try:
                with open("/etc/os-release", "r") as f:
                    content = f.read().lower()
                    if "ubuntu" in content or "debian" in content or "mint" in content:
                        return "ubuntu"
                    elif "fedora" in content or "rhel" in content or "centos" in content:
                        return "fedora"
                    elif "arch" in content or "manjaro" in content:
                        return "arch"
                    elif "alpine" in content:
                        return "alpine"
            except Exception:
                pass
        return "linux"
    elif "windows" in system:
        return "windows"
    elif "darwin" in system:
        return "mac"
    return "unknown"

def get_os_display_name(os_type):
    names = {
        "wsl": "WSL2 (Windows Subsystem for Linux - Ubuntu)",
        "ubuntu": "Linux (Ubuntu / Debian / Mint)",
        "fedora": "Linux (Fedora / RHEL)",
        "arch": "Linux (Arch Linux / Manjaro)",
        "alpine": "Linux (Alpine Linux)",
        "linux": "Generic Linux",
        "windows": "Windows (Native Command Prompt / PowerShell)",
        "mac": "macOS",
        "unknown": "Unknown OS",
    }
    return names.get(os_type, "Unknown OS")

# ─────────────────────────────────────────────────────────────────────────────
# Check
# ─────────────────────────────────────────────────────────────────────────────

def run_check():
    os_type = detect_os_type()
    os_name = get_os_display_name(os_type)
    print(f"\n[CHECK] Detected Operating System: {os_name}")
    print("[CHECK] Verifying Toolchain & Build Dependencies...")

    has_cmake  = shutil.which("cmake") is not None
    has_ninja  = shutil.which("ninja") is not None
    has_nasm   = shutil.which("nasm") is not None
    has_gcc    = shutil.which("gcc") is not None
    has_ld     = shutil.which("ld") is not None
    has_grub   = find_grub_mkrescue() is not None
    has_qemu   = shutil.which("qemu-system-x86_64") is not None or os.path.exists(r"C:\Program Files\qemu\qemu-system-x86_64.exe")
    has_vbox   = bool(shutil.which("VBoxManage") or shutil.which("vboxmanage") or os.path.exists(r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"))
    has_xorriso = shutil.which("xorriso") is not None
    has_msys2_xorriso = False
    msys_bash = find_msys2_bash()
    if msys_bash:
        msys_root = Path(msys_bash).parent.parent
        if (msys_root / "usr" / "bin" / "xorriso.exe").exists():
            has_msys2_xorriso = True

    def ok(label): print(f"  + [OK] {label}")
    def warn(label): print(f"  + [!] {label}")

    if has_gcc:   ok("GCC cross-compiler found")
    else:         warn("GCC not found")
    if has_nasm:  ok("NASM assembler found")
    else:         warn("NASM not found")
    if has_ld:    ok("GNU Linker found")
    if has_cmake: ok("CMake found")
    else:         warn("CMake not found — required for build")
    if has_ninja: ok("Ninja build generator found")
    if has_grub:  ok(f"grub-mkrescue found -> {find_grub_mkrescue()}")
    elif has_msys2_xorriso: ok("MSYS2 xorriso found (ISO creation fallback)")
    elif has_xorriso: ok("System xorriso found (ISO creation fallback)")
    else:         warn("No ISO creator found — run 'python build.py -setup'")
    if has_vbox:  ok("VirtualBox found")
    else:         warn("VirtualBox not found")
    if has_qemu:  ok("QEMU found")
    else:         warn("QEMU not found")

    print()
    return True

# ─────────────────────────────────────────────────────────────────────────────
# Clean
# ─────────────────────────────────────────────────────────────────────────────

def run_clean(os_tag, mode):
    print(f"\n[CLEAN] Purging target: '{mode}'...")
    dirs_to_remove = []
    files_to_remove = [ROOT_DIR / "FalkonOS.iso", ROOT_DIR / "FalkonOS.bin"]

    if mode == "dev":
        dirs_to_remove.extend([ROOT_DIR / "build", ROOT_DIR / "isodir"])
    elif mode == "release":
        dirs_to_remove.extend([ROOT_DIR / "build_release", ROOT_DIR / "isodir"])
    elif mode == "all":
        dirs_to_remove.extend([ROOT_DIR / "build", ROOT_DIR / "build_release", ROOT_DIR / "isodir"])
        # Also clean numbered slots (but NOT saved_ snapshots)
        for slot in range(1, 4):
            for m in ["dev", "release"]:
                d = ROOT_DIR / f"falkon_{os_tag}_{m}_{slot}"
                if d.exists():
                    dirs_to_remove.append(d)

    for d in dirs_to_remove:
        if d.exists():
            print(f"  -> Removing: {d.name}")
            shutil.rmtree(d, ignore_errors=True)
    for f in files_to_remove:
        if f.exists():
            print(f"  -> Removing: {f.name}")
            try:
                f.unlink()
            except Exception as e:
                print(f"     [!] {e}")
    print("[CLEAN] Done.")

# ─────────────────────────────────────────────────────────────────────────────
# ISO builder
# ─────────────────────────────────────────────────────────────────────────────

def build_bootable_iso(kernel_bin: Path, grub_cfg: Path, target_iso: Path) -> bool:
    """
    Create a bootable El-Torito ISO from kernel_bin + grub.cfg.
    Tries in order:
      1. System grub-mkrescue
      2. Bundled tools/grub/grub-mkrescue.exe  (auto-downloaded)
      3. MSYS2 grub-mkrescue (if installed)
      4. MSYS2 xorriso -as mkisofs (fallback — only works when grub modules exist on CD)
    Returns True on success.
    """
    isodir = target_iso.parent / "isodir"
    boot_dir = isodir / "boot"
    grub_dir = boot_dir / "grub"
    grub_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(kernel_bin, boot_dir / "FalkonOS.bin")
    if grub_cfg.exists():
        shutil.copy2(grub_cfg, grub_dir / "grub.cfg")

    # --- Try 1: find grub-mkrescue anywhere ---
    gmr = find_grub_mkrescue()

    if not gmr:
        # --- Try 2: auto-download & extract ---
        print("  -> grub-mkrescue not found. Bootstrapping bundled GRUB tools...")
        gmr = ensure_grub_tools()

    if gmr:
        print(f"  -> Building ISO with grub-mkrescue: {gmr}")
        env = os.environ.copy()
        # Ensure grub can find its prefix/modules from our tools dir
        grub_prefix = GRUB_TOOLS_DIR / "i386-pc"
        if grub_prefix.exists():
            env["GRUB_MKRESCUE_SED"] = str(GRUB_TOOLS_DIR)

        cmd = [gmr, "-o", str(target_iso), str(isodir)]
        # If using our extracted copy, explicitly pass lib dir
        if str(GRUB_TOOLS_DIR) in gmr:
            lib_dir = GRUB_TOOLS_DIR
            cmd = [gmr, f"--directory={lib_dir}", "-o", str(target_iso), str(isodir)]

        res = subprocess.run(cmd, env=env, capture_output=True, text=True)
        if target_iso.exists() and target_iso.stat().st_size > 32768:
            print(f"  [OK] Bootable ISO created: {target_iso}")
            return True
        else:
            print(f"  [!] grub-mkrescue failed:\n{res.stderr or res.stdout}")

    # --- Try 3: MSYS2 xorriso with El Torito + embedded grub modules ---
    msys_bash = find_msys2_bash()
    if msys_bash:
        print("  -> Trying MSYS2 xorriso with El Torito boot record...")
        msys_iso = to_msys_path(target_iso)
        msys_dir = to_msys_path(isodir)

        # xorriso -as mkisofs approach with El Torito flags
        # We embed grub2's cdboot.img if we have it bundled, otherwise basic data ISO
        cdboot = GRUB_TOOLS_DIR / "i386-pc" / "cdboot.img"
        core_img = GRUB_TOOLS_DIR / "i386-pc" / "eltorito.img"

        if cdboot.exists():
            msys_cdboot = to_msys_path(cdboot)
            xorriso_cmd = (
                f"xorriso -as mkisofs "
                f"-R -J "
                f"-b boot/grub/i386-pc/cdboot.img "
                f"-no-emul-boot -boot-load-size 4 -boot-info-table "
                f"-o '{msys_iso}' '{msys_dir}'"
            )
        else:
            # Pure data ISO — not bootable by BIOS, but QEMU -kernel still works
            xorriso_cmd = f"xorriso -as mkisofs -R -J -o '{msys_iso}' '{msys_dir}'"

        res = subprocess.run([msys_bash, "-l", "-c", xorriso_cmd], capture_output=True, text=True)
        if target_iso.exists() and target_iso.stat().st_size > 32768:
            if cdboot.exists():
                print(f"  [OK] Bootable ISO created via MSYS2 xorriso: {target_iso}")
            else:
                print(f"  [!] Data-only ISO created (no boot image). QEMU -kernel still works.")
            return target_iso.exists()
        else:
            print(f"  [!] MSYS2 xorriso failed:\n{res.stderr or res.stdout}")

    print("  [!] ERROR: Could not create bootable ISO.")
    print("      Run 'python build.py -setup' to install required tools.")
    return False

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

def safe_copy(src, dst):
    if not src or not src.exists() or Path(src) == Path(dst):
        return True
    try:
        shutil.copy2(src, dst)
        return True
    except OSError as e:
        if platform.system().lower() == "windows":
            try:
                subprocess.run(
                    "taskkill /F /IM qemu-system-x86_64.exe",
                    shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                )
                shutil.copy2(src, dst)
                return True
            except Exception:
                pass
        print(f"  [!] Warning: Could not copy {Path(src).name}: {e}")
        return False

def rotate_builds(os_tag, mode, iso_src, do_save):
    base_name = f"falkon_{os_tag}_{mode}"
    dir_1 = ROOT_DIR / f"{base_name}_1"
    dir_2 = ROOT_DIR / f"{base_name}_2"
    dir_3 = ROOT_DIR / f"{base_name}_3"

    print(f"\n[STAGE] Rotating build history (keeping last 3 builds)...")

    if dir_3.exists():
        print(f"  -> Purging oldest slot 3: {dir_3.name}")
        shutil.rmtree(dir_3, ignore_errors=True)
    if dir_2.exists():
        print(f"  -> Shift slot 2->3: {dir_2.name} -> {dir_3.name}")
        dir_2.rename(dir_3)
    if dir_1.exists():
        print(f"  -> Shift slot 1->2: {dir_1.name} -> {dir_2.name}")
        dir_1.rename(dir_2)

    dir_1.mkdir(parents=True, exist_ok=True)

    if iso_src and iso_src.exists():
        safe_copy(iso_src, dir_1 / "FalkonOS.iso")
        safe_copy(iso_src, ROOT_DIR / "FalkonOS.iso")
        print(f"  + Staged (Slot 1): {dir_1 / 'FalkonOS.iso'}")
    else:
        print("[!] ERROR: FalkonOS.iso was not generated.")
        sys.exit(1)

    if do_save:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        saved_dir = ROOT_DIR / f"{base_name}_saved_{timestamp}"
        saved_dir.mkdir(parents=True, exist_ok=True)
        safe_copy(iso_src, saved_dir / "FalkonOS.iso")
        print(f"  + [SAVE] Snapshot -> {saved_dir.name}")

    print(f"\n[SUCCESS] FalkonOS.iso ready -> {dir_1 / 'FalkonOS.iso'}")

def run_build(os_tag, mode, do_save=False):
    print(f"\n[BUILD] Compiling Falkon-OS in '{mode.upper()}' mode for {os_tag}...")

    build_type = "Debug" if mode == "dev" else "Release"
    build_dir  = ROOT_DIR / ("build" if mode == "dev" else "build_release")

    if not shutil.which("cmake"):
        print("[!] ERROR: CMake is required. Run 'python build.py -setup'.")
        sys.exit(1)

    print("[BUILD] Using CMake + Ninja...")
    cmake_cfg = ["cmake", "-B", str(build_dir)]
    if shutil.which("ninja"):
        cmake_cfg += ["-G", "Ninja"]
    cmake_cfg.append(f"-DCMAKE_BUILD_TYPE={build_type}")

    print(f"> {' '.join(cmake_cfg)}")
    res = subprocess.run(cmake_cfg, cwd=ROOT_DIR)
    if res.returncode != 0:
        print("[!] CMake configuration failed.")
        sys.exit(1)

    build_cmd = ["cmake", "--build", str(build_dir), "--config", build_type]
    print(f"> {' '.join(build_cmd)}")
    res = subprocess.run(build_cmd, cwd=ROOT_DIR)
    if res.returncode != 0:
        print("[!] Compilation failed.")
        sys.exit(1)

    # ── The compiled kernel lives here
    bin_src = build_dir / "FalkonOS.bin"
    if not bin_src.exists():
        print("[!] ERROR: FalkonOS.bin not found after build.")
        sys.exit(1)

    # ── Generate the bootable ISO
    target_iso = build_dir / "FalkonOS.iso"
    grub_cfg   = ROOT_DIR / "src" / "arch" / "x86_64" / "boot" / "grub.cfg"

    print("\n[BUILD] Generating bootable FalkonOS.iso...")
    ok = build_bootable_iso(bin_src, grub_cfg, target_iso)

    if not ok or not target_iso.exists():
        print("[!] ISO generation failed. Build aborted.")
        sys.exit(1)

    rotate_builds(os_tag, mode, target_iso, do_save)

# ─────────────────────────────────────────────────────────────────────────────
# VirtualBox / QEMU runner
# ─────────────────────────────────────────────────────────────────────────────

def find_vboxmanage():
    for name in ["VBoxManage", "vboxmanage"]:
        p = shutil.which(name)
        if p:
            return p
    std = r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
    if os.path.exists(std):
        return f'"{std}"'
    return None

def run_vm(os_tag, mode):
    iso_path = ROOT_DIR / "FalkonOS.iso"
    bin_path  = ROOT_DIR / "FalkonOS.bin"

    if not iso_path.exists() and not bin_path.exists():
        print("\n[!] Build artifacts not found. Triggering build...")
        run_build(os_tag, "dev")

    if mode == "vbox":
        if not iso_path.exists():
            print("[!] ERROR: FalkonOS.iso is required for VirtualBox.")
            sys.exit(1)

        vbox = find_vboxmanage()
        if not vbox:
            print("[!] ERROR: VirtualBox (VBoxManage) not found.")
            sys.exit(1)

        print(f"\n[RUN] Launching in VirtualBox...")
        vm_name = "FalkonOS"

        def exec_vbox(cmd_str):
            subprocess.run(f"{vbox} {cmd_str}", shell=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        vms = subprocess.check_output(f"{vbox} list vms", shell=True).decode("utf-8", errors="ignore")
        if f'"{vm_name}"' in vms:
            exec_vbox(f'controlvm "{vm_name}" poweroff')
            exec_vbox(f'unregistervm "{vm_name}" --delete')

        exec_vbox(f'createvm --name "{vm_name}" --ostype "Linux26_64" --register')
        exec_vbox(f'modifyvm "{vm_name}" --memory 512 --vram 16 --graphicscontroller vmsvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2')
        exec_vbox(f'storagectl "{vm_name}" --name "IDE Controller" --add ide')
        exec_vbox(f'storageattach "{vm_name}" --name "IDE Controller" --port 0 --device 0 --type dvddrive --medium "{iso_path}"')
        exec_vbox(f'startvm "{vm_name}"')
        print("[SUCCESS] Falkon-OS running in VirtualBox!")

    elif mode == "qemu":
        qemu_bin = shutil.which("qemu-system-x86_64")
        if not qemu_bin:
            std = r"C:\Program Files\qemu\qemu-system-x86_64.exe"
            if os.path.exists(std):
                qemu_bin = std

        if not qemu_bin:
            print("[!] ERROR: qemu-system-x86_64 not found.")
            sys.exit(1)

        if iso_path.exists():
            print(f"\n[RUN] Booting FalkonOS.iso in QEMU...")
            subprocess.run([qemu_bin, "-cdrom", str(iso_path), "-m", "512M", "-vga", "std"])
        elif bin_path.exists():
            print(f"\n[RUN] Booting FalkonOS.bin via QEMU direct kernel (no ISO)...")
            subprocess.run([qemu_bin, "-kernel", str(bin_path), "-m", "512M", "-vga", "std"])
        else:
            print("[!] Neither FalkonOS.iso nor FalkonOS.bin found. Run 'python build.py -build'.")

# ─────────────────────────────────────────────────────────────────────────────
# Setup
# ─────────────────────────────────────────────────────────────────────────────

def run_setup():
    os_type = detect_os_type()
    os_name = get_os_display_name(os_type)
    print(f"\n[SETUP] Starting automated setup for {os_name}...")

    if os_type == "windows":
        if not shutil.which("winget"):
            print("[!] ERROR: 'winget' not found. Please install tools manually.")
            return False

        tools_to_install = [
            ("MSYS2 Environment",      "MSYS2.MSYS2"),
            ("NASM Assembler",         "NASM.NASM"),
            ("CMake Build Engine",     "Kitware.CMake"),
            ("Ninja Build Generator",  "Ninja-build.Ninja"),
            ("VirtualBox VM",          "Oracle.VirtualBox"),
            ("QEMU Emulator",          "SoftwareFreedomConservancy.QEMU"),
        ]
        for name, pkg_id in tools_to_install:
            print(f"  -> Installing {name} ({pkg_id})...")
            cmd = ["winget", "install", pkg_id,
                   "--accept-source-agreements", "--accept-package-agreements", "--silent"]
            res = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print(f"     {'[OK]' if res.returncode == 0 else '[already installed or skipped]'} {name}")

        # Populate MSYS2 with ISO creation tools
        msys_bash = find_msys2_bash()
        if msys_bash:
            print(f"  -> Installing xorriso + build tools in MSYS2...")
            subprocess.run(
                [msys_bash, "-l", "-c",
                 "pacman -S --noconfirm --needed msys/xorriso "
                 "mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-nasm "
                 "mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja"],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )

        # Bootstrap bundled GRUB tools (downloads grub-2.06-for-windows.zip)
        print("  -> Bootstrapping bundled GRUB 2.06 tools for ISO creation...")
        gmr = ensure_grub_tools()
        if gmr:
            print(f"  [OK] grub-mkrescue ready: {gmr}")
        else:
            print("  [!] GRUB bootstrap failed. ISO creation will fall back to xorriso.")

        fix_windows_path()
        print("\n[SUCCESS] Setup complete! Run 'python build.py -check' to verify.")
        return True

    elif os_type in ["ubuntu", "wsl"]:
        cmd = ("sudo apt update && sudo apt install -y "
               "build-essential cmake nasm grub-pc-bin grub-common xorriso mtools "
               "qemu-system-x86")
        res = subprocess.run(cmd, shell=True)
        return res.returncode == 0

    elif os_type == "arch":
        cmd = "sudo pacman -S --needed base-devel cmake nasm grub xorriso mtools qemu-system-x86"
        res = subprocess.run(cmd, shell=True)
        return res.returncode == 0

    elif os_type == "fedora":
        cmd = "sudo dnf install -y gcc nasm cmake ninja-build grub2-tools-extra xorriso mtools qemu-system-x86"
        res = subprocess.run(cmd, shell=True)
        return res.returncode == 0

    print(f"[!] Automated setup not supported for '{os_type}'. See 'python build.py -check'.")
    return False

# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    print_banner()
    os_tag = get_os_tag()

    if "-setup" in sys.argv or "-install" in sys.argv:
        run_setup()
        sys.exit(0)

    config = parse_cli_args(sys.argv)

    if config["show_help"]:
        print_help()
        sys.exit(0)

    if config["do_check"]:
        run_check()

    if config["do_clean"]:
        run_clean(os_tag, config["clean_mode"])

    if config["do_build"]:
        run_build(os_tag, config["build_mode"], config["do_save"])

    if config["do_run"]:
        run_vm(os_tag, config["run_mode"])

if __name__ == "__main__":
    main()
