#!/usr/bin/env python3
"""Redirect the missing PS5 OpenGL SDK 1.0.2 asset to a byte-identical release mirror.

The dependency remains pinned to ProsperoEden's original SHA-256. Refuses any
unexpected hash or URL. This changes only the CI working checkout.
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

EXPECTED_SHA256 = "30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4"
ORIGINAL = (
    "https://github.com/blackbearreloaded/ps5-opengl/releases/download/"
    "v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz"
)
MIRROR = (
    "https://github.com/darkxex/ps5-opengl/releases/download/"
    "v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz"
)


def switch_source(checkout: Path) -> str:
    deps = checkout.resolve() / "tools" / "deps.json"
    data = json.loads(deps.read_text(encoding="utf-8"))
    sdk = [x for x in data["items"] if x.get("name") == "opengl-sdk"]
    if len(sdk) != 1:
        raise ValueError("Expected exactly one pinned opengl-sdk dependency")
    sdk = sdk[0]
    if sdk.get("sha256") != EXPECTED_SHA256:
        raise ValueError("OpenGL SDK digest changed; cannot silently replace dependency")
    if sdk.get("url") not in (ORIGINAL, MIRROR):
        raise ValueError("OpenGL SDK URL changed; refusing an unknown download")
    if sdk["url"] == MIRROR:
        return "already pinned to verified mirror"
    sdk["url"] = MIRROR
    deps.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return "switched to matching-SHA-256 GitHub release mirror"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    print(switch_source(args.source))
    print("SDK SHA-256 remains:", EXPECTED_SHA256)
