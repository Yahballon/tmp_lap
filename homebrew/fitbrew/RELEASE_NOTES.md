# FITBREW v0.1.3 — Original AI Workout Artwork Built In

The twelve AI-generated workout images requested by the user are now baked into the **native PS5 homebrew app**; no USB stick, separate photo pack, connection or manual installation of assets.

- 12 images from the previously created FITBREW AI batch, correctly mapped by each workout's `plans[].image` index.
- Includes 12 JPEG files and 12 ready-to-upload 512x512 RGBA8 GPU textures inside `PPSA99790.zip`.
- GitHub Actions imports and checks the image sources, persists the source JPEGs to the repository, then builds the native app using those files.
- Bundled art takes priority over old USB/data photo overrides.
- Title ID `PPSA99790` and `/data/FITBREW/progress.ini` are unchanged, preserving previous workout progress.
- Same 36 exercises, 12 workouts, timings and native DualSense UI.

The first test is still on PS5 firmware 5.10. Original artwork provides attractive per-*program* photos; exercise-level photo specificity and motion instruction are still future work.

See `assets/PHOTO_CREDITS.md` for photo-to-program mapping.
