#!/usr/bin/env python3
"""Fast offline validation, before cross-compiling."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
meta = json.loads((root / "app.json").read_text())
assert meta["titleId"] == "PPSA99791"
assert meta["version"] == "0.1.0"
assert re.fullmatch(r"UP[0-9]{4}-PPSA[0-9]{5}_00-[A-Z0-9]{16}", meta["contentId"])
for name in ("hello.py", "fibonacci.py", "collections.py"):
    src = (root / "assets/scripts" / name).read_text()
    compile(src, name, "exec")
    assert src.strip()
config = (root / "native/mpconfigport.h").read_text()
assert "MICROPY_ENABLE_COMPILER (1)" in config
assert "MICROPY_ENABLE_GC (1)" in config
ui = (root / "native/main.cpp").read_text()
for token in ("mp_embed_init(", "mp_embed_exec_str(", "mp_embed_deinit(",
              "ps5_mp_console_write", "/data/PS5MicroPython/scripts"):
    assert token in ui, token
print("Host-side source checks passed")
