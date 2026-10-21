# Uninstaller v0.1.4 – native PS5 sandbox bridge

- Compare against Yahballon/PS5_Payload_LauncherX USB scanner and reuse its proven architecture: a separate read-only scanner ELF submitted to an already running PS5 elfldr on localhost TCP 9021.
- Find homebrew, visible game folders, external USB PKG archives through a bounded metadata-only helper, merge real results into tabs.
- Show missing elfldr / access errors in /data/Uninstaller/uninstaller.log. Correct log prefix to v0.1.4.
- Privileged/remote inventory records are **READ ONLY**. Installed game and PS5 PKG uninstall remains disabled. Save data is untouched.
- Built/tested in GitHub CI; console execution still requires hardware validation.
