// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
namespace jukebox {
// The PS5 sandbox can deny lstat/stat/fstat on app-owned Cache/ paths,
// while open/read/lseek work. Reuse a successfully staged file instead of
// transferring potentially hundreds of MB again at every startup.
// O_NOFOLLOW prevents accepting symlinks as cached media.
inline bool media_cache_matches(const std::string& path,uint64_t expected){
 struct stat st{};
 if(lstat(path.c_str(),&st)==0)
  return S_ISREG(st.st_mode)&&st.st_size>=0&&(uint64_t)st.st_size==expected;
 if(errno!=EPERM&&errno!=EACCES&&errno!=ENOSYS)return false;
 int flags=O_RDONLY;
#ifdef O_NOFOLLOW
 flags|=O_NOFOLLOW;
#endif
 const int fd=open(path.c_str(),flags);
 if(fd<0)return false;
 bool result=false;
 if(fstat(fd,&st)==0)
  result=S_ISREG(st.st_mode)&&st.st_size>=0&&(uint64_t)st.st_size==expected;
 else if(errno==EPERM||errno==EACCES||errno==ENOSYS){
  const off_t end=lseek(fd,0,SEEK_END);
  result=end>=0&&(uint64_t)end==expected;
 }
 close(fd);
 return result;
}
}
