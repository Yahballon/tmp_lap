// SPDX-License-Identifier: GPL-3.0-or-later
// Host contract tests for native-PS5 Mesa temporary-file compatibility.
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
extern int mkstemps(char *, int);
int main(void) {
    char first[] = "/tmp/zcps5-mesa-XXXXXX.log";
    char second[] = "/tmp/zcps5-mesa-XXXXXX.log";
    int fd = mkstemps(first, 4);
    assert(fd >= 0 && strstr(first, "XXXXXX") == NULL);
    assert(write(fd, "a", 1) == 1);
    assert(close(fd) == 0);
    int other = mkstemps(second, 4);
    assert(other >= 0);
    assert(strcmp(first, second) != 0);
    assert(close(other) == 0);
    assert(unlink(first) == 0 && unlink(second) == 0);
    char invalid[] = "/tmp/bad-name";
    errno = 0;
    assert(mkstemps(invalid, 0) < 0 && errno == EINVAL);
    openlog("ZQuest PS5 CI", 0, LOG_USER);
    syslog(LOG_INFO, "Mesa logfile service works with %s", "stderr");
    closelog();
    puts("PASS: native Mesa mkstemps and syslog contract");
    return 0;
}
