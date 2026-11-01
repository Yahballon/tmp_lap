// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Durable breadcrumbs for abrupt native crashes (e.g., SIGSEGV during JIT).
// Keep file I/O outside hooks/signal handlers. No attempt to bypass memory policy.
#include <cstdint>
#include <cstdio>
#include <sys/stat.h>

namespace rxbench {
inline void jit_trace(const char* stage, int profile, std::uint64_t session, int value=0) {
  // Same writable destination already confirmed by benchmark.log on console.
  mkdir("/data/ps5-randomx",0777);
  FILE* f=std::fopen("/data/ps5-randomx/jit_trace.log","a");
  if(!f)return;
  std::fprintf(f,"PS5 RANDOMX JIT TRACE v0.5.0 session=%llu profile=%d stage=%s value=%d\n",
               (unsigned long long)session,profile,stage,value);
  std::fflush(f);
  std::fclose(f);
}
inline void jit_trace_pointer(const char* stage, int profile, std::uint64_t session,
                              std::uint64_t address) {
  mkdir("/data/ps5-randomx",0777);
  FILE* f=std::fopen("/data/ps5-randomx/jit_trace.log","a");
  if(!f)return;
  std::fprintf(f,"PS5 RANDOMX JIT TRACE v0.5.0 session=%llu profile=%d stage=%s ptr=0x%016llx\n",
               (unsigned long long)session,profile,stage,(unsigned long long)address);
  std::fflush(f);
  std::fclose(f);
}
} // namespace rxbench
