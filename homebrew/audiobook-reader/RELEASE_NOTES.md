# Audiobook Reader v0.2.0

New dedicated USB audiobook library at `/mnt/usb0/ABooks`, using an independent, read-only `abookscan.elf` launched by an existing elfldr on port 9021 (private helper service port 9177).

- Recursive `.m4b` discovery on USB with stable entries in the aluminum bookshelf.
- Separate local book folder: `/data/AudioBookReader/Books`.
- Per-app diagnostics: `/data/AudioBookReader/audiobook-reader.log`.
- Listening progress and favorites: `/data/AudioBookReader/progress.tsv`.
- Temporary USB copy cache: `/data/AudioBookReader/Cache`, with transfer progress, cancellation and resume-on-disconnect recovery.
- ABooks helper packaged directly into `PPSA99784.zip`.
- Original UI, controller input, chapter navigation, native M4B playback and existing saved settings retained.

**Installation:** Extract `PPSA99784.zip` to `/data/homebrew` and register through a compatible loader; copy your own DRM-free `.m4b` files to the USB or local ABooks/Books directory.

**Known limitations:** USB requires an active elfldr on TCP 9021. M4B is copied locally before playback; adequate free disk space is required. Only non-DRM codecs available in the native FFmpeg build are supported. Actual hardware verification is still required.
