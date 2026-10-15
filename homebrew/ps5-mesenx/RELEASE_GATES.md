# PS5MesenX v0.1.0 native release gate

- [x] MesenCE source, PS5SNESX frontend and test fixtures imported
- [x] Native MesenCE Linux smoke test passes (ROM, frame, state)
- [x] HD-capable MesenCE-Libretro session integrated for future native title
- [ ] PS5 x86_64-sie-ps5 cross-compile of MesenCE core succeeds
- [ ] PS5 Vulkan frontend links to MesenCE and generates a real eboot.elf
- [ ] PS5 native SELF signer produces a verified eboot.bin
- [ ] sce_sys, libc.prx, GPL notices and required helper packaged
- [ ] Verified PPSA99896.zip published as ps5-mesenx-v0.1.0
- [ ] TESTBUILDS release workflow subscription updates version.json
- [ ] Actual PS5 game playback/HD pack test

Never publish a fake or non-PS5 executable. The cross-core archive is
not a launchable app and must not be put into TESTBUILDS.
