// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library.hpp"
#include <atomic>
#include <string>
#include <vector>
namespace smb_music {
struct ScanResult{
 jukebox::Library library;
 std::vector<jukebox::PackedAlbum> archives;
 std::string status;
};
bool is_smb(const std::string& path);
ScanResult scan();
bool artwork(const std::string& path,std::string& data,std::string& error);
bool stage(const std::string& path,uint64_t expected,std::atomic<bool>& cancel,
           std::string& local,std::string& error,std::atomic<uint64_t>* copied=nullptr);
}
