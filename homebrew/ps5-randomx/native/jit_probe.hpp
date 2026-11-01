// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Controlled W^X mapping probe. Never executes mapped bytes; never maps RWX.
// Passing this test is NECESSARY, not SUFFICIENT, for executing a RandomX JIT VM.
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

namespace rxbench {
struct JitProbe {
  bool mapped=false;
  bool rx=false;
  bool rw_restored=false;
  bool unmapped=false;
  int error=0;
  const char* failed_at="none";
  bool passed() const { return mapped && rx && rw_restored && unmapped && !error; }
};

inline JitProbe probe_wx_transition() {
  constexpr std::size_t kPage=16384; // multiple of 4K; also works with 16K PS5 pages
  JitProbe result;
  void* region=mmap(nullptr,kPage,PROT_READ|PROT_WRITE,
                    MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(region==MAP_FAILED) {
    result.error=errno;result.failed_at="mmap(RW)";return result;
  }
  result.mapped=true;
  // Use an innocuous marker; this probe DOES NOT execute data/code.
  static_cast<volatile std::uint8_t*>(region)[0]=0x59;
  if(mprotect(region,kPage,PROT_READ|PROT_EXEC)!=0) {
    result.error=errno;result.failed_at="mprotect(RX)";
  }else{
    result.rx=true;
    if(mprotect(region,kPage,PROT_READ|PROT_WRITE)!=0){
      result.error=errno;result.failed_at="mprotect(RW)";
    }else result.rw_restored=true;
  }
  if(munmap(region,kPage)==0)result.unmapped=true;
  else if(!result.error){result.error=errno;result.failed_at="munmap";}
  return result;
}
} // namespace rxbench
