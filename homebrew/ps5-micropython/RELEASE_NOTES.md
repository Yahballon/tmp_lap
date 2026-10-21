# PS5 MicroPython v0.2.3

Native editor SAVE bug fix based on a real PS5 capture showing `Unsafe existing file` when saving an already-readable `helloworld.py`.

- The native PS5 writer now uses `sceKernelOpen`, `sceKernelWrite`, `sceKernelFsync`, `sceKernelClose`, and `sceKernelRename`, matching the libkernel backend used to read scripts.
- Removes libc `lstat` from the native writer, which could incorrectly reject a regular, existing script.
- Atomic write via exclusive temporary file and rename; if opening, writing or renaming fails the old file remains and the unsaved buffer stays in the editor.
- Reports SAVE OPEN, SAVE WRITE or SAVE RENAME with the kernel error code instead of the vague `Unsafe existing file` warning.
- Native directory-based collision checks for new script names, including symlinks and unreadable entries.
- Mocked kernel I/O tests exercise new saves, overwrite, failed open, failed write, failed rename and safe cleanup.
- The interpreter, controller editor and DATA/USB script loader are unchanged.

CI success confirms a build and simulated file I/O; **a real PS5 saving test is still required**.
