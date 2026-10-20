# Drink Roulette v0.1.5 — Missing Bottle Fix

- Fix real PS5 `BOTTLE ASSET MISSING` screen: native loader first reads a 512×512 RGBA8 sprite generated and shipped with the PS5 app, avoiding PNG decoding.
- Fallback to PNG from native installed app, managed data and USB paths if raw image loading fails.
- Remove brittle exact-size checks when rendering a valid GPU texture; keep a bottle-shaped native fallback instead of error text.
- Write path, load size, decoder failure and graphics allocation status to `/data/DrinkRoulette/drink-roulette.log`.
- CI verifies the raw image is pixel-identical to the source PNG both in the signed title folder and final `PPSA99841.zip`.
- Keep current 433 challenges, modes, player setup, roulette wheel and corrected side-panel layout.
- Real-device rendering must be verified on a jailbroken PS5.
