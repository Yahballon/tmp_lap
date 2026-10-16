# ThemeChanger v0.3.17 - Signal connection and navigation fixes

- Start the bundled Signal service automatically when opening Signal Companion; retry remains available on Status.
- Hide optional ps5-autoloader controls when its list is unavailable. It is not required for Colour or Startup.
- Read packaged files through the native stdio path; use explicit ELF-loader EOF handoff, native-compatible HTTP parsing and a 2 MB worker stack.
- Add SIGNAL_WORKER, SIGNAL_CONNECT, SIGNAL_PAYLOAD, SIGNAL_START and SIGNAL_HTTP diagnostics to themechanger.log.
- L1/R1 switches Signal tabs while inside Signal, and cycles application pages elsewhere. Circle opens/closes sidebar instead of exiting the app. Right and Cross both open the highlighted sidebar item.
- Colour sliders preview automatically; Startup exposes visible Add, Preview, Save and Clear controls.
- Remove the synchronous LiveShell status request from every wallpaper frame.
- Remove the blue G logo from the top-left header and align title/creator text to the left.

Host tests cover the real loopback HTTP transport and ELF-loader handoff in addition to configuration saves and editor limits. On-console operation still needs hardware verification; failures now include Signal-specific diagnostics.
