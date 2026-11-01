import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
metadata = json.loads((root / "app.json").read_text())
assert metadata["titleId"] == "PPSA99053"
assert len(metadata["contentId"].split("_00-")[1]) == 16
p = (root / "scripts/prepare.py").read_text()
assert "core_names=(scummvm)" in p
assert "configure-vulkan-probe" in p
assert "tools/scummvm-core-abi.py" in p
assert (root / "scripts/normalize_core_abi.py").is_file()
assert "manifest_check.write_text" in p
assert "lapy_build" in p and "lapy_stage" in p
assert "donor Lapy service" in p
assert "generator.write_text" in p
assert 'if [[ $vulkan_driver == radv ]]; then vulkan_archives=(); fi' in p
assert "967fd9757170b14c28590837186b31cc7d5cc46c" in json.dumps(metadata)
assert (root / "assets/icon-source.webp").stat().st_size > 1000
print("Source metadata and ScummVM-only patch preflight PASS")
