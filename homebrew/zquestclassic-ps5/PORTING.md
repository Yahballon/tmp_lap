# ZQuest Classic → PS5 native-port engineering notes

## Scope and provenance

Upstream: https://github.com/ZQuestClassic/ZQuestClassic, revision
\`c44dde7d0146da78bf8b541af457d5ff4e67a830\`. Upstream is GPLv3.
The **actual playable game** is upstream CMake target \`zplayer\`, not \`zlauncher\`
(the desktop launcher) or \`zeditor\` (a mouse-first level editor).

This initial PS5 app **does not compile or execute upstream zplayer**. It is a
controller-first native quest-discovery prototype to exercise UI, storage,
input, packaging, and SDK, without misrepresenting a fake engine as a port.

## Technical blockers to playable ZQuest Classic

1. **Allegro 5 and Allegro-Legacy:** ZQuest Classic implements legacy Allegro
   APIs through Allegro-Legacy and a patched Allegro 5 fork. Allegro's X11,
   Win32, macOS and browser targets do not supply a PS5 EGL/display/pad/audio
   backend. Implement/verify display bitmap updates, event loop, audio mixer,
   timer, input mapping, datafile routines, and cleanup for the PS5 SDK.
2. **zplayer linkage:** Cross-compile its shared core, VM/ZScript support,
   gameplay objects, resources and vendored third-party libs as PS5 objects.
   Use \`-DJIT_BACKEND=none\` initially rather than attempting platform-specific
   executable-memory JIT paths. Treat this as a hypothesis pending compilation.
3. **Filesystem:** Remap current working directory, shipped datafiles, config,
   saves, quest paths, logging and asset lookup to tested PS5 readable/writable
   paths. Never require writable \`/app0\`. Validate \`/mnt/usbN\` mount access.
4. **DualSense:** Bridge PS5 pad state to the engine's actual Allegro keyboard
   and gamepad event semantics, preserving gamepad rebinding, menus and pause.
5. **Graphics/audio timing:** Exercise full quest drawing and game tick/audio
   in CI/console, including shaders vs software bitmap fallback and music.
6. **Packaging/resources:** Ship permissible upstream engine resources and
   full third-party GPL notices. Do not bundle Nintendo-owned game/quest art
   or paid/unauthorized quests. Run upstream recorded input replays and save
   compatibility tests when the renderer actually exists.

## Completed in the first prototype

- Native PS5 UI staged from \`PS5-DevBridge/templates/simple-calc\`.
- Input (D-pad/left stick, Cross, Circle, Square, Options) and exit lifecycle.
- Incremental bounded quest filename discovery (depth 3, 256 files, 6000 entries),
  case-insensitive \`.qst\` extension, symlink guard and nonblocking per-frame batches.
- Game library/detail display (filename and size only; no quest parsing).
- Native ELF packaging and ZIP checks, procedural icon, host unit tests.
- No fake Play control; gameplay and upstream engine absent by design.

## Acceptance gates for a *real* playable port

- \`zplayer\` source from a documented upstream commit cross-compiles and links
  without stubbed gameplay, into the shipped PS5 binary.
- A freely redistributable original test quest opens; rendering, controls,
  audio, transitions, save/resume and exit work on real jailbroken hardware.
- Replays/host regression checks and on-console logs confirm behavior.
- Only then label the release "playable" and add it to TESTBUILDS \`version.json\`.

Do not treat a green prototype workflow as evidence of playable ZQuest Classic.
