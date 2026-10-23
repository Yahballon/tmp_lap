#!/usr/bin/env python3
"""Adapt the pinned MIT Godot Asset Library #2201 to an external PS5 PCK.

Never alter original upstream player/physics/audio/level code. Only augment
DualSense actions, renderer settings, and tutorial labels.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil

ASSET_COMMIT = "a48fc994cbcb168be472df84f2e13b55d2a2792f"
ASSET_REPO = "https://github.com/AdilDevStuff/2D-Platformer-Starter-Kit"
SOURCE = Path(__file__).resolve().parent

def prepare(project: Path) -> None:
    config = project / "project.godot"
    raw = config.read_text(encoding="utf-8")
    assert 'config_version=5' in raw
    assert 'run/main_scene="res://Scenes/Levels/Level_01.tscn"' in raw
    assert 'GameManager="*res://Scripts/GameManager.gd"' in raw
    assert '[autoload]' in raw
    assert 'config/features=PackedStringArray("4.7", "Forward Plus")' in raw
    raw = raw.replace(
        'config/features=PackedStringArray("4.7", "Forward Plus")',
        'config/features=PackedStringArray("4.7", "GL Compatibility")')
    # Native PS5 Godot was compiled with GLES3/Compatibility, not Vulkan.
    raw += (
        '\n; PS5 external PCK only: original game uses Godot Compatibility renderer.\n'
        '[rendering]\n'
        'renderer/rendering_method="gl_compatibility"\n'
        'renderer/rendering_method.mobile="gl_compatibility"\n'
    )
    raw = raw.replace('[autoload]\n', '[autoload]\n\n'
        'PS5Controls="*res://PS5Controls.gd"\n', 1)
    config.write_text(raw, encoding="utf-8")
    shutil.copy2(SOURCE / "PS5Controls.gd", project / "PS5Controls.gd")
    shutil.copy2(SOURCE / "pack_m12.gd", project / "pack_m12.gd")
    # Replace desktop-only tutorial hints; preserve the actual level scenes.
    level1 = project / "Scenes/Levels/Level_01.tscn"
    scene = level1.read_text(encoding="utf-8")
    original = 'A & D To Move\\nSpacebar To Jump'
    assert original in scene, "Original Level_01 tutorial text changed"
    scene = scene.replace(original, 'LEFT STICK / D-PAD\\nX TO JUMP')
    level1.write_text(scene, encoding="utf-8")
    print("GODOT_PS5_M12_UPSTREAM_PREPARED", ASSET_COMMIT)

def manifest(project: Path) -> int:
    imported = project / ".godot/imported"
    if not imported.is_dir() or not any(imported.iterdir()):
        raise RuntimeError("headless Godot editor did NOT import images, fonts and WAV: cannot package unimported game")
    files = []
    for file in sorted(project.rglob("*")):
        if not file.is_file():
            continue
        relative = file.relative_to(project)
        if relative.parts[0] == ".git":
            continue
        if relative.name in ("m12-platformer-asset2201.pck", "ps5_m12_manifest.txt"):
            continue
        # Editor shader cache and internal per-host binary data are not game assets.
        if relative.parts[:2] == (".godot", "editor"):
            continue
        if relative.parts[:2] == (".godot", "shader_cache"):
            continue
        if relative.parts[:2] == (".godot", "mono"):
            continue
        if relative.suffix in (".tmp", ".lock"):
            continue
        files.append(relative.as_posix())
    if not any(x.endswith(".ctex") for x in files):
        raise RuntimeError("No imported Godot .ctex textures found")
    if not any(x.endswith(".sample") for x in files):
        raise RuntimeError("No imported Godot .sample WAV effects found")
    if not any(x.endswith(".fontdata") or x.endswith(".fontdata") for x in files):
        # Native TTF can be loaded directly; no need to fail if no fontdata.
        print("GODOT_PS5_M12_FONT_DIRECT_TTF")
    (project / "ps5_m12_manifest.txt").write_text("\n".join(files) + "\n", encoding="utf-8")
    print("GODOT_PS5_M12_MANIFEST_OK", len(files))
    return len(files)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["prepare", "manifest"])
    parser.add_argument("project", type=Path)
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.project.resolve())
    else:
        manifest(args.project.resolve())
