# First native PS5 SDL2 build target

This make target compiles the **real, upstream Gold872 engine source**, pinned to a specific revision, using PlayStation 5's SDL2 libraries.

## Cross-compile
Run at the repository root:

```sh
bash homebrew/starbound-rebased/tools/fetch-engine.sh build/gold872-engine
cd homebrew/starbound-rebased/ps5
make UPSTREAM=../../../build/gold872-engine PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
```

PacBrew must have installed SDL2, SDL2_image, SDL2_mixer and SDL2_ttf to `target/user/homebrew/lib` within the PS5 SDK first. This may require dependency archives for image/mixer/TTF as well as system import stubs.

**Do not assume that compilation is proof of a working game:** upstream still loads non-redistributable media from `res/` and has PC-specific file paths, font loading, controller assumptions, and scene transitions. Running the output unmodified would not produce a lawful finished game. The required future work is a complete original-art replacement, an original level pack and port testing on real hardware.

This make target produces a **payload ELF**, not `eboot.bin` and not `PPSA99641.zip`. Do not send this prototype to TESTBUILDS or advertise it as a release until runtime issues are fixed.
