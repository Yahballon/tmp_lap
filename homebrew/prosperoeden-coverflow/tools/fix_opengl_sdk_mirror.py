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
