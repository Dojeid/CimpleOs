import struct

data = open('build/FalkonOS.bin', 'rb').read()
print('ELF class:', 'ELF32' if data[4] == 1 else 'ELF64')
e_entry = struct.unpack_from('<I', data, 24)[0]
e_phoff = struct.unpack_from('<I', data, 28)[0]
e_phnum = struct.unpack_from('<H', data, 44)[0]
print(f'Entry point: 0x{e_entry:x}')
print(f'Program headers at: {e_phoff}, count: {e_phnum}')
for i in range(e_phnum):
    off = e_phoff + i*32
    p_type  = struct.unpack_from('<I', data, off)[0]
    p_offset= struct.unpack_from('<I', data, off+4)[0]
    p_vaddr = struct.unpack_from('<I', data, off+8)[0]
    p_filesz= struct.unpack_from('<I', data, off+16)[0]
    print(f'  PT {i}: type=0x{p_type:x} offset=0x{p_offset:x} vaddr=0x{p_vaddr:x} filesz=0x{p_filesz:x}')

MBOOT_MAGIC = 0x1BADB002
for i in range(0, min(len(data), 0x10000), 4):
    val = struct.unpack_from('<I', data, i)[0]
    if val == MBOOT_MAGIC:
        flags = struct.unpack_from('<I', data, i+4)[0]
        cksum = struct.unpack_from('<I', data, i+8)[0]
        s = (MBOOT_MAGIC + flags + cksum) & 0xFFFFFFFF
        print(f'\nMultiboot header at file offset 0x{i:x}:')
        print(f'  flags=0x{flags:x} checksum=0x{cksum:x} sum=0x{s:x} {"OK" if s==0 else "INVALID!"}')
