// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/library_cache.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
int main(){
 const std::string path="/tmp/psjukebox-library-cache-test.bin";
 jukebox::Library source;
 jukebox::Artist a;a.name="Tupac";a.path="/data/PSJukebox/Music/Tupac";
 a.logo="jbox://0/Tupac/logo.png";a.background="jbox://0/Tupac/bg.png";
 a.theme.accent=0xeedd33;a.theme.secondary=0x112233;a.theme.text=0xffffff;
 jukebox::Album al;al.name="Killuminati";al.path="/data/PSJukebox/AlbumCache/abc/Music/Tupac/Killuminati";
 al.cover=al.path+"/cover.jpg";al.background=al.path+"/background.png";
 al.tracks.push_back({"01 Hail Mary",al.path+"/01 Hail Mary.mp3",3576210});
 al.tracks.push_back({"02 Bomb First",al.path+"/02 Bomb First.flac",7543210});
 a.albums.push_back(al);
 jukebox::Artist b;b.name="Wu Tang";b.path="smbj://Wu Tang";
 jukebox::Album newAlbum;newAlbum.name="Forever";newAlbum.path="smbj://Wu Tang/Forever";
 newAlbum.tracks.push_back({"Track","smbj://Wu Tang/Forever/track.mp3",13000});
 b.albums.push_back(newAlbum);
 source.artists={a,b};source.trackCount=3;
 std::string error;
 assert(jukebox::save_library_cache(path,source,error));
 jukebox::Library recovered;
 assert(jukebox::load_library_cache(path,recovered,error));
 assert(recovered.artists.size()==2&&recovered.trackCount==3);
 assert(recovered.artists[0].theme.accent==0xeedd33);
 assert(recovered.artists[0].albums[0].cover==al.cover);
 assert(recovered.artists[0].albums[0].tracks[1].size==7543210);
 assert(recovered.artists[1].albums[0].tracks[0].path.find("smbj://")==0);
 FILE* f=fopen(path.c_str(),"r+b");assert(f);
 assert(fseek(f,25,SEEK_SET)==0);assert(fputc('X',f)!=EOF);fclose(f);
 jukebox::Library wrong;
 assert(!jukebox::load_library_cache(path,wrong,error)); // CRC mismatch
 assert(wrong.artists.empty());
 assert(jukebox::save_library_cache(path,source,error));
 f=fopen(path.c_str(),"wb");assert(f);assert(fwrite("PSJBLIB3",1,8,f)==8);fclose(f);
 assert(!jukebox::load_library_cache(path,wrong,error)); // truncated cache
 // A bad next snapshot must not replace the previous valid one.
 assert(jukebox::save_library_cache(path,source,error));
 jukebox::Library oversize=source;
 oversize.artists[0].name.assign(5000,'Q');
 assert(!jukebox::save_library_cache(path,oversize,error));
 jukebox::Library stillGood;
 assert(jukebox::load_library_cache(path,stillGood,error));
 assert(stillGood.trackCount==3);
 remove(path.c_str());
 puts("PASS fast library cache roundtrip, album artwork, corruption/truncation, bounded atomic writes");
}
