# ZQuest Classic PS5 v0.1.5 — Native 8-Bit Video Test

**Experimental TESTBUILD · by YAHBALLON · PPSA99127**

**NOT PLAYABLE:** This release still does not link the upstream ZQuestClassic
`zplayer` game loop. It adds a safe, optional PS5 GPU rendering diagnostic.

## New video test

- **Triangle** toggles a clearly labeled original 256×224 moving pixel test
  pattern in the right panel; pressing Circle first closes the video test.
- Converts 8-bit indexed bitmaps with Allegro 6-bit RGB palette data to RGBA8,
  respecting source row pitch and buffer limits.
- Reuses a single GL_RGBA8 texture, updating with `glTexSubImage2D` and
  nearest-neighbor pixel filtering through the PS5-DevBridge EGL renderer.
- Native startup is unchanged: GPU palette test initializes lazily on
  Triangle, so the existing quest browser remains usable if you never enable it.
- Logs `PS5_INDEXED_VIDEO_BRIDGE_READY` or
  `PS5_INDEXED_VIDEO_BRIDGE_FAILED`, plus `VIDEO_TEST_ON/OFF`
  under `/data/ZQuestClassic/logs/port.log`.

## Unchanged working functions

- Directory listing via kernel-based getdents under
  `/data/ZQuestClassic/quests`.
- **X** sequentially reads selected .qst with progress + CRC32 for
  `the_deep.qst` and `255example.qst`.
- Square rescans; Options shows folder-access diagnostics.

## Testing

Install v0.1.5 from TESTBUILDS, press Triangle, and check that the
animated palette pattern appears and keeps moving without artifacts.
Press Triangle again to return to the library, then test X on your quests.
If video fails or the app crashes, send the log. Real PS5 GPU behavior
has not been validated on hardware yet.

**No Nintendo artwork, commercial quests or proprietary game assets are included.**
Upstream `zplayer`, Allegro-Legacy, script interpreter, sound, saves and
real gameplay still require full native engine integration.
