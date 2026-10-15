# PSTUBE v0.2.1 — Video Catalog and Thumbnail Fix

- Automatically retry video search and Trending on multiple trusted community Invidious instances.
- Surface the current provider and useful error messages rather than silently showing an empty page.
- Save the last successful **real** video feed locally and restore it on startup if services fail.
- Download JPEG thumbnails with three concurrent background workers; retry missing images for cached videos.
- Keep favourites, downloads and video history from prior versions.
- Test search and Trending JSON parsing in CI using representative response fixtures.

## Important limitations
Video availability still depends on external public Invidious services. No service can guarantee search uptime, downloads or a stable media URL. If an HTTPS request fails because the console clock is invalid, correct PS5 date/time; TLS verification stays enabled. Hardware testing of streaming, video and audio remains necessary.

Install: download PPSA99762.zip, replace /data/homebrew/PPSA99762 and re-register with your homebrew loader. Existing saved content under /data/pstube is preserved.
