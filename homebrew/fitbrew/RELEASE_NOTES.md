# FITBREW v0.1.1 — Native PS5 Image Loading Fix

Corrective update after PS5 user feedback: the fitness app appeared to work but training pictures did not display in v0.1.0.

- Pre-decode all six 512×512 exercise illustrations as RGBA8 during CI packaging, avoiding fragile on-console JPEG decoding.
- Retain JPEG fallback for older/manual packages and log exact image failure stages: missing bytes, decoded dimensions and GPU upload IDs.
- Prioritize optional original photo pack from `/data/FITBREW/assets/` or `/mnt/usb0..usb3/FITBREW/assets/`, no online account required.
- Verify all six RGBA8 assets, all six JPEGs, app metadata and PS5 launcher icon are inside the real installer ZIP.
- Preserve the Title ID `PPSA99790` and local workout history. All 36 exercises, 12 plans, native screens and DualSense controls remain unchanged.

**Validation:** Dedicated host asset tests plus native PS5 CI build. Hardware retest of image rendering is still required, with `/data/FITBREW/logs/fitbrew.log` as diagnostic evidence. This release contains bundled illustrations, not the separate generated high-resolution photography pack.
