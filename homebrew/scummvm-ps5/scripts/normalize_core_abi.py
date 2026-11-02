#!/usr/bin/env python3
"""Verify PS5 core ELF identity and repair only the ELF OSABI identification byte.

PS5's Clang/LLD pipeline can emit a valid PS5 ET_DYN image with OSABI=0 (SYSV)
rather than OSABI=9 (FreeBSD). The project check-core.py deliberately rejects
that byte. This script does NOT convert a Linux-host .so into a PS5 library:
it checks architecture, dynamic dependencies and segment shape before touching
one byte; check-core.py subsequently validates ALL normal ABI requirements.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import struct
import subprocess

ALLOWED = {"libkernel_web.sprx", "libSceLibcInternal.sprx", "libScePosixForWebKit.sprx"}


def normalize(path: pathlib.Path) -> str:
    raw = bytearray(path.read_bytes())
    if len(raw) < 64 or raw[:7] != b"\x7fELF\x02\x01\x01":
        raise RuntimeError("not ELF64 little-endian version 1")
    kind, machine = struct.unpack_from("<HH", raw, 16)
    if (kind, machine) != (3, 62):
        raise RuntimeError(f"unexpected ELF type/machine: {kind}/{machine}")
    osabi = raw[7]
    if osabi not in (0, 9):
        raise RuntimeError(f"unexpected ELF OSABI={osabi}; refusing modification")
    dyn = subprocess.check_output(["readelf", "-dW", str(path)], text=True)
    needed = re.findall(r"\(NEEDED\).*?\[([^\]]+)\]", dyn)
    if "libkernel_web.sprx" not in needed or set(needed) - ALLOWED:
        raise RuntimeError(f"not a PS5-target core; linked dependencies: {needed}")
    phoff = struct.unpack_from("<Q", raw, 32)[0]
    entsize, count = struct.unpack_from("<HH", raw, 54)
    if entsize != 56 or count == 0 or phoff + entsize * count > len(raw):
        raise RuntimeError("invalid ELF program header table")
    loads = 0
    for i in range(count):
        ptype, flags, offset, addr, _, filesz, memsz, align = struct.unpack_from(
            "<IIQQQQQQ", raw, phoff + i * entsize
        )
        if ptype != 1:
            continue
        loads += 1
        if (align < 0x4000 or (flags & 3) == 3
                or offset % 0x4000 != addr % 0x4000
                or filesz > memsz or offset + filesz > len(raw)):
            raise RuntimeError(f"unsafe PS5 load segment {i}: flags={flags} align={align}")
    if not loads:
        raise RuntimeError("no loadable segments")
    if osabi == 9:
        print(f"Already PS5/FreeBSD OSABI, PS5 imports={needed}, segments={loads}")
        return "unchanged"
    raw[7] = 9
    path.write_bytes(raw)
    print(f"Normalized only ELF e_ident[EI_OSABI] 0 -> 9; PS5 imports={needed}; segments={loads}")
    return "normalized"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("core", type=pathlib.Path)
    normalize(parser.parse_args().core)
