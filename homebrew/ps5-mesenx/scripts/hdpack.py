#!/usr/bin/env python3
"""Stage an NES MesenCE HD pack safely. Does not launch or emulate ROMs."""
import argparse
from pathlib import Path, PurePosixPath
import shutil
import stat
import sys
import tempfile
import zipfile

MAX_TOTAL = 2 * 1024**3
MAX_FILE = 512 * 1024**2
MAX_FILES = 20000


def _validate_zip(items):
    names = []
    total = 0
    for item in items:
        name = item.filename
        if not name or name.startswith("/") or "\\" in name or ":" in name:
            raise ValueError("Unsafe ZIP entry name: " + repr(name))
        parts = PurePosixPath(name).parts
        if ".." in parts or "." in parts or not parts:
            raise ValueError("Unsafe ZIP traversal: " + repr(name))
        if stat.S_ISLNK(item.external_attr >> 16):
            raise ValueError("ZIP symlink is forbidden: " + repr(name))
        if item.file_size > MAX_FILE:
            raise ValueError("Oversized HD pack entry: " + repr(name))
        total += item.file_size
        if total > MAX_TOTAL:
            raise ValueError("HD pack exceeds size limit")
        if not item.is_dir():
            names.append(parts)
    if len(names) > MAX_FILES:
        raise ValueError("HD pack has too many files")
    hires = [p for p in names if p[-1].lower() == "hires.txt"]
    if len(hires) != 1:
        raise ValueError("Pack must contain exactly one hires.txt definition")
    base = hires[0][:-1]
    selected = {}
    for item in items:
        p = PurePosixPath(item.filename).parts
        if item.is_dir() or p[:len(base)] != base or len(p) <= len(base):
            continue
        relative = p[len(base):]
        lower = "/".join(relative).casefold()
        if lower in selected:
            raise ValueError("Case-colliding ZIP paths")
        selected[lower] = (item, relative)
    if "hires.txt" not in selected:
        raise ValueError("Missing root hires.txt")
    return list(selected.values())


def install(archive: Path, rom_name: str, output: Path) -> Path:
    rom = Path(rom_name)
    if rom.name != rom_name or rom.suffix.lower() != ".nes":
        raise ValueError("ROM must be a single .nes filename (not a path)")
    game = rom.stem
    if not game or game in (".", "..") or "/" in game or "\\" in game:
        raise ValueError("Invalid ROM filename")
    output.mkdir(parents=True, exist_ok=True)
    target = output / game
    if target.exists():
        raise FileExistsError(f"HD pack already installed: {target}")
    with zipfile.ZipFile(archive) as zf:
        selected = _validate_zip(zf.infolist())
        with tempfile.TemporaryDirectory(prefix=".mesen-pack-", dir=output) as t:
            staged = Path(t) / "pack"
            staged.mkdir()
            for info, parts in selected:
                dest = staged.joinpath(*parts)
                dest.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(info) as src, dest.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
            if not (staged / "hires.txt").is_file():
                raise ValueError("Pack definition missing")
            staged.rename(target)
    return target


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)
    ins = sub.add_parser("install", help="Stage HD pack into <output>/<ROM stem>")
    ins.add_argument("archive", type=Path)
    ins.add_argument("--rom", required=True, help="ROM filename only, e.g. Zelda.nes")
    ins.add_argument("--output", type=Path, required=True, help="HdPacks directory")
    args = p.parse_args()
    if args.command == "install":
        try:
            target = install(args.archive, args.rom, args.output)
            print("Staged HD pack:", target)
        except (OSError, ValueError, zipfile.BadZipFile) as ex:
            p.error(str(ex))


if __name__ == "__main__":
    main()
