#!/usr/bin/env python3
"""Apply the isolated native PS5 entrypoint to exact upstream Open Surge source.

Does NOT claim to implement Allegro, audio, GPU, SDL2, or PS5 packaging.
"""
import argparse
import shutil
from pathlib import Path

INCLUDE = '#include "core/commandline.h"'
ENTRY = "    commandline_t cmd = commandline_parse(argc, argv);"
INCLUDE_PATCH = '#if defined(OPENSURGE_PS5)\n#include "ps5_launcher.h"\n#endif'
ENTRY_PATCH = '#if defined(OPENSURGE_PS5)\n    opensurge_ps5_prepare_arguments(&argc, &argv);\n#endif'


def apply(source: Path, header: Path) -> None:
    main = source / "src" / "main.c"
    destination = source / "src" / "ps5_launcher.h"
    data = main.read_text(encoding="utf-8")
    if INCLUDE_PATCH in data or ENTRY_PATCH in data:
        raise ValueError("Already patched; start from a pristine upstream checkout")
    if data.count(INCLUDE) != 1 or data.count(ENTRY) != 1:
        raise ValueError("Upstream entrypoint changed: refusing unsafe patch")
    if destination.exists():
        raise ValueError("Header already present: refusing overwrite")
    patched = data.replace(INCLUDE, INCLUDE + "\n" + INCLUDE_PATCH)
    patched = patched.replace(ENTRY, ENTRY_PATCH + "\n" + ENTRY)
    if patched == data:
        raise AssertionError("Expected entrypoint changes")
    shutil.copy2(header, destination)
    main.write_text(patched, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("upstream", type=Path)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    apply(args.upstream, project / "ps5" / "ps5_launcher.h")
    print("PS5 launch-argument integration added to original Open Surge main.c")


if __name__ == "__main__":
    main()
