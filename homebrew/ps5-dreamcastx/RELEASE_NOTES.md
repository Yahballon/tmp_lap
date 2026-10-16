# PS5DreamcastX v0.1.8

- New persistent 640px predecoded RGBA thumbnail cache on internal SSD (`/data/PS5DreamcastX/cache/covers/`).
- Keeps the original downloaded JPG/PNG artwork in `/data/PS5DreamcastX/covers/`.
- Reloads thumbnails on subsequent launches without re-decoding, and invalidates stale entries when source files change.
- Prioritizes local artwork over TheGamesDB requests while browsing.
- Backs off failing online metadata requests for 15 minutes instead of 10 seconds (HTTP 503).
- Adds cold/warm/invalidation regression tests. Case rendering and saves are unchanged.

Native PS5 installation package: `PPSA99765.zip`, Title ID `PPSA99765`.
