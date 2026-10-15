# PSTUBE v0.5.0 — Native Streaming & Infinite Search Scroll

- Play progressive signed MP4 URLs straight through native FFmpeg HTTPS without waiting for the entire file to finish downloading.
- Keep SSL certificate and hostname verification enabled using trusted certificate roots from the installed PS5 package. Configure HTTP timeouts, buffering and retries.
- Downloading to the Downloads library is now a separate action (**Square** while playing).
- Add background infinite search pagination: YouTube continuation tokens and Invidious page numbers; deduplicate results, keep the selected item, and indicate when further results are loading.
- Preserve video thumbnails, history, favourites and existing complete downloads.

Known limitations: Direct YouTube playback may require login, JS signature solving, PO tokens or separate adaptive tracks; no claim is made that every YouTube video works. The FFmpeg network backend and audio/video sync require real PS5 testing. Online search pagination depends on external YouTube API structures, which may change. Existing saved media is preserved.
