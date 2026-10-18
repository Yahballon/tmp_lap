# PS5NESX TESTBUILDS port
Source snapshot from https://github.com/Yahballon/PS5NESX.
The native app is in `nes/` and uses `ps5/` and Vulkan headers in `3rdparty/`.
The unrelated PCSX2 desktop scaffold is excluded deliberately.
Edit this folder in tmp_lap, update `app.json` to the next SemVer
(e.g. 1.0.10), and run **Build PS5NESX Native**.
The resulting GitHub Release is named `ps5nesx-v<version>`
with the installable `PPSA99204.zip` and is discovered by `version.json`.
`1.0.9` is an unchanged rehost of the upstream `v1.0.0.9` binary.
Never bundle commercial NES ROMs.
