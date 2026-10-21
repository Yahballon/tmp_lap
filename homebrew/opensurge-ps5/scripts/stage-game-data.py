#!/usr/bin/env python3
"""Stage the ORIGINAL pinned Open Surge game data for native PS5 packaging.

Does not build or impersonate an installable PS5 package. Assets remain upstream's.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT_FILES = (
    "LICENSE", "README.md", "CHANGELOG.md", "CONTRIBUTING.md",
    "logo.png", "surge.png", "surge.rocks", "surge.cfg",
)
ASSET_DIRS = (
    "characters", "scripts", "sprites", "images", "levels", "musics",
    "quests", "samples", "themes", "languages", "fonts", "inputs", "licenses",
)


def stage(source: Path, destination: Path) -> dict:
    """Build a verifiable root/game folder from exactly the source game's data."""
    source = source.resolve(strict=True)
    destination = destination.resolve()
    if destination.exists():
        raise ValueError(f"Refusing to overwrite an existing staged game: {destination}")
    if source == destination or source in destination.parents:
        raise ValueError("Refusing to stage inside upstream checkout")
    for name in (*ROOT_FILES, *ASSET_DIRS):
        if not (source / name).exists():
            raise FileNotFoundError(f"Missing upstream asset: {name}")

    # Preflight symlinks to prevent outside-the-checkout data from being shipped.
    # In-tree links are copied by value so the PS5 folder remains self-contained.
    paths = [source / name for name in ROOT_FILES]
    for directory in ASSET_DIRS:
        paths.extend(p for p in (source / directory).rglob("*") if p.is_file())
    for path in paths:
        if not path.resolve().is_relative_to(source):
            raise ValueError(f"Outside-repository symlink: {path}")

    folder = destination / "game"
    folder.mkdir(parents=True)
    for name in ROOT_FILES:
        shutil.copy2(source / name, folder / name)
    for name in ASSET_DIRS:
        shutil.copytree(source / name, folder / name)

    entries = []
    for path in sorted(folder.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(folder).as_posix()
        digest = hashlib.sha256()
        with path.open("rb") as handle:
            for block in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(block)
        entries.append({"path": rel, "size": path.stat().st_size, "sha256": digest.hexdigest()})

    if not entries:
        raise ValueError("Empty game data")
    manifest = {
        "title_id": "PPSA99358",
        "upstream": "alemart/opensurge",
        "version": "0.6.1.3",
        "game_root": "game/",
        "file_count": len(entries),
        "total_bytes": sum(x["size"] for x in entries),
        "files": entries,
    }
    (destination / "game-data-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="pristine pinned Open Surge source")
    parser.add_argument("destination", type=Path, help="new staging directory, not existing")
    args = parser.parse_args()
    out = stage(args.source, args.destination)
    print(f"Staged {out['file_count']} upstream assets ({out['total_bytes']} bytes)")
    print("This contains game data ONLY. It is NOT a playable PS5 package.")


if __name__ == "__main__":
    main()
