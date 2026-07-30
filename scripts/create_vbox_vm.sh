#!/usr/bin/env bash
set -e

VM_NAME="FalkonOS"
ISO_PATH="$(pwd)/FalkonOS.iso"

if [ ! -f "$ISO_PATH" ]; then
    echo "[!] FalkonOS.iso not found! Building first..."
    make all
fi

echo "=========================================="
echo "    VirtualBox Automated Setup (Falkon-OS)"
echo "=========================================="

# Check VBoxManage
if ! command -v VBoxManage &> /dev/null; then
    echo "[ERROR] VBoxManage command not found."
    echo "Please install Oracle VM VirtualBox and ensure VBoxManage is in your PATH."
    exit 1
fi

# Check if VM already exists
if VBoxManage list vms | grep -q "\"$VM_NAME\""; then
    echo "[*] Existing VM '$VM_NAME' found. Removing previous instance..."
    VBoxManage controlvm "$VM_NAME" poweroff 2>/dev/null || true
    sleep 1
    VBoxManage unregistervm "$VM_NAME" --delete 2>/dev/null || true
fi

echo "[1/4] Registering new VirtualBox VM '$VM_NAME'..."
VBoxManage createvm --name "$VM_NAME" --ostype "Linux26_64" --register

echo "[2/4] Configuring VM settings (512MB RAM, VMSVGA Graphics, PS/2 Input)..."
VBoxManage modifyvm "$VM_NAME" --memory 512 --vram 16 --graphicscontroller vmsvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2

echo "[3/4] Attaching FalkonOS.iso to IDE Controller..."
VBoxManage storagectl "$VM_NAME" --name "IDE Controller" --add ide
VBoxManage storageattach "$VM_NAME" --name "IDE Controller" --port 0 --device 0 --type dvddrive --medium "$ISO_PATH"

echo "[4/4] Launching VirtualBox VM..."
VBoxManage startvm "$VM_NAME"

echo "=========================================="
echo "  [SUCCESS] Falkon-OS is now running in VirtualBox!"
echo "=========================================="
