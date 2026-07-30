@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo    VirtualBox Automated Setup (Windows)
echo ==========================================

set VM_NAME=FalkonOS
set ISO_PATH=%~dp0..\FalkonOS.iso

if not exist "%ISO_PATH%" (
    echo [!] FalkonOS.iso not found in project root!
    echo Please compile or copy FalkonOS.iso into the root directory before running.
    pause
    exit /b 1
)

:: Find VBoxManage executable location
set VBOX_CMD="C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
if not exist %VBOX_CMD% (
    where VBoxManage >nul 2>nul
    if %errorlevel% equ 0 (
        set VBOX_CMD=VBoxManage
    ) else (
        echo [ERROR] VirtualBox not found at standard path!
        echo Please install VirtualBox from https://www.virtualbox.org/
        pause
        exit /b 1
    )
)

echo [*] Checking existing VM status...
%VBOX_CMD% list vms | findstr /c:"\"%VM_NAME%\"" >nul 2>&1
if %errorlevel% equ 0 (
    echo [*] Removing existing VirtualBox VM instance...
    %VBOX_CMD% controlvm "%VM_NAME%" poweroff >nul 2>&1
    timeout /t 1 >nul
    %VBOX_CMD% unregistervm "%VM_NAME%" --delete >nul 2>&1
)

echo [1/4] Creating VM '%VM_NAME%'...
%VBOX_CMD% createvm --name "%VM_NAME%" --ostype "Linux26_64" --register

echo [2/4] Configuring VM settings (512MB RAM, VMSVGA Graphics)...
%VBOX_CMD% modifyvm "%VM_NAME%" --memory 512 --vram 16 --graphicscontroller vmsvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2

echo [3/4] Mounting FalkonOS.iso...
%VBOX_CMD% storagectl "%VM_NAME%" --name "IDE Controller" --add ide
%VBOX_CMD% storageattach "%VM_NAME%" --name "IDE Controller" --port 0 --device 0 --type dvddrive --medium "%ISO_PATH%"

echo [4/4] Starting VirtualBox VM...
%VBOX_CMD% startvm "%VM_NAME%"

echo ==========================================
echo   [SUCCESS] Falkon-OS launched in VirtualBox!
echo ==========================================
pause
