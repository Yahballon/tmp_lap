// SPDX-License-Identifier: GPL-3.0-or-later
#include "randomx.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
  const char* key="PS5Coin benchmark - genesis-independent key - v0.1";
  auto* cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  assert(cache);
  randomx_init_cache(cache,key,std::strlen(key));
  auto* vm=randomx_create_vm(RANDOMX_FLAG_DEFAULT,cache,nullptr);
  assert(vm);
  unsigned char input[64]={}, a[32]={}, b[32]={}, c[32]={};
  std::memcpy(input,"PS5RandomX-v0.1 PS5Coin independent bench",40);
  randomx_calculate_hash(vm,input,sizeof(input),a);
  randomx_calculate_hash(vm,input,sizeof(input),b);
  assert(std::memcmp(a,b,32)==0);
  input[56]=1;
  randomx_calculate_hash(vm,input,sizeof(input),c);
  assert(std::memcmp(a,c,32)!=0);
  randomx_destroy_vm(vm);
  randomx_release_cache(cache);
  std::puts("PASS: real light-mode RandomX deterministic hashing");
  return 0;
}
