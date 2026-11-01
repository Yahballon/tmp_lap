#!/usr/bin/env python3
"""Host regression tests for the PS5 OSABI identification-only normalizer."""
import importlib.util
import pathlib
import struct
import tempfile
from unittest.mock import patch

root = pathlib.Path(__file__).resolve().parents[1]
script = root / "scripts/normalize_core_abi.py"
spec = importlib.util.spec_from_file_location("normalize_core", script)
core = importlib.util.module_from_spec(spec)
spec.loader.exec_module(core)

def fake_elf(osabi=0, align=0x4000, machine=62):
    elf = bytearray(128)
    elf[:8] = b"\x7fELF\x02\x01\x01" + bytes((osabi,))
    struct.pack_into("<HH", elf, 16, 3, machine)
    struct.pack_into("<Q", elf, 32, 64)
    struct.pack_into("<HH", elf, 54, 56, 1)
    struct.pack_into("<IIQQQQQQ", elf, 64, 1, 5, 0, 0, 0, 128, 128, align)
    return elf

with tempfile.TemporaryDirectory() as tmp:
    target = pathlib.Path(tmp) / "test_libretro.so"
    with patch.object(core.subprocess, "check_output", return_value=" (NEEDED) Shared library: [libkernel_web.sprx]\n"):
        target.write_bytes(fake_elf())
        assert core.normalize(target) == "normalized"
        assert target.read_bytes()[7] == 9
        assert core.normalize(target) == "unchanged"
        bad = fake_elf(0, 0x1000)
        target.write_bytes(bad)
        try:
            core.normalize(target)
            raise AssertionError("misaligned host library accepted")
        except RuntimeError:
            assert target.read_bytes() == bad
        bad = fake_elf(machine=183)
        target.write_bytes(bad)
        try:
            core.normalize(target)
            raise AssertionError("non-x86_64 accepted")
        except RuntimeError:
            assert target.read_bytes() == bad
    target.write_bytes(fake_elf())
    with patch.object(core.subprocess, "check_output", return_value=" (NEEDED) Shared library: [libc.so.6]\n"):
        try:
            core.normalize(target)
            raise AssertionError("host dependencies accepted")
        except RuntimeError:
            assert target.read_bytes()[7] == 0
print("PASS: PS5 ELF OSABI correction and host rejection")
