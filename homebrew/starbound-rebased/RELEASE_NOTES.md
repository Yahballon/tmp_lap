# Starbound Rebased v0.1.7 – PS5 pre-log crash regression fix

Hardware feedback: v0.1.5 rendered the first frames successfully, but v0.1.6 crashed instantly without even creating a boot log. The v0.1.6-only native `getcwd()` probe was executed before the first boot trace; this is a strong regression candidate because that libc entry point is not known to work with the current PS5 ShadowMount/CRT combination.

This fix removes that probe, writes `BOOT00 entered main before resource setup` as the first action in `main()`, and keeps the original `01 main entered` and low-risk `ASSET01`–`ASSET04` probes afterwards. CI checks the trace order and prevents reintroducing `getcwd()`. The corrected `assets/res/**` resource layout from v0.1.6 remains.

This is an experimental runtime fix. We have **not** verified on real PS5 hardware that it removes the crash or restores the game menu.
