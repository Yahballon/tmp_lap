#!/usr/bin/env python3
"""Patch the pinned itsPLK WebKit Autoloader v0.4.0 into an isolated PS5 Game tile.

No exploit source or payload chain is changed. This only changes the homescreen
installer identity and the param.json template consumed by gen_version.py.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

UPSTREAM_VERSION = "0.4.0"
MEDIA_ID = "WKAL00001"
GAME_ID = "WKAG00001"
GAME_CONTENT_ID = "IV9999-WKAG00001_00-WEBKITAUTOLOADER"
DEEPLINK = "http://127.0.0.1:18181/app/index.html"


def patch(root: Path) -> None:
    header_path = root / "include" / "wkali.h"
    template_path = root / "assets" / "param.json.template"
    if not header_path.is_file() or not template_path.is_file():
        raise ValueError("Expected v0.4.0 upstream source tree, not a build output")

    header = header_path.read_text(encoding="utf-8")
    if f'#define WKAL_VERSION "{UPSTREAM_VERSION}"' not in header:
        raise ValueError("Refusing to modify an unexpected upstream version")

    original_id = re.search(r'(#define WKAL_TITLE_ID\s+)"([^"]+)"', header)
    if not original_id or original_id.group(2) not in (MEDIA_ID, GAME_ID):
        raise ValueError("Unexpected upstream launcher title ID")
    header = re.sub(
        r'(#define WKAL_TITLE_ID\s+)"[^"]+"',
        lambda match: f'{match.group(1)}"{GAME_ID}"',
        header, count=1,
    )
    # Keep the test installer's process name separate from upstream.
    header = header.replace('#define WKALI_THREAD_NAME "wkali.elf"',
                            '#define WKALI_THREAD_NAME "wkag.elf"')
    header_path.write_text(header, encoding="utf-8")

    param = json.loads(template_path.read_text(encoding="utf-8"))
    if param.get("titleId") not in (MEDIA_ID, GAME_ID):
        raise ValueError("Unexpected param.json titleId")
    if param.get("deeplinkUri") != DEEPLINK:
        raise ValueError("Upstream deeplink changed; aborting instead of breaking offline entry")
    if param.get("applicationCategoryType") not in (0, 65536):
        raise ValueError("Unexpected upstream application category")

    # These properties describe the homescreen shortcut, not an executable
    # game. Sony's launcher support for Game-category deeplinks is unverified.
    param["titleId"] = GAME_ID
    param["applicationCategoryType"] = 0
    param["contentBadgeType"] = 1
    param["applicationDrmType"] = "free"
    param["contentId"] = GAME_CONTENT_ID
    param["contentVersion"] = "01.000.000"
    param["localizedParameters"]["en-US"]["titleName"] = (
        "WebKit Autoloader GAME v[[VERSION_PLACEHOLDER]]"
    )
    template_path.write_text(
        json.dumps(param, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"Patched WebKit Autoloader {UPSTREAM_VERSION}: "
          f"{MEDIA_ID} (Media) -> {GAME_ID} (Game); deeplink unchanged")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=Path)
    args = parser.parse_args()
    patch(args.upstream.resolve())


if __name__ == "__main__":
    main()
