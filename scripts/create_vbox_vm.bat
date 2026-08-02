@echo off
REM Falkon-OS VirtualBox VM Creation Script
REM Creates a VirtualBox VM for Falkon-OS and attaches the ISO
REM Compatible with Windows

REM Set error handling
if "%ERRORLEVEL%" NEQ 0 exit /b %ERRORLEVEL%

REM Configuration
SET SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..\set ISO_PATH=%ROOT_DIR%\out\FalkonOS.iso
set VM_NAME=FalkonOS
set VM_MEMORY=512
set VM_VRAM=16
set VM_CPU=2

REM Display help information
if "%1%"=="-h" goto :help
if "%1%"=="--help" goto :help

REM Check VirtualBox availability
if not exist "%PROGRAMFILES%\Oracle\VirtualBox\VBoxManage.exe" (
    echo Error: VirtualBox is not installed or not in PATH
    echo On Windows, please install VirtualBox from https://www.virtualbox.org
    exit /b 1
)

REM Check if ISO exists
if not exist "%ISO_PATH%" (
    echo Error: ISO not found at %ISO_PATH%
    echo Please build the ISO first using:
    echo   python3 %ROOT_DIR%\build.py -build
    echo   or
    echo   cd %ROOT_DIR% && python3 build.py -build
    exit /b 1
)

REM Create VirtualBox VM
echo Creating VM '%VM_NAME%'...
vboxmanage createvm --name "%VM_NAME%" --register --outputjson > nul

REM Set hardware characteristics
echo Configuring VM hardware...
vboxmanage modifyvm "%VM_NAME%" --memory %VM_MEMORY% --vram %VM_VRAM% --cpus %VM_CPU%
vboxmanage modifyvm "%VM_NAME%" --chipset pc --acpi on --ioapic on

REM Set display
echo Configuring display...
vboxmanage modifyvm "%VM_NAME%" --displayfullscreen off
vboxmanage modifyvm "%VM_NAME%" --displaywidth 1024
vboxmanage modifyvm "%VM_NAME%" --displayheight 768

REM Set serial port (optional, useful for debugging)
echo Configuring serial port...
vboxmanage modifyvm "%VM_NAME%" --uart1 0x3F8 0x3F8 4 0xFFFF 0xFFFF
vboxmanage modifyvm "%VM_NAME%" --uartmode1 raw:%ROOT_DIR%\serial.log

REM Add optical drive
echo Adding optical drive with ISO...
vboxmanage storagectl "%VM_NAME%" --name "IDE Controller" --add ide
vboxmanage storagectl "%VM_NAME%" --name "IDE Controller" --add ide
vboxmanage storageattach "%VM_NAME%" --storagectl "IDE Controller" --port 0 --device 0 --type dvddrive --medium "%ISO_PATH%"

REM Set boot order
echo Setting boot order...
vboxmanage modifyvm "%VM_NAME%" --bootorder d

echo VM '%VM_NAME%' created successfully!
echo.
echo Next steps:
echo   1. Start the VM: vboxmanage startvm "%VM_NAME%" --type headless
@echo   2. Or start with GUI: vboxmanage startvm "%VM_NAME%" --type gui

exit /b 0

:help
echo Falkon-OS VirtualBox VM Creation Script
echo.
echo Usage: create_vbox_vm.bat [OPTIONS]
echo.
echo Options:
echo   -h, --help              Show this help message
