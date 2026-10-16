# ThemeChanger v0.3.4 – Reliable Background Switching

- Fixed root cause of 'The home screen did not release the old video': LiveShell game pause treated ThemeChanger PPSA99891 as a game. Exempts this title in the GPL-source-built LiveShell engine while keeping pause enabled for real games.
- Removed the upstream LiveShell Media tile installation call: ThemeChanger already autostarts the engine. A previously installed LVSH00001 tile may be deleted once through PS5 UI.
- Compile LiveShell directly from ZiZc3 source within the OnionHEN payload toolchain; include patched sources/build script and preserve GPL attribution.
- Keep MotionBGS MP4 downloads, JPEG previews, installed gallery and detailed ShellUI monitoring.
- A full console reboot + jailbreak is required to unload the resident older LiveShell payload.
- Real console playback testing is still required; GitHub CI confirms source patch and native compilation only.
