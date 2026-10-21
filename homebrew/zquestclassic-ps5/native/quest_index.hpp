// SPDX-License-Identifier: GPL-3.0-or-later
// Incremental PS5 quest scanner. Filename discovery only; no game engine yet.
#pragma once
#include "scan_directory.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace zcps5 {
struct QuestEntry {
  std::string path;
  std::string name;
  uint64_t bytes = 0;
  bool size_known = false;
};

inline bool is_quest_filename(const std::string& name) {
  if (name.size() < 5) return false;
  const auto pos = name.rfind('.');
  if (pos == std::string::npos || name.size() - pos != 4) return false;
  return std::tolower(static_cast<unsigned char>(name[pos + 1])) == 'q' &&
         std::tolower(static_cast<unsigned char>(name[pos + 2])) == 's' &&
         std::tolower(static_cast<unsigned char>(name[pos + 3])) == 't';
}
inline std::string display_name(std::string name) {
  if (is_quest_filename(name)) name.resize(name.size() - 4);
  for (auto& c : name) if (c == '_' || c == '\t' || c == '\n') c = ' ';
  if (name.size() > 64) name.resize(64);
  return name;
}

class QuestIndex {
 public:
  using LogFn = void (*)(const char*);
  static constexpr size_t kMaxQuests = 256;
  static constexpr size_t kMaxEntries = 6000;
  static constexpr size_t kMaxDirs = 512;
  void set_log(LogFn logger) { logger_ = logger; }
  void begin(const std::vector<std::string>& roots) {
    directory_.close();
    quests_.clear();
    queue_.clear();
    examined_ = 0;
    opened_ = failed_ = 0;
    error_.clear();
    done_ = false;
    for (const auto& root : roots)
      if (!root.empty() && root.size() < 240) queue_.push_back({root, 0});
    log("SCAN_START");
  }
  QuestIndex() = default;
  QuestIndex(const QuestIndex&) = delete;
  QuestIndex& operator=(const QuestIndex&) = delete;

  bool step(size_t budget = 16) {
    if (done_) return true;
    size_t processed = 0;
    while (processed < budget && examined_ < kMaxEntries &&
           quests_.size() < kMaxQuests) {
      if (!opened_directory_) {
        if (queue_.empty()) break;
        current_ = std::move(queue_.front());
        queue_.pop_front();
        if (!directory_.open(current_.first)) {
          ++failed_;
          char message[340];
          snprintf(message, sizeof(message), "DIR_FAIL code=%d path=%s",
                   directory_.error_code(), current_.first.c_str());
          log(message);
          if (error_.empty()) error_ = message;
          continue;
        }
        opened_directory_ = true;
        ++opened_;
        char message[340];
        snprintf(message, sizeof(message), "DIR_OK path=%s", current_.first.c_str());
        log(message);
      }
      ScannedEntry entry;
      const int result = directory_.next(entry);
      if (result <= 0) {
        if (result < 0) {
          ++failed_;
          char message[340];
          snprintf(message, sizeof(message), "DENTS_FAIL code=%d path=%s",
                   directory_.error_code(), current_.first.c_str());
          log(message);
          if (error_.empty()) error_ = message;
        }
        directory_.close();
        opened_directory_ = false;
        continue;
      }
      ++processed;
      ++examined_;
      const std::string& name = entry.name;
      if (name == "." || name == "..") continue;
      if (name.size() > 240 || current_.first.size() + name.size() + 1 > 1023)
        continue;
      const std::string full = current_.first + "/" + name;

      // Native libc lstat() can fail in PS5 bigapp sandboxes: never use it
      // as a prerequisite for showing a game. Use getdents d_type instead.
      bool directory = entry.type == 4;  // DT_DIR
      bool regular = entry.type == 8;    // DT_REG
      if (entry.type == 10) continue;    // DT_LNK: do not leave selected roots
#if !defined(ZCPS5_NATIVE_SCAN)
      // Host filesystems may report DT_UNKNOWN. Resolve it without following
      // symlinks (and retain size metadata when available).
      if (entry.type == 0) {
        struct stat st{};
        if (lstat(full.c_str(), &st) != 0) continue;
        directory = S_ISDIR(st.st_mode);
        regular = S_ISREG(st.st_mode);
      }
#else
      // PS5 kernel may report DT_UNKNOWN. Permit a matching .qst filename;
      // do not silently hide a quest due to missing stat/lstat privileges.
      if (entry.type == 0 && is_quest_filename(name)) regular = true;
#endif
      if (directory) {
        if (current_.second < 3 && queue_.size() < kMaxDirs)
          queue_.push_back({full, current_.second + 1});
      } else if (regular && is_quest_filename(name)) {
        QuestEntry q;
        q.path = full;
        q.name = display_name(name);
        struct stat st{};
        // Optional metadata: stats may fail although getdents found the file.
        if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
          q.bytes = static_cast<uint64_t>(st.st_size);
          q.size_known = true;
        }
        quests_.push_back(std::move(q));
        char message[340];
        snprintf(message, sizeof(message), "QUEST_FOUND %s", full.c_str());
        log(message);
      }
    }
    if ((queue_.empty() && !opened_directory_) ||
        examined_ >= kMaxEntries || quests_.size() >= kMaxQuests) {
      directory_.close();
      opened_directory_ = false;
      queue_.clear();
      std::sort(quests_.begin(), quests_.end(),
                [](const QuestEntry& a, const QuestEntry& b) {
                  return a.name == b.name ? a.path < b.path : a.name < b.name;
                });
      done_ = true;
      char summary[200];
      snprintf(summary, sizeof(summary), "SCAN_DONE quests=%zu entries=%zu opened=%zu errors=%zu",
               quests_.size(), examined_, opened_, failed_);
      log(summary);
    }
    return done_;
  }
  bool done() const { return done_; }
  size_t examined() const { return examined_; }
  size_t opened() const { return opened_; }
  size_t failed() const { return failed_; }
  const std::string& first_error() const { return error_; }
  const std::vector<QuestEntry>& quests() const { return quests_; }
 private:
  void log(const char* message) const { if (logger_) logger_(message); }
  ScanDirectory directory_;
  bool opened_directory_ = false;
  std::deque<std::pair<std::string, unsigned>> queue_;
  std::pair<std::string, unsigned> current_;
  std::vector<QuestEntry> quests_;
  size_t examined_ = 0;
  size_t opened_ = 0, failed_ = 0;
  std::string error_;
  LogFn logger_ = nullptr;
  bool done_ = true;
};
}  // namespace zcps5
