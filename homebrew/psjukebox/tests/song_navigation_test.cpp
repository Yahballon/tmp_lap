// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/song_navigation.hpp"
#include <cassert>
#include <cstdio>
namespace {
jukebox::Album album(int n){
 jukebox::Album a;
 for(int i=0;i<n;i++)a.tracks.push_back({"Track","",""});
 return a;
}
}
int main(){
 jukebox::Library library;
 jukebox::Artist a;
 a.albums={album(3),album(0),album(2)};
 jukebox::Artist b;b.albums={album(1)};
 jukebox::Artist c;c.albums={album(0)};
 library.artists={a,b,c};
 jukebox::SongIndex out;
 auto step=[&](jukebox::SongIndex from,int dir,int aa,int al,int track){
  assert(jukebox::step_song(library,from,dir,out));
  assert(out.artist==aa&&out.album==al&&out.track==track);
 };
 step({0,0,0},+1,0,0,1);         // within album
 step({0,0,1},+1,0,0,2);
 step({0,0,2},+1,0,2,0);         // skip empty middle album
 step({0,2,0},-1,0,0,2);         // previous album -> last track
 step({0,2,1},+1,0,0,0);         // last album -> first album cyclic
 step({0,0,0},-1,0,2,1);         // first album -> last album cyclic
 step({1,0,0},+1,1,0,0);         // single song stays in its artist
 step({1,0,0},-1,1,0,0);
 assert(!jukebox::step_song(library,{2,0,0},1,out)); // no playable songs
 assert(!jukebox::step_song(library,{0,5,0},1,out)); // invalid album
 assert(!jukebox::step_song(library,{0,0,99},1,out));// invalid track
 assert(!jukebox::step_song(library,{0,0,0},0,out)); // only +/- 1
 assert(jukebox::start_song(library,0,0,1,out)&&out.album==0&&out.track==0);
 assert(jukebox::start_song(library,0,0,-1,out)&&out.album==0&&out.track==2);
 assert(jukebox::start_song(library,0,1,1,out)&&out.album==2&&out.track==0);
 assert(jukebox::start_song(library,0,1,-1,out)&&out.album==0&&out.track==2);
 assert(!jukebox::start_song(library,2,0,1,out));
 puts("PASS R2/L2 track traversal across nonempty albums, artist isolation, wrap, empty library");
}
