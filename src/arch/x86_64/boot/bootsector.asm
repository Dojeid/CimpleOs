; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; Performs INT 13h LBA disk read to load kernel payload from Sector 21 to 0x10000,
; switches to 32-bit Protected Mode, relocates kernel to 0x100000, and jumps to entry.
; =============================================================================

[BITS 16]
[ORG 0x7C00]

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl    ; Save BIOS boot drive number

    ; Welcome Banner
    mov si, msg_welcome
    call print_string

    ; 1. Read Kernel Payload from CD-ROM LBA Sector 21 into RAM 0x10000 via INT 13h AH=42h
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .load_ok

    ; Fallback: Retry INT 13h if first attempt failed
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13

.load_ok:
    mov si, msg_loaded
    call print_string

    ; 2. Enable A20 Line
    call enable_a20

    ; 3. Switch to 32-bit Protected Mode
    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit Protected Mode entry point
    jmp 0x08:init_32

[BITS 32]
init_32:
    ; Reload 32-bit Data Segment Selectors (0x10 = GDT Data Segment)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; Set up safe 32-bit stack

    ; Relocate Kernel payload from 0x10000 (loaded address) to 0x100000 (1MB target base)
    mov esi, 0x10000        ; Source address
    mov edi, 0x100000       ; Destination address (1MB)
    mov ecx, 65536          ; Copy 256KB (65536 dwords)
    cld
    rep movsd

    ; Pass Multiboot 1 Boot Magic Signature in EAX for boot.asm
    mov eax, 0x2BADB002
    mov ebx, 0              ; Multiboot info pointer (NULL)

    ; Jump to Kernel Entry Point (Multiboot entry at 0x00100030)
    jmp dword 0x00100030

[BITS 16]
; --- Helper: Print String in Real Mode via BIOS INT 10h ---
print_string:
    mov ah, 0x0E
.loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; --- Helper: Enable A20 Line ---
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; --- Disk Address Packet (DAP) for INT 13h Extended Read (AH=42h) ---
align 4
dap:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw 256                  ; Number of sectors to read (256 * 2048 = 512KB payload)
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 21                   ; Starting LBA sector (Sector 21 = FalkonOS.bin start)

; --- 32-bit Global Descriptor Table (GDT) ---
align 8
gdt32_start:
    dd 0, 0                 ; Null Descriptor

gdt32_code:                 ; 0x08: Code Segment (Base=0, Limit=4GB, 32-bit, RX)
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00

gdt32_data:                 ; 0x10: Data Segment (Base=0, Limit=4GB, 32-bit, RW)
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00

gdt32_end:

gdt32_desc:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

; --- Global Variables & Messages ---
boot_drive  db 0
msg_welcome db "Booting Falkon-OS Stage 1...", 0x0D, 0x0A, 0
msg_loaded  db "Kernel payload loaded.", 0x0D, 0x0A, 0

; --- Pad to 510 bytes and append Boot Signature 0xAA55 ---
times 510-($-$$) db 0
dw 0xAA55
