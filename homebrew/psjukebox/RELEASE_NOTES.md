# PSJukebox v0.1.8 — Fix archived album import on actual PS5

Root cause based on actual v0.1.7 PS5 log:
`ALBUM_FOUND ... size=109407657` + `ALBUM_TRANSFER_OK` + `ALBUM_ARTIST_ERROR: Invalid packed album`.

The ZIP was discovered and transferred, but PS5's app sandbox denies `lstat` for writable app-managed paths (errno=1). The archive reader was checking `lstat` before opening the local cached archive, causing an incorrect invalid-file error. v0.1.8 opens packed files using an `O_NOFOLLOW` descriptor, uses `fstat` or a descriptor-seek fallback to validate size, and reports real file/ZIP errors. It also updates album-cache directory creation and extracted music library scanning to handle PS5 metadata restrictions.

New host regression test simulates EPERM for `lstat` and `fstat` during ZIP import and library scan. All other features and the isolated JBOX2 helper remain as-is.

Please test on PS5 and send last log lines if importing still fails (e.g. ZIP format limitations or extraction errors). File-size limit remains 1 GiB compressed.
