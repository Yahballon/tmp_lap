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
