# M27 hardware acceptance – Godot PS5 2D Platformer Starter Kit

Status: **READY FOR PHYSICAL TEST**, not yet tested with M26.
Date: 2026-10-24. Signed engine baseline: **v0.2.10, PPSA99847, Godot 4.7.2 GLES3**, PS5 firmware 5.10. **Do not rebuild or modify this known-good engine for this test.**

## Why this test

Real PS5 TAR #46 and user feedback establish the first ~50-second **60 FPS, no-stall Pong acceptance** on v0.2.10. After the initial warmup window, there were 9 consecutive intervals of 300 frames over 5 seconds with 0 >=80ms hitch counts. Only meaningful native difference vs v0.2.9 was stdout/stderr buffering with `M26_IO_A_B_MODE=1`. External gameplay remains an unverified separate acceptance gate.

The real 2D Platformer Starter Kit 1.1.1 (Godot Asset Library #2201) was packaged as **M12d** and already worked as a scene in older builds, albeit with progressive frame stalls. The user's archive `bfpilot-download (46).tar` contains `launch.txt.bak` with exactly `m12d-platformer-asset2201.pck`, and prior runtime history proves the loader resolves it to `/data/GodotPS5/games/m12d-platformer-asset2201.pck`. Earlier uploaded TAR #31 includes the original 3,325,680-byte PCK. SHA256:
`6d2145876d9c73e9d33c5c98336c56bb6e0339636e38b5ea58367c7f28ed05f2`.

## Activation – no new TESTBUILD homebrew needed

1. Ensure the user has **Godot PS5 GLES3 Canvas v0.2.10** installed through TESTBUILD (the already released signed title).
2. Check that `/data/GodotPS5/games/m12d-platformer-asset2201.pck` exists. Previous logs suggest it does.
3. Rename **`/data/GodotPS5/launch.txt.bak` to `/data/GodotPS5/launch.txt`** (its single line is `m12d-platformer-asset2201.pck`). Or create the same text if backup is missing. If the PCK is missing, use the original M12d package or supplied companion kit to copy `games/m12d-platformer-asset2201.pck`.
4. Keep `m26_log_mode.txt` absent, because **buffered logs are essential** to validated stable M26 performance. Keep `present_mode.txt` absent or `vsync`; leave native AudioOut enabled.
5. Fully close/restart same installed **PPSA99847**. Do not modify PS5 firmware or replace the engine EBOOT. Verify latest `gles3-runtime.log` says `PACK_SOURCE=CONFIG PACK=/data/GodotPS5/games/m12d-platformer-asset2201.pck` and `M26_IO_A_B_MODE=1`.

## Test matrix

| Subsystem | Physical action | Pass condition |
| --- | --- | --- |
| Scene loading | Start external M12d PCK | Fully loads both game menus/level 1 without remaining frozen |
| Controls | Left stick or D-pad move; X jump | Responds within frame, no repeat-button input delay |
| Collision + camera | Play Level 1 and Level 2 with jumps and moving camera | No severe geometry/collision regression, no seconds-long motion freezes |
| Rendering | Run / move continuously for 3–5 minutes | Ideally ~60 FPS; no progressive stalls / >=80ms bursts recurring |
| UI + score | Collect coins, restart, change level | HUD and interaction update reliably |
| Audio | Trigger game effects | Native AudioOut works; audio does not produce frame stalls |
| Regression | Rename `launch.txt` back to `launch.txt.bak` | Built-in Pong again loads at validated 60 FPS |

### Logs

The M26 stdout/stderr use static 128KiB full buffering. If the app is forcibly terminated, recent lines may not reach disk. Prefer normal exit if offered; collect `gles3-runtime.log` independently to confirm loaded pack and mode. For measured FPS, isolate lines from the **last session** after the external `PACK_SOURCE=CONFIG` marker, not appended older Pong summaries. Look for `GODOT_PS5_M13_FRAME_5S` with ~300 frames/5 sec, `M24_RENDER_5S`, `M25_VIEWPORT_5S`, M17 frame hitches and M12d script controls/audio markers.

Send user's next TAR and qualitative performance. **Do not mark external game / general Godot compatibility stable before that evidence arrives.** Preserve release `godot-ps5-gles3-canvas-v0.2.10` and main `version.json` with unchanged slug and title ID. This is a **test activation**, not a firmware update or an engine code change.

## Source / licenses

Upstream [2D Platformer Starter Kit](https://github.com/AdilDevStuff/2D-Platformer-Starter-Kit) pinned at `a48fc994cbcb168be472df84f2e13b55d2a2792f`. Original M12d packaging includes MIT license and credits (G2P Studios/AdilDevStuff, Kenney.nl art, Gdfxr sound). Do not expose runtime user's own uploaded PCK publicly in repo without explicit request; share the test kit directly with user only.
