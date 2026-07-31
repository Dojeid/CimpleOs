; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; Multi-stage fallback sector reader supporting ISO 9660 (LBA 22),
; Raw Disk Image (LBA 1), and CHS BIOS disk reads.
; =============================================================================

[BITS 16]
[ORG 0x7C00]

; --- Kernel payload size limits (must match iso_builder.c) ---
KERNEL_BYTES        equ 320 * 1024      ; Max kernel payload size (320KB)
KERNEL_SECTORS      equ KERNEL_BYTES / 512      ; 640 (raw disk / CHS)
ISO_KERNEL_SECTORS  equ KERNEL_BYTES / 2048     ; 160 (ISO 9660 2048B sectors)
IMG_KERNEL_SECTORS  equ KERNEL_SECTORS          ; 640

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

    ; Try Stage 3: INT 13h AH=02h CHS loop (loads full kernel payload)
    mov ax, 0x1000          ; ES:BX = 0x1000:0x0000 = 0x10000
    mov es, ax
    xor bx, bx
    mov word [chs_remaining], KERNEL_SECTORS
    mov byte [chs_cyl], 0
    mov byte [chs_head], 0
    mov byte [chs_sector], 2    ; Kernel starts at CHS C0/H0/S2 (LBA 1)

.chs_loop:
    cmp word [chs_remaining], 0
    je .load_ok
    mov ax, [chs_remaining]     ; Read at most 63 sectors (one track)
    cmp ax, 63
    jbe .chs_chunk
    mov ax, 63
.chs_chunk:
    push ax                     ; Save chunk sector count
    mov ah, 0x02                ; AH=02 (Read), AL = sector count
    mov ch, [chs_cyl]
    mov cl, [chs_sector]
    mov dh, [chs_head]
    mov dl, [boot_drive]
    xor bx, bx                  ; ES:BX = ES:0 (BX stays 0, ES advances)
    int 0x13
    pop ax                      ; AX = sectors actually read
    jc .chs_fail

    ; Advance ES by (sectors * 512) / 16 paragraphs
    mov word [chs_count], ax
    sub word [chs_remaining], ax
    shl ax, 5                   ; sectors * 32 paragraphs
    mov bx, es
    add bx, ax
    mov es, bx

    ; Advance CHS: sector += count (wraps at 63 per track, 16 heads)
    mov al, byte [chs_count]
    add byte [chs_sector], al
    cmp byte [chs_sector], 63
    jbe .chs_loop
    sub byte [chs_sector], 63
    inc byte [chs_head]
    cmp byte [chs_head], 16
    jb .chs_loop
    mov byte [chs_head], 0
    inc byte [chs_cyl]
    jmp .chs_loop

.chs_fail:
    mov si, msg_chs_fail
    call print_string

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
    mov ecx, (KERNEL_BYTES / 4)   ; Copy full kernel payload (dwords)
    cld
    rep movsd

    ; Pass Multiboot 1 Boot Magic Signature in EAX for boot.asm
    mov eax, 0x2BADB002
    mov ebx, 0              ; Multiboot info pointer (NULL)

    ; Jump directly to Kernel Entry Point (start at 0x00100010)
    jmp dword 0x00100010

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
    dw ISO_KERNEL_SECTORS   ; Number of CD-ROM sectors (each 2048 bytes)
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 22                   ; Starting LBA sector (Sector 22 = FalkonOS.bin on ISO)

; --- Disk Address Packet for Raw Disk Image Read (LBA Sector 1) ---
align 4
dap_img:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw IMG_KERNEL_SECTORS   ; Number of 512B sectors
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
boot_drive      db 0
chs_remaining   dw 0
chs_count       dw 0
chs_cyl         db 0
chs_head        db 0
chs_sector      db 0
msg_welcome     db "Booting Falkon-OS Stage 1...", 0x0D, 0x0A, 0
msg_loaded      db "Kernel payload loaded.", 0x0D, 0x0A, 0
msg_chs_fail    db "CHS read failed.", 0x0D, 0x0A, 0

; --- Pad to 510 bytes and append Boot Signature 0xAA55 ---
times 510-($-$$) db 0
dw 0xAA55
