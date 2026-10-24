#!/usr/bin/env python3
"""Regression: FreeType candidate must be v0.1.4 end-to-end, not v0.1.3.

GitHub PS5 CI previously compiled, linked and signed v0.1.4, but its final
ZIP gate still checked contentVersion 01.000.004, failing after sign.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[3]
workflow = (root / ".github/workflows/build-godot-ps5-m10c-freetype-fonts.yml").read_text()
# The publisher is deliberately only on default branch 'main', not the
# PS5 engine branch; don't fail native cross-build by assuming main-only
# publish workflows are present in the PR checkout.
publisher_path = root / ".github/workflows/publish-godot-ps5-m10c-freetype.yml"
publisher = publisher_path.read_text() if publisher_path.is_file() else None
assert "contentVersion='01.000.005'" in workflow, "stage must use PS5 v0.1.4"
assert "assert meta['contentVersion']=='01.000.005'" in workflow, "ZIP validator must expect PS5 v0.1.4"
assert "assert meta['contentVersion']=='01.000.004'" not in workflow
assert "Godot-PS5-M10c-NATIVE-FREETYPE-v0.1.4-PS5-HARDWARE-UNTESTED" in workflow
assert "GODOT_PS5_FREETYPE: '1'" in workflow
assert "GODOT_PS5_M10C_FREETYPE_ARCHIVES_FOUND" in workflow
assert "FT_Init_FreeType" in workflow
if publisher is not None:
    assert "assert meta['contentVersion']=='01.000.005'" in publisher
    assert "godot-ps5-gles3-canvas-v0.1.4" in publisher
    assert "tools/update-version.py" in publisher
else:
    print("INFO: TESTBUILD publisher is main-only; native v0.1.4 gate verified independently")
print("PASS: v0.1.4 FreeType signed native version, final gate and TESTBUILD publisher agree")
