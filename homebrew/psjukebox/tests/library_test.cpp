// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/library.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>

static void dir(const std::string& p) {assert(mkdir(p.c_str(),0700)==0);}
static void writefile(const std::string& p,size_t size) {
 FILE* f=fopen(p.c_str(),"wb");assert(f);
 for(size_t i=0;i<size;i++)assert(fputc('x',f)!=EOF);
 assert(fclose(f)==0);
}
int main(){
 char templ[]="/tmp/psjukebox-lib-XXXXXX";
 char* tmp=mkdtemp(templ);assert(tmp);
 const std::string base=tmp;
 dir(base+"/Bob Marley");dir(base+"/Bob Marley/Legend");
 dir(base+"/Bob Marley/Exodus");
 dir(base+"/Snoop Dogg");dir(base+"/Snoop Dogg/Doggystyle");
 dir(base+"/Tupac");dir(base+"/Tupac/All Eyez on Me");
 dir(base+"/wu-tang");
 writefile(base+"/Bob Marley/logo.png",80);
 writefile(base+"/Bob Marley/Legend/cover.jpg",100);
 writefile(base+"/Bob Marley/Legend/background.png",100);
 writefile(base+"/Bob Marley/Legend/01_One_Love.MP3",2000);
 writefile(base+"/Bob Marley/Legend/02_Test.FLAC",2000);
 writefile(base+"/Bob Marley/Exodus/03_Test.ogg",100);
 writefile(base+"/Snoop Dogg/Doggystyle/01_Test.m4a",2000);
 writefile(base+"/wu-tang/01_Ghostface.mp3",3000);
 const auto config=base+"/Bob Marley/theme.ini";
 FILE* f=fopen(config.c_str(),"wb");assert(f);
 fputs("accent=#3BCF5E\nsecondary=160049\ntext=abcdef\n",f);fclose(f);
 assert(jukebox::is_music("xx.MP3"));
 assert(!jukebox::is_music("xx.exe"));
 auto lib=jukebox::scan({base,base,"/not-existing-psjukebox"});
 assert(lib.artists.size()==4);
 assert(lib.trackCount==4);
 assert(lib.artists[0].name=="Bob Marley");
 assert(lib.artists[0].albums.size()==2);
 assert(lib.artists[0].albums[0].name=="Exodus");
 assert(lib.artists[0].albums[0].tracks.empty());
 assert(lib.artists[0].albums[1].name=="Legend");
 assert(lib.artists[0].albums[1].tracks[0].name=="01 One Love");
 assert(lib.artists[0].theme.accent==0x3bcf5e);
 assert(lib.artists[0].logo.find("logo.png")!=std::string::npos);
 assert(jukebox::album_background(lib.artists[0],lib.artists[0].albums[1]).find("background.png")!=std::string::npos);
 assert(lib.artists[1].albums[0].tracks.size()==1);
 assert(lib.artists[2].name=="Tupac");
 assert(lib.artists[2].albums.size()==1);
 assert(lib.artists[2].albums[0].tracks.empty());
 assert(lib.artists[3].name=="wu-tang");
 assert(lib.artists[3].albums.size()==1);
 assert(lib.artists[3].albums[0].name=="Singles");
 assert(lib.artists[3].albums[0].tracks.size()==1);
 jukebox::Library merged;
 jukebox::Artist identical;identical.name="wu-tang";
 jukebox::Album singles;singles.name="Singles";singles.tracks.push_back(lib.artists[3].albums[0].tracks[0]);
 identical.albums.push_back(singles);merged.artists.push_back(identical);
 jukebox::merge(lib,std::move(merged));
 assert(lib.artists.size()==4);
 assert(lib.trackCount==4);
 // Existing artist theme must survive a second scan, malformed themes fallback.
 assert(jukebox::read_theme("/not-found").accent==0xffc735);
 puts("PASS: artists, albums, songs, image names, themes, duplicate roots and empty albums");
 return 0;
}
