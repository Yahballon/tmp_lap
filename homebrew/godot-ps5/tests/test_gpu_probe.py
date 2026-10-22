#!/usr/bin/env python3
"""Guard the PS5 isolated-GPU experiment against accidental engine mixing."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root / "gpu-probe/main.cpp").read_text()
wf = (root.parent.parent / ".github/workflows/build-godot-ps5-gpu-probe.yml").read_text()
readme = (root / "gpu-probe/README.md").read_text()
for symbol in ("egl", "display.open", "glClear(", "display.swap()",
               "FIRST_FRAME_PRESENTED", "PRESENTED_FRAME_COUNT", "CLEAN_SHUTDOWN",
               "kCross", "kCircle", "graphics-probe.log"):
    assert symbol in src, symbol
assert "PPSA99844" in wf and "PPSA99844" in readme
assert "PPSA99843.zip" not in wf
assert "publish" not in wf.lower().split("jobs:",1)[1].split("name:",1)[0]
assert "godot_ps5_run_pack" not in src and "os_ps5" not in src
print("PASS: isolated GPU probe source & release separation")
