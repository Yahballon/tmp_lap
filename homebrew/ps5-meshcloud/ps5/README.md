# MeshCloud PS5 Native v0.1.0

**By YAHBALLON** — native controller-first OpenGL console prototype.

Title ID: PPSA99718
Content ID: UP9000-PPSA99718_00-MESHCLOUDPS5APP1
Build workflow: .github/workflows/build-ps5-meshcloud.yml

## Implemented

The native PS5 app has its own 1920×1080 OpenGL interface using the PS5-DevBridge homebrew UI. It displays five controller-navigable pages: Overview, Storage, Local files, Network and About.

Storage checks use POSIX statvfs on /data, /mnt/usb0 and /mnt/usb1 when available. They display **filesystem-reported** free bytes and whether at least **10 GiB + 2 GiB safety margin** appears free. This is not actual storage reservation, proof of storage, validation of a contribution, or permission to use the P2P network.

The Local files page lists regular files from <selected mount>/PS5MeshCloud/files without following symbolic links. It does not scan or open private keys, privileged console files or copyrighted content outside those paths. It does **not** upload, download, encrypt or register those files.

A best-effort log is written to /data/PS5MeshCloud/meshcloud-ps5.log when writable. No network listener is created; it is a local-only UI for now.

## Controller

- D-pad left/right or L1/R1: move among pages.
- D-pad up/down: move among pages; scroll local files when on Local files.
- Triangle: refresh local mount/file observations.
- Square: switch selected storage mount (/data, /mnt/usb0, /mnt/usb1).
- Cross: select or inspect; never begins a network operation.
- Options: About.
- Circle: go to Overview, then close the app.

## Build and installation

GitHub Actions checks host-side filesystem diagnostics, compiles the application using PS5-DevBridge, generates an original blue MeshCloud app icon, verifies installer metadata/PS5 executable signature and builds PPSA99718.zip.

When GitHub Actions has **actually passed**, extract PPSA99718/ into /data/homebrew/ and register it with a supported PS5 homebrew loader such as ShadowMountPlus. Compilation is not a substitute for hardware tests; the application should still be tested on a jailbroken PS5.

## Strict limitations

**No remote MeshCloud access is functional on this PS5 prototype.** The desktop C++ engine (AES-256-GCM local vault, SQLite, signed publisher manifests, 90-day expiry, simulated CID exchange, Reed-Solomon) is NOT linked into this native executable. The PS5 target lacks a verified port of those dependencies; there are no real auditor servers or authenticated relay transport yet.

In particular:
- No access to searches, CIDs, metadata, uploads or downloads for unverified members.
- The 10-GiB rule applies to ALL eventual members and platforms; free-space results alone do not pass the proof.
- A signed 90-day file expiry is planned at network level. The PS5 prototype cannot yet enforce it against remote peers.
- Network IP privacy is not implemented. A real relay would still see its client IP.
- The custom app icon has no Sony or PlayStation trademark graphics.

Keep sensitive files out of the files folders until production encryption and policy handling exist.
