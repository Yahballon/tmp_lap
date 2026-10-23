// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 platform functions used by statically linked ps5-opengl/Mesa.
// mkstemps implementation adapted from the real PS5 homebrew OpenGL runtime:
// https://github.com/tertiumndatur/ps5-cloud-play (GPL-3.0-or-later).
// Only included in the independent zplayer PS5 cross-compile target.
// This is not a fake graphics driver or a replacement for Mesa.
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>

#ifndef ZC_PS5_COMPAT_HOST_TEST
extern uint64_t sceKernelGetProcessTime(void);
#else
#include <time.h>
static uint64_t sceKernelGetProcessTime(void) {
    return (uint64_t)time(NULL) * UINT64_C(1000000);
}
#endif

int mkstemps(char *pattern, int suffix_length) {
    static unsigned counter;
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    const size_t size = pattern ? strlen(pattern) : 0;
    if (suffix_length < 0 || size < (size_t)suffix_length + 6u) {
        errno = EINVAL;
        return -1;
    }
    char *slot = pattern + size - (size_t)suffix_length - 6u;
    for (unsigned i = 0; i < 6; ++i) {
        if (slot[i] != 'X') {
            errno = EINVAL;
            return -1;
        }
    }
    // O_EXCL and mode 0600 prevent overwrites or permission exposure.
    // This generator is for temporary Mesa diagnostic files, not secrets.
    for (unsigned tries = 0; tries < 128; ++tries) {
        uint64_t seed = sceKernelGetProcessTime() ^
            ((uint64_t)__atomic_add_fetch(&counter, 1u, __ATOMIC_RELAXED) *
             UINT64_C(0x9e3779b97f4a7c15));
        for (unsigned k = 0; k < 6; ++k) {
            slot[k] = chars[seed % 36u];
            seed /= 36u;
        }
        int fd = open(pattern, O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd >= 0 || errno != EEXIST)
            return fd;
    }
    errno = EEXIST;
    return -1;
}

// PS5 homebrew does not expose the desktop syslog daemon's socket.
// Direct Mesa's log entries to stderr, which our app's logging layer can
// capture. Keep the standard openlog/syslog/closelog API and don't discard
// diagnostic text silently.
static char ps5_log_ident[64];
static int ps5_log_facility = LOG_USER;
void openlog(const char *ident, int option, int facility) {
    (void)option;
    ps5_log_facility = facility;
    if (!ident) { ps5_log_ident[0] = '\0'; return; }
    size_t count = strlen(ident);
    if (count >= sizeof(ps5_log_ident)) count = sizeof(ps5_log_ident) - 1;
    memcpy(ps5_log_ident, ident, count);
    ps5_log_ident[count] = '\0';
}
void closelog(void) { ps5_log_ident[0] = '\0'; ps5_log_facility = LOG_USER; }
void vsyslog(int priority, const char *fmt, va_list ap) {
    (void)priority;
    (void)ps5_log_facility;
    if (ps5_log_ident[0]) fprintf(stderr, "[%s] ", ps5_log_ident);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr);
}
void syslog(int priority, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsyslog(priority, fmt, ap);
    va_end(ap);
}
