// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library.hpp"
#include <cstddef>
namespace jukebox {
struct SongIndex {int artist=0,album=0,track=0;};
// R2/L2 act on the currently playing song, not whichever album the user
// happens to be browsing. Album traversal wraps within the same artist,
// skipping empty albums. On crossing an album: R2 -> its first playable song,
// L2 -> its last playable song. Only valid indices are returned.
inline bool step_song(const Library& music,SongIndex current,int direction,
                      SongIndex& target){
 if((direction!=1&&direction!=-1)||current.artist<0||
    (size_t)current.artist>=music.artists.size())return false;
 const auto& albums=music.artists[(size_t)current.artist].albums;
 if(current.album<0||(size_t)current.album>=albums.size()||albums.empty())return false;
 const auto& tracks=albums[(size_t)current.album].tracks;
 if(tracks.empty()||current.track<0||(size_t)current.track>=tracks.size())return false;
 const int next=current.track+direction;
 if(next>=0&&(size_t)next<tracks.size()){
  target={current.artist,current.album,next};
  return true;
 }
 const int count=(int)albums.size();
 for(int offset=1;offset<=count;offset++){
  const int album=(current.album+direction*offset%count+count)%count;
  const auto& candidates=albums[(size_t)album].tracks;
  if(candidates.empty())continue;
  target={current.artist,album,direction==1?0:(int)candidates.size()-1};
  return true;
 }
 return false;
}
// Starting playback via a trigger without any active song starts at the
// currently selected album (first track for R2, last track for L2).
// Empty selected albums are skipped in the requested direction.
inline bool start_song(const Library& music,int artist,int album,int direction,
                       SongIndex& target){
 if((direction!=1&&direction!=-1)||artist<0||
    (size_t)artist>=music.artists.size())return false;
 const auto& albums=music.artists[(size_t)artist].albums;
 if(albums.empty()||album<0||(size_t)album>=albums.size())return false;
 for(int offset=0;offset<(int)albums.size();offset++){
  const int index=(album+direction*offset%(int)albums.size()+
                   (int)albums.size())%(int)albums.size();
  const auto& songs=albums[(size_t)index].tracks;
  if(songs.empty())continue;
  target={artist,index,direction==1?0:(int)songs.size()-1};
  return true;
 }
 return false;
}
}
