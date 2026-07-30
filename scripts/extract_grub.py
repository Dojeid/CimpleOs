#!/usr/bin/env python3
"""
Extract grub-2.06-for-windows.zip and test grub-mkrescue.
Run this after the zip is downloaded to tools/grub-win.zip
"""
import zipfile, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).parent.parent
TOOLS_DIR = ROOT / "tools"
GRUB_TOOLS = TOOLS_DIR / "grub"
ZIP = TOOLS_DIR / "grub-win.zip"

if not ZIP.exists():
    print(f"ERROR: {ZIP} not found. Download it first.")
    sys.exit(1)

print(f"Extracting {ZIP} -> {GRUB_TOOLS}")
GRUB_TOOLS.mkdir(parents=True, exist_ok=True)

# List top-level folder name in zip
with zipfile.ZipFile(ZIP) as zf:
    names = zf.namelist()
    top = names[0].split('/')[0]
    print(f"Top-level dir in zip: {top}")
    print(f"Total files: {len(names)}")
    
    # Count how many files contain key patterns
    gmr_files = [n for n in names if n.endswith('grub-mkrescue.exe') or n.endswith('grub-mkrescue')]
    i386_files = [n for n in names if '/i386-pc/' in n]
    print(f"grub-mkrescue candidates: {gmr_files[:5]}")
    print(f"i386-pc files: {len(i386_files)}")
    
    # Extract all to GRUB_TOOLS (stripping the top-level dir name)
    for member in names:
        if member.endswith('/'):
            continue
        parts = member.split('/', 1)
        if len(parts) < 2:
            continue
        rel = parts[1]
        dst = GRUB_TOOLS / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        data = zf.read(member)
        dst.write_bytes(data)

# Check what we got
gmr = GRUB_TOOLS / "grub-mkrescue.exe"
print(f"\ngrub-mkrescue.exe exists: {gmr.exists()}")
if gmr.exists():
    print(f"  Size: {gmr.stat().st_size} bytes")
    # Try running it
    res = subprocess.run([str(gmr), "--version"], capture_output=True, text=True)
    print(f"  Version: {(res.stdout or res.stderr).strip()}")

# Check for i386-pc modules
i386_dir = GRUB_TOOLS / "lib" / "grub" / "i386-pc"
if not i386_dir.exists():
    # Try another common layout
    for d in GRUB_TOOLS.rglob("i386-pc"):
        if d.is_dir():
            i386_dir = d
            break
print(f"\ni386-pc modules dir: {i386_dir}")
if i386_dir.exists():
    files = list(i386_dir.iterdir())
    print(f"  Files: {len(files)}")
    key_files = [f.name for f in files if f.name in ['cdboot.img', 'eltorito.img', 'boot.img', 'core.img']]
    print(f"  Key files: {key_files}")
