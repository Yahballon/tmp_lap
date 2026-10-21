# Uninstaller v0.1.10 — Console Storage box removed

- Completely removed the Console Storage panel, disk usage ring, fake volume capacities and L2/R2 storage selector from the UI.
- Replaced three columns with a two-column layout: game/homebrew list on the left and a spacious detail panel with large title artwork, metadata, and uninstall control on the right.
- Preserved game uninstall, USB homebrew deletion, USB PKG deletion, progress indicator, logos, and logging. No changes to the privileged helpers or the title removal API.

## ShadowMountPlus 1.7 source removal

- Fix persistent game duplicates that reappear after native uninstall when ShadowMountPlus still discovers the original backing source.
- For the specific user-tested title **PPSA21564**, pressing **X** opens a separate physical-source removal confirmation, including the path returned by the official ShadowMountPlus game info API.
- Confirmation submits `POST /api/v1/games/delete` with the title ID and `confirm: true`, only after verifying the source mapping, version capability and unchanged source path. The ShadowMount background worker performs protected deletion; UI polls job status and shows progress. No direct recursive deletion is performed by Uninstaller.
- **Square** continues to invoke the existing PS5 native title uninstall. If the library tile remains after the source has been deleted, Square can remove the remaining registration.
- Requires ShadowMountPlus **1.7** with its localhost API enabled (typically port 10101). On older versions, only an error message appears; source files are not touched.
- Deleting a game source is permanent and can remove many GB. Choose the exact item and confirm the displayed path before proceeding.
