# PSJukebox v0.1.14 — R2/L2 Song Skip with Album Wrap

- **R2** skips forward one song; from an album's last track, plays track 1 of the next album
- **L2** skips backward one song; from track 1, plays the final track of the preceding album
- Skips empty albums and wraps between last and first albums **within the same artist**
- During playback the carousel and Tracklist follow manual R2/L2 changes, including local, packed album, USB and SMB tracks
- Natural song-end auto-advance uses the same album traversal logic
- L1/R1 remain artist navigation, D-pad left/right album covers, analog stick left/right seeks 10 seconds
- Clock with creator/version, Favorites, settings and all other features remain
- Host tests cover album boundaries, skipping empty albums, cyclic edges, artist isolation and single-track albums

Native PS5 build and package checks run in CI; real-console testing is still required.
