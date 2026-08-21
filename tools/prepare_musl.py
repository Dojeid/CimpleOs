#!/usr/bin/env python3
"""
tools/prepare_musl.py
Cross-platform header generator for musl-1.2.6 on x86_64.
Prepares bits/alltypes.h, bits/syscall.h, version.h and copies arch & generic bits headers.
"""

import os
import re
import sys
import shutil
from pathlib import Path

def prepare_musl(root_dir=None):
    if root_dir is None:
        root_dir = Path(__file__).parent.parent.resolve()
    else:
        root_dir = Path(root_dir).resolve()

    musl_dir = root_dir / "musl-1.2.6"
    if not musl_dir.exists():
        print(f"[ERROR] musl-1.2.6 directory not found at {musl_dir}")
        return False

    include_dir = musl_dir / "include"
    bits_dir = include_dir / "bits"
    generic_bits_dir = musl_dir / "arch" / "generic" / "bits"
    arch_bits_dir = musl_dir / "arch" / "x86_64" / "bits"

    bits_dir.mkdir(parents=True, exist_ok=True)

    # 1. Generate version.h
    version_h = include_dir / "version.h"
    version_file = musl_dir / "VERSION"
    ver_str = "1.2.6"
    if version_file.exists():
        ver_str = version_file.read_text(encoding="utf-8").strip()
    
    version_h.write_text(f'#define VERSION "{ver_str}"\n', encoding="utf-8")
    print(f"[musl] Generated {version_h.relative_to(root_dir)}")

    # 2. Copy generic arch bits headers first
    if generic_bits_dir.exists():
        for item in generic_bits_dir.iterdir():
            if item.is_file() and item.name.endswith(".h"):
                dest = bits_dir / item.name
                shutil.copy2(item, dest)

    # 3. Override with arch/x86_64/bits/*.h
    if arch_bits_dir.exists():
        for item in arch_bits_dir.iterdir():
            if item.is_file() and item.name.endswith(".h"):
                dest = bits_dir / item.name
                shutil.copy2(item, dest)
                print(f"[musl] Copied {item.name} to {dest.relative_to(root_dir)}")

    # 4. Generate bits/alltypes.h
    arch_alltypes_in = arch_bits_dir / "alltypes.h.in"
    inc_alltypes_in = include_dir / "alltypes.h.in"
    out_alltypes_h = bits_dir / "alltypes.h"

    lines = []
    if arch_alltypes_in.exists():
        lines.extend(arch_alltypes_in.read_text(encoding="utf-8").splitlines())
    if inc_alltypes_in.exists():
        lines.extend(inc_alltypes_in.read_text(encoding="utf-8").splitlines())

    transformed = []
    typedef_re = re.compile(r"^TYPEDEF\s+(.+)\s+([^;\s]+);$")
    struct_re  = re.compile(r"^STRUCT\s+([^\s]+)\s+(.+);$")
    union_re   = re.compile(r"^UNION\s+([^\s]+)\s+(.+);$")

    for line in lines:
        m_td = typedef_re.match(line)
        if m_td:
            type_decl, name = m_td.group(1), m_td.group(2)
            transformed.append(f"#if defined(__NEED_{name}) && !defined(__DEFINED_{name})")
            transformed.append(f"typedef {type_decl} {name};")
            transformed.append(f"#define __DEFINED_{name}")
            transformed.append("#endif")
            continue

        m_st = struct_re.match(line)
        if m_st:
            name, body = m_st.group(1), m_st.group(2)
            transformed.append(f"#if defined(__NEED_struct_{name}) && !defined(__DEFINED_struct_{name})")
            transformed.append(f"struct {name} {body};")
            transformed.append(f"#define __DEFINED_struct_{name}")
            transformed.append("#endif")
            continue

        m_un = union_re.match(line)
        if m_un:
            name, body = m_un.group(1), m_un.group(2)
            transformed.append(f"#if defined(__NEED_union_{name}) && !defined(__DEFINED_union_{name})")
            transformed.append(f"union {name} {body};")
            transformed.append(f"#define __DEFINED_union_{name}")
            transformed.append("#endif")
            continue

        transformed.append(line)

    out_alltypes_h.write_text("\n".join(transformed) + "\n", encoding="utf-8")
    print(f"[musl] Generated {out_alltypes_h.relative_to(root_dir)}")

    # 5. Generate bits/syscall.h
    syscall_in = arch_bits_dir / "syscall.h.in"
    out_syscall_h = bits_dir / "syscall.h"

    if syscall_in.exists():
        sc_lines = syscall_in.read_text(encoding="utf-8").splitlines()
        sc_out = []
        for line in sc_lines:
            sc_out.append(line)
            m = re.match(r"^#define\s+__NR_([A-Za-z0-9_]+)\s+(.+)", line)
            if m:
                sc_name = m.group(1)
                sc_out.append(f"#define SYS_{sc_name} __NR_{sc_name}")
        out_syscall_h.write_text("\n".join(sc_out) + "\n", encoding="utf-8")
        print(f"[musl] Generated {out_syscall_h.relative_to(root_dir)}")

    # Also sync musl obj/include/bits directory if needed
    obj_bits = musl_dir / "obj" / "include" / "bits"
    obj_bits.mkdir(parents=True, exist_ok=True)
    for f in bits_dir.iterdir():
        if f.is_file():
            shutil.copy2(f, obj_bits / f.name)

    print("[musl] All musl headers prepared successfully!")
    return True

if __name__ == "__main__":
    success = prepare_musl()
    sys.exit(0 if success else 1)
