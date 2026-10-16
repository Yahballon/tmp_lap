# ThemeChanger v0.2.0 – LiveShell integrated

- Native ThemeChanger UI corrected to the screenshot's 1920×1080 canvas and card grid. All 12 entries now fit above the footer.
- Includes the **official ZiZc3 LiveShell Engine v1.0 ELF**, SHA256 verified from GitHub Release.
- Automatically sends the bundled engine to the jailbreak's elfldr listener on `127.0.0.1:9021` if not already running.
- Still uses LiveShell's own home-screen video injection, game pause, and localhost API port 9050.
- Wallpaper themes selected while the engine is starting are queued and applied after LiveShell becomes ready.
- Press **Square** to retry engine start; no separate LiveShell download or manual ELF injection required on a correctly configured jailbreak.
- No Windows helper software needed. Requires **existing** jailbreak and `elfldr` localhost 9021.
- LiveShell v1.0 is distributed under GPL-3.0; native UI under GPL-3.0-or-later.
- ShellUI hook compatibility, particularly PS5 firmware, still needs console testing. See `/data/LiveShell/liveshell.log`.
