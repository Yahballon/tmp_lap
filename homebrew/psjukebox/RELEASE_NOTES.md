# PSJukebox v0.1.7 — Music-root .album recognition fix

- Fix helper-version mismatch across PSJukebox upgrades: unique port 9189 + JBOX2 version handshake ensures the app cannot accidentally reuse the v0.1.5-era helper that ignores Music-root albums.
- Log discovery, archive transfer, extraction, success and failure immediately, so errors are observable even during large scans.
- If `Music/name.album` lacks an `ArtistName.artist` marker, import it under **Unsorted Albums** rather than silently dropping it. A valid internal `.artist` still sorts it into the correct artist.
- Reject malformed, encrypted and oversized archives with descriptive log entries; continue scanning other music.
- Preserve normal artist folders, Favorites, SMB sources and DualSense coverflow / tracklist controls.
- Regression tests validate new USB catalog handshake and root archive discovery.

Hardware validation is still required; especially confirm updated elfldr launches the new read-only helper on port 9189.