# PS5 RandomX Benchmark by YAHBALLON v0.4.2

## JIT constructor crash diagnosis

PS5 console reports confirm the **JIT VM INIT ONLY** profile in v0.4.1 repeatedly terminates the process in the `randomx_create_vm` call: all stages through `after_cache_init` succeed, and the last durable log line is `before_init_only_vm_create`. The earlier 16 KiB W^X memory preflight passes. There is **no evidence JIT-generated code has executed successfully**.

This update adds precise, persistent breadcrumbs **inside upstream RandomX 1.2.1 during the PS5 build**, not only around the API call:
- Enter `randomx_create_vm`; before switch / after constructor; before setCache and scratchpad allocation
- Enter JIT compiler constructor; code-buffer allocation and resulting mapping status
- Actual JIT-code-buffer `mmap` and its errno if it fails
- Before/after copying the JIT prologue and epilogue
- C++ exception on VM creation
- Continues logging to `/data/ps5-randomx/jit_trace.log` with the same session ID and profile ID, flushing each marker

The upstream RandomX code is unchanged semantically: only diagnostic instrumentation is applied to the pinned checkout during native cross-compilation; normal C++ RandomX hashing is unaffected. The standard **AUTO 4 LIGHT** stays the default. **No JIT benchmark is in AUTO**, and a JIT VM creation attempt may still crash.

After installing, run **JIT VM INIT ONLY** (profile index 6) once on a test console, then retrieve the end of `jit_trace.log` to pinpoint the failing substage. Do NOT run **1 THREAD / SECURE JIT** unless JIT VM initialization is proven safe.

Also preserves real PS5 hashes, native SDK packaging and TESTBUILD's GitHub Actions-generated `version.json`. No wallet, P2P, coin or mining.

Title ID PPSA99795; release tag `ps5-randomx-v0.4.2`.
