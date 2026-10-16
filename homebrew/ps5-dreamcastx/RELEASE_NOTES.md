# PS5DreamcastX v0.1.6

## Faster, nonblocking artwork in the game library

- Eliminated synchronous cover JPEG/PNG decoding on the PS5 menu thread.
- One background worker loads and shrinks cover images to a maximum of
  640 pixels, reducing PS5 GPU texture upload time and memory.
- Selected game and nearest neighbours are preloaded ahead of distant covers.
- Finished textures upload from the graphics thread at most twice per frame.
- Bounded waiting/staging queue avoids large-library RAM spikes.
- Existing driver texture cache continues to serve already-seen covers.
- v0.1.5's working DVD-style case artwork renderer is unchanged.
- No changes to save data, games, BIOS, USB discovery or controls.

ZIP: `PPSA99765.zip` · Title: `PPSA99765`.
