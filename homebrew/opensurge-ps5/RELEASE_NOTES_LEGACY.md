# Open Surge PS5 v0.5.0 — Original music and sound effects

Based on the real PS5 working v0.4.0 combat/physics/Spin Dash native preview.

- **16 authentic source Open Surge WAV effects** converted only to 48 kHz stereo PCM: jump, collectible pickup/loss, spring, roll, charge, charge boost, dash release, brake, damage, death, destroy, spikes, shots, goal and checkpoint.
- **Original demo music** `musics/template.ogg` decoded from genuine Ogg Vorbis and streamed at 48 kHz, matching the level's `music` metadata.
- Native PS5 `sceAudioOut` 256-frame output thread using ps5-homebrew-ui `Mixer`, with a roughly 0.5-s-ahead nonblocking music ring and per-effect clip voices.
- Triangle toggles music in the level; music fades off in the app's diagnostic menu.
- `ORIGINAL_AUDIO_INIT`, `AUDIO_PERF` / underruns and audio errors in the existing `/data/OpenSurge/legacy-port.log`.
- Audio initialization is optional: failure logs diagnostics but never blocks the already user-confirmed controller and graphics pipeline.

Not the complete Open Surge engine: SurgeScript, full stage flow, missing object logic, authentic enemy physics, dynamic loss rings and save state are still pending. Real PS5 test required to confirm actual sound output.
