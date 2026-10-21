# PS5 MicroPython v0.2.2

Filesystem backend redesign based on [ZQuest Classic PS5](https://github.com/Yahballon/tmp_lap/tree/main/homebrew/zquestclassic-ps5/native).

- Replaces sandbox-blocked libc `opendir/readdir` with direct `sceKernelOpen` and `sceKernelGetdents` using FreeBSD dirent decoding and the known-working aligned 64KiB buffer.
- Native `sceKernelRead` loads selected scripts directly from DATA and USB, avoiding libc file API quirks.
- Scans `/data/PS5MicroPython/scripts`, `/mnt/usb0..7/PS5MicroPython/scripts`, and `/mnt/usb0..7/scripts`.
- USB entries display their source; editing a USB script saves an independent copy to DATA.
- Preserves original DEMO scripts and previously saved user files; `stat()` is no longer a prerequisite for `.py` listing.
- Adds host-native mocked libkernel regression tests and updated root/error diagnostics.

**Hardware verification remains pending**; some jailbreak/firmware mount restrictions can still block access. Press Triangle to rescan and inspect `/data/PS5MicroPython/logs/scan.log` if needed.
