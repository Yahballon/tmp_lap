# Next major milestone: native Godot display + controller for PS5

Status: **design/bring-up plan, not a working renderer**. Source baseline:
Godot 4.7.2; PS5 firmware 5.10; homebrew `PPSA99843`. The console
already executed `GODOT_PS5_SMOKE_READY` three times with v0.2.11.

## Current execution contract

The native Godot VM / GDScript / scene lifecycle is verified headlessly.
Experimental v0.3.0 adds external .pck selection and bundle fallback; this
is kernel-backed file discovery only, not a game browser or graphics backend.
The next acceptance test must launch an external PCK and show
`PACK_SOURCE=CONFIG` or `PACK_SOURCE=AUTOSCAN`, then verify the script
output and a clean PS5 shell exit. No renderer or pad support yet.

## Choose a real graphics path — two distinct options

**Candidate A: Godot Vulkan RenderingDevice.** Original Godot 4.7.2
sources: `drivers/vulkan/rendering_context_driver_vulkan.cpp`,
`drivers/vulkan/rendering_device_driver_vulkan.cpp` and
`drivers/vulkan/vulkan_hooks.cpp`. The port needs a real PS5
DisplayServer, a native Vulkan surface/swapchain bridge, function
dispatch without assuming a desktop Vulkan ICD, device features, shader
compilation, memory mapping, synchronization and teardown. Candidate
public graphics stacks to investigate:

- `mihawk-99/PS5_VulkanTemplate`: experimental Vulkan RADV-based PS5
  homebrew reference with framebuffer and pad examples; repository is MIT,
  **but third-party graphics components retain their own licenses**.
- `mpereiraesaa/ps5-vulkan`: experimental PS5 graphics/compute implementation;
  its static SDK is GPL-3.0 and **does not provide a conventional loader/ICD**.
  It requires compatibility work and GPL compliance if its library is linked.

**Candidate B: OpenGL compatibility renderer.** Godot's compatibility
renderer depends on a working OpenGL context + EGL window and Godot
DisplayServer integration. A previously indexed
`blackbearreloaded/ps5-opengl` repository currently returns GitHub 404,
so **do not pin a CI dependency to this unavailable URL**. Locate a live,
audited SDK with a demonstrably supported PS5 EGL/GL context before
enabling `opengl3=yes`. The current PS5 SCons target intentionally
sets `vulkan=no opengl3=no` to keep GDScript bring-up stable.

## Hardware-gated implementation steps

1. **Isolated GPU probe title** — verify a framebuffer/swapchain clear and
   present on the actual console, with an identifiable frame colour and
   clean Close Game behavior. Keep this separate from `PPSA99843` so the
   proven GDScript headless runtime is not regressed.
2. **DisplayServerPS5 minimal native API** — device/screen/window sizing,
   event pump, lifecycle, presentation, and renderer context ownership.
   Add validation for unimplemented virtual methods and fail at compile
   time rather than providing bogus successes.
3. **Godot 2D frame** — enable renderer in a separate experimental build,
   display an upstream Godot-generated scene, and verify visible motion
   at a stable refresh rate. Record a console screenshot and diagnostic
   `PRESENTED_FRAME` marker; a green CI build alone is insufficient.
4. **DualSense Input bridge** — create the real pad device, map pressed /
   released buttons plus analog axes to Godot Input, handle disconnect and
   reconnect, and verify events via a game scene (not just a raw pad poll).
5. **Audio + saves** — port PS5 AudioOut, buffer/latency handling,
   user:// writable directory behavior and crash-safe shutdown.
6. **First actual Godot game** — one small 2D project with animations,
   controls, audio, and persistent settings; only then attempt larger 3D
   content and renderer feature compatibility.

## Release discipline

- Never label headless GDScript success as graphical gameplay success.
- Keep `GODOT_PS5_SMOKE_READY` as a regression oracle.
- Continue signed `PPSA99843.zip` via GitHub Actions and TESTBUILDS only
  when link, packaging and signature tests pass.
- Keep graphics probe releases separate from the proven headless engine.
- Record which firmware and SDK commit each native witness used.
- Track license compliance for every linked graphics component and its
  transitive dependencies; no third-party GPU binaries or vendor SDK
  headers should be redistributed without a compatible license.

## Primary code and reference repositories

- [Godot Engine](https://github.com/godotengine/godot) — engine renderer
  and DisplayServer integration.
- [PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate)
  — experimental graphics/launcher examples (inspect exact versions).
- [ps5-vulkan](https://github.com/mpereiraesaa/ps5-vulkan) —
  experimental PS5 Vulkan implementation (GPL-3.0).
- [ps5link-sdk](https://github.com/Rufidj/ps5link-sdk) — PS5 GPU, VideoOut,
  and DualSense working examples; GPL-3.0.
