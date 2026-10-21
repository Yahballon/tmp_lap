# Uninstaller v0.1.12 — Turbo Delete

- Preserve the working one-button PS5 game and ShadowMount source removal flow. The exact matching USB game folder is renamed into hidden quarantine on its own volume immediately, then safely cleaned up.
- Keep the complete symlink/special-file preflight; optimize the actual removal phase using descriptor-relative openat, fstatat and unlinkat to avoid repeated full-path resolution on USB drives.
- Stream progress and timings for the real filesystem work: entries checked, entries removed, percentage, elapsed seconds and entries per second. Fix earlier 4 KB progress-response limit.
- Never report disk space as free before actual cleanup completes; raw files are not removed instantaneously.
- Existing AppInstUtil, USB PKG and homebrew behavior remains unchanged, with save data not targeted.
- Actual performance gains must be benchmarked on PS5 firmware 5.10 and will vary with the USB drive and file count.
