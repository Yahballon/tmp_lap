// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
namespace jbx {
struct CatalogResult {
 jukebox::Library library;
 bool helperRunning=false;
 std::string status;
};
CatalogResult scan();
bool is_remote(const std::string& path);
bool image_bytes(const std::string& path,std::string& data,std::string& error);
bool stage_track(const std::string& path,uint64_t expected,std::atomic<bool>& cancelled,
                 std::string& local,std::string& error);
}
