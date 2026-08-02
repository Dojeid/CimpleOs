; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (16-bit BIOS Real Mode -> Protected Mode)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; =============================================================================

[ORG 0x7C00]
[BITS 16]

KERNEL_BYTES        equ 1 * 1024 * 1024     ; 1MB Kernel capacity
KERNEL_SECTORS      equ KERNEL_BYTES / 512
ISO_KERNEL_SECTORS  equ KERNEL_BYTES / 2048
PARA_PER_SECTOR_512 equ 32
PARA_PER_SECTOR_2K  equ 128
MAX_CHUNK_512       equ 127
MAX_CHUNK_2K        equ 32

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

    mov si, msg_welcome
    call print_string

    ; Select disk read path based on DL
    mov dl, [boot_drive]
    cmp dl, 0x9F
    je .cdemu
    cmp dl, 0xE0
    jae .iso

.img:
    mov si, dap_img
    mov bx, PARA_PER_SECTOR_512
    mov cx, MAX_CHUNK_512
    jmp .load

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
    jc .fallback
    mov ax, 0x1000
    mov es, ax
    cmp dword [es:0x0000], 0x1BADB002
    je .boot_success

.fallback:
    ; Fallback to ISO DAP if IMG DAP failed (or vice versa)
    mov si, dap_iso
    mov bx, PARA_PER_SECTOR_2K
    mov cx, MAX_CHUNK_2K
    call dap_read_loop
    jc .dap_fail

    mov ax, 0x1000
    mov es, ax
    cmp dword [es:0x0000], 0x1BADB002
    jne .load_fail

.boot_success:
    mov si, msg_loaded
    call print_string

    mov ax, 0x4F02
    mov bx, 0x4118          ; VBE 1024x768 32bpp LFB mode
    int 0x10
    call enable_a20

    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_32

.dap_fail:
    mov si, msg_dap_fail
    call print_string
    mov al, ah
    call print_hex8
    mov al, ':'
    call print_char
    mov al, dl
    call print_hex8
    cli
    hlt

.load_fail:
    mov si, msg_bad_kernel
    call print_string
    cli
    hlt

[BITS 32]
init_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, (KERNEL_BYTES / 4)
    cld
    rep movsd

    mov eax, 0x2BADB002
    mov ebx, 0
    jmp dword 0x00100010

[BITS 16]
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

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

print_char:
    push ax
    mov ah, 0x0E
    int 0x10
    pop ax
    ret

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
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    pop di
    jc .fail
    sub word [load_remaining], di
    mov ax, di
    xor dx, dx
    add word [si+8], ax
    adc word [si+10], dx
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

align 4
dap_iso:
    db 0x10, 0x00
    dw ISO_KERNEL_SECTORS
    dw 0x0000, 0x1000
    dq 22

align 4
dap_img:
    db 0x10, 0x00
    dw KERNEL_SECTORS
    dw 0x0000, 0x1000
    dq 1

align 4
dap_cd_emu:
    db 0x10, 0x00
    dw KERNEL_SECTORS
    dw 0x0000, 0x1000
    dq 4

align 8
gdt32_start:
    dd 0, 0
gdt32_code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt32_data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt32_end:

gdt32_desc:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

boot_drive:     db 0
load_remaining: dw 0

msg_welcome:    db "Booting...", 13, 10, 0
msg_loaded:     db "OK.", 13, 10, 0
msg_dap_fail:   db "Fail : ", 0
msg_bad_kernel: db "Bad Kernel!", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
