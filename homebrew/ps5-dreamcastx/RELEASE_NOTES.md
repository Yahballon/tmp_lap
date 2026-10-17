# PS5DreamcastX v0.2.0

## Unified library and SMB connection editor

- Removed Internal, USB and Network header tabs.
- CoverFlow, Grid, List, Search, Favourites and Recently played now include
  discovered Internal, USB and SMB games together.
- Automatic asynchronous SMB scan when opening the combined library.
- Square rescans all sources instead of just the previously selected tab.
- The source of each game appears in its subtitle.
- Edit SMB shares, username, password and domain in Settings > Library using
  the native PS5 keyboard; six fields for shares, remaining shares preserved.
- Saves network.cfg atomically with permissions 0600; password shown only
  as masked characters. Restart app to activate new SMB credentials.
- Unchanged emulator core, CoverFlow image renderer, cover cache, games,
  BIOS, memory card and save paths.

Install: PPSA99765.zip -> /data/homebrew/PS5DreamcastX/.
