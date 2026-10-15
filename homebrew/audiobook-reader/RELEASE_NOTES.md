# Audiobook Reader v0.2.1

Fix USB audiobooks in dedicated ABooks folders. Probe /mnt/usb0/ABooks, /mnt/usb1/ABooks and compatible fallbacks. The read-only elfldr helper now reports exact mount diagnostics instead of a generic 'not accessible' message. New logs include USB_HELPER_START, USB_ROOT, USB_SCAN_DONE/ERROR and SCAN_LOCAL, stored at /data/AudioBookReader/audiobook-reader.log.

Install: update PPSA99784.zip under /data/homebrew, register with the compatible homebrew loader, and keep elfldr running on port 9021. Put DRM-free .m4b files on USB in ABooks or locally in /data/AudioBookReader/Books. Existing listening progress, favorites and audio cache are preserved.

Known limitations: USB mounts and elfldr must be accessible from the jailbreak runtime. USB audiobooks are copied to /data/AudioBookReader/Cache before playback. No DRM support. Hardware playback is not verified by CI.
