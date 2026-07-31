; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
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

    ; Enable A20 Line
    call enable_a20

    ; Switch to 32-bit Protected Mode
    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump into 32-bit Protected Mode (Jump to kernel entry point at 0x100030)
    jmp dword 0x08:0x00100030

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

; --- Pad to 510 bytes and append Boot Signature 0xAA55 ---
times 510-($-$$) db 0
dw 0xAA55
