# Open Surge PS5 v0.2.0 — original demo level preview

**Native PS5 homebrew experimental test, NOT the original Open Surge gameplay engine.**

Compared with the confirmed on-console 0.1.1 diagnostic:

- Parses original Open Surge 0.6.1.3 "levels/demo-1.lev" and "themes/sunshine.brk" to create a native preview cache from real 1,323 placed blocks, original graphics, and full brick definitions.
- Renders the exact original "images/sunshine.png" bricks with sprite-sheet UVs via the proven PS5-DevBridge / ps5-homebrew-ui OpenGL renderer.
- Renders and animates the original Surge character frames from "images/players/surge.png".
- Move with D-pad / left stick; Cross jumps; Square resets spawn; Circle returns to the diagnostic screen.
- First experimental gravity and simple flat-top platform collision. Collision masks, ramps/slopes, rings, enemies, cinematics, save data, audio, SurgeScript VM/gameplay, and full original Allegro/physics engine are **NOT** integrated.
- Keeps the complete original Open Surge game files and their licenses, native test menu and /data/OpenSurge/legacy-port.log diagnostics.

Entering the preview is not a playable completion of Open Surge. Test console boot and screenshot rendering first; share any level errors from the log. This release should remain visibly labeled an experimental test.
