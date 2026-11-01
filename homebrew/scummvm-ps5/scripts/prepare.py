#!/usr/bin/env python3
"""Strict overlay for Mihawk's pinned PS5 RetroArch, ScummVM-only edition."""
import argparse
import json
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
TITLE = "PPSA99053"
DONOR = "7268510b12fe7461342e0cc4497c87a0910c1055"

def patch(source: pathlib.Path):
    source = source.resolve()
    repo = subprocess.run(["git", "-C", str(source), "rev-parse", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
    if repo != DONOR:
        raise RuntimeError(f"Donor commit mismatch: {repo} != {DONOR}")
    build = source / "tools/build-title.sh"
    original = build.read_text()
    start = original.index("core_names=(fceumm mgba")
    end = original.index("\nif [[", start)
    old = original[start:end]
    assert "scummvm ps5sx2)" in old and "core_names=(fceumm" in old
    updated = original[:start] + "core_names=(scummvm)" + original[end:]
    # Optional extra core branches are disabled for this dedicated build.
    updated = updated.replace("if [[ ${PS5_WITH_SWANSTATION:-0} == 1 ]]; then", "if false; then")
    updated = updated.replace("if [[ ${PS5_WITH_MELONDSDS:-0} == 1 ]]; then", "if false; then")
    updated = updated.replace("if [[ ${PS5_WITH_RPCS3:-0} == 1 ]]; then", "if false; then")
    # The upstream title checks ps5vk-only archives before it branches to RADV.
    # A clean, RADV-only CI title never builds the unused demo ps5vk driver;
    # reset the unused archive list before the common missing-file check.
    # RADV is still linked and checked using its own validated archive below.
    anchor = "vulkan_missing=()"
    if updated.count(anchor) != 1:
        raise RuntimeError("Pinned upstream Vulkan branch check changed")
    updated = updated.replace(anchor,
        'if [[ $vulkan_driver == radv ]]; then vulkan_archives=(); fi\n' + anchor, 1)
    build.write_text(updated)
    # Dedicated build only stages ScummVM, so the WebUI core-option
    # generator must not request 31 missing other cores' source trees.
    generator = source / "tools/generate-core-metadata.py"
    metadata_source = generator.read_text()
    begin = metadata_source.index("CORES = [")
    end = metadata_source.index("\nif os.environ.get('PS5_WITH_SWANSTATION')", begin)
    old_block = metadata_source[begin:end]
    scummvm_line = next(
        (line for line in old_block.splitlines()
         if line.lstrip().startswith("('scummvm', 'ScummVM',")),
        None,
    )
    if scummvm_line is None:
        raise RuntimeError("Pinned upstream ScummVM metadata definition missing")
    metadata_source = (
        metadata_source[:begin] + "CORES = [\n" + scummvm_line + "\n]"
        + metadata_source[end:]
    )
    # Workflows do not turn on the donor's optional additional core flags.
    generator.write_text(metadata_source)
    param = source / "sce_sys/param.json"
    metadata = json.loads(param.read_text())
    if metadata.get("titleId") != "PPSA99169":
        raise RuntimeError("Unexpected upstream title identity")
    metadata.update(titleId=TITLE, conceptId=TITLE[4:],
                    contentId="UP9000-PPSA99053_00-SCUMMVMPS5000001",
                    contentVersion="01.000.001")
    metadata["localizedParameters"]["en-US"]["titleName"] = "ScummVM PS5 by YAHBALLON"
    param.write_text(json.dumps(metadata, indent=2) + "\n")
    cfg = source / "config/retroarch.cfg"
    config = cfg.read_text()
    config += '\n# ScummVM-only launcher preference.\nmenu_driver = "rgui"\nvideo_refresh_rate = "59.940000"\nvideo_swap_interval = "1"\n'
    cfg.write_text(config)
    # Dedicated title without the usual dual-frontend picker or EmulationStation.
    # PS5_FRONTENDS is intentionally unset in development CI (no PS5_RELEASE_TAG).
    # The proven RetroArch menu is the fallback when no picker is present.
    art = ROOT / "assets/icon-source.webp"
    if not art.is_file():
        raise RuntimeError("Original artwork missing")
    try:
        from PIL import Image
    except ImportError as exc:
        raise RuntimeError("Install pillow to integrate the original PS5 icon") from exc
    with Image.open(art) as im:
        im.convert("RGB").resize((512, 512), Image.Resampling.LANCZOS).save(source / "sce_sys/icon0.png", "PNG")
    print(f"Prepared dedicated ScummVM PS5 source {TITLE} from {repo}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    patch(parser.parse_args().source)
