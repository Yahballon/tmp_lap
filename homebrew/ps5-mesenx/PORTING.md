# MesenCE port architecture / engineering checklist

## Why this is not a core-file swap

PS5SNESX uses the bsnes-mercury libretro ABI and a session frontend
(`snes/src/core_session.cpp`); upstream MesenCE is a native C++ emulator
using its own emulation interfaces and InteropDLL. Directly calling
`retro_load_game` will **not** load MesenCE. Its desktop UI depends on
.NET 10 and cannot be shipped as the PS5 UI.

Keep the existing PS5SNESX source for a proven PS5 Vulkan shelf, and
implement a native C++ MesenCE session adapter rather than porting the
Avalonia/.NET interface. Do not reuse the existing app ID PPSA99205.
Allocate a unique identifier following the community policy before release.

## Native tasks

1. Integrate MesenCE C++ `Core`, `Utilities`, `InteropDLL` and transitive
   dependencies (including image codecs and HD audio) with PS5 Clang 18.
2. Replace bsnes-mercury callbacks with an explicit lifecycle:
   initialize -> NES ROM load -> tick -> video/audio -> input -> SRAM/states -> unload.
3. Configure `NesConfig.EnableHdPacks` and map Mesen's
   `FolderUtilities::GetHdPackFolder()` to
   `/data/PS5MesenX/HdPacks`, with optional USB pack discovery.
   The existing `HdPackLoader::InitializeLoader` requires
   `<HdPacks>/<ROM stem>/hires.txt`.
4. Update all hardcoded SNES-only assumptions. The existing
   `Session::Video` rejects frames greater than 512x576: up to 10x
   scaling in MesenCE HD packs requires dynamic frame geometry,
   overflow-safe strides and GPU texture reallocation. Avoid a
   512x576 cap or preallocating the maximum resolution.
5. Preserve texture aspect ratio and selectable integer scaling. Ensure
   HD audio and A/V pacing can cope with variable NES/NTSC/PAL output.
6. Keep per-ROM SRAM, states and controller mappings isolated from PS5SNESX
   paths. Do not assume Mesen state compatibility with bsnes states.
7. Compile the native app and package `eboot.bin` plus all required
   `sce_sys` and runtime files; perform real-console testing.
8. Only when that passes, add an `app.json` and release workflow to
   make TESTBUILDS discover the application.

## Dependencies and provenance

- `upstream/PS5SNESX`: selected necessary folders from the pinned
  PS5SNESX checkout; the original remains unchanged.
- `upstream/MesenCE`: MesenCE source including NES HD pack rendering.
- `upstream/UPSTREAM_SOURCES.json`: revision manifest written by sync.
- Licenses are copied from each upstream with their source.

The sync workflow is **not** a native PS5 compiler and must not publish
a release or advertise game playback as working.
