# FITBREW Original AI Workout Art — v0.1.3

All 12 workout photos were generated for FITBREW in the user-requested October 26, 2026 AI batch. They are not third-party Unsplash source photos and **not** real exercise technique demonstrations.

Each image is converted by `generate_assets.py` into 512×512 JPEG and GPU RGBA8 and bundled in `PPSA99790.zip`; the console never has to download photos or load an extra USB pack.

| Workout | AI image source |
| --- | --- |
| Full Body Starter | `assets/pose_0.jpg` |
| Low Impact Cardio | `assets/pose_3.jpg` |
| Core Control | `assets/pose_2.jpg` |
| Full Body HIIT | `assets/pose_4.jpg` |
| Leg Day | `assets/pose_1.jpg` |
| Chest & Core | `assets/pose_6.jpg` |
| Morning Energy | `assets/pose_11.jpg` |
| No-Jump Sweat | `assets/pose_10.jpg` |
| Bodyweight Burn | `assets/pose_9.jpg` |
| Posture Reset | `assets/pose_5.jpg` |
| Glutes & Legs | `assets/pose_7.jpg` |
| Quick 10 | `assets/pose_8.jpg` |

Two photo crops (pose_7 and pose_8) were derived from the existing AI-generated FITBREW workout contact sheet. The first GitHub Actions v0.1.3 release imports the original art into the repository and commits all 12 JPEGs to the `main` branch.

**Release validation:** `tests/art_test.py` checks 12 distinct sources, 12 exact-sized GPU RGBA textures, 12 fallback JPEGs and the icon. The PS5 installer validation checks all 24 image files inside the actual ZIP. This verifies packaging, not image semantics or real PS5 GPU behavior.
