# PSJukebox v0.1.16 — New Neon App Icon

- Replace the old generated PSJukebox app icon with the user-supplied neon retro-futuristic jukebox design (pink/cyan jukebox, vinyl and PSJukebox lettering)
- Apply the updated image to both `sce_sys/icon0.png` (native PS5 Home menu) and `assets/icon0.png` (app asset)
- Package 512 × 512 PNG files from the checked-in optimized artwork
- Release verification now checks actual image content and requires both copies to be identical to the new artwork
- README preview displays the new icon
- Preserve Title ID `PPSA99947`, settings, library index, SMB/.album support, coverflow, favorites and controller mapping

Native CI verifies image content and package structure; the PS5 Home menu should display the new icon after installing the new package. The real-console result still needs testing.
