// SPDX-License-Identifier: GPL-3.0-or-later
// Compile with -DZCPS5_NATIVE_SCAN=1: simulate PS5 libkernel dirent buffers
// while actual filesystem opendir() is only used by this host-side mock.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static std::map<int, DIR*> dirs;
static int next_handle = 30;
extern "C" int sceKernelOpen(const char* path, int flags, int) {
  assert((flags & O_DIRECTORY) != 0 && (flags & O_NOFOLLOW) != 0);
  DIR* d = opendir(path);
  if (!d) return -errno;
  const int fd = next_handle++;
  dirs[fd] = d;
  return fd;
}
extern "C" int sceKernelGetdents(int fd, char* buf, int cap) {
  auto it = dirs.find(fd);
  if (it == dirs.end()) return -9;
  // FreeBSD native dirent: ino(4), reclen(2), type(1), namlen(1), name.
  int used = 0;
  // One entry per call exercises partial getdents buffer/refill repeatedly.
  while (dirent* d = readdir(it->second)) {
    size_t len = std::strlen(d->d_name);
    assert(len < 255);
    size_t reclen = (8 + len + 1 + 3) & ~size_t(3);
    assert(reclen <= static_cast<size_t>(cap - used));
    unsigned char* dst = reinterpret_cast<unsigned char*>(buf + used);
    memset(dst, 0, reclen);
    const unsigned int inode = static_cast<unsigned int>(d->d_ino);
    memcpy(dst, &inode, 4);
    dst[4] = static_cast<unsigned char>(reclen & 0xff);
    dst[5] = static_cast<unsigned char>((reclen >> 8) & 0xff);
    dst[6] = d->d_type;
    dst[7] = static_cast<unsigned char>(len);
    memcpy(dst + 8, d->d_name, len);
    used += static_cast<int>(reclen);
    break;
  }
  return used;
}
extern "C" int sceKernelClose(int fd) {
  auto it = dirs.find(fd);
  if (it == dirs.end()) return -9;
  int ret = closedir(it->second);
  dirs.erase(it);
  return ret;
}

#include "../native/quest_index.hpp"
static void put(const std::string& p) {
  FILE* f = fopen(p.c_str(), "wb");
  assert(f != nullptr);
  assert(fwrite("quest", 1, 5, f) == 5);
  assert(fclose(f) == 0);
}
static std::string loglines;
static void recorder(const char* s) { loglines += s; loglines += '\n'; }

int main() {
  char path[] = "/tmp/zcps5-native-scan-XXXXXX";
  char* p = mkdtemp(path);
  assert(p);
  const std::string root(p);
  assert(mkdir((root + "/nested").c_str(), 0700) == 0);
  put(root + "/the_deep.qst");
  put(root + "/255example.qst");
  put(root + "/nested/MORE.QST");
  put(root + "/unrelated.txt");
  assert(symlink("/etc", (root + "/escape").c_str()) == 0);

  zcps5::QuestIndex index;
  index.set_log(recorder);
  index.begin({root, root + "/missing"});
  for (int i = 0; i < 50 && !index.done(); ++i) index.step(1);
  assert(index.done());
  assert(index.opened() == 2);
  assert(index.failed() == 1);
  assert(index.quests().size() == 3);
  assert(index.quests()[0].name == "255example");
  assert(index.quests()[1].name == "MORE");
  assert(index.quests()[2].name == "the deep");
  assert(loglines.find("QUEST_FOUND " + root + "/the_deep.qst") != std::string::npos);
  assert(loglines.find("QUEST_FOUND " + root + "/255example.qst") != std::string::npos);
  assert(loglines.find("DIR_FAIL") != std::string::npos);
  assert(loglines.find("SCAN_DONE quests=3") != std::string::npos);

  // Failure must be distinguishable from a readable empty directory.
  index.begin({root + "/missing"});
  for (int i = 0; i < 10 && !index.done(); ++i) index.step(1);
  assert(index.done() && index.opened() == 0 && index.failed() == 1);
  assert(index.quests().empty());

  assert(dirs.empty());
  assert(unlink((root + "/escape").c_str()) == 0);
  assert(unlink((root + "/the_deep.qst").c_str()) == 0);
  assert(unlink((root + "/255example.qst").c_str()) == 0);
  assert(unlink((root + "/nested/MORE.QST").c_str()) == 0);
  assert(unlink((root + "/unrelated.txt").c_str()) == 0);
  assert(rmdir((root + "/nested").c_str()) == 0);
  assert(rmdir(root.c_str()) == 0);
  puts("PASS: mocked PS5 sceKernelGetdents finds both reported quests");
}
