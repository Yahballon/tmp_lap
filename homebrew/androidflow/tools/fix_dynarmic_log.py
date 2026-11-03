#!/usr/bin/env python3
"""Declare Hydroid's droid_log C ABI for patched suyu Dynarmic C++ translation.

Upstream PS5 dynarmic patch adds droid_log() calls in a64_interface.cpp but
doesn't declare the function there. Without this declaration, Clang C++ stops.
This patch is idempotent and source-pinned; it does not stub/remove JIT logic.
"""
import argparse
from pathlib import Path

DECL = 'extern "C" void droid_log(const char *fmt, ...);'
MARKER = "namespace Dynarmic::A64 {"
RELATIVE = Path("src/dynarmic/backend/x64/a64_interface.cpp")

def patch(root: Path) -> bool:
    target = root / RELATIVE
    original = target.read_text(encoding="utf-8")
    if DECL in original:
        return False
    if original.count(MARKER) != 1:
        raise RuntimeError(f"Dynarmic namespace anchor changed in {target}")
    modified = original.replace(MARKER, DECL + "\n\n" + MARKER, 1)
    target.write_text(modified, encoding="utf-8")
    return True

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="pinned suyu-emu/dynarmic checkout")
    args = parser.parse_args()
    changed = patch(args.source)
    print(f"Dynarmic droid_log declaration: {'inserted' if changed else 'already present'}")

if __name__ == "__main__":
    main()
