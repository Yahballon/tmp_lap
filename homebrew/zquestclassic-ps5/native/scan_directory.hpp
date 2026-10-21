// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 directory access. The ordinary libc opendir() may return EPERM
// in PS5 bigapp sandboxes, even if the directory exists in /data.
// The direct libkernel path follows the proven PS5 dirshim implementation
// used by Yahballon/PS5GBCX (ps5/coreorbis/orbis-shims/dirshim.cpp).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#if defined(ZCPS5_NATIVE_SCAN)
#include <fcntl.h>
extern "C" int sceKernelOpen(const char*, int, int);
extern "C" int sceKernelGetdents(int, char*, int);
extern "C" int sceKernelClose(int);
#else
#include <cerrno>
#include <dirent.h>
#endif

namespace zcps5 {
struct ScannedEntry {
  std::string name;
  unsigned char type = 0;
};

class ScanDirectory {
 public:
  ScanDirectory() = default;
  ~ScanDirectory() { close(); }
  ScanDirectory(const ScanDirectory&) = delete;
  ScanDirectory& operator=(const ScanDirectory&) = delete;

  bool open(const std::string& path) {
    close();
    error_ = 0;
#if defined(ZCPS5_NATIVE_SCAN)
    // Never traverse directory symlinks outside allowed roots.
    fd_ = sceKernelOpen(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW, 0);
    if (fd_ < 0) {
      error_ = fd_;  // libkernel returns a signed SCE error code.
      fd_ = -1;
      return false;
    }
    pos_ = 0;
    used_ = 0;
    return true;
#else
    dir_ = opendir(path.c_str());
    if (!dir_) { error_ = errno; return false; }
    return true;
#endif
  }
  // 1: entry; 0: end; -1: failed (error_code() is available).
  int next(ScannedEntry& item) {
#if defined(ZCPS5_NATIVE_SCAN)
    if (fd_ < 0) return 0;
    for (;;) {
      if (pos_ + 8 <= used_) {
        const unsigned char* p = data_ + pos_;
        const size_t record = size_t(p[4]) | (size_t(p[5]) << 8);
        const size_t length = p[7];
        // FreeBSD dirent header: ino(4), reclen(2), type(1), namlen(1).
        // Never read a truncated/invalid record or loop forever on reclen=0.
        if (record < 12 || (record % 4) != 0 ||
            pos_ + record > used_ || length == 0 ||
            length > 255 || 8 + length >= record) {
          error_ = -100001;
          return -1;
        }
        pos_ += record;
        item.type = p[6];
        item.name.assign(reinterpret_cast<const char*>(p + 8), length);
        if (item.name.find('\0') != std::string::npos) {
          error_ = -100002;
          return -1;
        }
        return 1;
      }
      if (pos_ != used_) { error_ = -100003; return -1; }
      const int n = sceKernelGetdents(fd_, reinterpret_cast<char*>(data_),
                                      static_cast<int>(sizeof(data_)));
      if (n < 0) { error_ = n; return -1; }
      if (n == 0) return 0;
      if (size_t(n) > sizeof(data_)) { error_ = -100004; return -1; }
      used_ = static_cast<size_t>(n);
      pos_ = 0;
    }
#else
    if (!dir_) return 0;
    errno = 0;
    dirent* de = readdir(dir_);
    if (!de) { if (errno) { error_ = errno; return -1; } return 0; }
    item.name = de->d_name;
    item.type = de->d_type;
    return 1;
#endif
  }
  void close() {
#if defined(ZCPS5_NATIVE_SCAN)
    if (fd_ >= 0) sceKernelClose(fd_);
    fd_ = -1;
    used_ = pos_ = 0;
#else
    if (dir_) closedir(dir_);
    dir_ = nullptr;
#endif
  }
  int error_code() const { return error_; }
 private:
#if defined(ZCPS5_NATIVE_SCAN)
  int fd_ = -1;
  // Match the working PS5GBCX dirshim: 16-KiB alignment, 64-KiB buffer.
  // The old 0x4000-byte allocation produced PS5 EINVAL (0x80020016).
  alignas(0x4000) unsigned char data_[0x10000]{};
  size_t pos_ = 0;
  size_t used_ = 0;
#else
  DIR* dir_ = nullptr;
#endif
  int error_ = 0;
};
}  // namespace zcps5
