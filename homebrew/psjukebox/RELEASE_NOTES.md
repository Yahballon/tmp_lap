# PSJukebox v0.1.20 — SMB Share Settings

- Configure or update SMB server/IP, share, optional music subfolder, username, password and optional domain from Settings > SMB Network Share.
- Built-in DualSense on-screen keyboard with lowercase, uppercase and symbols, delete, space, confirm and cancel.
- Password entries are masked in the interface. SMB credentials remain on the console in /data/PSJukebox/smb.ini (local file mode 0600) and are never included in release ZIPs.
- Save & Rescan persists settings and refreshes the library without restarting the player.
- Cache SMB media separately per server/share to prevent cross-share reuse.
- Preserve all v0.1.19 compact artist rail, coverflow and PSJUKE BOX styling.

Compiled with GitHub Actions; final functionality must be confirmed on PS5 hardware.
