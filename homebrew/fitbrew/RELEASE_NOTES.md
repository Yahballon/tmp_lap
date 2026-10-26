# FITBREW v0.1.2 — Photo Edition

The v0.1.1 screen showed geometric silhouette artwork instead of professional photos, repeated images, stretched hero bodies and overflowing workout names. This update repairs the visual design while leaving the fitness engine unchanged.

- Bundles 12 distinct credited real-life sports photographs from freely usable Unsplash sources (strength, squats, planks, cardio and flexibility).
- Downloads/validates image sources during GitHub Actions and embeds all 12 JPEG and PS5 RGBA8 textures into the installer: entirely offline on PS5.
- Missing photo source now fails the build. No accidental stick-figure fallback will be released.
- Uses aspect-ratio-correct UV cover crops, readable clipped photo-card labels and two-line Today titles.
- Gives all 12 workout plans distinct photos while retaining the same 36 original exercises and bodyweight-only approach.
- Keeps PPSA99790 and the existing local FITBREW progress directory.

Validation: CI source/image/ZIP checks are automated. Real PS5 firmware 5.10 screenshot retest is still required. The photos illustrate workout categories, not exact step-by-step form instruction for each exercise.

Photography author/source credits: assets/PHOTO_CREDITS.md.
