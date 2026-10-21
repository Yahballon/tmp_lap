# Uninstaller v0.1.5 – real Console Storage and logos

- Console Storage panel now shows available/used/total capacity and a blue usage ring when reliable values are measured in the privileged, read-only ELF helper; L2/R2 switches connected volumes.
- PS5 games and Homebrew now load their actual logo PNGs from sce_sys/icon0.png or system appmeta locations using on-demand localhost port 9175, with memory cache and renderer thread decoding.
- Keeps gameplay smooth by loading only currently visible logos on background worker. Missing logos show fallback icons.
- Distinguishes inaccessible drive capacity from a measured value.
- Existing registered titles and privileged inventory results stay **read-only**; this update does not uninstall installed games. Save data remains untouched.
