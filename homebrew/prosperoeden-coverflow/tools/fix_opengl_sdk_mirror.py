#!/usr/bin/env python3
"""Restore withdrawn pinned ProsperoEden build inputs via exact-content mirrors.

Only temporary CI checkout tools/deps.json is modified. Archives retain their
original SHA-256 and Git dependencies retain their exact commit SHAs.
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

EXPECTED_SHA256 = "30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4"
ORIGINAL = "https://github.com/blackbearreloaded/ps5-opengl/releases/download/v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz"
MIRROR = "https://github.com/darkxex/ps5-opengl/releases/download/v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz"

# The selected forks are confirmed to expose these *same* pinned Git objects,
# not a different branch/version of the upstream code.
SOURCES = {
    "opengl-sdk": {
        "kind": "archive",
        "original": ORIGINAL,
        "mirror": MIRROR,
        "pin_key": "sha256",
        "pin": EXPECTED_SHA256,
    },
    "boilerplate": {
        "kind": "git",
        "original": "https://github.com/blackbearreloaded/ps5-native-app-boilerplate.git",
        "mirror": "https://github.com/drakmor/ps5-native-app-boilerplate.git",
        "pin_key": "commit",
        "pin": "dd44bbdc75437332ed22e3ba95126733419ef25a",
    },
    "lapy-owned-helper": {
        "kind": "git",
        "original": "https://github.com/blackbearreloaded/PS5-Lapy-JB-Daemon.git",
        "mirror": "https://github.com/Phononenom/PS5-Lapy-JB-Daemon-bbr.git",
        "pin_key": "commit",
        "pin": "57fcc19b7f57e46c823d6f2a6c6233ebf3288079",
    },
}


# The original ps5-homebrew-ui repo disappeared, but the exact pinned source
# commit exists in Yahballon/ps5-homebrew-ui. All original per-file SHA-256
# values remain in tools/deps.json and are revalidated by tools/deps.py.
UI_KIT_COMMIT = "aed286b81cdf70a814d3059f54633ba34844e46e"
UI_KIT_OLD = "https://raw.githubusercontent.com/blackbearreloaded/ps5-homebrew-ui/" + UI_KIT_COMMIT + "/"
UI_KIT_NEW = "https://raw.githubusercontent.com/Yahballon/ps5-homebrew-ui/" + UI_KIT_COMMIT + "/"
UI_KIT_SHA256 = {
    "src/ui/theme.cpp": "15ca512c61df48864ee787fa93b91d4f67d5103819e4a596eb4fa9bc38db42b6",
    "src/ui/theme.hpp": "9726e50acbf33a89494c1763f0fa4ba6464b152800bed6b8b8053aa41cc4a709",
    "src/gfx/draw_list.hpp": "8c19ec93966fe03f61f7ffdb701a39a1fb011436742949453a4801173478ee9f",
    "src/gfx/font.hpp": "c6fd392e18d6dc6c7dbaf104078f10f57af963fd116fb201c4c55de54d3c4ade",
    "src/gfx/font_format.hpp": "d8beee75b7c823bbe16a93258e65a67cb565d178c2b4dd9f241bea731d277484",
    "src/gfx/backdrop_spec.hpp": "0e03db2915bc0a7b65b280d0b9c817264289a69a6f86493e3c3bba4cbcf73852",
    "src/audio/cues.hpp": "ca3c3961ae2086abe429d76e8d7c30dea3b669c5a0a4553f3eaaad6932a0db2c",
    "src/audio/mixer.hpp": "7d438dcaa70fea00fb9121e74b644162f836adb353e47a0b104270c64405752c",
    "src/audio/stream_ring.hpp": "ffb6427b2a32662609cb1156971d748277573a2da0e2ed99bc6488c5da87be3b",
    "LICENSE": "3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986",
}


def switch_source(checkout: Path) -> str:
    deps = checkout.resolve() / "tools" / "deps.json"
    data = json.loads(deps.read_text(encoding="utf-8"))
    changed = []
    for name, spec in SOURCES.items():
        items = [entry for entry in data["items"] if entry.get("name") == name]
        if len(items) != 1:
            raise ValueError(f"Expected exactly one pinned dependency: {name}")
        item = items[0]
        if item.get("kind") != spec["kind"]:
            raise ValueError(f"{name}: unexpected dependency kind")
        if item.get(spec["pin_key"]) != spec["pin"]:
            raise ValueError(f"{name}: pinned checksum/commit changed; refusing substitution")
        if item.get("url") not in (spec["original"], spec["mirror"]):
            raise ValueError(f"{name}: unknown upstream URL; refusing substitution")
        changed.append((item, spec["mirror"], name, item["url"] != spec["mirror"]))
    kits = [entry for entry in data["items"] if entry.get("name") == "ui-kit"]
    if len(kits) != 1 or kits[0].get("kind") != "files":
        raise ValueError("Expected exactly one pinned ui-kit file dependency")
    entries = kits[0].get("files", [])
    if len(entries) != len(UI_KIT_SHA256) or {f.get("name") for f in entries} != set(UI_KIT_SHA256):
        raise ValueError("ui-kit file list drifted; refusing source substitution")
    for entry in entries:
        name = entry["name"]
        if entry.get("sha256") != UI_KIT_SHA256[name]:
            raise ValueError(f"ui-kit pin changed: {name}")
        if entry.get("url") not in (UI_KIT_OLD + name, UI_KIT_NEW + name):
            raise ValueError(f"ui-kit URL drifted: {name}")
        changed.append((entry, UI_KIT_NEW + name, "ui-kit:" + name,
                        entry["url"] != UI_KIT_NEW + name))
    # Atomic decision: validate the complete manifest before changing a byte.
    if not any(replaced for _, _, _, replaced in changed):
        return "all sources already pinned to verified mirrors"
    for item, mirror, _, _ in changed:
        item["url"] = mirror
    tmp = deps.with_suffix(".json.tmp")
    tmp.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    tmp.replace(deps)
    changed_names = [name for _, _, name, replaced in changed if replaced]
    return "redirected exact-pinned dependencies: " + ", ".join(changed_names)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    print(switch_source(parser.parse_args().source))
    for name, entry in SOURCES.items():
        print(f"{name}: {entry['pin_key']}={entry['pin']}")
    print(f"ui-kit: commit {UI_KIT_COMMIT}, {len(UI_KIT_SHA256)} file SHA-256 pins preserved")
