// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/remote_bridge.hpp"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <sys/stat.h>
int main() {
 auto found=jbx::scan();
 assert(found.helperRunning);
 assert(found.library.artists.size()==1);
 auto& artist=found.library.artists[0];
 assert(artist.name=="Bob Marley");
 assert(artist.theme.accent==0x44cc77);
 assert(artist.albums.size()==2);
 assert(found.library.trackCount==2);
 const jukebox::Album* album=nullptr;
 for(const auto& a:artist.albums)if(a.name=="Legend")album=&a;
 assert(album&&album->tracks.size()==1);
 assert(!album->cover.empty()&&!album->background.empty());
 std::string bytes,error;
 assert(jbx::image_bytes(artist.logo,bytes,error)&&bytes.size()==264);
 std::atomic<bool> cancelled{false};
 std::string local;
 assert(jbx::stage_track(album->tracks[0].path,album->tracks[0].size,cancelled,local,error));
 struct stat st{};
 assert(stat(local.c_str(),&st)==0);
 assert((uint64_t)st.st_size==album->tracks[0].size);
 std::puts("PASS PSJukebox USB music helper integration");
}
