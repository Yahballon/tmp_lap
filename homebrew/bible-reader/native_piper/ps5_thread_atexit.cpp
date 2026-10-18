// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 experimental compatibility implementation for the C++ ABI's
// __cxa_thread_atexit_impl hook. Native PS5 SDK stubs do not export this hook.
// POSIX TSD lets thread_local object destructors run at thread exit.
// Not intended as a replacement for regular process-wide __cxa_atexit.
#include <pthread.h>
#include <stdlib.h>

namespace {
using Destructor = void (*)(void*);
struct DtorNode {
  Destructor callback;
  void* object;
  DtorNode* next;
};
pthread_key_t tls_key;
pthread_once_t init_once = PTHREAD_ONCE_INIT;
int key_created = 0;

void cleanup_tls(void* ptr) noexcept {
  auto* current = static_cast<DtorNode*>(ptr);
  // A TLS destructor may register further TLS objects, so drain both the
  // original chain and anything added while processing it.
  for (;;) {
    while (current) {
      DtorNode* next = current->next;
      Destructor callback = current->callback;
      void* object = current->object;
      free(current);
      callback(object);
      current = next;
    }
    current = static_cast<DtorNode*>(pthread_getspecific(tls_key));
    if (!current) break;
    (void)pthread_setspecific(tls_key, nullptr);
  }
}

void create_tls_key() noexcept {
  if (pthread_key_create(&tls_key, cleanup_tls) == 0)
    key_created = 1;
}
}  // namespace

// A DSO handle is supplied by the compiler, but this statically linked PS5
// homebrew never unloads the app's code dynamically. Per-DSO unregistering is
// therefore unnecessary. Preserve LIFO destructor order within each thread.
extern "C" int __cxa_thread_atexit_impl(
    void (*destructor)(void*), void* object, void* dso_handle) noexcept {
  (void)dso_handle;
  if (!destructor || pthread_once(&init_once, create_tls_key) != 0 || !key_created)
    return -1;
  auto* node = static_cast<DtorNode*>(malloc(sizeof(DtorNode)));
  if (!node) return -1;
  node->callback = destructor;
  node->object = object;
  node->next = static_cast<DtorNode*>(pthread_getspecific(tls_key));
  if (pthread_setspecific(tls_key, node) != 0) {
    free(node);
    return -1;
  }
  return 0;
}
