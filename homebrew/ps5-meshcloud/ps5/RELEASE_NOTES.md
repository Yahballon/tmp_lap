# MeshCloud PS5 v0.1.0 pre-alpha

Native OpenGL homebrew launcher for PS5 by YAHBALLON.

- Controller-ready five-page UI, original icon, local filesystem and file-list diagnostics.
- Initial 10 GiB + 2 GiB disk free-space check; clearly differentiated from actual remote proof of storage.
- No network access, download, upload, CID search or claim of remote connectivity.
- Reuses PS5-DevBridge native application scaffolding and display/controller framework.

**Important:** This is a read-only prototype; it does not yet include the desktop encryption, SQLite vault, Reed-Solomon or authenticated P2P network. GitHub Actions compilation must be followed by testing on an actual PS5.

Install by extracting PPSA99718.zip into /data/homebrew and registering with your compatible homebrew loader.
