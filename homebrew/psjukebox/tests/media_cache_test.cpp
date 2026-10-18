// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/media_cache.hpp"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#ifdef TEST_PS5_BLOCKED_METADATA
extern "C" int __wrap_lstat(const char*,struct stat*){errno=EPERM;return -1;}
extern "C" int __wrap_fstat(int,struct stat*){errno=EPERM;return -1;}
#endif
int main(){
 const std::string file="/tmp/psjukebox-staged-album-test.album";
 const std::string link="/tmp/psjukebox-staged-album-link.album";
 FILE* f=fopen(file.c_str(),"wb");assert(f);
 const char* data="A packed album that is already in cache.";
 assert(fwrite(data,1,strlen(data),f)==strlen(data));fclose(f);
 assert(jukebox::media_cache_matches(file,strlen(data)));
 assert(!jukebox::media_cache_matches(file,strlen(data)+1));
 assert(!jukebox::media_cache_matches("/tmp/jbx-missing-file-999999",0));
 unlink(link.c_str());assert(symlink(file.c_str(),link.c_str())==0);
 assert(!jukebox::media_cache_matches(link,strlen(data)));
 unlink(file.c_str());unlink(link.c_str());
 puts("PASS staged archive reuse with stat/PS5 EPERM fallback, missing and symlink rejection");
}
