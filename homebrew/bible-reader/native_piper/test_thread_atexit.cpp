// SPDX-License-Identifier: GPL-3.0-or-later
// Host validation for the PS5 thread_local ABI compatibility shim.
#include <pthread.h>
#include <cassert>
#include <atomic>
extern "C" int __cxa_thread_atexit_impl(void (*)(void*), void*, void*) noexcept;
static std::atomic<int> a{0}, b{0}, nested{0};
static void destructor(void* obj) {
  auto* ctr = static_cast<std::atomic<int>*>(obj);
  ctr->fetch_add(1, std::memory_order_relaxed);
}
static void register_nested(void* obj) {
  destructor(obj);
  assert(__cxa_thread_atexit_impl(destructor, &nested, nullptr) == 0);
}
static void* run(void*) {
  assert(__cxa_thread_atexit_impl(destructor, &a, nullptr) == 0);
  assert(__cxa_thread_atexit_impl(register_nested, &b, nullptr) == 0);
  assert(a.load() == 0 && b.load() == 0);
  return nullptr;
}
int main() {
  pthread_t workers[3]{};
  for (auto& worker : workers)
    assert(pthread_create(&worker, nullptr, run, nullptr) == 0);
  for (auto& worker : workers)
    assert(pthread_join(worker, nullptr) == 0);
  assert(a.load() == 3);
  assert(b.load() == 3);
  assert(nested.load() == 3);
}
