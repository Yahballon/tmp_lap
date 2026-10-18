// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library.hpp"
#include <string>
namespace jukebox {
// A bounded, CRC-checked binary snapshot used only for instant UI startup.
// The authoritative USB/local/SMB discovery still refreshes asynchronously.
// No passwords, network credentials or audio samples are written.
std::string library_cache_path();
bool save_library_cache(const std::string& path,const Library& library,std::string& error);
bool load_library_cache(const std::string& path,Library& library,std::string& error);
}
