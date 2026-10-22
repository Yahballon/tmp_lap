#!/usr/bin/env python3
"""Host-only packaging checks; synthetic headers are NOT actual Godot builds."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import zipfile

module_path = Path(__file__).resolve().parents[1] / "samples/external-pack-smoke/make_ps5_test_bundle.py"
spec = importlib.util.spec_from_file_location("godot_ps5_bundle", module_path)
assert spec and spec.loader
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)

with tempfile.TemporaryDirectory() as td:
    directory = Path(td)
    fixture = directory / "external-smoke.pck"
    fixture.write_bytes(b"GDPC" + struct.pack("<III", 3, 4, 7) + b"\x00" * 128)
    output, digest = bundle.build_bundle(fixture, directory / "out")
    assert len(digest) == 64
    with zipfile.ZipFile(output) as z:
        assert z.testzip() is None
        assert z.read("GodotPS5/games/external-smoke.pck") == fixture.read_bytes()
        assert z.read("GodotPS5/launch.txt") == b"external-smoke.pck\n"
        assert digest.encode() in z.read("GodotPS5/external-smoke.sha256")
        assert b"PPSA99847" in z.read("GodotPS5/README-M8c.txt")
    for bad in (b"BAD!", b"GDPC" + struct.pack("<III", 3, 3, 7),
                b"GDPC" + struct.pack("<III", 3, 4, 8)):
        fixture.write_bytes(bad + b"\x00" * 128)
        try:
            bundle.build_bundle(fixture, directory / "bad")
        except ValueError:
            pass
        else:
            raise AssertionError("incompatible/invalid PCK header accepted")
print("PASS: M8c PS5 starter bundle layout, checksum and strict header gate")
