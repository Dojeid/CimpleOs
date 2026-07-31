; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; Multi-stage fallback sector reader supporting ISO 9660 (LBA 22),
; Raw Disk Image (LBA 1), and CHS BIOS disk reads.
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

    ; Try Stage 1: INT 13h AH=42h LBA 22 (ISO CD-ROM layout)
    mov si, dap_iso
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .load_ok

    ; Try Stage 2: INT 13h AH=42h LBA 1 (Raw Disk .img layout)
    mov si, dap_img
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .load_ok

    ; Try Stage 3: Standard INT 13h AH=02h CHS Sector 2 Read
    mov ax, 0x0270         ; AH=02 (Read), AL=112 sectors (~56KB)
    mov cx, 0x0002         ; Cylinder 0, Sector 2
    mov dh, 0              ; Head 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    mov es, bx
    xor bx, bx             ; ES:BX = 0x1000:0x0000 = 0x10000
    int 0x13

.load_ok:
    ; Reset ES back to 0
    xor ax, ax
    mov es, ax

    mov si, msg_loaded
    call print_string

    ; Enable A20 Line
    call enable_a20

    ; Switch to 32-bit Protected Mode
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
    mov ecx, 51200          ; Copy 200KB (51200 dwords = 204,800 bytes)
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

; --- Disk Address Packet for ISO CD-ROM Read (LBA Sector 22) ---
align 4
dap_iso:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw 100                  ; Number of CD-ROM sectors (100 * 2048 = 200KB payload)
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 22                   ; Starting LBA sector (Sector 22 = FalkonOS.bin on ISO)

; --- Disk Address Packet for Raw Disk Image Read (LBA Sector 1) ---
align 4
dap_img:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw 400                  ; Number of 512B sectors (400 * 512 = 200KB payload)
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 1                    ; Starting LBA sector (Sector 1 = FalkonOS.bin on raw disk)

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
