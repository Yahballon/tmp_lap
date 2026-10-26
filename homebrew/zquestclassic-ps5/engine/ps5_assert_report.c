// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 libc compatibility: FreeBSD <assert.h> declares this 4-argument ABI.
// The payload SDK's libunwind.a references __assert, but current SDK import
// libraries do not export it. This is genuine, fail-fast assertion handling:
// it reports the failed expression and aborts. It never silently succeeds.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((noreturn))
void __assert(const char *function, const char *file,
              int line, const char *expression) {
  fprintf(stderr, "PS5 ASSERT: %s:%d in %s: %s\n",
          file ? file : "?", line,
          function ? function : "?", expression ? expression : "?");
  fflush(stderr);
  abort();
}
