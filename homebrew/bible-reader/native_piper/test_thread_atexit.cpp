// SPDX-License-Identifier: GPL-3.0-or-later
// Host validation for the PS5 thread_local ABI compatibility shim.
#include <pthread.h>
#include <cassert>
#include <atomic>
extern "C" int __cxa_thread_atexit_impl(void (*)(void*), void*, void*) noexcept;
static std::atomic<int> a{0}, b{0}, nested{0};
static pthread_barrier_t all_registered;
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
  // All three workers must register their TLS destructors before any
  // worker may leave. Without this barrier, an earlier worker can exit
  // and increment these shared counters before a later worker asserts.
  assert(a.load() == 0 && b.load() == 0);
  const int wait = pthread_barrier_wait(&all_registered);
  assert(wait == 0 || wait == PTHREAD_BARRIER_SERIAL_THREAD);
  return nullptr;
}
int main() {
  assert(pthread_barrier_init(&all_registered, nullptr, 3) == 0);
  pthread_t workers[3]{};
  for (auto& worker : workers)
    assert(pthread_create(&worker, nullptr, run, nullptr) == 0);
  for (auto& worker : workers)
    assert(pthread_join(worker, nullptr) == 0);
  assert(a.load() == 3);
  assert(b.load() == 3);
  assert(nested.load() == 3);
  assert(pthread_barrier_destroy(&all_registered) == 0);
}
