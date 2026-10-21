# PS5 MicroPython

**Native PS5 Homebrew · by YAHBALLON · v0.2.2 · PPSA99791**

![Original launcher icon](assets/icon.svg)

A real, embedded [MicroPython](https://github.com/micropython/micropython) interpreter for jailbroken PS5 consoles, with a controller-operated Python script editor, script library and on-screen console output.

**Testing:** A user-provided PS5 photo confirmed the v0.1.0 Fibonacci demo. The v0.2.2 editor requires a new console test. CI compilation and ZIP verification are not equivalent to a hardware test.

## Features

- Run the bundled Hello, Fibonacci and Collections scripts on the native MicroPython VM.
- Create, edit and save your own Python scripts **directly on PS5** using an on-screen keyboard.
- Keyboard includes lowercase, uppercase and symbol pages; insert newlines, four-space indentation and reposition the text caret.
- Built-in examples open as editable **copies**; installed read-only demo assets are never overwritten.
- Atomic replacement when saving existing scripts: write a temporary file before rename; confirm before discarding unsaved edits.
- Script-file limit of 64 KiB; persistent user storage, interpreter error output and runtime log.
- Native PS5 `sceKernelOpen` + aligned `sceKernelGetdents` directory scanning, adapted from the ZQuest Classic PS5 source. The previous libc `opendir()` was not reliable within PS5 BigApp sandboxes.
- PS5 `sceKernelRead` source loading for user scripts; no POSIX `opendir()` or `stat()` required to list `.py` names.
- Internal DATA folder and USB0–USB7 roots with two supported layouts per USB device.
- Live scanner status on the console-output screen and detailed diagnostics in `/data/PS5MicroPython/logs/scan.log`.
- Script location is shown within the script library UI for FTP/file-manager transfer.

## Controls

| Context | Button | Action |
| --- | --- | --- |
| Library | D-pad / Left stick | Select file |
| Library | Cross | Run selected script |
| Library | Square | Edit selected script (demo = new copy) |
| Library | Options | New Python script |
| Library | Triangle | Rescan folder |
| Library | Touchpad | Clear console output |
| Library | L1 / R1 | Scroll console log |
| Library | Circle | Exit app |
| Editor | D-pad / Left stick | Choose on-screen keyboard character |
| Editor | Cross | Insert character |
| Editor | Square | Delete character before caret |
| Editor | Triangle | Newline |
| Editor | L2 | Insert four spaces |
| Editor | R2 | Cycle lowercase / uppercase / symbols |
| Editor | L1 / R1 | Move caret left / right |
| Editor | L3 / R3 | Move caret up / down a code line |
| Editor | Options | Save and return to script library |
| Editor | Circle | Back; confirm if unsaved changes would be lost |

## Saving scripts

Create a script by pressing **Options** in the library. New files get available automatic names, such as `script_001.py`, `script_002.py`, etc. Press **Options** from the editor to save. To edit an existing saved script, highlight it and press **Square**.

Scripts are stored under:

```text
/data/PS5MicroPython/scripts/
```

You can also put scripts on USB at `/mnt/usb0/PS5MicroPython/scripts/` or `/mnt/usb0/scripts/`, and similarly for USB1–USB7. USB scripts are listed with a `USB0`, `USB1`, etc. prefix. Running them reads directly from USB; editing a USB script creates a **copy** under `/data/PS5MicroPython/scripts/` rather than overwriting the USB file.

Scripts may also be transferred by FTP or a compatible file manager. Press **Triangle** in the library to reload the list. The data folder is created on launch; no saved user scripts are deleted by this update.

Runtime logs: `/data/PS5MicroPython/logs/runtime.log`.

Scanner diagnostics: `/data/PS5MicroPython/logs/scan.log`. The UI shows `SCAN N files; roots X/17` with a DATA error code if its own internal folder cannot be opened. The log reports separate `open_error` and `read_error` codes for every root. Matching `.py` filenames are listed even if `stat()` fails; files larger than 64 KiB or empty/binary files are rejected when opening/running. If files cannot be loaded, press Triangle and send the `scan.log` and on-screen status. USB mounts depend on the active jailbreak/runtime.

## Install

Download a validated `PPSA99791.zip` from [GitHub Releases](https://github.com/Yahballon/tmp_lap/releases), not a GitHub source archive. Extract the title folder into `/data/homebrew/` and register it using your compatible homebrew loader. Preserve your `/data/PS5MicroPython/` directory across updates.

## Build

See [Build PS5 MicroPython](../../.github/workflows/build-ps5-micropython.yml). The workflow pins MicroPython v1.26.1, generates its C embedding port, stages the PS5-DevBridge OpenGL runtime, compiles the native app, runs host editor tests and checks the final PS5 ZIP before making a release.

## Known limitations

This is an embedded, minimal MicroPython configuration rather than CPython. No pip, Unix extensions, native package management or live REPL. The controller keyboard is simple, not Sony's system keyboard, and new scripts use numbered filenames. Running scripts blocks the renderer until execution completes; infinite loops can hang the title. Only run trusted code.

**SCREENSHOT: v0.2.2 PS5 hardware capture pending.** A previous user-provided v0.1.0 PS5 image demonstrates Fibonacci, not this editor revision.

## License / credits

App/UI by **YAHBALLON**, GPL-3.0-or-later. MicroPython © Damien P. George and contributors, MIT. See [Third-party notices](THIRD_PARTY_NOTICES.md). Artwork is original and not AI generated.
