// SPDX-License-Identifier: GPL-3.0-or-later
// Host-only v0.2.0 regression: real RandomX, AES equivalence, independent VMs.
#include "randomx.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <pthread.h>
struct ThreadData {
  randomx_cache* cache;
  randomx_flags flags;
  unsigned worker;
  unsigned char result[32]{};
  bool okay=false;
};
void* workerFn(void* p) {
  auto* a=static_cast<ThreadData*>(p);
  randomx_vm* vm=randomx_create_vm(a->flags,a->cache,nullptr);
  if(!vm)return nullptr;
  unsigned char input[64]{};
  std::memcpy(input,"PS5RandomX-v0.1 PS5Coin independent bench",40);
  input[48]=static_cast<unsigned char>(a->worker);
  for(unsigned i=0;i<3;i++){
    input[56]=static_cast<unsigned char>(i);
    randomx_calculate_hash(vm,input,sizeof(input),a->result);
  }
  a->okay=true;
  randomx_destroy_vm(vm);
  return nullptr;
}
int main() {
  const char* key="PS5Coin benchmark - genesis-independent key - v0.1";
  randomx_cache* cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
  assert(cache);
  randomx_init_cache(cache,key,std::strlen(key));
  auto* vm=randomx_create_vm(RANDOMX_FLAG_DEFAULT,cache,nullptr);
  assert(vm);
  unsigned char input[64]={},first[32]={},again[32]={},altered[32]={};
  std::memcpy(input,"PS5RandomX-v0.1 PS5Coin independent bench",40);
  randomx_calculate_hash(vm,input,sizeof(input),first);
  randomx_calculate_hash(vm,input,sizeof(input),again);
  assert(std::memcmp(first,again,32)==0);
  input[56]=1; randomx_calculate_hash(vm,input,sizeof(input),altered);
  assert(std::memcmp(first,altered,32)!=0);
  randomx_destroy_vm(vm);

  const bool hw=(randomx_get_flags() & RANDOMX_FLAG_HARD_AES)!=0;
  if(hw) {
    vm=randomx_create_vm(RANDOMX_FLAG_HARD_AES,cache,nullptr);
    assert(vm);
    input[56]=0;randomx_calculate_hash(vm,input,sizeof(input),again);
    assert(std::memcmp(first,again,32)==0);
    randomx_destroy_vm(vm);
    std::puts("PASS: software and hardware AES produce same RandomX hash");
  } else std::puts("SKIP: host has no hardware AES");

  ThreadData a[2]={{cache,hw?RANDOMX_FLAG_HARD_AES:RANDOMX_FLAG_DEFAULT,0},
                   {cache,hw?RANDOMX_FLAG_HARD_AES:RANDOMX_FLAG_DEFAULT,1}};
  pthread_t threads[2]{};
  assert(pthread_create(&threads[0],nullptr,workerFn,&a[0])==0);
  assert(pthread_create(&threads[1],nullptr,workerFn,&a[1])==0);
  pthread_join(threads[0],nullptr);pthread_join(threads[1],nullptr);
  assert(a[0].okay&&a[1].okay);
  assert(std::memcmp(a[0].result,a[1].result,32)!=0);
  randomx_release_cache(cache);
  std::puts("PASS: deterministic hashes and parallel RandomX light-mode VMs");
  return 0;
}
