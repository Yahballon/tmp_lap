// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 native sequential reader for the selected original .qst file.
// This is an I/O diagnostic, not the ZQuestClassic quest parser or zplayer.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>

#if defined(ZCPS5_NATIVE_SCAN)
extern "C" int sceKernelOpen(const char*, int, int);
extern "C" std::int64_t sceKernelRead(int, void*, std::size_t);
extern "C" int sceKernelClose(int);
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace zcps5 {
class QuestReader {
 public:
  enum class State { Idle, Reading, Complete, Failed };
  QuestReader() = default;
  ~QuestReader() { reset(); }
  QuestReader(const QuestReader&) = delete;
  QuestReader& operator=(const QuestReader&) = delete;

  bool begin(const std::string& absolute_path, std::uint64_t expected_size = 0) {
    reset();
    expected_size_ = expected_size;
    // This method is only called from a scanned QuestEntry; reject anything
    // that is not a concrete absolute .qst path.
    if (absolute_path.empty() || absolute_path[0] != '/' ||
        absolute_path.size() > 1023 || absolute_path.find('\0') != std::string::npos ||
        absolute_path.find("/../") != std::string::npos ||
        absolute_path.find("/./") != std::string::npos ||
        absolute_path.find("//") != std::string::npos ||
        absolute_path.find('\\') != std::string::npos) {
      fail(-100010);
      return false;
    }
    std::string suffix;
    if (absolute_path.size() >= 4)
      suffix = absolute_path.substr(absolute_path.size() - 4);
    for (auto& c : suffix)
      if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    if (suffix != ".qst") {
      fail(-100011);
      return false;
    }
#if defined(ZCPS5_NATIVE_SCAN)
    fd_ = sceKernelOpen(absolute_path.c_str(), O_RDONLY | O_NOFOLLOW, 0);
    if (fd_ < 0) { const int error = fd_; fd_ = -1; fail(error); return false; }
#else
    fd_ = ::open(absolute_path.c_str(), O_RDONLY | O_NOFOLLOW);
    if (fd_ < 0) { fail(errno); return false; }
#endif
    state_ = State::Reading;
    return true;
  }

  void step() {
    if (state_ != State::Reading) return;
    alignas(16) unsigned char bytes[32768];
#if defined(ZCPS5_NATIVE_SCAN)
    const std::int64_t n = sceKernelRead(fd_, bytes, sizeof(bytes));
#else
    const std::int64_t n = ::read(fd_, bytes, sizeof(bytes));
#endif
    if (n < 0) {
#if defined(ZCPS5_NATIVE_SCAN)
      fail(static_cast<int>(n));
#else
      fail(errno);
#endif
      return;
    }
    if (n == 0) {
      close_fd();
      if (read_bytes_ == 0) fail(-100012);
      else {
        crc_ ^= 0xffffffffu;
        state_ = State::Complete;
      }
      return;
    }
    // Bounded memory: no allocation proportional to quest size.
    if (n > static_cast<std::int64_t>(sizeof(bytes)) ||
        read_bytes_ > UINT64_MAX - static_cast<std::uint64_t>(n)) {
      fail(-100013);
      return;
    }
    for (std::int64_t i = 0; i < n; ++i) {
      if (first_size_ < sizeof(first_bytes_))
        first_bytes_[first_size_++] = bytes[i];
      crc_ ^= bytes[i];
      for (int k = 0; k < 8; ++k)
        crc_ = (crc_ >> 1) ^ (0xedb88320u & (0u - (crc_ & 1u)));
    }
    read_bytes_ += static_cast<std::uint64_t>(n);
  }

  void reset() {
    close_fd();
    state_ = State::Idle;
    error_ = 0;
    read_bytes_ = 0;
    expected_size_ = 0;
    crc_ = 0xffffffffu;
    first_size_ = 0;
    std::memset(first_bytes_, 0, sizeof(first_bytes_));
  }
  State state() const { return state_; }
  std::uint64_t bytes_read() const { return read_bytes_; }
  std::uint64_t expected_size() const { return expected_size_; }
  std::uint32_t crc32() const { return state_ == State::Complete ? crc_ : crc_ ^ 0xffffffffu; }
  int error_code() const { return error_; }
  std::string header_hex() const {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i = 0; i < first_size_; ++i) {
      if (i) out += ' ';
      out += hex[first_bytes_[i] >> 4];
      out += hex[first_bytes_[i] & 15];
    }
    return out;
  }
 private:
  void close_fd() {
    if (fd_ >= 0) {
#if defined(ZCPS5_NATIVE_SCAN)
      sceKernelClose(fd_);
#else
      ::close(fd_);
#endif
    }
    fd_ = -1;
  }
  void fail(int code) {
    close_fd();
    error_ = code;
    state_ = State::Failed;
  }
  int fd_ = -1;
  State state_ = State::Idle;
  std::uint64_t read_bytes_ = 0;
  std::uint64_t expected_size_ = 0;
  std::uint32_t crc_ = 0xffffffffu;
  int error_ = 0;
  unsigned char first_bytes_[8]{};
  std::size_t first_size_ = 0;
};
} // namespace zcps5
