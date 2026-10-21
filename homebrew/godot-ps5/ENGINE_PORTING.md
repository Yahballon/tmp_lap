# Godot 4.7.2 -> PS5 Homebrew: actual gates

Upstream: `godotengine/godot` tag `4.7.2-stable` (MIT).

1. Cross-compile actual Godot core/modules using public jailbreak PS5 SDK; fix Godot/FreeBSD libc and toolchain incompatibilities shown by real compiler logs.
2. Link the static engine with PS5-DevBridge's native CRT, linker, import stubs and runtime. Run a script smoke fixture on hardware and inspect logs.
3. Add a native Godot `DisplayServer` and hardware-supported rendering backend (OpenGL/GLES or Vulkan if genuinely available). Godot's LinuxBSD display backend cannot be used unchanged.
4. Add DualSense input event mapping, audio stream playback/mixing, proper save directories, timers, event dispatch, app lifecycle and crash reporting.
5. Test graphical 2D scenes with scripts/assets, controller navigation, music, save/resume and clean exit.
6. Develop a packaged PS5 game-library UI/launcher with a tested icon, title metadata and versioned ZIP, then 3D/per-game compatibility.

The PCK header checker is **not** a complete pack parser, game compatibility verifier, or game runtime.

No verified PS5 cross-compile, linked executable, package, gameplay, live screenshot or PS5 hardware test exists in this milestone.
