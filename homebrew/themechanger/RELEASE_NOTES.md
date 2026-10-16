# ThemeChanger v0.3.19 - Use the original Signal Companion service binary

- Bundle the unmodified ps5-signal.elf shipped by Marice's Signal Companion v00.001.000, rather than rebuilding it with ThemeChanger's older payload SDK.
- Original ELF: 147856 bytes, SHA-256 e39199ab216986442dd4ae66df7fe7ec012c50984a27fc8aff9b42834359e9d8. Checked both during download and inside the finished installer.
- The identity was verified against the original Companion ZIP (SHA-256 8404cc3cf05c9e9e12ecdd961f5257f65a730c0c3136663a89ba9f6112548840).
- Retain detailed Signal request/readback logs under /data/ThemeChanger/signal.log.

After installing, open Signal Companion > Status and choose Restart ps5-signal once to replace the old background process. Installing the app alone does not replace a running service.

The supplied console log proves commands arrive and save, but reported channels remain 0/255/0. It does not prove the SDK difference is the cause. This update removes that binary difference; hardware validation remains required. Colour availability and brightness availability are separate capabilities.
