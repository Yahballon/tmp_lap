# Open Surge PS5 v0.2.1 – Magenta transparency and collision preview fix

Firmware 5.10 hardware preview test after the user's v0.2.0 screenshot revealed severe hot-pink rectangles, an invisible/unclear player and unreliable physics. This revision preserves the known-working PS5-DevBridge native OpenGL stack.

- Convert Open Surge's original magenta color-key (`#FF00FF`) into real transparent RGBA; clear RGB on transparent pixels.
- Verify that frame 0 of the original Surge spritesheet actually has visible nontransparent pixels.
- Read the original `images/sunshine_mask.png` collision atlas and use its solid pixel coverage (not large flat bounding rectangles) for the native motion prototype.
- Detect floors, cloud platform tops, wall/ceiling hits and short step-ups. Add velocity-based movement and jump; bounded timestep.
- Read only the latest DualSense packet when determining held buttons; prevent repeat resets from duplicate pad samples.
- Show player coordinates, grounded flag, vertical speed and tiles rendered. Log spawn, jump, landing and reset events to `/data/OpenSurge/legacy-port.log`.
- Keep all original level art, scripts and asset attribution.

**Limitations:** This is NOT the original Open Surge physics system and does not execute gameplay SurgeScript, enemies, rings, audio, save games, dynamic obstacles or original level transitions. Verify transparency and whether the player is visible in the next PS5 screenshot. No console validation claimed until tested.
