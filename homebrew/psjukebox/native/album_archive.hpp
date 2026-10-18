// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include "library.hpp"
namespace jukebox {
// A .album is a regular ZIP file renamed to .album. Audio/cover/background
// entries can be stored at archive root or inside one wrapping folder.
bool is_album_file(const std::string& filename);
std::string album_stem(const std::string& filename);
// Read the name of the zero- or small-content <Artist>.artist marker in a ZIP.
std::string album_artist_marker(const std::string& archive,std::string& error);
std::string archive_cache_root();
// Unpack only supported music and cover/background images into an isolated
// cache root/Music/<Artist>/<Album>. Never writes into the source artist folder.
bool unpack_album(const std::string& archive,const std::string& artist,
                  const std::string& album,const std::string& sourceKey,
                  const std::string& cacheRoot,std::string& musicRoot,
                  std::string& error,Library* imported=nullptr);
}
