# PS5 RandomX Benchmark by YAHBALLON v0.4.4

## Root cause: Native PS5 execute-only text segments

Three v0.4.3 hardware traces ended at the first read of the static RandomX x86-64 JIT assembler prologue, address `0x420640`. The 77,824-byte destination buffer at `0x200110000` had been allocated and written successfully. Native PS5 homebrew splits executable text (X-only) and readable data; attempts to **read the embedded machine-code template from executable text** raise a fault. The original RandomX JIT constructor performs exactly this copy.

### Fix attempt: copy template source to readable data at build time

- Generates **a separate copy** of the exact upstream RandomX x86-64 static assembly templates in a non-executable `.rodata.rxjit` section at build time, under distinct `rxblob_` symbols.
- Preserves the original executable assembler object and its callable functions.
- Redirects the JIT compiler's *source template pointers only* to the readable mirror, leaving the algorithm and the generated instructions unchanged.
- Adds a CI verification that the data copy is a non-executable section with the required symbol.
- Adds **JIT SAFE DIAGNOSTICS** (profile 9): in one test checks the 77,824-byte writable JIT destination independently, then requests read-only mapping for the template source page. It does not read/execute the static template, nor generate JIT code.
- On an RX or readable source page permission failure during JIT VM initialization, record an error rather than immediately copying from an inaccessible page.
- Persistent trace log: `/data/ps5-randomx/jit_trace.log` now reports `source_mprotect_read_rc` / `source_mprotect_read_errno`.

**No JIT on PS5 has yet been proven to work.** Template mirroring solves the documented X-only *source read* issue in principle, but other PS5 JIT compatibility problems may remain. Do not run SECURE JIT hashing until the safe diagnostics and VM-init-only tests succeed.

Default **AUTO 4 LIGHT** interpreter, independent title ID `PPSA99795`, deterministic native build and GitHub Actions-generated TESTBUILD index remain unchanged. No network or mining rewards are enabled.
