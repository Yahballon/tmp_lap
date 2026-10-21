// SPDX-License-Identifier: GPL-3.0-or-later
// ZQuestClassic zplayer argument bridge; no claim that zplayer is linked yet.
// Based on upstream src/zc/zelda.cpp: zapp_check_switch("-standalone", {"qst"}).
#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace zcps5::engine {

// These are real paths used by the existing PS5 quest browser. This rejects
// unsanitized arbitrary input; quest file selection must come from the browser.
inline bool is_supported_quest_path(const std::string& path) {
  if (path.find('\0') != std::string::npos || path.find('\\') != std::string::npos ||
      path.find("//") != std::string::npos || path.find("/../") != std::string::npos ||
      path.find("/./") != std::string::npos || path.size() > 1023)
    return false;
  if (path.size() < 5 || path.back() == '/') return false;
  const auto dot = path.rfind('.');
  if (dot == std::string::npos || path.size() - dot != 4) return false;
  std::string ext = path.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (ext != ".qst") return false;
  const char* const roots[] = {
    "/data/ZQuestClassic/quests/", "/data/homebrew/PPSA99127/quests/",
    "/mnt/usb0/ZQuestClassic/quests/", "/mnt/usb1/ZQuestClassic/quests/",
    "/mnt/usb0/quests/", "/mnt/usb1/quests/"
  };
  for (const char* root : roots)
    if (path.compare(0, std::char_traits<char>::length(root), root) == 0)
      return true;
  return false;
}

// Returns argv string storage for the upstream zplayer main(argc,argv).
// Caller constructs char* pointers AFTER storing the vector permanently.
// The standalone save path is fixed inside the console's writable data dir,
// independent of untrusted quest names.
inline std::vector<std::string> make_standalone_args(
    const std::string& absolute_quest_path) {
  if (!is_supported_quest_path(absolute_quest_path)) return {};
  const auto slash = absolute_quest_path.find_last_of('/');
  const auto dot = absolute_quest_path.find_last_of('.');
  const std::string base = absolute_quest_path.substr(slash + 1, dot - slash - 1);
  std::string save = base;
  for (char& ch : save)
    if (!(ch >= 'a' && ch <= 'z') && !(ch >= 'A' && ch <= 'Z') &&
        !(ch >= '0' && ch <= '9') && ch != '_' && ch != '-')
      ch = '_';
  if (save.size() > 80) save.resize(80);
  if (save.empty()) save = "quest";
  return {"zplayer", "-standalone", absolute_quest_path,
          "/data/ZQuestClassic/saves/" + save + ".sav"};
}

}  // namespace zcps5::engine
