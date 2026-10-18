# PSJukebox v0.1.15 — Fast Coverflow Startup

- **Instant repeat launches:** restore last known artist/album/cover catalog from `/data/PSJukebox/library-v3.cache` before background discovery
- **Progressive first discovery:** publish each .album into the UI as its import finishes instead of waiting for every packed album
- **Artwork priority:** selected cover ahead of large backgrounds, artist logos and off-screen jackets
- **Avoid redundant transfers:** reuse previously staged USB/SMB .album archives and audio despite PS5 EPERM restrictions on stat/lstat/fstat; verify file length and reject symbolic links
- **Non-blocking refresh:** retain the current coverflow and ongoing song playback while the music library scans in the background
- **Safe persistent index:** bounded size, checksum, atomic update, corruption recovery, no credentials or song data inside
- Maintains PSJukebox .album, SMB shares, background/artist themes, Favorites, and v0.1.14 L2/R2 song navigation

First-ever scan may still stage fresh archives; subsequent boots get the cached catalog immediately. Real hardware timing and permission behavior need PS5 confirmation.
