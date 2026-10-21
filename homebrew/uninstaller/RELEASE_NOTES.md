# Uninstaller v0.1.9 — Game uninstall fix and diagnostics

v0.1.8 USB homebrew and USB PKG deletion are confirmed on firmware 5.10, but installed games do not complete. The log shows REMOVE_HELPER ready followed by "No authenticated removal result" for PPSA21564, PPSA21567 and PPSA03639.

- Replace unverified dynamic installer resolution and wrong 3-argument signature with directly linked one-argument sceAppInstUtilAppUnInstall(titleId), matching Playden's PS5 payload.
- Initialize AppInstUtil before adopting ShadowMountPlus installer authid.
- Stream UREM2_STAGE diagnostics to foreground UI, which now reads all lines through UREM2_RESULT. Show the final reached stage if the native helper terminates.
- Keep existing USB deletion and homebrew code paths, user confirmation and safeguards unchanged.
- Never recursively delete /user/app titles manually. Do not delete saved games.
- Game removal remains to be confirmed on actual PS5 hardware.
