# PS5 RandomX Benchmark by YAHBALLON v0.4.4

## PS5 JIT crash: static assembler source, not code destination

In three independent native PS5 v0.4.3 sessions the JIT buffer at `0x0000000200110000` was successfully mapped and written; the source address was `0x0000000000420640`. The last breadcrumb was `jit_before_source_read` each time. The first attempted read of the static x86 RandomX assembler prologue crashes before JIT code is executed.

### Safer diagnostic
- **JIT SAFE DIAGNOSTICS**, new optional manual mode (profile 9): tests a 77,824-byte RW mapping (first/last page plus 256-byte stack pattern copy), then asks the kernel whether the static assembler source page can be mapped RX with `mprotect`. **It never dereferences that source or executes generated code.**
- Logs `static_source_addr`, `source_page_aligned`, `source_mprotect_rx_rc` and `source_mprotect_rx_errno` to `/data/ps5-randomx/jit_trace.log`.
- The existing **JIT VM INIT ONLY** constructor now performs the same preflight and **throws a normal diagnostic exception if the kernel denies RX**, before trying to read assembly bytes. A passing RX permission call is *not* proof that the source is actually readable and an additional crash remains possible.
- **Do not run JIT VM INIT ONLY or SECURE JIT** before inspecting the safe probe; successful JIT on PS5 is not established.
- The default AUTO 4 light interpreter benchmark, full-memory option, existing nonexecuting probes, title ID `PPSA99795`, release packaging and auto-generated TESTBUILD index are unchanged.

No kernel bypass, no RWX memory request, no cryptocurrency network, and no mining.
