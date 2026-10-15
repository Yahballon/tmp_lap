// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace usbaudio {
struct Entry {std::string relative;size_t bytes=0;bool local=false;};
struct Catalog {std::vector<Entry> books;std::string message;std::string usbRoot;bool viaHelper=false;};
Catalog scan();
Catalog scan_local();
using ProgressCallback=bool (*)(size_t transferred,size_t total,void* context);
bool fetch(const Entry& book,std::string& localPath,std::string& error,
           ProgressCallback progress=nullptr,void* context=nullptr);
// Only return fully verified copies: no USB transfer or cache write.
bool cached_copy(const Entry& book,std::string& localPath);
bool is_remote(const std::string& path);
std::string remote_path(const Entry& book);
bool fetch_cover(const std::string& remotePath,std::string& encoded,std::string& error);
bool entry_from_remote(const std::string& path,Entry& out);
} // namespace usbaudio
