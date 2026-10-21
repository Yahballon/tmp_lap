#!/usr/bin/env python3
"""PS5-only portability patch for pinned SurgeScript 0.6.1 malloc.h behavior."""
from __future__ import annotations
import argparse
from pathlib import Path

ANCHOR = '''/* require alloca */
#if !(defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh))
#include <malloc.h>
#if defined(__linux__) || defined(__linux) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif
#endif
'''
REPLACEMENT = '''/* require alloca; the PS5 SDK deliberately forbids <malloc.h> */
#if defined(OPENSURGE_PS5)
#define alloca __builtin_alloca
#elif !(defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh))
#include <malloc.h>
#if defined(__linux__) || defined(__linux) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif
#endif
'''


def apply(source: Path) -> None:
    file = source / "src" / "surgescript" / "runtime" / "program.c"
    text = file.read_text(encoding="utf-8")
    if text.count(ANCHOR) != 1 or REPLACEMENT in text:
        raise ValueError("Unexpected SurgeScript source; no unsafe patch attempted")
    file.write_text(text.replace(ANCHOR, REPLACEMENT), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("upstream", type=Path)
    args = parser.parse_args()
    apply(args.upstream)
    print("PS5-only alloca bridge applied to pinned SurgeScript source")
