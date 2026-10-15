#!/usr/bin/env python3
"""Import pinned source trees needed to port PS5SNESX to MesenCE.

This is a source import, NOT a functional Mesen PS5 build.
"""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

BASE = Path(__file__).resolve().parents[1]
DEST = BASE / "upstream"
UPSTREAMS = {
    "PS5SNESX": {
        "url": "https://github.com/Yahballon/PS5SNESX.git",
        "sha": "4e0bca4df88e015dc71bc214b72545ce1305168d",
        "items": ["ps5", "snes", "3rdparty", "bin", "README.md",
                  "COPYING.GPLv3", "THIRD_PARTY_NOTICES.md", "LICENSE"],
    },
    "MesenCE": {
        "url": "https://github.com/nesdev-org/MesenCE.git",
        "sha": "4b8a196e299b3de297b204fd1208d2981562c89b",
        "items": ["Core", "InteropDLL", "Utilities", "Sdl", "Linux", "Lua",
                  "SevenZip", "makefile", "README.md", "LICENSE", "COMPILING.md"],
    },
}


def run(*args, cwd=None):
    subprocess.run(args, check=True, cwd=cwd)


def copy_pinned_source(label, config, workspace):
    checkout = workspace / label
    run("git", "init", "-q", str(checkout))
    run("git", "remote", "add", "origin", config["url"], cwd=checkout)
    run("git", "fetch", "--depth", "1", "--no-tags", "origin", config["sha"], cwd=checkout)
    run("git", "checkout", "-q", "--detach", "FETCH_HEAD", cwd=checkout)
    actual = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=checkout, text=True).strip()
    if actual != config["sha"]:
        raise RuntimeError(f"{label}: unexpected source revision {actual}")
    staged = workspace / f"{label}-staged"
    staged.mkdir()
    for entry in config["items"]:
        origin = checkout / entry
        if not origin.exists():
            raise RuntimeError(f"Missing upstream {label}/{entry}")
        target = staged / entry
        if origin.is_dir():
            shutil.copytree(origin, target, symlinks=True,
                            ignore=shutil.ignore_patterns(".git", "__pycache__",
                                                          "build", "build-*", "obj.*"))
        else:
            shutil.copy2(origin, target)
    return staged


def sync():
    with tempfile.TemporaryDirectory(prefix="ps5-mesenx-sync-") as root:
        tmp = Path(root)
        staged = {name: copy_pinned_source(name, cfg, tmp) for name, cfg in UPSTREAMS.items()}
        required = [
            staged["PS5SNESX"] / "snes/src/core_session.cpp",
            staged["PS5SNESX"] / "ps5/frontend/fe_app.cpp",
            staged["MesenCE"] / "Core/NES/HdPacks/HdPackLoader.cpp",
            staged["MesenCE"] / "Core/NES/HdPacks/HdNesPpu.cpp",
            staged["MesenCE"] / "InteropDLL",
        ]
        for path in required:
            if not path.exists():
                raise RuntimeError(f"Upstream source contract missing: {path}")
        DEST.mkdir(parents=True, exist_ok=True)
        for name, folder in staged.items():
            target = DEST / name
            if target.exists():
                shutil.rmtree(target)
            shutil.copytree(folder, target, symlinks=True)
        manifest = {name: {"url": config["url"], "sha": config["sha"],
                           "included_paths": config["items"]}
                    for name, config in UPSTREAMS.items()}
        (DEST / "UPSTREAM_SOURCES.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Imported pinned PS5SNESX and MesenCE sources into", DEST)


if __name__ == "__main__":
    sync()
