# PS5 RandomX Benchmark by YAHBALLON v0.4.1

## JIT crash isolation hotfix

PS5 hardware confirms **JIT W^X permission probes succeeded** (9 successful profile-5 log entries, `jit_probe_errno=0`). A subsequent native crash was reported but the standard benchmark.log did not include an actual JIT hash or failure receipt. **The crash location is not proven**.

Changes:
- New intermediate **JIT VM INIT ONLY** profile, placed between the W^X probe and the full JIT execution profile.
- Separately opens and destroys the RandomX Secure JIT VM without calling `randomx_calculate_hash`.
- Stage-by-stage persistent `/data/ps5-randomx/jit_trace.log`: before/after W^X check, RandomX cache allocation/initialization, JIT VM construction/destruction, interpreter comparison and first JIT hash.
- Each stage append is flushed and closed before proceeding, helping diagnose crash without a normal benchmark report.
- Host CI checks a real RandomX secure-JIT digest against the interpreter, in addition to compiling the PS5 executable.
- Fixed version strings for both regular benchmark receipts and automatic test summaries to `v0.4.1`.
- The default AUTO 4/5 modes are still **interpreter-only**. No networking or crypto mining.

**PS5 test order:** First run W^X, next run **JIT VM INIT ONLY** (without any generated hash). Only after these both pass, consider the experimental full JIT profile; it can still crash. If a crash occurs, provide the new `jit_trace.log` to pinpoint the last stage.

Title ID: `PPSA99795`. Installer: `PPSA99795.zip`. Release: `ps5-randomx-v0.4.1`.
