#!/usr/bin/env bash
set -e

echo "=========================================="
echo "           Building Falkon-OS             "
echo "=========================================="

# Check configure/tools
if [ -f "./configure" ]; then
    bash ./configure
fi

echo "Cleaning previous build artifacts..."
make clean || true

echo "Building kernel and generating FalkonOS.iso..."
make all

if [ -f "FalkonOS.iso" ]; then
    echo "=========================================="
    echo "  [SUCCESS] FalkonOS.iso generated successfully!"
    echo "  Size: $(du -h FalkonOS.iso | cut -f1)"
    echo "=========================================="
    echo "To try Falkon-OS in VirtualBox (Recommended):"
    echo "  bash ./scripts/create_vbox_vm.sh"
    echo "To try in QEMU:"
    echo "  make qemu"
else
    echo "[ERROR] ISO generation failed."
    exit 1
fi
