# Drink Roulette v0.1.4 — Single wheel and restored glass bottle

- Fixed underlying bug: existing photographic artwork already contains the wheel; the native overlay had drawn a second wheel, offset about 34px right and 24px down.
- Photo mode now preserves its beautiful baked roulette slices and redraws just the dark hub and the **animated** transparent glass bottle.
- New artwork-derived 1024x1024 RGBA bottle sprite replaces green-line geometry. Its gold pivot is aligned with the hub.
- Full-opacity mask on stale sidebars and footer removes all duplicate labels.
- Player setup, 433 challenges and game modes preserved. Hardware rendering still must be checked.
