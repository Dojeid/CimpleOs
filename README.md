<div align="center">

# 🦅 Falkon-OS
### A 64-bit Educational Hobby Operating System with Custom Desktop GUI & Window Manager

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86__64-orange.svg)
![Mode](https://img.shields.io/badge/Mode-64--bit_Long_Mode-purple.svg)
![VirtualBox Ready](https://img.shields.io/badge/VirtualBox-Recommended-brightgreen.svg)
![Build](https://img.shields.io/badge/Build-Python_%7C_GCC_%7C_NASM_%7C_GRUB-blue.svg)

---

[📖 Explore Interactive HTML Docs](docs/index.html) • [🪟 Windows Setup](#1-🪟-windows-setup-guide-windows-first) • [🐧 Linux Setup](#2-🐧-linux-setup-guide-linux-second) • [⚙️ WSL Setup](#3-⚙️-wsl2-setup-guide-wsl-third) • [🐍 Singular Build Script Explained](#-singular-unified-python-script-buildpy-explained)

</div>

---

## 📌 Overview

**Falkon-OS** is a modern 64-bit operating system written from scratch in **C** and **NASM assembly**. It features a full custom 32-bit color **Framebuffer Desktop GUI**, real-time **Window Manager**, interactive **Terminal Shell**, **Physical & Virtual Memory Management (PMM/VMM)**, and hardware drivers for PS/2 input and VBE graphics.

---

## 🛠️ Complete Installation & Setup Guide

Below are clear, step-by-step setup guides organized by operating system: **Windows FIRST**, **Linux SECOND**, and **WSL THIRD**.

---

### 1. 🪟 Windows Setup Guide (Windows First)

Windows is the primary supported environment for building and running Falkon-OS using VirtualBox and Python.

#### Tools Needed on Windows:
1. **Python 3.8+**: Download from [python.org](https://www.python.org/downloads/) (Check *"Add python.exe to PATH"* during installation).
2. **Oracle VM VirtualBox (v7.0+)**: Download from [virtualbox.org](https://www.virtualbox.org/).
3. **Git for Windows**: Download from [git-scm.com](https://git-scm.com/).
4. **Compiler Toolchain (Choice of WSL2 or MSYS2)**:
   - **Option A (WSL2 - Recommended)**: Allows native Linux compilation with VirtualBox launch on Windows.
   - **Option B (MSYS2)**: Installs `nasm`, `gcc`, and `make` natively on Windows.

#### Step-by-Step Compilation & VM Creation on Windows:

1. **Clone the Repository**:
   Open Command Prompt (`cmd`) or PowerShell:
   ```cmd
   git clone https://github.com/Saravanan-Codez/Falkon-Os.git
   cd Falkon-Os
   ```

2. **One-Click Build & VirtualBox Launch**:
   Run the singular unified Python script:
   ```cmd
   python build.py --vbox
   ```
   *This script automatically verifies dependencies, compiles the kernel, generates `FalkonOS.iso`, creates a VirtualBox VM named "FalkonOS", configures 512MB RAM + VMSVGA graphics, attaches the ISO, and boots the OS!*

3. **Manual VirtualBox Setup (Alternative)**:
   - Open VirtualBox ➔ Click **New**.
   - **Name**: `FalkonOS` | **Type**: `Linux` | **Version**: `Other Linux (64-bit)`.
   - **Memory**: `512 MB` | **Hard Disk**: Select *"Do not add a virtual hard disk"*.
   - **Display Settings**: Set **Video Memory** to `16 MB` and **Graphics Controller** to `VMSVGA`.
   - **Storage**: Attach `FalkonOS.iso` to the Optical Drive.
   - Click **Start 🟢**!

---

### 2. 🐧 Linux Setup Guide (Linux Second)

Linux provides native compilation tools out of the box.

#### Tools Needed on Linux:
- `gcc` (Host C compiler)
- `nasm` (x86_64 Assembly assembler)
- `ld` (GNU Linker)
- `grub-mkrescue`, `xorriso`, `mtools` (ISO image creation utilities)
- `python3` (Python environment runner)
- `virtualbox` or `qemu-system-x86` (Virtual Machine emulators)

#### Step-by-Step Tool Installation by Package Manager:

- **Ubuntu / Debian / Mint**:
  ```bash
  sudo apt update
  sudo apt install -y python3 build-essential nasm grub-pc-bin grub-common xorriso mtools virtualbox qemu-system-x86
  ```

- **Fedora / RHEL**:
  ```bash
  sudo dnf groupinstall -y "Development Tools"
  sudo dnf install -y python3 gcc nasm grub2-tools-extra xorriso mtools virtualbox qemu-system-x86
  ```

- **Arch Linux / Manjaro**:
  ```bash
  sudo pacman -S --needed python base-devel nasm grub xorriso mtools virtualbox qemu-desktop
  ```

- **Alpine Linux**:
  ```bash
  sudo apk add python3 build-base nasm grub grub-efi xorriso mtools qemu-system-x86_64
  ```

#### Step-by-Step Compilation & VM Launch on Linux:

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Saravanan-Codez/Falkon-Os.git
   cd Falkon-Os
   ```

2. **Build ISO & Launch in VirtualBox**:
   ```bash
   python3 build.py --vbox
   ```

3. **Or Launch in QEMU**:
   ```bash
   python3 build.py --qemu
   ```

---

### 3. ⚙️ WSL2 Setup Guide (WSL Third)

WSL2 (Windows Subsystem for Linux) allows you to compile Falkon-OS using Ubuntu inside Windows while running the generated ISO in VirtualBox on Windows.

#### Step-by-Step WSL Setup:

1. **Install WSL2 on Windows**:
   Open PowerShell as Administrator:
   ```powershell
   wsl --install -d Ubuntu
   ```
   *Restart your computer if prompted.*

2. **Install Build Dependencies in WSL**:
   Launch **Ubuntu** from your Start menu and run:
   ```bash
   sudo apt update && sudo apt install -y python3 build-essential nasm grub-pc-bin grub-common xorriso mtools
   ```

3. **Navigate to your Project Directory**:
   WSL mounts your Windows drives under `/mnt/`:
   ```bash
   cd /mnt/d/Falkon_labs/Falkon-Os
   ```

4. **Compile the ISO in WSL**:
   ```bash
   python3 build.py
   ```

5. **Launch in VirtualBox on Windows**:
   Open PowerShell in Windows and run:
   ```cmd
   python build.py --vbox
   ```

---

## 🐍 Singular Unified Python Script (`build.py`) Explained

Rather than relying on fragmented shell scripts or complex command combinations, Falkon-OS includes a single, cross-platform Python script in the repository root: **`build.py`**.

### What `build.py` Does (Step-by-Step Explanation):

```
                       ┌──────────────────────────────┐
                       │       python build.py        │
                       └──────────────┬───────────────┘
                                      │
           ┌──────────────────────────┴──────────────────────────┐
           ▼                                                     ▼
┌──────────────────────┐                             ┌──────────────────────┐
│  check_dependencies  │                             │      clean()         │
│ Checks gcc, nasm, ld │                             │ Removes build/,      │
│ & grub-mkrescue path │                             │ isodir/, FalkonOS.iso│
└──────────┬───────────┘                             └──────────────────────┘
           │
           ▼
┌──────────────────────┐
│     build_iso()      │
│ 1. Assembles boot.asm│
│ 2. Compiles C files  │
│ 3. Links FalkonOS.bin│
│ 4. Calls grub-rescue │
└──────────┬───────────┘
           │
           ├──────────────────────────┐
           ▼                          ▼
┌──────────────────────┐    ┌──────────────────────┐
│     run_vbox()       │    │      run_qemu()      │
│ 1. Locates VBoxManage│    │ Launches QEMU with   │
│ 2. Creates Linux VM  │    │ -cdrom FalkonOS.iso  │
│ 3. Configures 512MB  │    │ -m 512M -vga std     │
│ 4. Mounts ISO & runs │    └──────────────────────┘
└──────────────────────┘
```

1. **`check_dependencies()`**:
   Inspects your operating system's `PATH` to ensure `gcc`, `nasm`, `ld`, and `grub-mkrescue` are present before attempting compilation.
2. **`clean()`**:
   Safely removes temporary object files, compiled binaries (`build/`), GRUB ISO staging folders (`isodir/`), and `FalkonOS.iso`.
3. **`build_iso()`**:
   Executes the build system pipeline:
   - Assembles 64-bit bootloader assembly (`src/arch/x86_64/boot/boot.asm`).
   - Compiles kernel C sources (`src/kernel/`, `src/mm/`, `src/drivers/`, `src/gui/`, `src/lib/`).
   - Links the kernel binary into `build/FalkonOS.bin`.
   - Invokes `grub-mkrescue` to package `FalkonOS.bin` and `grub.cfg` into bootable `FalkonOS.iso`.
4. **`run_vbox()`**:
   Automates VirtualBox VM setup via `VBoxManage`:
   - Checks if a VM named `"FalkonOS"` already exists and safely turns it off.
   - Registers a new VM configured for 64-bit Linux (`Linux26_64`).
   - Sets 512MB RAM, 16MB Video RAM, VMSVGA graphics controller, and PS/2 mouse/keyboard.
   - Creates a virtual IDE optical drive and mounts `FalkonOS.iso`.
   - Powers on the VM instantly (`startvm`).
5. **`run_qemu()`**:
   Alternative emulator launcher for developers testing via command line.

### CLI Usage Commands for `build.py`:

```bash
# Build the ISO image
python build.py

# Build ISO and launch in VirtualBox (Recommended)
python build.py --vbox

# Build ISO and launch in QEMU
python build.py --qemu

# Verify system dependencies
python build.py --check

# Clean all build artifacts
python build.py --clean
```

---

## ✨ Features & Upgraded Shell Commands

Falkon-OS comes equipped with an upgraded interactive terminal shell supporting new system commands:

| Command | Description | Example Output |
| :--- | :--- | :--- |
| `help` | Shows list of available terminal commands | Lists all commands |
| `fetch` / `neofetch` | Displays ASCII logo & system specs | 🦅 Falkon-OS v0.4 64-bit Long Mode |
| `uname` | Displays kernel release & architecture details | `Falkon-OS x86_64 0.4.0-generic Long_Mode` |
| `whoami` | Displays active user session | `root@falkon-os` |
| `calc <expr>` | Performs basic math operations | `calc 25 + 15` ➔ `Result: 40` |
| `echo <text>` | Echoes input text back to screen | `echo Hello OS World` |
| `theme <id>` | Switches wallpaper theme live | `theme 2` (Cyber Blue), `theme 3` (Forest) |
| `sysinfo` | Displays RAM memory map and CPU specs | PMM & VMM status |
| `time` | Displays uptime clock timer | `Uptime: 00h 02m 14s` |
| `clear` | Clears current terminal window screen | Cleared buffer |

---

## 📁 Repository Directory Structure

```
Falkon-Os/
├── .gitignore              # Build & VM artifact ignore rules
├── build.py                # Singular Unified Build & VM Python script
├── docs/                   # Interactive HTML Documentation Portal
│   ├── index.html          # Documentation homepage
│   ├── styles.css          # Theme styling
│   └── app.js              # Interactive tab & search logic
├── scripts/                # Auxiliary shell scripts
│   ├── build.sh            # Auxiliary Linux build script
│   ├── create_vbox_vm.sh   # Auxiliary VirtualBox launcher (Linux/macOS)
│   └── create_vbox_vm.bat  # Auxiliary VirtualBox launcher (Windows)
├── src/                    # Kernel & Driver Source Code
│   ├── arch/x86_64/        # Bootloader assembly & GDT/IDT tables
│   ├── drivers/            # VBE Video Framebuffer, PS/2 Mouse & Keyboard, PCI, USB
│   ├── gui/                # Desktop, Taskbar, Window Manager & Hardware Cursor
│   ├── include/            # Multiboot headers & embedded bitmap fonts
│   ├── kernel/             # main.c, Shell commands (cmd.c), CPUID, System Info
│   ├── lib/                # C standard library primitives (printf, string, io)
│   └── mm/                 # Physical (PMM), Virtual (VMM), and Heap allocators
├── configure               # Dependency validator script
├── Makefile                # Low-level Makefile
├── LICENSE                 # MIT License
└── README.md               # Main Documentation
```

---

## 🔧 Troubleshooting Matrix

| Issue | Root Cause | Solution |
| :--- | :--- | :--- |
| `grub-mkrescue: command not found` | Missing GRUB ISO tools. | Install `grub-pc-bin`, `xorriso`, and `mtools` via your package manager. |
| VirtualBox shows black screen | Low VRAM or wrong graphics driver. | Set VM Display Graphics Controller to `VMSVGA` or `VBoxVGA` and increase VRAM to 16MB. |
| `nasm: command not found` | NASM assembler is not installed. | Run `sudo apt install nasm` or `sudo dnf install nasm`. |
| Mouse cursor frozen in VM | Pointing device set to tablet/USB. | Change VirtualBox Settings ➔ System ➔ Motherboard Pointing Device to `PS/2 Mouse`. |

---

## ⚖️ License

Falkon-OS is released under the permissive [MIT License](LICENSE).

---

<div align="center">
  <sub>Built with ❤️ for operating systems enthusiasts and kernel developers.</sub>
</div>
