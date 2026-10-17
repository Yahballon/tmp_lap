// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace jukebox {
struct Theme {
 uint32_t accent=0xffc735;
 uint32_t secondary=0x173f37;
 uint32_t text=0xffffff;
};
struct Track {
 std::string name;
 std::string path;
 uint64_t size=0;
};
struct Album {
 std::string name;
 std::string path;
 std::string cover;
 std::string background;
 std::vector<Track> tracks;
};
struct Artist {
 std::string name;
 std::string path;
 std::string logo;
 std::string background;
 Theme theme;
 std::vector<Album> albums;
};
struct Library {
 std::vector<Artist> artists;
 size_t trackCount=0;
 std::string report;
};
bool is_music(const std::string& filename);
Theme read_theme(const std::string& artistFolder);
Theme parse_theme_text(const std::string& data);
Library scan(const std::vector<std::string>& roots);
void merge(Library& target,Library&& other);
std::vector<std::string> music_roots();
std::string album_background(const Artist& artist,const Album& album);
} // namespace jukebox
