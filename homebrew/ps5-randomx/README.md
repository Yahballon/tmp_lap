## Critical PS5 freeze hotfix: v0.6.1

v0.6.0 attempted automatic Full Memory Secure JIT (up to ~2,080 MiB of dataset) and real-console testing reported a complete hang requiring a reboot. **Do not rerun v0.6.0 AUTO ALL 10**.

In this version, the **default one-click AUTO SAFE 7** executes 4 interpreter-light profiles plus 1/2/4-thread light Secure JIT, all in one press of X. All full-memory profiles (including legacy AUTO FULL) immediately report `FULL MEMORY DISABLED` before any large dataset allocation. This intentionally trades off full-memory performance testing for PS5 usability until the memory freeze has been diagnosed safely.

Results go to `/data/ps5-randomx/benchmark.log`. Previous real PS5 four-thread Secure JIT Light result was around 144 H/s. Full-memory JIT has **not** been successfully measured.

## One-click AUTO 10 (v0.6.1)
**Default on startup: just press X once.** The app runs ten profiles sequentially: interpreter Light (1 software AES, 1/2/4 hardware AES), Secure JIT Light (1/2/4), then Secure JIT Full Memory (1/2/4). Every result appears in a two-column summary and is appended to `/data/ps5-randomx/benchmark.log`. The previous individual tests and smaller AUTO modes remain selectable.

**Full-memory warning:** Each full-mode run initializes a 2,080 MiB RandomX dataset before the timed benchmark, potentially requiring 2.3 GiB or more with its cache and overhead. This can take time and may fail or crash due to PS5 homebrew memory restrictions. The suite stops on a reported failure; an OS-level kill may not permit logging. The already verified light secure JIT result was **144.03 H/s with 4 threads** on a real console. Full-memory JIT performance is still unknown.

# PS5 RandomX Benchmark by YAHBALLON

**v0.6.1 | PS5 Title ID: PPSA99795 | Real RandomX 1.2.1**

This app measures actual RandomX proof-of-work hashing on homebrew PS5s. It does **not** mine a coin, join a network, or earn currency.

## Console benchmark results (user-provided)
| Version | Mode | Hashrate |
| --- | --- | --- |
| v0.1.0 | 1T software AES, light/interpreter | 1.9862 H/s (60s) |
| v0.2.0 | 4T hardware AES, light/interpreter | 7.8487 H/s (30s) |

Four hardware-AES workers achieve ~3.95x total throughput, but this **does not establish the speedup from AES alone**; compare 1T with and without hardware AES.

## AUTO 3 SECURE JIT benchmark (v0.6.1)

**PS5 measurement from v0.4.4:** Secure JIT, **1 CPU thread**, 30.011 s, 1,166 hashes, **38.8520 H/s**, correct against interpreter (`jit_vector_ok=1`) and `status=completed`. This is the only confirmed native JIT thread count so far.

**New automatic test:** From the default `AUTO 4 LIGHT MODES` selection, press **Down twice** to choose **AUTO 3 SECURE JIT**, then **X**. It runs **1T Secure JIT → 2T Secure JIT → 4T Secure JIT** automatically, with 10/30/60 s duration set using Left/Right. The mode is deliberately **opt-in**; normal launch still selects interpreter AUTO4, and AUTO FULL remains separate. Existing five individual non-JIT profiles and all earlier diagnostic modes remain available (manual profiles start after the three AUTO selections).

Every JIT worker must pass an output comparison with the independent interpreter before timing. Any worker mismatch or allocation failure aborts that profile and stops the suite; no silent fallback to interpreter. Circle or Options cancels the entire remaining sequence. Each run reports to `/data/ps5-randomx/benchmark.log` plus a combined `auto_kind=secure-jit` summary; `jit_trace.log` retains the per-stage debugging breadcrumbs.

**Warning:** The console has not yet demonstrated stable 2T or 4T secure-JIT runs. CPU temperatures, homebrew runtime stability and actual scaling must be tested; multi-thread results must not be inferred from the 1-thread score. This is a benchmark only, without blockchain, mining or rewards.

## v0.6.1: Execute-only text fix and nonexecuting diagnostics

The PS5 native executable's `.text` segment is **execute-only**, not readable. v0.4.3 reproducibly crashed reading the first byte of RandomX's static JIT template at `0x420640`; its mapped destination at `0x200110000` was writable.

The build now copies the same RandomX assembly template definitions into **non-executable read-only `.rodata.rxjit`** under private `rxblob_` names, and points the JIT compiler to those readable templates instead of reading the executable text segment. The original executable assembler is still present. The native CI checks that the separate source section is non-executable and contains the prologue symbol. **This fix has not yet been validated on real PS5 hardware**.

New manual profile **JIT SAFE DIAGNOSTICS** (profile index 9, at the end of the selector) sequentially tests 77,824-byte writable memory then requests read-only permissions on the relocated static template page. It never reads or executes template instructions or creates a JIT VM. Send the fresh `jit_trace.log` containing `source_mprotect_read_rc` and `source_mprotect_read_errno`.

If this succeeds, the separate **JIT VM INIT ONLY** profile is the next experiment. It can still crash if the runtime restricts JIT allocation or code execution. **Do not test SECURE JIT hashing yet**. Default AUTO 4 remains interpreter-only.

## Earlier diagnostics

Three v0.4.3 PS5 traces have the same culprit: the 77,824-byte JIT destination mapping **can be written**, while reading byte 0 at the static assembler prologue source address `0x420640` crashes. This can mean the static section is absent, non-readable or incorrectly relocated; an RX mapping request alone cannot distinguish every case.

New optional manual profile **JIT SAFE DIAGNOSTICS** (profile index 9, last mode in the selector) runs a normal 77,824-byte buffer write/read check **and then only requests RX rights** for the assembler's source page. It **does not dereference static code or execute JIT**. Any denial is recorded rather than triggering the same unsafe load.

Inspect `/data/ps5-randomx/jit_trace.log` for `source_mprotect_rx_rc` and `source_mprotect_rx_errno`. If this diagnostic passes, do **not** infer RandomX JIT is now safe; first investigate code-section mapping/relocation. Only experimental JIT VM INIT mode attempts a source read, and it still may crash if permissions checks are insufficient.

## JIT prologue crash narrowed in v0.6.1

Two real-console v0.4.2 JIT-VM attempts returned a 77,824-byte RW mapping, then crashed **at the first 256-byte code-prologue copy**. Source and destination access are not yet distinguished. This release adds an end-of-menu **JIT 78 KiB BUFFER PROBE** (profile #8), which only maps, writes and verifies a 77,824-byte buffer with ordinary stack data—**no JIT, no assembly prologue read, no generated code**. The JIT constructor now records the full 64-bit code-buffer and prologue-source pointers and performs a bytewise prologue copy with 32-byte progress checkpoints.

**Recommended hardware procedure:** Run the independent buffer probe first and inspect `/data/ps5-randomx/jit_trace.log`. If it passes, optionally retest **JIT VM INIT ONLY** to diagnose static assembly source access. **1 THREAD / SECURE JIT** must remain untested while init crashes.

The stable AUTO4 interpreter-only benchmark still works. JIT success is not established.

## Crash isolation in v0.6.1
**v0.6.1 update:** Multiple console traces now confirm a failure *inside* `randomx_create_vm` when constructing the JIT VM, before any hash is attempted. This release instruments the pinned RandomX JIT VM constructor and its real executable buffer mapping/copy, writing `rx_*`, `jit_*` and `page_*` stages to the same trace log. The W^X probe alone is insufficient. A crash during the new test is still possible; install only on a console suitable for homebrew testing.


A console report confirmed that **JIT MEMORY PERMISSIONS repeatedly completed successfully** (mmap RW, mprotect RX, mprotect RW, munmap; errno 0). A later native crash occurred with no JIT-execution result in benchmark.log, so the failing JIT stage is **not yet known**.

Every manual JIT operation now appends a stage-specific breadcrumb to:
`/data/ps5-randomx/jit_trace.log`.

**Test order (do not jump directly to JIT execution):**
1. **JIT MEMORY PERMISSIONS** — standard W^X test, no generated code.
2. **JIT VM INIT ONLY** — *new safer intermediate test*. Initializes a RandomX secure JIT VM and destroys it, **without calculating any hash or executing generated code**.
3. **1 THREAD / SECURE JIT** — still experimental, may crash a native PS5 title. Do this **only if** the first two tests pass and the console is disposable/test-only.

After a crash, reopen via TESTBUILD and retrieve **both** `benchmark.log` and `jit_trace.log`. The last `stage=` entry helps identify exactly which call had begun when the process stopped. Log writes are flushed and closed at each critical step; system-wide crashes could still lose recent filesystem writes.

The normal **AUTO 4 LIGHT** and **AUTO ALL 5** remain untouched and cannot start JIT execution.

## JIT compatibility experiments (v0.6.1, opt-in)
After the original five individual modes there are **two new experimental profiles** (choose via Up/Down or L1/R1):

- **JIT MEMORY PERMISSIONS** — the recommended first test. Maps one 16 KiB page RW, changes protections to RX then RW, and unmaps it. **Does not execute JIT or grant RWX.** Results are logged as `jit_probe_*`, including errno and failure stage.
- **JIT VM INIT ONLY** — creates/destroys a secure JIT VM without generating hashes or executing generated code.
- **1 THREAD / SECURE JIT** — only after the first test passes on the console. Checks the same memory transitions, RandomX JIT flag, and CPU AES support; creates a RandomX light VM with `RANDOMX_FLAG_JIT | RANDOMX_FLAG_SECURE | RANDOMX_FLAG_HARD_AES` and verifies one output against an interpreter before a timed run. It can still crash the console application if JIT execution is restricted even when the permissions probe passed.

**Safety:** Default AUTO 4 and optional AUTO ALL 5 remain strictly interpreter-only. The JIT profiles are never run automatically. A successful mmap/mprotect probe does not prove that executing dynamic code is safe or permitted. Use a test console and do not bypass platform security restrictions.

The output log records: `jit=secure-wx` or `jit=off`, `jit_probe_mapped`, `jit_probe_rx`, `jit_probe_rw_restored`, `jit_probe_unmapped`, `jit_probe_errno`, `jit_probe_failed_at`, and `jit_vector_ok`.

## Automatic benchmark (default screen selection)
**Press X once** on **AUTO 4 LIGHT MODES**, the default selection on app startup. The app automatically runs the four light profiles in sequence, each for the chosen 10/30/60-second duration, shows live phase progress, saves every measurement, and writes a combined **PS5 RANDOMX AUTO TEST** summary to `/data/ps5-randomx/benchmark.log`. No manual profile switching or restart needed.

**AUTO ALL 5 (FULL RAM)** adds the fifth full-memory mode only if explicitly chosen. It may require more than 2 GiB RAM and take substantial setup time or fail on console. The default auto-test does **not** allocate the 2-GiB dataset.

Up/Down or L1/R1 switches between AUTO 4, AUTO ALL 5, and individual test modes. O cancels the entire automatic sequence; a failed step stops the sequence instead of hiding the error. The combined summary shows every successful and interrupted step.

## Profiles (Up/Down or L1/R1)
1. 1 thread / software AES / light
2. 1 thread / hardware AES / light
3. 2 threads / hardware AES / light
4. 4 threads / hardware AES / light
5. **1 thread / full memory / hardware AES / no JIT (new, experimental)**

The first four profiles share one 256 MiB cache; each worker has its own interpreter VM and scratchpad.

The fifth profile allocates the full **~2.08 GiB** dataset and needs the **~256 MiB** cache during setup, plus program overhead. This **may exceed the available memory of a native PS5 homebrew title**. It is never selected automatically. Failed allocation is reported; a platform-level out-of-memory kill may still occur. Data is generated in bounded chunks so preparation progress can be shown and most cancellation requests can be honored.

## Controls
- **X**: start the current test.
- **Up/Down** or **L1/R1**: change profile (including opt-in full memory).
- **Left/Right**: choose 10/30/60 seconds.
- **O**: cancel benchmark or preparation; exit when idle.
- **Options**: cancel while running.
- **Triangle**: clear result display.
- **Square**: show the automatic report location.

## Log and interpretation
Results append to `/data/ps5-randomx/benchmark.log` and include actual mode, AES flag, thread count, requested vs measured time, total/per-thread hashes, dataset bytes, initialization seconds, last hash and status. Initial cache/dataset preparation is excluded from H/s but separately logged for the full memory profile.

Hashing stays in interpreter mode and **JIT remains off**, because executable memory mapping on a jailbroken PS5 needs independent validation before any production mining attempt. Full memory alone should not be taken as a prediction of optimized JIT throughput.

## Release
- Repo source: `homebrew/ps5-randomx/native/main.cpp`.
- Native workflow: `.github/workflows/build-ps5-randomx.yml`.
- RandomX pinned to upstream v1.2.1 (`102f8acf90a7649ada410de5499a7ec62e49e1da`).
- Release tag: `ps5-randomx-v0.6.1`.
- Installer: `PPSA99795.zip`.
- TESTBUILD `version.json` is **automatically generated by GitHub Actions**, never edited manually.

## Licensing
Homebrew integration: GPL-3.0-or-later. RandomX upstream BSD-3-Clause included with build output.
