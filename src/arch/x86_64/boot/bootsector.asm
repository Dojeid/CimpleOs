; =============================================================================
; Falkon-OS Custom Stage 1 Boot Sector (Unreal Mode BIOS ISO/HDD Payload Loader)
; Address: 0x7C00 | Size: 512 bytes | Boot Signature: 0xAA55
; =============================================================================

[ORG 0x7C00]
[BITS 16]

KERNEL_BYTES        equ 2048 * 1024         ; 2048KB (2MB) Kernel stage 1 payload capacity
KERNEL_SECTORS      equ KERNEL_BYTES / 512
ISO_KERNEL_SECTORS  equ KERNEL_BYTES / 2048
MAX_CHUNK_512       equ 32                  ; 16KB per read call
MAX_CHUNK_2K        equ 16                  ; 32KB per read call

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_welcome
    call print_string

    call enable_a20
    call enable_unreal_mode

    ; Try ISO 9660 path first (LBA 22, 2048-byte sectors)
.try_iso:
    mov si, dap_iso
    mov bx, 2048
    mov cx, MAX_CHUNK_2K
    call read_payload
    jnc .boot_success

    ; Try Raw HDD path second (LBA 1, 512-byte sectors)
.try_img:
    mov si, dap_img
    mov bx, 512
    mov cx, MAX_CHUNK_512
    call read_payload
    jc .load_fail

.boot_success:
    mov si, msg_ok
    call print_string

    mov ax, 0x4F01
    mov cx, 0x0118
    mov di, 0x5000
    int 0x10

    mov eax, [di + 0x28]
    mov dword [0x500], eax

    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_32

.load_fail:
    mov si, msg_err
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

    mov eax, 0x2BADB002
    mov ebx, 0
    cmp dword [0x00100000], 0x1BADB002
    je .flat_jump
    jmp dword 0x00101010
.flat_jump:
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

enable_unreal_mode:
    cli
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax            ; Enter Protected Mode temporarily
    mov ax, 0x10            ; Load 4GB flat data descriptor into ES
    mov es, ax
    mov eax, cr0
    and eax, ~1
    mov cr0, eax            ; Exit Protected Mode back to Real Mode
    xor ax, ax
    mov ds, ax
    sti
    ret

read_payload:
    mov [sector_bytes], bx
    mov [max_chunk], cx
    mov ax, [si+2]
    mov word [load_remaining], ax
    mov eax, [si+8]
    mov dword [load_lba_low], eax
    mov dword [dest_phys], 0x00100000

.chunk_loop:
    mov ax, [load_remaining]
    test ax, ax
    jz .read_done

    mov cx, [max_chunk]
    mov di, cx
    cmp ax, cx
    jae .do_read
    mov di, ax

.do_read:
    push dword 0
    push dword [load_lba_low]
    push word 0x1000        ; Buffer at 0x1000:0000 (0x10000 physical)
    push word 0x0000
    push di
    push word 0x0010

    push si
    mov si, sp
    add si, 2
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    pop si
    add sp, 16
    jc .read_fail

    sub word [load_remaining], di
    movzx eax, di
    add dword [load_lba_low], eax

    mov ax, di
    mul word [sector_bytes]
    mov word [chunk_bytes], ax

    call copy_chunk_unreal
    jmp .chunk_loop

.read_done:
    clc
    ret
.read_fail:
    stc
    ret

copy_chunk_unreal:
    push ds
    push si
    push di
    push cx

    mov ax, 0x1000
    mov ds, ax
    xor si, si
    mov edi, [cs:dest_phys]
    movzx ecx, word [cs:chunk_bytes]
    shr ecx, 2
    cld
    a32 rep movsd

    mov [cs:dest_phys], edi

    pop cx
    pop di
    pop si
    pop ds
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
load_lba_low:   dd 0
dest_phys:      dd 0x00100000
chunk_bytes:    dw 0
sector_bytes:   dw 0
max_chunk:      dw 0

msg_welcome:    db "Booting...", 13, 10, 0
msg_ok:         db "OK.", 13, 10, 0
msg_err:        db "ERR!", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
