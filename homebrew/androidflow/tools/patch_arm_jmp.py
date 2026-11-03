#!/usr/bin/env python3
"""Enable Hydroid's existing ARM64-native setjmp/longjmp for Fruity Game.

The Hydroid source already contains guest AArch64 stubs in src/arm_imports.c,
but enables them only for Candy Crush. Fruity Game otherwise hits the fatal
host linux_abi l_longjmp(). Reuse the guest stubs without modifying Dynarmic
or changing the ABI path of any other game.
"""
import argparse
from pathlib import Path

PACKAGE = "page.codeberg.marshreaper.fruitygame"
ORIGINAL = '#define ARM_JMP_GAMES "com.king.candycrushsaga" // setjmp / longjmp in ARM code'
REPLACEMENT = (
    '#define ARM_JMP_GAMES "com.king.candycrushsaga,'
    + PACKAGE + '" // setjmp / longjmp in ARM code'
)
LOG_ANCHOR = (
    '    if (thunk && (!strcmp(name, "memcpy") || !strcmp(name, "memset") '
    '|| (!strcmp(name, "strcmp") && !host_strcmp) ||'
)
LOG_REPLACEMENT = (
    '    if (thunk && (is_setjmp(name) || is_longjmp(name)) && '
    'droid_game_in("' + PACKAGE + '"))\n'
    '        droid_log("androidflow: ARM64-native %s enabled for Fruity Game", name);\n'
    + LOG_ANCHOR
)

def patch_source(source: str) -> str:
    if source.count(ORIGINAL) != 1:
        raise RuntimeError("Hydroid ARM_JMP_GAMES definition changed: refusing unsafe patch")
    if source.count(LOG_ANCHOR) != 1:
        raise RuntimeError("Hydroid arm_import trampoline hook changed: refusing unsafe patch")
    if "if (is_setjmp(name)) return &code[70];" not in source:
        raise RuntimeError("Hydroid guest-native setjmp code unavailable")
    if "if (is_longjmp(name)) return &code[84];" not in source:
        raise RuntimeError("Hydroid guest-native longjmp code unavailable")
    if "((is_setjmp(name) || is_longjmp(name)) && droid_game_in(ARM_JMP_GAMES))" not in source:
        raise RuntimeError("Hydroid import routing changed; do not bypass host ABI")
    return source.replace(ORIGINAL, REPLACEMENT, 1).replace(
        LOG_ANCHOR, LOG_REPLACEMENT, 1)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--source", type=Path, required=True)
    args = p.parse_args()
    file = args.source / "src/arm_imports.c"
    if not file.is_file():
        p.error("Hydroid src/arm_imports.c does not exist")
    file.write_text(patch_source(file.read_text()))
    print("AndroidFlow: Fruity Game uses native AArch64 setjmp/longjmp; other packages unchanged")

if __name__ == "__main__":
    main()
