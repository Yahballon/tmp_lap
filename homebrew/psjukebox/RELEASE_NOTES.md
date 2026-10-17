# PSJukebox v0.1.1 — USB music permission fix (TESTBUILD)

**Fix:** USB and local music were not found because the sandbox denied directory access with errno 1 (EPERM).

- Add a read-only filesystem helper `jbxscan.elf` to the native ZIP.
- Enumerate artist/album music files, logo.png, theme.ini, cover and full-screen background files via a dedicated localhost relay.
- Launch the helper via an already-running elfldr on localhost :9021; helper serves port :9188.
- Perform media scans and artwork reads asynchronously, preserving native album carousel and individual artist themes.
- Copy songs on demand to a validated local cache with progress and cancellation; never edit originals.
- Include host helper integration testing and installation package checks.

**Setup:** On USB use `PSJukebox/Music/Artist/Album/` or `Music/Artist/Album/`. Start a compatible elfldr before opening PSJukebox. Extract `PPSA99947.zip` under `/data/homebrew/`.

**Limitations:** USB helper permissions depend on firmware and loader, and require a real PS5 hardware test. Audio and images remain user-provided; no commercial content is bundled.

This GitHub Release is marked TESTBUILD in its title and published as a normal release so TESTBUILDS schema 2 can discover it.
