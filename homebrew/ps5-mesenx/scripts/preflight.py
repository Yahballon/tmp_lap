#!/usr/bin/env python3
"""Check source-import integrity and PS5MesenX integration assumptions."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
M = ROOT / "upstream/MesenCE"
P = ROOT / "upstream/PS5SNESX"

def test_import():
    assert (M / "Core/NES/HdPacks/HdPackLoader.cpp").is_file()
    assert (M / "Core/Shared/Emulator.h").is_file()
    assert (P / "snes/src/shelf_runtime.cpp").is_file()

def test_native_adapter():
    source = (ROOT / "src/core_session.cpp").read_text()
    assert "NesConfig" not in source or "EnableHdPacks" in source
    assert "GetHdPackFolder" in (M / "Core/NES/HdPacks/HdPackLoader.cpp").read_text()
    assert "frame.Width > 4096" in source
    assert "RegisterInputProvider" in source
    assert "SaveStateManager" in source
    assert "retro_load_game" not in source
    assert "SetHomeFolder(root_)" in source

def test_original_snesx_unchanged():
    assert not (P / "snes/src/core_session.cpp").read_text().startswith("// PS5MesenX")

if __name__ == "__main__":
    for fn in (test_import, test_native_adapter, test_original_snesx_unchanged):
        fn()
        print("PASS", fn.__name__)
