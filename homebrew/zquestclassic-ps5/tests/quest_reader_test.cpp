// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#if defined(ZCPS5_NATIVE_SCAN)
extern "C" int sceKernelOpen(const char* path, int flags, int mode) {
  const int fd = ::open(path, flags, mode);
  return fd >= 0 ? fd : -2;
}
extern "C" std::int64_t sceKernelRead(int fd, void* buf, std::size_t size) {
  return ::read(fd, buf, size);
}
extern "C" int sceKernelClose(int fd) { return ::close(fd); }
#endif

#include "../native/quest_reader.hpp"

int main() {
  char dir[] = "/tmp/zquest-quest-read-XXXXXX";
  char* root = mkdtemp(dir);
  assert(root);
  const std::string path = std::string(root) + "/255example.qst";
  FILE* f = std::fopen(path.c_str(), "wb");
  assert(f);
  assert(std::fwrite("123456789", 1, 9, f) == 9);
  assert(std::fclose(f) == 0);

  zcps5::QuestReader qr;
  assert(qr.begin(path, 9));
  int steps = 0;
  while (qr.state() == zcps5::QuestReader::State::Reading && ++steps < 32)
    qr.step();
  assert(qr.state() == zcps5::QuestReader::State::Complete);
  assert(qr.bytes_read() == 9);
  assert(qr.expected_size() == 9);
  assert(qr.crc32() == 0xcbf43926u);
  assert(qr.header_hex() == "31 32 33 34 35 36 37 38");
  qr.reset();
  assert(qr.state() == zcps5::QuestReader::State::Idle);
  assert(!qr.begin("/data/ZQuestClassic/quests/../bad.qst"));
  assert(qr.state() == zcps5::QuestReader::State::Failed);
  assert(!qr.begin("/etc/bad.txt"));
  assert(!qr.begin(path + ".bak"));
  assert(!qr.begin(std::string(root) + "/missing.qst"));
  assert(qr.state() == zcps5::QuestReader::State::Failed);

  assert(unlink(path.c_str()) == 0);
  assert(rmdir(root) == 0);
  puts("PASS: real PS5 quest I/O read contract, CRC32 and failure handling");
}
