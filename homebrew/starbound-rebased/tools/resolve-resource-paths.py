#!/usr/bin/env python3
"""Give the actual Gold872 game engine absolute native PS5 resource paths.

The original engine passes many different 'res/...' literal paths to SDL2,
SDL_image, SDL_ttf, SDL_mixer and C++ streams. Changing process cwd is not
sufficient on all PS5 payload/ShadowMount libc implementations.
"""
from pathlib import Path
import re
import sys

root=Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/gold872-engine")
source=root/"src"
assert (root/"include/StarboundAssetPaths.hpp").is_file(), "Resource resolver header missing"

main=source/"main.cpp"
text=main.read_text()
old='''   // ELF loader does not guarantee the game's working directory.
   if (access("res/sprites/characters/PlayerSpriteSheet.png", F_OK) != 0) {
      if (chdir("/app0/assets") != 0 && chdir("/data/homebrew/PPSA99642/assets") != 0)
         chdir("/mnt/usb0/STARBOUND_REBASED");
   }'''
assert old in text, "Upstream startup path block has changed"
text=text.replace(old,'''   // Resource access is now independent of working-directory changes and is resolved
   // via absolute verified app0/homebrew paths for every file open.''')
main.write_text(text)

pattern=re.compile(r'"res/[^"\n]*"')
patched={}
for file in sorted(source.rglob("*.cpp")):
    text=file.read_text()
    matches=pattern.findall(text)
    if not matches:
        continue
    assert '#include "StarboundAssetPaths.hpp"' not in text, file
    rewritten=pattern.sub(lambda m: f"SB_ASSET_PATH({m.group(0)})",text)
    file.write_text('#include "StarboundAssetPaths.hpp"\n'+rewritten)
    patched[file.relative_to(root).as_posix()]=len(matches)
assert sum(patched.values())>=30, f"Expected many original resource paths: {patched}"
assert "src/main.cpp" in patched, "Missing probes of actual PS5 files"
assert "src/scenes/MenuScene.cpp" in patched, "Menu level not made absolute"
assert "src/systems/MenuSystem.cpp" in patched, "Logo not made absolute"
assert "src/systems/RenderSystem.cpp" in patched, "Font not made absolute"
assert "src/SoundManager.cpp" in patched, "Audio not made absolute"
assert "src/Map.cpp" in patched, "Irregular map references not made absolute"
for file in source.rglob("*.cpp"):
    contents=file.read_text()
    wrapped_prefix="SB_ASSET_PATH("
    for match in pattern.finditer(contents):
        prefix=contents[max(0,match.start()-len(wrapped_prefix)):match.start()]
        assert prefix==wrapped_prefix, f"Unconverted relative resource path: {file}: {match.group(0)}"
print(f"Patched {sum(patched.values())} relative game-asset paths in {len(patched)} source files:")
for k,v in patched.items():
    print(" ",k,v)
