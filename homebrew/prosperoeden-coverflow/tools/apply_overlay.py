#!/usr/bin/env python3
"""Apply a pinned, native GBAXY coverflow overlay to a clean ProsperoEden clone.

Copyright (C) 2026 YAHBALLON. GPL-3.0-or-later.
"""
from pathlib import Path
import argparse
import json
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "SOURCE.json").read_text(encoding="utf-8"))
OVERLAY = ROOT / "overlay"
EXPECTED = {
    "headless/prosperoeden/pe/ui/library.cpp",
    "headless/prosperoeden/pe/ui/launcher.cpp",
    "headless/prosperoeden/pe/ui/theme.cpp",
    "headless/prosperoeden/pe/ui/shell.cpp",
    "headless/prosperoeden/pe/ui/coverflow.hpp",
    "headless/prosperoeden/pe/gfx/draw_list.hpp",
    "headless/prosperoeden/pe/gfx/draw_list.cpp",
    "headless/prosperoeden/pe/gfx/gl_batch.cpp",
}

def git(folder, *args):
    return subprocess.check_output(["git", "-C", str(folder), *args], text=True).strip()

def apply(folder):
    folder = folder.resolve()
    if not (folder / ".git").exists():
        raise RuntimeError("Expected a Git clone of Swordpdf/ProsperoEden")
    head = git(folder, "rev-parse", "HEAD")
    if head != MANIFEST["upstream_commit"]:
        raise RuntimeError(f"Upstream mismatch: {head}; expected {MANIFEST['upstream_commit']}")
    if git(folder, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("Refusing to overwrite a checkout with tracked local changes")
    observed = {str(f.relative_to(OVERLAY)) for f in OVERLAY.rglob("*") if f.is_file()}
    if observed != EXPECTED:
        raise RuntimeError(f"Unexpected overlay files: missing={EXPECTED-observed}, extra={observed-EXPECTED}")

    # Validate ALL sources before writing ANY target.
    planned = []
    for relative in sorted(EXPECTED):
        dst = folder / relative
        if dst.is_symlink():
            raise RuntimeError(f"Refusing symlink at {relative}")
        if dst.exists():
            original = subprocess.check_output(["git", "-C", str(folder), "show", "HEAD:" + relative])
            if dst.read_bytes() != original:
                raise RuntimeError(f"Modified source: {relative}")
        planned.append((OVERLAY / relative, dst))
    for src, dst in planned:
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
    return [str(dst.relative_to(folder)) for _, dst in planned]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    try:
        touched = apply(args.source)
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print("ERROR:", error, file=sys.stderr)
        sys.exit(2)
    print("ProsperoEden GBAXY coverflow source installed:")
    for p in touched:
        print(" -", p)
