// SPDX-License-Identifier: GPL-3.0-or-later
// Host-only verification: JIT flags, VM constructor and generated hash agree.
#include "randomx.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
  const randomx_flags supported=randomx_get_flags();
  if(!(supported&RANDOMX_FLAG_JIT)||!(supported&RANDOMX_FLAG_HARD_AES)){
    std::puts("SKIP: JIT or AES unsupported on host");
    return 0;
  }
  const char* key="PS5Coin benchmark - genesis-independent key - v0.1";
  randomx_cache* c=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  assert(c);
  randomx_init_cache(c,key,std::strlen(key));
  randomx_vm* interpreter=randomx_create_vm(RANDOMX_FLAG_HARD_AES,c,nullptr);
  assert(interpreter);
  randomx_vm* jit=randomx_create_vm(RANDOMX_FLAG_HARD_AES|RANDOMX_FLAG_JIT|
                                     RANDOMX_FLAG_SECURE,c,nullptr);
  assert(jit);
  unsigned char input[64]={},a[32]={},b[32]={};
  std::memcpy(input,"PS5RandomX-v0.1 PS5Coin independent bench",40);
  randomx_calculate_hash(interpreter,input,sizeof(input),a);
  randomx_calculate_hash(jit,input,sizeof(input),b);
  assert(std::memcmp(a,b,sizeof(a))==0);
  randomx_destroy_vm(jit);
  randomx_destroy_vm(interpreter);
  randomx_release_cache(c);
  std::puts("PASS: host RandomX secure JIT matches interpreter");
  return 0;
}
