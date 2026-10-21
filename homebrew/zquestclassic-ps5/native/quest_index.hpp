// SPDX-License-Identifier: GPL-3.0-or-later
// PS5-specific incremental quest directory scanner. No quest parsing happens here.
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <deque>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace zcps5 {
struct QuestEntry {
  std::string path;
  std::string name;
  uint64_t bytes = 0;
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
  static constexpr size_t kMaxQuests = 256;
  static constexpr size_t kMaxEntries = 6000;
  static constexpr size_t kMaxDirs = 512;
  void begin(const std::vector<std::string>& roots) {
    close_current();
    quests_.clear();
    queue_.clear();
    examined_ = 0;
    done_ = false;
    for (const auto& r : roots)
      if (!r.empty() && r.size() < 240) queue_.push_back({r, 0});
  }
  ~QuestIndex() { close_current(); }
  QuestIndex() = default;
  QuestIndex(const QuestIndex&) = delete;
  QuestIndex& operator=(const QuestIndex&) = delete;
  // Scan at most budget entries per frame so filesystem I/O cannot monopolize
  // the render loop for entire directory trees.
  bool step(size_t budget = 16) {
    if (done_) return true;
    size_t processed = 0;
    while (processed < budget && examined_ < kMaxEntries &&
           quests_.size() < kMaxQuests) {
      if (!dir_) {
        if (queue_.empty()) break;
        current_ = std::move(queue_.front());
        queue_.pop_front();
        dir_ = opendir(current_.first.c_str());
        if (!dir_) continue;
      }
      dirent* de = readdir(dir_);
      if (!de) { close_current(); continue; }
      ++processed;
      ++examined_;
      const std::string name(de->d_name);
      if (name == "." || name == "..") continue;
      if (name.size() > 240 || current_.first.size() + name.size() + 1 > 1023)
        continue;
      const std::string full = current_.first + "/" + name;
      struct stat st{};
      // Do not follow symlinks outside the explicitly configured quest roots.
      if (lstat(full.c_str(), &st) != 0) continue;
      if (S_ISDIR(st.st_mode)) {
        if (current_.second < 3 && queue_.size() < kMaxDirs)
          queue_.push_back({full, current_.second + 1});
      } else if (S_ISREG(st.st_mode) && is_quest_filename(name)) {
        quests_.push_back({full, display_name(name), static_cast<uint64_t>(st.st_size)});
      }
    }
    if ((queue_.empty() && !dir_) || examined_ >= kMaxEntries ||
        quests_.size() >= kMaxQuests) {
      close_current();
      queue_.clear();
      std::sort(quests_.begin(), quests_.end(),
                [](const QuestEntry& a, const QuestEntry& b) {
                  return a.name == b.name ? a.path < b.path : a.name < b.name;
                });
      done_ = true;
    }
    return done_;
  }
  bool done() const { return done_; }
  size_t examined() const { return examined_; }
  const std::vector<QuestEntry>& quests() const { return quests_; }
 private:
  void close_current() {
    if (dir_) { closedir(dir_); dir_ = nullptr; }
  }
  std::deque<std::pair<std::string, unsigned>> queue_;
  std::pair<std::string, unsigned> current_;
  std::vector<QuestEntry> quests_;
  DIR* dir_ = nullptr;
  size_t examined_ = 0;
  bool done_ = true;
};
}  // namespace zcps5
