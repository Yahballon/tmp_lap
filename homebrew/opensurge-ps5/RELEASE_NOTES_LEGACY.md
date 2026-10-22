# Open Surge PS5 v0.6.1 — Original music for every Sunshine stage

Building on v0.6.0's four native playable preview stages, stage selection and automatic goal progression now select the authentic upstream `.lev` music:

- Demo: `musics/template.ogg`.
- Sunshine Paradise Acts 1, 2 and 3: `musics/sunshine.ogg`.
- Two persistent 48-kHz stereo Vorbis decode streams crossfade through the native PS5 `sceAudioOut` mixer in 0.18 seconds. There is no interruption to effects or PS5 graphics.
- The stage loader records `STAGE_MUSIC` diagnostic lines and falls back gracefully if one track is missing.
- The complete signed native package still has four original stages, stage selection and goal transitions, checkpoints/progress.cfg, original enemies, rings and springs, sound effects, and the previous collision/Spin Dash safety logic.

This remains a native compatibility preview rather than full SurgeScript gameplay. Test the track switching between demo-1 and Sunshine Act 1 on a real PS5.
