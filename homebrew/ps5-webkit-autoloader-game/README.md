# PS5 WebKit Autoloader v0.4.0 — Game category test build

This experiment is based on [itsPLK/ps5-webkit-autoloader](https://github.com/itsPLK/ps5-webkit-autoloader/tree/v0.4.0), specifically upstream commit `137f065d2ba8022ba8e796895e7cb05c204f2ce9`.

**Goal:** Display the offline WebKit Autoloader shortcut under **Games** instead of **Media**, without changing the underlying exploit, cache, or payload loading logic.

- Game application category: `applicationCategoryType: 0`
- Game content badge: `contentBadgeType: 1`
- Dedicated test title ID: `WKAG00001` (the original `WKAL00001` is untouched)
- Original local browser deeplink remains: `http://127.0.0.1:18181/app/index.html`
- Original upstream version: `0.4.0`
- Original upstream author and GPL-3.0 license preserved; all source is fetched from the pinned original repository during CI.

## Build and download

Run the GitHub Actions workflow **Build WebKit Autoloader GAME v0.4.0** from the Actions tab. Download the `PS5-WebKit-Autoloader-GAME-v0.4.0` artifact containing the ELF installer and standalone Python PC host.

The workflow checks out upstream with recursive submodules, patches the launcher configuration, builds using the upstream PS5 SDK Docker image, and checks the compiled ELF for the new title ID.

## Install / PS5 test

1. Keep your working Media autoloader installed; this uses a different title ID.
2. On an already-jailbroken PS5, run `webkit-autoloader-GAME-installer_v0.4.0.elf` using an ELF loader / Payload Manager. For a fresh setup, use the built Python host and the upstream instructions.
3. Allow the installer to finish caching, then check the **Games** area for **WebKit Autoloader GAME**.
4. Restart the console and test the Game shortcut before removing your original Media shortcut.

**Experimental limitation:** A Game-category deep link might be treated differently by Sony's home screen app launcher. Successful compilation or correct placement does not establish that the Game shortcut launches WebKit after a clean reboot. Test on hardware and provide installer logs if it fails.

**Safety:** Do not delete `WKAL00001` or clear the existing WebKit browser cache just to test the category. The two shortcuts may share the browser cache, even though their title IDs differ.



## FIX: JAILBREAK ME custom icon + Game tile selection music

The **only supported workflow** is [Build WebKit Autoloader GAME - Custom Icon and Music](../../.github/workflows/build-webkit-autoloader-game-music.yml). The old GAME-only workflow was removed because its successful artifacts had the original icon and no audio.

### Required files

- \`homebrew/ps5-webkit-autoloader-game/icon.png\` — the user-provided **JAILBREAK ME** image, exactly 512 x 512 PNG.
- \`homebrew/ps5-webkit-autoloader-game/soundtrack.mp3\` — user-provided source music (already uploaded; only publish if licensed).

**Important:** The CI workflow deliberately fails when icon.png is absent so it cannot silently build an installer with the old logo. Upload the file with the exact name above before starting the workflow. An upload of icon.png automatically starts the music+icon workflow.

The workflow generates a 60-second, 192 kb/s, looped ATRAC9 \`snd0.at9\`, patching \`pubtools.loudnessSnd0\`, and builds a PS5 installer that embeds both the custom PNG and soundtrack. The upstream \`tools/gen_icons.py\` is patched to respect the custom icon **after** its normal icon generation, otherwise it would regenerate the old default PNG.

### Fixes for Game tile presentation

The installer uses the test ID \`WKAG00001\`, **not** the Media launcher ID \`WKAL00001\`. It writes the icon and song to all needed locations:

\`\`\`text
/user/app/WKAG00001/icon0.png
/user/app/WKAG00001/sce_sys/icon0.png
/user/app/WKAG00001/sce_sys/snd0.at9
/user/app/WKAG00001/sce_sys/param.json
/user/appmeta/WKAG00001/icon0.png
/user/appmeta/WKAG00001/snd0.at9
/user/appmeta/WKAG00001/param.json
\`\`\`

It then updates only the \`WKAG00001\` row in the PS5 app database to point \`tbl_contentinfo.snd0info\` to \`/user/appmeta/WKAG00001/snd0.at9\`. It accepts the existing-install restoration status and logs explicit asset staging and DB registration results. It **does not** uninstall the Media app or modify other DB rows.

### Build and test

Download **PS5-WebKit-Autoloader-GAME-ICON-MUSIC-v0.4.0** from the successful GitHub Actions run (not an older artifact). Launch \`webkit-autoloader-GAME-ICON-MUSIC-installer_v0.4.0.elf\` via your existing PS5 ELF loader, allow caching to complete, then **fully power off and restart the PS5** to refresh the Home Screen icon/music cache. Confirm **Home Screen Music** is on in PS5 audio settings.

Look for the installer logs:
\`ICON_REGISTERED\`, \`MUSIC_FILE_REGISTERED\`, \`MUSIC_DB_REGISTER title=WKAG00001 rows=1\`, \`GAME_INSTALL_DONE\`.
If the database update is blocked or \`rows=0\`, upload the logs instead of deleting system databases. Do not edit app.db manually or delete the working Media launcher.

The selected-tile audio for a Game-category browser deeplink remains a **hardware test**, even after correct file/DB registration. This is not a native \`eboot.bin\` game. If it is still silent after the fixes, we need PS5-side logs.
