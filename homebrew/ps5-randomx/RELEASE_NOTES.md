# PS5 RandomX Benchmark by YAHBALLON v0.4.0

## First experimental JIT compatibility tests
The v0.3.0 PS5 AUTO run measured **2.04 / 2.04 / 4.07 / 8.09 H/s** (1T software, 1T hardware, 2T hardware, 4T hardware; light interpreter). The 4-thread result is not a JIT score.

This release adds **two OPTIONAL manual profiles** after the five existing profiles:

1. **JIT MEMORY PERMISSIONS** (safe first test). Attempts a 16 KiB RW `mmap`, transitions the same page to RX with `mprotect`, transitions back to RW, then unmaps. **Never executes the mapped page or grants RWX permissions.** Prints `jit_probe_*` diagnostic fields and errno/stage if denied.
2. **1 THREAD / SECURE JIT** (experimental execution). Runs the same W^X preflight, checks RandomX JIT and hardware-AES flags, then creates an **opt-in RandomX v1.2.1 JIT Light VM with `RANDOMX_FLAG_SECURE`**. Before measuring H/s, compares one generated JIT hash against the reference interpreter hash. Fails the test on mismatch instead of silently using the interpreter.

**Important:** This is a PS5 homebrew experiment. The W^X mapping test is necessary but *not proof* that JIT code execution works. PS5 sandbox/HEN security could deny permissions or crash the native process. No circumvention of kernel permissions is included. Only attempt JIT execution after the W^X probe reports success.

The default **AUTO 4 LIGHT** and optional **AUTO ALL 5** sequences remain **JIT-free** and unchanged. JIT tests require deliberate menu navigation.

Logging: `/data/ps5-randomx/benchmark.log`; the first stage records mapping/protection results without executing JIT code.

No network, payout, blockchain or mining is performed.
