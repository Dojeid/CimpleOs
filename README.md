<div align="center">

# 🦅 Falkon-OS
### A 64-bit Educational Hobby Operating System with Custom Desktop GUI & Window Manager

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-x86__64-orange.svg)
![Mode](https://img.shields.io/badge/Mode-64--bit_Long_Mode-purple.svg)
![VirtualBox Ready](https://img.shields.io/badge/VirtualBox-Recommended-brightgreen.svg)
![Build System](https://img.shields.io/badge/Build_System-CMake_%7C_Python-blue.svg)

---

[📖 Explore Interactive HTML Docs](docs/index.html) • [🪟 Windows Setup](#1-🪟-windows-setup-guide-windows-first) • [🐧 Linux Setup](#2-🐧-linux-setup-guide-linux-second) • [⚙️ WSL Setup](#3-⚙️-wsl2-setup-guide-wsl-third) • [🐍 Cross-Platform Build Driver](#-cross-platform-build-driver-buildpy-explained)

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
2. **Oracle VM VirtualBox (v7.0+)**: Download from [virtualbox.org](https://www.virtualbox.org/) or install via `winget install Oracle.VirtualBox`.
3. **NASM Assembler**: Download from [nasm.us](https://www.nasm.us/) or install via `winget install NASM.NASM` or `choco install nasm -y`.
4. **Build Toolchain (MSYS2 / MinGW64)**: Installs `nasm`, `gcc`, `cmake`, `make`, and GRUB tools natively on Windows.

#### Step-by-Step Compilation & VM Creation on Windows:

1. **Clone the Repository**:
   Open Command Prompt (`cmd`) or PowerShell:
   ```cmd
   git clone https://github.com/Saravanan-Codez/Falkon-Os.git
   cd Falkon-Os
   ```

2. **One-Click Build & VirtualBox Launch**:
   Run the cross-platform driver script:
   ```cmd
   python build.py -run vbox
   ```
   *This script automatically verifies dependencies, compiles the kernel in dev mode, stages output in `falkon_win_dev`, creates a VirtualBox VM named "FalkonOS", configures 512MB RAM + VMSVGA graphics, attaches the ISO, and boots the OS!*

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
- `cmake` & `ninja` (Optional CMake build system)
- `nasm` (x86_64 Assembly assembler)
- `ld` (GNU Linker)
- `grub-mkrescue`, `xorriso`, `mtools` (ISO image creation utilities)
- `python3` (Python environment runner)
- `virtualbox` or `qemu-system-x86` (Virtual Machine emulators)

#### Step-by-Step Tool Installation by Package Manager:

- **Ubuntu / Debian / Mint**:
  ```bash
  sudo apt update
  sudo apt install -y python3 build-essential cmake ninja-build nasm grub-pc-bin grub-common xorriso mtools virtualbox qemu-system-x86
  ```

- **Fedora / RHEL**:
  ```bash
  sudo dnf groupinstall -y "Development Tools"
  sudo dnf install -y python3 gcc cmake ninja-build nasm grub2-tools-extra xorriso mtools virtualbox qemu-system-x86
  ```

- **Arch Linux / Manjaro**:
  ```bash
  sudo pacman -S --needed python base-devel cmake ninja nasm grub xorriso mtools virtualbox qemu-desktop
  ```

- **Alpine Linux**:
  ```bash
  sudo apk add python3 build-base cmake samu nasm grub grub-efi xorriso mtools qemu-system-x86_64
  ```

#### Step-by-Step Compilation & VM Launch on Linux:

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Saravanan-Codez/Falkon-Os.git
   cd Falkon-Os
   ```

2. **Build ISO & Launch in VirtualBox**:
   ```bash
   python3 build.py -run vbox
   ```

3. **Or Launch in QEMU**:
   ```bash
   python3 build.py -run qemu
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
   sudo apt update && sudo apt install -y python3 build-essential cmake ninja-build nasm grub-pc-bin grub-common xorriso mtools
   ```

3. **Navigate to your Project Directory**:
   WSL mounts your Windows drives under `/mnt/`:
   ```bash
   cd /mnt/d/Falkon_labs/Falkon-Os
   ```

4. **Compile the ISO in WSL**:
   ```bash
   python3 build.py -build
   ```

5. **Launch in VirtualBox on Windows**:
   Open PowerShell in Windows and run:
   ```cmd
   python build.py -run vbox
   ```

---

## 🐍 Cross-Platform Build Driver (`build.py`) Explained

Falkon-OS includes a single, case-insensitive, cross-platform driver script in the root directory: **`build.py`** (inspired by the Falkon Compiler toolchain architecture).

### Key Features & Workflow Rules:
- **Rule 1 (`-clean`)**: Cleaning targets ALWAYS execute BEFORE building.
- **Rule 2 (`-build`)**: Configures CMake (preferring Ninja) or Makefile, compiles the 64-bit kernel, generates `FalkonOS.iso`, and stages output.
- **Rule 3 (`-run`)**: Auto-detects VirtualBox (`-run vbox`) or QEMU (`-run qemu`), creates the VM, mounts the ISO, and boots.
- **Rule 4 (`-check`)**: Runs the smart environment diagnostic engine to detect compilers (GCC/Clang), NASM, CMake, Ninja, and VM emulators. Prompts for custom tool locations (e.g. custom MSYS2 / NASM paths), saves them persistently in `falkon.ini`, and outputs OS-tailored installation options.

```
Usage: python build.py [-build [dev|release]] [-clean [dev|release|all]] [-run [vbox|qemu]] [-check]

Examples:
  python build.py -check                     # Smart diagnostic check & custom path setup
  python build.py -build                     # Compiles kernel in dev mode
  python build.py -build release             # Compiles kernel in release mode
  python build.py -run vbox                  # Builds ISO & launches VirtualBox (Recommended)
  python build.py -run qemu                  # Builds ISO & launches QEMU
  python build.py -clean all                 # Removes all build & staging trees
  python build.py -clean dev -build dev -run vbox # Full clean, rebuild & launch workflow
```

---

## ✨ Upgraded Shell Commands

| Command | Description | Example Output |
| :--- | :--- | :--- |
| `help` | Shows list of available terminal commands | Lists all commands |
| `fetch` / `neofetch` | Displays ASCII logo & system specs | 🦅 Falkon-OS v0.4 64-bit Long Mode |
| `uname` | Displays kernel release & architecture details | `Falkon-OS x86_64 0.4.0-generic Long_Mode` |
| `whoami` | Displays active user session | `root@falkon-os` |
| `calc <expr>` | Performs basic math operations | `calc 25 + 15` ➔ `Result: 40` |
| `echo <text>` | Echoes input text back to screen | `echo Hello OS World` |
| `matrix` | Engages Matrix digital rain mode | Green binary rain |
| `theme <id>` | Switches wallpaper theme live | `theme 2` (Cyber Blue), `theme 3` (Forest) |
| `sysinfo` | Displays RAM memory map and CPU specs | PMM & VMM status |
| `time` | Displays uptime clock timer | `Uptime: 00h 02m 14s` |
| `clear` | Clears current terminal window screen | Cleared buffer |

---

## 📁 Repository Directory Structure

```
Falkon-Os/
├── .gitignore              # Build & VM artifact ignore rules
├── build.py                # Cross-Platform Driver Script
├── CMakeLists.txt          # Modern CMake Build Definition
├── Makefile                # Traditional GNU Makefile Definition
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
├── LICENSE                 # MIT License
└── README.md               # Main Documentation
```

---

## ⚖️ License

Falkon-OS is released under the permissive [MIT License](LICENSE).

---

<div align="center">
  <sub>Built with ❤️ for operating systems enthusiasts and kernel developers.</sub>
</div>
