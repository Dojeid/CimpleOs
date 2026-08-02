; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; Chunked INT 13h AH=42h payload reader supporting:
;   - ISO 9660 CD (LBA 22, 2048B sectors)            DL >= 0xE0
;   - SeaBIOS El-Torito emulated CD (virtual LBA 4)   DL == 0x9F
;   - Raw Disk Image (LBA 1)                          DL == 0x80
; Chunks are capped at 32 sectors (CD) / 127 sectors (512B drives) due to the
; 64KB per-INT13h-transfer limit and the BIOS DAP count cap.
; =============================================================================

[ORG 0x7C00]
[BITS 16]

; --- Kernel payload size limits (dynamically scales with iso_builder.c) ---
KERNEL_BYTES        equ 64 * 1024 * 1024    ; Dynamic kernel capacity scaling (up to 64MB)
KERNEL_SECTORS      equ KERNEL_BYTES / 512      ; 131072 sectors (512B units)
ISO_KERNEL_SECTORS  equ KERNEL_BYTES / 2048     ; 32768 sectors (2048B ISO units)
PARA_PER_SECTOR_512 equ 32              ; paragraphs per 512-byte sector
PARA_PER_SECTOR_2K  equ 128             ; paragraphs per 2048-byte sector
MAX_CHUNK_512       equ 127             ; INT13 DAP count cap (512B drives)
MAX_CHUNK_2K        equ 32              ; 64KB transfer limit (2048B CD sectors)

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

    ; Select the disk read path from the BIOS boot drive (DL):
    ;   0x9F  -> SeaBIOS El-Torito emulated CD (virtual LBA 4, 512B sectors)
    ;   0xE0+ -> BIOS CD-ROM (ISO 9660, kernel at LBA 22)
    ;   0x80  -> hard disk raw image (kernel at LBA 1)
    mov dl, [boot_drive]
    cmp dl, 0x9F
    je .cdemu
    cmp dl, 0xE0
    jae .iso
    mov si, dap_img
    mov bx, PARA_PER_SECTOR_512
    mov cx, MAX_CHUNK_512
    cmp dl, 0x80
    je .load
    jmp .dap_fail

.cdemu:
    mov si, dap_cd_emu
    mov bx, PARA_PER_SECTOR_512
    mov cx, MAX_CHUNK_512
    jmp .load

.iso:
    mov si, dap_iso
    mov bx, PARA_PER_SECTOR_2K
    mov cx, MAX_CHUNK_2K

.load:
    call dap_read_loop
    jc .dap_fail

    ; Verify the Multiboot 1 magic in the loaded kernel header at 0x10000
    mov ax, 0x1000
    mov es, ax
    cmp dword [es:0x0000], 0x1BADB002
    jne .load_fail

    mov si, msg_loaded
    call print_string

    jmp .boot_continue

.dap_fail:
    mov si, msg_dap_fail
    call print_string
    mov al, ah              ; INT 13h status
    call print_hex8
    mov al, ':'
    call print_char
    mov al, dl              ; drive number
    call print_hex8
    cli
    hlt

.load_fail:
    mov si, msg_bad_kernel
    call print_string
    cli
    hlt

.boot_continue:
    mov ax, 0x4F02
    mov bx, 0x4118          ; Set VBE 1024x768 32bpp LFB mode
    int 0x10
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

; --- Helper: Print single character ---
print_char:
    push ax
    mov ah, 0x0E
    int 0x10
    pop ax
    ret

; --- Helper: Print 8-bit value in AL as two hex digits ---
print_hex8:
    push ax
    push cx
    mov cl, al
    shr al, 4
    call .nibble
    mov al, cl
    and al, 0x0F
    call .nibble
    pop cx
    pop ax
    ret
.nibble:
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 7
.out:
    mov ah, 0x0E
    int 0x10
    ret

; --- Chunked DAP read: INT 13h AH=42h in chunks to honor the 64KB/127-sector
;     BIOS limits. Updates the DAP count/LBA/segment fields in place.
;     si = DAP pointer   dl = drive number
;     bx = paragraphs per sector (32 = 512B, 128 = 2048B)
;     cx = max sectors per chunk (127 = 512B, 32 = 2048B)
;     Returns: CF clear on full success, CF set on failure. ---
dap_read_loop:
    mov ax, [si+2]
    mov word [load_remaining], ax
.chunk:
    mov ax, [load_remaining]
    test ax, ax
    jz .done
    mov di, cx
    cmp ax, cx
    jae .chunk_size
    mov di, ax
.chunk_size:
    push di
    mov word [si+2], di
    mov dl, [boot_drive]    ; loop clobbers DL - restore drive number
    mov ah, 0x42
    int 0x13
    pop di
    jc .fail
    sub word [load_remaining], di
    ; Advance DAP LBA (qword at si+8) by the chunk count
    mov ax, di
    xor dx, dx
    add word [si+8], ax
    adc word [si+10], dx
    ; Advance DAP buffer segment by (chunk * paragraphs-per-sector)
    mov ax, di
    mul bx
    add word [si+6], ax
    jmp .chunk
.done:
    clc
    ret
.fail:
    stc
    ret

; --- Disk Address Packet for ISO CD-ROM Read (2048B sectors, LBA 22) ---
align 4
dap_iso:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw ISO_KERNEL_SECTORS   ; Total 2048B sectors (160); chunked by the loop
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 22                   ; Starting LBA (Sector 22 = kernel on ISO)

; --- Disk Address Packet for Raw Disk Image Read (512B sectors, LBA 1) ---
align 4
dap_img:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw KERNEL_SECTORS       ; Total 512B sectors (640); chunked by the loop
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 1                    ; Starting LBA (Sector 1 = kernel on raw disk)

; --- Disk Address Packet for SeaBIOS El-Torito CD Emulation (512B units) ---
align 4
dap_cd_emu:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved (0)
    dw KERNEL_SECTORS       ; Total 512B units (640); chunked by the loop
    dw 0x0000               ; Buffer Offset
    dw 0x1000               ; Buffer Segment (0x1000:0x0000 = 0x10000 physical)
    dq 4                    ; Virtual LBA 4 = first 512B block of kernel (CD LBA 22)

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
load_remaining  dw 0
msg_welcome     db "Booting...", 13, 10, 0
msg_loaded      db "OK.", 13, 10, 0
msg_dap_fail    db "Fail:", 0
msg_bad_kernel  db "Bad kernel", 0

; --- Pad to 510 bytes and append Boot Signature 0xAA55 ---
times 510-($-$$) db 0
dw 0xAA55
